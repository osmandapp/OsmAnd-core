#include "RasterizedSymbol.h"

#include <cassert>

OsmAnd::RasterizedSymbol::RasterizedSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const Model::MapObject>& mapObject_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_)
    : group(group_)
    , mapObject(mapObject_)
    , bitmap(bitmap_)
    , order(order_)
    , content(content_)
{
    assert(mapObject_);
}

OsmAnd::RasterizedSymbol::~RasterizedSymbol()
{
}
