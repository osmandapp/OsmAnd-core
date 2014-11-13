#include "MapStyleEvaluator.h"
#include "MapStyleEvaluator_P.h"

#include <cassert>

#include "UnresolvedMapStyle.h"
#include "UnresolvedMapStyle_P.h"
#include "Logging.h"

OsmAnd::MapStyleEvaluator::MapStyleEvaluator(
    const std::shared_ptr<const ResolvedMapStyle>& resolvedStyle_,
    const float displayDensityFactor_)
    : _p(new MapStyleEvaluator_P(this))
    , resolvedStyle(resolvedStyle_)
    , displayDensityFactor(displayDensityFactor_)
{
}

OsmAnd::MapStyleEvaluator::~MapStyleEvaluator()
{
}

void OsmAnd::MapStyleEvaluator::setBooleanValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const bool value)
{
    _p->setBooleanValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const int value)
{
    _p->setIntegerValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const unsigned int value)
{
    _p->setIntegerValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setFloatValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const float value)
{
    _p->setFloatValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setStringValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const QString& value)
{
    _p->setStringValue(valueDefId, value);
}

bool OsmAnd::MapStyleEvaluator::evaluate(
    const std::shared_ptr<const MapObject>& mapObject,
    const MapStyleRulesetType rulesetType,
    MapStyleEvaluationResult* const outResultStorage /*= nullptr*/) const
{
    return _p->evaluate(mapObject, rulesetType, outResultStorage);
}

bool OsmAnd::MapStyleEvaluator::evaluate(
    const std::shared_ptr<const ResolvedMapStyle::Attribute>& attribute,
    MapStyleEvaluationResult* const outResultStorage /*= nullptr*/) const
{
    return _p->evaluate(attribute, outResultStorage);
}
