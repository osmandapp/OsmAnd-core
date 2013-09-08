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

bool OsmAnd::OfflineMapRasterTileProvider::obtainTile(const TileId& tileId, const ZoomLevel& zoom, std::shared_ptr<MapTile>& outTile)
{
    return _d->obtainTile(tileId, zoom, outTile);
}
