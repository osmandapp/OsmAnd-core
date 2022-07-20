#include "WeatherTileResourceProvider_P.h"
#include "WeatherTileResourceProvider.h"
#include "WeatherTileResourcesManager.h"
#include "GeoTileRasterizer.h"

#include "Logging.h"
#include "Utilities.h"
#include "SkiaUtilities.h"
#include "GlobalMercator.h"
#include "ArchiveReader.h"
#include "FunctorQueryController.h"
#include "GeoTileEvaluator.h"

static const QString WEATHER_TILES_URL_PREFIX = QStringLiteral("https://osmand.net/weather/gfs/tiff/");

OsmAnd::WeatherTileResourceProvider_P::WeatherTileResourceProvider_P(
    WeatherTileResourceProvider* const owner_,
    const QDateTime& dateTime_,
    const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings_,
    const bool localData_,
    const QString& localCachePath_,
    const QString& projResourcesPath_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/,
    const std::shared_ptr<const IWebClient>& webClient_ /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
    : owner(owner_)
    , _threadPool(new QThreadPool())
    , _obtainValueThreadPool(new QThreadPool())
    , _bandSettings(bandSettings_)
    , _localData(localData_)
    , _priority(0)
    , _obtainValuePriority(0)
    , _lastRequestedZoom(ZoomLevel::InvalidZoomLevel)
    , _requestVersion(0)
    , webClient(webClient_)
    , dateTime(dateTime_)
    , localCachePath(localCachePath_)
    , projResourcesPath(projResourcesPath_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
    _obtainValueThreadPool->setMaxThreadCount(1);
}

OsmAnd::WeatherTileResourceProvider_P::~WeatherTileResourceProvider_P()
{
    _obtainValueThreadPool->clear();
    delete _obtainValueThreadPool;
    
    _threadPool->clear();
    delete _threadPool;
}

int OsmAnd::WeatherTileResourceProvider_P::getAndDecreasePriority()
{
    QWriteLocker scopedLocker(&_lock);
    
    return --_priority;
}

int OsmAnd::WeatherTileResourceProvider_P::getAndDecreaseObtainValuePriority()
{
    QWriteLocker scopedLocker(&_lock);
    
    return --_obtainValuePriority;
}

const QHash<OsmAnd::BandIndex, std::shared_ptr<const OsmAnd::GeoBandSettings>> OsmAnd::WeatherTileResourceProvider_P::getBandSettings() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _bandSettings;
}

void OsmAnd::WeatherTileResourceProvider_P::setBandSettings(const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings)
{
    {
        QWriteLocker scopedLocker(&_lock);

        _bandSettings = bandSettings;
    }
    getAndUpdateRequestVersion();
}

const bool OsmAnd::WeatherTileResourceProvider_P::isLocalData() const
{
    QReadLocker scopedLocker(&_lock);

    return _localData;
}

void OsmAnd::WeatherTileResourceProvider_P::setLocalData(const bool localData)
{
    {
        QWriteLocker scopedLocker(&_lock);

        _localData = localData;
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
        if (_lastRequestedZoom != request->zoom || _lastRequestedBands != request->bands || _lastRequestedLocalData != request->localData)
        {
            _lastRequestedZoom = request->zoom;
            _lastRequestedBands = request->bands;
            _lastRequestedLocalData = request->localData;
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

std::shared_ptr<OsmAnd::TileSqliteDatabase> OsmAnd::WeatherTileResourceProvider_P::createRasterTilesDatabase(
        BandIndex band,
        bool localData)
{
    auto dateTimeStr = dateTime.toString(QStringLiteral("yyyyMMdd_hh00"));
    auto rasterDbCachePath = localCachePath
            + QDir::separator()
            + (localData ? "offline" : "online");

    QDir dir(rasterDbCachePath);
    if (!dir.exists())
        dir.mkpath(".");

    rasterDbCachePath += QDir::separator()
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
            meta.setTileNumbering(QStringLiteral("WeatherForecast"));
            db->storeMeta(meta);
        }
        return db;
    }
    return nullptr;
}

std::shared_ptr<OsmAnd::TileSqliteDatabase> OsmAnd::WeatherTileResourceProvider_P::createGeoTilesDatabase(bool localData)
{
    auto dateTimeStr = dateTime.toString(QStringLiteral("yyyyMMdd_hh00"));
    auto geoDbCachePath = localCachePath
            + QDir::separator()
            + (localData ? "offline" : "online");

    QDir dir(geoDbCachePath);
    if (!dir.exists())
        dir.mkpath(".");

    geoDbCachePath += QDir::separator()
            + dateTimeStr
            + QStringLiteral(".tiff.db");

    auto db = std::make_shared<TileSqliteDatabase>(geoDbCachePath);
    if (db->open())
    {
        TileSqliteDatabase::Meta meta;
        if (!db->obtainMeta(meta))
        {
            meta.setMinZoom(WeatherTileResourceProvider::getGeoTileZoom());
            meta.setMaxZoom(WeatherTileResourceProvider::getGeoTileZoom());
            meta.setTileNumbering(QStringLiteral("WeatherForecast"));
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
    auto dateTimeStr = dateTime.toString(QStringLiteral("yyyyMMdd_hh00"));
    auto geoTileUrl = WEATHER_TILES_URL_PREFIX + dateTimeStr + "/"
            + QString::number(zoom) + QStringLiteral("_")
            + QString::number(tileId.x) + QStringLiteral("_")
            + QString::number(15 - tileId.y) + QStringLiteral(".tiff.gz");

    lockGeoTile(tileId, zoom);

    auto geoDb = getGeoTilesDatabase(_localData);
    if (geoDb->isOpened())
    {
        bool needToDownload = !geoDb->obtainTileData(tileId, zoom, outData) || outData.isEmpty();
        if ((needToDownload && !_localData) || forceDownload)
        {
            auto filePath = localCachePath
                    + QDir::separator()
                    + (_localData ? "offline" : "online")
                    + QDir::separator()
                    + dateTimeStr + QStringLiteral("_")
                    + QString::number(zoom) + QStringLiteral("_")
                    + QString::number(tileId.x) + QStringLiteral("_")
                    + QString::number(15 - tileId.y)
                    + QStringLiteral(".tiff");

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

void OsmAnd::WeatherTileResourceProvider_P::lockRasterTile(const TileId tileId, const ZoomLevel zoom)
{
    QMutexLocker scopedLocker(&_rasterTilesInProcessMutex);

    while(_rasterTilesInProcess[zoom].contains(tileId))
        _waitUntilAnyRasterTileIsProcessed.wait(&_rasterTilesInProcessMutex);

    _rasterTilesInProcess[zoom].insert(tileId);
}

void OsmAnd::WeatherTileResourceProvider_P::unlockRasterTile(const TileId tileId, const ZoomLevel zoom)
{
    QMutexLocker scopedLocker(&_rasterTilesInProcessMutex);

    _rasterTilesInProcess[zoom].remove(tileId);

    _waitUntilAnyRasterTileIsProcessed.wakeAll();
}

void OsmAnd::WeatherTileResourceProvider_P::lockContourTile(const TileId tileId, const ZoomLevel zoom)
{
    QMutexLocker scopedLocker(&_contourTilesInProcessMutex);

    while(_contourTilesInProcess[zoom].contains(tileId))
        _waitUntilAnyContourTileIsProcessed.wait(&_contourTilesInProcessMutex);

    _contourTilesInProcess[zoom].insert(tileId);
}

void OsmAnd::WeatherTileResourceProvider_P::unlockContourTile(const TileId tileId, const ZoomLevel zoom)
{
    QMutexLocker scopedLocker(&_contourTilesInProcessMutex);

    _contourTilesInProcess[zoom].remove(tileId);

    _waitUntilAnyContourTileIsProcessed.wakeAll();
}

bool OsmAnd::WeatherTileResourceProvider_P::getCachedValues(const PointI point31, const ZoomLevel zoom, QList<double>& values)
{
    QReadLocker scopedLocker(&_cachedValuesLock);
        
    if (_cachedValuesPoint31 == point31 && _cachedValuesZoom == zoom)
    {
        values = _cachedValues;
        return true;
    }
    return false;
}

void OsmAnd::WeatherTileResourceProvider_P::setCachedValues(const PointI point31, const ZoomLevel zoom, const QList<double>& values)
{
    QWriteLocker scopedLocker(&_cachedValuesLock);
    
    _cachedValuesPoint31 = point31;
    _cachedValuesZoom = zoom;
    _cachedValues = values;
}

void OsmAnd::WeatherTileResourceProvider_P::obtainValue(
    const WeatherTileResourceProvider::ValueRequest& request,
    const WeatherTileResourceProvider::ObtainValueAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto requestClone = request.clone();
    ObtainValueTask *task = new ObtainValueTask(shared_from_this(), requestClone, callback, collectMetric);
    task->run();
    delete task;
}

void OsmAnd::WeatherTileResourceProvider_P::obtainValueAsync(
    const WeatherTileResourceProvider::ValueRequest& request,
    const WeatherTileResourceProvider::ObtainValueAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto requestClone = request.clone();
    ObtainValueTask *task = new ObtainValueTask(shared_from_this(), requestClone, callback, collectMetric);
    task->setAutoDelete(true);
    _obtainValueThreadPool->start(task, getAndDecreaseObtainValuePriority());
}

void OsmAnd::WeatherTileResourceProvider_P::obtainData(
    const WeatherTileResourceProvider::TileRequest& request,
    const WeatherTileResourceProvider::ObtainTileDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto requestClone = request.clone();
    if (requestClone->weatherType == WeatherType::Raster)
        requestClone->version = getAndUpdateRequestVersion(requestClone);

    ObtainTileTask *task = new ObtainTileTask(shared_from_this(), requestClone, callback, collectMetric);
    task->run();
    delete task;
}

void OsmAnd::WeatherTileResourceProvider_P::obtainDataAsync(
    const WeatherTileResourceProvider::TileRequest& request,
    const WeatherTileResourceProvider::ObtainTileDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto requestClone = request.clone();
    if (requestClone->weatherType == WeatherType::Raster)
        requestClone->version = getAndUpdateRequestVersion(requestClone);

    ObtainTileTask *task = new ObtainTileTask(shared_from_this(), requestClone, callback, collectMetric);
    task->setAutoDelete(true);
    _threadPool->start(task, getAndDecreasePriority());
}

void OsmAnd::WeatherTileResourceProvider_P::downloadGeoTiles(
    const WeatherTileResourceProvider::DownloadGeoTileRequest& request,
    const WeatherTileResourceProvider::DownloadGeoTilesAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto requestClone = request.clone();
    DownloadGeoTileTask *task = new DownloadGeoTileTask(shared_from_this(), requestClone, callback, collectMetric);
    task->run();
    delete task;
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

std::shared_ptr<OsmAnd::TileSqliteDatabase> OsmAnd::WeatherTileResourceProvider_P::getGeoTilesDatabase(bool localData)
{
    QString key = dateTime.toString(QStringLiteral("yyyyMMdd_hh00"));
    if (!localData)
        key.append(QStringLiteral("_online"));

    {
        QReadLocker geoScopedLocker(&_geoDbLock);

        const auto citGeoDb = _geoTilesDbMap.constFind(key);
        if (citGeoDb != _geoTilesDbMap.cend())
            return *citGeoDb;
    }
    {
        QWriteLocker geoScopedLocker(&_geoDbLock);

        auto db = createGeoTilesDatabase(localData);
        if (db)
            _geoTilesDbMap.insert(key, db);

        return db;
    }
}

std::shared_ptr<OsmAnd::TileSqliteDatabase> OsmAnd::WeatherTileResourceProvider_P::getRasterTilesDatabase(
        BandIndex band,
        bool localData)
{
    QString key = dateTime.toString(QStringLiteral("yyyyMMdd_hh00"))
            + QStringLiteral("_")
            + QString::number(band);
    if (!localData)
        key.append(QStringLiteral("_online"));

    {
        QReadLocker rasterScopedLocker(&_rasterDbLock);

        const auto citRasterDb = _rasterTilesDbMap.constFind(key);
        if (citRasterDb != _rasterTilesDbMap.cend())
            return *citRasterDb;
    }
    {
        QWriteLocker rasterScopedLocker(&_rasterDbLock);

        auto db = createRasterTilesDatabase(band, localData);
        if (db)
            _rasterTilesDbMap.insert(key, db);

        return db;
    }
}

bool OsmAnd::WeatherTileResourceProvider_P::storeLocalTileData(
        const TileId tileId,
        const ZoomLevel zoom,
        QByteArray& outData)
{
    bool res = false;
    lockGeoTile(tileId, zoom);

    auto geoDb = getGeoTilesDatabase(true);
    if (geoDb->isOpened())
        res = geoDb->storeTileData(tileId, zoom, outData);

    unlockGeoTile(tileId, zoom);
    return res;
}

bool OsmAnd::WeatherTileResourceProvider_P::containsLocalTileId(
        const TileId tileId,
        const QDateTime dataTime,
        const ZoomLevel zoom,
        QByteArray& outData)
{
    QReadLocker geoScopedLocker(&_geoDbLock);
    bool res = false;

    QString dateTimeStr = dataTime.toString(QStringLiteral("yyyyMMdd_hh00"));
    for (auto& db : _geoTilesDbMap.values())
    {
        if (!db->filename.contains("online") && db->filename.contains(dateTimeStr))
        {
            if (!db->isOpened())
                db->open();

            res = db->containsTileData(tileId, zoom);
            db->obtainTileData(tileId, zoom, outData);
        }
    }

    return res;
}

bool OsmAnd::WeatherTileResourceProvider_P::closeProvider(
        const TileId tileId,
        const ZoomLevel zoom)
{
    QWriteLocker geoScopedLocker(&_geoDbLock);

    for (auto& db : _geoTilesDbMap.values())
    {
        if (!db->filename.contains("online"))
        {
            if (!db->isOpened())
                db->open();

            db->removeTileData(tileId, zoom);
            TileSqliteDatabase::Meta meta;
            if (db->isEmpty() && db->close())
                _geoTilesDbMap.remove(_geoTilesDbMap.key(db));
        }
    }

    QWriteLocker rasterScopedLocker(&_rasterDbLock);

    for (auto& db : _rasterTilesDbMap.values())
    {
        if (!db->filename.contains("online"))
        {
            if (!db->isOpened())
                db->open();

            db->removeTileData(tileId, zoom);
            TileSqliteDatabase::Meta meta;
            if (db->isEmpty() && db->close())
                _geoTilesDbMap.remove(_geoTilesDbMap.key(db));
        }
    }

    return _geoTilesDbMap.size() == 0 && _geoTilesDbMap.size() == 0;
}

bool OsmAnd::WeatherTileResourceProvider_P::closeProvider(bool localData)
{
    QWriteLocker geoScopedLocker(&_geoDbLock);

    bool res = true;

    for (auto& db : _geoTilesDbMap.values())
    {
        if ((!localData && db->filename.contains("online")) || (localData && db->filename.contains("offline")))
        {
            if (db->close())
                _geoTilesDbMap.remove(_geoTilesDbMap.key(db));
            else
                res = false;
        }
    }

    QWriteLocker rasterScopedLocker(&_rasterDbLock);

    for (auto& db : _rasterTilesDbMap.values())
    {
        if ((!localData && db->filename.contains("online")) || (localData && db->filename.contains("offline")))
        {
            if (db->close())
                _rasterTilesDbMap.remove(_rasterTilesDbMap.key(db));
            else
                res = false;
        }
    }

    return res;
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
    
    QList<double> values;
    if (_provider->getCachedValues(point31, zoom, values))
    {
        callback(true, values, nullptr);
        return;
    }
    
    const auto geoTileZoom = WeatherTileResourceProvider::getGeoTileZoom();
    const auto latLon = Utilities::convert31ToLatLon(point31);
    const auto geoTileId = TileId::fromXY(
        Utilities::getTileNumberX(geoTileZoom, latLon.longitude),
        Utilities::getTileNumberY(geoTileZoom, latLon.latitude)
    );
    
    QByteArray geoTileData;
    if (_provider->obtainGeoTile(geoTileId, geoTileZoom, geoTileData, false))
    {
        GeoTileEvaluator *evaluator = new GeoTileEvaluator(
            geoTileId,
            geoTileData,
            zoom,
            _provider->projResourcesPath
        );
        if (evaluator->evaluate(latLon, values))
        {
            _provider->setCachedValues(point31, zoom, values);
            callback(true, values, nullptr);
        }
        else
        {
            callback(false, QList<double>(), nullptr);
        }
        delete evaluator;
    }
    else
    {
        callback(false, QList<double>(), nullptr);
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
    const auto& bandSettings = _provider->getBandSettings();
    for (auto band : bands)
    {
        const auto citImage = bandImages.constFind(band);
        if (citImage != bandImages.cend())
        {
            images << *citImage;
            alphas << bandSettings[band]->opacity;
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
    switch (request->weatherType)
    {
        case WeatherType::Raster:
            obtainRasterTile();
            break;
        case WeatherType::Contour:
            obtainContourTile();
            break;
        default:
            break;
    }
}

void OsmAnd::WeatherTileResourceProvider_P::ObtainTileTask::obtainRasterTile()
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
    
    _provider->lockRasterTile(tileId, zoom);

    QList<BandIndex> missingBands;
    QHash<BandIndex, sk_sp<const SkImage>> images;
    
    for (auto band : bands)
    {
        auto db = _provider->getRasterTilesDatabase(band, request->localData);
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
        _provider->unlockRasterTile(tileId, zoom);

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
        _provider->unlockRasterTile(tileId, zoom);

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
    
    if (!_provider->obtainGeoTile(geoTileId, geoTileZoom, geoTileData, false) || geoTileData.isEmpty())
    {
        _provider->unlockRasterTile(tileId, zoom);

        LogPrintf(LogSeverityLevel::Error,
            "GeoTile %dx%dx%d is empty", geoTileId.x, geoTileId.y, geoTileZoom);
        callback(false, nullptr, nullptr);
        return;
    }
    if (request->queryController && request->queryController->isAborted())
    {
        _provider->unlockRasterTile(tileId, zoom);

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
        _provider->getBandSettings(),
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
        _provider->unlockRasterTile(tileId, zoom);

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
        auto db = _provider->getRasterTilesDatabase(band, request->localData);
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
    
    _provider->unlockRasterTile(tileId, zoom);
    
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

void OsmAnd::WeatherTileResourceProvider_P::ObtainTileTask::obtainContourTile()
{
    TileId tileId = request->tileId;
    ZoomLevel zoom = request->zoom;
    auto bands = request->bands;

    if (request->queryController && request->queryController->isAborted())
    {
        LogPrintf(LogSeverityLevel::Debug,
            "Stop creating weather contour tile %dx%dx%d.", tileId.x, tileId.y, zoom);

        callback(false, nullptr, nullptr);
        return;
    }
    
    _provider->lockContourTile(tileId, zoom);

    QHash<BandIndex, QList<Ref<GeoContour>>> contourMap;
                
    ZoomLevel geoTileZoom = WeatherTileResourceProvider::getGeoTileZoom();
    QByteArray geoTileData;
    TileId geoTileId;
    if (zoom < geoTileZoom)
    {
        _provider->unlockContourTile(tileId, zoom);

        // Underzoom for geo tiles currently not supported
        LogPrintf(LogSeverityLevel::Error,
            "Failed to resolve geoTileId for weather contour tile %dx%dx%d. Geo tile zoom (%d) > tile zoom (%d).", tileId.x, tileId.y, zoom, geoTileZoom, zoom);
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
    
    if (!_provider->obtainGeoTile(geoTileId, geoTileZoom, geoTileData, false) || geoTileData.isEmpty())
    {
        _provider->unlockContourTile(tileId, zoom);

        LogPrintf(LogSeverityLevel::Error,
            "GeoTile %dx%dx%d is empty", geoTileId.x, geoTileId.y, geoTileZoom);
        callback(false, nullptr, nullptr);
        return;
    }
    if (request->queryController && request->queryController->isAborted())
    {
        _provider->unlockContourTile(tileId, zoom);

        LogPrintf(LogSeverityLevel::Debug,
            "Stop creating weather contour tile %dx%dx%d.", tileId.x, tileId.y, zoom);

        callback(false, nullptr, nullptr);
        return;
    }
    
    GeoTileRasterizer *rasterizer = new GeoTileRasterizer(
        geoTileData,
        request->tileId,
        request->zoom,
        bands,
        _provider->getBandSettings(),
        _provider->tileSize,
        _provider->densityFactor,
        _provider->projResourcesPath
    );
    
    const auto rasterizeQueryController = std::make_shared<FunctorQueryController>(
        [this]
        (const FunctorQueryController* const queryController) -> bool
        {
            return request->queryController && request->queryController->isAborted();
        });
    const auto evaluatedContours = rasterizer->evaluateContours(nullptr, rasterizeQueryController);
    delete rasterizer;

    if (request->queryController && request->queryController->isAborted())
    {
        _provider->unlockContourTile(tileId, zoom);

        LogPrintf(LogSeverityLevel::Debug,
            "Stop creating weather contour tile %dx%dx%d.", tileId.x, tileId.y, zoom);

        callback(false, nullptr, nullptr);
        return;
    }
    
    _provider->unlockContourTile(tileId, zoom);
    
    if (evaluatedContours.empty())
    {
        LogPrintf(LogSeverityLevel::Debug,
            "Failed to evaluate weather contour tile %dx%dx%d", tileId.x, tileId.y, zoom);
    }
    contourMap.insert(evaluatedContours);
    if (contourMap.empty())
    {
        LogPrintf(LogSeverityLevel::Debug,
            "Failed to evaluate weather contour tile %dx%dx%d", tileId.x, tileId.y, zoom);
        callback(false, nullptr, nullptr);
        return;
    }

    auto data = std::make_shared<OsmAnd::WeatherTileResourceProvider::Data>(
        tileId,
        zoom,
        AlphaChannelPresence::Present,
        _provider->densityFactor,
        nullptr,
        contourMap
    );
    callback(true, data, nullptr);
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
    QVector<TileId> geoTileIds = WeatherTileResourcesManager::generateGeoTileIds(topLeft, bottomRight, geoTileZoom);
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
