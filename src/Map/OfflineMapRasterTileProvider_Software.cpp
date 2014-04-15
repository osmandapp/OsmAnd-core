#include "OfflineMapRasterTileProvider_Software.h"
#include "OfflineMapRasterTileProvider_Software_P.h"

OsmAnd::OfflineMapRasterTileProvider_Software::OfflineMapRasterTileProvider_Software( const std::shared_ptr<OfflineMapDataProvider>& dataProvider_, const uint32_t outputTileSize /*= 256*/, const float density /*= 1.0f*/)
    : _p(new OfflineMapRasterTileProvider_Software_P(this, outputTileSize, density))
    , dataProvider(dataProvider_)
{
}

OsmAnd::OfflineMapRasterTileProvider_Software::~OfflineMapRasterTileProvider_Software()
{
}

float OsmAnd::OfflineMapRasterTileProvider_Software::getTileDensity() const
{
    return _p->density;
}

uint32_t OsmAnd::OfflineMapRasterTileProvider_Software::getTileSize() const
{
    return _p->outputTileSize;
}

bool OsmAnd::OfflineMapRasterTileProvider_Software::obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile, const IQueryController* const queryController)
{
    return _p->obtainTile(tileId, zoom, outTile, queryController);
}

OsmAnd::ZoomLevel OsmAnd::OfflineMapRasterTileProvider_Software::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::OfflineMapRasterTileProvider_Software::getMaxZoom() const
{
    return _p->getMaxZoom();
}
