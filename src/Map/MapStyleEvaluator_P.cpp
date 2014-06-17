#include "MapStyleEvaluator_P.h"
#include "MapStyleEvaluator.h"

#include <cassert>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleEvaluationResult_P.h"
#include "MapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleRule_P.h"
#include "BinaryMapObject.h"
#include "QKeyValueIterator.h"

OsmAnd::MapStyleEvaluator_P::MapStyleEvaluator_P( MapStyleEvaluator* owner_ )
    : owner(owner_)
    , _builtinValueDefs(MapStyle::getBuiltinValueDefinitions())
{
}

OsmAnd::MapStyleEvaluator_P::~MapStyleEvaluator_P()
{
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject, const MapStyleRulesetType ruleset,
    MapStyleEvaluationResult* const outResultStorage,
    bool evaluateChildren)
{
    const auto tagKey = _inputValues[_builtinValueDefs->id_INPUT_TAG].asUInt;
    const auto valueKey = _inputValues[_builtinValueDefs->id_INPUT_VALUE].asUInt;
    const auto& rules = owner->style->_p->obtainRulesRef(ruleset);

    auto evaluationResult = evaluate(mapObject, rules, tagKey, valueKey, outResultStorage, evaluateChildren);
    if (evaluationResult)
        return true;

    evaluationResult = evaluate(mapObject, rules, tagKey, 0, outResultStorage, evaluateChildren);
    if (evaluationResult)
        return true;

    evaluationResult = evaluate(mapObject, rules, 0, 0, outResultStorage, evaluateChildren);
    if (evaluationResult)
        return true;

    return false;
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const std::shared_ptr<const MapStyleRule>& singleRule,
    MapStyleEvaluationResult* const outResultStorage,
    bool evaluateChildren)
{
    return evaluate(nullptr, singleRule, outResultStorage, evaluateChildren);
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
    const QMap< uint64_t, std::shared_ptr<MapStyleRule> >& rules,
    const uint32_t tagKey, const uint32_t valueKey,
    MapStyleEvaluationResult* const outResultStorage,
    bool evaluateChildren)
{
    _inputValues[_builtinValueDefs->id_INPUT_TAG].asUInt = tagKey;
    _inputValues[_builtinValueDefs->id_INPUT_VALUE].asUInt = valueKey;

    const auto ruleId = MapStyle_P::encodeRuleId(tagKey, valueKey);
    auto itRule = rules.constFind(ruleId);
    if (itRule == rules.cend())
        return false;

    return evaluate(mapObject.get(), *itRule, outResultStorage, evaluateChildren);
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const Model::BinaryMapObject* const mapObject, const std::shared_ptr<const MapStyleRule>& rule,
    MapStyleEvaluationResult* const outResultStorage,
    bool evaluateChildren)
{
    // Check all values of a rule until all are checked.
    for(const auto& ruleValueEntry : rangeOf(constOf(rule->_p->_values)))
    {
        const auto& valueDef = ruleValueEntry.key();

        // Test only input values, the ones that start with INPUT_*
        if (valueDef->valueClass != MapStyleValueClass::Input)
            continue;

        const auto& ruleValue = ruleValueEntry.value();
        const auto& inputValue = _inputValues[valueDef->id];

        bool evaluationResult = false;
        if (valueDef->id == _builtinValueDefs->id_INPUT_MINZOOM)
        {
            assert(!ruleValue.isComplex);
            evaluationResult = (ruleValue.asSimple.asInt <= inputValue.asInt);
        }
        else if (valueDef->id == _builtinValueDefs->id_INPUT_MAXZOOM)
        {
            assert(!ruleValue.isComplex);
            evaluationResult = (ruleValue.asSimple.asInt >= inputValue.asInt);
        }
        else if (valueDef->id == _builtinValueDefs->id_INPUT_ADDITIONAL)
        {
            if (!mapObject)
                evaluationResult = true;
            else
            {
                assert(!ruleValue.isComplex);
                const auto& strValue = owner->style->_p->lookupStringValue(ruleValue.asSimple.asUInt);
                auto equalSignIdx = strValue.indexOf('=');
                if (equalSignIdx >= 0)
                {
                    const auto& tag = strValue.mid(0, equalSignIdx);
                    const auto& value = strValue.mid(equalSignIdx + 1);
                    evaluationResult = mapObject->containsTypeSlow(tag, value, true);
                }
                else
                    evaluationResult = false;
            }
        }
        else if (valueDef->id == _builtinValueDefs->id_INPUT_TEST)
        {
            evaluationResult = (inputValue.asInt == 1);
        }
        else if (valueDef->dataType == MapStyleValueDataType::Float)
        {
            const auto lvalue = ruleValue.isComplex ? ruleValue.asComplex.asFloat.evaluate(owner->displayDensityFactor) : ruleValue.asSimple.asFloat;

            evaluationResult = qFuzzyCompare(lvalue, inputValue.asFloat);
        }
        else
        {
            const auto lvalue = ruleValue.isComplex ? ruleValue.asComplex.asInt.evaluate(owner->displayDensityFactor) : ruleValue.asSimple.asInt;

            evaluationResult = (lvalue == inputValue.asInt);
        }

        // If at least one value of rule does not match, it's failure
        if (!evaluationResult)
            return false;
    }

    // Fill output values from rule to result storage, if requested
    if (outResultStorage)
    {
        for(const auto& ruleValueEntry : rangeOf(constOf(rule->_p->_values)))
        {
            const auto& valueDef = ruleValueEntry.key();
            if (valueDef->valueClass != MapStyleValueClass::Output)
                continue;

            const auto& ruleValue = ruleValueEntry.value();

            switch(valueDef->dataType)
            {
            case MapStyleValueDataType::Boolean:
                assert(!ruleValue.isComplex);
                outResultStorage->_p->_values[valueDef->id] = (ruleValue.asSimple.asUInt == 1);
                break;
            case MapStyleValueDataType::Integer:
                outResultStorage->_p->_values[valueDef->id] =
                    ruleValue.isComplex
                    ? ruleValue.asComplex.asInt.evaluate(owner->displayDensityFactor)
                    : ruleValue.asSimple.asInt;
                break;
            case MapStyleValueDataType::Float:
                outResultStorage->_p->_values[valueDef->id] =
                    ruleValue.isComplex
                    ? ruleValue.asComplex.asFloat.evaluate(owner->displayDensityFactor)
                    : ruleValue.asSimple.asFloat;
                break;
            case MapStyleValueDataType::String:
                // Save value of a string instead of it's id
                outResultStorage->_p->_values[valueDef->id] =
                    owner->style->_p->lookupStringValue(ruleValue.asSimple.asUInt);
                break;
            case MapStyleValueDataType::Color:
                assert(!ruleValue.isComplex);
                outResultStorage->_p->_values[valueDef->id] = ruleValue.asSimple.asUInt;
                break;
            }
        }
    }

    if (evaluateChildren)
    {
        for(const auto& child : constOf(rule->_p->_ifElseChildren))
        {
            const auto evaluationResult = evaluate(mapObject, child, outResultStorage, true);
            if (evaluationResult)
                break;
        }

        for(const auto& child : constOf(rule->_p->_ifChildren))
            evaluate(mapObject, child, outResultStorage, true);
    }

    return true;
}
