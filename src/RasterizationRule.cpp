#include "RasterizationRule.h"

#include <cassert>

#include "Logging.h"
#include "Utilities.h"

OsmAnd::RasterizationRule::RasterizationRule(RasterizationStyle* owner_, const QHash< QString, QString >& attributes)
    : owner(owner_)
{
    //TODO:storage->childRules.push_back(this);
    _valueDefinitionsRefs.reserve(attributes.size());
    _values.reserve(attributes.size());
    
    for(auto itAttribute = attributes.begin(); itAttribute != attributes.end(); ++itAttribute)
    {
        const auto& key = itAttribute.key();
        const auto& value = itAttribute.value();

        std::shared_ptr<RasterizationStyle::ValueDefinition> valueDef;
        bool ok = owner->resolveValueDefinition(key, valueDef);
        assert(ok);

        _valueDefinitionsRefs.push_back(valueDef);
        Value parsedValue;
        switch (valueDef->dataType)
        {
        case RasterizationStyle::ValueDefinition::Boolean:
            parsedValue.asInt = (value == "true") ? 1 : 0;
            break;
        case RasterizationStyle::ValueDefinition::Integer:
            parsedValue.asInt = Utilities::parseArbitraryInt(value, -1);
            break;
        case RasterizationStyle::ValueDefinition::Float:
            parsedValue.asFloat = Utilities::parseArbitraryFloat(value, -1.0f);
            break;
        case RasterizationStyle::ValueDefinition::String:
            parsedValue.asUInt = owner->lookupStringId(value);
            break;
        case RasterizationStyle::ValueDefinition::Color:
            {
                parsedValue.asUInt = value.mid(1).toUInt(nullptr, 16);
                if(value.size() <= 7)
                    parsedValue.asUInt |= 0xFF000000;
            }
            break;
        }
        
        _values.insert(key, parsedValue);
    }
}

OsmAnd::RasterizationRule::~RasterizationRule()
{
}

void OsmAnd::RasterizationRule::dump( const QString& prefix /*= QString()*/ ) const
{
    auto newPrefix = prefix + "\t";
    
    for(auto itValueDef = _valueDefinitionsRefs.begin(); itValueDef != _valueDefinitionsRefs.end(); ++itValueDef)
    {
        auto valueDef = *itValueDef;

        QString strValue;
        switch (valueDef->type)
        {
        case RasterizationStyle::ValueDefinition::Boolean:
            strValue = (getIntegerAttribute(valueDef->name) == 1) ? "true" : "false";
            break;
        case RasterizationStyle::ValueDefinition::Integer:
            strValue = QString("%1").arg(getIntegerAttribute(valueDef->name));
            break;
        case RasterizationStyle::ValueDefinition::Float:
            strValue = QString("%1").arg(getFloatAttribute(valueDef->name));
            break;
        case RasterizationStyle::ValueDefinition::String:
            strValue = getStringAttribute(valueDef->name);
            break;
        case RasterizationStyle::ValueDefinition::Color:
            {
                auto color = getIntegerAttribute(valueDef->name);
                if((color & 0xFF000000) != 0)
                    strValue = '#' + QString::number(color, 16).mid(2);
                else
                    strValue = '#' + QString::number(color, 16);
            }
            break;
        }

        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s%s = %s\n", newPrefix.toStdString().c_str(), valueDef->name.toStdString().c_str(), strValue.toStdString().c_str());
    }
        
    for(auto itChild = _ifElseChildren.begin(); itChild != _ifElseChildren.end(); ++itChild)
    {
        auto child = *itChild;

        child->dump(newPrefix);
    }
}

const QString& OsmAnd::RasterizationRule::getStringAttribute( const QString& key, bool* ok /*= nullptr*/ ) const
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
    return owner->lookupStringValue(itValue->asUInt);
}

int OsmAnd::RasterizationRule::getIntegerAttribute( const QString& key, bool* ok /*= nullptr*/ ) const
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

float OsmAnd::RasterizationRule::getFloatAttribute( const QString& key, bool* ok /*= nullptr*/ ) const
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
