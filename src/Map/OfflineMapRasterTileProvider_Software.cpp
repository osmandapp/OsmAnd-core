#include "OfflineMapRasterTileProvider_Software.h"
#include "OfflineMapRasterTileProvider_Software_P.h"

OsmAnd::OfflineMapRasterTileProvider_Software::OfflineMapRasterTileProvider_Software( const std::shared_ptr<OfflineMapDataProvider>& dataProvider_, const float& displayDensity_ )
    : _d(new OfflineMapRasterTileProvider_Software_P(this))
    , displayDensity(displayDensity_)
    , tileSize(static_cast<uint32_t>(256 * displayDensity_))
    , dataProvider(dataProvider_)
{
}

OsmAnd::OfflineMapRasterTileProvider_Software::~OfflineMapRasterTileProvider_Software()
{
    _d->_taskHostBridge.onOwnerIsBeingDestructed();
}

float OsmAnd::OfflineMapRasterTileProvider_Software::getTileDensity() const
{
    return displayDensity;
}

uint32_t OsmAnd::OfflineMapRasterTileProvider_Software::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::OfflineMapRasterTileProvider_Software::obtainTile(const TileId& tileId, const ZoomLevel& zoom, std::shared_ptr<MapTile>& outTile)
{
    return _d->obtainTile(tileId, zoom, outTile);
}
