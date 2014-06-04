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

int OsmAnd::MapMarkerBuilder::getBaseOrder() const
{
    return _p->getBaseOrder();
}

void OsmAnd::MapMarkerBuilder::setBaseOrder(const int baseOrder)
{
    _p->setBaseOrder(baseOrder);
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

std::shared_ptr<const SkBitmap> OsmAnd::MapMarkerBuilder::getPinIcon() const
{
    return _p->getPinIcon();
}

void OsmAnd::MapMarkerBuilder::setPinIcon(const std::shared_ptr<const SkBitmap>& bitmap)
{
    _p->setPinIcon(bitmap);
}

std::shared_ptr<OsmAnd::MapMarker> OsmAnd::MapMarkerBuilder::buildAndAddToCollection(const std::shared_ptr<MapMarkersCollection>& collection)
{
    return _p->buildAndAddToCollection(collection);
}

QHash< OsmAnd::MapMarker::OnSurfaceIconKey, std::shared_ptr<const SkBitmap> > OsmAnd::MapMarkerBuilder::getOnMapSurfaceIcons() const
{
    return _p->getOnMapSurfaceIcons();
}

void OsmAnd::MapMarkerBuilder::addOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key, const std::shared_ptr<const SkBitmap>& bitmap)
{
    _p->addOnMapSurfaceIcon(key, bitmap);
}

void OsmAnd::MapMarkerBuilder::removeOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key)
{
    _p->removeOnMapSurfaceIcon(key);
}

void OsmAnd::MapMarkerBuilder::clearOnMapSurfaceIcons()
{
    _p->clearOnMapSurfaceIcons();
}
