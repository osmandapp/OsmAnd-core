#include "MapStyleEvaluator_P.h"
#include "MapStyleEvaluator.h"

#include "stdlib_common.h"
#include <cassert>

#include "QtExtensions.h"
#include "QtCommon.h"

#include "MapStyleBuiltinValueDefinitions.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleConstantValue.h"
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
    MapStyleConstantValue parsedValue;
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

OsmAnd::MapStyleConstantValue OsmAnd::MapStyleEvaluator_P::evaluateConstantValue(
    const Model::BinaryMapObject* const mapObject,
    const MapStyleValueDataType dataType,
    const ResolvedMapStyle::ResolvedValue& resolvedValue,
    const InputValuesDictionary& inputValues) const
{
    if (resolvedValue.isDynamic)
    {
        IntermediateEvaluationResult tempEvaluationResult;
        evaluate(mapObject, resolvedValue.asDynamicValue.attribute->rootNode, inputValues, &tempEvaluationResult);

        ResolvedMapStyle::ResolvedValue evaluatedValue;
        switch (dataType)
        {
            case MapStyleValueDataType::Boolean:
            {
                const auto citOutputValue = tempEvaluationResult.constFind(_builtinValueDefs->id_OUTPUT_ATTR_BOOL_VALUE);
                if (citOutputValue != tempEvaluationResult.cend())
                    evaluatedValue = *citOutputValue;
                break;
            }

            case MapStyleValueDataType::Integer:
            {
                const auto citOutputValue = tempEvaluationResult.constFind(_builtinValueDefs->id_OUTPUT_ATTR_INT_VALUE);
                if (citOutputValue != tempEvaluationResult.cend())
                    evaluatedValue = *citOutputValue;
                break;
            }

            case MapStyleValueDataType::Float:
            {
                const auto citOutputValue = tempEvaluationResult.constFind(_builtinValueDefs->id_OUTPUT_ATTR_FLOAT_VALUE);
                if (citOutputValue != tempEvaluationResult.cend())
                    evaluatedValue = *citOutputValue;
                break;
            }

            case MapStyleValueDataType::String:
            {
                const auto citOutputValue = tempEvaluationResult.constFind(_builtinValueDefs->id_OUTPUT_ATTR_STRING_VALUE);
                if (citOutputValue != tempEvaluationResult.cend())
                    evaluatedValue = *citOutputValue;
                break;
            }

            case MapStyleValueDataType::Color:
            {
                const auto citOutputValue = tempEvaluationResult.constFind(_builtinValueDefs->id_OUTPUT_ATTR_COLOR_VALUE);
                if (citOutputValue != tempEvaluationResult.cend())
                    evaluatedValue = *citOutputValue;
                break;
            }
        }

        if (evaluatedValue.isDynamic)
        {
            return evaluateConstantValue(
                mapObject,
                dataType,
                evaluatedValue,
                inputValues);
        }
        return evaluatedValue.asConstantValue;
    }

    return resolvedValue.asConstantValue;
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

    IntermediateEvaluationResult intermediateEvaluationResult;
    IntermediateEvaluationResult* const pIntermediateEvaluationResult = outResultStorage ? &intermediateEvaluationResult : nullptr;

    const auto success = evaluate(mapObject.get(), rule->rootNode, inputValues, pIntermediateEvaluationResult);
    if (!success)
        return false;

    if (outResultStorage)
    {
        postprocessEvaluationResult(
            mapObject.get(),
            inputValues,
            intermediateEvaluationResult,
            *outResultStorage);
    }

    return true;
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const Model::BinaryMapObject* const mapObject,
    const std::shared_ptr<const ResolvedMapStyle::RuleNode>& ruleNode,
    const InputValuesDictionary& inputValues,
    IntermediateEvaluationResult* const outResultStorage) const
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

        const auto constantRuleValue = evaluateConstantValue(
            mapObject,
            valueDef->dataType,
            ruleValueEntry.value(),
            inputValues);

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
            assert(!constantRuleValue.isComplex);
            evaluationResult = (constantRuleValue.asSimple.asInt <= inputValue.asInt);
        }
        else if (valueDefId == _builtinValueDefs->id_INPUT_MAXZOOM)
        {
            assert(!constantRuleValue.isComplex);
            evaluationResult = (constantRuleValue.asSimple.asInt >= inputValue.asInt);
        }
        else if (valueDefId == _builtinValueDefs->id_INPUT_ADDITIONAL)
        {
            if (!mapObject)
                evaluationResult = true;
            else
            {
                assert(!constantRuleValue.isComplex);
                const auto valueString = owner->resolvedStyle->getStringById(constantRuleValue.asSimple.asUInt);
                auto equalSignIdx = valueString.indexOf(QLatin1Char('='));
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
            const auto lvalue = constantRuleValue.isComplex
                ? constantRuleValue.asComplex.asFloat.evaluate(owner->displayDensityFactor)
                : constantRuleValue.asSimple.asFloat;

            evaluationResult = qFuzzyCompare(lvalue, inputValue.asFloat);
        }
        else
        {
            const auto lvalue = constantRuleValue.isComplex
                ? constantRuleValue.asComplex.asInt.evaluate(owner->displayDensityFactor)
                : constantRuleValue.asSimple.asInt;

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
        const auto disableValue = evaluateConstantValue(
            mapObject,
            _builtinValueDefs->OUTPUT_DISABLE->dataType,
            *citDisabledValue,
            inputValues);

        assert(!disableValue.isComplex);
        if (disableValue.asSimple.asUInt != 0)
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
    if (!atLeastOneConditionalMatched && !ruleNode->oneOfConditionalSubnodes.isEmpty())
        return false;

    if (outResultStorage && atLeastOneConditionalMatched)
        fillResultFromRuleNode(ruleNode, *outResultStorage, false);

    for (const auto& applySubnode : constOf(ruleNode->applySubnodes))
        evaluate(mapObject, applySubnode, inputValues, outResultStorage);

    return true;
}

void OsmAnd::MapStyleEvaluator_P::fillResultFromRuleNode(
    const std::shared_ptr<const ResolvedMapStyle::RuleNode>& ruleNode,
    IntermediateEvaluationResult& outResultStorage,
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
        const auto itResultValue = outResultStorage.find(valueDefId);
        if (itResultValue != outResultStorage.end() && !allowOverride)
            continue;
        
        // Store result
        if (itResultValue != outResultStorage.end())
            *itResultValue = ruleValueEntry.value();
        else
            outResultStorage[valueDefId] = ruleValueEntry.value();
    }
}

void OsmAnd::MapStyleEvaluator_P::postprocessEvaluationResult(
    const Model::BinaryMapObject* const mapObject,
    const InputValuesDictionary& inputValues,
    const IntermediateEvaluationResult& intermediateResult,
    MapStyleEvaluationResult& outResultStorage) const
{
    for (const auto& intermediateResultEntry : rangeOf(constOf(intermediateResult)))
    {
        const auto valueDefId = intermediateResultEntry.key();
        const auto& valueDef = owner->resolvedStyle->getValueDefinitionById(valueDefId);

        const auto constantRuleValue = evaluateConstantValue(
            mapObject,
            valueDef->dataType,
            intermediateResultEntry.value(),
            inputValues);

        auto& postprocessedValue = outResultStorage.values[valueDefId];

        switch (valueDef->dataType)
        {
            case MapStyleValueDataType::Boolean:
                assert(!constantRuleValue.isComplex);
                postprocessedValue = (constantRuleValue.asSimple.asUInt != 0);
                break;
            case MapStyleValueDataType::Integer:
                postprocessedValue = constantRuleValue.isComplex
                    ? constantRuleValue.asComplex.asInt.evaluate(owner->displayDensityFactor)
                    : constantRuleValue.asSimple.asInt;
                break;
            case MapStyleValueDataType::Float:
                postprocessedValue = constantRuleValue.isComplex
                    ? constantRuleValue.asComplex.asFloat.evaluate(owner->displayDensityFactor)
                    : constantRuleValue.asSimple.asFloat;
                break;
            case MapStyleValueDataType::String:
                assert(!constantRuleValue.isComplex);
                // Save value of a string instead of it's id
                postprocessedValue = owner->resolvedStyle->getStringById(constantRuleValue.asSimple.asUInt);
                break;
            case MapStyleValueDataType::Color:
                assert(!constantRuleValue.isComplex);
                postprocessedValue = constantRuleValue.asSimple.asUInt;
                break;
        }
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
    IntermediateEvaluationResult intermediateEvaluationResult;
    IntermediateEvaluationResult* const pIntermediateEvaluationResult = outResultStorage ? &intermediateEvaluationResult : nullptr;

    const auto success = evaluate(
        nullptr,
        attribute->rootNode,
        _inputValues,
        pIntermediateEvaluationResult);
    if (!success)
        return false;

    if (outResultStorage)
    {
        postprocessEvaluationResult(
            nullptr,
            _inputValues,
            intermediateEvaluationResult,
            *outResultStorage);
    }

    return true;
}
