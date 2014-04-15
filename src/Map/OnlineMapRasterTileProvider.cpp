#include "OnlineMapRasterTileProvider.h"
#include "OnlineMapRasterTileProvider_P.h"

#include <cassert>

#include "Logging.h"

OsmAnd::OnlineMapRasterTileProvider::OnlineMapRasterTileProvider(
    const QString& id_,
    const QString& urlPattern_,
    const ZoomLevel minZoom_ /*= MinZoomLevel*/,
    const ZoomLevel maxZoom_ /*= MaxZoomLevel*/,
    const uint32_t maxConcurrentDownloads_ /*= 1*/,
    const uint32_t providerTileSize_ /*= 256*/,
    const AlphaChannelData alphaChannelData_ /*= AlphaChannelData::Undefined*/)
    : _p(new OnlineMapRasterTileProvider_P(this))
    , localCachePath(_p->_localCachePath)
    , networkAccessAllowed(_p->_networkAccessAllowed)
    , id(id_)
    , urlPattern(urlPattern_)
    , minZoom(minZoom_)
    , maxZoom(maxZoom_)
    , maxConcurrentDownloads(maxConcurrentDownloads_)
    , providerTileSize(providerTileSize_)
    , alphaChannelData(alphaChannelData_)
{
    _p->_localCachePath = QDir(QDir::current().filePath(id));
}

OsmAnd::OnlineMapRasterTileProvider::~OnlineMapRasterTileProvider()
{
}

void OsmAnd::OnlineMapRasterTileProvider::setLocalCachePath( const QDir& localCachePath )
{
    QMutexLocker scopedLocker(&_p->_localCachePathMutex);
    _p->_localCachePath = localCachePath;
}

void OsmAnd::OnlineMapRasterTileProvider::setNetworkAccessPermission( bool allowed )
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
