#include "OnSurfaceMapSymbol.h"

OsmAnd::OnSurfaceMapSymbol::OnSurfaceMapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& group_,
    const bool isShareable_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& location31_)
    : BoundToPointMapSymbol(group_, isShareable_, bitmap_, order_, content_, languageId_, minDistance_, location31_)
{
}

OsmAnd::OnSurfaceMapSymbol::~OnSurfaceMapSymbol()
{
}

OsmAnd::MapSymbol* OsmAnd::OnSurfaceMapSymbol::cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& replacementBitmap) const
{
    return new OnSurfaceMapSymbol(
        group.lock(),
        isShareable,
        replacementBitmap,
        order,
        content,
        languageId,
        minDistance,
        location31);
}
