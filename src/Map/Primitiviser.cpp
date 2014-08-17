#include "Primitiviser.h"
#include "Primitiviser_P.h"

#include "MapPresentationEnvironment.h"
#include "BinaryMapObject.h"

OsmAnd::Primitiviser::Primitiviser(const std::shared_ptr<const MapPresentationEnvironment>& environment_)
    : _p(new Primitiviser_P(this))
    , environment(environment_)
{
}

OsmAnd::Primitiviser::~Primitiviser()
{
}

std::shared_ptr<const OsmAnd::Primitiviser::PrimitivisedArea> OsmAnd::Primitiviser::primitivise(
    const AreaI area31,
    const PointI sizeInPixels,
    const ZoomLevel zoom,
    const MapFoundationType foundation,
    const QList< std::shared_ptr<const Model::BinaryMapObject> >& objects,
    const std::shared_ptr<Cache>& cache /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    Primitiviser_Metrics::Metric_primitivise* const metric /*= nullptr*/)
{
    return _p->primitivise(area31, sizeInPixels, zoom, foundation, objects, cache, controller, metric);
}

OsmAnd::Primitiviser::PrimitivesGroup::PrimitivesGroup(const std::shared_ptr<const Model::BinaryMapObject>& sourceObject_)
    : sourceObject(sourceObject_)
{
}

OsmAnd::Primitiviser::PrimitivesGroup::~PrimitivesGroup()
{
}

OsmAnd::Primitiviser::Primitive::Primitive(
    const std::shared_ptr<const PrimitivesGroup>& group_,
    const PrimitiveType type_,
    const uint32_t typeRuleIdIndex_)
    : group(group_)
    , sourceObject(group_->sourceObject)
    , type(type_)
    , typeRuleIdIndex(typeRuleIdIndex_)
    , zOrder(0.0)
{
}

OsmAnd::Primitiviser::Primitive::Primitive(
    const std::shared_ptr<const PrimitivesGroup>& group_,
    const PrimitiveType type_,
    const uint32_t typeRuleIdIndex_,
    const MapStyleEvaluationResult& evaluationResult_)
    : group(group_)
    , sourceObject(group_->sourceObject)
    , type(type_)
    , typeRuleIdIndex(typeRuleIdIndex_)
    , evaluationResult(evaluationResult_)
    , zOrder(0.0)
{
}

#ifdef Q_COMPILER_RVALUE_REFS
OsmAnd::Primitiviser::Primitive::Primitive(
    const std::shared_ptr<const PrimitivesGroup>& group_,
    const PrimitiveType type_,
    const uint32_t typeRuleIdIndex_,
    MapStyleEvaluationResult&& evaluationResult_)
    : group(group_)
    , sourceObject(group_->sourceObject)
    , type(type_)
    , typeRuleIdIndex(typeRuleIdIndex_)
    , evaluationResult(qMove(evaluationResult_))
    , zOrder(0.0)
{
}
#endif // Q_COMPILER_RVALUE_REFS

OsmAnd::Primitiviser::Primitive::~Primitive()
{
}

OsmAnd::Primitiviser::SymbolsGroup::SymbolsGroup(const std::shared_ptr<const Model::BinaryMapObject>& sourceObject_)
    : sourceObject(sourceObject_)
{
}

OsmAnd::Primitiviser::SymbolsGroup::~SymbolsGroup()
{
}

OsmAnd::Primitiviser::Symbol::Symbol(const std::shared_ptr<const Primitive>& primitive_)
    : primitive(primitive_)
    , order(-1)
    , drawAlongPath(false)
{
}

OsmAnd::Primitiviser::Symbol::~Symbol()
{
}

OsmAnd::Primitiviser::TextSymbol::TextSymbol(const std::shared_ptr<const Primitive>& primitive_)
    : Symbol(primitive_)
{
}

OsmAnd::Primitiviser::TextSymbol::~TextSymbol()
{
}

OsmAnd::Primitiviser::IconSymbol::IconSymbol(const std::shared_ptr<const Primitive>& primitive_)
    : Symbol(primitive_)
{
}

OsmAnd::Primitiviser::IconSymbol::~IconSymbol()
{
}

OsmAnd::Primitiviser::Cache::Cache()
{
}

OsmAnd::Primitiviser::Cache::~Cache()
{
}

OsmAnd::Primitiviser::Cache::SharedPrimitivesGroupsContainer& OsmAnd::Primitiviser::Cache::getPrimitivesGroups(const ZoomLevel zoom)
{
    return _sharedPrimitivesGroups[zoom];
}

const OsmAnd::Primitiviser::Cache::SharedPrimitivesGroupsContainer& OsmAnd::Primitiviser::Cache::getPrimitivesGroups(const ZoomLevel zoom) const
{
    return _sharedPrimitivesGroups[zoom];
}

OsmAnd::Primitiviser::Cache::SharedSymbolsGroupsContainer& OsmAnd::Primitiviser::Cache::getSymbolsGroups(const ZoomLevel zoom)
{
    return _sharedSymbolsGroups[zoom];
}

const OsmAnd::Primitiviser::Cache::SharedSymbolsGroupsContainer& OsmAnd::Primitiviser::Cache::getSymbolsGroups(const ZoomLevel zoom) const
{
    return _sharedSymbolsGroups[zoom];
}

OsmAnd::Primitiviser::Cache::SharedPrimitivesGroupsContainer* OsmAnd::Primitiviser::Cache::getPrimitivesGroupsPtr(const ZoomLevel zoom)
{
    return &getPrimitivesGroups(zoom);
}

const OsmAnd::Primitiviser::Cache::SharedPrimitivesGroupsContainer* OsmAnd::Primitiviser::Cache::getPrimitivesGroupsPtr(const ZoomLevel zoom) const
{
    return &getPrimitivesGroups(zoom);
}

OsmAnd::Primitiviser::Cache::SharedSymbolsGroupsContainer* OsmAnd::Primitiviser::Cache::getSymbolsGroupsPtr(const ZoomLevel zoom)
{
    return &getSymbolsGroups(zoom);
}

const OsmAnd::Primitiviser::Cache::SharedSymbolsGroupsContainer* OsmAnd::Primitiviser::Cache::getSymbolsGroupsPtr(const ZoomLevel zoom) const
{
    return &getSymbolsGroups(zoom);
}

OsmAnd::Primitiviser::PrimitivisedArea::PrimitivisedArea(
    const AreaI area31_,
    const PointI sizeInPixels_,
    const ZoomLevel zoom_,
    const std::shared_ptr<Cache>& cache_,
    const std::shared_ptr<const MapPresentationEnvironment>& mapPresentationEnvironment_)
    : _cache(cache_)
    , area31(area31_)
    , sizeInPixels(sizeInPixels_)
    , zoom(zoom_)
    , mapPresentationEnvironment(mapPresentationEnvironment_)
{
}

OsmAnd::Primitiviser::PrimitivisedArea::~PrimitivisedArea()
{
    const auto cache = _cache.lock();

    // If context has binded shared context, remove all primitives groups
    // that are owned only current context
    if (cache)
    {
        auto& sharedGroups = cache->getPrimitivesGroups(zoom);
        for (auto& group : primitivesGroups)
        {
            const auto canBeShared = (group->sourceObject->section != mapPresentationEnvironment->dummyMapSection);
            if (!canBeShared)
                continue;

            // Remove reference to this group from shared ones
            sharedGroups.releaseReference(group->sourceObject->id, group);
        }
    }

    // If context has binded shared context, remove all symbols groups
    // that are owned only current context
    if (cache)
    {
        auto& sharedGroups = cache->getSymbolsGroups(zoom);
        for (auto& group : symbolsGroups)
        {
            const auto canBeShared = (group->sourceObject->section != mapPresentationEnvironment->dummyMapSection);
            if (!canBeShared)
                continue;

            // Remove reference to this group from shared ones
            sharedGroups.releaseReference(group->sourceObject->id, group);
        }
    }
}

bool OsmAnd::Primitiviser::PrimitivisedArea::isEmpty() const
{
    return primitivesGroups.isEmpty() && symbolsGroups.isEmpty();
}
