#include "OnlineMapRasterTileProvider.h"

OsmAnd::OnlineMapRasterTileProvider::OnlineMapRasterTileProvider(const QString& urlPattern, uint32_t maxZoom /*= 31*/, uint32_t minZoom /*= 0*/)
    : _networkAccessAllowed(true)
    , localCachePath(_localCachePath)
    , networkAccessAllowed(_networkAccessAllowed)
{

}

OsmAnd::OnlineMapRasterTileProvider::~OnlineMapRasterTileProvider()
{

}

void OsmAnd::OnlineMapRasterTileProvider::setLocalCachePath( const std::shared_ptr<QDir>& localCachePath )
{

}

void OsmAnd::OnlineMapRasterTileProvider::setNetworkAccessPermission( bool allowed )
{

}

bool OsmAnd::OnlineMapRasterTileProvider::obtainTile(
    const uint64_t& tileId, uint32_t zoom,
    std::shared_ptr<SkBitmap>& tile )
{
    return false;
}

void OsmAnd::OnlineMapRasterTileProvider::obtainTile(
    const uint64_t& tileId, uint32_t zoom,
    std::function<void (const uint64_t& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tile)> receiverCallback )
{

}

const std::shared_ptr<OsmAnd::OnlineMapRasterTileProvider> OsmAnd::OnlineMapRasterTileProvider::Mapnik(new OsmAnd::OnlineMapRasterTileProvider(
     "http://mapnik.osmand.net/${x}/${y}/${zoom}.png", 18, 1
));

const std::shared_ptr<OsmAnd::OnlineMapRasterTileProvider> OsmAnd::OnlineMapRasterTileProvider::CycleMap(new OsmAnd::OnlineMapRasterTileProvider(
    "http://b.tile.opencyclemap.org/cycle/${x}/${y}/${zoom}.png", 16, 1
));
