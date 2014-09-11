#include "MapStyleEvaluator.h"
#include "MapStyleEvaluator_P.h"

#include <cassert>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "Logging.h"

OsmAnd::MapStyleEvaluator::MapStyleEvaluator(const std::shared_ptr<const MapStyle>& style_, const float displayDensityFactor_)
    : _p(new MapStyleEvaluator_P(this))
    , style(style_)
    , displayDensityFactor(displayDensityFactor_)
{
}

OsmAnd::MapStyleEvaluator::~MapStyleEvaluator()
{
}

void OsmAnd::MapStyleEvaluator::setBooleanValue(const int valueDefId, const bool value)
{
    _p->setBooleanValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const int valueDefId, const int value)
{
    _p->setIntegerValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const int valueDefId, const unsigned int value)
{
    _p->setIntegerValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setFloatValue(const int valueDefId, const float value)
{
    _p->setFloatValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setStringValue(const int valueDefId, const QString& value)
{
    _p->setStringValue(valueDefId, value);
}

bool OsmAnd::MapStyleEvaluator::evaluate(
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
    const MapStyleRulesetType ruleset,
    MapStyleEvaluationResult* const outResultStorage /*= nullptr*/,
    bool evaluateChildren /*= true*/,
    std::shared_ptr<const MapStyleNode>* outMatchedRule /*= nullptr*/) const
{
    return _p->evaluate(mapObject, ruleset, outResultStorage, evaluateChildren, outMatchedRule);
}

bool OsmAnd::MapStyleEvaluator::evaluate(
    const std::shared_ptr<const MapStyleNode>& singleRule,
    MapStyleEvaluationResult* const outResultStorage /*= nullptr*/,
    bool evaluateChildren /*= true*/) const
{
    return _p->evaluate(singleRule, outResultStorage, evaluateChildren);
}
