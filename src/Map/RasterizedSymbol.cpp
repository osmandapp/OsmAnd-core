#include "RasterizedSymbol.h"

#include <cassert>

OsmAnd::RasterizedSymbol::RasterizedSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const Model::MapObject>& mapObject_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_)
    : group(group_)
    , mapObject(mapObject_)
    , bitmap(bitmap_)
    , order(order_)
    , content(content_)
    , languageId(languageId_)
    , minDistance(minDistance_)
{
    assert(mapObject_);
}

OsmAnd::RasterizedSymbol::~RasterizedSymbol()
{
}
