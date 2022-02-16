#include "WeatherTileResourceProvider_P.h"
#include "WeatherTileResourceProvider.h"
#include "GeoTileRasterizer.h"

#include "Logging.h"
#include "Utilities.h"
#include "SkiaUtilities.h"
#include "GlobalMercator.h"
#include "ArchiveReader.h"
#include "FunctorQueryController.h"

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

    if (request->version != _provider->getCurrentRequestVersion())
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
    
    auto dt = _provider->dateTime;
    auto dateTimeStr = dt.toString(QStringLiteral("yyyyMMdd_hh00"));
    
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
        auto t = Utilities::getTileIdOverscaledByZoomShift(tileId, zoom - geoTileZoom);
        geoTileId = TileId::fromXY(t.x, 15 - t.y);
    }
    else
    {
        geoTileId = TileId::fromXY(tileId.x, 15 - tileId.y);
    }
    
    _provider->lockGeoTile(geoTileId, geoTileZoom);
    
    auto geoDb = _provider->getGeoTilesDatabase();
    if (geoDb->isOpened())
    {
        if (!geoDb->obtainTileData(geoTileId, geoTileZoom, geoTileData) || geoTileData.isEmpty())
        {
            auto geoTileUrl = WEATHER_TILES_URL_PREFIX + dateTimeStr + "/"
                + QString::number(geoTileZoom) + QStringLiteral("_")
                + QString::number(geoTileId.x) + QStringLiteral("_")
                + QString::number(geoTileId.y) + QStringLiteral(".tiff.gz");
            
            auto filePath = _provider->localCachePath + QDir::separator()
                + dateTimeStr + QStringLiteral("_")
                + QString::number(geoTileZoom) + QStringLiteral("_")
                + QString::number(geoTileId.x) + QStringLiteral("_")
                + QString::number(geoTileId.y) + QStringLiteral(".tiff");

            auto filePathGz = filePath + QStringLiteral(".gz");

            if (_provider->webClient->downloadFile(geoTileUrl, filePathGz))
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
                            geoTileData = tileFile.readAll();
                            tileFile.close();
                            if (!geoTileData.isEmpty())
                            {
                                geoDb->storeTileData(geoTileId, geoTileZoom, geoTileData);
                            }
                        }
                    }
                }
                QFile(filePathGz).remove();
                QFile(filePath).remove();
            }
        }
    }
    
    _provider->unlockGeoTile(geoTileId, geoTileZoom);

    if (geoTileData.isEmpty())
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
