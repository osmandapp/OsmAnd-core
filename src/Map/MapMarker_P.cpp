#include "MapMarker_P.h"
#include "MapMarker.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkImage.h>
#include "restore_internal_warnings.h"

#include "MapSymbol.h"
#include "MapSymbolsGroup.h"
#include "BillboardRasterMapSymbol.h"
#include "OnSurfaceRasterMapSymbol.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "QKeyValueIterator.h"
#include "MapSymbolIntersectionClassesRegistry.h"
#include "IMapRenderer.h"

OsmAnd::MapMarker_P::MapMarker_P(MapMarker* const owner_)
    : owner(owner_)
    , textRasterizer(TextRasterizer::getDefault())
    , _accuracyCircleRadius(0.0)
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
    
    if (_isHidden != hidden)
    {
        _isHidden = hidden;
        _hasUnappliedChanges = true;
    }
}

bool OsmAnd::MapMarker_P::isAccuracyCircleVisible() const
{
    QReadLocker scopedLocker(&_lock);

    return _isAccuracyCircleVisible;
}

void OsmAnd::MapMarker_P::setIsAccuracyCircleVisible(const bool visible)
{
    QWriteLocker scopedLocker(&_lock);

    if (_isAccuracyCircleVisible != visible)
    {
        _isAccuracyCircleVisible = visible;
        _hasUnappliedChanges = true;
    }
}

double OsmAnd::MapMarker_P::getAccuracyCircleRadius() const
{
    QReadLocker scopedLocker(&_lock);

    return _accuracyCircleRadius;
}

void OsmAnd::MapMarker_P::setAccuracyCircleRadius(const double radius)
{
    QWriteLocker scopedLocker(&_lock);

    if (_accuracyCircleRadius != radius)
    {
        _accuracyCircleRadius = radius;
        _hasUnappliedChanges = true;
    }
}

OsmAnd::PointI OsmAnd::MapMarker_P::getPosition() const
{
    QReadLocker scopedLocker(&_lock);

    return _position;
}

void OsmAnd::MapMarker_P::setPosition(const PointI position)
{
    QWriteLocker scopedLocker(&_lock);

    if (_position != position)
    {
        _position = position;
        _hasUnappliedChanges = true;
    }
}

float OsmAnd::MapMarker_P::getHeight() const
{
    QReadLocker scopedLocker(&_lock);

    return _height;
}

void OsmAnd::MapMarker_P::setHeight(const float height)
{
    QWriteLocker scopedLocker(&_lock);

    if (_height != height)
    {
        _height = height;
        _hasUnappliedChanges = true;
    }
}

float OsmAnd::MapMarker_P::getElevationScaleFactor() const
{
    QReadLocker scopedLocker(&_lock);

    return _elevationScaleFactor;
}

void OsmAnd::MapMarker_P::setElevationScaleFactor(const float scaleFactor)
{
    QWriteLocker scopedLocker(&_lock);

    if (_elevationScaleFactor != scaleFactor)
    {
        _elevationScaleFactor = scaleFactor;
        _hasUnappliedChanges = true;
    }
}

float OsmAnd::MapMarker_P::getOnMapSurfaceIconDirection(const MapMarker::OnSurfaceIconKey key) const
{
    QReadLocker scopedLocker(&_lock);

    return _directions[key];
}

void OsmAnd::MapMarker_P::setOnMapSurfaceIconDirection(const MapMarker::OnSurfaceIconKey key, const float direction)
{
    QWriteLocker scopedLocker(&_lock);

    if (_directions[key] != direction)
    {
        _directions[key] = direction;
        _hasUnappliedChanges = true;
    }
}

OsmAnd::ColorARGB OsmAnd::MapMarker_P::getPinIconModulationColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _pinIconModulationColor;
}

void OsmAnd::MapMarker_P::setPinIconModulationColor(const ColorARGB colorValue)
{
    QWriteLocker scopedLocker(&_lock);

    if (_pinIconModulationColor != colorValue)
    {
        _pinIconModulationColor = colorValue;
        _hasUnappliedChanges = true;
    }
}

bool OsmAnd::MapMarker_P::hasUnappliedChanges() const
{
    QReadLocker scopedLocker(&_lock);

    return _hasUnappliedChanges;
}

bool OsmAnd::MapMarker_P::applyChanges(IMapRenderer& mapRenderer)
{
    QReadLocker scopedLocker1(&_lock);
    
    if (!_hasUnappliedChanges)
        return false;

    if (_isAccuracyCircleVisible && !_isHidden)
    {
        mapRenderer.setMyLocationColor(owner->accuracyCircleBaseColor.withAlpha(0.25f));
        mapRenderer.setMyLocation31(_position);
        mapRenderer.setMyLocationRadiusInMeters(_accuracyCircleRadius);
    }

    QReadLocker scopedLocker2(&_symbolsGroupsRegistryLock);
    for (const auto& symbolGroup_ : constOf(_symbolsGroupsRegistry))
    {
        const auto symbolGroup = symbolGroup_.lock();
        if (!symbolGroup)
            continue;

        for (const auto& symbol_ : constOf(symbolGroup->symbols))
        {
            symbol_->isHidden = _isHidden;

            if (const auto symbol = std::dynamic_pointer_cast<BillboardRasterMapSymbol>(symbol_))
            {
                symbol->setPosition31(_position);
                symbol->setElevation(_height);
                symbol->setElevationScaleFactor(_elevationScaleFactor);
                symbol->modulationColor = _pinIconModulationColor;
            }

            if (const auto symbol = std::dynamic_pointer_cast<KeyedOnSurfaceRasterMapSymbol>(symbol_))
            {
                symbol->setPosition31(_position);

                const auto citDirection = _directions.constFind(symbol->key);
                if (citDirection != _directions.cend())
                    symbol->direction = *citDirection;

                symbol->setElevation(_height);
                symbol->setElevationScaleFactor(_elevationScaleFactor);
            }
        }
    }

    _hasUnappliedChanges = false;

    return true;
}

std::shared_ptr<OsmAnd::MapMarker::SymbolsGroup> OsmAnd::MapMarker_P::inflateSymbolsGroup() const
{
    QReadLocker scopedLocker(&_lock);

    bool ok;

    auto& mapSymbolIntersectionClassesRegistry = MapSymbolIntersectionClassesRegistry::globalInstance();

    // Construct new map symbols group for this marker
    const std::shared_ptr<MapMarker::SymbolsGroup> symbolsGroup(new MapMarker::SymbolsGroup(
        std::const_pointer_cast<MapMarker_P>(shared_from_this())));
    symbolsGroup->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAnything;

    int order = owner->baseOrder;

    // SpriteMapSymbol with pinIcon as an icon
    if (owner->pinIcon)
    {
        const std::shared_ptr<BillboardRasterMapSymbol> pinIconSymbol(new BillboardRasterMapSymbol(symbolsGroup));
        pinIconSymbol->order = order++;
        pinIconSymbol->image = owner->pinIcon;
        pinIconSymbol->size = PointI(owner->pinIcon->width(), owner->pinIcon->height());
        pinIconSymbol->content = QString().sprintf(
           "markerGroup(%p:%p)->pinIcon:%p",
           this,
           symbolsGroup.get(),
           owner->pinIcon.get());
        pinIconSymbol->languageId = LanguageId::Invariant;
        pinIconSymbol->position31 = _position;
        PointI offset;
        switch (owner->pinIconHorisontalAlignment)
        {
            case PinIconHorisontalAlignment::Left:
                offset.x = -owner->pinIcon->width() / 2;
                break;
            case PinIconHorisontalAlignment::Right:
                offset.x = owner->pinIcon->width() / 2;
                break;
            case PinIconHorisontalAlignment::CenterHorizontal:
            default:
                offset.x = 0;
                break;
        }
        switch (owner->pinIconVerticalAlignment)
        {
            case PinIconVerticalAlignment::Top:
                offset.y = -owner->pinIcon->height() / 2;
                break;
            case PinIconVerticalAlignment::Bottom:
                offset.y = owner->pinIcon->height() / 2;
                break;
            case PinIconVerticalAlignment::CenterVertical:
            default:
                offset.y = 0;
                break;
        }
        pinIconSymbol->offset = offset + owner->pinIconOffset;
        pinIconSymbol->isHidden = _isHidden;
        pinIconSymbol->modulationColor = _pinIconModulationColor;
        symbolsGroup->symbols.push_back(pinIconSymbol);
        
        const auto caption = owner->caption;
        if (!caption.isEmpty())
        {
            const auto textStyle = owner->captionStyle;
            const auto textImage = textRasterizer->rasterize(caption, textStyle);
            if (textImage)
            {
                const auto mapSymbolCaption = std::make_shared<BillboardRasterMapSymbol>(symbolsGroup);
                mapSymbolCaption->order = order - 2;
                mapSymbolCaption->image = textImage;
                mapSymbolCaption->contentClass = OsmAnd::MapSymbol::ContentClass::Caption;
                mapSymbolCaption->intersectsWithClasses.insert(
                    mapSymbolIntersectionClassesRegistry.getOrRegisterClassIdByName(QStringLiteral("text_layer_caption")));
                mapSymbolCaption->setOffset(PointI(offset.x, owner->pinIcon->height() / 2 + textImage->height() / 2 + offset.y + owner->captionTopSpace));
                mapSymbolCaption->size = PointI(textImage->width(), textImage->height());
                mapSymbolCaption->languageId = LanguageId::Invariant;
                mapSymbolCaption->position31 = _position;
                mapSymbolCaption->elevation = _height;
                mapSymbolCaption->elevationScaleFactor = _elevationScaleFactor;
                symbolsGroup->symbols.push_back(mapSymbolCaption);
            }
        }
    }
    
    order++;
    
    int surfOrder = order;
    // Set of OnSurfaceMapSymbol from onMapSurfaceIcons
    for (const auto& itOnMapSurfaceIcon : rangeOf(constOf(owner->onMapSurfaceIcons)))
    {
        const auto key = itOnMapSurfaceIcon.key();
        const auto& onMapSurfaceIcon = itOnMapSurfaceIcon.value();
        if (!onMapSurfaceIcon)
            continue;

        // Get direction
        float direction = 0.0f;
        const auto citDirection = _directions.constFind(key);
        if (citDirection != _directions.cend())
            direction = *citDirection;
        
        const std::shared_ptr<KeyedOnSurfaceRasterMapSymbol> onMapSurfaceIconSymbol(new KeyedOnSurfaceRasterMapSymbol(
            key,
            symbolsGroup));
        int o = (int)(surfOrder + (long)key);
        if (order < o)
            order = o;
        
        onMapSurfaceIconSymbol->order = o;
        
        onMapSurfaceIconSymbol->image = onMapSurfaceIcon;
        onMapSurfaceIconSymbol->size = PointI(onMapSurfaceIcon->width(), onMapSurfaceIcon->height());
        onMapSurfaceIconSymbol->content = QString().sprintf(
            "markerGroup(%p:%p)->onMapSurfaceIcon:%p",
            this,
            symbolsGroup.get(),
            onMapSurfaceIcon.get());
        onMapSurfaceIconSymbol->languageId = LanguageId::Invariant;
        onMapSurfaceIconSymbol->position31 = _position;
        onMapSurfaceIconSymbol->direction = direction;
        onMapSurfaceIconSymbol->elevation = _height;
        onMapSurfaceIconSymbol->elevationScaleFactor = _elevationScaleFactor;
        onMapSurfaceIconSymbol->isHidden = _isHidden;
        symbolsGroup->symbols.push_back(onMapSurfaceIconSymbol);
    }
    
    order++;

    return symbolsGroup;
}

std::shared_ptr<OsmAnd::MapMarker::SymbolsGroup> OsmAnd::MapMarker_P::createSymbolsGroup() const
{
    const auto inflatedSymbolsGroup = inflateSymbolsGroup();
    registerSymbolsGroup(inflatedSymbolsGroup);
    return inflatedSymbolsGroup;
}

void OsmAnd::MapMarker_P::registerSymbolsGroup(const std::shared_ptr<MapSymbolsGroup>& symbolsGroup) const
{
    QWriteLocker scopedLocker(&_symbolsGroupsRegistryLock);

    _symbolsGroupsRegistry.insert(symbolsGroup.get(), symbolsGroup);
}

void OsmAnd::MapMarker_P::unregisterSymbolsGroup(MapSymbolsGroup* const symbolsGroup) const
{
    QWriteLocker scopedLocker(&_symbolsGroupsRegistryLock);

    _symbolsGroupsRegistry.remove(symbolsGroup);
}

OsmAnd::MapMarker_P::KeyedOnSurfaceRasterMapSymbol::KeyedOnSurfaceRasterMapSymbol(
    const MapMarker::OnSurfaceIconKey key_,
    const std::shared_ptr<MapSymbolsGroup>& group_)
    : OnSurfaceRasterMapSymbol(group_)
    , key(key_)
{
}

OsmAnd::MapMarker_P::KeyedOnSurfaceRasterMapSymbol::~KeyedOnSurfaceRasterMapSymbol()
{
}
