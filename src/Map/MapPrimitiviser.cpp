#include "MapPrimitiviser.h"
#include "MapPrimitiviser_P.h"

#include "MapPresentationEnvironment.h"
#include "MapObject.h"

OsmAnd::MapPrimitiviser::MapPrimitiviser(const std::shared_ptr<const MapPresentationEnvironment>& environment_)
    : _p(new MapPrimitiviser_P(this))
    , environment(environment_)
{
}

OsmAnd::MapPrimitiviser::~MapPrimitiviser()
{
}

std::shared_ptr<OsmAnd::MapPrimitiviser::PrimitivisedObjects> OsmAnd::MapPrimitiviser::primitiviseAllMapObjects(
    const ZoomLevel zoom,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/,
    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric /*= nullptr*/)
{
    return _p->primitiviseAllMapObjects(zoom, objects, cache, queryController, metric);
}

std::shared_ptr<OsmAnd::MapPrimitiviser::PrimitivisedObjects> OsmAnd::MapPrimitiviser::primitiviseAllMapObjects(
    const ZoomLevel zoom,
    const TileId tileId,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/,
    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric /*= nullptr*/)
{
    return _p->primitiviseAllMapObjects(zoom, tileId, objects, cache, queryController, metric);
}

std::shared_ptr<OsmAnd::MapPrimitiviser::PrimitivisedObjects> OsmAnd::MapPrimitiviser::primitiviseAllMapObjects(
    const PointD scaleDivisor31ToPixel,
    const ZoomLevel zoom,
    const TileId tileId,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/,
    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric /*= nullptr*/)
{
    return _p->primitiviseAllMapObjects(scaleDivisor31ToPixel, zoom, tileId, objects, cache, queryController, metric);
}

std::shared_ptr<OsmAnd::MapPrimitiviser::PrimitivisedObjects> OsmAnd::MapPrimitiviser::primitiviseWithSurface(
    const AreaI area31,
    const PointI areaSizeInPixels,
    const ZoomLevel zoom,
    const TileId tileId,
    const AreaI visibleArea31,
    const int64_t visibleAreaTime,
    const MapSurfaceType surfaceType,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/,
    MapPrimitiviser_Metrics::Metric_primitiviseWithSurface* const metric /*= nullptr*/)
{
    return _p->primitiviseWithSurface(area31, areaSizeInPixels, zoom, tileId, visibleArea31, visibleAreaTime,
        surfaceType, objects, cache, queryController, metric);
}

std::shared_ptr<OsmAnd::MapPrimitiviser::PrimitivisedObjects> OsmAnd::MapPrimitiviser::primitiviseWithoutSurface(
    const PointD scaleDivisor31ToPixel,
    const ZoomLevel zoom,
    const TileId tileId,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/,
    MapPrimitiviser_Metrics::Metric_primitiviseWithoutSurface* const metric /*= nullptr*/)
{
    return _p->primitiviseWithoutSurface(scaleDivisor31ToPixel, zoom, tileId, objects, cache, queryController, metric);
}

OsmAnd::MapPrimitiviser::CoastlineMapObject::CoastlineMapObject()
{
}

OsmAnd::MapPrimitiviser::CoastlineMapObject::~CoastlineMapObject()
{
}

OsmAnd::MapPrimitiviser::SurfaceMapObject::SurfaceMapObject()
{
}

OsmAnd::MapPrimitiviser::SurfaceMapObject::~SurfaceMapObject()
{
}

OsmAnd::MapPrimitiviser::PrimitivesGroup::PrimitivesGroup(const std::shared_ptr<const MapObject>& sourceObject_)
    : sourceObject(sourceObject_)
{
}

OsmAnd::MapPrimitiviser::PrimitivesGroup::~PrimitivesGroup()
{
}

OsmAnd::MapPrimitiviser::Primitive::Primitive(
    const std::shared_ptr<const PrimitivesGroup>& group_,
    const PrimitiveType type_,
    const uint32_t typeRuleIdIndex_)
    : group(group_)
    , sourceObject(group_->sourceObject)
    , type(type_)
    , attributeIdIndex(typeRuleIdIndex_)
    , zOrder(0)
    , doubledArea(-1)
{
}

OsmAnd::MapPrimitiviser::Primitive::Primitive(
    const std::shared_ptr<const PrimitivesGroup>& group_,
    const PrimitiveType type_,
    const uint32_t typeRuleIdIndex_,
    const MapStyleEvaluationResult& evaluationResult_)
    : group(group_)
    , sourceObject(group_->sourceObject)
    , type(type_)
    , attributeIdIndex(typeRuleIdIndex_)
    , evaluationResult(evaluationResult_.pack())
    , zOrder(0)
    , doubledArea(-1)
{
}

OsmAnd::MapPrimitiviser::Primitive::~Primitive()
{
}

OsmAnd::MapPrimitiviser::SymbolsGroup::SymbolsGroup(
    const std::shared_ptr<const MapObject>& sourceObject_, bool canBeShownWithoutIcon_ /* = false */)
    : sourceObject(sourceObject_)
    , canBeShownWithoutIcon(canBeShownWithoutIcon_)
{
}

OsmAnd::MapPrimitiviser::SymbolsGroup::~SymbolsGroup()
{
}

OsmAnd::MapPrimitiviser::GridSymbolsGroup::GridSymbolsGroup(
    const std::shared_ptr<const SymbolsGroup>& symbolsGroup_,
    const bool canBeShared_,
    const MapObject::SharingKey sharingKey_)
    : symbolsGroup(symbolsGroup_)
    , canBeShared(canBeShared_)
    , sharingKey(sharingKey_)
{
}

OsmAnd::MapPrimitiviser::GridSymbolsGroup::~GridSymbolsGroup()
{
}

OsmAnd::MapPrimitiviser::Symbol::Symbol(const std::shared_ptr<const Primitive>& primitive_)
    : primitive(primitive_)
    , order(-1)
    , drawAlongPath(false)
    , intersectionSizeFactor(std::numeric_limits<float>::quiet_NaN())
    , intersectionSize(std::numeric_limits<float>::quiet_NaN())
    , intersectionMargin(std::numeric_limits<float>::quiet_NaN())
    , minDistance(-1.0f)
    , scaleFactor(1.0f)
    , ignoreClick(false)
{
}

OsmAnd::MapPrimitiviser::Symbol::~Symbol()
{
}

bool OsmAnd::MapPrimitiviser::Symbol::operator==(const Symbol& that) const
{
    return
        this->primitive == that.primitive &&
        this->location31 == that.location31 &&
        this->order == that.order &&
        this->drawAlongPath == that.drawAlongPath &&
        this->intersectsWith == that.intersectsWith &&
        qFuzzyCompare(this->intersectionSizeFactor, that.intersectionSizeFactor) &&
        qFuzzyCompare(this->intersectionSize, that.intersectionSize) &&
        qFuzzyCompare(this->intersectionMargin, that.intersectionMargin) &&
        qFuzzyCompare(this->minDistance, that.minDistance) &&
        qFuzzyCompare(this->scaleFactor, that.scaleFactor);
}

bool OsmAnd::MapPrimitiviser::Symbol::operator!=(const Symbol& that) const
{
    return
        this->primitive != that.primitive ||
        this->location31 != that.location31 ||
        this->order != that.order ||
        this->drawAlongPath != that.drawAlongPath ||
        this->intersectsWith != that.intersectsWith ||
        !qFuzzyCompare(this->intersectionSizeFactor, that.intersectionSizeFactor) ||
        !qFuzzyCompare(this->intersectionSize, that.intersectionSize) ||
        !qFuzzyCompare(this->intersectionMargin, that.intersectionMargin) ||
        !qFuzzyCompare(this->minDistance, that.minDistance) ||
        !qFuzzyCompare(this->scaleFactor, that.scaleFactor);
}

bool OsmAnd::MapPrimitiviser::Symbol::hasSameContentAs(const Symbol& that) const
{
    return
        this->location31 == that.location31 &&
        this->order == that.order &&
        this->drawAlongPath == that.drawAlongPath;
}

bool OsmAnd::MapPrimitiviser::Symbol::hasDifferentContentAs(const Symbol& that) const
{
    return
        this->location31 != that.location31 ||
        this->order != that.order ||
        this->drawAlongPath != that.drawAlongPath;
}

OsmAnd::MapPrimitiviser::TextSymbol::TextSymbol(const std::shared_ptr<const Primitive>& primitive_)
    : Symbol(primitive_)
    , drawOnPath(false)
    , placement(Placement::Default)
    , verticalOffset(0)
    , size(0)
    , shadowRadius(0)
    , wrapWidth(0)
    , isBold(false)
    , isItalic(false)
{
}

OsmAnd::MapPrimitiviser::TextSymbol::~TextSymbol()
{
}

bool OsmAnd::MapPrimitiviser::TextSymbol::operator==(const TextSymbol& that) const
{
    return
        Symbol::operator==(that) &&
        this->value == that.value &&
        this->languageId == that.languageId &&
        this->drawOnPath == that.drawOnPath &&
        this->placement == that.placement &&
        this->additionalPlacements == that.additionalPlacements &&
        this->verticalOffset == that.verticalOffset &&
        this->color == that.color &&
        this->size == that.size &&
        this->shadowRadius == that.shadowRadius &&
        this->shadowColor == that.shadowColor &&
        this->wrapWidth == that.wrapWidth &&
        this->isBold == that.isBold &&
        this->isItalic == that.isItalic &&
        this->shieldResourceName == that.shieldResourceName &&
        this->underlayIconResourceNames == that.underlayIconResourceNames;
}

bool OsmAnd::MapPrimitiviser::TextSymbol::operator!=(const TextSymbol& that) const
{
    return
        Symbol::operator!=(that) ||
        this->value != that.value ||
        this->languageId != that.languageId ||
        this->drawOnPath != that.drawOnPath ||
        this->placement != that.placement ||
        this->additionalPlacements != that.additionalPlacements ||
        this->verticalOffset != that.verticalOffset ||
        this->color != that.color ||
        this->size != that.size ||
        this->shadowRadius != that.shadowRadius ||
        this->shadowColor != that.shadowColor ||
        this->wrapWidth != that.wrapWidth ||
        this->isBold != that.isBold ||
        this->isItalic != that.isItalic ||
        this->shieldResourceName != that.shieldResourceName ||
        this->underlayIconResourceNames != that.underlayIconResourceNames;
}

bool OsmAnd::MapPrimitiviser::TextSymbol::hasSameContentAs(const TextSymbol& that) const
{
    return
        Symbol::hasSameContentAs(that) &&
        this->value == that.value &&
        this->drawOnPath == that.drawOnPath &&
        this->placement == that.placement &&
        this->additionalPlacements == that.additionalPlacements &&
        this->verticalOffset == that.verticalOffset &&
        this->color == that.color &&
        this->size == that.size &&
        this->shadowRadius == that.shadowRadius &&
        this->shadowColor == that.shadowColor &&
        this->wrapWidth == that.wrapWidth &&
        this->isBold == that.isBold &&
        this->isItalic == that.isItalic &&
        this->shieldResourceName == that.shieldResourceName &&
        this->underlayIconResourceNames == that.underlayIconResourceNames;
}

bool OsmAnd::MapPrimitiviser::TextSymbol::hasDifferentContentAs(const TextSymbol& that) const
{
    return
        Symbol::hasDifferentContentAs(that) ||
        this->value != that.value ||
        this->drawOnPath != that.drawOnPath ||
        this->placement != that.placement ||
        this->additionalPlacements != that.additionalPlacements ||
        this->verticalOffset != that.verticalOffset ||
        this->color != that.color ||
        this->size != that.size ||
        this->shadowRadius != that.shadowRadius ||
        this->shadowColor != that.shadowColor ||
        this->wrapWidth != that.wrapWidth ||
        this->isBold != that.isBold ||
        this->isItalic != that.isItalic ||
        this->shieldResourceName != that.shieldResourceName ||
        this->underlayIconResourceNames != that.underlayIconResourceNames;
}

OsmAnd::MapPrimitiviser::TextSymbol::Placement OsmAnd::MapPrimitiviser::TextSymbol::placementFromString(const QString& placement)
{
    if (placement == QLatin1String("top"))
        return Placement::Top;
    else if (placement == QLatin1String("additionalBottom"))
        return Placement::BottomAdditional;
    else
        return Placement::Default;
}

OsmAnd::MapPrimitiviser::IconSymbol::IconSymbol(const std::shared_ptr<const Primitive>& primitive_)
    : Symbol(primitive_)
{
}

OsmAnd::MapPrimitiviser::IconSymbol::~IconSymbol()
{
}

bool OsmAnd::MapPrimitiviser::IconSymbol::operator==(const IconSymbol& that) const
{
    return
        Symbol::operator==(that) &&
        this->resourceName == that.resourceName &&
        this->underlayResourceNames == that.underlayResourceNames &&
        this->overlayResourceNames == that.overlayResourceNames &&
        this->shieldResourceName == that.shieldResourceName;
}

bool OsmAnd::MapPrimitiviser::IconSymbol::operator!=(const IconSymbol& that) const
{
    return
        Symbol::operator!=(that) ||
        this->resourceName != that.resourceName ||
        this->underlayResourceNames != that.underlayResourceNames ||
        this->overlayResourceNames != that.overlayResourceNames ||
        this->shieldResourceName != that.shieldResourceName;
}

bool OsmAnd::MapPrimitiviser::IconSymbol::hasSameContentAs(const IconSymbol& that) const
{
    return
        Symbol::hasSameContentAs(that) &&
        this->resourceName == that.resourceName &&
        this->underlayResourceNames == that.underlayResourceNames &&
        this->overlayResourceNames == that.overlayResourceNames &&
        this->shieldResourceName == that.shieldResourceName;
}

bool OsmAnd::MapPrimitiviser::IconSymbol::hasDifferentContentAs(const IconSymbol& that) const
{
    return
        Symbol::hasDifferentContentAs(that) ||
        this->resourceName != that.resourceName ||
        this->underlayResourceNames != that.underlayResourceNames ||
        this->overlayResourceNames != that.overlayResourceNames ||
        this->shieldResourceName != that.shieldResourceName;
}

OsmAnd::MapPrimitiviser::Cache::Cache()
{
}

OsmAnd::MapPrimitiviser::Cache::~Cache()
{
}

OsmAnd::MapPrimitiviser::Cache::SharedPrimitivesGroupsContainer& OsmAnd::MapPrimitiviser::Cache::getPrimitivesGroups(const ZoomLevel zoom)
{
    return _sharedPrimitivesGroups[zoom];
}

const OsmAnd::MapPrimitiviser::Cache::SharedPrimitivesGroupsContainer& OsmAnd::MapPrimitiviser::Cache::getPrimitivesGroups(const ZoomLevel zoom) const
{
    return _sharedPrimitivesGroups[zoom];
}

OsmAnd::MapPrimitiviser::Cache::SharedSymbolsGroupsContainer& OsmAnd::MapPrimitiviser::Cache::getSymbolsGroups(const ZoomLevel zoom)
{
    return _sharedSymbolsGroups[zoom];
}

const OsmAnd::MapPrimitiviser::Cache::SharedSymbolsGroupsContainer& OsmAnd::MapPrimitiviser::Cache::getSymbolsGroups(const ZoomLevel zoom) const
{
    return _sharedSymbolsGroups[zoom];
}

OsmAnd::MapPrimitiviser::Cache::SharedPrimitivesGroupsContainer* OsmAnd::MapPrimitiviser::Cache::getPrimitivesGroupsPtr(const ZoomLevel zoom)
{
    return &getPrimitivesGroups(zoom);
}

const OsmAnd::MapPrimitiviser::Cache::SharedPrimitivesGroupsContainer* OsmAnd::MapPrimitiviser::Cache::getPrimitivesGroupsPtr(const ZoomLevel zoom) const
{
    return &getPrimitivesGroups(zoom);
}

OsmAnd::MapPrimitiviser::Cache::SharedSymbolsGroupsContainer* OsmAnd::MapPrimitiviser::Cache::getSymbolsGroupsPtr(const ZoomLevel zoom)
{
    return &getSymbolsGroups(zoom);
}

const OsmAnd::MapPrimitiviser::Cache::SharedSymbolsGroupsContainer* OsmAnd::MapPrimitiviser::Cache::getSymbolsGroupsPtr(const ZoomLevel zoom) const
{
    return &getSymbolsGroups(zoom);
}

OsmAnd::MapPrimitiviser::PrimitivisedObjects::PrimitivisedObjects(
    const std::shared_ptr<const MapPresentationEnvironment>& mapPresentationEnvironment_,
    const std::shared_ptr<Cache>& cache_,
    const ZoomLevel zoom_,
    const PointD scaleDivisor31ToPixel_)
    : _cache(cache_)
    , mapPresentationEnvironment(mapPresentationEnvironment_)
    , zoom(zoom_)
    , scaleDivisor31ToPixel(scaleDivisor31ToPixel_)
{
}

OsmAnd::MapPrimitiviser::PrimitivisedObjects::~PrimitivisedObjects()
{
    const auto cache = _cache.lock();

    // If context has binded shared context, remove all primitives groups
    // that are owned only current context
    if (cache)
    {
        auto& sharedGroups = cache->getPrimitivesGroups(zoom);
        for (auto& group : primitivesGroups)
        {
            MapObject::SharingKey sharingKey;
            if (!group->sourceObject->obtainSharingKey(sharingKey))
                continue;

            // Remove reference to this group from shared ones
            sharedGroups.releaseReference(sharingKey, group);
        }
    }

    // If context has binded shared context, remove all symbols groups
    // that are owned only current context
    if (cache)
    {
        auto& sharedGroups = cache->getSymbolsGroups(zoom);
        for (auto& group : symbolsGroups)
        {
            MapObject::SharingKey sharingKey;
            if (!group->sourceObject->obtainSharingKey(sharingKey))
                continue;

            // Remove reference to this group from shared ones
            sharedGroups.releaseReference(sharingKey, group);
        }
    }
}

bool OsmAnd::MapPrimitiviser::PrimitivisedObjects::isEmpty() const
{
    return primitivesGroups.isEmpty() && symbolsGroups.isEmpty();
}
