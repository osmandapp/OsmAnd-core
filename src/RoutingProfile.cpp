#include "RoutingProfile.h"

OsmAnd::RoutingProfile::RoutingProfile()
    : _restrictionsAware(true)
    , _minDefaultSpeed(10)
    , _maxDefaultSpeed(10)
{
    /*objectAttributes = new RouteAttributeContext[RouteDataObjectAttribute.values().length];
    for (int i = 0; i < objectAttributes.length; i++) {
        objectAttributes[i] = new RouteAttributeContext();
    }
    universalRules = new LinkedHashMap<String, Integer>();
    universalRulesById = new ArrayList<String>();
    tagRuleMask = new LinkedHashMap<String, BitSet>();
    ruleToValue = new ArrayList<Object>();
    parameters = new LinkedHashMap<String, GeneralRouter.RoutingParameter>();*/
}

OsmAnd::RoutingProfile::~RoutingProfile()
{
}

void OsmAnd::RoutingProfile::addAttribute( const QString& key, const QString& value )
{
    _attributes.insert(key, value);

    if(key == "restrictionsAware")
    {
        _restrictionsAware = (value.compare("true", Qt::CaseInsensitive) == 0);
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

void OsmAnd::RoutingProfile::registerNumericParameter( const QString& id, const QString& name, const QString& description, QList<double>& values, QList<QString> valuesDescriptions )
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

std::shared_ptr<OsmAnd::RoutingRulesetContext> OsmAnd::RoutingProfile::getRulesetContext( RulesetType type )
{
    auto context = _rulesetContexts[static_cast<int>(type)];
    if(!context)
    {
        _rulesetContexts[static_cast<int>(type)].reset(new RoutingRulesetContext());
        context = _rulesetContexts[static_cast<int>(type)];
    }
    return context;
}
