#include "MapMarkerBuilder_P.h"
#include "MapMarkerBuilder.h"

#include "MapMarker.h"
#include "MapMarker_P.h"
#include "MapMarkersCollection.h"
#include "MapMarkersCollection_P.h"
#include "MapSymbol.h"
#include "MapSymbolsGroup.h"
#include "SpriteMapSymbol.h"
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

float OsmAnd::MapMarkerBuilder_P::getDirection() const
{
    QReadLocker scopedLocker(&_lock);

    return _direction;
}

void OsmAnd::MapMarkerBuilder_P::setDirection(const float direction)
{
    QWriteLocker scopedLocker(&_lock);

    _direction = Utilities::normalizedAngleDegrees(direction);
}

const SkBitmap OsmAnd::MapMarkerBuilder_P::getPinIcon() const
{
    QReadLocker scopedLocker(&_lock);

    return _pinIconBitmap;
}

void OsmAnd::MapMarkerBuilder_P::setPinIcon(const SkBitmap& bitmap)
{
    QWriteLocker scopedLocker(&_lock);

    _pinIconBitmap.reset();
    bool ok = bitmap.deepCopyTo(&_pinIconBitmap, bitmap.getConfig());
    assert(ok);
}

QList< QPair<const SkBitmap, bool> > OsmAnd::MapMarkerBuilder_P::getMapIcons() const
{
    QReadLocker scopedLocker(&_lock);

    return detachedOf(_mapIconsBitmaps);
}

void OsmAnd::MapMarkerBuilder_P::clearMapIcons()
{
    QWriteLocker scopedLocker(&_lock);

    _mapIconsBitmaps.clear();
}

void OsmAnd::MapMarkerBuilder_P::addMapIcon(const SkBitmap& bitmap, const bool respectsDirection)
{
    QWriteLocker scopedLocker(&_lock);

    SkBitmap bitmapClone;
    bool ok = bitmap.deepCopyTo(&bitmapClone, bitmap.getConfig());
    assert(ok);

    _mapIconsBitmaps.push_back(QPair<const SkBitmap, bool>(bitmapClone, respectsDirection));
}

std::shared_ptr<OsmAnd::MapMarker> OsmAnd::MapMarkerBuilder_P::buildAndAddToCollection(const std::shared_ptr<MapMarkersCollection>& collection)
{
    QReadLocker scopedLocker(&_lock);

    // Construct map symbols group for this marker
    const std::shared_ptr<MapSymbolsGroup> symbolsGroup(new MapSymbolsGroup());
    const std::shared_ptr<MapMarker> marker(new MapMarker(symbolsGroup));
    marker->setIsHidden(_isHidden);
    marker->setIsPrecisionCircleEnabled(_isPrecisionCircleEnabled);
    marker->setPrecisionCircleRadius(_precisionCircleRadius);
    marker->setPrecisionCircleBaseColor(_precisionCircleBaseColor);
    marker->setPosition(_position);
    marker->setDirection(_direction);
    int order = _baseOrder;

    /*
    // Map marker consists one or more of:
    // - Set of OnSurfaceMapSymbol from _mapIconsBitmaps
    for (const auto& mapIconPair : constOf(_mapIconsBitmaps))
    {
        std::shared_ptr<OnSurfaceMapSymbol> symbol(new OnSurfaceMapSymbol(
            marker->_p->_symbolsGroup,
            false,
            BITMAP,
            order,
            content,
            language,
            minDistance,
            location));
    }

    // 2. Special OnSurfaceMapSymbol that represents precision circle (PrimitiveOnSurfaceMapSymbol)
    */
    // 3. SpriteMapSymbol with _pinIconBitmap as an icon
    if (!_pinIconBitmap.empty())
    {
        std::shared_ptr<SkBitmap> pinIconBitmap(new SkBitmap());
        _pinIconBitmap.deepCopyTo(pinIconBitmap.get(), _pinIconBitmap.getConfig());

        const std::shared_ptr<SpriteMapSymbol> pinIconSymbol(new SpriteMapSymbol(
            symbolsGroup,
            false, // This symbol is not shareable
            pinIconBitmap,
            order++,
            static_cast<MapSymbol::IntersectionModeFlags>(MapSymbol::IgnoredByIntersectionTest | MapSymbol::TransparentForIntersectionLookup),
            QString().sprintf("marker(%p)->pinBitmap:%p", marker.get(), _pinIconBitmap.getPixels()),
            LanguageId::Invariant,
            PointI(), // Since minDistance is (0, 0), this map symbol will not be compared to others
            _position,
            PointI(0, -pinIconBitmap->height()/2)));
        symbolsGroup->symbols.push_back(pinIconSymbol);
    }

    // Add marker to collection and return it if adding was successful
    marker->applyChanges();
    if (!collection->_p->addMarker(marker))
        return nullptr;
    return marker;
}
