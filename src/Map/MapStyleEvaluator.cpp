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
    auto& entry = _p->_inputValues[valueDefId];

    entry.asInt = value ? 1 : 0;
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const int valueDefId, const int value)
{
    auto& entry = _p->_inputValues[valueDefId];

    entry.asInt = value;
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const int valueDefId, const unsigned int value)
{
    auto& entry = _p->_inputValues[valueDefId];

    entry.asUInt = value;
}

void OsmAnd::MapStyleEvaluator::setFloatValue(const int valueDefId, const float value)
{
    auto& entry = _p->_inputValues[valueDefId];

    entry.asFloat = value;
}

void OsmAnd::MapStyleEvaluator::setStringValue(const int valueDefId, const QString& value)
{
    auto& entry = _p->_inputValues[valueDefId];

    bool ok = style->_p->lookupStringId(value, entry.asUInt);
    if (!ok)
        entry.asUInt = std::numeric_limits<uint32_t>::max();
}

bool OsmAnd::MapStyleEvaluator::evaluate(
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject, const MapStyleRulesetType ruleset,
    MapStyleEvaluationResult* const outResultStorage /*= nullptr*/,
    bool evaluateChildren /*= true*/)
{
    return _p->evaluate(mapObject, ruleset, outResultStorage, evaluateChildren);
}

bool OsmAnd::MapStyleEvaluator::evaluate(
    const std::shared_ptr<const MapStyleRule>& singleRule,
    MapStyleEvaluationResult* const outResultStorage /*= nullptr*/,
    bool evaluateChildren /*=true*/)
{
    return _p->evaluate(singleRule, outResultStorage, evaluateChildren);
}

void OsmAnd::MapStyleEvaluator::dump( bool input /*= true*/, bool output /*= true*/, const QString& prefix /*= QString::null*/ ) const
{
    /*
    for(auto itValue = _p->_inputValues.cbegin(); itValue != _p->_inputValues.cend(); ++itValue)
    {
        const auto& valueDef = itValue.key();
        const auto& value = itValue.value();

        if ((valueDef->valueClass == MapStyleValueClass::Input && input) || (valueDef->valueClass == MapStyleValueClass::Output && output))
        {
            auto strType = valueDef->valueClass == MapStyleValueClass::Input ? ">" : "<";
            LogPrintf(LogSeverityLevel::Debug, "%s%s%s = ", qPrintable(prefix), strType, qPrintable(valueDef->name));

            switch(valueDef->dataType)
            {
            case MapStyleValueDataType::Boolean:
                LogPrintf(LogSeverityLevel::Debug, "%s", value.asSimple.asUInt == 1 ? "true" : "false");
                break;
            case MapStyleValueDataType::Integer:
                if (value.isComplex)
                    LogPrintf(LogSeverityLevel::Debug, "%d:%d", value.asComplex.asInt.dip, value.asComplex.asInt.px);
                else
                    LogPrintf(LogSeverityLevel::Debug, "%d", value.asSimple.asInt);
                break;
            case MapStyleValueDataType::Float:
                if (value.isComplex)
                    LogPrintf(LogSeverityLevel::Debug, "%f:%f", value.asComplex.asFloat.dip, value.asComplex.asFloat.px);
                else
                    LogPrintf(LogSeverityLevel::Debug, "%f", value.asSimple.asFloat);
                break;
            case MapStyleValueDataType::String:
                LogPrintf(LogSeverityLevel::Debug, "%s", qPrintable(style->_p->lookupStringValue(value.asSimple.asUInt)));
                break;
            case MapStyleValueDataType::Color:
                LogPrintf(LogSeverityLevel::Debug, "#%s", qPrintable(QString::number(value.asSimple.asUInt, 16)));
                break;
            }
        }
    }
    */
}
