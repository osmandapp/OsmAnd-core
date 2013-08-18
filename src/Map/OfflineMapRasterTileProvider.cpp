#include "OfflineMapRasterTileProvider.h"
#include "OfflineMapRasterTileProvider_P.h"

OsmAnd::OfflineMapRasterTileProvider::OfflineMapRasterTileProvider( const std::shared_ptr<OfflineMapDataProvider>& dataProvider_, const float& displayDensity_ )
    : _d(new OfflineMapRasterTileProvider_P(this))
    , displayDensity(displayDensity_)
    , tileSize(static_cast<uint32_t>(256 * displayDensity_))
    , dataProvider(dataProvider_)
{
}

OsmAnd::OfflineMapRasterTileProvider::~OfflineMapRasterTileProvider()
{
    _d->_taskHostBridge.onOwnerIsBeingDestructed();
}

float OsmAnd::OfflineMapRasterTileProvider::getTileDensity() const
{
    return displayDensity;
}

uint32_t OsmAnd::OfflineMapRasterTileProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::OfflineMapRasterTileProvider::obtainTileImmediate( const TileId& tileId, const ZoomLevel& zoom, std::shared_ptr<IMapTileProvider::Tile>& tile )
{
    // Raster tiles are not available immediately, since none of them are stored in memory unless they are just
    // downloaded. In that case, a callback will be called
    return false;
}

void OsmAnd::OfflineMapRasterTileProvider::obtainTileDeffered( const TileId& tileId, const ZoomLevel& zoom, TileReadyCallback readyCallback )
{
    _d->obtainTileDeffered(tileId, zoom, readyCallback);
}
