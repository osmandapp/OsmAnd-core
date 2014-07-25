#include "BinaryMapRasterBitmapTileProvider_P.h"
#include "BinaryMapRasterBitmapTileProvider.h"

#include "BinaryMapPrimitivesProvider.h"

OsmAnd::BinaryMapRasterBitmapTileProvider_P::BinaryMapRasterBitmapTileProvider_P(BinaryMapRasterBitmapTileProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapRasterBitmapTileProvider_P::~BinaryMapRasterBitmapTileProvider_P()
{
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterBitmapTileProvider_P::getMinZoom() const
{
    return owner->primitivesProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterBitmapTileProvider_P::getMaxZoom() const
{
    return owner->primitivesProvider->getMaxZoom();
}
