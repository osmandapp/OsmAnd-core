#include "BinaryMapRasterBitmapTileProvider_P.h"
#include "BinaryMapRasterBitmapTileProvider.h"

#include "BinaryMapPrimitivesProvider.h"
#include "Primitiviser.h"
#include "MapRasterizer.h"

OsmAnd::BinaryMapRasterBitmapTileProvider_P::BinaryMapRasterBitmapTileProvider_P(BinaryMapRasterBitmapTileProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapRasterBitmapTileProvider_P::~BinaryMapRasterBitmapTileProvider_P()
{
}

void OsmAnd::BinaryMapRasterBitmapTileProvider_P::initialize()
{
    _mapRasterizer.reset(new MapRasterizer(owner->primitivesProvider->primitiviser->environment));
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterBitmapTileProvider_P::getMinZoom() const
{
    return owner->primitivesProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterBitmapTileProvider_P::getMaxZoom() const
{
    return owner->primitivesProvider->getMaxZoom();
}

bool OsmAnd::BinaryMapRasterBitmapTileProvider_P::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const IQueryController* const queryController)
{
    return obtainData(tileId, zoom, outTiledData, nullptr, queryController);
}
