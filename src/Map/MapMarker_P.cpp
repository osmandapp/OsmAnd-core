#include "MapMarker_P.h"
#include "MapMarker.h"

#include "MapSymbol.h"
#include "MapSymbolsGroup.h"
#include "BoundToPointMapSymbol.h"
#include "SpriteMapSymbol.h"

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
    QWriteLocker scopedLocker(&_lock);

    _isHidden = hidden;
    _hasUnappliedChanges = true;
}

bool OsmAnd::MapMarker_P::isPrecisionCircleEnabled() const
{
    QReadLocker scopedLocker(&_lock);

    return _isPrecisionCircleEnabled;
}

void OsmAnd::MapMarker_P::setIsPrecisionCircleEnabled(const bool enabled)
{
    QWriteLocker scopedLocker(&_lock);

    _isPrecisionCircleEnabled = enabled;
    _hasUnappliedChanges = true;
}

double OsmAnd::MapMarker_P::getPrecisionCircleRadius() const
{
    QReadLocker scopedLocker(&_lock);

    return _precisionCircleRadius;
}

void OsmAnd::MapMarker_P::setPrecisionCircleRadius(const double radius)
{
    QWriteLocker scopedLocker(&_lock);

    _precisionCircleRadius = radius;
    _hasUnappliedChanges = true;
}

SkColor OsmAnd::MapMarker_P::getPrecisionCircleBaseColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _precisionCircleBaseColor;
}

void OsmAnd::MapMarker_P::setPrecisionCircleBaseColor(const SkColor baseColor)
{
    QWriteLocker scopedLocker(&_lock);

    _precisionCircleBaseColor = baseColor;
    _hasUnappliedChanges = true;
}

OsmAnd::PointI OsmAnd::MapMarker_P::getPosition() const
{
    QReadLocker scopedLocker(&_lock);

    return _position;
}

void OsmAnd::MapMarker_P::setPosition(const PointI position)
{
    QWriteLocker scopedLocker(&_lock);

    _position = position;
    _hasUnappliedChanges = true;
}

float OsmAnd::MapMarker_P::getDirection() const
{
    QReadLocker scopedLocker(&_lock);

    return _direction;
}

void OsmAnd::MapMarker_P::setDirection(const float direction)
{
    QWriteLocker scopedLocker(&_lock);

    _direction = direction;
    _hasUnappliedChanges = true;
}

bool OsmAnd::MapMarker_P::hasUnappliedChanges() const
{
    QReadLocker scopedLocker(&_lock);

    return _hasUnappliedChanges;
}

bool OsmAnd::MapMarker_P::applyChanges()
{
    QReadLocker scopedLocker(&_lock);

    if (!_hasUnappliedChanges)
        return false;

    //for (const auto& symbol_ : constOf(owner->mapSymbolsGroup->symbols))
    //{
    //    const auto symbol = std::const_pointer_cast<MapSymbol>(symbol_);

    //    //TODO:
    //    //marker->setIsHidden(_isHidden);
    //    //marker->setIsPrecisionCircleEnabled(_isPrecisionCircleEnabled);
    //    //marker->setPrecisionCircleRadius(_precisionCircleRadius);
    //    //marker->setPrecisionCircleBaseColor(_precisionCircleBaseColor);

    //    if (const auto symbolWithPosition = std::dynamic_pointer_cast<BoundToPointMapSymbol>(symbol))
    //        symbolWithPosition->location31 = _position;

    //    //marker->setDirection(_direction);
    //}

    _hasUnappliedChanges = false;

    return true;
}

std::shared_ptr<OsmAnd::MapSymbolsGroup> OsmAnd::MapMarker_P::inflateSymbolsGroup() const
{
    QReadLocker scopedLocker(&_lock);

    // Construct new map symbols group for this marker
    const std::shared_ptr<MapSymbolsGroup> symbolsGroup(new MapSymbolsGroup());
    int order = owner->baseOrder;

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

    // 3. SpriteMapSymbol with pinIconBitmap as an icon
    if (!owner->pinIcon)
    {
        std::shared_ptr<SkBitmap> pinIcon(new SkBitmap());
        owner->pinIcon->deepCopyTo(pinIcon.get(), owner->pinIcon->getConfig());

        const std::shared_ptr<SpriteMapSymbol> pinIconSymbol(new SpriteMapSymbol(
            symbolsGroup,
            false, // This symbol is not shareable
            pinIcon,
            order++,
            static_cast<MapSymbol::IntersectionModeFlags>(MapSymbol::IgnoredByIntersectionTest | MapSymbol::TransparentForIntersectionLookup),
            QString().sprintf("markerGroup(%p:%p)->pinIconBitmap:%p", this, symbolsGroup.get(), pinIcon->getPixels()),
            LanguageId::Invariant,
            PointI(), // Since minDistance is (0, 0), this map symbol will not be compared to others
            _position,
            PointI(0, -pinIcon->height() / 2)));
        symbolsGroup->symbols.push_back(pinIconSymbol);
    }

    return symbolsGroup;
}

std::shared_ptr<OsmAnd::MapSymbolsGroup> OsmAnd::MapMarker_P::createSymbolsGroup() const
{
    const auto inflatedSymbolsGroup = inflateSymbolsGroup();

    return inflatedSymbolsGroup;
}
