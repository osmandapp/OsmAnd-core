#include "BinaryMapRasterBitmapTileProvider_GPU.h"
#include "BinaryMapRasterBitmapTileProvider_GPU_P.h"

OsmAnd::BinaryMapRasterBitmapTileProvider_GPU::BinaryMapRasterBitmapTileProvider_GPU(const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider_)
    : BinaryMapRasterBitmapTileProvider(new BinaryMapRasterBitmapTileProvider_GPU_P(this), primitivesProvider_)
{
}

OsmAnd::BinaryMapRasterBitmapTileProvider_GPU::~BinaryMapRasterBitmapTileProvider_GPU()
{
}
