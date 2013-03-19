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
        //TODO:_restrictionsAware = parseSilentBoolean(v, _restrictionsAware);
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
