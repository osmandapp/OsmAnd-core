#include "MapRasterLayerProvider_GPU.h"
#include "MapRasterLayerProvider_GPU_P.h"

OsmAnd::MapRasterLayerProvider_GPU::MapRasterLayerProvider_GPU(
    const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider_,
    const bool fillBackground_ /* = true */,
    const bool forceObtainDataAsync_ /* = false */,
    const bool adjustToDetailedZoom_ /* = false */)
    : MapRasterLayerProvider(new MapRasterLayerProvider_GPU_P(this),
        primitivesProvider_, fillBackground_, forceObtainDataAsync_, adjustToDetailedZoom_)
{
}

OsmAnd::MapRasterLayerProvider_GPU::~MapRasterLayerProvider_GPU()
{
}
