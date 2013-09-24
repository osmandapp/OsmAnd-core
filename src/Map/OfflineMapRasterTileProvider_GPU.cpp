#include "OfflineMapRasterTileProvider_GPU.h"
#include "OfflineMapRasterTileProvider_GPU_P.h"

OsmAnd::OfflineMapRasterTileProvider_GPU::OfflineMapRasterTileProvider_GPU( const std::shared_ptr<OfflineMapDataProvider>& dataProvider_, const float& displayDensity_ )
    : _d(new OfflineMapRasterTileProvider_GPU_P(this))
    , displayDensity(displayDensity_)
    , tileSize(static_cast<uint32_t>(256 * displayDensity_))
    , dataProvider(dataProvider_)
{
}

OsmAnd::OfflineMapRasterTileProvider_GPU::~OfflineMapRasterTileProvider_GPU()
{
    _d->_taskHostBridge.onOwnerIsBeingDestructed();
}

float OsmAnd::OfflineMapRasterTileProvider_GPU::getTileDensity() const
{
    return displayDensity;
}

uint32_t OsmAnd::OfflineMapRasterTileProvider_GPU::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::OfflineMapRasterTileProvider_GPU::obtainTile(const TileId& tileId, const ZoomLevel& zoom, std::shared_ptr<MapTile>& outTile)
{
    return _d->obtainTile(tileId, zoom, outTile);
}
