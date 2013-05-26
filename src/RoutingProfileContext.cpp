#include "RoutingProfileContext.h"

OsmAnd::RoutingProfileContext::RoutingProfileContext( const std::shared_ptr<RoutingProfile>& profile, QHash<QString, QString>* contextValues /*= nullptr*/ )
    : profile(profile)
{
    for(auto type = 0; type < RoutingRuleset::TypesCount; type++)
    {
        auto rulesetType = static_cast<RoutingRuleset::Type>(type);
        auto ruleset = profile->getRuleset(rulesetType);

        _rulesetContexts[type].reset(new OsmAnd::RoutingRulesetContext(this, ruleset, contextValues));
    }
}

OsmAnd::RoutingProfileContext::~RoutingProfileContext()
{
}

std::shared_ptr<OsmAnd::RoutingRulesetContext> OsmAnd::RoutingProfileContext::getRulesetContext( RoutingRuleset::Type type )
{
    return _rulesetContexts[static_cast<int>(type)];
}

OsmAnd::Model::Road::Direction OsmAnd::RoutingProfileContext::getDirection( Model::Road* road )
{
    auto value = getRulesetContext(RoutingRuleset::OneWay)->evaluateAsInteger(road, 0);
    return static_cast<Model::Road::Direction>(value);
}

bool OsmAnd::RoutingProfileContext::acceptsRoad( Model::Road* road )
{
    auto value = getRulesetContext(RoutingRuleset::Access)->evaluateAsInteger(road, 0);
    return value >= 0;
}

bool OsmAnd::RoutingProfileContext::acceptsBorderLinePoint( ObfRoutingSection* section, ObfRoutingSection::BorderLinePoint* point )
{
    auto value = getRulesetContext(RoutingRuleset::Access)->evaluateAsInteger(section, point->types, 0);
    return value >= 0;
}

float OsmAnd::RoutingProfileContext::getSpeedPriority( Model::Road* road )
{
    auto value = getRulesetContext(RoutingRuleset::RoadPriorities)->evaluateAsFloat(road, 1.0f);
    return value;
}

float OsmAnd::RoutingProfileContext::getSpeed( Model::Road* road )
{
    auto value = getRulesetContext(RoutingRuleset::RoadSpeed)->evaluateAsFloat(road, profile->minDefaultSpeed);
    return value;
}

float OsmAnd::RoutingProfileContext::getObstaclesExtraTime( Model::Road* road, uint32_t pointIndex )
{
    auto itPointTypes = road->pointsTypes.find(pointIndex);
    if(itPointTypes == road->pointsTypes.end())
        return 0.0f;

    auto value = getRulesetContext(RoutingRuleset::Obstacles)->evaluateAsFloat(road->subsection->section.get(), *itPointTypes, 0.0f);
    return value;
}

float OsmAnd::RoutingProfileContext::getRoutingObstaclesExtraTime( Model::Road* road, uint32_t pointIndex )
{
    auto itPointTypes = road->pointsTypes.find(pointIndex);
    if(itPointTypes == road->pointsTypes.end())
        return 0.0f;

    auto value = getRulesetContext(RoutingRuleset::RoutingObstacles)->evaluateAsFloat(road->subsection->section.get(), *itPointTypes, 0.0f);
    return value;
}
