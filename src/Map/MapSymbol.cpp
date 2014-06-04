#include "MapSymbol.h"

OsmAnd::MapSymbol::MapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_)
    : group(group_)
    , groupPtr(group_.get())
    , isShareable(isShareable_)
    , bitmap(bitmap_)
    , order(order_)
    , intersectionModeFlags(intersectionModeFlags_)
    , content(content_)
    , languageId(languageId_)
    , minDistance(minDistance_)
    , isHidden(false)
{
}

OsmAnd::MapSymbol::~MapSymbol()
{
}
