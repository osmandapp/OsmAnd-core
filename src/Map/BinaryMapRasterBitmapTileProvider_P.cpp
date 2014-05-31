#include "BinaryMapRasterBitmapTileProvider_P.h"
#include "BinaryMapRasterBitmapTileProvider.h"

#include "BinaryMapDataProvider.h"

OsmAnd::BinaryMapRasterBitmapTileProvider_P::BinaryMapRasterBitmapTileProvider_P(BinaryMapRasterBitmapTileProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapRasterBitmapTileProvider_P::~BinaryMapRasterBitmapTileProvider_P()
{
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterBitmapTileProvider_P::getMinZoom() const
{
    return owner->dataProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterBitmapTileProvider_P::getMaxZoom() const
{
    return owner->dataProvider->getMaxZoom();
}
