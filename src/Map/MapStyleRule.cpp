#include "MapStyleRule.h"
#include "MapStyleRule_P.h"

#include <cassert>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleValue.h"
#include "QKeyValueIterator.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::MapStyleRule::MapStyleRule(MapStyle* owner_, const QHash< QString, QString >& attributes)
    : _d(new MapStyleRule_P(this))
    , owner(owner_)
{
    _d->_values.reserve(attributes.size());
    _d->_resolvedValueDefinitions.reserve(attributes.size());
    
    for(const auto& attributeEntry : rangeOf(constOf(attributes)))
    {
        const auto& key = attributeEntry.key();
        const auto& value = attributeEntry.value();

        std::shared_ptr<const MapStyleValueDefinition> valueDef;
        bool ok = owner->resolveValueDefinition(key, valueDef);
        assert(ok);

        // Store resolved value definition
        _d->_resolvedValueDefinitions.insert(key, valueDef);

        MapStyleValue parsedValue;
        switch (valueDef->dataType)
        {
        case MapStyleValueDataType::Boolean:
            parsedValue.asSimple.asInt = (value == QLatin1String("true")) ? 1 : 0;
            break;
        case MapStyleValueDataType::Integer:
            {
                if(valueDef->isComplex)
                {
                    parsedValue.isComplex = true;
                    if(!value.contains(':'))
                    {
                        parsedValue.asComplex.asInt.dip = Utilities::parseArbitraryInt(value, -1);
                        parsedValue.asComplex.asInt.px = 0.0;
                    }
                    else
                    {
                        // 'dip:px' format
                        const auto& complexValue = value.split(':', QString::KeepEmptyParts);

                        parsedValue.asComplex.asInt.dip = Utilities::parseArbitraryInt(complexValue[0], 0);
                        parsedValue.asComplex.asInt.px = Utilities::parseArbitraryInt(complexValue[1], 0);
                    }
                }
                else
                {
                    assert(!value.contains(':'));
                    parsedValue.asSimple.asInt = Utilities::parseArbitraryInt(value, -1);
                }
            }
            break;
        case MapStyleValueDataType::Float:
            {
                if(valueDef->isComplex)
                {
                    parsedValue.isComplex = true;
                    if(!value.contains(':'))
                    {
                        parsedValue.asComplex.asFloat.dip = Utilities::parseArbitraryFloat(value, -1.0f);
                        parsedValue.asComplex.asFloat.px = 0.0f;
                    }
                    else
                    {
                        // 'dip:px' format
                        const auto& complexValue = value.split(':', QString::KeepEmptyParts);

                        parsedValue.asComplex.asFloat.dip = Utilities::parseArbitraryFloat(complexValue[0], 0);
                        parsedValue.asComplex.asFloat.px = Utilities::parseArbitraryFloat(complexValue[1], 0);
                    }
                }
                else
                {
                    assert(!value.contains(':'));
                    parsedValue.asSimple.asFloat = Utilities::parseArbitraryFloat(value, -1.0f);
                }
            }
            break;
        case MapStyleValueDataType::String:
            parsedValue.asSimple.asUInt = owner->_d->lookupStringId(value);
            break;
        case MapStyleValueDataType::Color:
            {
                assert(value[0] == '#');
                parsedValue.asSimple.asUInt = value.mid(1).toUInt(nullptr, 16);
                if(value.size() <= 7)
                    parsedValue.asSimple.asUInt |= 0xFF000000;
            }
            break;
        }
        
        _d->_values.insert(valueDef, parsedValue);
    }
}

OsmAnd::MapStyleRule::~MapStyleRule()
{
}

bool OsmAnd::MapStyleRule::getAttribute(const std::shared_ptr<const MapStyleValueDefinition>& key, MapStyleValue& value) const
{
    auto itValue = _d->_values.constFind(key);
    if(itValue == _d->_values.cend())
        return false;

    value = *itValue;
    return true;
}

void OsmAnd::MapStyleRule::dump( const QString& prefix /*= QString()*/ ) const
{
    auto newPrefix = prefix + QLatin1String("\t");
    
    for(const auto& valueEntry : rangeOf(constOf(_d->_values)))
    {
        const auto& valueDef = valueEntry.key();
        const auto& value = valueEntry.value();

        QString strValue;
        switch (valueDef->dataType)
        {
        case MapStyleValueDataType::Boolean:
            strValue = (value.asSimple.asInt == 1) ? QLatin1String("true") : QLatin1String("false");
            break;
        case MapStyleValueDataType::Integer:
            if(value.isComplex)
                strValue = QString::fromLatin1("%1:%2").arg(value.asComplex.asInt.dip).arg(value.asComplex.asInt.px);
            else
                strValue = QString::fromLatin1("%1").arg(value.asSimple.asInt);
            break;
        case MapStyleValueDataType::Float:
            if(value.isComplex)
                strValue = QString::fromLatin1("%1:%2").arg(value.asComplex.asFloat.dip).arg(value.asComplex.asFloat.px);
            else
                strValue = QString::fromLatin1("%1").arg(value.asSimple.asFloat);
            break;
        case MapStyleValueDataType::String:
            strValue = owner->_d->lookupStringValue(value.asSimple.asUInt);
            break;
        case MapStyleValueDataType::Color:
            {
                auto color = value.asSimple.asUInt;
                if((color & 0xFF000000) == 0xFF000000)
                    strValue = '#' + QString::number(color, 16).right(6);
                else
                    strValue = '#' + QString::number(color, 16).right(8);
            }
            break;
        }

        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s%s%s = %s",
            qPrintable(newPrefix),
            (valueDef->valueClass == MapStyleValueClass::Input) ? ">" : "<",
            qPrintable(valueDef->name),
            qPrintable(strValue));
    }

    if(!_d->_ifChildren.empty())
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sIf(",
            qPrintable(newPrefix));
        for(const auto& child : constOf(_d->_ifChildren))
        {
            OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sAND",
                qPrintable(newPrefix));
            child->dump(newPrefix);
        }
        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s)",
            qPrintable(newPrefix));
    }

    if(!_d->_ifElseChildren.empty())
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sSelector: [",
            qPrintable(newPrefix));
        for(const auto& child : constOf(_d->_ifElseChildren))
        {
            OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sOR",
                qPrintable(newPrefix));
            child->dump(newPrefix);
        }
        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s]",
            qPrintable(newPrefix));
    }
}
