#include "GeoTileRasterizer_P.h"
#include "GeoTileRasterizer.h"

#include <cassert>

#include "ignore_warnings_on_external_includes.h"
#include <gdal.h>
#include <gdal_priv.h>
#include <cpl_vsi.h>
#include <gdal_utils.h>
#include <SkCanvas.h>
#include "restore_internal_warnings.h"

#include "Logging.h"
#include "Utilities.h"
#include "SkiaUtilities.h"
#include "GlobalMercator.h"

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
        
    double latExtent = 1.0;
    double lonExtent = 1.0;
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
    const auto filename = QString::asprintf("/vsimem/geoTile@%p", this);
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
        if (!owner->colorProfiles.contains(band))
            continue;

        auto bandIndexStr = QString::number(band).toLatin1();

        auto tlExtLonStr = doubleToStr(tlExtLatLon.longitude).toLatin1();
        auto tlExtLatStr = doubleToStr(tlExtLatLon.latitude).toLatin1();
        auto brExtLonStr = doubleToStr(brExtLatLon.longitude).toLatin1();
        auto brExtLatStr = doubleToStr(brExtLatLon.latitude).toLatin1();

        const auto cropFilename = QString::asprintf("/vsimem/geoTile@%p_%d_%dx%d@%d_crop",
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
                "Failed to create options for crop GeoTIFF %dx%d@%d to %s / %s.",
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
                "Failed to crop GeoTIFF %dx%d@%d to %s / %s. Error: %d",
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

        const auto interpolateFilename = QString::asprintf("/vsimem/geoTile@%p_%d_%dx%d@%d_interpolate",
                                                           this, band, tileId.x, tileId.y, zoom);
        //gdalwarp -of GTiff -co "SPARSE_OK=TRUE" -t_srs epsg:3857 -r cubic -multi -ts 2048 2048 "20211217_0000.gt.tiff" "20211217_0000.gt_M.tiff" -overwrite
        const char* interpolateArgs[] = {
            "-of", "GTiff",
            "-co", "SPARSE_OK=TRUE",
            //"-t_srs", "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs",
            "-t_srs", "epsg:3857",
            "-r", "cubic",
            "-multi",
            "-wo", "NUM_THREADS=ALL_CPUS",
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
                "Failed to create options for interpolate GeoTIFF %dx%d@%d", tileId.x, tileId.y, zoom);
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
        hCroppedDS = nullptr;
        if (!hInterpolatedDS || bWarpError)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to interpolate GeoTIFF %dx%d@%d. Error: %d", tileId.x, tileId.y, zoom, bWarpError);
            return QHash<BandIndex, sk_sp<const SkImage>>();
        }

        if (queryController && queryController->isAborted())
            return QHash<BandIndex, sk_sp<const SkImage>>();

        auto colorProfilePath = owner->colorProfiles[band];
        const auto colorizeFilename = QString::asprintf("/vsimem/geoTile@%p_%d_%dx%d@%d_colorize",
                                                        this, band, tileId.x, tileId.y, zoom);
        const char* colorizeArgs[] = {
            //"-b", "1",
            "-alpha",
            NULL };
        GDALDEMProcessingOptions *psColorizeOptions = GDALDEMProcessingOptionsNew(const_cast<char **>(colorizeArgs), NULL);
        if (!psColorizeOptions)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to to create options for colorize GeoTIFF %dx%d@%d", tileId.x, tileId.y, zoom);
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
            
        auto origin = PointD(abs(tileMinPix.x - tileMinExtPix.x), abs(tileMinPix.y - tileMinExtPix.y));
        
        auto originShiftXStr = doubleToStr(origin.x).toLatin1();
        auto originShiftYStr = doubleToStr(origin.y).toLatin1();
        auto tileSizeStr = QString::number(owner->tileSize).toLatin1();

        const auto tileFilename = QString::asprintf("/vsimem/geoTile%p_%d_%dx%dx%d",
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
                "Failed to create options for result tile %dx%dx%d band=%d", tileX, tileX, zoom, band);
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
                "Failed to create result weather tile %dx%dx%d band=%d. Error: %d", tileX, tileX, zoom, band, bTranslateError);
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
                    QByteArray data(pszDBData, nBufLength);
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
