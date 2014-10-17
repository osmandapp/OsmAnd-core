#include "BinaryMapRasterLayerProvider_Software.h"
#include "BinaryMapRasterLayerProvider_Software_P.h"

OsmAnd::BinaryMapRasterLayerProvider_Software::BinaryMapRasterLayerProvider_Software(const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider_)
    : BinaryMapRasterLayerProvider(new BinaryMapRasterLayerProvider_Software_P(this), primitivesProvider_)
{
}

OsmAnd::BinaryMapRasterLayerProvider_Software::~BinaryMapRasterLayerProvider_Software()
{
}
