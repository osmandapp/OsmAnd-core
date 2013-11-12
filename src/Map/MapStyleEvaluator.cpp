#include "MapStyleEvaluator.h"
#include "MapStyleEvaluator_P.h"

#include <cassert>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleRule_P.h"
#include "MapObject.h"
#include "Logging.h"

OsmAnd::MapStyleEvaluator::MapStyleEvaluator( const std::shared_ptr<const MapStyle>& style_, const float displayDensityFactor_, MapStyleRulesetType ruleset_, const std::shared_ptr<const OsmAnd::Model::MapObject>& mapObject_ /*= std::shared_ptr<const OsmAnd::Model::MapObject>()*/ )
    : _d(new MapStyleEvaluator_P(this))
    , style(style_)
    , displayDensityFactor(displayDensityFactor_)
    , mapObject(mapObject_)
    , ruleset(ruleset_)
{
}

OsmAnd::MapStyleEvaluator::MapStyleEvaluator( const std::shared_ptr<const MapStyle>& style_, const float displayDensityFactor_, const std::shared_ptr<const MapStyleRule>& singleRule_ )
    : _d(new MapStyleEvaluator_P(this))
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
    _d->_values[ref] = value;
}

void OsmAnd::MapStyleEvaluator::setBooleanValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, const bool value )
{
    _d->_values[ref].isComplex = false;
    _d->_values[ref].asSimple.asInt = value ? 1 : 0;
}

void OsmAnd::MapStyleEvaluator::setIntegerValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, const int value )
{
    _d->_values[ref].isComplex = false;
    _d->_values[ref].asSimple.asInt = value;
}

void OsmAnd::MapStyleEvaluator::setIntegerValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, const unsigned int value )
{
    _d->_values[ref].isComplex = false;
    _d->_values[ref].asSimple.asUInt = value;
}

void OsmAnd::MapStyleEvaluator::setFloatValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, const float value )
{
    _d->_values[ref].isComplex = false;
    _d->_values[ref].asSimple.asFloat = value;
}

void OsmAnd::MapStyleEvaluator::setStringValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, const QString& value )
{
    _d->_values[ref].isComplex = false;
    bool ok = style->_d->lookupStringId(value, _d->_values[ref].asSimple.asUInt);
    if(!ok)
        _d->_values[ref].asSimple.asUInt = std::numeric_limits<uint32_t>::max();
}

bool OsmAnd::MapStyleEvaluator::getBooleanValue( const std::shared_ptr<const MapStyleValueDefinition>& ref, bool& value ) const
{
    const auto& itValue = _d->_values.constFind(ref);
    if(itValue == _d->_values.cend())
        return false;

    assert(!itValue->isComplex);
    value = itValue->asSimple.asInt == 1;
    return true;
}

bool OsmAnd::MapStyleEvaluator::getIntegerValue( const std::shared_ptr<const OsmAnd::MapStyleValueDefinition>& ref, int& value ) const
{
    const auto& itValue = _d->_values.constFind(ref);
    if(itValue == _d->_values.cend())
        return false;

    if(itValue->isComplex)
        value = (*itValue).asComplex.asInt.evaluate(displayDensityFactor);
    else
        value = (*itValue).asSimple.asInt;
    return true;
}

bool OsmAnd::MapStyleEvaluator::getIntegerValue( const std::shared_ptr<const OsmAnd::MapStyleValueDefinition>& ref, unsigned int& value ) const
{
    const auto& itValue = _d->_values.constFind(ref);
    if(itValue == _d->_values.cend())
        return false;

    if(itValue->isComplex)
        value = (*itValue).asComplex.asUInt.evaluate(displayDensityFactor);
    else
        value = (*itValue).asSimple.asUInt;
    return true;
}

bool OsmAnd::MapStyleEvaluator::getFloatValue( const std::shared_ptr<const OsmAnd::MapStyleValueDefinition>& ref, float& value ) const
{
    const auto& itValue = _d->_values.constFind(ref);
    if(itValue == _d->_values.cend())
        return false;

    if(itValue->isComplex)
        value = (*itValue).asComplex.asFloat.evaluate(displayDensityFactor);
    else
        value = (*itValue).asSimple.asFloat;
    return true;
}

bool OsmAnd::MapStyleEvaluator::getStringValue( const std::shared_ptr<const OsmAnd::MapStyleValueDefinition>& ref, QString& value ) const
{
    const auto& itValue = _d->_values.constFind(ref);
    if(itValue == _d->_values.cend())
        return false;

    assert(!itValue->isComplex);
    value = style->_d->lookupStringValue((*itValue).asSimple.asUInt);
    return true;
}

void OsmAnd::MapStyleEvaluator::clearValue( const std::shared_ptr<const OsmAnd::MapStyleValueDefinition>& ref )
{
    _d->_values.remove(ref);
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
        assert(!_d->_values[MapStyle::builtinValueDefinitions.INPUT_TAG].isComplex);
        const auto tagKey = _d->_values[MapStyle::builtinValueDefinitions.INPUT_TAG].asSimple.asUInt;
        assert(!_d->_values[MapStyle::builtinValueDefinitions.INPUT_VALUE].isComplex);
        const auto valueKey = _d->_values[MapStyle::builtinValueDefinitions.INPUT_VALUE].asSimple.asUInt;

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
    _d->_values[MapStyle::builtinValueDefinitions.INPUT_TAG].asSimple.asUInt = tagKey;
    _d->_values[MapStyle::builtinValueDefinitions.INPUT_VALUE].asSimple.asUInt = valueKey;

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
    auto itValueDef = rule->_d->_valueDefinitionsRefs.cbegin();
    auto itValueData = rule->_d->_values.cbegin();
    for(; itValueDef != rule->_d->_valueDefinitionsRefs.cend(); ++itValueDef, ++itValueData)
    {
        const auto& valueDef = *itValueDef;

        if(valueDef->valueClass != MapStyleValueClass::Input)
            continue;

        const auto& valueData = *itValueData;
        const auto& stackValue = _d->_values[valueDef];

        bool evaluationResult = false;
        if(valueDef == MapStyle::builtinValueDefinitions.INPUT_MINZOOM)
        {
            assert(!valueData.isComplex);
            assert(!stackValue.isComplex);
            evaluationResult = (valueData.asSimple.asInt <= stackValue.asSimple.asInt);
        }
        else if(valueDef == MapStyle::builtinValueDefinitions.INPUT_MAXZOOM)
        {
            assert(!valueData.isComplex);
            assert(!stackValue.isComplex);
            evaluationResult = (valueData.asSimple.asInt >= stackValue.asSimple.asInt);
        }
        else if(valueDef == MapStyle::builtinValueDefinitions.INPUT_ADDITIONAL)
        {
            if(!mapObject)
                evaluationResult;
            else
            {
                assert(!valueData.isComplex);
                const auto& strValue = style->_d->lookupStringValue(valueData.asSimple.asUInt);
                auto equalSignIdx = strValue.indexOf('=');
                if(equalSignIdx >= 0)
                {
                    const auto& tag = strValue.mid(0, equalSignIdx);
                    const auto& value = strValue.mid(equalSignIdx + 1);
                    evaluationResult = mapObject->containsType(tag, value, true);
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
        auto itValueDef = rule->_d->_valueDefinitionsRefs.cbegin();
        auto itValueData = rule->_d->_values.cbegin();
        for(; itValueDef != rule->_d->_valueDefinitionsRefs.cend(); ++itValueDef, ++itValueData)
        {
            const auto& valueDef = *itValueDef;
            const auto& valueData = *itValueData;

            if(valueDef->valueClass != MapStyleValueClass::Output)
                continue;

            _d->_values[valueDef] = valueData;
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
    for(auto itValue = _d->_values.cbegin(); itValue != _d->_values.cend(); ++itValue)
    {
        auto pValueDef = itValue.key();
        const auto& value = itValue.value();

        if((pValueDef->valueClass == MapStyleValueClass::Input && input) || (pValueDef->valueClass == MapStyleValueClass::Output && output))
        {
            auto strType = pValueDef->valueClass == MapStyleValueClass::Input ? ">" : "<";
            OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s%s%s = ", qPrintable(prefix), strType, qPrintable(pValueDef->name));

            switch (pValueDef->dataType)
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
}
