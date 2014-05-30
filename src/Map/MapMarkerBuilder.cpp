#include "MapMarkerBuilder.h"
#include "MapMarkerBuilder_P.h"

OsmAnd::MapMarkerBuilder::MapMarkerBuilder()
    : _p(new MapMarkerBuilder_P(this))
{
}

OsmAnd::MapMarkerBuilder::~MapMarkerBuilder()
{
}

bool OsmAnd::MapMarkerBuilder::isHidden() const
{
    return _p->isHidden();
}

void OsmAnd::MapMarkerBuilder::setIsHidden(const bool hidden)
{
    _p->setIsHidden(hidden);
}

bool OsmAnd::MapMarkerBuilder::isPrecisionCircleEnabled() const
{
    return _p->isPrecisionCircleEnabled();
}

void OsmAnd::MapMarkerBuilder::setIsPrecisionCircleEnabled(const bool enabled)
{
    _p->setIsPrecisionCircleEnabled(enabled);
}

double OsmAnd::MapMarkerBuilder::getPrecisionCircleRadius() const
{
    return _p->getPrecisionCircleRadius();
}

void OsmAnd::MapMarkerBuilder::setPrecisionCircleRadius(const double radius)
{
    _p->setPrecisionCircleRadius(radius);
}

SkColor OsmAnd::MapMarkerBuilder::getPrecisionCircleBaseColor() const
{
    return _p->getPrecisionCircleBaseColor();
}

void OsmAnd::MapMarkerBuilder::setPrecisionCircleBaseColor(const SkColor baseColor)
{
    _p->setPrecisionCircleBaseColor(baseColor);
}

OsmAnd::PointI OsmAnd::MapMarkerBuilder::getPosition() const
{
    return _p->getPosition();
}

void OsmAnd::MapMarkerBuilder::setPosition(const PointI position)
{
    _p->setPosition(position);
}

float OsmAnd::MapMarkerBuilder::getDirection() const
{
    return _p->getDirection();
}

void OsmAnd::MapMarkerBuilder::setDirection(const float direction)
{
    _p->setDirection(direction);
}

const SkBitmap OsmAnd::MapMarkerBuilder::getPinIcon() const
{
    return _p->getPinIcon();
}

void OsmAnd::MapMarkerBuilder::setPinIcon(const SkBitmap& bitmap)
{
    _p->setPinIcon(bitmap);
}

QList< QPair<const SkBitmap, bool> > OsmAnd::MapMarkerBuilder::getMapIcons() const
{
    return _p->getMapIcons();
}

void OsmAnd::MapMarkerBuilder::clearMapIcons()
{
    _p->clearMapIcons();
}

void OsmAnd::MapMarkerBuilder::addMapIcon(const SkBitmap& bitmap, const bool respectsDirection /*= true*/)
{
    _p->addMapIcon(bitmap, respectsDirection);
}

std::shared_ptr<OsmAnd::MapMarker> OsmAnd::MapMarkerBuilder::buildAndAddToCollection(const std::shared_ptr<MapMarkersCollection>& collection)
{
    return _p->buildAndAddToCollection(collection);
}
