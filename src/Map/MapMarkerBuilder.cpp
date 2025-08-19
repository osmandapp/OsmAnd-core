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

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setIsHidden(const bool hidden)
{
    _p->setIsHidden(hidden);

    return *this;
}

int OsmAnd::MapMarkerBuilder::getMarkerId() const
{
    return _p->getMarkerId();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setMarkerId(const int markerId)
{
    _p->setMarkerId(markerId);
    
    return *this;
}

int OsmAnd::MapMarkerBuilder::getBaseOrder() const
{
    return _p->getBaseOrder();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setBaseOrder(const int baseOrder)
{
    _p->setBaseOrder(baseOrder);

    return *this;
}

bool OsmAnd::MapMarkerBuilder::isAccuracyCircleSupported() const
{
    return _p->isAccuracyCircleSupported();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setIsAccuracyCircleSupported(const bool supported)
{
    _p->setIsAccuracyCircleSupported(supported);

    return *this;
}

bool OsmAnd::MapMarkerBuilder::isAccuracyCircleVisible() const
{
    return _p->isAccuracyCircleVisible();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setIsAccuracyCircleVisible(const bool visible)
{
    _p->setIsAccuracyCircleVisible(visible);

    return *this;
}

double OsmAnd::MapMarkerBuilder::getAccuracyCircleRadius() const
{
    return _p->getAccuracyCircleRadius();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setAccuracyCircleRadius(const double radius)
{
    _p->setAccuracyCircleRadius(radius);

    return *this;
}

OsmAnd::FColorRGB OsmAnd::MapMarkerBuilder::getAccuracyCircleBaseColor() const
{
    return _p->getAccuracyCircleBaseColor();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setAccuracyCircleBaseColor(const FColorRGB baseColor)
{
    _p->setAccuracyCircleBaseColor(baseColor);

    return *this;
}

OsmAnd::PointI OsmAnd::MapMarkerBuilder::getPosition() const
{
    return _p->getPosition();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setPosition(const PointI position)
{
    _p->setPosition(position);

    return *this;
}

float OsmAnd::MapMarkerBuilder::getHeight() const
{
    return _p->getHeight();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setHeight(const float height)
{
    _p->setHeight(height);

    return *this;
}

float OsmAnd::MapMarkerBuilder::getElevationScaleFactor() const
{
    return _p->getElevationScaleFactor();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setElevationScaleFactor(const float scaleFactor)
{
    _p->setElevationScaleFactor(scaleFactor);

    return *this;
}

sk_sp<const SkImage> OsmAnd::MapMarkerBuilder::getPinIcon() const
{
    return _p->getPinIcon();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setPinIcon(const SingleSkImage& image)
{
    _p->setPinIcon(image);

    return *this;
}

OsmAnd::MapMarker::PinIconVerticalAlignment OsmAnd::MapMarkerBuilder::getPinIconVerticalAlignment() const
{
    return _p->getPinIconVerticalAlignment();
}

OsmAnd::MapMarker::PinIconHorisontalAlignment OsmAnd::MapMarkerBuilder::getPinIconHorisontalAlignment() const
{
    return _p->getPinIconHorisontalAlignment();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setPinIconVerticalAlignment(const MapMarker::PinIconVerticalAlignment value)
{
    _p->setPinIconVerticalAlignment(value);

    return *this;
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setPinIconHorisontalAlignment(const MapMarker::PinIconHorisontalAlignment value)
{
    _p->setPinIconHorisontalAlignment(value);
    
    return *this;
}

OsmAnd::PointI OsmAnd::MapMarkerBuilder::getPinIconOffset() const
{
    return _p->getPinIconOffset();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setPinIconOffset(const PointI pinIconOffset)
{
    _p->setPinIconOffset(pinIconOffset);
    
    return *this;
}

OsmAnd::ColorARGB OsmAnd::MapMarkerBuilder::getPinIconModulationColor() const
{
    return _p->getPinIconModulationColor();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setPinIconModulationColor(const ColorARGB colorValue)
{
    _p->setPinIconModulationColor(colorValue);

    return *this;
}

OsmAnd::ColorARGB OsmAnd::MapMarkerBuilder::getOnSurfaceIconModulationColor() const
{
    return _p->getOnSurfaceIconModulationColor();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setOnSurfaceIconModulationColor(const ColorARGB colorValue)
{
    _p->setOnSurfaceIconModulationColor(colorValue);

    return *this;
}

QString OsmAnd::MapMarkerBuilder::getCaption() const
{
    return _p->getCaption();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setCaption(const QString& caption)
{
    _p->setCaption(caption);

    return *this;
}

OsmAnd::TextRasterizer::Style OsmAnd::MapMarkerBuilder::getCaptionStyle() const
{
    return _p->getCaptionStyle();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setCaptionStyle(const TextRasterizer::Style captionStyle)
{
    _p->setCaptionStyle(captionStyle);

    return *this;
}

double OsmAnd::MapMarkerBuilder::getCaptionTopSpace() const
{
    return _p->getCaptionTopSpace();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setCaptionTopSpace(const double captionTopSpace)
{
    _p->setCaptionTopSpace(captionTopSpace);

    return *this;
}

std::shared_ptr<OsmAnd::MapMarker> OsmAnd::MapMarkerBuilder::buildAndAddToCollection(
    const std::shared_ptr<MapMarkersCollection>& collection)
{
    return _p->buildAndAddToCollection(collection);
}

QHash< OsmAnd::MapMarker::OnSurfaceIconKey, sk_sp<const SkImage> > OsmAnd::MapMarkerBuilder::getOnMapSurfaceIcons() const
{
    return _p->getOnMapSurfaceIcons();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::addOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key, const SingleSkImage& image)
{
    _p->addOnMapSurfaceIcon(key, image);

    return *this;
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::removeOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key)
{
    _p->removeOnMapSurfaceIcon(key);

    return *this;
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::clearOnMapSurfaceIcons()
{
    _p->clearOnMapSurfaceIcons();

    return *this;
}

std::shared_ptr<const OsmAnd::Model3D> OsmAnd::MapMarkerBuilder::getModel3D() const
{
    return _p->getModel3D();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setModel3D(const std::shared_ptr<const Model3D>& model3D)
{
    _p->setModel3D(model3D);

    return *this;
}

QHash<QString, OsmAnd::FColorARGB> OsmAnd::MapMarkerBuilder::getModel3DCustomMaterialColors() const
{
    return _p->getModel3DCustomMaterialColors();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::addModel3DCustomMaterialColor(const QString& materialName, const FColorARGB& color)
{
    _p->addModel3DCustomMaterialColor(materialName, color);

    return *this;
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::removeModel3DCustomMaterialColor(const QString& materialName)
{
    _p->removeModel3DCustomMaterialColor(materialName);

    return *this;
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::clearModel3DCustomMaterialColors()
{
    _p->clearModel3DCustomMaterialColors();

    return *this;
}

int OsmAnd::MapMarkerBuilder::getModel3DMaxSizeInPixels() const
{
    return _p->getModel3DMaxSizeInPixels();
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setModel3DMaxSizeInPixels(const int maxSizeInPixels)
{
    _p->setModel3DMaxSizeInPixels(maxSizeInPixels);

    return *this;
}

OsmAnd::MapMarkerBuilder& OsmAnd::MapMarkerBuilder::setUpdateAfterCreated(bool updateAfterCreated)
{
    _p->setUpdateAfterCreated(updateAfterCreated);

    return *this;
}
