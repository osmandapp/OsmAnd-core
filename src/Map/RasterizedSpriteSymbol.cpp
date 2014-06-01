#include "RasterizedSpriteSymbol.h"

OsmAnd::RasterizedSpriteSymbol::RasterizedSpriteSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const Model::MapObject>& mapObject_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& location31_,
    const PointI& offset_)
    : RasterizedSymbol(group_, mapObject_, bitmap_, order_, content_, languageId_, minDistance_)
    , location31(location31_)
    , offset(offset_)
{
}

OsmAnd::RasterizedSpriteSymbol::~RasterizedSpriteSymbol()
{
}
