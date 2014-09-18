#include "MapStyleEvaluator_P.h"
#include "MapStyleEvaluator.h"

#include "stdlib_common.h"
#include <cassert>

#include "QtExtensions.h"
#include "QtCommon.h"

#include "MapStyleBuiltinValueDefinitions.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleValue.h"
#include "BinaryMapObject.h"
#include "QKeyValueIterator.h"
#include "Logging.h"

OsmAnd::MapStyleEvaluator_P::MapStyleEvaluator_P(MapStyleEvaluator* owner_)
    : _builtinValueDefs(MapStyleBuiltinValueDefinitions::get())
    , owner(owner_)
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
    MapStyleValue parsedValue;
    const auto ok = owner->resolvedStyle->parseValue(value, valueDefId, parsedValue);
    if (!ok)
    {
        //LogPrintf(LogSeverityLevel::Warning,
        //    "Map style input string '%s' was not resolved in lookup table",
        //    qPrintable(value));
        return;
    }

    // Set value only in case it was resolved successfully
    auto& entry = _inputValues[valueDefId];
    entry.asUInt = parsedValue.asSimple.asUInt;
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
    const QHash< TagValueId, std::shared_ptr<const ResolvedMapStyle::Rule> >& ruleset,
    const ResolvedMapStyle::StringId tagStringId,
    const ResolvedMapStyle::StringId valueStringId,
    MapStyleEvaluationResult* const outResultStorage) const
{
    // Create copy of input values to change "tag" and "value" attributes
    auto inputValues = detachedOf(_inputValues);
    inputValues[_builtinValueDefs->id_INPUT_TAG].asUInt = tagStringId;
    inputValues[_builtinValueDefs->id_INPUT_VALUE].asUInt = valueStringId;

    const auto ruleId = TagValueId::compose(tagStringId, valueStringId);
    const auto citRule = ruleset.constFind(ruleId);
    if (citRule == ruleset.cend())
        return false;
    const auto& rule = *citRule;

    return evaluate(mapObject.get(), rule->rootNode, inputValues, outResultStorage);
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const Model::BinaryMapObject* const mapObject,
    const std::shared_ptr<const ResolvedMapStyle::RuleNode>& ruleNode,
    const InputValuesDictionary& inputValues,
    MapStyleEvaluationResult* const outResultStorage) const
{
    const auto citInputValuesEnd = inputValues.cend();

    // Check all values of a rule until all are checked.
    for (const auto& ruleValueEntry : rangeOf(constOf(ruleNode->values)))
    {
        const auto valueDefId = ruleValueEntry.key();
        const auto& valueDef = owner->resolvedStyle->getValueDefinitionById(valueDefId);

        // Test only input values
        if (valueDef->valueClass != MapStyleValueDefinition::Class::Input)
            continue;

        const auto& ruleValue = ruleValueEntry.value();
        const auto citInputValue = inputValues.constFind(valueDefId);
        InputValue inputValue;
        if (citInputValue == citInputValuesEnd)
        {
            //LogPrintf(LogSeverityLevel::Warning,
            //    "Input '%s' was not defined, will use default value (empty string or 0, depending on type)",
            //    qPrintable(valueDef->name));
        }
        else
        {
            inputValue = *citInputValue;
        }

        bool evaluationResult = false;
        if (valueDefId == _builtinValueDefs->id_INPUT_MINZOOM)
        {
            assert(!ruleValue.isComplex);
            evaluationResult = (ruleValue.asSimple.asInt <= inputValue.asInt);
        }
        else if (valueDefId == _builtinValueDefs->id_INPUT_MAXZOOM)
        {
            assert(!ruleValue.isComplex);
            evaluationResult = (ruleValue.asSimple.asInt >= inputValue.asInt);
        }
        else if (valueDefId == _builtinValueDefs->id_INPUT_ADDITIONAL)
        {
            if (!mapObject)
                evaluationResult = true;
            else
            {
                assert(!ruleValue.isComplex);
                const auto valueString = owner->resolvedStyle->getStringById(ruleValue.asSimple.asUInt);
                auto equalSignIdx = valueString.indexOf('=');
                if (equalSignIdx >= 0)
                {
                    const auto& tag = valueString.mid(0, equalSignIdx);
                    const auto& value = valueString.mid(equalSignIdx + 1);
                    evaluationResult = mapObject->containsTypeSlow(tag, value, true);
                }
                else
                    evaluationResult = false;
            }
        }
        else if (valueDefId == _builtinValueDefs->id_INPUT_TEST)
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

    // In case rule sets "disable", stop processing
    const auto citDisabledValue = ruleNode->values.constFind(_builtinValueDefs->id_OUTPUT_DISABLE);
    if (citDisabledValue != ruleNode->values.cend())
    {
        if (citDisabledValue->asSimple.asUInt != 0)
            return false;
    }

    if (outResultStorage && ruleNode->oneOfConditionalSubnodes.isEmpty())
        fillResultFromRuleNode(ruleNode, *outResultStorage, true);

    bool atLeastOneConditionalMatched = false;
    for (const auto& oneOfConditionalSubnode : constOf(ruleNode->oneOfConditionalSubnodes))
    {
        const auto evaluationResult = evaluate(mapObject, oneOfConditionalSubnode, inputValues, outResultStorage);
        if (evaluationResult)
        {
            atLeastOneConditionalMatched = true;
            break;
        }
    }

    if (atLeastOneConditionalMatched || ruleNode->oneOfConditionalSubnodes.isEmpty())
    {
        if (outResultStorage && atLeastOneConditionalMatched)
            fillResultFromRuleNode(ruleNode, *outResultStorage, false);

        for (const auto& applySubnode : constOf(ruleNode->applySubnodes))
            evaluate(mapObject, applySubnode, inputValues, outResultStorage);

        return true;
    }

    return false;
}

void OsmAnd::MapStyleEvaluator_P::fillResultFromRuleNode(
    const std::shared_ptr<const ResolvedMapStyle::RuleNode>& ruleNode,
    MapStyleEvaluationResult& outResultStorage,
    const bool allowOverride) const
{
    for (const auto& ruleValueEntry : rangeOf(constOf(ruleNode->values)))
    {
        const auto valueDefId = ruleValueEntry.key();
        const auto& valueDef = owner->resolvedStyle->getValueDefinitionById(valueDefId);

        // Skip all non-Output values
        if (valueDef->valueClass != MapStyleValueDefinition::Class::Output)
            continue;

        // If value already defined and override not allowed, do nothing
        const auto itResultValue = outResultStorage.values.find(valueDefId);
        if (itResultValue != outResultStorage.values.end() && !allowOverride)
            continue;
        
        QVariant resultValue;
        const auto& ruleValue = ruleValueEntry.value();
        switch (valueDef->dataType)
        {
            case MapStyleValueDataType::Boolean:
                assert(!ruleValue.isComplex);
                resultValue = (ruleValue.asSimple.asUInt == 1);
                break;
            case MapStyleValueDataType::Integer:
                resultValue =
                    ruleValue.isComplex
                    ? ruleValue.asComplex.asInt.evaluate(owner->displayDensityFactor)
                    : ruleValue.asSimple.asInt;
                break;
            case MapStyleValueDataType::Float:
                resultValue =
                    ruleValue.isComplex
                    ? ruleValue.asComplex.asFloat.evaluate(owner->displayDensityFactor)
                    : ruleValue.asSimple.asFloat;
                break;
            case MapStyleValueDataType::String:
                // Save value of a string instead of it's id
                resultValue =
                    owner->resolvedStyle->getStringById(ruleValue.asSimple.asUInt);
                break;
            case MapStyleValueDataType::Color:
                assert(!ruleValue.isComplex);
                resultValue = ruleValue.asSimple.asUInt;
                break;
        }

        // Store result
        if (itResultValue != outResultStorage.values.end())
            *itResultValue = resultValue;
        else
            outResultStorage.values[valueDefId] = resultValue;
    }
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
    const MapStyleRulesetType rulesetType,
    MapStyleEvaluationResult* const outResultStorage) const
{
    const auto& ruleset = owner->resolvedStyle->getRuleset(rulesetType);

    const auto citTagKey = _inputValues.constFind(_builtinValueDefs->id_INPUT_TAG);
    const auto citValueKey = _inputValues.constFind(_builtinValueDefs->id_INPUT_VALUE);
    const auto citEnd = _inputValues.cend();

    if (citTagKey != citEnd && citValueKey != citEnd)
    {
        const auto evaluationResult = evaluate(
            mapObject,
            ruleset,
            citTagKey->asUInt,
            citValueKey->asUInt,
            outResultStorage);
        if (evaluationResult)
            return true;
    }

    if (citTagKey != citEnd)
    {
        const auto evaluationResult = evaluate(
            mapObject,
            ruleset,
            citTagKey->asUInt,
            ResolvedMapStyle::EmptyStringId,
            outResultStorage);
        if (evaluationResult)
            return true;
    }

    const auto evaluationResult = evaluate(
        mapObject,
        ruleset,
        ResolvedMapStyle::EmptyStringId,
        ResolvedMapStyle::EmptyStringId,
        outResultStorage);
    if (evaluationResult)
        return true;

    return false;
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const std::shared_ptr<const ResolvedMapStyle::Attribute>& attribute,
    MapStyleEvaluationResult* const outResultStorage) const
{
    return evaluate(
        nullptr,
        attribute->rootNode,
        _inputValues,
        outResultStorage);
}
