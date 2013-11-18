#include "MapStyleEvaluator.h"
#include "MapStyleEvaluator_P.h"

#include <cassert>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "Logging.h"

OsmAnd::MapStyleEvaluator::MapStyleEvaluator(
    const std::shared_ptr<const MapStyle>& style_,
    const float displayDensityFactor_,
    MapStyleRulesetType ruleset_,
    const std::shared_ptr<const OsmAnd::Model::MapObject>& mapObject_ /*= std::shared_ptr<const OsmAnd::Model::MapObject>()*/)
    : _d(new MapStyleEvaluator_P(this))
    , style(style_)
    , displayDensityFactor(displayDensityFactor_)
    , mapObject(mapObject_)
    , ruleset(ruleset_)
{
}

OsmAnd::MapStyleEvaluator::MapStyleEvaluator(
    const std::shared_ptr<const MapStyle>& style_,
    const float displayDensityFactor_,
    const std::shared_ptr<const MapStyleRule>& singleRule_)
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

void OsmAnd::MapStyleEvaluator::setBooleanValue(const int valueDefId, const bool value)
{
    auto& entry = _d->_inputValues[valueDefId];

    entry.asInt = value ? 1 : 0;
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const int valueDefId, const int value)
{
    auto& entry = _d->_inputValues[valueDefId];

    entry.asInt = value;
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const int valueDefId, const unsigned int value)
{
    auto& entry = _d->_inputValues[valueDefId];

    entry.asUInt = value;
}

void OsmAnd::MapStyleEvaluator::setFloatValue(const int valueDefId, const float value)
{
    auto& entry = _d->_inputValues[valueDefId];

    entry.asFloat = value;
}

void OsmAnd::MapStyleEvaluator::setStringValue(const int valueDefId, const QString& value)
{
    auto& entry = _d->_inputValues[valueDefId];

    bool ok = style->_d->lookupStringId(value, entry.asUInt);
    if(!ok)
        entry.asUInt = std::numeric_limits<uint32_t>::max();
}

bool OsmAnd::MapStyleEvaluator::evaluate(MapStyleEvaluationResult* const outResultStorage /*= nullptr*/, bool evaluateChildren /*=true*/)
{
    return _d->evaluate(outResultStorage, evaluateChildren);
}

void OsmAnd::MapStyleEvaluator::dump( bool input /*= true*/, bool output /*= true*/, const QString& prefix /*= QString()*/ ) const
{
    /*
    for(auto itValue = _d->_inputValues.cbegin(); itValue != _d->_inputValues.cend(); ++itValue)
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
