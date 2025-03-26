#include "MapStyleEvaluator.h"
#include "MapStyleEvaluator_P.h"

#include <cassert>

#include "Logging.h"

OsmAnd::MapStyleEvaluator::MapStyleEvaluator(
    const std::shared_ptr<const IMapStyle>& mapStyle_,
    const float ptScaleFactor_)
    : _p(new MapStyleEvaluator_P(this))
    , mapStyle(mapStyle_)
    , ptScaleFactor(ptScaleFactor_)
{
    if (mapStyle_)
        _p->prepare();
}

OsmAnd::MapStyleEvaluator::~MapStyleEvaluator()
{
}

void OsmAnd::MapStyleEvaluator::setBooleanValue(const IMapStyle::ValueDefinitionId valueDefId, const bool value)
{
    _p->setBooleanValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const IMapStyle::ValueDefinitionId valueDefId, const int value)
{
    _p->setIntegerValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const IMapStyle::ValueDefinitionId valueDefId, const unsigned int value)
{
    _p->setIntegerValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setFloatValue(const IMapStyle::ValueDefinitionId valueDefId, const float value)
{
    _p->setFloatValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setStringValue(const IMapStyle::ValueDefinitionId valueDefId, const QString& value)
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
    const std::shared_ptr<const IMapStyle::IAttribute>& attribute,
    MapStyleEvaluationResult* const outResultStorage /*= nullptr*/) const
{
    return _p->evaluate(attribute, outResultStorage);
}
