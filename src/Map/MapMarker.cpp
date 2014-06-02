#include "MapMarker.h"
#include "MapMarker_P.h"

OsmAnd::MapMarker::MapMarker(
    const int baseOrder_,
    const std::shared_ptr<const SkBitmap>& pinIcon_)
    : _p(new MapMarker_P(this))
    , baseOrder(baseOrder_)
    , pinIcon(pinIcon_)
{
}

OsmAnd::MapMarker::~MapMarker()
{
}

bool OsmAnd::MapMarker::isHidden() const
{
    return _p->isHidden();
}

void OsmAnd::MapMarker::setIsHidden(const bool hidden)
{
    _p->setIsHidden(hidden);
}

bool OsmAnd::MapMarker::isPrecisionCircleEnabled() const
{
    return _p->isPrecisionCircleEnabled();
}

void OsmAnd::MapMarker::setIsPrecisionCircleEnabled(const bool enabled)
{
    _p->setIsPrecisionCircleEnabled(enabled);
}

double OsmAnd::MapMarker::getPrecisionCircleRadius() const
{
    return _p->getPrecisionCircleRadius();
}

void OsmAnd::MapMarker::setPrecisionCircleRadius(const double radius)
{
    _p->setPrecisionCircleRadius(radius);
}

SkColor OsmAnd::MapMarker::getPrecisionCircleBaseColor() const
{
    return _p->getPrecisionCircleBaseColor();
}

void OsmAnd::MapMarker::setPrecisionCircleBaseColor(const SkColor baseColor)
{
    _p->setPrecisionCircleBaseColor(baseColor);
}

OsmAnd::PointI OsmAnd::MapMarker::getPosition() const
{
    return _p->getPosition();
}

void OsmAnd::MapMarker::setPosition(const PointI position)
{
    _p->setPosition(position);
}

float OsmAnd::MapMarker::getDirection() const
{
    return _p->getDirection();
}

void OsmAnd::MapMarker::setDirection(const float direction)
{
    _p->setDirection(direction);
}

bool OsmAnd::MapMarker::hasUnappliedChanges() const
{
    return _p->hasUnappliedChanges();
}

bool OsmAnd::MapMarker::applyChanges()
{
    return _p->applyChanges();
}

std::shared_ptr<OsmAnd::MapSymbolsGroup> OsmAnd::MapMarker::createSymbolsGroup() const
{
    return _p->createSymbolsGroup();
}
