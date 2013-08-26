#include "MapStyleRule.h"

#include <cassert>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "MapStyleValueDefinition.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::MapStyleRule::MapStyleRule(MapStyle* owner_, const QHash< QString, QString >& attributes)
    : owner(owner_)
{
    _valueDefinitionsRefs.reserve(attributes.size());
    _values.reserve(attributes.size());
    
    for(auto itAttribute = attributes.begin(); itAttribute != attributes.end(); ++itAttribute)
    {
        const auto& key = itAttribute.key();
        const auto& value = itAttribute.value();

        std::shared_ptr<const MapStyleValueDefinition> valueDef;
        bool ok = owner->resolveValueDefinition(key, valueDef);
        assert(ok);

        _valueDefinitionsRefs.push_back(valueDef);
        MapStyleValue parsedValue;
        switch (valueDef->dataType)
        {
        case MapStyleValueDataType::Boolean:
            parsedValue.asInt = (value == "true") ? 1 : 0;
            break;
        case MapStyleValueDataType::Integer:
            parsedValue.asInt = Utilities::parseArbitraryInt(value, -1);
            break;
        case MapStyleValueDataType::Float:
            parsedValue.asFloat = Utilities::parseArbitraryFloat(value, -1.0f);
            break;
        case MapStyleValueDataType::String:
            parsedValue.asUInt = owner->_d->lookupStringId(value);
            break;
        case MapStyleValueDataType::Color:
            {
                assert(value[0] == '#');
                parsedValue.asUInt = value.mid(1).toUInt(nullptr, 16);
                if(value.size() <= 7)
                    parsedValue.asUInt |= 0xFF000000;
            }
            break;
        }
        
        _values.insert(key, parsedValue);
    }
}

OsmAnd::MapStyleRule::~MapStyleRule()
{
}

void OsmAnd::MapStyleRule::dump( const QString& prefix /*= QString()*/ ) const
{
    auto newPrefix = prefix + "\t";
    
    for(auto itValueDef = _valueDefinitionsRefs.begin(); itValueDef != _valueDefinitionsRefs.end(); ++itValueDef)
    {
        auto valueDef = *itValueDef;

        QString strValue;
        switch (valueDef->dataType)
        {
        case MapStyleValueDataType::Boolean:
            strValue = (getIntegerAttribute(valueDef->name) == 1) ? "true" : "false";
            break;
        case MapStyleValueDataType::Integer:
            strValue = QString("%1").arg(getIntegerAttribute(valueDef->name));
            break;
        case MapStyleValueDataType::Float:
            strValue = QString("%1").arg(getFloatAttribute(valueDef->name));
            break;
        case MapStyleValueDataType::String:
            strValue = getStringAttribute(valueDef->name);
            break;
        case MapStyleValueDataType::Color:
            {
                auto color = getIntegerAttribute(valueDef->name);
                if((color & 0xFF000000) == 0xFF000000)
                    strValue = '#' + QString::number(color, 16).right(6);
                else
                    strValue = '#' + QString::number(color, 16).right(8);
            }
            break;
        }

        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s%s%s = %s",
            newPrefix.toStdString().c_str(),
            (valueDef->valueClass == MapStyleValueClass::Input) ? ">" : "<",
            valueDef->name.toStdString().c_str(),
            strValue.toStdString().c_str());
    }

    if(!_ifChildren.isEmpty())
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sIf(",
            newPrefix.toStdString().c_str());
        for(auto itChild = _ifChildren.begin(); itChild != _ifChildren.end(); ++itChild)
        {
            auto child = *itChild;

            OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sAND",
                newPrefix.toStdString().c_str());
            child->dump(newPrefix);
        }
        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s)",
            newPrefix.toStdString().c_str());
    }

    if(!_ifElseChildren.isEmpty())
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sSelector: [",
            newPrefix.toStdString().c_str());
        for(auto itChild = _ifElseChildren.begin(); itChild != _ifElseChildren.end(); ++itChild)
        {
            auto child = *itChild;

            OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sOR",
                newPrefix.toStdString().c_str());
            child->dump(newPrefix);
        }
        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s]",
            newPrefix.toStdString().c_str());
    }
}

const QString& OsmAnd::MapStyleRule::getStringAttribute( const QString& key, bool* ok /*= nullptr*/ ) const
{
    auto itValue = _values.find(key);
    if(itValue == _values.end())
    {
        if(ok)
            *ok = false;

        static QString stub;
        return stub;
    }

    if(ok)
        *ok = true;
    return owner->_d->lookupStringValue(itValue->asUInt);
}

int OsmAnd::MapStyleRule::getIntegerAttribute( const QString& key, bool* ok /*= nullptr*/ ) const
{
    auto itValue = _values.find(key);
    if(itValue == _values.end())
    {
        if(ok)
            *ok = false;
        return -1;
    }

    if(ok)
        *ok = true;
    return itValue->asInt;
}

float OsmAnd::MapStyleRule::getFloatAttribute( const QString& key, bool* ok /*= nullptr*/ ) const
{
    auto itValue = _values.find(key);
    if(itValue == _values.end())
    {
        if(ok)
            *ok = false;
        return -1;
    }

    if(ok)
        *ok = true;
    return itValue->asFloat;
}
