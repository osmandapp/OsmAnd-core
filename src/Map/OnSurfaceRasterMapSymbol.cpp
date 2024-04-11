#include "OnSurfaceRasterMapSymbol.h"

OsmAnd::OnSurfaceRasterMapSymbol::OnSurfaceRasterMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_)
    : RasterMapSymbol(group_)
    , direction(0.0f)
    , elevation(NAN)
    , elevationScaleFactor(1.0f)
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

float OsmAnd::OnSurfaceRasterMapSymbol::getElevation() const
{
    return elevation;
}

void OsmAnd::OnSurfaceRasterMapSymbol::setElevation(const float newElevation)
{
    elevation = newElevation;
}

float OsmAnd::OnSurfaceRasterMapSymbol::getElevationScaleFactor() const
{
    return elevationScaleFactor;
}

void OsmAnd::OnSurfaceRasterMapSymbol::setElevationScaleFactor(const float scaleFactor)
{
    elevationScaleFactor = scaleFactor;
}
