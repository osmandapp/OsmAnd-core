#include "BinaryMapRasterBitmapTileProvider_Software.h"
#include "BinaryMapRasterBitmapTileProvider_Software_P.h"

OsmAnd::BinaryMapRasterBitmapTileProvider_Software::BinaryMapRasterBitmapTileProvider_Software(
    const std::shared_ptr<BinaryMapDataProvider>& dataProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : BinaryMapRasterBitmapTileProvider(new BinaryMapRasterBitmapTileProvider_Software_P(this), dataProvider_, tileSize_, densityFactor_)
{
}

OsmAnd::BinaryMapRasterBitmapTileProvider_Software::~BinaryMapRasterBitmapTileProvider_Software()
{
}
