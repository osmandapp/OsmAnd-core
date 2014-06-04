#include "RasterMapSymbol.h"

OsmAnd::RasterMapSymbol::RasterMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_)
    : MapSymbol(group_, isShareable_, order_, intersectionModeFlags_)
    , bitmap(bitmap_)
    , content(content_)
    , languageId(languageId_)
    , minDistance(minDistance_)
{
}

OsmAnd::RasterMapSymbol::~RasterMapSymbol()
{
}
