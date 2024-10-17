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
#include "MapObject.h"
#include "QKeyValueIterator.h"
#include "Logging.h"

OsmAnd::MapStyleEvaluator_P::MapStyleEvaluator_P(MapStyleEvaluator* owner_)
    : _builtinValueDefs(MapStyleBuiltinValueDefinitions::get())
    , owner(owner_)
    , intermediateEvaluationResultAllocator(std::bind(&MapStyleEvaluator_P::allocateIntermediateEvaluationResult, this))
{
}

OsmAnd::MapStyleEvaluator_P::~MapStyleEvaluator_P()
{
}

void OsmAnd::MapStyleEvaluator_P::prepare()
{
    const auto valueDefinitionsCount = owner->mapStyle->getValueDefinitionsCount();

    _inputValues.reset(new ArrayMap<InputValue>(valueDefinitionsCount));
    _inputValuesShadow.reset(new ArrayMap<InputValue>(valueDefinitionsCount));
    _intermediateEvaluationResult.reset(new ArrayMap<IMapStyle::Value>(valueDefinitionsCount));
    _constantIntermediateEvaluationResult.reset(new ArrayMap<IMapStyle::Value>(valueDefinitionsCount));
}

OsmAnd::ArrayMap<OsmAnd::IMapStyle::Value>* OsmAnd::MapStyleEvaluator_P::allocateIntermediateEvaluationResult()
{
    return new ArrayMap<IMapStyle::Value>(owner->mapStyle->getValueDefinitionsCount());
}

OsmAnd::MapStyleConstantValue OsmAnd::MapStyleEvaluator_P::evaluateConstantValue(
    const MapObject* const mapObject,
    const MapStyleValueDataType dataType,
    const IMapStyle::Value& resolvedValue,
    const std::shared_ptr<const InputValues>& inputValues,
    IntermediateEvaluationResult* outResultStorage,
    OnDemand<IntermediateEvaluationResult>& intermediateEvaluationResult) const
{
    if (!resolvedValue.isDynamic
        || resolvedValue.asDynamicValue.symbolClasses || resolvedValue.asDynamicValue.symbolClassTemplates)
        return resolvedValue.asConstantValue;

    bool wasDisabled = false;
    intermediateEvaluationResult->clear();
    OnDemand<IntermediateEvaluationResult> innerConstantEvaluationResult(intermediateEvaluationResultAllocator);
    evaluate(
        mapObject,
        resolvedValue.asDynamicValue.attribute->getRootNodeRef(),
        inputValues,
        wasDisabled,
        intermediateEvaluationResult.get(),
        innerConstantEvaluationResult);

    IMapStyle::Value evaluatedValue;
    switch (dataType)
    {
        case MapStyleValueDataType::Boolean:
            intermediateEvaluationResult->get(_builtinValueDefs->id_OUTPUT_ATTR_BOOL_VALUE, evaluatedValue);
            break;
        case MapStyleValueDataType::Integer:
            intermediateEvaluationResult->get(_builtinValueDefs->id_OUTPUT_ATTR_INT_VALUE, evaluatedValue);
            break;
        case MapStyleValueDataType::Float:
            intermediateEvaluationResult->get(_builtinValueDefs->id_OUTPUT_ATTR_FLOAT_VALUE, evaluatedValue);
            break;
        case MapStyleValueDataType::String:
            intermediateEvaluationResult->get(_builtinValueDefs->id_OUTPUT_ATTR_STRING_VALUE, evaluatedValue);
            break;
        case MapStyleValueDataType::Color:
            intermediateEvaluationResult->get(_builtinValueDefs->id_OUTPUT_ATTR_COLOR_VALUE, evaluatedValue);
            break;
    }

    if (!evaluatedValue.isDynamic)
        return evaluatedValue.asConstantValue;
    
    return evaluateConstantValue(
        mapObject,
        dataType,
        evaluatedValue,
        inputValues,
        outResultStorage,
        intermediateEvaluationResult);
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const std::shared_ptr<const MapObject>& mapObject,
    const QHash< TagValueId, std::shared_ptr<const IMapStyle::IRule> >& ruleset,
    const ResolvedMapStyle::StringId tagStringId,
    const ResolvedMapStyle::StringId valueStringId,
    MapStyleEvaluationResult* const outResultStorage,
    OnDemand<IntermediateEvaluationResult>& constantEvaluationResult) const
{
    const auto ruleId = TagValueId::compose(tagStringId, valueStringId);
    const auto citRule = ruleset.constFind(ruleId);
    if (citRule == ruleset.cend())
        return false;
    const auto& rule = *citRule;

    InputValue inputTag;
    inputTag.asUInt = tagStringId;
    _inputValuesShadow->set(_builtinValueDefs->id_INPUT_TAG, inputTag);

    InputValue inputValue;
    inputValue.asUInt = valueStringId;
    _inputValuesShadow->set(_builtinValueDefs->id_INPUT_VALUE, inputValue);

    if (outResultStorage)
        _intermediateEvaluationResult->clear();

    bool wasDisabled = false;
    const auto success = evaluate(
        mapObject.get(),
        rule->getRootNodeRef(),
        _inputValuesShadow,
        wasDisabled,
        _intermediateEvaluationResult.get(),
        constantEvaluationResult);
    if (!success || wasDisabled)
        return false;

    if (outResultStorage)
    {
        postprocessEvaluationResult(
            mapObject.get(),
            _inputValuesShadow,
            *_intermediateEvaluationResult,
            *outResultStorage,
            constantEvaluationResult);
    }

    return true;
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const MapObject* const mapObject,
    const std::shared_ptr<const IMapStyle::IRuleNode>& ruleNode,
    const std::shared_ptr<const InputValues>& inputValues,
    bool& outDisabled,
    IntermediateEvaluationResult* const outResultStorage,
    OnDemand<IntermediateEvaluationResult>& constantEvaluationResult) const
{
    // Check all values of a rule until all are checked.
    const auto& ruleNodeValues = ruleNode->getValuesRef();
    for (const auto& ruleValueEntry : rangeOf(constOf(ruleNodeValues)))
    {
        const auto valueDefId = ruleValueEntry.key();
        const auto& valueDef = owner->mapStyle->getValueDefinitionRefById(valueDefId);

        // Test only input values
        if (valueDef->valueClass != MapStyleValueDefinition::Class::Input)
            continue;

        const auto constantRuleValue = evaluateConstantValue(
            mapObject,
            valueDef->dataType,
            ruleValueEntry.value(),
            inputValues,
            outResultStorage,
            constantEvaluationResult);

        InputValue inputValue;
        inputValues->get(valueDefId, inputValue);

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
                evaluationResult = constantRuleValue.asSimple.asInt == inputValue.asInt;
            else
            {
                assert(!constantRuleValue.isComplex);
                const auto valueString = owner->mapStyle->getStringById(constantRuleValue.asSimple.asUInt);
                auto equalSignIdx = valueString.indexOf(QLatin1Char('='));
                if (equalSignIdx >= 0)
                {
                    const auto& tagRef = valueString.midRef(0, equalSignIdx);
                    const auto& valueRef = valueString.midRef(equalSignIdx + 1);
                    evaluationResult = mapObject->containsAttribute(tagRef, valueRef, true);
                }
                else
                    evaluationResult = mapObject->containsTag(valueString, true);
            }
        }
        else if (valueDefId == _builtinValueDefs->id_INPUT_TEST)
        {
            evaluationResult = (inputValue.asInt == 1);
        }
        else if (valueDef->dataType == MapStyleValueDataType::Float)
        {
            const auto lvalue = constantRuleValue.isComplex
                ? constantRuleValue.asComplex.asFloat.evaluate(owner->ptScaleFactor)
                : constantRuleValue.asSimple.asFloat;

            evaluationResult = qFuzzyCompare(lvalue, inputValue.asFloat);
        }
        else
        {
            const auto lvalue = constantRuleValue.isComplex
                ? constantRuleValue.asComplex.asInt.evaluate(owner->ptScaleFactor)
                : constantRuleValue.asSimple.asInt;

            evaluationResult = (lvalue == inputValue.asInt);
        }

        // If at least one value of rule does not match, it's failure
        if (!evaluationResult)
            return false;
    }

    // In case rule sets "disable", stop processing
    const auto citDisabledValue = ruleNodeValues.constFind(_builtinValueDefs->id_OUTPUT_DISABLE);
    if (citDisabledValue != ruleNodeValues.cend())
    {
        const auto disableValue = evaluateConstantValue(
            mapObject,
            _builtinValueDefs->OUTPUT_DISABLE->dataType,
            *citDisabledValue,
            inputValues,
            outResultStorage,
            constantEvaluationResult);

        assert(!disableValue.isComplex);
        if (disableValue.asSimple.asUInt != 0)
        {
            outDisabled = true;
            return false;
        }
    }

    // In case rule doesn't have any enabled class in "rClass", stop processing
    const auto citSymbolClassValue = ruleNodeValues.constFind(_builtinValueDefs->id_OUTPUT_CLASS);
    if (citSymbolClassValue != ruleNodeValues.cend())
    {
        bool atLeastOneClassEnabled = false;
        if (citSymbolClassValue->asDynamicValue.symbolClasses)
        {
            const auto& symbolClasses = *citSymbolClassValue->asDynamicValue.symbolClasses;
            for (const auto& symbolClass : symbolClasses)
            {
                const auto& symbolClassDefId = owner->mapStyle->getValueDefinitionIdByNameId(symbolClass->getNameId());
                if (symbolClassDefId > -1)
                {
                    InputValue symbolClassValue;
                    inputValues->get(symbolClassDefId, symbolClassValue);
                    if (symbolClassValue.asUInt != 0)
                        atLeastOneClassEnabled = true;
                }
            }
        }
        if (citSymbolClassValue->asDynamicValue.symbolClassTemplates)
        {
            const auto& symbolClassTemplates = *citSymbolClassValue->asDynamicValue.symbolClassTemplates;
            for (const auto& symbolClassTemplateId : symbolClassTemplates)
            {
                const auto& symbolClassTemplate = owner->mapStyle->getStringById(symbolClassTemplateId);
                const auto splitPosition = symbolClassTemplate.indexOf(QLatin1Char('$'));
                const auto& classNameHeadPart = symbolClassTemplate.left(splitPosition);
                const auto& classNameTagName = symbolClassTemplate.mid(splitPosition + 1);
                const auto& valueDefId = owner->mapStyle->getValueDefinitionIdByName(classNameTagName);
                if (valueDefId > -1)
                {
                    InputValue inputValue;
                    if (inputValues->get(valueDefId, inputValue))
                    {
                        const auto& classNameTailPart = owner->mapStyle->getStringById(inputValue.asUInt);
                        const auto& className = classNameHeadPart + classNameTailPart;
                        const auto& symbolClassDefId = owner->mapStyle->getValueDefinitionIdByName(className);
                        if (symbolClassDefId > -1)
                        {
                            InputValue symbolClassValue;
                            inputValues->get(symbolClassDefId, symbolClassValue);
                            if (symbolClassValue.asUInt != 0)
                                atLeastOneClassEnabled = true;
                        }
                    }
                }
            }
        }
        if (!atLeastOneClassEnabled)
        {
            outDisabled = true;
            return false;
        }
    }

    if (outResultStorage && !ruleNode->getIsSwitch())
        fillResultFromRuleNode(ruleNode, *outResultStorage, true);

    bool atLeastOneConditionalMatched = false;
    const auto& oneOfConditionalSubnodes = ruleNode->getOneOfConditionalSubnodesRef();
    for (const auto& oneOfConditionalSubnode : constOf(oneOfConditionalSubnodes))
    {
        const auto evaluationResult = evaluate(
            mapObject,
            oneOfConditionalSubnode,
            inputValues,
            outDisabled,
            outResultStorage,
            constantEvaluationResult);

        if (evaluationResult)
        {
            atLeastOneConditionalMatched = true;
            break;
        }
    }
    if (!atLeastOneConditionalMatched && ruleNode->getIsSwitch())
        return false;

    if (outResultStorage && ruleNode->getIsSwitch())
    {
        // Fill values from <switch> keeping values previously set by <case>
        fillResultFromRuleNode(ruleNode, *outResultStorage, false);
    }

    const auto& applySubnodes = ruleNode->getApplySubnodesRef();
    for (const auto& applySubnode : constOf(applySubnodes))
    {
        evaluate(
            mapObject,
            applySubnode,
            inputValues,
            outDisabled,
            outResultStorage,
            constantEvaluationResult);
    }

    if (outDisabled)
        return false;

    return true;
}

void OsmAnd::MapStyleEvaluator_P::fillResultFromRuleNode(
    const std::shared_ptr<const IMapStyle::IRuleNode>& ruleNode,
    IntermediateEvaluationResult& outResultStorage,
    const bool allowOverride) const
{
    const auto& values = ruleNode->getValuesRef();
    for (const auto& ruleValueEntry : rangeOf(constOf(values)))
    {
        const auto valueDefId = ruleValueEntry.key();
        const auto& valueDef = owner->mapStyle->getValueDefinitionRefById(valueDefId);

        // Skip all non-Output values
        if (valueDef->valueClass != MapStyleValueDefinition::Class::Output)
            continue;

        // If value already defined and override not allowed, do nothing
        if (outResultStorage.contains(valueDefId) && !allowOverride)
            continue;
        
        // Store result
        outResultStorage.set(valueDefId, ruleValueEntry.value());
    }
}

void OsmAnd::MapStyleEvaluator_P::postprocessEvaluationResult(
    const MapObject* const mapObject,
    const std::shared_ptr<const InputValues>& inputValues,
    IntermediateEvaluationResult& intermediateResult,
    MapStyleEvaluationResult& outResultStorage,
    OnDemand<IntermediateEvaluationResult>& constantEvaluationResult) const
{
    IMapStyle::Value value;
    for (IntermediateEvaluationResult::KeyType idx = 0, count = intermediateResult.size(); idx < count; idx++)
    {
        if (!intermediateResult.get(idx, value))
            continue;

        const auto valueDefId = static_cast<IMapStyle::ValueDefinitionId>(idx);
        const auto& valueDef = owner->mapStyle->getValueDefinitionRefById(valueDefId);

        const auto constantRuleValue = evaluateConstantValue(
            mapObject,
            valueDef->dataType,
            value,
            inputValues,
            &intermediateResult,
            constantEvaluationResult);

        QVariant postprocessedValue;
        switch (valueDef->dataType)
        {
            case MapStyleValueDataType::Boolean:
                assert(!constantRuleValue.isComplex);
                postprocessedValue = (constantRuleValue.asSimple.asUInt != 0);
                break;
            case MapStyleValueDataType::Integer:
                postprocessedValue = constantRuleValue.isComplex
                    ? constantRuleValue.asComplex.asInt.evaluate(owner->ptScaleFactor)
                    : constantRuleValue.asSimple.asInt;
                break;
            case MapStyleValueDataType::Float:
                postprocessedValue = constantRuleValue.isComplex
                    ? constantRuleValue.asComplex.asFloat.evaluate(owner->ptScaleFactor)
                    : constantRuleValue.asSimple.asFloat;
                break;
            case MapStyleValueDataType::String:
                assert(!constantRuleValue.isComplex);
                // Save value of a string instead of it's id
                postprocessedValue = owner->mapStyle->getStringById(constantRuleValue.asSimple.asUInt);
                break;
            case MapStyleValueDataType::Color:
                assert(!constantRuleValue.isComplex);
                postprocessedValue = constantRuleValue.asSimple.asUInt;
                break;
        }

        outResultStorage.setValue(valueDefId, postprocessedValue);
    }
}

void OsmAnd::MapStyleEvaluator_P::setBooleanValue(const int valueDefId, const bool value)
{
    InputValue valueEntry;
    valueEntry.asInt = value ? 1 : 0;

    _inputValues->set(valueDefId, valueEntry);
    _inputValuesShadow->set(valueDefId, valueEntry);
}

void OsmAnd::MapStyleEvaluator_P::setIntegerValue(const int valueDefId, const int value)
{
    InputValue valueEntry;
    valueEntry.asInt = value;

    _inputValues->set(valueDefId, valueEntry);
    _inputValuesShadow->set(valueDefId, valueEntry);
}

void OsmAnd::MapStyleEvaluator_P::setIntegerValue(const int valueDefId, const unsigned int value)
{
    InputValue valueEntry;
    valueEntry.asUInt = value;

    _inputValues->set(valueDefId, valueEntry);
    _inputValuesShadow->set(valueDefId, valueEntry);
}

void OsmAnd::MapStyleEvaluator_P::setFloatValue(const int valueDefId, const float value)
{
    InputValue valueEntry;
    valueEntry.asFloat = value;

    _inputValues->set(valueDefId, valueEntry);
    _inputValuesShadow->set(valueDefId, valueEntry);
}

void OsmAnd::MapStyleEvaluator_P::setStringValue(const int valueDefId, const QString& value)
{
    InputValue valueEntry;

    MapStyleConstantValue parsedValue;
    const auto ok = owner->mapStyle->parseValue(value, valueDefId, parsedValue);
    if (!ok)
    {
        //LogPrintf(LogSeverityLevel::Warning,
        //    "Map style input string '%s' was not resolved in lookup table",
        //    qPrintable(value));
        valueEntry.asUInt = std::numeric_limits<uint32_t>::max();
    }
    else
        valueEntry.asUInt = parsedValue.asSimple.asUInt;

    _inputValues->set(valueDefId, valueEntry);
    _inputValuesShadow->set(valueDefId, valueEntry);
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const std::shared_ptr<const MapObject>& mapObject,
    const MapStyleRulesetType rulesetType,
    MapStyleEvaluationResult* const outResultStorage) const
{
    //////////////////////////////////////////////////////////////////////////
    //if ((mapObject->id >> 1) == 266877135u)
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    const auto& ruleset = owner->mapStyle->getRuleset(rulesetType);

    _constantIntermediateEvaluationResult->clear();
    OnDemand<IntermediateEvaluationResult> constantEvaluationResult(_constantIntermediateEvaluationResult);

    if (_inputValues->contains(_builtinValueDefs->id_INPUT_TAG) && _inputValues->contains(_builtinValueDefs->id_INPUT_VALUE))
    {
        const auto evaluationResult = evaluate(
            mapObject,
            ruleset,
            _inputValues->getRef(_builtinValueDefs->id_INPUT_TAG)->asUInt,
            _inputValues->getRef(_builtinValueDefs->id_INPUT_VALUE)->asUInt,
            outResultStorage,
            constantEvaluationResult);
        if (evaluationResult)
            return true;
    }

    if (_inputValues->contains(_builtinValueDefs->id_INPUT_TAG))
    {
        const auto evaluationResult = evaluate(
            mapObject,
            ruleset,
            _inputValues->getRef(_builtinValueDefs->id_INPUT_TAG)->asUInt,
            ResolvedMapStyle::EmptyStringId,
            outResultStorage,
            constantEvaluationResult);
        if (evaluationResult)
            return true;
    }

    const auto evaluationResult = evaluate(
        mapObject,
        ruleset,
        ResolvedMapStyle::EmptyStringId,
        ResolvedMapStyle::EmptyStringId,
        outResultStorage,
        constantEvaluationResult);
    if (evaluationResult)
        return true;

    return false;
}

bool OsmAnd::MapStyleEvaluator_P::evaluate(
    const std::shared_ptr<const IMapStyle::IAttribute>& attribute,
    MapStyleEvaluationResult* const outResultStorage) const
{
    if (outResultStorage)
        _intermediateEvaluationResult->clear();

    _constantIntermediateEvaluationResult->clear();
    OnDemand<IntermediateEvaluationResult> constantEvaluationResult(_constantIntermediateEvaluationResult);

    bool wasDisabled = false;
    const auto success = evaluate(
        nullptr,
        attribute->getRootNodeRef(),
        _inputValues,
        wasDisabled,
        _intermediateEvaluationResult.get(),
        constantEvaluationResult);
    if (!success || wasDisabled)
        return false;

    if (outResultStorage)
    {
        postprocessEvaluationResult(
            nullptr,
            _inputValues,
            *_intermediateEvaluationResult,
            *outResultStorage,
            constantEvaluationResult);
    }

    return true;
}
