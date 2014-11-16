#include "MapRasterLayerProvider_GPU.h"
#include "MapRasterLayerProvider_GPU_P.h"

OsmAnd::MapRasterLayerProvider_GPU::MapRasterLayerProvider_GPU(
    const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider_,
    const bool fillBackground_ /*= true*/)
    : MapRasterLayerProvider(new MapRasterLayerProvider_GPU_P(this), primitivesProvider_, fillBackground_)
{
}

OsmAnd::MapRasterLayerProvider_GPU::~MapRasterLayerProvider_GPU()
{
}
