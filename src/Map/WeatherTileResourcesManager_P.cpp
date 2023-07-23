#include "WeatherTileResourcesManager_P.h"
#include "WeatherTileResourcesManager.h"

#include "Utilities.h"
#include "Logging.h"
#include "WeatherDataConverter.h"
#include "TileSqliteDatabase.h"

OsmAnd::WeatherTileResourcesManager_P::WeatherTileResourcesManager_P(
    WeatherTileResourcesManager* const owner_,
    const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings_,
    const QString& localCachePath_,
    const QString& projResourcesPath_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/,
    const std::shared_ptr<const IWebClient>& webClient_ /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
    : owner(owner_)
    , _bandSettings(bandSettings_)
    , webClient(webClient_)
    , localCachePath(localCachePath_)
    , projResourcesPath(projResourcesPath_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::WeatherTileResourcesManager_P::~WeatherTileResourcesManager_P()
{
}

std::shared_ptr<OsmAnd::WeatherTileResourceProvider> OsmAnd::WeatherTileResourcesManager_P::createResourceProvider()
{
    return std::make_shared<WeatherTileResourceProvider>(
        _bandSettings,
        localCachePath,
        projResourcesPath,
        tileSize,
        densityFactor,
        webClient
    );
}

std::shared_ptr<OsmAnd::WeatherTileResourceProvider> OsmAnd::WeatherTileResourcesManager_P::getResourceProvider()
{
    {
        QReadLocker scopedLocker(&_resourceProviderLock);

        if (_resourceProvider)
            return _resourceProvider;
    }
    {
        QWriteLocker scopedLocker(&_resourceProviderLock);

        if (_resourceProvider)
            return _resourceProvider;

        _resourceProvider = createResourceProvider();
        return _resourceProvider;
    }
}

void OsmAnd::WeatherTileResourcesManager_P::updateProvidersBandSettings()
{    
    QWriteLocker scopedLocker(&_resourceProviderLock);

    if (_resourceProvider)
        _resourceProvider->setBandSettings(_bandSettings);
}

const QHash<OsmAnd::BandIndex,
    std::shared_ptr<const OsmAnd::GeoBandSettings>> OsmAnd::WeatherTileResourcesManager_P::getBandSettings() const
{
    QReadLocker scopedLocker(&_bandSettingsLock);

    return _bandSettings;
}

void OsmAnd::WeatherTileResourcesManager_P::setBandSettings(
    const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings)
{
    QWriteLocker scopedLocker(&_bandSettingsLock);

    _bandSettings = bandSettings;
    updateProvidersBandSettings();
}

double OsmAnd::WeatherTileResourcesManager_P::getConvertedBandValue(const BandIndex band, const double value) const
{
    double result = value;
    const auto bandSettings = getBandSettings();
    if (bandSettings.contains(band))
    {
        const auto settings = bandSettings[band];
        const auto& unit = settings->unit;
        const auto& interalUnit = settings->internalUnit;
        if (unit != interalUnit)
        {
            const auto weatherBand = static_cast<WeatherBand>(band);
            switch (weatherBand)
            {
                case WeatherBand::Cloud:
                    // Assume cloud in % only
                    break;
                case WeatherBand::Temperature:
                {
                    const auto unit_ = WeatherDataConverter::Temperature::unitFromString(unit);
                    const auto interalUnit_ = WeatherDataConverter::Temperature::unitFromString(interalUnit);
                    const auto *temp = new WeatherDataConverter::Temperature(interalUnit_, value);
                    result = temp->toUnit(unit_);
                    delete temp;
                    break;
                }
                case WeatherBand::Pressure:
                {
                    const auto unit_ = WeatherDataConverter::Pressure::unitFromString(unit);
                    const auto interalUnit_ = WeatherDataConverter::Pressure::unitFromString(interalUnit);
                    const auto *pressure = new WeatherDataConverter::Pressure(interalUnit_, value);
                    result = pressure->toUnit(unit_);
                    delete pressure;
                    break;
                }
                case WeatherBand::WindSpeed:
                {
                    const auto unit_ = WeatherDataConverter::Speed::unitFromString(unit);
                    const auto interalUnit_ = WeatherDataConverter::Speed::unitFromString(interalUnit);
                    const auto *speed = new WeatherDataConverter::Speed(interalUnit_, value);
                    result = speed->toUnit(unit_);
                    delete speed;
                    break;
                }
                case WeatherBand::Precipitation:
                {
                    const auto unit_ = WeatherDataConverter::Precipitation::unitFromString(unit);
                    const auto interalUnit_ = WeatherDataConverter::Precipitation::unitFromString(interalUnit);
                    const auto *precip = new WeatherDataConverter::Precipitation(interalUnit_, value);
                    result = precip->toUnit(unit_);
                    delete precip;
                    break;
                }
                case WeatherBand::Undefined:
                    break;
            }
        }
    }
    return result;
}

QString OsmAnd::WeatherTileResourcesManager_P::getFormattedBandValue(
    const BandIndex band, const double value, const bool precise) const
{
    const auto bandSettings = getBandSettings();
    if (bandSettings.contains(band))
    {
        const auto settings = bandSettings[band];
        const auto& format = precise ? settings->unitFormatPrecise : settings->unitFormatGeneral;
        int roundValue = (int)(value + 0.5);
        // d i u o x X - int
        if (format.contains('d')
            || format.contains('i')
            || format.contains('u')
            || format.contains('o')
            || format.contains('x')
            || format.contains('X'))
        {
            return QString::asprintf(qPrintable(format), roundValue);
        }
        else
        {
            auto intStr = QString::asprintf(qPrintable(format), (double)roundValue);
            auto floatStr = QString::asprintf(qPrintable(format), value);
            return intStr == floatStr ? QString::number(roundValue) : floatStr;
        }
    }
    return QString::number(value);
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourcesManager_P::getGeoTileZoom() const
{
    return WeatherTileResourceProvider::getGeoTileZoom();
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourcesManager_P::getMinTileZoom(
    const WeatherType type, const WeatherLayer layer) const
{
    switch (type)
    {
        case WeatherType::Raster:
            return WeatherTileResourceProvider::getTileZoom(layer);
        case WeatherType::Contour:
            return WeatherTileResourceProvider::getTileZoom(WeatherLayer::Low);
        default:
            return ZoomLevel::MinZoomLevel;
    }
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourcesManager_P::getMaxTileZoom(
    const WeatherType type, const WeatherLayer layer) const
{
    switch (type)
    {
        case WeatherType::Raster:
            return WeatherTileResourceProvider::getTileZoom(layer);
        case WeatherType::Contour:
            return (OsmAnd::ZoomLevel)(WeatherTileResourceProvider::getTileZoom(WeatherLayer::High)
                + WeatherTileResourceProvider::getMaxMissingDataZoomShift(WeatherLayer::High));
        default:
            return ZoomLevel::MaxZoomLevel;
    }
}

int OsmAnd::WeatherTileResourcesManager_P::getMaxMissingDataZoomShift(
    const WeatherType type, const WeatherLayer layer) const
{
    switch (type)
    {
        case WeatherType::Raster:
            return WeatherTileResourceProvider::getMaxMissingDataZoomShift(layer);
        case WeatherType::Contour:
            return 0;
        default:
            return 0;
    }
}

int OsmAnd::WeatherTileResourcesManager_P::getMaxMissingDataUnderZoomShift(
    const WeatherType type, const WeatherLayer layer) const
{
    switch (type)
    {
        case WeatherType::Raster:
            return WeatherTileResourceProvider::getMaxMissingDataUnderZoomShift(layer);
        case WeatherType::Contour:
            return 0;
        default:
            return 0;
    }
}

bool OsmAnd::WeatherTileResourcesManager_P::isDownloadingTiles(const int64_t dateTime)
{
    auto resourceProvider = getResourceProvider();
    return resourceProvider && resourceProvider->isDownloadingTiles();
}

bool OsmAnd::WeatherTileResourcesManager_P::isEvaluatingTiles(const int64_t dateTime)
{
    auto resourceProvider = getResourceProvider();
    return resourceProvider && resourceProvider->isEvaluatingTiles();
}

QList<OsmAnd::TileId> OsmAnd::WeatherTileResourcesManager_P::getCurrentDownloadingTileIds(const int64_t dateTime)
{
    auto resourceProvider = getResourceProvider();
    return resourceProvider ? resourceProvider->getCurrentDownloadingTileIds() : QList<OsmAnd::TileId>();
}

QList<OsmAnd::TileId> OsmAnd::WeatherTileResourcesManager_P::getCurrentEvaluatingTileIds(const int64_t dateTime)
{
    auto resourceProvider = getResourceProvider();
    return resourceProvider ? resourceProvider->getCurrentEvaluatingTileIds() : QList<OsmAnd::TileId>();
}

void OsmAnd::WeatherTileResourcesManager_P::obtainValue(
    const WeatherTileResourcesManager::ValueRequest& request,
    const WeatherTileResourcesManager::ObtainValueAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    auto resourceProvider = getResourceProvider();
    if (resourceProvider)
    {
        WeatherTileResourceProvider::ValueRequest rr;
        rr.dateTime = request.dateTime;
        rr.point31 = request.point31;
        rr.zoom = request.zoom;
        rr.band = request.band;
        rr.localData = request.localData;
        rr.queryController = request.queryController;
        
        const auto point31 = request.point31;
        const auto dateTime = request.dateTime;
        const auto band = request.band;
        const WeatherTileResourceProvider::ObtainValueAsyncCallback rc =
            [callback, point31, dateTime, band]
            (const bool requestSucceeded,
                const QList<double>& values,
                const std::shared_ptr<Metric>& metric)
            {
                callback(requestSucceeded, point31, dateTime, values[band], nullptr);
            };
        
        resourceProvider->obtainValue(rr, rc);
    }
    else
    {
        callback(false, request.point31, request.dateTime, 0, nullptr);
    }
}

void OsmAnd::WeatherTileResourcesManager_P::obtainValueAsync(
    const WeatherTileResourcesManager::ValueRequest& request,
    const WeatherTileResourcesManager::ObtainValueAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    auto resourceProvider = getResourceProvider();
    if (resourceProvider)
    {
        WeatherTileResourceProvider::ValueRequest rr;
        rr.clientId = request.clientId;
        rr.dateTime = request.dateTime;
        rr.point31 = request.point31;
        rr.zoom = request.zoom;
        rr.band = request.band;
        rr.localData = request.localData;
        rr.abortIfNotRecent = request.abortIfNotRecent;
        rr.queryController = request.queryController;
     
        const auto point31 = request.point31;
        const auto dateTime = request.dateTime;
        const auto band = request.band;
        const WeatherTileResourceProvider::ObtainValueAsyncCallback rc =
            [callback, point31, dateTime, band]
            (const bool requestSucceeded,
                const QList<double>& values,
                const std::shared_ptr<Metric>& metric)
            {
                if (requestSucceeded)
                    callback(true, point31, dateTime, values[band], nullptr);
                else
                    callback(false, point31, dateTime, 0, nullptr);
            };
        
        resourceProvider->obtainValueAsync(rr, rc);
    }
    else
    {
        callback(false, request.point31, request.dateTime, 0, nullptr);
    }
}

void OsmAnd::WeatherTileResourcesManager_P::obtainData(
    const WeatherTileResourcesManager::TileRequest& request,
    const WeatherTileResourcesManager::ObtainTileDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    auto resourceProvider = getResourceProvider();
    if (resourceProvider)
    {
        WeatherTileResourceProvider::TileRequest rr;
        rr.dateTime = request.dateTime;
        rr.weatherType = request.weatherType;
        rr.tileId = request.tileId;
        rr.zoom = request.zoom;
        rr.bands = request.bands;
        rr.localData = request.localData;
        rr.queryController = request.queryController;
        
        const WeatherTileResourceProvider::ObtainTileDataAsyncCallback rc =
            [callback]
            (const bool requestSucceeded,
                const std::shared_ptr<WeatherTileResourceProvider::Data>& data,
                const std::shared_ptr<Metric>& metric)
            {
                if (data)
                {
                    const auto d = std::make_shared<WeatherTileResourcesManager::Data>(
                        data->tileId,
                        data->zoom,
                        data->alphaChannelPresence,
                        data->densityFactor,
                        data->image,
                        data->contourMap
                    );
                    callback(requestSucceeded, d, metric);
                }
                else
                {
                    callback(false, nullptr, nullptr);
                }
            };
        
        resourceProvider->obtainData(rr, rc);
    }
    else
    {
        callback(false, nullptr, nullptr);
    }
}

void OsmAnd::WeatherTileResourcesManager_P::obtainDataAsync(
    const WeatherTileResourcesManager::TileRequest& request,
    const WeatherTileResourcesManager::ObtainTileDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    auto resourceProvider = getResourceProvider();
    if (resourceProvider)
    {
        WeatherTileResourceProvider::TileRequest rr;
        rr.dateTime = request.dateTime;
        rr.weatherType = request.weatherType;
        rr.tileId = request.tileId;
        rr.zoom = request.zoom;
        rr.bands = request.bands;
        rr.localData = request.localData;
        rr.cacheOnly = request.cacheOnly;
        rr.queryController = request.queryController;
        
        const WeatherTileResourceProvider::ObtainTileDataAsyncCallback rc =
            [callback]
            (const bool requestSucceeded,
                const std::shared_ptr<WeatherTileResourceProvider::Data>& data,
                const std::shared_ptr<Metric>& metric)
            {
                if (data)
                {
                    const auto d = std::make_shared<WeatherTileResourcesManager::Data>(
                        data->tileId,
                        data->zoom,
                        data->alphaChannelPresence,
                        data->densityFactor,
                        data->image,
                        data->contourMap
                    );
                    callback(requestSucceeded, d, metric);
                }
                else
                {
                    callback(requestSucceeded, nullptr, nullptr);
                }
            };
        
        resourceProvider->obtainDataAsync(rr, rc);
    }
    else
    {
        callback(false, nullptr, nullptr);
    }
}

void OsmAnd::WeatherTileResourcesManager_P::downloadGeoTiles(
    const WeatherTileResourcesManager::DownloadGeoTileRequest& request,
    const WeatherTileResourcesManager::DownloadGeoTilesAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    auto resourceProvider = getResourceProvider();
    if (resourceProvider)
    {
        WeatherTileResourceProvider::DownloadGeoTileRequest rr;
        rr.dateTime = request.dateTime;
        rr.topLeft = request.topLeft;
        rr.bottomRight = request.bottomRight;
        rr.forceDownload = request.forceDownload;
        rr.localData = request.localData;
        rr.queryController = request.queryController;

        const WeatherTileResourceProvider::DownloadGeoTilesAsyncCallback rc =
            [callback]
            (const bool succeeded,
                const uint64_t downloadedTiles,
                const uint64_t totalTiles,
                const std::shared_ptr<Metric>& metric)
            {
                callback(succeeded, downloadedTiles, totalTiles, metric);
            };

        resourceProvider->downloadGeoTiles(rr, rc);
    }
    else
    {
        callback(false, 0, 0, nullptr);
    }
}

void OsmAnd::WeatherTileResourcesManager_P::downloadGeoTilesAsync(
    const WeatherTileResourcesManager::DownloadGeoTileRequest& request,
    const WeatherTileResourcesManager::DownloadGeoTilesAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    auto resourceProvider = getResourceProvider();
    if (resourceProvider)
    {
        WeatherTileResourceProvider::DownloadGeoTileRequest rr;
        rr.dateTime = request.dateTime;
        rr.topLeft = request.topLeft;
        rr.bottomRight = request.bottomRight;
        rr.forceDownload = request.forceDownload;
        rr.localData = request.localData;
        rr.queryController = request.queryController;

        const WeatherTileResourceProvider::DownloadGeoTilesAsyncCallback rc =
            [callback]
            (const bool succeeded,
                const uint64_t downloadedTiles,
                const uint64_t totalTiles,
                const std::shared_ptr<Metric>& metric)
            {
                callback(succeeded, downloadedTiles, totalTiles, metric);
            };

        resourceProvider->downloadGeoTilesAsync(rr, rc);
    }
    else
    {
        callback(false, 0, 0, nullptr);
    }
}

bool OsmAnd::WeatherTileResourcesManager_P::importDbCache(const QString& dbFilePath)
{
    auto resourceProvider = getResourceProvider();
    if (resourceProvider)
        return resourceProvider->importTileData(dbFilePath);
    return false;
}

uint64_t OsmAnd::WeatherTileResourcesManager_P::calculateDbCacheSize(
    const QList<TileId>& tileIds,
    const QList<TileId>& excludeTileIds,
    const ZoomLevel zoom)
{
    uint64_t size = 0;
    auto resourceProvider = getResourceProvider();
    if (resourceProvider)
        size += resourceProvider->calculateTilesSize(tileIds, excludeTileIds, zoom, 0);

    return size;
}

bool OsmAnd::WeatherTileResourcesManager_P::clearDbCache(
    const QList<TileId>& tileIds,
    const QList<TileId>& excludeTileIds,
    const ZoomLevel zoom)
{
    auto resourceProvider = getResourceProvider();
    if (resourceProvider)
    {
        QWriteLocker scopedLocker(&_resourceProviderLock);

        if (!resourceProvider->removeTileData(tileIds, excludeTileIds, zoom, 0))
        {
            LogPrintf(LogSeverityLevel::Error,
                    "Failed to delete tile ids from weather resource provider");
        }

        bool isEmpty = resourceProvider->isEmpty();
        if (isEmpty)
        {
            resourceProvider->closeProvider();
            _resourceProvider.reset();

            auto geoDbCachePath = localCachePath
            + QDir::separator()
            + QStringLiteral("weather_tiffs.db");

            if (!QFile(geoDbCachePath).remove())
            {
                LogPrintf(LogSeverityLevel::Error,
                        "Failed to delete weather geo cache db file: %s", qPrintable(geoDbCachePath));
            }
            else
            {
                QFileInfoList rasterFileInfos;
                Utilities::findFiles(QDir(localCachePath),
                    QStringList() << (QStringLiteral("weather_cache*.db")), rasterFileInfos, false);
                for (const auto &rasterFileInfo: constOf(rasterFileInfos))
                {
                    auto rasterDbCachePath = rasterFileInfo.absoluteFilePath();
                    if (!QFile(rasterDbCachePath).remove())
                    {
                        LogPrintf(LogSeverityLevel::Error,
                                "Failed to delete weather raster cache db file: %s", qPrintable(rasterDbCachePath));
                    }
                }
            }
        }
    }

    return true;
}

bool OsmAnd::WeatherTileResourcesManager_P::clearDbCache(int64_t beforeDateTime /*= 0*/)
{
    if (beforeDateTime == 0)
        beforeDateTime = QDateTime().toUTC().toMSecsSinceEpoch();

    bool clearBefore = beforeDateTime > 0;

    bool res = true;
    auto resourceProvider = getResourceProvider();
    if (resourceProvider)
    {
        QWriteLocker scopedLocker(&_resourceProviderLock);

        if (!resourceProvider->removeTileDataBefore(beforeDateTime))
        {
            res = false;
            LogPrintf(LogSeverityLevel::Error,
                    "Failed to delete tile ids from weather resource provider");
        }

        bool isEmpty = resourceProvider->isEmpty();
        if (isEmpty)
        {
            resourceProvider->closeProvider();
            _resourceProvider.reset();

            auto geoDbCachePath = localCachePath
            + QDir::separator()
            + QStringLiteral("weather_tiffs.db");

            if (!QFile(geoDbCachePath).remove())
            {
                res = false;
                LogPrintf(LogSeverityLevel::Error,
                        "Failed to delete weather geo cache db file: %s", qPrintable(geoDbCachePath));
            }
            else
            {
                QFileInfoList rasterFileInfos;
                Utilities::findFiles(QDir(localCachePath),
                    QStringList() << (QStringLiteral("weather_cache*.db")), rasterFileInfos, false);
                for (const auto &rasterFileInfo: constOf(rasterFileInfos))
                {
                    auto rasterDbCachePath = rasterFileInfo.absoluteFilePath();
                    if (!QFile(rasterDbCachePath).remove())
                    {
                        res = false;
                        LogPrintf(LogSeverityLevel::Error,
                                "Failed to delete weather raster cache db file: %s", qPrintable(rasterDbCachePath));
                    }
                }
            }
        }
    }

    return res;
}
