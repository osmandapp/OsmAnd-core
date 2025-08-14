#include "MapMarkerBuilder_P.h"
#include "MapMarkerBuilder.h"

#include "QtCommon.h"

#include "MapMarker.h"
#include "MapMarker_P.h"
#include "MapMarkersCollection.h"
#include "MapMarkersCollection_P.h"
#include "Utilities.h"

OsmAnd::MapMarkerBuilder_P::MapMarkerBuilder_P(MapMarkerBuilder* const owner_)
    : _isHidden(false)
    , _markerId(0)
    , _baseOrder(std::numeric_limits<int>::min())
    , _isAccuracyCircleSupported(false)
    , _isAccuracyCircleVisible(false)
    , _accuracyCircleRadius(0.0)
    , _height(NAN)
    , _elevationScaleFactor(1.0f)
    , _pinIconVerticalAlignment(MapMarker::PinIconVerticalAlignment::CenterVertical)
    , _pinIconHorisontalAlignment(MapMarker::PinIconHorisontalAlignment::CenterHorizontal)
    , _model3DMaxSizeInPixels(0)
    , owner(owner_)
{
}

OsmAnd::MapMarkerBuilder_P::~MapMarkerBuilder_P()
{
}

bool OsmAnd::MapMarkerBuilder_P::isHidden() const
{
    QReadLocker scopedLocker(&_lock);

    return _isHidden;
}

void OsmAnd::MapMarkerBuilder_P::setIsHidden(const bool hidden)
{
    QWriteLocker scopedLocker(&_lock);

    _isHidden = hidden;
}

int OsmAnd::MapMarkerBuilder_P::getMarkerId() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _markerId;
}

void OsmAnd::MapMarkerBuilder_P::setMarkerId(const int markerId)
{
    QWriteLocker scopedLocker(&_lock);
    
    _markerId = markerId;
}

int OsmAnd::MapMarkerBuilder_P::getBaseOrder() const
{
    QReadLocker scopedLocker(&_lock);

    return _baseOrder;
}

void OsmAnd::MapMarkerBuilder_P::setBaseOrder(const int baseOrder)
{
    QWriteLocker scopedLocker(&_lock);

    _baseOrder = baseOrder;
}

bool OsmAnd::MapMarkerBuilder_P::isAccuracyCircleSupported() const
{
    QReadLocker scopedLocker(&_lock);

    return _isAccuracyCircleSupported;
}

void OsmAnd::MapMarkerBuilder_P::setIsAccuracyCircleSupported(const bool supported)
{
    QWriteLocker scopedLocker(&_lock);

    _isAccuracyCircleSupported = supported;
}

bool OsmAnd::MapMarkerBuilder_P::isAccuracyCircleVisible() const
{
    QReadLocker scopedLocker(&_lock);

    return _isAccuracyCircleVisible;
}

void OsmAnd::MapMarkerBuilder_P::setIsAccuracyCircleVisible(const bool visible)
{
    QWriteLocker scopedLocker(&_lock);

    _isAccuracyCircleVisible = visible;
}

double OsmAnd::MapMarkerBuilder_P::getAccuracyCircleRadius() const
{
    QReadLocker scopedLocker(&_lock);

    return _accuracyCircleRadius;
}

void OsmAnd::MapMarkerBuilder_P::setAccuracyCircleRadius(const double radius)
{
    QWriteLocker scopedLocker(&_lock);

    _accuracyCircleRadius = radius;
}

OsmAnd::FColorRGB OsmAnd::MapMarkerBuilder_P::getAccuracyCircleBaseColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _accuracyCircleBaseColor;
}

void OsmAnd::MapMarkerBuilder_P::setAccuracyCircleBaseColor(const FColorRGB baseColor)
{
    QWriteLocker scopedLocker(&_lock);

    _accuracyCircleBaseColor = baseColor;
}

OsmAnd::PointI OsmAnd::MapMarkerBuilder_P::getPosition() const
{
    QReadLocker scopedLocker(&_lock);

    return _position;
}

void OsmAnd::MapMarkerBuilder_P::setPosition(const PointI position)
{
    QWriteLocker scopedLocker(&_lock);

    _position = position;
}

float OsmAnd::MapMarkerBuilder_P::getHeight() const
{
    QReadLocker scopedLocker(&_lock);

    return _height;
}

void OsmAnd::MapMarkerBuilder_P::setHeight(const float height)
{
    QWriteLocker scopedLocker(&_lock);

    _height = height;
}

float OsmAnd::MapMarkerBuilder_P::getElevationScaleFactor() const
{
    QReadLocker scopedLocker(&_lock);

    return _elevationScaleFactor;
}

void OsmAnd::MapMarkerBuilder_P::setElevationScaleFactor(const float scaleFactor)
{
    QWriteLocker scopedLocker(&_lock);

    _elevationScaleFactor = scaleFactor;
}

sk_sp<const SkImage> OsmAnd::MapMarkerBuilder_P::getPinIcon() const
{
    QReadLocker scopedLocker(&_lock);

    return _pinIcon;
}

void OsmAnd::MapMarkerBuilder_P::setPinIcon(const SingleSkImage& image)
{
    QWriteLocker scopedLocker(&_lock);

    _pinIcon = image.sp;
}

OsmAnd::MapMarker::PinIconVerticalAlignment OsmAnd::MapMarkerBuilder_P::getPinIconVerticalAlignment() const
{
    QReadLocker scopedLocker(&_lock);

    return _pinIconVerticalAlignment;
}

OsmAnd::MapMarker::PinIconHorisontalAlignment OsmAnd::MapMarkerBuilder_P::getPinIconHorisontalAlignment() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _pinIconHorisontalAlignment;
}

void OsmAnd::MapMarkerBuilder_P::setPinIconVerticalAlignment(const MapMarker::PinIconVerticalAlignment value)
{
    QWriteLocker scopedLocker(&_lock);

    _pinIconVerticalAlignment = value;
}

void OsmAnd::MapMarkerBuilder_P::setPinIconHorisontalAlignment(const MapMarker::PinIconHorisontalAlignment value)
{
    QWriteLocker scopedLocker(&_lock);
    
    _pinIconHorisontalAlignment = value;
}

OsmAnd::PointI OsmAnd::MapMarkerBuilder_P::getPinIconOffset() const
{
    QReadLocker scopedLocker(&_lock);

    return _pinIconOffset;
}

void OsmAnd::MapMarkerBuilder_P::setPinIconOffset(const PointI pinIconOffset)
{
    QWriteLocker scopedLocker(&_lock);

    _pinIconOffset = pinIconOffset;
}

OsmAnd::ColorARGB OsmAnd::MapMarkerBuilder_P::getPinIconModulationColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _pinIconModulationColor;
}

void OsmAnd::MapMarkerBuilder_P::setPinIconModulationColor(const ColorARGB colorValue)
{
    QWriteLocker scopedLocker(&_lock);

    _pinIconModulationColor = colorValue;
}

OsmAnd::ColorARGB OsmAnd::MapMarkerBuilder_P::getOnSurfaceIconModulationColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _onSurfaceIconModulationColor;
}

void OsmAnd::MapMarkerBuilder_P::setOnSurfaceIconModulationColor(const ColorARGB colorValue)
{
    QWriteLocker scopedLocker(&_lock);

    _onSurfaceIconModulationColor = colorValue;
}


QString OsmAnd::MapMarkerBuilder_P::getCaption() const
{
    QReadLocker scopedLocker(&_lock);

    return _caption;
}

void OsmAnd::MapMarkerBuilder_P::setCaption(const QString& caption)
{
    QWriteLocker scopedLocker(&_lock);

    _caption = caption;
}

OsmAnd::TextRasterizer::Style OsmAnd::MapMarkerBuilder_P::getCaptionStyle() const
{
    QReadLocker scopedLocker(&_lock);

    return _captionStyle;
}

void OsmAnd::MapMarkerBuilder_P::setCaptionStyle(const TextRasterizer::Style captionStyle)
{
    QWriteLocker scopedLocker(&_lock);

    _captionStyle = captionStyle;
}

double OsmAnd::MapMarkerBuilder_P::getCaptionTopSpace() const
{
    QReadLocker scopedLocker(&_lock);

    return _captionTopSpace;
}

void OsmAnd::MapMarkerBuilder_P::setCaptionTopSpace(const double captionTopSpace)
{
    QWriteLocker scopedLocker(&_lock);

    _captionTopSpace = captionTopSpace;
}

QHash< OsmAnd::MapMarker::OnSurfaceIconKey, sk_sp<const SkImage> >
OsmAnd::MapMarkerBuilder_P::getOnMapSurfaceIcons() const
{
    QReadLocker scopedLocker(&_lock);

    return detachedOf(_onMapSurfaceIcons);
}

void OsmAnd::MapMarkerBuilder_P::addOnMapSurfaceIcon(
    const MapMarker::OnSurfaceIconKey key,
    const SingleSkImage& image)
{
    QWriteLocker scopedLocker(&_lock);

    _onMapSurfaceIcons.insert(key, image.sp);
}

void OsmAnd::MapMarkerBuilder_P::removeOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key)
{
    QWriteLocker scopedLocker(&_lock);

    _onMapSurfaceIcons.remove(key);
}

void OsmAnd::MapMarkerBuilder_P::clearOnMapSurfaceIcons()
{
    QWriteLocker scopedLocker(&_lock);

    _onMapSurfaceIcons.clear();
}

std::shared_ptr<const OsmAnd::Model3D> OsmAnd::MapMarkerBuilder_P::getModel3D() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _model3D;
}

void OsmAnd::MapMarkerBuilder_P::setModel3D(const std::shared_ptr<const Model3D>& model3D)
{
    QWriteLocker scopedLocker(&_lock);
    
    _model3D = model3D;
}

QHash<QString, OsmAnd::FColorARGB> OsmAnd::MapMarkerBuilder_P::getModel3DCustomMaterialColors() const
{
    QReadLocker scopedLocker(&_lock);
    
    return detachedOf(_model3DCustomMaterialColors);
}

void OsmAnd::MapMarkerBuilder_P::addModel3DCustomMaterialColor(const QString& materialName, const FColorARGB& color)
{
    QWriteLocker scopedLocker(&_lock);
    
    _model3DCustomMaterialColors.insert(materialName, color);
}

void OsmAnd::MapMarkerBuilder_P::removeModel3DCustomMaterialColor(const QString& materialName)
{
    QWriteLocker scopedLocker(&_lock);
    
    _model3DCustomMaterialColors.remove(materialName);
}

void OsmAnd::MapMarkerBuilder_P::clearModel3DCustomMaterialColors()
{
    QWriteLocker scopedLocker(&_lock);
    
    _model3DCustomMaterialColors.clear();
}

int OsmAnd::MapMarkerBuilder_P::getModel3DMaxSizeInPixels() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _model3DMaxSizeInPixels;
}

void OsmAnd::MapMarkerBuilder_P::setModel3DMaxSizeInPixels(const int maxSizeInPixels)
{
    QWriteLocker scopedLocker(&_lock);
    
    _model3DMaxSizeInPixels = maxSizeInPixels;
}

void OsmAnd::MapMarkerBuilder_P::setUpdateAfterCreated(bool updateAfterCreated)
{
    QWriteLocker scopedLocker(&_lock);
    
    _updateAfterCreated = updateAfterCreated;
}

std::shared_ptr<OsmAnd::MapMarker> OsmAnd::MapMarkerBuilder_P::buildAndAddToCollection(
    const std::shared_ptr<MapMarkersCollection>& collection)
{
    QReadLocker scopedLocker(&_lock);

    // Construct map symbols group for this marker
    const std::shared_ptr<MapMarker> marker(new MapMarker(
        _markerId,
        _baseOrder,
        _pinIcon,
        _pinIconVerticalAlignment,
        _pinIconHorisontalAlignment,
        _pinIconOffset,
        _captionTopSpace,
        detachedOf(_onMapSurfaceIcons),
        _model3D,
        detachedOf(_model3DCustomMaterialColors),
        _isAccuracyCircleSupported,
        _accuracyCircleBaseColor));
    
    marker->setCaption(_caption);
    marker->setCaptionStyle(_captionStyle);
    marker->setIsHidden(_isHidden);
    if (_isAccuracyCircleSupported)
    {
        marker->setIsAccuracyCircleVisible(_isAccuracyCircleVisible);
        marker->setAccuracyCircleRadius(_accuracyCircleRadius);
    }
    marker->setPosition(_position);
    marker->setHeight(_height);
    marker->setElevationScaleFactor(_elevationScaleFactor);
    marker->setPinIconModulationColor(_pinIconModulationColor);
    marker->setOnSurfaceIconModulationColor(_onSurfaceIconModulationColor);
    marker->setUpdateAfterCreated(_updateAfterCreated);

    if (marker->model3D)
    {
        marker->setModel3DMaxSizeInPixels(_model3DMaxSizeInPixels);
    }
    
    marker->applyChanges();

    // Add marker to collection and return it if adding was successful
    if (!collection->_p->addMarker(marker))
        return nullptr;
    return marker;
}
