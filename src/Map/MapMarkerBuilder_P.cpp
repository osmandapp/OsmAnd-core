#include "MapMarkerBuilder_P.h"
#include "MapMarkerBuilder.h"

#include "MapMarker.h"
#include "MapMarker_P.h"
#include "MapMarkersCollection.h"
#include "MapMarkersCollection_P.h"
#include "Utilities.h"

OsmAnd::MapMarkerBuilder_P::MapMarkerBuilder_P(MapMarkerBuilder* const owner_)
    : _isHidden(false)
    , _baseOrder(std::numeric_limits<int>::min()) //NOTE: See Rasterizer_P.cpp:1115 - this is needed to keep markers as the most important symbols
    , _isPrecisionCircleEnabled(false)
    , _precisionCircleRadius(0.0)
    , _precisionCircleBaseColor(SK_ColorTRANSPARENT)
    , _direction(0.0f)
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

bool OsmAnd::MapMarkerBuilder_P::isPrecisionCircleEnabled() const
{
    QReadLocker scopedLocker(&_lock);

    return _isPrecisionCircleEnabled;
}

void OsmAnd::MapMarkerBuilder_P::setIsPrecisionCircleEnabled(const bool enabled)
{
    QWriteLocker scopedLocker(&_lock);

    _isPrecisionCircleEnabled = enabled;
}

double OsmAnd::MapMarkerBuilder_P::getPrecisionCircleRadius() const
{
    QReadLocker scopedLocker(&_lock);

    return _precisionCircleRadius;
}

void OsmAnd::MapMarkerBuilder_P::setPrecisionCircleRadius(const double radius)
{
    QWriteLocker scopedLocker(&_lock);

    _precisionCircleRadius = radius;
}

SkColor OsmAnd::MapMarkerBuilder_P::getPrecisionCircleBaseColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _precisionCircleBaseColor;
}

void OsmAnd::MapMarkerBuilder_P::setPrecisionCircleBaseColor(const SkColor baseColor)
{
    QWriteLocker scopedLocker(&_lock);

    _precisionCircleBaseColor = baseColor;
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

std::shared_ptr<const SkBitmap> OsmAnd::MapMarkerBuilder_P::getPinIcon() const
{
    QReadLocker scopedLocker(&_lock);

    return _pinIcon;
}

void OsmAnd::MapMarkerBuilder_P::setPinIcon(const std::shared_ptr<const SkBitmap>& bitmap)
{
    QWriteLocker scopedLocker(&_lock);

    _pinIcon = bitmap;
}

QHash< OsmAnd::MapMarker::OnSurfaceIconKey, std::shared_ptr<const SkBitmap> > OsmAnd::MapMarkerBuilder_P::getOnMapSurfaceIcons() const
{
    QReadLocker scopedLocker(&_lock);

    return detachedOf(_onMapSurfaceIcons);
}

void OsmAnd::MapMarkerBuilder_P::addOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key, const std::shared_ptr<const SkBitmap>& bitmap)
{
    QWriteLocker scopedLocker(&_lock);

    _onMapSurfaceIcons.insert(key, bitmap);
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

std::shared_ptr<OsmAnd::MapMarker> OsmAnd::MapMarkerBuilder_P::buildAndAddToCollection(const std::shared_ptr<MapMarkersCollection>& collection)
{
    QReadLocker scopedLocker(&_lock);

    // Construct map symbols group for this marker
    const std::shared_ptr<MapMarker> marker(new MapMarker(_baseOrder, _pinIcon, detachedOf(_onMapSurfaceIcons)));
    marker->setIsHidden(_isHidden);
    marker->setIsPrecisionCircleEnabled(_isPrecisionCircleEnabled);
    marker->setPrecisionCircleRadius(_precisionCircleRadius);
    marker->setPrecisionCircleBaseColor(_precisionCircleBaseColor);
    marker->setPosition(_position);
    marker->applyChanges();

    // Add marker to collection and return it if adding was successful
    if (!collection->_p->addMarker(marker))
        return nullptr;
    return marker;
}
