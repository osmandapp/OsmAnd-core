#include "OfflineMapRasterTileProvider_Software.h"
#include "OfflineMapRasterTileProvider_Software_P.h"

OsmAnd::OfflineMapRasterTileProvider_Software::OfflineMapRasterTileProvider_Software( const std::shared_ptr<OfflineMapDataProvider>& dataProvider_, const uint32_t outputTileSize /*= 256*/, const float density /*= 1.0f*/)
    : _d(new OfflineMapRasterTileProvider_Software_P(this, outputTileSize, density))
    , dataProvider(dataProvider_)
{
}

OsmAnd::OfflineMapRasterTileProvider_Software::~OfflineMapRasterTileProvider_Software()
{
    _d->_taskHostBridge.onOwnerIsBeingDestructed();
}

float OsmAnd::OfflineMapRasterTileProvider_Software::getTileDensity() const
{
    return _d->density;
}

uint32_t OsmAnd::OfflineMapRasterTileProvider_Software::getTileSize() const
{
    return _d->outputTileSize;
}

bool OsmAnd::OfflineMapRasterTileProvider_Software::obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile)
{
    return _d->obtainTile(tileId, zoom, outTile);
}
