#include "MapMarker.h"
#include "MapMarker_P.h"

OsmAnd::MapMarker::MapMarker(
    const int markerId_,
    const int baseOrder_,
    const sk_sp<const SkImage>& pinIcon_,
    const PinIconVerticalAlignment pinIconVerticalAlignment_,
    const PinIconHorisontalAlignment pinIconHorisontalAlignment_,
    const PointI pinIconOffset_,
    const QString& caption_,
    const TextRasterizer::Style captionStyle_,
    const double captionTopSpace_,
    const QHash< OnSurfaceIconKey, sk_sp<const SkImage> >& onMapSurfaceIcons_,
    const bool isAccuracyCircleSupported_,
    const FColorRGB accuracyCircleBaseColor_)
    : _p(new MapMarker_P(this))
    , markerId(markerId_)
    , baseOrder(baseOrder_)
    , pinIcon(pinIcon_)
    , pinIconVerticalAlignment(pinIconVerticalAlignment_)
    , pinIconHorisontalAlignment(pinIconHorisontalAlignment_)
    , pinIconOffset(pinIconOffset_)
    , caption(caption_)
    , captionStyle(captionStyle_)
    , captionTopSpace(captionTopSpace_)
    , onMapSurfaceIcons(onMapSurfaceIcons_)
    , isAccuracyCircleSupported(isAccuracyCircleSupported_)
    , accuracyCircleBaseColor(accuracyCircleBaseColor_)
{
}

OsmAnd::MapMarker::~MapMarker()
{
}

bool OsmAnd::MapMarker::isHidden() const
{
    return _p->isHidden();
}

void OsmAnd::MapMarker::setIsHidden(const bool hidden)
{
    _p->setIsHidden(hidden);
}

bool OsmAnd::MapMarker::isAccuracyCircleVisible() const
{
    return _p->isAccuracyCircleVisible();
}

void OsmAnd::MapMarker::setIsAccuracyCircleVisible(const bool visible)
{
    _p->setIsAccuracyCircleVisible(visible);
}

double OsmAnd::MapMarker::getAccuracyCircleRadius() const
{
    return _p->getAccuracyCircleRadius();
}

void OsmAnd::MapMarker::setAccuracyCircleRadius(const double radius)
{
    _p->setAccuracyCircleRadius(radius);
}

OsmAnd::PointI OsmAnd::MapMarker::getPosition() const
{
    return _p->getPosition();
}

void OsmAnd::MapMarker::setPosition(const PointI position)
{
    _p->setPosition(position);
}

float OsmAnd::MapMarker::getHeight() const
{
    return _p->getHeight();
}

void OsmAnd::MapMarker::setHeight(const float height)
{
    _p->setHeight(height);
}

float OsmAnd::MapMarker::getOnMapSurfaceIconDirection(const OnSurfaceIconKey key) const
{
    return _p->getOnMapSurfaceIconDirection(key);
}

void OsmAnd::MapMarker::setOnMapSurfaceIconDirection(const OnSurfaceIconKey key, const float direction)
{
    _p->setOnMapSurfaceIconDirection(key, direction);
}

OsmAnd::ColorARGB OsmAnd::MapMarker::getPinIconModulationColor() const
{
    return _p->getPinIconModulationColor();
}

void OsmAnd::MapMarker::setPinIconModulationColor(const ColorARGB colorValue)
{
    _p->setPinIconModulationColor(colorValue);
}

bool OsmAnd::MapMarker::hasUnappliedChanges() const
{
    return _p->hasUnappliedChanges();
}

bool OsmAnd::MapMarker::applyChanges()
{
    return _p->applyChanges();
}

std::shared_ptr<OsmAnd::MapMarker::SymbolsGroup> OsmAnd::MapMarker::createSymbolsGroup() const
{
    return _p->createSymbolsGroup();
}

OsmAnd::MapMarker::SymbolsGroup::SymbolsGroup(const std::shared_ptr<MapMarker_P>& mapMarkerP_)
    : _mapMarkerP(mapMarkerP_)
{
}

OsmAnd::MapMarker::SymbolsGroup::~SymbolsGroup()
{
    if (const auto mapMarkerP = _mapMarkerP.lock())
        mapMarkerP->unregisterSymbolsGroup(this);
}

const OsmAnd::MapMarker* OsmAnd::MapMarker::SymbolsGroup::getMapMarker() const
{
    if (const auto mapMarkerP = _mapMarkerP.lock())
        return mapMarkerP->owner;
    return nullptr;
}

bool OsmAnd::MapMarker::SymbolsGroup::updatesPresent()
{
    if (const auto mapMarkerP = _mapMarkerP.lock())
        return mapMarkerP->hasUnappliedChanges();

    return false;
}

OsmAnd::IUpdatableMapSymbolsGroup::UpdateResult OsmAnd::MapMarker::SymbolsGroup::update(const MapState& mapState)
{
    if (const auto mapMarkerP = _mapMarkerP.lock())
        return mapMarkerP->applyChanges() ? UpdateResult::Properties : UpdateResult::None;

    return UpdateResult::None;
}

bool OsmAnd::MapMarker::SymbolsGroup::supportsResourcesRenew()
{
    return false;
}
