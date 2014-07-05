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

bool OsmAnd::MapMarkerBuilder::isAccuracyCircleSupported() const
{
    return _p->isAccuracyCircleSupported();
}

void OsmAnd::MapMarkerBuilder::setIsAccuracyCircleSupported(const bool supported)
{
    _p->setIsAccuracyCircleSupported(supported);
}

bool OsmAnd::MapMarkerBuilder::isAccuracyCircleVisible() const
{
    return _p->isAccuracyCircleVisible();
}

void OsmAnd::MapMarkerBuilder::setIsAccuracyCircleVisible(const bool visible)
{
    _p->setIsAccuracyCircleVisible(visible);
}

double OsmAnd::MapMarkerBuilder::getAccuracyCircleRadius() const
{
    return _p->getAccuracyCircleRadius();
}

void OsmAnd::MapMarkerBuilder::setAccuracyCircleRadius(const double radius)
{
    _p->setAccuracyCircleRadius(radius);
}

OsmAnd::FColorRGB OsmAnd::MapMarkerBuilder::getAccuracyCircleBaseColor() const
{
    return _p->getAccuracyCircleBaseColor();
}

void OsmAnd::MapMarkerBuilder::setAccuracyCircleBaseColor(const FColorRGB baseColor)
{
    _p->setAccuracyCircleBaseColor(baseColor);
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

OsmAnd::ColorARGB OsmAnd::MapMarkerBuilder::getPinIconModulationColor() const
{
    return _p->getPinIconModulationColor();
}

void OsmAnd::MapMarkerBuilder::setPinIconModulationColor(const ColorARGB colorValue)
{
    _p->setPinIconModulationColor(colorValue);
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
