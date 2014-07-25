#include "BinaryMapRasterBitmapTileProvider_Software.h"
#include "BinaryMapRasterBitmapTileProvider_Software_P.h"

OsmAnd::BinaryMapRasterBitmapTileProvider_Software::BinaryMapRasterBitmapTileProvider_Software(const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider_)
    : BinaryMapRasterBitmapTileProvider(new BinaryMapRasterBitmapTileProvider_Software_P(this), primitivesProvider_)
{
}

OsmAnd::BinaryMapRasterBitmapTileProvider_Software::~BinaryMapRasterBitmapTileProvider_Software()
{
}
