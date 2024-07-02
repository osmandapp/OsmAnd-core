#include "GeoTileRasterizer_P.h"
#include "GeoTileRasterizer.h"

#include <cassert>

#include "ignore_warnings_on_external_includes.h"
#include <gdal_priv.h>
#include <cpl_vsi.h>
#include <gdal_utils.h>
#include <gdal_alg.h>
#include <ogr_api.h>
#include <ogrsf_frmts.h>
#include <SkCanvas.h>
#include <SkPath.h>
#include "restore_internal_warnings.h"

#include "Logging.h"
#include "Utilities.h"
#include "SkiaUtilities.h"
#include "GlobalMercator.h"
#include "WeatherTileResourceProvider.h"

static QMutex _geoDataMutex;

OsmAnd::GeoTileRasterizer_P::GeoTileRasterizer_P(
    GeoTileRasterizer* const owner_)
    : owner(owner_)
{
}

OsmAnd::GeoTileRasterizer_P::~GeoTileRasterizer_P()
{
}

QString OsmAnd::GeoTileRasterizer_P::doubleToStr(const double value)
{
    return QString::number(value, 'f', 12);
}

QHash<OsmAnd::BandIndex, sk_sp<const SkImage>> OsmAnd::GeoTileRasterizer_P::rasterize(
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    QHash<BandIndex, QByteArray> outEncImgData;
    return rasterize(outEncImgData, false, pOutMetric, queryController);
}

QHash<OsmAnd::BandIndex, sk_sp<const SkImage>> OsmAnd::GeoTileRasterizer_P::rasterize(
    QHash<BandIndex, QByteArray>& outEncImgData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    return rasterize(outEncImgData, true, pOutMetric, queryController);
}

QHash<OsmAnd::BandIndex, sk_sp<const SkImage>> OsmAnd::GeoTileRasterizer_P::rasterize(
    QHash<BandIndex, QByteArray>& outEncImgData,
    bool fillEncImgData /*= false*/,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    if (pOutMetric)
        pOutMetric->reset();
    
    if (owner->geoTileData.length() == 0)
        return QHash<BandIndex, sk_sp<const SkImage>>();
    
    TileId tileId = owner->tileId;
    ZoomLevel zoom = owner->zoom;

    GlobalMercator mercator;
        
    double latExtent = zoom == ZoomLevel7 ? 0.5 : 3.0;
    double lonExtent = latExtent;
    auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);
    auto tlLatLon = Utilities::convert31ToLatLon(tileBBox31.topLeft);
    auto brLatLon = Utilities::convert31ToLatLon(tileBBox31.bottomRight);
    /*
    auto tlExtLatLon = LatLon(Utilities::normalizeLatitude(tlLatLon.latitude + latExtent),
                              Utilities::normalizeLongitude(tlLatLon.longitude - lonExtent));
    auto brExtLatLon = LatLon(Utilities::normalizeLatitude(brLatLon.latitude - latExtent),
                              Utilities::normalizeLongitude(brLatLon.longitude + lonExtent));
     */
    auto tlExtLatLon = LatLon(tlLatLon.latitude + latExtent, tlLatLon.longitude - lonExtent);
    auto brExtLatLon = LatLon(brLatLon.latitude - latExtent, brLatLon.longitude + lonExtent);

    //tlExtLatLon = LatLon(tlExtLatLon.latitude - fmod(tlExtLatLon.latitude, 0.25) + 0.125, tlExtLatLon.longitude - fmod(tlExtLatLon.longitude, 0.25) - 0.125);
    //brExtLatLon = LatLon(brExtLatLon.latitude - fmod(brExtLatLon.latitude, 0.25) + 0.125, brExtLatLon.longitude - fmod(brExtLatLon.longitude, 0.25) + 0.125);
    
    //QMutexLocker scopedLocker(&_dataMutex);
    
    // Map data to VSI memory
    auto geoTileData(owner->geoTileData);
    const auto filename = QString::asprintf("/vsimem/geoTile@%p_%dx%d@%d_raster", this, tileId.x, tileId.y, zoom);
    const auto file = VSIFileFromMemBuffer(
        qPrintable(filename),
        reinterpret_cast<GByte*>(geoTileData.data()),
        geoTileData.length(),
        FALSE
    );
    if (!file)
    {
        LogPrintf(LogSeverityLevel::Error,
                  "Failed to map geotiff %dx%d@%d",
                  tileId.x,
                  tileId.y,
                  zoom);
        return QHash<BandIndex, sk_sp<const SkImage>>();
    }
    VSIFCloseL(file);
    
    if (queryController && queryController->isAborted())
        return QHash<BandIndex, sk_sp<const SkImage>>();

    auto projSearchPath = owner->projSearchPath.toUtf8();
    const char* projPaths[] = { projSearchPath.constData(), NULL };
    OSRSetPROJSearchPaths(projPaths);

    // Decode data as GeoTIFF
    std::shared_ptr<void> hSourceDS(
        GDALOpen(qPrintable(filename), GA_ReadOnly),
            [filename]
            (auto hSourceDS)
            {
                GDALClose(hSourceDS);
                VSIUnlink(qPrintable(filename));
            }
        );
    if (!hSourceDS)
    {
        LogPrintf(LogSeverityLevel::Error,
                  "Failed to open %dx%d@%d as GeoTIFF",
                  tileId.x,
                  tileId.y,
                  zoom);
        return QHash<BandIndex, sk_sp<const SkImage>>();
    }
        
    if (queryController && queryController->isAborted())
        return QHash<BandIndex, sk_sp<const SkImage>>();

    QHash<BandIndex, sk_sp<const SkImage>> bandImages;
    for (auto band : owner->bands)
    {
        if (!owner->bandSettings.contains(band)
            && band != static_cast<BandIndex>(WeatherBand::WindWestToEast)
            && band != static_cast<BandIndex>(WeatherBand::WindSouthToNorth))
            continue;

        auto bandIndexStr = QString::number(band).toLatin1();

        auto tlExtLonStr = doubleToStr(tlExtLatLon.longitude).toLatin1();
        auto tlExtLatStr = doubleToStr(tlExtLatLon.latitude).toLatin1();
        auto brExtLonStr = doubleToStr(brExtLatLon.longitude).toLatin1();
        auto brExtLatStr = doubleToStr(brExtLatLon.latitude).toLatin1();

        const auto cropFilename = QString::asprintf("/vsimem/geoTile@%p_%d_%dx%d@%d_raster_crop",
                                                    this, band, tileId.x, tileId.y, zoom);
        const char* cropArgs[] = {
            "-b", bandIndexStr.constData(),
            "-projwin",
            tlExtLonStr.constData(),
            tlExtLatStr.constData(),
            brExtLonStr.constData(),
            brExtLatStr.constData(),
            NULL };
    
        GDALTranslateOptions* psCropOptions = GDALTranslateOptionsNew((char**)cropArgs, NULL);
        if (!psCropOptions)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to create options for crop GeoTIFF %d:%dx%d@%d to %s / %s.",
                band,
                tileId.x,
                tileId.y,
                zoom,
                qPrintable(tlExtLatLon.toQString()),
                qPrintable(brExtLatLon.toQString()));
            return QHash<BandIndex, sk_sp<const SkImage>>();
        }
        int bCropError = FALSE;
        std::shared_ptr<void> hCroppedDS(
            GDALTranslate(
                qPrintable(cropFilename),
                hSourceDS.get(),
                psCropOptions,
                &bCropError),
            [cropFilename]
            (auto hCroppedDS)
            {
                GDALClose(hCroppedDS);
                VSIUnlink(qPrintable(cropFilename));
            }
        );
        GDALTranslateOptionsFree(psCropOptions);
        if (!hCroppedDS || bCropError)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to crop GeoTIFF %d:%dx%d@%d to %s / %s. Error: %d",
                band,
                tileId.x,
                tileId.y,
                zoom,
                qPrintable(tlExtLatLon.toQString()),
                qPrintable(brExtLatLon.toQString()),
                bCropError);
            return QHash<BandIndex, sk_sp<const SkImage>>();
        }
        
        if (queryController && queryController->isAborted())
            return QHash<BandIndex, sk_sp<const SkImage>>();

        auto interBoundsMin = mercator.latLonToMeters(tlExtLatLon);
        auto interBoundsMax = mercator.latLonToMeters(brExtLatLon);

        auto xRes = mercator.resolution(zoom);
        auto yRes = mercator.resolution(zoom);

        auto interBoundsMinXStr = doubleToStr(interBoundsMin.x).toLatin1();
        auto interBoundsMinYStr = doubleToStr(interBoundsMax.y).toLatin1();
        auto interBoundsMaxXStr = doubleToStr(interBoundsMax.x).toLatin1();
        auto interBoundsMaxYStr = doubleToStr(interBoundsMin.y).toLatin1();
        auto interBoundsXResStr = doubleToStr(xRes).toLatin1();
        auto interBoundsYResStr = doubleToStr(yRes).toLatin1();

        const auto interpolateFilename = QString::asprintf("/vsimem/geoTile@%p_%d_%dx%d@%d_raster_interpolate",
                                                           this, band, tileId.x, tileId.y, zoom);
        //gdalwarp -of GTiff -co "SPARSE_OK=TRUE" -t_srs epsg:3857 -r cubic -multi -ts 2048 2048 "20211217_0000.gt.tiff" "20211217_0000.gt_M.tiff" -overwrite
        const char* interpolateArgs[] = {
            "-of", "GTiff",
            "-co", "SPARSE_OK=TRUE",
            //"-t_srs", "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs",
            "-t_srs", "epsg:3857",
            "-r", "cubic",
            "-multi",
            //"-wo", "NUM_THREADS=ALL_CPUS",
            "-wo", "NUM_THREADS=2",
            "-te",
            interBoundsMinXStr.constData(),
            interBoundsMinYStr.constData(),
            interBoundsMaxXStr.constData(),
            interBoundsMaxYStr.constData(),
            "-tr",
            interBoundsXResStr.constData(),
            interBoundsYResStr.constData(),
            NULL };
        
        GDALWarpAppOptions* psInterpolateOptions = GDALWarpAppOptionsNew((char**)interpolateArgs, NULL);
        if (!psInterpolateOptions)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to create options for interpolate GeoTIFF %d:%dx%d@%d. ProjDB path: %s. Error: %s",
                 band, tileId.x, tileId.y, zoom, qPrintable(projSearchPath), CPLGetLastErrorMsg());
            return QHash<BandIndex, sk_sp<const SkImage>>();
        }
        int bWarpError = FALSE;
        auto hCropDSPtr = hCroppedDS.get();
        std::shared_ptr<void> hInterpolatedDS(
            GDALWarp(
                qPrintable(interpolateFilename),
                NULL,
                1,
                &hCropDSPtr,
                psInterpolateOptions,
                &bWarpError),
            [interpolateFilename]
            (auto hInterpolateDS)
            {
                GDALClose(hInterpolateDS);
                VSIUnlink(qPrintable(interpolateFilename));
            }
        );
        GDALWarpAppOptionsFree(psInterpolateOptions);
        //hCroppedDS = nullptr;
        if (!hInterpolatedDS || bWarpError)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to interpolate GeoTIFF %d:%dx%d@%d. Error: %d",
                band, tileId.x, tileId.y, zoom, bWarpError);
            return QHash<BandIndex, sk_sp<const SkImage>>();
        }

        if (queryController && queryController->isAborted())
            return QHash<BandIndex, sk_sp<const SkImage>>();

        auto colorProfilePath = band == static_cast<BandIndex>(WeatherBand::WindWestToEast)
            || band == static_cast<BandIndex>(WeatherBand::WindSouthToNorth)
            ? WeatherTileResourceProvider::_windColorProfile
            : owner->bandSettings[band]->colorProfilePath;
        
        const auto colorizeFilename = QString::asprintf("/vsimem/geoTile@%p_%d_%dx%d@%d_raster_colorize",
                                                        this, band, tileId.x, tileId.y, zoom);
        const char* colorizeArgs[] = {
            //"-b", "1",
            "-alpha",
            NULL };
        GDALDEMProcessingOptions *psColorizeOptions = GDALDEMProcessingOptionsNew(const_cast<char **>(colorizeArgs), NULL);
        if (!psColorizeOptions)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to to create options for colorize GeoTIFF %d:%dx%d@%d: %s",
                band, tileId.x, tileId.y, zoom, CPLGetLastErrorMsg());
            return QHash<BandIndex, sk_sp<const SkImage>>();
        }

        int bDEMError = FALSE;
        std::shared_ptr<void> hColorizedDS(
            GDALDEMProcessing(
                qPrintable(colorizeFilename),
                hInterpolatedDS.get(),
                "color-relief",
                qPrintable(colorProfilePath),
                psColorizeOptions,
                &bDEMError),
            [colorizeFilename]
            (auto hColorizedDS)
            {
                GDALClose(hColorizedDS);
                VSIUnlink(qPrintable(colorizeFilename));
            }
        );
        GDALDEMProcessingOptionsFree(psColorizeOptions);
        hInterpolatedDS = nullptr;
        if (!hColorizedDS || bDEMError)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to colorize GeoTIFF %dx%d@%d band=%d. Error: %d", tileId.x, tileId.y, zoom, band, bDEMError);
            continue;
        }
        
        auto interBoundsMinOrig = mercator.latLonToMeters(tlLatLon);
        auto tileMinPix = mercator.metersToPixels(interBoundsMinOrig.x, interBoundsMinOrig.y, zoom);
        auto tileMinExtPix = mercator.metersToPixels(interBoundsMin.x, interBoundsMin.y, zoom);
        
        int tileX = tileId.x;
        int tileY = tileId.y;
            
        const auto tileSize = owner->tileSize;

        // Get overlapped tile data to predict its move with the wind
        const auto overlap = tileSize / 4;
        const auto sideOverlap = overlap / 2;

        auto origin = PointD(
            abs(tileMinPix.x - tileMinExtPix.x) - sideOverlap,
            abs(tileMinPix.y - tileMinExtPix.y) - sideOverlap);
        
        auto originShiftXStr = doubleToStr(origin.x).toLatin1();
        auto originShiftYStr = doubleToStr(origin.y).toLatin1();
        auto tileSizeStr = QString::number(tileSize + overlap).toLatin1();

        const auto tileFilename = QString::asprintf("/vsimem/geoTile%p_%d_%dx%dx%d_raster_tile",
                                                    this, band, (int) zoom, tileX, tileY);
        const char* args[] = {
            "-of", "png",
            "-srcwin",
            originShiftXStr.constData(),
            originShiftYStr.constData(),
            tileSizeStr.constData(),
            tileSizeStr.constData(),
            NULL };
        
        GDALTranslateOptions* psOptions = GDALTranslateOptionsNew((char**)args, NULL);
        if (!psOptions)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to create options for result tile %dx%dx%d band=%d error=%s",
                tileX, tileX, zoom, band, CPLGetLastErrorMsg());
            continue;
        }
        int bTranslateError = FALSE;
        std::shared_ptr<void> hResultDS(
            GDALTranslate(
                qPrintable(tileFilename),
                hColorizedDS.get(),
                psOptions,
                &bTranslateError),
            [tileFilename]
            (auto hResultDS)
            {
                GDALClose(hResultDS);
                VSIUnlink(qPrintable(tileFilename));
            }
        );
        GDALTranslateOptionsFree(psOptions);
        if (!hResultDS || bTranslateError)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to create result weather tile %dx%dx%d band=%d. Error: %d",
                 tileX, tileX, zoom, band, bTranslateError);
            continue;
        }
        
        VSIStatBufL sStatBuf;
        if (VSIStatL(qPrintable(tileFilename), &sStatBuf) == 0)
        {
            auto nBufLength = sStatBuf.st_size;
            auto fpDB = VSIFOpenL(qPrintable(tileFilename), "r");
            char *pszDBData = static_cast<char *>(CPLCalloc(1, nBufLength + 1));
            if (VSIFReadL(pszDBData, 1, nBufLength, fpDB) == static_cast<size_t>(nBufLength))
            {
                if (fillEncImgData)
                {
                    QByteArray data(pszDBData, (int)nBufLength);
                    outEncImgData.insert(band, data);
                }
                auto image = SkImage::MakeFromEncoded(SkData::MakeWithProc(
                    pszDBData,
                    nBufLength,
                    [](const void* ptr, void* context) {
                        CPLFree(context);
                    },
                    pszDBData
                ));
                if (image)
                    bandImages.insert(band, image);
            }
            CPL_IGNORE_RET_VAL(VSIFCloseL(fpDB));
        }

        hColorizedDS = nullptr;
    }

    return bandImages;
}

QHash<OsmAnd::BandIndex, sk_sp<const SkImage>> OsmAnd::GeoTileRasterizer_P::rasterizeContours(
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    QHash<BandIndex, QByteArray> outEncImgData;
    return rasterizeContours(outEncImgData, false, pOutMetric, queryController);
}

QHash<OsmAnd::BandIndex, sk_sp<const SkImage>> OsmAnd::GeoTileRasterizer_P::rasterizeContours(
    QHash<BandIndex, QByteArray>& outEncImgData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    return rasterizeContours(outEncImgData, true, pOutMetric, queryController);
}

QHash<OsmAnd::BandIndex, sk_sp<const SkImage>> OsmAnd::GeoTileRasterizer_P::rasterizeContours(
    QHash<BandIndex, QByteArray>& outEncImgData,
    bool fillEncImgData /*= false*/,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    auto contourMap = evaluateContours(pOutMetric, queryController);
    if (contourMap.empty() || (queryController && queryController->isAborted()))
        return QHash<BandIndex, sk_sp<const SkImage>>();
        
    QHash<BandIndex, sk_sp<const SkImage>> bandImages;
    for (auto band : owner->bands)
    {
        if (!contourMap.contains(band) || (queryController && queryController->isAborted()))
            return QHash<BandIndex, sk_sp<const SkImage>>();

        auto contours = contourMap[band];
        auto imageSize = owner->tileSize * owner->densityFactor;
        auto image = rasterizeBandContours(contours, owner->tileId, owner->zoom, imageSize, imageSize);
        if (image)
        {
            if (fillEncImgData)
            {
                const auto data = image->encodeToData(SkEncodedImageFormat::kPNG, 100);
                if (data)
                    outEncImgData.insert(band, QByteArray(reinterpret_cast<const char *>(data->bytes()), (int) data->size()));
            }
            bandImages.insert(band, image);
        }
    }

    return bandImages;
}

QHash<OsmAnd::BandIndex, QList<std::shared_ptr<OsmAnd::GeoContour>>> OsmAnd::GeoTileRasterizer_P::evaluateContours(
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    if (pOutMetric)
        pOutMetric->reset();
    
    if (owner->geoTileData.length() == 0)
        return QHash<BandIndex, QList<std::shared_ptr<GeoContour>>>();

    TileId tileId = owner->tileId;
    ZoomLevel zoom = owner->zoom;

    GlobalMercator mercator;
        
    double latExtent = 1.0;
    double lonExtent = 1.0;
    auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);
    auto tlLatLon = Utilities::convert31ToLatLon(tileBBox31.topLeft);
    auto brLatLon = Utilities::convert31ToLatLon(tileBBox31.bottomRight);

    auto tlExtLatLon = LatLon(tlLatLon.latitude + latExtent, qMax(tlLatLon.longitude - lonExtent, -179.5));
    auto brExtLatLon = LatLon(brLatLon.latitude - latExtent, qMin(brLatLon.longitude + lonExtent, 179.5));

    auto geoTileZoom = WeatherTileResourceProvider::getGeoTileZoom();
    
    int scaleCoef = (int) (100.0 * Utilities::getPowZoom(zoom - geoTileZoom));
    scaleCoef = scaleCoef > 6400 ? 6400 : scaleCoef;
    
    // Map data to VSI memory
    auto geoTileData(owner->geoTileData);
    const auto filename = QString::asprintf("/vsimem/geoTile@%p_%dx%d@%d_contour", this, tileId.x, tileId.y, zoom);
    const auto file = VSIFileFromMemBuffer(
        qPrintable(filename),
        reinterpret_cast<GByte*>(geoTileData.data()),
        geoTileData.length(),
        FALSE
    );
    if (!file)
    {
        LogPrintf(LogSeverityLevel::Error,
                  "Failed to map geotiff %dx%d@%d",
                  tileId.x,
                  tileId.y,
                  zoom);
        return QHash<BandIndex, QList<std::shared_ptr<GeoContour>>>();
    }
    VSIFCloseL(file);
    
    if (queryController && queryController->isAborted())
        return QHash<BandIndex, QList<std::shared_ptr<GeoContour>>>();

    auto projSearchPath = owner->projSearchPath.toUtf8();
    const char* projPaths[] = { projSearchPath.constData(), NULL };
    OSRSetPROJSearchPaths(projPaths);

    // Decode data as GeoTIFF
    std::shared_ptr<void> hSourceDS(
        GDALOpen(qPrintable(filename), GA_ReadOnly),
            [filename]
            (auto hSourceDS)
            {
                GDALClose(hSourceDS);
                VSIUnlink(qPrintable(filename));
            }
        );
    if (!hSourceDS)
    {
        LogPrintf(LogSeverityLevel::Error,
                  "Failed to open %dx%d@%d as GeoTIFF",
                  tileId.x,
                  tileId.y,
                  zoom);
        return QHash<BandIndex, QList<std::shared_ptr<GeoContour>>>();
    }
        
    if (queryController && queryController->isAborted())
        return QHash<BandIndex, QList<std::shared_ptr<GeoContour>>>();

    QHash<BandIndex, QList<std::shared_ptr<GeoContour>>> bandContours;
    for (auto band : owner->bands)
    {
        if (!owner->bandSettings.contains(band))
            continue;

        auto bandIndexStr = QString::number(band).toLatin1();
        
        auto tlExtLonStr = doubleToStr(tlExtLatLon.longitude).toLatin1();
        auto tlExtLatStr = doubleToStr(tlExtLatLon.latitude).toLatin1();
        auto brExtLonStr = doubleToStr(brExtLatLon.longitude).toLatin1();
        auto brExtLatStr = doubleToStr(brExtLatLon.latitude).toLatin1();

        auto scaleCoefStr = QString::asprintf("%d%%", scaleCoef).toLatin1();
        
        const auto cropFilename = QString::asprintf("/vsimem/geoTile@%p_%d_%dx%d@%d_contour_crop",
                                                    this, band, tileId.x, tileId.y, zoom);
        const char* cropArgs[] = {
            "-b", bandIndexStr.constData(),
            "-projwin",
            tlExtLonStr.constData(),
            tlExtLatStr.constData(),
            brExtLonStr.constData(),
            brExtLatStr.constData(),
            "-r", "cubic",
            "-outsize",
            scaleCoefStr.constData(),
            scaleCoefStr.constData(),
            NULL };
    
        GDALTranslateOptions* psCropOptions = GDALTranslateOptionsNew((char**)cropArgs, NULL);
        if (!psCropOptions)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to create options for crop GeoTIFF %dx%d@%d to %s / %s. Error: %s",
                tileId.x,
                tileId.y,
                zoom,
                qPrintable(tlExtLatLon.toQString()),
                qPrintable(brExtLatLon.toQString()),
                CPLGetLastErrorMsg());
            return QHash<BandIndex, QList<std::shared_ptr<GeoContour>>>();
        }
        int bCropError = FALSE;
        std::shared_ptr<void> hCroppedDS(
            GDALTranslate(
                qPrintable(cropFilename),
                hSourceDS.get(),
                psCropOptions,
                &bCropError),
            [cropFilename]
            (auto hCroppedDS)
            {
                GDALClose(hCroppedDS);
                VSIUnlink(qPrintable(cropFilename));
            }
        );
        GDALTranslateOptionsFree(psCropOptions);
        if (!hCroppedDS || bCropError)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to crop GeoTIFF %dx%d@%d to %s / %s. Error: %d",
                tileId.x,
                tileId.y,
                zoom,
                qPrintable(tlExtLatLon.toQString()),
                qPrintable(brExtLatLon.toQString()),
                bCropError);
            return QHash<BandIndex, QList<std::shared_ptr<GeoContour>>>();
        }
        
        if (queryController && queryController->isAborted())
            return QHash<BandIndex, QList<std::shared_ptr<GeoContour>>>();

        if (owner->bandSettings.contains(band))
        {
            auto levels = owner->bandSettings[band]->contourLevels;
            if (levels.contains(zoom))
            {
                auto contours = evaluateBandContours(hCroppedDS.get(), 1, levels[zoom], tileBBox31, zoom);
                if (!contours.empty())
                    bandContours.insert(band, contours);
            }
        }
    }

    return bandContours;
}

QList<std::shared_ptr<OsmAnd::GeoContour>> OsmAnd::GeoTileRasterizer_P::evaluateBandContours(
    GDALDatasetH hDataset,
    const OsmAnd::BandIndex band,
    QList<double>& levels,
    const AreaI tileBBox31,
    const ZoomLevel zoom)
{
    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(pszDriverName);
    if (!driver)
    {
        LogPrintf(LogSeverityLevel::Error, "%s driver not available", pszDriverName);
        return QList<std::shared_ptr<GeoContour>>();
    }

    auto rand = QRandomGenerator::global()->generate() % 1000;
    const auto fileName = QString::asprintf("/vsimem/shapeFile%p_%d_%d_%dx%d@%d_contour_%d", this, band, levels.length(), tileBBox31.left(), tileBBox31.top(), zoom, rand);
    std::shared_ptr<GDALDataset> spDataset(
        driver->Create(qPrintable(fileName), 0, 0, 0, GDT_Float32, NULL),
        [fileName]
        (auto spDataset)
        {
            GDALClose(spDataset);
            VSIUnlink(qPrintable(fileName));
        }
    );
    if (!spDataset)
    {
        LogPrintf(LogSeverityLevel::Error, "Creation of Gdal shape output file failed");
        return QList<std::shared_ptr<GeoContour>>();
    }

    OGRLayer *layer = spDataset->CreateLayer("ContoursLayer", NULL, wkbMultiLineString, NULL );
    if (!layer)
    {
        LogPrintf(LogSeverityLevel::Error, "Gdal layer creation failed %s", qPrintable(fileName));
        return QList<std::shared_ptr<GeoContour>>();
    }
    
    OGRFieldDefn valueField("Value", OFTReal);
    if (layer->CreateField(&valueField) != OGRERR_NONE)
    {
        LogPrintf(LogSeverityLevel::Error, "Gdal layer creation value field failed");
        return QList<std::shared_ptr<GeoContour>>();
    }
    
    auto hBand = GDALGetRasterBand(hDataset, band);

    QStringList levelsList;
    for (auto level : levels)
        levelsList << QString::number(level);
    
    char** options = nullptr;
//    options = CSLAppendPrintf(options, "LEVEL_INTERVAL=%f", 2.0);
    options = CSLAppendPrintf(options, "FIXED_LEVELS=%s", qPrintable(levelsList.join(QStringLiteral(","))));
//    options = CSLAppendPrintf(options, "ID_FIELD=%d", 0);
    options = CSLAppendPrintf(options, "ELEV_FIELD=%d", 0);
//    options = CSLAppendPrintf(options, "ELEV_FIELD_MIN=%d", 2);
//    options = CSLAppendPrintf(options, "ELEV_FIELD_MAX=%d", 3);
    options = CSLAppendPrintf(options, "POLYGONIZE=NO");

    auto err = GDALContourGenerateEx(hBand, layer, options, nullptr, NULL);
    CSLDestroy(options);
    
    if (err != CE_None)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to generate gdal layer contour lines: %s", CPLGetLastErrorMsg());
        return QList<std::shared_ptr<GeoContour>>();
    }
    
    OGRFeature *feature;
    OGRMultiLineString *multiLine = static_cast<OGRMultiLineString *>(
        OGRGeometryFactory::createGeometry(wkbMultiLineString));
    QList<double> values;
    layer->ResetReading();
    
    while ((feature = layer->GetNextFeature()) != nullptr)
    {
        OGRGeometry *geometry = feature->GetGeometryRef();
        if (geometry == nullptr)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Gdal LineString feature without a geometry.");
            OGRFeature::DestroyFeature(feature);
            continue;
        }
        OGRwkbGeometryType eType = wkbFlatten(geometry->getGeometryType());
        if (eType == wkbLineString)
        {
            if (multiLine->addGeometry(geometry) == OGRERR_NONE)
                values << feature->GetFieldAsDouble(0);
        }
        else if (eType == wkbMultiLineString)
        {
            auto mls = geometry->toMultiLineString();
            for (int iGeom = 0; iGeom < mls->getNumGeometries(); iGeom++)
            {
                if (multiLine->addGeometry(mls->getGeometryRef(iGeom)) == OGRERR_NONE)
                    values << feature->GetFieldAsDouble(0);
            }
        }
        OGRFeature::DestroyFeature(feature);
    }
    
    if (multiLine->getNumGeometries() == 0)
    {
        OGRGeometryFactory::destroyGeometry(multiLine);
        LogPrintf(LogSeverityLevel::Error,
            "Did not get any LineString features");
        return QList<std::shared_ptr<GeoContour>>();
    }

    QList<std::shared_ptr<GeoContour>> contours;
    for (int iGeom = 0; iGeom < multiLine->getNumGeometries(); iGeom++)
    {
        auto geom = multiLine->getGeometryRef(iGeom);
        auto count = geom->getNumPoints();
        if (count > 1)
        {
            QVector<PointI> points;
            for (int i = 0; i < count; i++)
            {
                double x = geom->getX(i);
                double y = geom->getY(i);
                points << Utilities::convertLatLonTo31(LatLon(y, x));
            }
            auto segments = calculateVisibleSegments(points, tileBBox31);
            for (auto& segment : constOf(segments))
                if (segment.length() > 1)
                    contours << std::make_shared<GeoContour>(values[iGeom], segment);
        }
    }
    
    OGRGeometryFactory::destroyGeometry(multiLine);
    
    return contours;
}

sk_sp<SkImage> OsmAnd::GeoTileRasterizer_P::rasterizeBandContours(
    const QList<std::shared_ptr<GeoContour>>& contourMap,
    const TileId tileId,
    const ZoomLevel zoom,
    const int width,
    const int height)
{
    if (contourMap.empty())
    {
        LogPrintf(LogSeverityLevel::Error, "No contours for rasterization");
        return nullptr;
    }
    
    SkBitmap target;
    if (!target.tryAllocPixels(SkImageInfo::MakeN32Premul(width, height)))
    {
        LogPrintf(LogSeverityLevel::Error,
                  "Failed to allocate SkBitmap %dx%d for contours rasterization", width, height);
        return nullptr;
    }

    target.eraseColor(SK_ColorTRANSPARENT);

    SkCanvas canvas(target);

    SkPaint linePaint;
    linePaint.setColor(OsmAnd::ColorARGB(0xFFFF0000).toSkColor());
    linePaint.setStroke(true);
    linePaint.setStrokeWidth(1.0);
    linePaint.setAntiAlias(true);
    //linePaint.setStrokeJoin(SkPaint::kRound_Join);
    //linePaint.setStrokeCap(SkPaint::kRound_Cap);
    
    SkPath linePath;
    
    auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);
    auto scaleDiv = Utilities::getScaleDivisor31ToPixel(PointI(width, height), zoom);
    for (const auto& contours : constOf(contourMap))
    {
        for (int i = 0; i < contours->points.length(); i++)
        {
            auto point31 = contours->points[i];
            auto x = (point31.x - tileBBox31.left()) / scaleDiv.x;
            auto y = (point31.y - tileBBox31.top()) / scaleDiv.y;
            
            if (i == 0)
                linePath.moveTo(x, y);
            else
                linePath.lineTo(x, y);
        }
    }
    if (linePath.countPoints() > 0)
        canvas.drawPath(linePath, linePaint);
    
    canvas.flush();
    return target.asImage();
}

QVector<QVector<OsmAnd::PointI>> OsmAnd::GeoTileRasterizer_P::calculateVisibleSegments(const QVector<PointI>& points, const AreaI bbox31) const
{
    QVector<QVector<PointI>> segments;
    bool segmentStarted = false;
    auto visibleArea = bbox31;
    PointI curr, drawFrom, drawTo, inter1, inter2;
    auto prev = drawFrom = points[0];
    int drawFromIdx = 0, drawToIdx = 0;
    QVector<PointI> segment;
    bool prevIn = bbox31.contains(prev);
    for (int i = 1; i < points.size(); i++)
    {
        curr = drawTo = points[i];
        drawToIdx = i;
        bool currIn = visibleArea.contains(curr);
        bool draw = false;
        if (prevIn && currIn)
        {
            draw = true;
        }
        else
        {
            if (Utilities::calculateIntersection(curr, prev, visibleArea, inter1))
            {
                draw = true;
                if (prevIn)
                {
                    drawTo = inter1;
                    drawToIdx = i;
                }
                else if (currIn)
                {
                    drawFrom = inter1;
                    drawFromIdx = i;
                    segmentStarted = false;
                }
                else if (Utilities::calculateIntersection(prev, curr, visibleArea, inter2))
                {
                    drawFrom = inter1;
                    drawTo = inter2;
                    drawFromIdx = drawToIdx;
                    drawToIdx = i;
                    segmentStarted = false;
                }
                else
                {
                    draw = false;
                }
            }
        }
        if (draw)
        {
            if (!segmentStarted)
            {
                if (!segment.empty())
                {
                    segments.push_back(segment);
                    segment = QVector<PointI>();
                }
                segment.push_back(drawFrom);
                segmentStarted = currIn;
            }
            if (segment.empty() || segment.back() != drawTo)
            {
                segment.push_back(drawTo);
            }
        }
        else
        {
            segmentStarted = false;
        }
        prevIn = currIn;
        prev = drawFrom = curr;
        drawFromIdx = i;
    }
    if (!segment.empty())
        segments.push_back(segment);

    return segments;
}

