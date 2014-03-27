#include "OfflineMapRasterTileProvider_GPU.h"
#include "OfflineMapRasterTileProvider_GPU_P.h"

OsmAnd::OfflineMapRasterTileProvider_GPU::OfflineMapRasterTileProvider_GPU( const std::shared_ptr<OfflineMapDataProvider>& dataProvider_, const uint32_t outputTileSize /*= 256*/, const float density /*= 1.0f*/ )
    : _d(new OfflineMapRasterTileProvider_GPU_P(this, outputTileSize, density))
    , dataProvider(dataProvider_)
{
}

OsmAnd::OfflineMapRasterTileProvider_GPU::~OfflineMapRasterTileProvider_GPU()
{
}

float OsmAnd::OfflineMapRasterTileProvider_GPU::getTileDensity() const
{
    return _d->density;
}

uint32_t OsmAnd::OfflineMapRasterTileProvider_GPU::getTileSize() const
{
    return _d->outputTileSize;
}

bool OsmAnd::OfflineMapRasterTileProvider_GPU::obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile, const IQueryController* const queryController)
{
    return _d->obtainTile(tileId, zoom, outTile, queryController);
}
