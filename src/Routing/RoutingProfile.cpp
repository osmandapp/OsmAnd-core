#include "RoutingProfile.h"

#include <cassert>

#include "RoutingConfiguration.h"
#include "Utilities.h"

OsmAnd::RoutingProfile::RoutingProfile()
    : _restrictionsAware(true)
    , _oneWayAware(true)
    , _followSpeedLimitations(true)
    , _minDefaultSpeed(10)
    , _maxDefaultSpeed(10)
    , name(_name)
    , attributes(_attributes)
    , parameters(_parameters)
    , restrictionsAware(_restrictionsAware)
    , leftTurn(_leftTurn)
    , roundaboutTurn(_roundaboutTurn)
    , rightTurn(_rightTurn)
    , minDefaultSpeed(_minDefaultSpeed)
    , maxDefaultSpeed(_maxDefaultSpeed)
{
    for(auto type = 0; type < RoutingRuleset::TypesCount; type++)
        _rulesets[type].reset(new OsmAnd::RoutingRuleset(this, static_cast<RoutingRuleset::Type>(type)));
}

OsmAnd::RoutingProfile::~RoutingProfile()
{
}

void OsmAnd::RoutingProfile::addAttribute( const QString& key, const QString& value )
{
    _attributes.insert(key, value);

    if(key == "restrictionsAware")
    {
        _restrictionsAware = Utilities::parseArbitraryBool(value, _restrictionsAware);
    }
    else if(key == "leftTurn")
    {
        bool ok;
        auto parsed = value.toFloat(&ok);
        if(ok)
            _leftTurn = parsed;
    }
    else if(key == "rightTurn")
    {
        bool ok;
        auto parsed = value.toFloat(&ok);
        if(ok)
            _rightTurn = parsed;
    }
    else if(key == "roundaboutTurn")
    {
        bool ok;
        auto parsed = value.toFloat(&ok);
        if(ok)
            _roundaboutTurn = parsed;
    }
    else if(key == "minDefaultSpeed")
    {
        bool ok;
        auto parsed = value.toFloat(&ok);
        if(ok)
            _minDefaultSpeed = parsed / 3.6f;
    }
    else if(key == "maxDefaultSpeed")
    {
        bool ok;
        auto parsed = value.toFloat(&ok);
        if(ok)
            _maxDefaultSpeed = parsed / 3.6f;
    }
}

void OsmAnd::RoutingProfile::registerBooleanParameter( const QString& id, const QString& name, const QString& description )
{
    std::shared_ptr<Parameter> parameter(new Parameter());
    parameter->_id = id;
    parameter->_type = Parameter::Boolean;
    parameter->_name = name;
    parameter->_description = description;
    _parameters.insert(id, parameter);
}

void OsmAnd::RoutingProfile::registerNumericParameter( const QString& id, const QString& name, const QString& description, QList<double>& values, const QStringList& valuesDescriptions )
{
    std::shared_ptr<Parameter> parameter(new Parameter());
    parameter->_id = id;
    parameter->_type = Parameter::Numeric;
    parameter->_name = name;
    parameter->_description = description;
    parameter->_possibleValues = values;
    parameter->_possibleValueDescriptions = valuesDescriptions;
    _parameters.insert(id, parameter);
}

std::shared_ptr<OsmAnd::RoutingRuleset> OsmAnd::RoutingProfile::getRuleset( RoutingRuleset::Type type ) const
{
    return _rulesets[static_cast<int>(type)];
}

uint32_t OsmAnd::RoutingProfile::registerTagValueAttribute( const QString& tag, const QString& value )
{
    auto key = tag + "$" + value;

    auto itId = _universalRules.find(key);
    if(itId != _universalRules.cend())
        return *itId;
    
    auto id = _universalRules.size();
    _universalRulesKeysById.push_back(key);
    _universalRules.insert(key, id);

    auto itTagRuleMask = _tagRuleMask.find(tag);
    if(itTagRuleMask == _tagRuleMask.cend())
        itTagRuleMask = _tagRuleMask.insert(tag, QBitArray());
    
    if(itTagRuleMask->size() <= id)
    {
        assert(id < std::numeric_limits<int>::max());
        itTagRuleMask->resize(id + 1);
    }
    itTagRuleMask->setBit(id);
    
    return id;
}

bool OsmAnd::RoutingProfile::parseTypedValueFromTag( uint32_t id, const QString& type, float& parsedValue )
{
    bool ok = true;

    auto itCachedValue = _ruleToValueCache.find(id);
    if(itCachedValue == _ruleToValueCache.cend())
    {
        const auto& key = _universalRulesKeysById[id];
        const auto& valueName = key.mid(key.indexOf('$') + 1);

        ok = RoutingConfiguration::parseTypedValue(valueName, type, parsedValue);
        if(ok)
            itCachedValue = _ruleToValueCache.insert(id, parsedValue);
    }

    if(ok)
        parsedValue = *itCachedValue;
    return ok;
}

OsmAnd::RoutingProfile::Parameter::Parameter()
    : id(_id)
    , name(_name)
    , description(_description)
    , type(_type)
    , possibleValues(_possibleValues)
    , possibleValueDescriptions(_possibleValueDescriptions)
{
}

OsmAnd::RoutingProfile::Parameter::~Parameter()
{
}
