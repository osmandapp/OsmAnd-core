#include "MapRasterLayerProvider_Software.h"
#include "MapRasterLayerProvider_Software_P.h"

OsmAnd::MapRasterLayerProvider_Software::MapRasterLayerProvider_Software(
    const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider_,
    const bool fillBackground_ /* = true */,
    const bool online_ /* = false */)
    : MapRasterLayerProvider(
        new MapRasterLayerProvider_Software_P(this), primitivesProvider_, fillBackground_, online_)
{
}

OsmAnd::MapRasterLayerProvider_Software::~MapRasterLayerProvider_Software()
{
}
