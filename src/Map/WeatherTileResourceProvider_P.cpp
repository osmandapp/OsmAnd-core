#include "WeatherTileResourceProvider_P.h"
#include "WeatherTileResourceProvider.h"
#include "GeoTileRasterizer.h"

#include "Logging.h"
#include "Utilities.h"
#include "SkiaUtilities.h"
#include "GlobalMercator.h"
#include "ArchiveReader.h"
#include "FunctorQueryController.h"
#include "GeoTileEvaluator.h"

static const QString WEATHER_TILES_URL_PREFIX = QStringLiteral("https://test.osmand.net/weather/gfs/tiff/");

OsmAnd::WeatherTileResourceProvider_P::WeatherTileResourceProvider_P(
    WeatherTileResourceProvider* const owner_,
    const QDateTime& dateTime_,
    const QHash<BandIndex, float>& bandOpacityMap_,
    const QHash<BandIndex, QString>& bandColorProfilePaths_,
    const QString& localCachePath_,
    const QString& projResourcesPath_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/,
    const std::shared_ptr<const IWebClient>& webClient_ /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
    : owner(owner_)
    , _threadPool(new QThreadPool())
    , _bandOpacityMap(bandOpacityMap_)
    , _priority(0)
    , _lastRequestedZoom(ZoomLevel::InvalidZoomLevel)
    , _requestVersion(0)
    , webClient(webClient_)
    , dateTime(dateTime_)
    , bandColorProfilePaths(bandColorProfilePaths_)
    , localCachePath(localCachePath_)
    , projResourcesPath(projResourcesPath_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
    auto dateTimeStr = dateTime_.toString(QStringLiteral("yyyyMMdd_hh00"));
    auto geoDbCachePath = localCachePath_
        + QDir::separator()
        + dateTimeStr
        + QStringLiteral(".tiff.db");
    
    _geoTilesDb = std::make_shared<TileSqliteDatabase>(geoDbCachePath);
    if (_geoTilesDb->open())
    {
        TileSqliteDatabase::Meta meta;
        if (!_geoTilesDb->obtainMeta(meta))
        {
            meta.setMinZoom(WeatherTileResourceProvider::getGeoTileZoom());
            meta.setMaxZoom(WeatherTileResourceProvider::getGeoTileZoom());
            _geoTilesDb->storeMeta(meta);
        }
    }
}

OsmAnd::WeatherTileResourceProvider_P::~WeatherTileResourceProvider_P()
{
    _threadPool->clear();
    delete _threadPool;
}

int OsmAnd::WeatherTileResourceProvider_P::getAndDecreasePriority()
{
    QWriteLocker scopedLocker(&_lock);
    
    return --_priority;
}

const QHash<OsmAnd::BandIndex, float> OsmAnd::WeatherTileResourceProvider_P::getBandOpacityMap() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _bandOpacityMap;
}

void OsmAnd::WeatherTileResourceProvider_P::setBandOpacityMap(const QHash<BandIndex, float>& bandOpacityMap)
{
    {
        QWriteLocker scopedLocker(&_lock);

        _bandOpacityMap = bandOpacityMap;
    }
    getAndUpdateRequestVersion();
}

int OsmAnd::WeatherTileResourceProvider_P::getCurrentRequestVersion() const
{
    QWriteLocker scopedLocker(&_lock);
    
    return _requestVersion;
}

int OsmAnd::WeatherTileResourceProvider_P::getAndUpdateRequestVersion(
    const std::shared_ptr<WeatherTileResourceProvider::TileRequest>& request /*= nullptr*/)
{
    QWriteLocker scopedLocker(&_lock);
    
    if (request)
    {
        if (_lastRequestedZoom != request->zoom || _lastRequestedBands != request->bands)
        {
            _lastRequestedZoom = request->zoom;
            _lastRequestedBands = request->bands;
            _priority = 0;
            return ++_requestVersion;
        }
    }
    else
    {
        _priority = 0;
        return ++_requestVersion;
    }
    return _requestVersion;
}

std::shared_ptr<OsmAnd::TileSqliteDatabase> OsmAnd::WeatherTileResourceProvider_P::createRasterTilesDatabase(BandIndex band)
{
    auto dateTimeStr = dateTime.toString(QStringLiteral("yyyyMMdd_hh00"));
    auto rasterDbCachePath = localCachePath
        + QDir::separator()
        + dateTimeStr + QStringLiteral("_")
        + QString::number(band)
        + QStringLiteral(".raster.db");
    
    auto db = std::make_shared<TileSqliteDatabase>(rasterDbCachePath);
    if (db->open())
    {
        TileSqliteDatabase::Meta meta;
        if (!db->obtainMeta(meta))
        {
            meta.setMinZoom(WeatherTileResourceProvider::getTileZoom(WeatherLayer::Low));
            meta.setMaxZoom(WeatherTileResourceProvider::getTileZoom(WeatherLayer::High));
            meta.setTileSize(tileSize);
            db->storeMeta(meta);
        }
        return db;
    }
    return nullptr;
}

bool OsmAnd::WeatherTileResourceProvider_P::obtainGeoTile(
    const TileId tileId,
    const ZoomLevel zoom,
    QByteArray& outData,
    bool forceDownload /*= false*/)
{
    bool res = false;
    lockGeoTile(tileId, zoom);
    
    auto geoDb = getGeoTilesDatabase();
    if (geoDb->isOpened())
    {
        if (forceDownload || !geoDb->obtainTileData(tileId, zoom, outData) || outData.isEmpty())
        {
            auto dateTimeStr = dateTime.toString(QStringLiteral("yyyyMMdd_hh00"));
            auto geoTileUrl = WEATHER_TILES_URL_PREFIX + dateTimeStr + "/"
                + QString::number(zoom) + QStringLiteral("_")
                + QString::number(tileId.x) + QStringLiteral("_")
                + QString::number(15 - tileId.y) + QStringLiteral(".tiff.gz");
            
            auto filePath = localCachePath + QDir::separator()
                + dateTimeStr + QStringLiteral("_")
                + QString::number(zoom) + QStringLiteral("_")
                + QString::number(tileId.x) + QStringLiteral("_")
                + QString::number(15 - tileId.y) + QStringLiteral(".tiff");

            auto filePathGz = filePath + QStringLiteral(".gz");

            if (webClient->downloadFile(geoTileUrl, filePathGz))
            {
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
                            outData = tileFile.readAll();
                            tileFile.close();
                            if (!outData.isEmpty())
                                geoDb->storeTileData(tileId, zoom, outData);

                            res = true;
                        }
                    }
                }
                QFile(filePathGz).remove();
                QFile(filePath).remove();
            }
        }
        else
        {
            res = true;
        }
    }
    
    unlockGeoTile(tileId, zoom);
    return res;
}

void OsmAnd::WeatherTileResourceProvider_P::lockGeoTile(const TileId tileId, const ZoomLevel zoom)
{
    QMutexLocker scopedLocker(&_geoTilesInProcessMutex);

    while(_geoTilesInProcess[zoom].contains(tileId))
        _waitUntilAnyGeoTileIsProcessed.wait(&_geoTilesInProcessMutex);

    _geoTilesInProcess[zoom].insert(tileId);
}

void OsmAnd::WeatherTileResourceProvider_P::unlockGeoTile(const TileId tileId, const ZoomLevel zoom)
{
    QMutexLocker scopedLocker(&_geoTilesInProcessMutex);

    _geoTilesInProcess[zoom].remove(tileId);

    _waitUntilAnyGeoTileIsProcessed.wakeAll();
}

void OsmAnd::WeatherTileResourceProvider_P::lockTile(const TileId tileId, const ZoomLevel zoom)
{
    QMutexLocker scopedLocker(&_tilesInProcessMutex);

    while(_tilesInProcess[zoom].contains(tileId))
        _waitUntilAnyTileIsProcessed.wait(&_tilesInProcessMutex);

    _tilesInProcess[zoom].insert(tileId);
}

void OsmAnd::WeatherTileResourceProvider_P::unlockTile(const TileId tileId, const ZoomLevel zoom)
{
    QMutexLocker scopedLocker(&_tilesInProcessMutex);

    _tilesInProcess[zoom].remove(tileId);

    _waitUntilAnyTileIsProcessed.wakeAll();
}

void OsmAnd::WeatherTileResourceProvider_P::obtainValueAsync(
    const WeatherTileResourceProvider::ValueRequest& request,
    const WeatherTileResourceProvider::ObtainValueAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto requestClone = request.clone();
    ObtainValueTask *task = new ObtainValueTask(shared_from_this(), requestClone, callback, collectMetric);
    task->setAutoDelete(true);
    _threadPool->start(task, getAndDecreasePriority());
}

void OsmAnd::WeatherTileResourceProvider_P::obtainDataAsync(
    const WeatherTileResourceProvider::TileRequest& request,
    const WeatherTileResourceProvider::ObtainTileDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto requestClone = request.clone();
    requestClone->version = getAndUpdateRequestVersion(requestClone);
    ObtainTileTask *task = new ObtainTileTask(shared_from_this(), requestClone, callback, collectMetric);
    task->setAutoDelete(true);
    _threadPool->start(task, getAndDecreasePriority());
}

void OsmAnd::WeatherTileResourceProvider_P::downloadGeoTilesAsync(
    const WeatherTileResourceProvider::DownloadGeoTileRequest& request,
    const WeatherTileResourceProvider::DownloadGeoTilesAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto requestClone = request.clone();
    DownloadGeoTileTask *task = new DownloadGeoTileTask(shared_from_this(), requestClone, callback, collectMetric);
    task->setAutoDelete(true);
    _threadPool->start(task, 1);
}

std::shared_ptr<OsmAnd::TileSqliteDatabase> OsmAnd::WeatherTileResourceProvider_P::getGeoTilesDatabase()
{
    QReadLocker scopedLocker(&_geoDbLock);

    return _geoTilesDb;
}

std::shared_ptr<OsmAnd::TileSqliteDatabase> OsmAnd::WeatherTileResourceProvider_P::getRasterTilesDatabase(BandIndex band)
{
    {
        QReadLocker scopedLocker(&_rasterDbLock);

        const auto citRasterDb = _rasterTilesDbMap.constFind(band);
        if (citRasterDb != _rasterTilesDbMap.cend())
            return *citRasterDb;
    }
    {
        QWriteLocker scopedLocker(&_rasterDbLock);

        auto db = createRasterTilesDatabase(band);
        if (db)
            _rasterTilesDbMap.insert(band, db);
        
        return db;
    }
}

bool OsmAnd::WeatherTileResourceProvider_P::closeProvider()
{
    QWriteLocker geoScopedLocker(&_geoDbLock);
    QWriteLocker rasterScopedLocker(&_rasterDbLock);

    _geoTilesDb->close();
    
    for (auto& db : _rasterTilesDbMap.values())
        db->close();
    
    return true;
}

OsmAnd::WeatherTileResourceProvider_P::ObtainValueTask::ObtainValueTask(
    std::shared_ptr<WeatherTileResourceProvider_P> provider,
    const std::shared_ptr<WeatherTileResourceProvider::ValueRequest> request_,
    const WeatherTileResourceProvider::ObtainValueAsyncCallback callback_,
    const bool collectMetric_ /*= false*/)
    : _provider(provider)
    , request(request_)
    , callback(callback_)
    , collectMetric(collectMetric_)
{
}

OsmAnd::WeatherTileResourceProvider_P::ObtainValueTask::~ObtainValueTask()
{
}

void OsmAnd::WeatherTileResourceProvider_P::ObtainValueTask::run()
{
    PointI point31 = request->point31;
    ZoomLevel zoom = request->zoom;
    auto band = request->band;
    
    const auto geoTileZoom = WeatherTileResourceProvider::getGeoTileZoom();
    const auto latLon = Utilities::convert31ToLatLon(point31);
    const auto geoTileId = TileId::fromXY(
        Utilities::getTileNumberX(geoTileZoom, latLon.longitude),
        Utilities::getTileNumberY(geoTileZoom, latLon.latitude)
    );
    
    QByteArray geoTileData;
    if (_provider->obtainGeoTile(geoTileId, geoTileZoom, geoTileData))
    {
        GeoTileEvaluator *evaluator = new GeoTileEvaluator(
            geoTileId,
            geoTileData,
            zoom,
            _provider->projResourcesPath
        );
        QHash<BandIndex, double> values;
        if (evaluator->evaluate(latLon, values) && values.contains(band))
            callback(true, values[band], nullptr);
        else
            callback(false, 0.0, nullptr);

        delete evaluator;
    }
    else
    {
        callback(false, 0.0, nullptr);
    }
}

OsmAnd::WeatherTileResourceProvider_P::ObtainTileTask::ObtainTileTask(
    std::shared_ptr<WeatherTileResourceProvider_P> provider,
    const std::shared_ptr<WeatherTileResourceProvider::TileRequest> request_,
    const WeatherTileResourceProvider::ObtainTileDataAsyncCallback callback_,
    const bool collectMetric_ /*= false*/)
    : _provider(provider)
    , request(request_)
    , callback(callback_)
    , collectMetric(collectMetric_)
{
}

OsmAnd::WeatherTileResourceProvider_P::ObtainTileTask::~ObtainTileTask()
{
}

sk_sp<const SkImage> OsmAnd::WeatherTileResourceProvider_P::ObtainTileTask::createTileImage(
    const QHash<BandIndex, sk_sp<const SkImage>>& bandImages,
    const QList<BandIndex>& bands)
{
    if (bandImages.empty() || bands.empty())
        return nullptr;
    
    QList<sk_sp<const SkImage>> images;
    QList<float> alphas;
    const auto& opacityMap = _provider->getBandOpacityMap();
    for (auto band : bands)
    {
        const auto citImage = bandImages.constFind(band);
        if (citImage != bandImages.cend())
        {
            images << *citImage;
            alphas << opacityMap[band];
        }
        else
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to create tile image of weather tile. Band %d is not rasterized.", band);
        }
    }

    return SkiaUtilities::mergeImages(images, alphas);
}

void OsmAnd::WeatherTileResourceProvider_P::ObtainTileTask::run()
{
    TileId tileId = request->tileId;
    ZoomLevel zoom = request->zoom;
    auto bands = request->bands;

    if (request->version != _provider->getCurrentRequestVersion() && !request->ignoreVersion)
    {
        LogPrintf(LogSeverityLevel::Debug,
            "Stop creating tile image of weather tile %dx%dx%d. Version changed %d:%d", tileId.x, tileId.y, zoom, request->version, _provider->getCurrentRequestVersion());

        callback(false, nullptr, nullptr);
        return;
    }
    if (request->queryController && request->queryController->isAborted())
    {
        LogPrintf(LogSeverityLevel::Debug,
            "Stop creating tile image of weather tile %dx%dx%d.", tileId.x, tileId.y, zoom);

        callback(false, nullptr, nullptr);
        return;
    }
    
    _provider->lockTile(tileId, zoom);

    QList<BandIndex> missingBands;
    QHash<BandIndex, sk_sp<const SkImage>> images;
    
    for (auto band : bands)
    {
        auto db = _provider->getRasterTilesDatabase(band);
        if (db && db->isOpened())
        {
            QByteArray data;
            if (db->obtainTileData(tileId, zoom, data) && !data.isEmpty())
            {
                const auto image = SkiaUtilities::createImageFromData(data);
                if (image)
                    images.insert(band, image);
                else
                    missingBands << band;
            }
            else
            {
                missingBands << band;
            }
        }
    }
    if (missingBands.empty())
    {
        _provider->unlockTile(tileId, zoom);

        auto image = createTileImage(images, bands);
        if (image)
        {
            auto data = std::make_shared<OsmAnd::WeatherTileResourceProvider::Data>(
                tileId,
                zoom,
                AlphaChannelPresence::Present,
                _provider->densityFactor,
                image
            );
            callback(true, data, nullptr);
            return;
        }
        else
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to create tile image of weather tile %dx%dx%d", tileId.x, tileId.y, zoom);
            callback(false, nullptr, nullptr);
            return;
        }
    }
            
    ZoomLevel geoTileZoom = WeatherTileResourceProvider::getGeoTileZoom();
    QByteArray geoTileData;
    TileId geoTileId;
    if (zoom < geoTileZoom)
    {
        _provider->unlockTile(tileId, zoom);

        // Underzoom for geo tiles currently not supported
        LogPrintf(LogSeverityLevel::Error,
            "Failed to resolve geoTileId for weather tile %dx%dx%d. Geo tile zoom (%d) > tile zoom (%d).", tileId.x, tileId.y, zoom, geoTileZoom, zoom);
        callback(false, nullptr, nullptr);
        return;
    }
    else if (zoom > geoTileZoom)
    {
        geoTileId = Utilities::getTileIdOverscaledByZoomShift(tileId, zoom - geoTileZoom);
    }
    else
    {
        geoTileId = tileId;
    }
    
    if (!_provider->obtainGeoTile(geoTileId, geoTileZoom, geoTileData) || geoTileData.isEmpty())
    {
        _provider->unlockTile(tileId, zoom);

        LogPrintf(LogSeverityLevel::Error,
            "GeoTile %dx%dx%d is empty", geoTileId.x, geoTileId.y, geoTileZoom);
        callback(false, nullptr, nullptr);
        return;
    }
    if (request->queryController && request->queryController->isAborted())
    {
        _provider->unlockTile(tileId, zoom);

        LogPrintf(LogSeverityLevel::Debug,
            "Stop creating tile image of weather tile %dx%dx%d.", tileId.x, tileId.y, zoom);

        callback(false, nullptr, nullptr);
        return;
    }
    
    GeoTileRasterizer *rasterizer = new GeoTileRasterizer(
        geoTileData,
        request->tileId,
        request->zoom,
        missingBands,
        _provider->bandColorProfilePaths,
        _provider->tileSize,
        _provider->densityFactor,
        _provider->projResourcesPath
    );
    
    QHash<BandIndex, QByteArray> encImgData;
    const auto rasterizeQueryController = std::make_shared<FunctorQueryController>(
        [this]
        (const FunctorQueryController* const queryController) -> bool
        {
            return request->version != _provider->getCurrentRequestVersion()
            || (request->queryController && request->queryController->isAborted());
        });
    const auto rasterizedImages = rasterizer->rasterize(encImgData, nullptr, rasterizeQueryController);
    delete rasterizer;

    if (request->queryController && request->queryController->isAborted())
    {
        _provider->unlockTile(tileId, zoom);

        LogPrintf(LogSeverityLevel::Debug,
            "Stop creating tile image of weather tile %dx%dx%d.", tileId.x, tileId.y, zoom);

        callback(false, nullptr, nullptr);
        return;
    }
    
    auto itBandImageData = iteratorOf(encImgData);
    while (itBandImageData.hasNext())
    {
        const auto& bandImageDataEntry = itBandImageData.next();
        const auto& band = bandImageDataEntry.key();
        const auto& data = bandImageDataEntry.value();
        auto db = _provider->getRasterTilesDatabase(band);
        if (db && db->isOpened())
        {
            if (!db->storeTileData(tileId, zoom, data))
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to store tile image of rasterized weather tile %dx%dx%d in sqlitedb %s",
                          tileId.x, tileId.y, zoom, qPrintable(db->filename));
            }
        }
    }
    
    _provider->unlockTile(tileId, zoom);
    
    if (request->version != _provider->getCurrentRequestVersion())
    {
        LogPrintf(LogSeverityLevel::Debug,
            "Cancel rasterization tile image of weather tile %dx%dx%d. Version changed %d:%d", tileId.x, tileId.y, zoom, request->version, _provider->getCurrentRequestVersion());

        callback(false, nullptr, nullptr);
        return;
    }
    if (rasterizedImages.empty())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed rasterize weather tile %dx%dx%d", tileId.x, tileId.y, zoom);
    }
    images.insert(rasterizedImages);
    if (!images.empty())
    {
        auto image = createTileImage(images, bands);
        if (image)
        {
            auto data = std::make_shared<OsmAnd::WeatherTileResourceProvider::Data>(
                tileId,
                zoom,
                AlphaChannelPresence::Present,
                _provider->densityFactor,
                image
            );
            callback(true, data, nullptr);
            return;
        }
        else
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to create tile image of rasterized weather tile %dx%dx%d", tileId.x, tileId.y, zoom);
            callback(false, nullptr, nullptr);
            return;
        }
    }
    else
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to create tile image of non rasterized weather tile %dx%dx%d", tileId.x, tileId.y, zoom);
        callback(false, nullptr, nullptr);
        return;
    }
}

OsmAnd::WeatherTileResourceProvider_P::DownloadGeoTileTask::DownloadGeoTileTask(
    std::shared_ptr<WeatherTileResourceProvider_P> provider,
    const std::shared_ptr<WeatherTileResourceProvider::DownloadGeoTileRequest> request_,
    const WeatherTileResourceProvider::DownloadGeoTilesAsyncCallback callback_,
    const bool collectMetric_ /*= false*/)
    : _provider(provider)
    , request(request_)
    , callback(callback_)
    , collectMetric(collectMetric_)
{
}

OsmAnd::WeatherTileResourceProvider_P::DownloadGeoTileTask::~DownloadGeoTileTask()
{
}

void OsmAnd::WeatherTileResourceProvider_P::DownloadGeoTileTask::run()
{
    LatLon topLeft = request->topLeft;
    LatLon bottomRight = request->bottomRight;
    
    auto dt = _provider->dateTime;
    auto dateTimeStr = dt.toString(QStringLiteral("yyyyMMdd_hh00"));
        
    if (request->queryController && request->queryController->isAborted())
    {
        LogPrintf(LogSeverityLevel::Debug,
                  "Stop downloading weather tiles area %f, %f / %f, %f for %s",
                  topLeft.latitude, topLeft.longitude, bottomRight.latitude, bottomRight.longitude, qPrintable(dateTimeStr));
        callback(false, 0, 0, nullptr);
        return;
    }

    ZoomLevel geoTileZoom = WeatherTileResourceProvider::getGeoTileZoom();
    QVector<TileId> geoTileIds;
        
    const auto topLeftTileId = OsmAnd::TileId::fromXY(
        Utilities::getTileNumberX(geoTileZoom, topLeft.longitude),
        Utilities::getTileNumberY(geoTileZoom, topLeft.latitude));
    
    const auto bottomRightTileId = OsmAnd::TileId::fromXY(
        Utilities::getTileNumberX(geoTileZoom, bottomRight.longitude),
        Utilities::getTileNumberY(geoTileZoom, bottomRight.latitude));
    
    if (topLeftTileId == bottomRightTileId)
    {
        geoTileIds << topLeftTileId;
    }
    else
    {
        int maxTileValue = (1 << geoTileZoom) - 1;
        auto x1 = topLeftTileId.x;
        auto y1 = topLeftTileId.y;
        auto x2 = bottomRightTileId.x;
        auto y2 = bottomRightTileId.y;
        if (x2 < x1)
            x2 = maxTileValue + x2 + 1;
        if (y2 < y1)
            y2 = maxTileValue + y2 + 1;

        auto y = y1;
        for (int iy = y1; iy <= y2; iy++)
        {
            auto x = x1;
            for (int ix = x1; ix <= x2; ix++)
            {
                geoTileIds << TileId::fromXY(x, y);
                x = x == maxTileValue ? 0 : x + 1;
            }
            y = y == maxTileValue ? 0 : y + 1;
        }
    }
    uint64_t downloadedTiles = 0;
    auto tilesCount = geoTileIds.size();
    for (const auto& tileId : constOf(geoTileIds))
    {
        QByteArray data;
        bool res = _provider->obtainGeoTile(tileId, geoTileZoom, data);
        callback(res, ++downloadedTiles, tilesCount, nullptr);
        
        if (request->queryController && request->queryController->isAborted())
        {
            LogPrintf(LogSeverityLevel::Debug,
                      "Cancel downloading weather tiles area %f, %f / %f, %f for %s",
                      topLeft.latitude, topLeft.longitude, bottomRight.latitude, bottomRight.longitude, qPrintable(dateTimeStr));
            callback(false, downloadedTiles, tilesCount, nullptr);
            return;
        }
    }
}
