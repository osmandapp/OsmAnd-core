#include "OnlineMapRasterTileProvider.h"
#include "OnlineMapRasterTileProvider_P.h"

#include <cassert>

#include "Logging.h"

OsmAnd::OnlineMapRasterTileProvider::OnlineMapRasterTileProvider(
    const QString& name_,
    const QString& urlPattern_,
    const ZoomLevel minZoom_ /*= MinZoomLevel*/,
    const ZoomLevel maxZoom_ /*= MaxZoomLevel*/,
    const uint32_t maxConcurrentDownloads_ /*= 1*/,
    const uint32_t providerTileSize_ /*= 256*/,
    const AlphaChannelData alphaChannelData_ /*= AlphaChannelData::Undefined*/)
    : _p(new OnlineMapRasterTileProvider_P(this))
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

OsmAnd::OnlineMapRasterTileProvider::~OnlineMapRasterTileProvider()
{
}

void OsmAnd::OnlineMapRasterTileProvider::setLocalCachePath(const QDir& localCachePath, const bool appendPathSuffix /*= true*/)
{
    QMutexLocker scopedLocker(&_p->_localCachePathMutex);
    _p->_localCachePath = appendPathSuffix ? QDir(localCachePath.absoluteFilePath(pathSuffix)) : localCachePath;
}

void OsmAnd::OnlineMapRasterTileProvider::setNetworkAccessPermission(bool allowed)
{
    _p->_networkAccessAllowed = allowed;
}

bool OsmAnd::OnlineMapRasterTileProvider::obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile, const IQueryController* const queryController)
{
    return _p->obtainTile(tileId, zoom, outTile, queryController);
}

float OsmAnd::OnlineMapRasterTileProvider::getTileDensity() const
{
    // Online tile providers do not have any idea about our tile density
    return 1.0f;
}

uint32_t OsmAnd::OnlineMapRasterTileProvider::getTileSize() const
{
    return providerTileSize;
}

OsmAnd::ZoomLevel OsmAnd::OnlineMapRasterTileProvider::getMinZoom() const
{
    return minZoom;
}

OsmAnd::ZoomLevel OsmAnd::OnlineMapRasterTileProvider::getMaxZoom() const
{
    return maxZoom;
}
