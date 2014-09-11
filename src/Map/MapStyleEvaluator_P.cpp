#include "MapStyleEvaluator_P.h"
#include "MapStyleEvaluator.h"

#include "stdlib_common.h"
#include <cassert>

#include "QtExtensions.h"
#include "QtCommon.h"

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleRule_P.h"
#include "BinaryMapObject.h"
#include "QKeyValueIterator.h"
#include "Logging.h"

OsmAnd::MapStyleEvaluator_P::MapStyleEvaluator_P(MapStyleEvaluator* owner_)
    : owner(owner_)
    , _builtinValueDefs(MapStyle::getBuiltinValueDefinitions())
{
}

OsmAnd::MapStyleEvaluator_P::~MapStyleEvaluator_P()
{
}

void OsmAnd::MapStyleEvaluator_P::setBooleanValue(const int valueDefId, const bool value)
{
    auto& entry = _inputValues[valueDefId];

    entry.asInt = value ? 1 : 0;
}

void OsmAnd::MapStyleEvaluator_P::setIntegerValue(const int valueDefId, const int value)
{
    auto& entry = _inputValues[valueDefId];

    entry.asInt = value;
}

void OsmAnd::MapStyleEvaluator_P::setIntegerValue(const int valueDefId, const unsigned int value)
{
    auto& entry = _inputValues[valueDefId];

    entry.asUInt = value;
}

void OsmAnd::MapStyleEvaluator_P::setFloatValue(const int valueDefId, const float value)
{
    auto& entry = _inputValues[valueDefId];

    entry.asFloat = value;
}

void OsmAnd::MapStyleEvaluator_P::setStringValue(const int valueDefId, const QString& value)
{
    auto& entry = _inputValues[valueDefId];

    const auto ok = owner->style->_p->lookupStringId(value, entry.asUInt);
    if (!ok)
    {
        LogPrintf(LogSeverityLevel::Warning,
            "Map style input string '%s' was not resolved in lookup table",
            qPrintable(value));
        entry.asUInt = std::numeric_limits<uint32_t>::max();
    }
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
    const MapStyleRulesetType ruleset,
    MapStyleEvaluationResult* const outResultStorage,
    bool evaluateChildren,
    std::shared_ptr<const MapStyleNode>* outMatchedRule) const
{
    const auto& rules = owner->style->_p->obtainRulesRef(ruleset);

    const auto citTagKey = _inputValues.constFind(_builtinValueDefs->id_INPUT_TAG);
    const auto citValueKey = _inputValues.constFind(_builtinValueDefs->id_INPUT_VALUE);
    const auto citEnd = _inputValues.cend();

    if (citTagKey != citEnd && citValueKey != citEnd)
    {
        const auto evaluationResult = evaluate(
            mapObject,
            rules,
            citTagKey->asUInt,
            citValueKey->asUInt,
            outResultStorage,
            evaluateChildren,
            outMatchedRule);
        if (evaluationResult)
            return true;
    }

    if (citTagKey != citEnd)
    {
        const auto evaluationResult = evaluate(
            mapObject,
            rules,
            citTagKey->asUInt,
            0,
            outResultStorage,
            evaluateChildren,
            outMatchedRule);
        if (evaluationResult)
            return true;
    }

    const auto evaluationResult = evaluate(
        mapObject,
        rules,
        0u,
        0u,
        outResultStorage,
        evaluateChildren,
        outMatchedRule);
    if (evaluationResult)
        return true;

    return false;
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const std::shared_ptr<const MapStyleNode>& singleRule,
    MapStyleEvaluationResult* const outResultStorage,
    bool evaluateChildren) const
{
    return evaluate(nullptr, singleRule, _inputValues, outResultStorage, evaluateChildren);
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
    const QMap< uint64_t, std::shared_ptr<const MapStyleNode> >& rules,
    const uint32_t tagKey, const uint32_t valueKey,
    MapStyleEvaluationResult* const outResultStorage,
    bool evaluateChildren,
    std::shared_ptr<const MapStyleNode>* outMatchedRule) const
{
    auto inputValues = detachedOf(_inputValues);
    inputValues[_builtinValueDefs->id_INPUT_TAG].asUInt = tagKey;
    inputValues[_builtinValueDefs->id_INPUT_VALUE].asUInt = valueKey;

    const auto ruleId = MapStyle_P::encodeRuleId(tagKey, valueKey);
    const auto citRule = rules.constFind(ruleId);
    if (citRule == rules.cend())
        return false;

    const auto result = evaluate(mapObject.get(), *citRule, inputValues, outResultStorage, evaluateChildren);
    if (result && outMatchedRule)
        *outMatchedRule = *citRule;
    return result;
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const Model::BinaryMapObject* const mapObject,
    const std::shared_ptr<const MapStyleNode>& rule,
    const InputValuesDictionary& inputValues,
    MapStyleEvaluationResult* const outResultStorage,
    bool evaluateChildren) const
{
    //////////////////////////////////////////////////////////////////////////
    if (mapObject && ((mapObject->id >> 1) == 25829290u))
    {
        int i = 5;
    }
    //////////////////////////////////////////////////////////////////////////

    // Check all values of a rule until all are checked.
    const auto citInputValuesEnd = inputValues.cend();
    for (const auto& ruleValueEntry : rangeOf(constOf(rule->_p->_values)))
    {
        const auto& valueDef = ruleValueEntry.key();

        // Test only input values
        if (valueDef->valueClass != MapStyleValueClass::Input)
            continue;

        const auto& ruleValue = ruleValueEntry.value();
        const auto citInputValue = inputValues.constFind(valueDef->id);
        InputValue inputValue;
        if (citInputValue == citInputValuesEnd)
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Input '%s' was not defined, will use default value",
                qPrintable(valueDef->name));
        }
        else
        {
            inputValue = *citInputValue;
        }

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
        for (const auto& ruleValueEntry : rangeOf(constOf(rule->_p->_values)))
        {
            const auto& valueDef = ruleValueEntry.key();
            if (valueDef->valueClass != MapStyleValueClass::Output)
                continue;

            const auto& ruleValue = ruleValueEntry.value();

            switch (valueDef->dataType)
            {
                case MapStyleValueDataType::Boolean:
                    assert(!ruleValue.isComplex);
                    outResultStorage->values[valueDef->id] = (ruleValue.asSimple.asUInt == 1);
                    break;
                case MapStyleValueDataType::Integer:
                    outResultStorage->values[valueDef->id] =
                        ruleValue.isComplex
                        ? ruleValue.asComplex.asInt.evaluate(owner->displayDensityFactor)
                        : ruleValue.asSimple.asInt;
                    break;
                case MapStyleValueDataType::Float:
                    outResultStorage->values[valueDef->id] =
                        ruleValue.isComplex
                        ? ruleValue.asComplex.asFloat.evaluate(owner->displayDensityFactor)
                        : ruleValue.asSimple.asFloat;
                    break;
                case MapStyleValueDataType::String:
                    // Save value of a string instead of it's id
                    outResultStorage->values[valueDef->id] =
                        owner->style->_p->lookupStringValue(ruleValue.asSimple.asUInt);
                    break;
                case MapStyleValueDataType::Color:
                    assert(!ruleValue.isComplex);
                    outResultStorage->values[valueDef->id] = ruleValue.asSimple.asUInt;
                    break;
            }
        }
    }

    // In case evaluation of children is not requested, stop processing
    if (!evaluateChildren)
        return true;

    bool atLeastOneIfElseWasPositive = false;
    for (const auto& child : constOf(rule->_p->_ifElseChildren))
    {
        const auto evaluationResult = evaluate(mapObject, child, inputValues, outResultStorage, true);
        if (evaluationResult)
        {
            atLeastOneIfElseWasPositive = true;
            break;
        }
                
    }

    for (const auto& child : constOf(rule->_p->_ifChildren))
        evaluate(mapObject, child, inputValues, outResultStorage, true);

    return atLeastOneIfElseWasPositive;
}
