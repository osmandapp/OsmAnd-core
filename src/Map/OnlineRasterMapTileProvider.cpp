#include "OnlineRasterMapTileProvider.h"
#include "OnlineRasterMapTileProvider_P.h"

OsmAnd::OnlineRasterMapTileProvider::OnlineRasterMapTileProvider(
    const QString& name_,
    const QString& urlPattern_,
    const ZoomLevel minZoom_ /*= MinZoomLevel*/,
    const ZoomLevel maxZoom_ /*= MaxZoomLevel*/,
    const uint32_t maxConcurrentDownloads_ /*= 1*/,
    const uint32_t providerTileSize_ /*= 256*/,
    const AlphaChannelData alphaChannelData_ /*= AlphaChannelData::Undefined*/)
    : _p(new OnlineRasterMapTileProvider_P(this))
    , localCachePath(_p->_localCachePath)
    , networkAccessAllowed(_p->_networkAccessAllowed)
    , name(name_)
    , pathSuffix(QString(name).replace(QRegExp(QLatin1String("\\W+")), QLatin1String("_")))
    , urlPattern(urlPattern_)
    , minZoom(minZoom_)
    , maxZoom(maxZoom_)
    , maxConcurrentDownloads(maxConcurrentDownloads_)
    , providerTileSize(providerTileSize_)
    , alphaChannelData(alphaChannelData_)
{
    _p->_localCachePath = QDir(QDir::temp().absoluteFilePath(pathSuffix));
}

OsmAnd::OnlineRasterMapTileProvider::~OnlineRasterMapTileProvider()
{
}

void OsmAnd::OnlineRasterMapTileProvider::setLocalCachePath(const QDir& localCachePath, const bool appendPathSuffix /*= true*/)
{
    QMutexLocker scopedLocker(&_p->_localCachePathMutex);
    _p->_localCachePath = appendPathSuffix ? QDir(localCachePath.absoluteFilePath(pathSuffix)) : localCachePath;
}

void OsmAnd::OnlineRasterMapTileProvider::setNetworkAccessPermission(bool allowed)
{
    _p->_networkAccessAllowed = allowed;
}

bool OsmAnd::OnlineRasterMapTileProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const IQueryController* const queryController)
{
    return _p->obtainData(tileId, zoom, outTiledData, queryController);
}

float OsmAnd::OnlineRasterMapTileProvider::getTileDensityFactor() const
{
    // Online tile providers do not have any idea about our tile density
    return 1.0f;
}

uint32_t OsmAnd::OnlineRasterMapTileProvider::getTileSize() const
{
    return providerTileSize;
}

OsmAnd::ZoomLevel OsmAnd::OnlineRasterMapTileProvider::getMinZoom() const
{
    return minZoom;
}

OsmAnd::ZoomLevel OsmAnd::OnlineRasterMapTileProvider::getMaxZoom() const
{
    return maxZoom;
}
