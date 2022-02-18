#include "WeatherTileResourcesManager.h"
#include "WeatherTileResourcesManager_P.h"

#include "Logging.h"

OsmAnd::WeatherTileResourcesManager::WeatherTileResourcesManager(
    const QHash<BandIndex, float>& bandOpacityMap,
    const QHash<BandIndex, QString>& bandColorProfilePaths,
    const QString& localCachePath,
    const QString& projResourcesPath,
    const uint32_t tileSize /*= 256*/,
    const float densityFactor /*= 1.0f*/,
    const std::shared_ptr<const IWebClient>& webClient /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
    : _p(new WeatherTileResourcesManager_P(this, bandOpacityMap, bandColorProfilePaths, localCachePath, projResourcesPath, tileSize, densityFactor, webClient))
    , networkAccessAllowed(true)
{
}

OsmAnd::WeatherTileResourcesManager::~WeatherTileResourcesManager()
{
}

QHash<OsmAnd::BandIndex, float> OsmAnd::WeatherTileResourcesManager::getBandOpacityMap() const
{
    return _p->getBandOpacityMap();
}

void OsmAnd::WeatherTileResourcesManager::setBandOpacityMap(const QHash<BandIndex, float>& bandOpacityMap)
{
    _p->setBandOpacityMap(bandOpacityMap);
}

QHash<OsmAnd::BandIndex, QString> OsmAnd::WeatherTileResourcesManager::getBandColorProfilePaths() const
{
    return _p->bandColorProfilePaths;
}

QString OsmAnd::WeatherTileResourcesManager::getLocalCachePath() const
{
    return _p->localCachePath;
}

QString OsmAnd::WeatherTileResourcesManager::getProjResourcesPath() const
{
    return _p->projResourcesPath;
}

uint32_t OsmAnd::WeatherTileResourcesManager::getTileSize() const
{
    return _p->tileSize;
}

float OsmAnd::WeatherTileResourcesManager::getDensityFactor() const
{
    return _p->densityFactor;
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourcesManager::getGeoTileZoom() const
{
    return _p->getGeoTileZoom();
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourcesManager::getTileZoom(const WeatherLayer layer) const
{
    return _p->getTileZoom(layer);
}

int OsmAnd::WeatherTileResourcesManager::getMaxMissingDataZoomShift(const WeatherLayer layer) const
{
    return _p->getMaxMissingDataZoomShift(layer);
}

int OsmAnd::WeatherTileResourcesManager::getMaxMissingDataUnderZoomShift(const WeatherLayer layer) const
{
    return _p->getMaxMissingDataUnderZoomShift(layer);
}

void OsmAnd::WeatherTileResourcesManager::obtainDataAsync(
    const TileRequest& request,
    const ObtainTileDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    if (request.zoom != getTileZoom(request.weatherLayer))
    {
        callback(false, nullptr, nullptr);
        return;
    }

    _p->obtainDataAsync(request, callback, collectMetric);
}

void OsmAnd::WeatherTileResourcesManager::downloadGeoTilesAsync(
    const DownloadGeoTileRequest& request,
    const DownloadGeoTilesAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->downloadGeoTilesAsync(request, callback, collectMetric);
}

bool OsmAnd::WeatherTileResourcesManager::clearDbCache(const bool clearGeoCache, const bool clearRasterCache)
{
    return _p->clearDbCache(clearGeoCache, clearRasterCache);
}

OsmAnd::WeatherTileResourcesManager::TileRequest::TileRequest()
    : weatherLayer(WeatherLayer::Undefined)
    , tileId(TileId::zero())
    , zoom(InvalidZoomLevel)
{
}

OsmAnd::WeatherTileResourcesManager::TileRequest::TileRequest(const TileRequest& that)
{
    copy(*this, that);
}

OsmAnd::WeatherTileResourcesManager::TileRequest::~TileRequest()
{
}

void OsmAnd::WeatherTileResourcesManager::TileRequest::copy(TileRequest& dst, const TileRequest& src)
{
    dst.weatherLayer = src.weatherLayer;
    dst.dataTime = src.dataTime;
    dst.tileId = src.tileId;
    dst.zoom = src.zoom;
    dst.bands = src.bands;
    dst.queryController = src.queryController;
}

std::shared_ptr<OsmAnd::WeatherTileResourcesManager::TileRequest> OsmAnd::WeatherTileResourcesManager::TileRequest::clone() const
{
    return std::shared_ptr<TileRequest>(new TileRequest(*this));
}

OsmAnd::WeatherTileResourcesManager::DownloadGeoTileRequest::DownloadGeoTileRequest()
    : forceDownload(false)
{
}

OsmAnd::WeatherTileResourcesManager::DownloadGeoTileRequest::DownloadGeoTileRequest(const DownloadGeoTileRequest& that)
{
    copy(*this, that);
}

OsmAnd::WeatherTileResourcesManager::DownloadGeoTileRequest::~DownloadGeoTileRequest()
{
}

void OsmAnd::WeatherTileResourcesManager::DownloadGeoTileRequest::copy(DownloadGeoTileRequest& dst, const DownloadGeoTileRequest& src)
{
    dst.dataTime = src.dataTime;
    dst.topLeft = src.topLeft;
    dst.bottomRight = src.bottomRight;
    dst.forceDownload = src.forceDownload;
    dst.queryController = src.queryController;
}

std::shared_ptr<OsmAnd::WeatherTileResourcesManager::DownloadGeoTileRequest> OsmAnd::WeatherTileResourcesManager::DownloadGeoTileRequest::clone() const
{
    return std::shared_ptr<DownloadGeoTileRequest>(new DownloadGeoTileRequest(*this));
}

OsmAnd::WeatherTileResourcesManager::Data::Data(
    TileId tileId_,
    ZoomLevel zoom_,
    AlphaChannelPresence alphaChannelPresence_,
    float densityFactor_,
    sk_sp<const SkImage> image_)
    : tileId(tileId_)
    , zoom(zoom_)
    , alphaChannelPresence(alphaChannelPresence_)
    , densityFactor(densityFactor_)
    , image(image_)
{
}

OsmAnd::WeatherTileResourcesManager::Data::~Data()
{
}
