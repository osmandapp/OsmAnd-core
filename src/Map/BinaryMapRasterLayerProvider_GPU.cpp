#include "BinaryMapRasterLayerProvider_GPU.h"
#include "BinaryMapRasterLayerProvider_GPU_P.h"

OsmAnd::BinaryMapRasterLayerProvider_GPU::BinaryMapRasterLayerProvider_GPU(const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider_)
    : BinaryMapRasterLayerProvider(new BinaryMapRasterLayerProvider_GPU_P(this), primitivesProvider_)
{
}

OsmAnd::BinaryMapRasterLayerProvider_GPU::~BinaryMapRasterLayerProvider_GPU()
{
}
