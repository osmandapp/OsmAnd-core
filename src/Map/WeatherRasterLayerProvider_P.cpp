#include "WeatherRasterLayerProvider_P.h"
#include "WeatherRasterLayerProvider.h"

#include <cassert>

#include "ignore_warnings_on_external_includes.h"
#include <gdal.h>
#include <gdal_priv.h>
#include <cpl_vsi.h>
#include <gdal_utils.h>
#include <SkCanvas.h>
#include "restore_internal_warnings.h"

#include <QDateTime>

#include "Logging.h"
#include "MapDataProviderHelpers.h"
#include "Utilities.h"
#include "SkiaUtilities.h"
#include "GlobalMercator.h"
#include "ArchiveReader.h"
#include "ElapsedTimer.h"

static QMutex _geoDataMutex;

OsmAnd::WeatherRasterLayerProvider_P::WeatherRasterLayerProvider_P(
    WeatherRasterLayerProvider* const owner_,
    const QDateTime& dateTime_,
    const int bandIndex_,
    const QString& colorProfilePath_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/,
    const QString& dataCachePath_ /*= QString()*/,
    const QString& projSearchPath_ /*= QString()*/,
    const std::shared_ptr<const IWebClient> webClient_ /*= nullptr*/)
    : owner(owner_)
    , _webClient(webClient_)
{
    auto dateTimeStr = dateTime_.toString(QStringLiteral("yyyyMMdd_hh00"));
    auto geoDbCachePath = dataCachePath_
        + QDir::separator()
        + dateTimeStr
        + QStringLiteral(".tiff.db");
    
    _geoDb = new TileSqliteDatabase(geoDbCachePath);
    if (_geoDb->open())
    {
        TileSqliteDatabase::Meta meta;
        if (!_geoDb->obtainMeta(meta))
        {
            meta.setMinZoom(getMinZoom());
            meta.setMaxZoom(getMaxZoom());
            _geoDb->storeMeta(meta);
        }
    }
    
    auto rasterDbCachePath = dataCachePath_
        + QDir::separator()
        + dateTimeStr + QStringLiteral("_")
        + QString::number(bandIndex_)
        + QStringLiteral(".raster.db");
    
    _rasterDb = new TileSqliteDatabase(rasterDbCachePath);
    if (_rasterDb->open())
    {
        TileSqliteDatabase::Meta meta;
        if (!_rasterDb->obtainMeta(meta))
        {
            meta.setMinZoom(getMinZoom());
            meta.setMaxZoom(getMaxZoom());
            meta.setTileSize(tileSize_);
            _rasterDb->storeMeta(meta);
        }
    }
}

OsmAnd::WeatherRasterLayerProvider_P::~WeatherRasterLayerProvider_P()
{
    if (_rasterDb)
        delete _rasterDb;
    
    if (_geoDb)
        delete _geoDb;
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider_P::getMinZoom() const
{
    return ZoomLevel4;
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider_P::getMaxZoom() const
{
    return ZoomLevel7;
}

QString OsmAnd::WeatherRasterLayerProvider_P::doubleToStr(const double value)
{
    return QString::number(value, 'f', 12);
}

bool OsmAnd::WeatherRasterLayerProvider_P::obtainData(
    const IMapDataProvider::Request& request_,  
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<IRasterMapLayerProvider::Request>(request_);
    if (pOutMetric)
        pOutMetric->reset();

    if (request.zoom < getMinZoom() || request.zoom > getMaxZoom())
        return false;
    
    //QReadLocker scopedLocker(&_dataLock);
    
    bool success = false;
    
    int bandIndex = owner->bandIndex;
    ZoomLevel zoom = request.zoom;
    auto dt = owner->dateTime;
    auto dateTimeStr = dt.toString(QStringLiteral("yyyyMMdd_hh00"));

    {
        QWriteLocker scopedLocker(&_dataLock);
        
        if (_rasterDb->isOpened())
        {
            QByteArray data;
            if (_rasterDb->obtainTileData(request.tileId, request.zoom, data) && !data.isEmpty())
            {
                const auto image = SkiaUtilities::createImageFromData(data);
                if (image)
                {
                    outData.reset(new IRasterMapLayerProvider::Data(
                        request.tileId,
                        request.zoom,
                        AlphaChannelPresence::Present,
                        owner->densityFactor,
                        image));
                    
                    success = true;
                }
            }
        }
    }

    if (success)
        return true;

    GlobalMercator mercator;
        
    double latExtent = 1.0;
    double lonExtent = 1.0;
    auto tileBBox31 = Utilities::tileBoundingBox31(request.tileId, zoom);
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
    
    ZoomLevel geoTileZoom = ZoomLevel4;
    QHash<TileId, QByteArray> geoTilesMap;
    QVector<TileId> geoTileIdsToDownload;
    QVector<TileId> geoTileIds;
    if (zoom < geoTileZoom)
    {
        geoTileIds << Utilities::getTileIdsUnderscaledByZoomShift(request.tileId, geoTileZoom - zoom);
    }
    else if (zoom > geoTileZoom)
    {
        auto t = Utilities::getTileIdOverscaledByZoomShift(request.tileId, zoom - geoTileZoom);
        geoTileIds << TileId::fromXY(t.x, 15 - t.y);
    }
    else
    {
        //geoTileIds << request.tileId;
        geoTileIds << TileId::fromXY(request.tileId.x, 15 - request.tileId.y);
    }
    
    {
        QMutexLocker scopedLocker(&_geoDataMutex);

        if (_geoDb->isOpened())
        {
            for (auto tileId : geoTileIds)
            {
                QByteArray data;
                if (_geoDb->obtainTileData(tileId, geoTileZoom, data) && !data.isEmpty())
                    geoTilesMap.insert(tileId, qMove(data));
                else
                    geoTileIdsToDownload << tileId;
            }
        }
    }

    if (!geoTileIdsToDownload.empty())
    {
        QMutexLocker scopedLocker(&_geoDataMutex);
        
        if (_geoDb->isOpened())
        {
            for (auto tileId : geoTileIdsToDownload)
            {
                auto prefix = QStringLiteral("https://test.osmand.net/weather/gfs/tiff/");
                auto tileUrl = prefix + dateTimeStr + "/"
                    + QString::number(geoTileZoom) + QStringLiteral("_")
                    + QString::number(tileId.x) + QStringLiteral("_")
                    + QString::number(tileId.y) + QStringLiteral(".tiff.gz");
                
                auto filePath = owner->dataCachePath + QDir::separator()
                    + dateTimeStr + QStringLiteral("_")
                    + QString::number(geoTileZoom) + QStringLiteral("_")
                    + QString::number(tileId.x) + QStringLiteral("_")
                    + QString::number(tileId.y) + QStringLiteral(".tiff");

                auto filePathGz = filePath + QStringLiteral(".gz");

                OsmAnd::ElapsedTimer downloadTimer;
                downloadTimer.Start();
                if (_webClient->downloadFile(tileUrl, filePathGz))
                {
                    downloadTimer.Pause();
                    LogPrintf(LogSeverityLevel::Error, "%f Downloading %s", (double)downloadTimer.GetElapsedMs() / 1000.0, qPrintable(tileUrl));

                    ArchiveReader archive(filePathGz);
                    bool ok = false;
                    const auto archiveItems = archive.getItems(&ok, true);
                    if (ok)
                    {
                        ArchiveReader::Item tiffArchiveItem;
                        for (const auto& archiveItem : constOf(archiveItems))
                        {
                            if (!archiveItem.isValid() || (!archiveItem.name.endsWith(QStringLiteral(".tiff"))))
                                continue;
                            
                            tiffArchiveItem = archiveItem;
                            break;
                        }
                        if (tiffArchiveItem.isValid() && archive.extractItemToFile(tiffArchiveItem.name, filePath, true))
                        {
                            auto tileFile = QFile(filePath);
                            if (tileFile.open(QIODevice::ReadOnly))
                            {
                                const auto& data = tileFile.readAll();
                                tileFile.close();
                                if (!data.isEmpty())
                                {
                                    _geoDb->storeTileData(tileId, geoTileZoom, data);
                                    geoTilesMap.insert(tileId, qMove(data));
                                }
                            }
                        }
                    }
                    QFile(filePathGz).remove();
                    QFile(filePath).remove();
                }
            }
        }
    }
    
    if (geoTileIds.empty())
        return false;
            
    const auto citGeoTileData = geoTilesMap.constFind(geoTileIds[0]);
    if (citGeoTileData == geoTilesMap.cend())
        return false;
    auto geoTileData = *citGeoTileData;
    
    if (geoTileData.length() == 0)
    {
        // There was no data at all, to avoid further requests, mark this tile as empty
        outData.reset();
        return true;
    }

    //QMutexLocker scopedLocker(&_dataMutex);

    QString tileUrl = QString::asprintf("%dx%dx%d", request.tileId.x,
                                        request.tileId.y,
                                        request.zoom);
    
    OsmAnd::ElapsedTimer allTimer;
    allTimer.Start();

    OsmAnd::ElapsedTimer timer;
    timer.Start();

    // Map data to VSI memory
    const auto filename = QString::asprintf("/vsimem/weatherTile@%p", geoTileData.data());
    const auto file = VSIFileFromMemBuffer(
        qPrintable(filename),
        reinterpret_cast<GByte*>(geoTileData.data()),
        geoTileData.length(),
        FALSE
        );
    if (!file)
    {
        LogPrintf(LogSeverityLevel::Error,
                  "Failed to map weather geotiff %dx%d@%d",
                  request.tileId.x,
                  request.tileId.y,
                  request.zoom);
        return false;
    }
    VSIFCloseL(file);
    
    timer.Pause();
    LogPrintf(LogSeverityLevel::Error, "%f Load geo tile to mem %s", (double)timer.GetElapsedMs() / 1000.0, qPrintable(tileUrl));
    timer.Reset();
    timer.Start();

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
                  request.tileId.x,
                  request.tileId.y,
                  request.zoom);
        return false;
    }
        
    timer.Pause();
    LogPrintf(LogSeverityLevel::Error, "%f GDALOpen %s", (double)timer.GetElapsedMs() / 1000.0, qPrintable(tileUrl));
    timer.Reset();
    timer.Start();

    auto bandIndexStr = QString::number(bandIndex).toLatin1();

    auto tlExtLonStr = doubleToStr(tlExtLatLon.longitude).toLatin1();
    auto tlExtLatStr = doubleToStr(tlExtLatLon.latitude).toLatin1();
    auto brExtLonStr = doubleToStr(brExtLatLon.longitude).toLatin1();
    auto brExtLatStr = doubleToStr(brExtLatLon.latitude).toLatin1();

    const auto cropFilename = QString::asprintf("/vsimem/weatherTile@%s_%d_%dx%d@%d_crop",
                                                qPrintable(dateTimeStr), bandIndex,
                                                request.tileId.x, request.tileId.y, request.zoom);
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
            "Failed to create options for crop weather GeoTIFF %dx%d@%d to %s / %s.",
            request.tileId.x,
            request.tileId.y,
            request.zoom,
            qPrintable(tlExtLatLon.toQString()),
            qPrintable(brExtLatLon.toQString()));
        return false;
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
            "Failed to crop weather GeoTIFF %dx%d@%d to %s / %s. Error: %d",
            request.tileId.x,
            request.tileId.y,
            request.zoom,
            qPrintable(tlExtLatLon.toQString()),
            qPrintable(brExtLatLon.toQString()),
            bCropError);
        return false;
    }
    
    timer.Pause();
    LogPrintf(LogSeverityLevel::Error, "%f GDALTranslate %s", (double)timer.GetElapsedMs() / 1000.0, qPrintable(tileUrl));
    timer.Reset();
    timer.Start();

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

    const auto interpolateFilename = QString::asprintf("/vsimem/weatherTile@%s_%d_%dx%d@%d_interpolate",
                                                       qPrintable(dateTimeStr), bandIndex,
                                                       request.tileId.x, request.tileId.y, request.zoom);
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
            "Failed to create options for interpolate weather GeoTIFF %dx%d@%d",
            request.tileId.x,
            request.tileId.y,
            request.zoom);
        return false;
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
            "Failed to interpolate weather GeoTIFF %dx%d@%d. Error: %d",
            request.tileId.x,
            request.tileId.y,
            request.zoom,
            bWarpError);
        return false;
    }

    timer.Pause();
    LogPrintf(LogSeverityLevel::Error, "%f GDALWarp %s", (double)timer.GetElapsedMs() / 1000.0, qPrintable(tileUrl));
    timer.Reset();
    timer.Start();

    const auto colorizeFilename = QString::asprintf("/vsimem/weatherTile@%s_%d_%dx%d@%d_colorize",
                                                    qPrintable(dateTimeStr), bandIndex,
                                                    request.tileId.x, request.tileId.y, request.zoom);
    const char* colorizeArgs[] = {
        //"-b", bandIndexStr.constData(),
        "-alpha",
        NULL };
    GDALDEMProcessingOptions *psColorizeOptions = GDALDEMProcessingOptionsNew(const_cast<char **>(colorizeArgs), NULL);
    if (!psColorizeOptions)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to colorize weather GeoTIFF %dx%d@%d",
            request.tileId.x,
            request.tileId.y,
            request.zoom);
        return false;
    }
                                    
    int bDEMError = FALSE;
    std::shared_ptr<void> hColorizedDS(
        GDALDEMProcessing(
            qPrintable(colorizeFilename),
            hInterpolatedDS.get(),
            "color-relief",
            qPrintable(owner->colorProfilePath),
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
            "Failed to colorize weather GeoTIFF %dx%d@%d. Error: %d",
            request.tileId.x,
            request.tileId.y,
            request.zoom,
            bDEMError);
        return false;
    }
    
    timer.Pause();
    LogPrintf(LogSeverityLevel::Error, "%f GDALDem %s", (double)timer.GetElapsedMs() / 1000.0, qPrintable(tileUrl));
    timer.Reset();
    timer.Start();

    auto interBoundsMinOrig = mercator.latLonToMeters(tlLatLon);
    auto tileMinPix = mercator.metersToPixels(interBoundsMinOrig.x, interBoundsMinOrig.y, zoom);
    auto tileMinExtPix = mercator.metersToPixels(interBoundsMin.x, interBoundsMin.y, zoom);
    
    int tileX = request.tileId.x;
    int tileY = request.tileId.y;
        
    auto origin = PointD(abs(tileMinPix.x - tileMinExtPix.x), abs(tileMinPix.y - tileMinExtPix.y));
    
    auto originShiftXStr = doubleToStr(origin.x).toLatin1();
    auto originShiftYStr = doubleToStr(origin.y).toLatin1();
    auto tileSizeStr = QString::number(owner->tileSize).toLatin1();

    const auto tileFilename = QString::asprintf("/vsimem/weatherTile_%s_%d_%dx%dx%d",
                                                qPrintable(dateTimeStr), bandIndex,
                                                (int) zoom, tileX, tileY);
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
            "Failed to create options for result weather tile %dx%dx%d", tileX, tileX, zoom);
        return false;
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
            "Failed to create result weather tile %dx%dx%d. Error: %d", tileX, tileX, zoom, bTranslateError);
        return false;
    }
    
    timer.Pause();
    LogPrintf(LogSeverityLevel::Error, "%f GDALTranslate raster %s", (double)timer.GetElapsedMs() / 1000.0, qPrintable(tileUrl));
    timer.Reset();
    timer.Start();

    VSIStatBufL sStatBuf;
    auto stat = VSIStatL(qPrintable(tileFilename), &sStatBuf);
    auto nBufLength = sStatBuf.st_size;
    auto fpDB = VSIFOpenL(qPrintable(tileFilename), "r");
    char *pszDBData = static_cast<char *>(CPLCalloc(1, nBufLength + 1));
    if (VSIFReadL(pszDBData, 1, nBufLength, fpDB) == static_cast<size_t>(nBufLength))
    {
        QWriteLocker scopedLocker(&_dataLock);
        
        OsmAnd::ElapsedTimer t;
        t.Start();

        if (_rasterDb->isOpened())
        {
            _rasterDb->storeTileData(request.tileId, request.zoom, QByteArray::fromRawData(pszDBData, (int) nBufLength));
        }

        t.Pause();
        LogPrintf(LogSeverityLevel::Error, "%f Store raster %s", (double)t.GetElapsedMs() / 1000.0, qPrintable(tileUrl));
        t.Reset();
        t.Start();

        auto image = SkImage::MakeFromEncoded(SkData::MakeWithProc(
            pszDBData,
            nBufLength,
            [](const void* ptr, void* context) {
                CPLFree(context);
            },
            pszDBData
        ));

        t.Pause();
        LogPrintf(LogSeverityLevel::Error, "%f Decode raster %s", (double)t.GetElapsedMs() / 1000.0, qPrintable(tileUrl));
        t.Reset();

        if (image)
        {
            outData.reset(new IRasterMapLayerProvider::Data(
                request.tileId,
                request.zoom,
                AlphaChannelPresence::Present,
                owner->densityFactor,
                image));
            
            success = true;
        }
    }
    CPL_IGNORE_RET_VAL(VSIFCloseL(fpDB));

    hColorizedDS = nullptr;

    timer.Pause();
    LogPrintf(LogSeverityLevel::Error, "%f Create png %s", (double)timer.GetElapsedMs() / 1000.0, qPrintable(tileUrl));
    timer.Reset();

    allTimer.Pause();
    LogPrintf(LogSeverityLevel::Error, "%f All %s", (double)allTimer.GetElapsedMs() / 1000.0, qPrintable(tileUrl));
    allTimer.Reset();

    return success;
}
