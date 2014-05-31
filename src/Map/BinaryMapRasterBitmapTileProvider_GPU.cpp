#include "BinaryMapRasterBitmapTileProvider_GPU.h"
#include "BinaryMapRasterBitmapTileProvider_GPU_P.h"

OsmAnd::BinaryMapRasterBitmapTileProvider_GPU::BinaryMapRasterBitmapTileProvider_GPU(
    const std::shared_ptr<BinaryMapDataProvider>& dataProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : BinaryMapRasterBitmapTileProvider(new BinaryMapRasterBitmapTileProvider_GPU_P(this), dataProvider_, tileSize_, densityFactor_)
{
}

OsmAnd::BinaryMapRasterBitmapTileProvider_GPU::~BinaryMapRasterBitmapTileProvider_GPU()
{
}
