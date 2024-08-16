#include "OnSurfaceVectorMapSymbol.h"

OsmAnd::OnSurfaceVectorMapSymbol::OnSurfaceVectorMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_)
    : VectorMapSymbol(group_)
    , direction(0.0f)
    , elevationScaleFactor(1.0f)
    , startingDistance(0.0f)
{
}

OsmAnd::OnSurfaceVectorMapSymbol::~OnSurfaceVectorMapSymbol()
{
}

float OsmAnd::OnSurfaceVectorMapSymbol::getDirection() const
{
    return direction;
}

void OsmAnd::OnSurfaceVectorMapSymbol::setDirection(const float newDirection)
{
    direction = newDirection;
}

OsmAnd::PointI OsmAnd::OnSurfaceVectorMapSymbol::getPosition31() const
{
    return position31;
}

void OsmAnd::OnSurfaceVectorMapSymbol::setPosition31(const PointI newPosition31)
{
    position31 = newPosition31;
}

float OsmAnd::OnSurfaceVectorMapSymbol::getElevationScaleFactor() const
{
    return elevationScaleFactor;
}

void OsmAnd::OnSurfaceVectorMapSymbol::setElevationScaleFactor(const float scaleFactor)
{
    elevationScaleFactor = scaleFactor;
}

float OsmAnd::OnSurfaceVectorMapSymbol::getStartingDistance() const
{
    return startingDistance;
}

void OsmAnd::OnSurfaceVectorMapSymbol::setStartingDistance(const float distanceInMeters)
{
    startingDistance = distanceInMeters;
}
