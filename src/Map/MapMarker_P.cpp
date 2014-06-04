#include "MapMarker_P.h"
#include "MapMarker.h"

#include "MapSymbol.h"
#include "MapSymbolsGroup.h"
#include "PositionedRasterMapSymbol.h"
#include "SpriteMapSymbol.h"
#include "OnSurfaceMapSymbol.h"
#include "QKeyValueIterator.h"

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

float OsmAnd::MapMarker_P::getOnMapSurfaceIconDirection(const MapMarker::OnSurfaceIconKey key) const
{
    QReadLocker scopedLocker(&_lock);

    return _directions[key];
}

void OsmAnd::MapMarker_P::setOnMapSurfaceIconDirection(const MapMarker::OnSurfaceIconKey key, const float direction)
{
    QWriteLocker scopedLocker(&_lock);

    _directions[key] = direction;
    _hasUnappliedChanges = true;
}

bool OsmAnd::MapMarker_P::hasUnappliedChanges() const
{
    QReadLocker scopedLocker(&_lock);

    return _hasUnappliedChanges;
}

bool OsmAnd::MapMarker_P::applyChanges()
{
    QReadLocker scopedLocker1(&_lock);
    
    if (!_hasUnappliedChanges)
        return false;

    QReadLocker scopedLocker2(&_symbolsGroupsRegisterLock);
    for (const auto& symbolGroup_ : constOf(_symbolsGroupsRegister))
    {
        const auto symbolGroup = symbolGroup_.lock();
        if (!symbolGroup)
            continue;

        //TODO:
        //marker->setIsPrecisionCircleEnabled(_isPrecisionCircleEnabled);
        //marker->setPrecisionCircleRadius(_precisionCircleRadius);
        //marker->setPrecisionCircleBaseColor(_precisionCircleBaseColor);

        for (const auto& symbol_ : constOf(symbolGroup->symbols))
        {
            symbol_->isHidden = _isHidden;

            if (const auto symbol = std::dynamic_pointer_cast<IPositionedMapSymbol>(symbol_))
                symbol->setPosition31(_position);

            if (const auto symbol = std::dynamic_pointer_cast<KeyedOnSurfaceMapSymbol>(symbol_))
            {
                const auto citDirection = _directions.constFind(symbol->key);
                if (citDirection != _directions.cend())
                    symbol->direction = *citDirection;
            }
        }
    }

    _hasUnappliedChanges = false;

    return true;
}

std::shared_ptr<OsmAnd::MapSymbolsGroup> OsmAnd::MapMarker_P::inflateSymbolsGroup() const
{
    QReadLocker scopedLocker(&_lock);

    // Construct new map symbols group for this marker
    const std::shared_ptr<MapSymbolsGroup> symbolsGroup(new LinkedMapSymbolsGroup(
        std::const_pointer_cast<MapMarker_P>(shared_from_this())));
    int order = owner->baseOrder;

    // Map marker consists one or more of:
    // 1. Set of OnSurfaceMapSymbol from onMapSurfaceIcons
    for (const auto& itOnMapSurfaceIcon : rangeOf(constOf(owner->onMapSurfaceIcons)))
    {
        const auto key = itOnMapSurfaceIcon.key();
        const auto& onMapSurfaceIcon = itOnMapSurfaceIcon.value();

        std::shared_ptr<SkBitmap> iconClone(new SkBitmap());
        onMapSurfaceIcon->deepCopyTo(iconClone.get(), onMapSurfaceIcon->getConfig());

        // Get direction
        float direction = 0.0f;
        const auto citDirection = _directions.constFind(key);
        if (citDirection != _directions.cend())
            direction = *citDirection;

        const std::shared_ptr<KeyedOnSurfaceMapSymbol> onMapSurfaceIconSymbol(new KeyedOnSurfaceMapSymbol(
            key,
            symbolsGroup,
            false, // This symbol is not shareable
            order++,
            static_cast<MapSymbol::IntersectionModeFlags>(MapSymbol::IgnoredByIntersectionTest | MapSymbol::TransparentForIntersectionLookup),
            iconClone,
            QString().sprintf("markerGroup(%p:%p)->onMapSurfaceIconBitmap:%p", this, symbolsGroup.get(), iconClone->getPixels()),
            LanguageId::Invariant,
            PointI(), // Since minDistance is (0, 0), this map symbol will not be compared to others
            _position));
        onMapSurfaceIconSymbol->direction = direction;
        onMapSurfaceIconSymbol->isHidden = _isHidden;
        symbolsGroup->symbols.push_back(onMapSurfaceIconSymbol);
    }

    //TODO: 2. Special OnSurfaceMapSymbol that represents precision circle (PrimitiveOnSurfaceMapSymbol)

    // 3. SpriteMapSymbol with pinIconBitmap as an icon
    if (owner->pinIcon)
    {
        std::shared_ptr<SkBitmap> pinIcon(new SkBitmap());
        owner->pinIcon->deepCopyTo(pinIcon.get(), owner->pinIcon->getConfig());

        const std::shared_ptr<MapSymbol> pinIconSymbol(new SpriteMapSymbol(
            symbolsGroup,
            false, // This symbol is not shareable
            order++,
            static_cast<MapSymbol::IntersectionModeFlags>(MapSymbol::IgnoredByIntersectionTest | MapSymbol::TransparentForIntersectionLookup),
            pinIcon,
            QString().sprintf("markerGroup(%p:%p)->pinIconBitmap:%p", this, symbolsGroup.get(), pinIcon->getPixels()),
            LanguageId::Invariant,
            PointI(), // Since minDistance is (0, 0), this map symbol will not be compared to others
            _position,
            PointI(0, -pinIcon->height() / 2)));
        pinIconSymbol->isHidden = _isHidden;
        symbolsGroup->symbols.push_back(pinIconSymbol);
    }

    return symbolsGroup;
}

std::shared_ptr<OsmAnd::MapSymbolsGroup> OsmAnd::MapMarker_P::createSymbolsGroup() const
{
    const auto inflatedSymbolsGroup = inflateSymbolsGroup();
    registerSymbolsGroup(inflatedSymbolsGroup);
    return inflatedSymbolsGroup;
}

void OsmAnd::MapMarker_P::registerSymbolsGroup(const std::shared_ptr<MapSymbolsGroup>& symbolsGroup) const
{
    QWriteLocker scopedLocker(&_symbolsGroupsRegisterLock);

    _symbolsGroupsRegister.insert(symbolsGroup.get(), symbolsGroup);
}

void OsmAnd::MapMarker_P::unregisterSymbolsGroup(MapSymbolsGroup* const symbolsGroup) const
{
    QWriteLocker scopedLocker(&_symbolsGroupsRegisterLock);

    _symbolsGroupsRegister.remove(symbolsGroup);
}

OsmAnd::MapMarker_P::LinkedMapSymbolsGroup::LinkedMapSymbolsGroup(const std::shared_ptr<MapMarker_P>& mapMarkerP_)
    : mapMarkerP(mapMarkerP_)
{
}

OsmAnd::MapMarker_P::LinkedMapSymbolsGroup::~LinkedMapSymbolsGroup()
{
    if (const auto mapMarkerP_ = mapMarkerP.lock())
        mapMarkerP_->unregisterSymbolsGroup(this);
}

void OsmAnd::MapMarker_P::LinkedMapSymbolsGroup::update()
{
    if (const auto mapMarkerP_ = mapMarkerP.lock())
        mapMarkerP_->applyChanges();
}

OsmAnd::MapMarker_P::KeyedOnSurfaceMapSymbol::KeyedOnSurfaceMapSymbol(
    const MapMarker::OnSurfaceIconKey key_,
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const QString& content_,
    const LanguageId& languageId_,
    const PointI& minDistance_,
    const PointI& location31_)
    : OnSurfaceMapSymbol(group_, isShareable_, order_, intersectionModeFlags_, bitmap_, content_, languageId_, minDistance_, location31_)
    , key(key_)
{
}

OsmAnd::MapMarker_P::KeyedOnSurfaceMapSymbol::~KeyedOnSurfaceMapSymbol()
{
}
