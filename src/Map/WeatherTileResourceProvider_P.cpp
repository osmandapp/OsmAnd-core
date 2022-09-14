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
    const QString& localCachePath_,
    const QString& projResourcesPath_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/,
    const std::shared_ptr<const IWebClient>& webClient_ /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
    : owner(owner_)
    , _threadPool(new QThreadPool())
    , _obtainValueThreadPool(new QThreadPool())
    , _bandSettings(bandSettings_)
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
    
    auto dateTimeStr = dateTime.toString(QStringLiteral("yyyyMMdd_hh00"));
    auto geoDbCachePath = localCachePath
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
            meta.setTileNumbering(QStringLiteral(""));
            _geoTilesDb->storeMeta(meta);
        }
    }
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
            meta.setTileNumbering(QStringLiteral(""));
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
        bool forceDownload /*= false*/,
        bool localData /*= false*/,
        std::shared_ptr<const IQueryController> queryController /*= nullptr*/)
{
    bool res = false;
    auto dateTimeStr = dateTime.toString(QStringLiteral("yyyyMMdd_hh00"));
    auto geoTileUrl = WEATHER_TILES_URL_PREFIX + dateTimeStr + "/"
        + QString::number(zoom) + QStringLiteral("_")
        + QString::number(tileId.x) + QStringLiteral("_")
        + QString::number(15 - tileId.y) + QStringLiteral(".tiff.gz");

    lockGeoTile(tileId, zoom);

    auto geoDb = getGeoTilesDatabase();
    if (geoDb->isOpened())
    {
        bool needToDownload = forceDownload;
        if (!forceDownload)
        {
            bool hasData = _geoTilesDb->obtainTileData(tileId, zoom, outData) && !outData.isEmpty();
            needToDownload = !hasData && !localData;
        }
        if (needToDownload)
        {
            auto filePath = localCachePath
                + QDir::separator()
                + dateTimeStr + QStringLiteral("_")
                + QString::number(zoom) + QStringLiteral("_")
                + QString::number(tileId.x) + QStringLiteral("_")
                + QString::number(15 - tileId.y)
                + QStringLiteral(".tiff");

            auto filePathGz = filePath + QStringLiteral(".gz");

            if (webClient->downloadFile(geoTileUrl, filePathGz, nullptr, nullptr, queryController))
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

std::shared_ptr<OsmAnd::TileSqliteDatabase> OsmAnd::WeatherTileResourceProvider_P::getGeoTilesDatabase()
{
    QReadLocker geoScopedLocker(&_geoDbLock);
    
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

bool OsmAnd::WeatherTileResourceProvider_P::isEmpty()
{
    QWriteLocker geoScopedLocker(&_geoDbLock);
    
    return _geoTilesDb->isEmpty();
}

uint64_t OsmAnd::WeatherTileResourceProvider_P::calculateTilesSize(
    const QList<TileId>& tileIds,
    const QList<TileId>& excludeTileIds,
    const ZoomLevel zoom,
    const bool rasterOnly /*= false*/)
{
    uint64_t size = 0;
    bool hasTileIds = !tileIds.isEmpty();
    if (!rasterOnly)
    {
        auto geoDb = getGeoTilesDatabase();
        if (geoDb)
        {
            QReadLocker geoScopedLocker(&_geoDbLock);

            auto geoDbCachePath = localCachePath
                    + QDir::separator()
                    + dateTime.toString(QStringLiteral("yyyyMMdd_hh00"))
                    + QStringLiteral(".tiff.db");

            QList<TileId> geoDBbTileIds;
            if (!geoDb->getTileIds(geoDBbTileIds, zoom))
            {
                LogPrintf(LogSeverityLevel::Error,
                        "Failed to get tile ids from weather geo cache db file: %s", qPrintable(geoDbCachePath));
            }
            else
            {
                QList<TileId> geoTileIdsToCalculate;
                for (const auto& geoDBTileId : constOf(geoDBbTileIds))
                {
                    if ((!hasTileIds || tileIds.contains(geoDBTileId)) && !excludeTileIds.contains(geoDBTileId))
                        geoTileIdsToCalculate.append(geoDBTileId);
                }
                if (!geoTileIdsToCalculate.isEmpty())
                    geoDb->getTilesSize(geoTileIdsToCalculate, size, zoom);
            }
        }
    }

    WeatherBand values[] = { WeatherBand::Cloud, WeatherBand::Temperature, WeatherBand::Pressure, WeatherBand::WindSpeed, WeatherBand::Precipitation };
    for (WeatherBand band : values)
    {
        auto rasterDbCachePath = localCachePath
            + QDir::separator()
            + dateTime.toString(QStringLiteral("yyyyMMdd_hh00"))
            + QStringLiteral("_")
            + QString::number(static_cast<BandIndex>(band))
            + QStringLiteral(".raster.db");

        if (QFile(rasterDbCachePath).exists())
        {
            auto rasterDb = getRasterTilesDatabase(static_cast<BandIndex>(band));
            if (rasterDb)
            {
                QReadLocker geoScopedLocker(&_rasterDbLock);

                QList<TileId> rasterDBTileIds;
                if (!rasterDb->getTileIds(rasterDBTileIds, zoom))
                {
                    LogPrintf(LogSeverityLevel::Error,
                            "Failed to get tile ids from weather raster cache db file: %s", qPrintable(rasterDbCachePath));
                }
                else
                {
                    QList<TileId> rasterTileIdsToCalculate = QList<TileId>();
                    const auto maxZoom = WeatherTileResourceProvider::getTileZoom(WeatherLayer::High);
                    int zoomShift = maxZoom - zoom;
                    for (const auto &rasterDBTileId : constOf(rasterDBTileIds))
                    {
                        if (((hasTileIds && tileIds.contains(rasterDBTileId)) || !hasTileIds) && !excludeTileIds.contains(rasterDBTileId))
                        {
                            rasterTileIdsToCalculate.append(rasterDBTileId);
                            if (zoomShift > 0)
                            {
                                const auto underscaleTiles = Utilities::getTileIdsUnderscaledByZoomShift(rasterDBTileId, zoomShift).toList();
                                size += calculateTilesSize(underscaleTiles, {}, maxZoom, true);
                            }
                        }
                    }
                    if (!rasterTileIdsToCalculate.isEmpty())
                    {
                        uint64_t rasterTilesSize = 0;
                        rasterDb->getTilesSize(rasterTileIdsToCalculate, rasterTilesSize, zoom);
                        size += rasterTilesSize;
                    }
                }
            }
        }
    }

    return size;
}

bool OsmAnd::WeatherTileResourceProvider_P::removeTileData(
    const QList<TileId>& tileIds,
    const QList<TileId>& excludeTileIds,
    const ZoomLevel zoom)
{
    bool res = false;
    auto geoDb = getGeoTilesDatabase();
    if (geoDb)
    {
        QWriteLocker geoScopedLocker(&_geoDbLock);

        res = removeTileIds(geoDb, tileIds, excludeTileIds, zoom);
    }

    WeatherBand values[] = { WeatherBand::Cloud, WeatherBand::Temperature, WeatherBand::Pressure, WeatherBand::WindSpeed, WeatherBand::Precipitation };
    for (WeatherBand band : values)
    {
        auto rasterDbCachePath = localCachePath
            + QDir::separator()
            + dateTime.toString(QStringLiteral("yyyyMMdd_hh00"))
            + QStringLiteral("_")
            + QString::number(static_cast<BandIndex>(band))
            + QStringLiteral(".raster.db");

        if (QFile(rasterDbCachePath).exists())
        {
            auto rasterTileDb = getRasterTilesDatabase(static_cast<BandIndex>(band));
            if (rasterTileDb)
            {
                QWriteLocker rasterScopedLocker(&_rasterDbLock);

                res |= removeTileIds(rasterTileDb, tileIds, excludeTileIds, zoom);
            }
        }
    }

    return res;
}

bool OsmAnd::WeatherTileResourceProvider_P::removeTileIds(
    const std::shared_ptr<TileSqliteDatabase>& tilesDb,
    const QList<TileId>& tileIds,
    const QList<TileId>& excludeTileIds,
    const ZoomLevel zoom)
{
    bool res = false;
    if (tilesDb->isEmpty())
    {
        res = true;
    }
    else
    {
        QList<TileId> dbTileIds;
        if (!tilesDb->getTileIds(dbTileIds, zoom))
        {
            LogPrintf(LogSeverityLevel::Error,
                      "Failed to get tile ids from weather cache db file: %s", qPrintable(tilesDb->filename));
        }
        else
        {
            bool hasTileIds = !tileIds.isEmpty();
            QList<TileId> tilesToDelete;
            for (auto &dbTileId : dbTileIds)
            {
                bool shouldDelete = ((hasTileIds && tileIds.contains(dbTileId)) || !hasTileIds) && !excludeTileIds.contains(dbTileId);
                if (shouldDelete)
                {
                    tilesToDelete.append(dbTileId);
                    res |= tilesDb->removeTileData(dbTileId, zoom);
                }
            }

            if (tilesToDelete.count() > 0)
            {
                const auto maxZoom = WeatherTileResourceProvider::getTileZoom(WeatherLayer::High);
                    QList<TileId> dbMaxZoomTileIds;
                if (tilesDb->getTileIds(dbMaxZoomTileIds, maxZoom))
                {
                    for (auto &dbMaxZoomTileId : dbMaxZoomTileIds)
                    {
                        const auto overscaledTileId = Utilities::getTileIdOverscaledByZoomShift(dbMaxZoomTileId, maxZoom - zoom);
                        if (tilesToDelete.contains(overscaledTileId))
                            res |= tilesDb->removeTileData(dbMaxZoomTileId, maxZoom);
                    }
                }
            }

            if (res)
                tilesDb->compact();
        }
    }

    return res;
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
    bool localData = request->localData;

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
    if (_provider->obtainGeoTile(geoTileId, geoTileZoom, geoTileData, false, localData))
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
    bool localData = request->localData;

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

    if (!_provider->obtainGeoTile(geoTileId, geoTileZoom, geoTileData, false, localData) || geoTileData.isEmpty())
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
    bool localData = request->localData;

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
    
    if (!_provider->obtainGeoTile(geoTileId, geoTileZoom, geoTileData, false, localData) || geoTileData.isEmpty())
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
    bool localData = request->localData;
    auto dateTimeStr = _provider->dateTime.toString(QStringLiteral("yyyyMMdd_hh00"));

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
        if (request->queryController && request->queryController->isAborted())
        {
            LogPrintf(LogSeverityLevel::Debug,
                    "Cancel downloading weather tiles area %f, %f / %f, %f for %s",
                    topLeft.latitude, topLeft.longitude, bottomRight.latitude, bottomRight.longitude, qPrintable(dateTimeStr));
            return;
        }

        QByteArray data;
        bool res = _provider->obtainGeoTile(tileId, geoTileZoom, data, request->forceDownload, localData, request->queryController);

        callback(res, ++downloadedTiles, tilesCount, nullptr);
    }
}
