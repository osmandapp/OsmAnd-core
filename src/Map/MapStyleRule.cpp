#include "MapStyleRule.h"
#include "MapStyleRule_P.h"

#include <cassert>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleValue.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::MapStyleRule::MapStyleRule(MapStyle* owner_, const QHash< QString, QString >& attributes)
    : _d(new MapStyleRule_P(this))
    , owner(owner_)
{
    _d->_valuesByRef.reserve(attributes.size());
    _d->_valuesByName.reserve(attributes.size());
    
    for(auto itAttribute = attributes.cbegin(); itAttribute != attributes.cend(); ++itAttribute)
    {
        const auto& key = itAttribute.key();
        const auto& value = itAttribute.value();

        std::shared_ptr<const MapStyleValueDefinition> valueDef;
        bool ok = owner->resolveValueDefinition(key, valueDef);
        assert(ok);

        std::shared_ptr<MapStyleValue> parsedValue(new MapStyleValue());
        switch (valueDef->dataType)
        {
        case MapStyleValueDataType::Boolean:
            parsedValue->asSimple.asInt = (value == QLatin1String("true")) ? 1 : 0;
            break;
        case MapStyleValueDataType::Integer:
            {
                if(valueDef->isComplex)
                {
                    parsedValue->isComplex = true;
                    if(!value.contains(':'))
                    {
                        parsedValue->asComplex.asInt.dip = Utilities::parseArbitraryInt(value, -1);
                        parsedValue->asComplex.asInt.px = 0.0;
                    }
                    else
                    {
                        // 'dip:px' format
                        const auto& complexValue = value.split(':', QString::KeepEmptyParts);

                        parsedValue->asComplex.asInt.dip = Utilities::parseArbitraryInt(complexValue[0], 0);
                        parsedValue->asComplex.asInt.px = Utilities::parseArbitraryInt(complexValue[1], 0);
                    }
                }
                else
                {
                    assert(!value.contains(':'));
                    parsedValue->asSimple.asInt = Utilities::parseArbitraryInt(value, -1);
                }
            }
            break;
        case MapStyleValueDataType::Float:
            {
                if(valueDef->isComplex)
                {
                    parsedValue->isComplex = true;
                    if(!value.contains(':'))
                    {
                        parsedValue->asComplex.asFloat.dip = Utilities::parseArbitraryFloat(value, -1.0f);
                        parsedValue->asComplex.asFloat.px = 0.0f;
                    }
                    else
                    {
                        // 'dip:px' format
                        const auto& complexValue = value.split(':', QString::KeepEmptyParts);

                        parsedValue->asComplex.asFloat.dip = Utilities::parseArbitraryFloat(complexValue[0], 0);
                        parsedValue->asComplex.asFloat.px = Utilities::parseArbitraryFloat(complexValue[1], 0);
                    }
                }
                else
                {
                    assert(!value.contains(':'));
                    parsedValue->asSimple.asFloat = Utilities::parseArbitraryFloat(value, -1.0f);
                }
            }
            break;
        case MapStyleValueDataType::String:
            parsedValue->asSimple.asUInt = owner->_d->lookupStringId(value);
            break;
        case MapStyleValueDataType::Color:
            {
                assert(value[0] == '#');
                parsedValue->asSimple.asUInt = value.mid(1).toUInt(nullptr, 16);
                if(value.size() <= 7)
                    parsedValue->asSimple.asUInt |= 0xFF000000;
            }
            break;
        }
        
        _d->_valuesByRef.insert(valueDef, parsedValue);
        _d->_valuesByName.insert(key, parsedValue);
    }
}

OsmAnd::MapStyleRule::~MapStyleRule()
{
}

bool OsmAnd::MapStyleRule::getAttribute(const QString& key, std::shared_ptr<const MapStyleValue>& value) const
{
    auto itValue = _d->_valuesByName.constFind(key);
    if(itValue == _d->_valuesByName.cend())
        return false;

    value = *itValue;
    return true;
}

void OsmAnd::MapStyleRule::dump( const QString& prefix /*= QString()*/ ) const
{
    auto newPrefix = prefix + QLatin1String("\t");
    
    for(auto itValueEntry = _d->_valuesByRef.cbegin(); itValueEntry != _d->_valuesByRef.cend(); ++itValueEntry)
    {
        const auto& valueDef = itValueEntry.key();
        const auto& value = *itValueEntry.value();

        QString strValue;
        switch (valueDef->dataType)
        {
        case MapStyleValueDataType::Boolean:
            strValue = (value.asSimple.asInt == 1) ? "true" : "false";
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
        for(auto itChild = _d->_ifChildren.cbegin(); itChild != _d->_ifChildren.cend(); ++itChild)
        {
            auto child = *itChild;

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
        for(auto itChild = _d->_ifElseChildren.cbegin(); itChild != _d->_ifElseChildren.cend(); ++itChild)
        {
            auto child = *itChild;

            OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sOR",
                qPrintable(newPrefix));
            child->dump(newPrefix);
        }
        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s]",
            qPrintable(newPrefix));
    }
}
