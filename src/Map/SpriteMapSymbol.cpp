#include "SpriteMapSymbol.h"

OsmAnd::SpriteMapSymbol::SpriteMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& location31_,
    const PointI& offset_)
    : BoundToPointMapSymbol(group_, isShareable_, bitmap_, order_, intersectionModeFlags_, content_, languageId_, minDistance_, location31_)
    , offset(offset_)
{
}

OsmAnd::SpriteMapSymbol::~SpriteMapSymbol()
{
}

std::shared_ptr<OsmAnd::MapSymbol> OsmAnd::SpriteMapSymbol::clone() const
{
    return std::shared_ptr<MapSymbol>(new SpriteMapSymbol(
        nullptr,
        isShareable,
        bitmap,
        order,
        intersectionModeFlags,
        content,
        languageId,
        minDistance,
        location31,
        offset));
}
