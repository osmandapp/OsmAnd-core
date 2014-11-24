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
    const IQueryController* const controller /*= nullptr*/,
    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric /*= nullptr*/)
{
    return _p->primitiviseAllMapObjects(zoom, objects, cache, controller, metric);
}

std::shared_ptr<OsmAnd::MapPrimitiviser::PrimitivisedObjects> OsmAnd::MapPrimitiviser::primitiviseAllMapObjects(
    const PointD scaleDivisor31ToPixel,
    const ZoomLevel zoom,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric /*= nullptr*/)
{
    return _p->primitiviseAllMapObjects(scaleDivisor31ToPixel, zoom, objects, cache, controller, metric);
}

std::shared_ptr<OsmAnd::MapPrimitiviser::PrimitivisedObjects> OsmAnd::MapPrimitiviser::primitiviseWithSurface(
    const AreaI area31,
    const PointI areaSizeInPixels,
    const ZoomLevel zoom,
    const MapSurfaceType surfaceType,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    MapPrimitiviser_Metrics::Metric_primitiviseWithSurface* const metric /*= nullptr*/)
{
    return _p->primitiviseWithSurface(area31, areaSizeInPixels, zoom, surfaceType, objects, cache, controller, metric);
}

std::shared_ptr<OsmAnd::MapPrimitiviser::PrimitivisedObjects> OsmAnd::MapPrimitiviser::primitiviseWithoutSurface(
    const PointD scaleDivisor31ToPixel,
    const ZoomLevel zoom,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    MapPrimitiviser_Metrics::Metric_primitiviseWithoutSurface* const metric /*= nullptr*/)
{
    return _p->primitiviseWithoutSurface(scaleDivisor31ToPixel, zoom, objects, cache, controller, metric);
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
    , typeRuleIdIndex(typeRuleIdIndex_)
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
    , typeRuleIdIndex(typeRuleIdIndex_)
    , evaluationResult(evaluationResult_)
    , zOrder(0)
    , doubledArea(-1)
{
}

#ifdef Q_COMPILER_RVALUE_REFS
OsmAnd::MapPrimitiviser::Primitive::Primitive(
    const std::shared_ptr<const PrimitivesGroup>& group_,
    const PrimitiveType type_,
    const uint32_t typeRuleIdIndex_,
    MapStyleEvaluationResult&& evaluationResult_)
    : group(group_)
    , sourceObject(group_->sourceObject)
    , type(type_)
    , typeRuleIdIndex(typeRuleIdIndex_)
    , evaluationResult(qMove(evaluationResult_))
    , zOrder(0)
    , doubledArea(-1)
{
}
#endif // Q_COMPILER_RVALUE_REFS

OsmAnd::MapPrimitiviser::Primitive::~Primitive()
{
}

OsmAnd::MapPrimitiviser::SymbolsGroup::SymbolsGroup(const std::shared_ptr<const MapObject>& sourceObject_)
    : sourceObject(sourceObject_)
{
}

OsmAnd::MapPrimitiviser::SymbolsGroup::~SymbolsGroup()
{
}

OsmAnd::MapPrimitiviser::Symbol::Symbol(const std::shared_ptr<const Primitive>& primitive_)
    : primitive(primitive_)
    , order(-1)
    , drawAlongPath(false)
    , intersectionSizeFactor(std::numeric_limits<float>::quiet_NaN())
    , intersectionSize(std::numeric_limits<float>::quiet_NaN())
    , intersectionMargin(std::numeric_limits<float>::quiet_NaN())
    , pathPaddingLeft(0.0f)
    , pathPaddingRight(0.0f)
    , minDistance(-1.0f)
{
}

OsmAnd::MapPrimitiviser::Symbol::~Symbol()
{
}

OsmAnd::MapPrimitiviser::TextSymbol::TextSymbol(const std::shared_ptr<const Primitive>& primitive_)
    : Symbol(primitive_)
    , drawOnPath(false)
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

OsmAnd::MapPrimitiviser::IconSymbol::IconSymbol(const std::shared_ptr<const Primitive>& primitive_)
    : Symbol(primitive_)
{
}

OsmAnd::MapPrimitiviser::IconSymbol::~IconSymbol()
{
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
