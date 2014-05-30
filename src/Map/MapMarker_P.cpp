#include "MapMarker_P.h"
#include "MapMarker.h"

OsmAnd::MapMarker_P::MapMarker_P(MapMarker* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapMarker_P::~MapMarker_P()
{
}

bool OsmAnd::MapMarker_P::isHidden() const
{
    QReadLocker scopedLocker(&_lock);

    return _isHidden;
}

void OsmAnd::MapMarker_P::setIsHidden(const bool hidden)
{

}

bool OsmAnd::MapMarker_P::isPrecisionCircleEnabled() const
{
    return false;
}

void OsmAnd::MapMarker_P::setIsPrecisionCircleEnabled(const bool enabled)
{

}

double OsmAnd::MapMarker_P::getPrecisionCircleRadius() const
{
    return 0.0;
}

void OsmAnd::MapMarker_P::setPrecisionCircleRadius(const double radius)
{

}

SkColor OsmAnd::MapMarker_P::getPrecisionCircleBaseColor() const
{
    return 0;
}

void OsmAnd::MapMarker_P::setPrecisionCircleBaseColor(const SkColor baseColor)
{

}

OsmAnd::PointI OsmAnd::MapMarker_P::getPosition() const
{
    return PointI();
}

void OsmAnd::MapMarker_P::setPosition(const PointI position)
{

}

float OsmAnd::MapMarker_P::getDirection() const
{
    return 0.0f;
}

void OsmAnd::MapMarker_P::setDirection(const float direction)
{

}

bool OsmAnd::MapMarker_P::hasUnappliedChanges() const
{
    return false;
}

void OsmAnd::MapMarker_P::applyChanges()
{

}

