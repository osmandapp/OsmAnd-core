#include "BoundToPointMapSymbol.h"

OsmAnd::BoundToPointMapSymbol::BoundToPointMapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& group_,
    const bool isShareable_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& location31_)
    : MapSymbol(group_, isShareable_, bitmap_, order_, content_, languageId_, minDistance_)
    , location31(location31_)
{
}

OsmAnd::BoundToPointMapSymbol::~BoundToPointMapSymbol()
{
}