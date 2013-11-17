#include "MapStyleEvaluator.h"
#include "MapStyleEvaluator_P.h"

#include <cassert>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleEvaluatorState.h"
#include "MapStyleEvaluatorState_P.h"
#include "MapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleRule_P.h"
#include "MapObject.h"
#include "Logging.h"

OsmAnd::MapStyleEvaluator::MapStyleEvaluator(
    const std::shared_ptr<MapStyleEvaluatorState>& state_,
    const std::shared_ptr<const MapStyle>& style_,
    const float displayDensityFactor_,
    MapStyleRulesetType ruleset_,
    const std::shared_ptr<const OsmAnd::Model::MapObject>& mapObject_ /*= std::shared_ptr<const OsmAnd::Model::MapObject>()*/)
    : _d(new MapStyleEvaluator_P(this))
    , state(state_)
    , style(style_)
    , displayDensityFactor(displayDensityFactor_)
    , mapObject(mapObject_)
    , ruleset(ruleset_)
{
}

OsmAnd::MapStyleEvaluator::MapStyleEvaluator(
    const std::shared_ptr<MapStyleEvaluatorState>& state_,
    const std::shared_ptr<const MapStyle>& style_,
    const float displayDensityFactor_,
    const std::shared_ptr<const MapStyleRule>& singleRule_)
    : _d(new MapStyleEvaluator_P(this))
    , state(state_)
    , style(style_)
    , displayDensityFactor(displayDensityFactor_)
    , singleRule(singleRule_)
    , mapObject()
    , ruleset(MapStyleRulesetType::Invalid)
{
}

OsmAnd::MapStyleEvaluator::~MapStyleEvaluator()
{
}

void OsmAnd::MapStyleEvaluator::setValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, const MapStyleValue value )
{
    setValue(ref->id, value);
}

void OsmAnd::MapStyleEvaluator::setValue(const int valueDefId, const MapStyleValue value)
{
    state->_d->_values[valueDefId] = value;
}

void OsmAnd::MapStyleEvaluator::setBooleanValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, const bool value )
{
    setBooleanValue(ref->id, value);
}

void OsmAnd::MapStyleEvaluator::setBooleanValue(const int valueDefId, const bool value)
{
    auto& entry = state->_d->_values[valueDefId];

    entry.isComplex = false;
    entry.asSimple.asInt = value ? 1 : 0;
}

void OsmAnd::MapStyleEvaluator::setIntegerValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, const int value )
{
    setIntegerValue(ref->id, value);
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const int valueDefId, const int value)
{
    auto& entry = state->_d->_values[valueDefId];

    entry.isComplex = false;
    entry.asSimple.asInt = value;
}

void OsmAnd::MapStyleEvaluator::setIntegerValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, const unsigned int value )
{
    setIntegerValue(ref->id, value);
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const int valueDefId, const unsigned int value)
{
    auto& entry = state->_d->_values[valueDefId];

    entry.isComplex = false;
    entry.asSimple.asUInt = value;
}

void OsmAnd::MapStyleEvaluator::setFloatValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, const float value )
{
    setFloatValue(ref->id, value);
}

void OsmAnd::MapStyleEvaluator::setFloatValue(const int valueDefId, const float value)
{
    auto& entry = state->_d->_values[valueDefId];

    entry.isComplex = false;
    entry.asSimple.asFloat = value;
}

void OsmAnd::MapStyleEvaluator::setStringValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, const QString& value )
{
    setStringValue(ref->id, value);
}

void OsmAnd::MapStyleEvaluator::setStringValue(const int valueDefId, const QString& value)
{
    auto& entry = state->_d->_values[valueDefId];

    entry.isComplex = false;
    bool ok = style->_d->lookupStringId(value, entry.asSimple.asUInt);
    if(!ok)
        entry.asSimple.asUInt = std::numeric_limits<uint32_t>::max();
}

bool OsmAnd::MapStyleEvaluator::getBooleanValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, bool& value ) const
{
    return getBooleanValue(ref->id, value);
}

bool OsmAnd::MapStyleEvaluator::getBooleanValue(const int valueDefId, bool& value) const
{
    const auto& itValue = state->_d->_values.constFind(valueDefId);
    if(itValue == state->_d->_values.cend())
        return false;

    assert(!itValue->isComplex);
    value = itValue->asSimple.asInt == 1;
    return true;
}

bool OsmAnd::MapStyleEvaluator::getIntegerValue( const std::shared_ptr<const OsmAnd::MapStyleValueDefinition>& ref, int& value ) const
{
    return getIntegerValue(ref->id, value);
}

bool OsmAnd::MapStyleEvaluator::getIntegerValue(const int valueDefId, int& value) const
{
    const auto& itValue = state->_d->_values.constFind(valueDefId);
    if(itValue == state->_d->_values.cend())
        return false;

    if(itValue->isComplex)
        value = (*itValue).asComplex.asInt.evaluate(displayDensityFactor);
    else
        value = (*itValue).asSimple.asInt;
    return true;
}

bool OsmAnd::MapStyleEvaluator::getIntegerValue( const std::shared_ptr<const OsmAnd::MapStyleValueDefinition>& ref, unsigned int& value ) const
{
    return getIntegerValue(ref->id, value);
}

bool OsmAnd::MapStyleEvaluator::getIntegerValue(const int valueDefId, unsigned int& value) const
{
    const auto& itValue = state->_d->_values.constFind(valueDefId);
    if(itValue == state->_d->_values.cend())
        return false;

    if(itValue->isComplex)
        value = (*itValue).asComplex.asUInt.evaluate(displayDensityFactor);
    else
        value = (*itValue).asSimple.asUInt;
    return true;
}

bool OsmAnd::MapStyleEvaluator::getFloatValue( const std::shared_ptr<const OsmAnd::MapStyleValueDefinition>& ref, float& value ) const
{
    return getFloatValue(ref->id, value);
}

bool OsmAnd::MapStyleEvaluator::getFloatValue(const int valueDefId, float& value) const
{
    const auto& itValue = state->_d->_values.constFind(valueDefId);
    if(itValue == state->_d->_values.cend())
        return false;

    if(itValue->isComplex)
        value = (*itValue).asComplex.asFloat.evaluate(displayDensityFactor);
    else
        value = (*itValue).asSimple.asFloat;
    return true;
}

bool OsmAnd::MapStyleEvaluator::getStringValue( const std::shared_ptr<const OsmAnd::MapStyleValueDefinition>& ref, QString& value ) const
{
    return getStringValue(ref->id, value);
}

bool OsmAnd::MapStyleEvaluator::getStringValue(const int valueDefId, QString& value) const
{
    const auto& itValue = state->_d->_values.constFind(valueDefId);
    if(itValue == state->_d->_values.cend())
        return false;

    assert(!itValue->isComplex);
    value = style->_d->lookupStringValue((*itValue).asSimple.asUInt);
    return true;
}

void OsmAnd::MapStyleEvaluator::clearValue( const std::shared_ptr<const OsmAnd::MapStyleValueDefinition>& ref )
{
    clearValue(ref->id);
}

void OsmAnd::MapStyleEvaluator::clearValue(const int valueDefId)
{
    state->_d->_values.remove(valueDefId);
}

bool OsmAnd::MapStyleEvaluator::evaluate( bool fillOutput /*= true*/, bool evaluateChildren /*=true*/ )
{
    if(singleRule)
    {
        auto evaluationResult = evaluate(singleRule, fillOutput, evaluateChildren);
        if(evaluationResult)
            return true;

        return false;
    }
    else
    {
        assert(!state->_d->_values[_d->_builtinValueDefs->id_INPUT_TAG].isComplex);
        const auto tagKey = state->_d->_values[_d->_builtinValueDefs->id_INPUT_TAG].asSimple.asUInt;
        assert(!state->_d->_values[_d->_builtinValueDefs->id_INPUT_VALUE].isComplex);
        const auto valueKey = state->_d->_values[_d->_builtinValueDefs->id_INPUT_VALUE].asSimple.asUInt;

        auto evaluationResult = evaluate(tagKey, valueKey, fillOutput, evaluateChildren);
        if(evaluationResult)
            return true;

        evaluationResult = evaluate(tagKey, 0, fillOutput, evaluateChildren);
        if(evaluationResult)
            return true;

        evaluationResult = evaluate(0, 0, fillOutput, evaluateChildren);
        if(evaluationResult)
            return true;

        return false;
    }
}

bool OsmAnd::MapStyleEvaluator::evaluate( uint32_t tagKey, uint32_t valueKey, bool fillOutput, bool evaluateChildren )
{
    state->_d->_values[_d->_builtinValueDefs->id_INPUT_TAG].asSimple.asUInt = tagKey;
    state->_d->_values[_d->_builtinValueDefs->id_INPUT_VALUE].asSimple.asUInt = valueKey;

    const auto& rules = style->_d->obtainRules(ruleset);
    uint64_t ruleId = MapStyle_P::encodeRuleId(tagKey, valueKey);
    auto itRule = rules.constFind(ruleId);
    if(itRule == rules.cend())
        return false;
    auto evaluationResult = evaluate(*itRule, fillOutput, evaluateChildren);
    return evaluationResult;
}

bool OsmAnd::MapStyleEvaluator::evaluate( const std::shared_ptr<const MapStyleRule>& rule, bool fillOutput, bool evaluateChildren )
{
    for(auto itRuleValueEntry = rule->_d->_valuesByRef.cbegin(); itRuleValueEntry != rule->_d->_valuesByRef.cend(); ++itRuleValueEntry)
    {
        const auto& valueDef = itRuleValueEntry.key();
        if(valueDef->valueClass != MapStyleValueClass::Input)
            continue;

        const auto& valueData = *itRuleValueEntry.value();
        const auto& stackValue = state->_d->_values[valueDef->id];

        bool evaluationResult = false;
        if(valueDef->id == _d->_builtinValueDefs->id_INPUT_MINZOOM)
        {
            assert(!valueData.isComplex);
            assert(!stackValue.isComplex);
            evaluationResult = (valueData.asSimple.asInt <= stackValue.asSimple.asInt);
        }
        else if(valueDef->id == _d->_builtinValueDefs->id_INPUT_MAXZOOM)
        {
            assert(!valueData.isComplex);
            assert(!stackValue.isComplex);
            evaluationResult = (valueData.asSimple.asInt >= stackValue.asSimple.asInt);
        }
        else if(valueDef->id == _d->_builtinValueDefs->id_INPUT_ADDITIONAL)
        {
            if(!mapObject)
                evaluationResult = true;
            else
            {
                assert(!valueData.isComplex);
                const auto& strValue = style->_d->lookupStringValue(valueData.asSimple.asUInt);
                auto equalSignIdx = strValue.indexOf('=');
                if(equalSignIdx >= 0)
                {
                    const auto& tag = strValue.mid(0, equalSignIdx);
                    const auto& value = strValue.mid(equalSignIdx + 1);
                    evaluationResult = mapObject->containsTypeSlow(tag, value, true);
                }
                else
                    evaluationResult = false;
            }
        }
        else if(valueDef->dataType == MapStyleValueDataType::Float)
        {
            const auto lvalue = valueData.isComplex ? valueData.asComplex.asFloat.evaluate(displayDensityFactor) : valueData.asSimple.asFloat;
            const auto rvalue = stackValue.isComplex ? stackValue.asComplex.asFloat.evaluate(displayDensityFactor) : stackValue.asSimple.asFloat;

            evaluationResult = qFuzzyCompare(lvalue, rvalue);
        }
        else
        {
            const auto lvalue = valueData.isComplex ? valueData.asComplex.asInt.evaluate(displayDensityFactor) : valueData.asSimple.asInt;
            const auto rvalue = stackValue.isComplex ? stackValue.asComplex.asInt.evaluate(displayDensityFactor) : stackValue.asSimple.asInt;

            evaluationResult = (lvalue == rvalue);
        }

        if(!evaluationResult)
            return false;
    }

    if (fillOutput || evaluateChildren)
    {
        for(auto itRuleValueEntry = rule->_d->_valuesByRef.cbegin(); itRuleValueEntry != rule->_d->_valuesByRef.cend(); ++itRuleValueEntry)
        {
            const auto& valueDef = itRuleValueEntry.key();
            if(valueDef->valueClass != MapStyleValueClass::Output)
                continue;

            state->_d->_values[valueDef->id] = *itRuleValueEntry.value();
        }
    }

    if(evaluateChildren)
    {
        for(auto itChild = rule->_d->_ifElseChildren.cbegin(); itChild != rule->_d->_ifElseChildren.cend(); ++itChild)
        {
            auto evaluationResult = evaluate(*itChild, fillOutput, true);
            if(evaluationResult)
                break;
        }

        for(auto itChild = rule->_d->_ifChildren.cbegin(); itChild != rule->_d->_ifChildren.cend(); ++itChild)
        {
            evaluate(*itChild, fillOutput, true);
        }
    }

    return true;
}

void OsmAnd::MapStyleEvaluator::dump( bool input /*= true*/, bool output /*= true*/, const QString& prefix /*= QString()*/ ) const
{
    /*
    for(auto itValue = state->_d->_values.cbegin(); itValue != state->_d->_values.cend(); ++itValue)
    {
        const auto& valueDef = itValue.key();
        const auto& value = itValue.value();

        if((valueDef->valueClass == MapStyleValueClass::Input && input) || (valueDef->valueClass == MapStyleValueClass::Output && output))
        {
            auto strType = valueDef->valueClass == MapStyleValueClass::Input ? ">" : "<";
            OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s%s%s = ", qPrintable(prefix), strType, qPrintable(valueDef->name));

            switch(valueDef->dataType)
            {
            case MapStyleValueDataType::Boolean:
                OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s", value.asSimple.asUInt == 1 ? "true" : "false");
                break;
            case MapStyleValueDataType::Integer:
                if(value.isComplex)
                    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%d:%d", value.asComplex.asInt.dip, value.asComplex.asInt.px);
                else
                    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%d", value.asSimple.asInt);
                break;
            case MapStyleValueDataType::Float:
                if(value.isComplex)
                    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%f:%f", value.asComplex.asFloat.dip, value.asComplex.asFloat.px);
                else
                    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%f", value.asSimple.asFloat);
                break;
            case MapStyleValueDataType::String:
                OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s", qPrintable(style->_d->lookupStringValue(value.asSimple.asUInt)));
                break;
            case MapStyleValueDataType::Color:
                OsmAnd::LogPrintf(LogSeverityLevel::Debug, "#%s", qPrintable(QString::number(value.asSimple.asUInt, 16)));
                break;
            }
        }
    }
    */
}
