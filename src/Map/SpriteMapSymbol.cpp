#include "SpriteMapSymbol.h"

OsmAnd::SpriteMapSymbol::SpriteMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& position31_,
    const PointI& offset_)
    : PositionedRasterMapSymbol(group_, isShareable_, order_, intersectionModeFlags_, bitmap_, content_, languageId_, minDistance_, position31_)
    , offset(offset_)
{
}

OsmAnd::SpriteMapSymbol::~SpriteMapSymbol()
{
}
