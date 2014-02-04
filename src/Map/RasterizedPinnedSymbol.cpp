#include "RasterizedPinnedSymbol.h"

OsmAnd::RasterizedPinnedSymbol::RasterizedPinnedSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const Model::MapObject>& mapObject_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const PointI& location31_,
    const PointI& offset_)
    : RasterizedSymbol(group_, mapObject_, bitmap_, order_)
    , location31(location31_)
    , offset(offset_)
{
}

OsmAnd::RasterizedPinnedSymbol::~RasterizedPinnedSymbol()
{
}
