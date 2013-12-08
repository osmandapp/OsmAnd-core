#include "RasterizedSymbol.h"

#include <cassert>

OsmAnd::RasterizedSymbol::RasterizedSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const Model::MapObject>& mapObject_,
    const PointI& location31_, const int order_, const PointI& offset_,
    const std::shared_ptr<const SkBitmap>& bitmap_)
    : group(group_)
    , mapObject(mapObject_)
    , location31(location31_)
    , order(order_)
    , offset(offset_)
    , bitmap(bitmap_)
{
    assert(mapObject_);
}

OsmAnd::RasterizedSymbol::~RasterizedSymbol()
{
}
