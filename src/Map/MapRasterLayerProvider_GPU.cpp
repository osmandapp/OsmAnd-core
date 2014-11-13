#include "MapRasterLayerProvider_GPU.h"
#include "MapRasterLayerProvider_GPU_P.h"

OsmAnd::MapRasterLayerProvider_GPU::MapRasterLayerProvider_GPU(const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider_)
    : MapRasterLayerProvider(new MapRasterLayerProvider_GPU_P(this), primitivesProvider_)
{
}

OsmAnd::MapRasterLayerProvider_GPU::~MapRasterLayerProvider_GPU()
{
}
