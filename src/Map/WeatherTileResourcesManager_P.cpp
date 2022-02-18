#include "WeatherTileResourcesManager_P.h"
#include "WeatherTileResourcesManager.h"

#include "Utilities.h"
#include "Logging.h"

OsmAnd::WeatherTileResourcesManager_P::WeatherTileResourcesManager_P(
    WeatherTileResourcesManager* const owner_,
    const QHash<BandIndex, float>& bandOpacityMap_,
    const QHash<BandIndex, QString>& bandColorProfilePaths_,
    const QString& localCachePath_,
    const QString& projResourcesPath_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/,
    const std::shared_ptr<const IWebClient>& webClient_ /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
    : owner(owner_)
    , _bandOpacityMap(bandOpacityMap_)
    , webClient(webClient_)
    , bandColorProfilePaths(bandColorProfilePaths_)
    , localCachePath(localCachePath_)
    , projResourcesPath(projResourcesPath_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::WeatherTileResourcesManager_P::~WeatherTileResourcesManager_P()
{
}

std::shared_ptr<OsmAnd::WeatherTileResourceProvider> OsmAnd::WeatherTileResourcesManager_P::createResourceProvider(const QDateTime& dateTime)
{
    return std::make_shared<WeatherTileResourceProvider>(
        dateTime,
        _bandOpacityMap,
        bandColorProfilePaths,
        localCachePath,
        projResourcesPath,
        tileSize,
        densityFactor,
        webClient
    );
}

std::shared_ptr<OsmAnd::WeatherTileResourceProvider> OsmAnd::WeatherTileResourcesManager_P::getResourceProvider(const QDateTime& dateTime)
{
    auto dateTimeStr = dateTime.toString(QStringLiteral("yyyyMMdd_hh00"));
    {
        QReadLocker scopedLocker(&_resourceProvidersLock);

        const auto citResourceProvider = _resourceProviders.constFind(dateTimeStr);
        if (citResourceProvider != _resourceProviders.cend())
            return *citResourceProvider;
    }
    {
        QWriteLocker scopedLocker(&_resourceProvidersLock);

        auto resourceProvider = createResourceProvider(dateTime);
        _resourceProviders.insert(dateTimeStr, resourceProvider);
        return resourceProvider;
    }
}

void OsmAnd::WeatherTileResourcesManager_P::updateProvidersBandOpacityMap()
{
    QWriteLocker scopedLocker(&_resourceProvidersLock);

    const auto& bandOpacityMap = _bandOpacityMap;
    for (const auto& provider : _resourceProviders.values())
        provider->setBandOpacityMap(bandOpacityMap);
}

const QHash<OsmAnd::BandIndex, float> OsmAnd::WeatherTileResourcesManager_P::getBandOpacityMap() const
{
    return _bandOpacityMap;
}

void OsmAnd::WeatherTileResourcesManager_P::setBandOpacityMap(const QHash<BandIndex, float>& bandOpacityMap)
{
    _bandOpacityMap = bandOpacityMap;
    updateProvidersBandOpacityMap();
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourcesManager_P::getGeoTileZoom() const
{
    return WeatherTileResourceProvider::getGeoTileZoom();
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourcesManager_P::getTileZoom(const WeatherLayer layer) const
{
    return WeatherTileResourceProvider::getTileZoom(layer);
}

int OsmAnd::WeatherTileResourcesManager_P::getMaxMissingDataZoomShift(const WeatherLayer layer) const
{
    return WeatherTileResourceProvider::getMaxMissingDataZoomShift(layer);
}

int OsmAnd::WeatherTileResourcesManager_P::getMaxMissingDataUnderZoomShift(const WeatherLayer layer) const
{
    return WeatherTileResourceProvider::getMaxMissingDataUnderZoomShift(layer);
}


void OsmAnd::WeatherTileResourcesManager_P::obtainDataAsync(
    const WeatherTileResourcesManager::TileRequest& request,
    const WeatherTileResourcesManager::ObtainTileDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    auto resourceProvider = getResourceProvider(request.dataTime);
    if (resourceProvider)
    {
        WeatherTileResourceProvider::TileRequest rr;
        rr.tileId = request.tileId;
        rr.zoom = request.zoom;
        rr.bands = request.bands;
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
                        data->image
                    );
                    callback(requestSucceeded, d, metric);
                }
                else
                {
                    callback(false, nullptr, nullptr);
                }
            };
        
        resourceProvider->obtainDataAsync(rr, rc);
    }
    else
    {
        callback(false, nullptr, nullptr);
    }
}

void OsmAnd::WeatherTileResourcesManager_P::downloadGeoTilesAsync(
    const WeatherTileResourcesManager::DownloadGeoTileRequest& request,
    const WeatherTileResourcesManager::DownloadGeoTilesAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    auto resourceProvider = getResourceProvider(request.dataTime);
    if (resourceProvider)
    {
        WeatherTileResourceProvider::DownloadGeoTileRequest rr;
        rr.topLeft = request.topLeft;
        rr.bottomRight = request.bottomRight;
        rr.forceDownload = request.forceDownload;
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

bool OsmAnd::WeatherTileResourcesManager_P::clearDbCache(const bool clearGeoCache, const bool clearRasterCache)
{
    QWriteLocker scopedLocker(&_resourceProvidersLock);

    for (auto& provider : _resourceProviders.values())
        provider->closeProvider();

    _resourceProviders.clear();
    
    bool res = true;
    auto cacheDir = QDir(localCachePath);
    if (clearGeoCache)
    {
        QFileInfoList obfFileInfos;
        Utilities::findFiles(cacheDir, QStringList() << QStringLiteral("*.tiff.db"), obfFileInfos, false);
        for (const auto& obfFileInfo : constOf(obfFileInfos))
        {
            const auto filePath = obfFileInfo.absoluteFilePath();
            if (!QFile(filePath).remove())
            {
                res = false;
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to delete geo cache db file: %s", qPrintable(filePath));
                
            }
        }
    }
    if (clearRasterCache)
    {
        QFileInfoList obfFileInfos;
        Utilities::findFiles(cacheDir, QStringList() << QStringLiteral("*.raster.db"), obfFileInfos, false);
        for (const auto& obfFileInfo : constOf(obfFileInfos))
        {
            const auto filePath = obfFileInfo.absoluteFilePath();
            if (!QFile(filePath).remove())
            {
                res = false;
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to delete raster cache db file: %s", qPrintable(filePath));
                
            }
        }
    }
    return res;
}
