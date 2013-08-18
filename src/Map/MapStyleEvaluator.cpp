#include "MapStyleEvaluator.h"

#include <cassert>

#include "MapStyleRule.h"
#include "MapObject.h"
#include "OsmAndCore/Logging.h"

OsmAnd::MapStyleEvaluator::MapStyleEvaluator( const std::shared_ptr<const MapStyle>& style_, MapStyle::RulesetType ruleset_, const std::shared_ptr<const OsmAnd::Model::MapObject>& mapObject_ /*= std::shared_ptr<const OsmAnd::Model::MapObject>()*/ )
    : style(style_)
    , mapObject(mapObject_)
    , ruleset(ruleset_)
{
}

OsmAnd::MapStyleEvaluator::MapStyleEvaluator( const std::shared_ptr<const MapStyle>& style_, const std::shared_ptr<MapStyleRule>& singleRule_ )
    : style(style_)
    , singleRule(singleRule_)
    , mapObject()
    , ruleset(MapStyle::RulesetType::Invalid)
{
}

OsmAnd::MapStyleEvaluator::~MapStyleEvaluator()
{
}

void OsmAnd::MapStyleEvaluator::setValue( const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, const OsmAnd::MapStyleRule::Value& value )
{
    _values[ref.get()] = value;
}

void OsmAnd::MapStyleEvaluator::setBooleanValue( const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, const bool& value )
{
    _values[ref.get()].asInt = value ? 1 : 0;
}

void OsmAnd::MapStyleEvaluator::setIntegerValue( const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, const int& value )
{
    _values[ref.get()].asInt = value;
}

void OsmAnd::MapStyleEvaluator::setIntegerValue( const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, const unsigned int& value )
{
    _values[ref.get()].asUInt = value;
}

void OsmAnd::MapStyleEvaluator::setFloatValue( const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, const float& value )
{
    _values[ref.get()].asFloat = value;
}

void OsmAnd::MapStyleEvaluator::setStringValue( const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, const QString& value )
{
    bool ok = style->lookupStringId(value, _values[ref.get()].asUInt);
    if(!ok)
        _values[ref.get()].asUInt = std::numeric_limits<uint32_t>::max();
}

bool OsmAnd::MapStyleEvaluator::getBooleanValue( const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, bool& value ) const
{
    const auto& itValue = _values.find(ref.get());
    if(itValue == _values.end())
        return false;
    value = (*itValue).asInt == 1;
    return true;
}

bool OsmAnd::MapStyleEvaluator::getIntegerValue( const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, int& value ) const
{
    const auto& itValue = _values.find(ref.get());
    if(itValue == _values.end())
        return false;
    value = (*itValue).asInt;
    return true;
}

bool OsmAnd::MapStyleEvaluator::getIntegerValue( const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, unsigned int& value ) const
{
    const auto& itValue = _values.find(ref.get());
    if(itValue == _values.end())
        return false;
    value = (*itValue).asUInt;
    return true;
}

bool OsmAnd::MapStyleEvaluator::getFloatValue( const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, float& value ) const
{
    const auto& itValue = _values.find(ref.get());
    if(itValue == _values.end())
        return false;
    value = (*itValue).asFloat;
    return true;
}

bool OsmAnd::MapStyleEvaluator::getStringValue( const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, QString& value ) const
{
    const auto& itValue = _values.find(ref.get());
    if(itValue == _values.end())
        return false;
    value = style->lookupStringValue((*itValue).asUInt);
    return true;
}

void OsmAnd::MapStyleEvaluator::clearValue( const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref )
{
    _values.remove(ref.get());
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
        auto tagKey = _values[MapStyle::builtinValueDefinitions.INPUT_TAG.get()].asUInt;
        auto valueKey = _values[MapStyle::builtinValueDefinitions.INPUT_VALUE.get()].asUInt;

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
    _values[MapStyle::builtinValueDefinitions.INPUT_TAG.get()].asUInt = tagKey;
    _values[MapStyle::builtinValueDefinitions.INPUT_VALUE.get()].asUInt = valueKey;
    
    const auto& rules = static_cast<const MapStyle*>(style.get())->obtainRules(ruleset);
    uint64_t ruleId = MapStyle::encodeRuleId(tagKey, valueKey);
    auto itRule = rules.find(ruleId);
    if(itRule == rules.end())
        return false;
    auto evaluationResult = evaluate(*itRule, fillOutput, evaluateChildren);
    return evaluationResult;
}

bool OsmAnd::MapStyleEvaluator::evaluate( const std::shared_ptr<OsmAnd::MapStyleRule>& rule, bool fillOutput, bool evaluateChildren )
{
    auto itValueDef = rule->_valueDefinitionsRefs.begin();
    auto itValueData = rule->_values.begin();
    for(; itValueDef != rule->_valueDefinitionsRefs.end(); ++itValueDef, ++itValueData)
    {
        const auto& valueDef = *itValueDef;

        if(valueDef->type != MapStyle::ValueDefinition::Input)
            continue;

        const auto& valueData = *itValueData;
        const auto& stackValue = _values[valueDef.get()];

        bool evaluationResult = false;
        if(valueDef == MapStyle::builtinValueDefinitions.INPUT_MINZOOM)
        {
            evaluationResult = valueData.asInt <= stackValue.asInt;
        }
        else if(valueDef == MapStyle::builtinValueDefinitions.INPUT_MAXZOOM)
        {
            evaluationResult = valueData.asInt >= stackValue.asInt;
        }
        else if(valueDef == MapStyle::builtinValueDefinitions.INPUT_ADDITIONAL)
        {
            if(!mapObject)
                evaluationResult;
            else
            {
                const auto& strValue = style->lookupStringValue(valueData.asUInt);
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
        else if(valueDef->dataType == MapStyle::ValueDefinition::Float)
        {
            evaluationResult = qFuzzyCompare(valueData.asFloat, stackValue.asFloat);
        }
        else
        {
            evaluationResult = valueData.asInt == stackValue.asInt;
        }

        if(!evaluationResult)
            return false;
    }

    if (fillOutput || evaluateChildren)
    {
        auto itValueDef = rule->_valueDefinitionsRefs.begin();
        auto itValueData = rule->_values.begin();
        for(; itValueDef != rule->_valueDefinitionsRefs.end(); ++itValueDef, ++itValueData)
        {
            const auto& valueDef = *itValueDef;
            const auto& valueData = *itValueData;

            if(valueDef->type != MapStyle::ValueDefinition::Output)
                continue;

            _values[valueDef.get()] = valueData;
        }
    }

    if(evaluateChildren)
    {
        for(auto itChild = rule->_ifElseChildren.begin(); itChild != rule->_ifElseChildren.end(); ++itChild)
        {
            auto evaluationResult = evaluate(*itChild, fillOutput, true);
            if(evaluationResult)
                break;
        }

        for(auto itChild = rule->_ifChildren.begin(); itChild != rule->_ifChildren.end(); ++itChild)
        {
            evaluate(*itChild, fillOutput, true);
        }
    }
    
    return true;
}

void OsmAnd::MapStyleEvaluator::dump( bool input /*= true*/, bool output /*= true*/, const QString& prefix /*= QString()*/ ) const
{
    for(auto itValue = _values.begin(); itValue != _values.end(); ++itValue)
    {
        auto pValueDef = itValue.key();
        const auto& value = itValue.value();

        if((pValueDef->type == MapStyle::ValueDefinition::Input && input) || (pValueDef->type == MapStyle::ValueDefinition::Output && output))
        {
            auto strType = pValueDef->type == MapStyle::ValueDefinition::Input ? ">" : "<";
            OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s%s%s = ", prefix.toStdString().c_str(), strType, pValueDef->name.toStdString().c_str());

            switch (pValueDef->dataType)
            {
            case MapStyle::ValueDefinition::Boolean:
                OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s", value.asUInt == 1 ? "true" : "false");
                break;
            case MapStyle::ValueDefinition::Integer:
                OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%d", value.asInt);
                break;
            case MapStyle::ValueDefinition::Float:
                OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%f", value.asFloat);
                break;
            case MapStyle::ValueDefinition::String:
                OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s", style->lookupStringValue(value.asUInt).toStdString().c_str());
                break;
            case MapStyle::ValueDefinition::Color:
                OsmAnd::LogPrintf(LogSeverityLevel::Debug, "#%s", QString::number(value.asUInt, 16).toStdString().c_str());
                break;
            }
        }
    }
}
