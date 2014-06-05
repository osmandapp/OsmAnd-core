#include "OnSurfaceRasterMapSymbol.h"

OsmAnd::OnSurfaceRasterMapSymbol::OnSurfaceRasterMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& position31_)
    : RasterMapSymbol(group_, isShareable_, order_, intersectionModeFlags_, bitmap_, content_, languageId_, minDistance_)
    , direction(0.0f)
    , position31(position31_)
{
}

OsmAnd::OnSurfaceRasterMapSymbol::~OnSurfaceRasterMapSymbol()
{
}

float OsmAnd::OnSurfaceRasterMapSymbol::getDirection() const
{
    return direction;
}

void OsmAnd::OnSurfaceRasterMapSymbol::setDirection(const float newDirection)
{
    direction = newDirection;
}

OsmAnd::PointI OsmAnd::OnSurfaceRasterMapSymbol::getPosition31() const
{
    return position31;
}

void OsmAnd::OnSurfaceRasterMapSymbol::setPosition31(const PointI position)
{
    position31 = position;
}
