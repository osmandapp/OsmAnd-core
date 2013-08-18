#include "OnlineMapRasterTileProvider.h"
#include "OnlineMapRasterTileProvider_P.h"

#include <assert.h>

OsmAnd::OnlineMapRasterTileProvider::OnlineMapRasterTileProvider(
    const QString& id_,
    const QString& urlPattern_,
    const ZoomLevel& maxZoom_ /*= 31*/,
    const ZoomLevel& minZoom_ /*= 0*/,
    const uint32_t& maxConcurrentDownloads_ /*= 1*/,
    const uint32_t& tileDimension_ /*= 256*/,
    const AlphaChannelData& alphaChannelData_ /*= AlphaChannelData::Undefined*/)
    : _d(new OnlineMapRasterTileProvider_P(this))
    , localCachePath(_d->_localCachePath)
    , networkAccessAllowed(_d->_networkAccessAllowed)
    , id(id_)
    , urlPattern(urlPattern_)
    , minZoom(minZoom_)
    , maxZoom(maxZoom_)
    , maxConcurrentDownloads(maxConcurrentDownloads_)
    , tileDimension(tileDimension_)
    , alphaChannelData(alphaChannelData_)
{
}

OsmAnd::OnlineMapRasterTileProvider::~OnlineMapRasterTileProvider()
{
    _d->_taskHostBridge.onOwnerIsBeingDestructed();
}

void OsmAnd::OnlineMapRasterTileProvider::setLocalCachePath( const QDir& localCachePath )
{
    _d->_localCachePath = localCachePath;
}

void OsmAnd::OnlineMapRasterTileProvider::setNetworkAccessPermission( bool allowed )
{
    _d->_networkAccessAllowed = allowed;
}

bool OsmAnd::OnlineMapRasterTileProvider::obtainTileImmediate( const TileId& tileId, const ZoomLevel& zoom, std::shared_ptr<IMapTileProvider::Tile>& tile )
{
    // Raster tiles are not available immediately, since none of them are stored in memory unless they are just
    // downloaded. In that case, a callback will be called
    return false;
}

void OsmAnd::OnlineMapRasterTileProvider::obtainTileDeffered( const TileId& tileId, const ZoomLevel& zoom, TileReadyCallback readyCallback )
{
    _d->obtainTileDeffered(tileId, zoom, readyCallback);
}

float OsmAnd::OnlineMapRasterTileProvider::getTileDensity() const
{
    // Online tile providers do not have any idea about our tile density
    return 1.0f;
}

uint32_t OsmAnd::OnlineMapRasterTileProvider::getTileSize() const
{
    return tileDimension;
}

std::shared_ptr<OsmAnd::IMapBitmapTileProvider> OsmAnd::OnlineMapRasterTileProvider::createMapnikProvider()
{
    auto provider = new OsmAnd::OnlineMapRasterTileProvider(
        "mapnik", "http://mapnik.osmand.net/${zoom}/${x}/${y}.png",
        ZoomLevel0, ZoomLevel18, 2,
        256, AlphaChannelData::NotPresent);
    return std::shared_ptr<OsmAnd::IMapBitmapTileProvider>(static_cast<OsmAnd::IMapBitmapTileProvider*>(provider));
}

std::shared_ptr<OsmAnd::IMapBitmapTileProvider> OsmAnd::OnlineMapRasterTileProvider::createCycleMapProvider()
{
    auto provider = new OsmAnd::OnlineMapRasterTileProvider(
        "cyclemap", "http://b.tile.opencyclemap.org/cycle/${zoom}/${x}/${y}.png",
        ZoomLevel0, ZoomLevel18, 2,
        256, AlphaChannelData::NotPresent);
    return std::shared_ptr<OsmAnd::IMapBitmapTileProvider>(static_cast<OsmAnd::IMapBitmapTileProvider*>(provider));
}
