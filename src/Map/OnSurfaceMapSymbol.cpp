#include "OnSurfaceMapSymbol.h"

OsmAnd::OnSurfaceMapSymbol::OnSurfaceMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& location31_,
    const float direction_ /*= 0.0f*/)
    : BoundToPointMapSymbol(group_, isShareable_, bitmap_, order_, intersectionModeFlags_, content_, languageId_, minDistance_, location31_)
    , direction(direction_)
{
}

OsmAnd::OnSurfaceMapSymbol::~OnSurfaceMapSymbol()
{
}

bool OsmAnd::OnSurfaceMapSymbol::isAzimuthAlignedDirection() const
{
    return qIsNaN(direction);
}
