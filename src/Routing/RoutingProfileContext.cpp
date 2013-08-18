#include "RoutingProfileContext.h"

#include "ObfRoutingSectionInfo.h"
#include "Road.h"

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

OsmAnd::Model::RoadDirection OsmAnd::RoutingProfileContext::getDirection( const std::shared_ptr<const OsmAnd::Model::Road>& road )
{
    auto value = getRulesetContext(RoutingRuleset::OneWay)->evaluateAsInteger(road, 0);
    return static_cast<Model::RoadDirection>(value);
}

bool OsmAnd::RoutingProfileContext::acceptsRoad( const std::shared_ptr<const OsmAnd::Model::Road>& road )
{
    auto value = getRulesetContext(RoutingRuleset::Access)->evaluateAsInteger(road, 0);
    return value >= 0;
}

bool OsmAnd::RoutingProfileContext::acceptsBorderLinePoint( const std::shared_ptr<const ObfRoutingSectionInfo>& section, const std::shared_ptr<const ObfRoutingBorderLinePoint>& point )
{
    auto value = getRulesetContext(RoutingRuleset::Access)->evaluateAsInteger(section, point->types, 0);
    return value >= 0;
}

float OsmAnd::RoutingProfileContext::getSpeedPriority( const std::shared_ptr<const OsmAnd::Model::Road>& road )
{
    auto value = getRulesetContext(RoutingRuleset::RoadPriorities)->evaluateAsFloat(road, 1.0f);
    return value;
}

float OsmAnd::RoutingProfileContext::getSpeed( const std::shared_ptr<const OsmAnd::Model::Road>& road )
{
    auto value = getRulesetContext(RoutingRuleset::RoadSpeed)->evaluateAsFloat(road, profile->minDefaultSpeed);
    return value;
}

float OsmAnd::RoutingProfileContext::getObstaclesExtraTime( const std::shared_ptr<const OsmAnd::Model::Road>& road, uint32_t pointIndex )
{
    auto itPointTypes = road->pointsTypes.find(pointIndex);
    if(itPointTypes == road->pointsTypes.end())
        return 0.0f;

    auto value = getRulesetContext(RoutingRuleset::Obstacles)->evaluateAsFloat(road->subsection->section, *itPointTypes, 0.0f);
    return value;
}

float OsmAnd::RoutingProfileContext::getRoutingObstaclesExtraTime( const std::shared_ptr<const OsmAnd::Model::Road>& road, uint32_t pointIndex )
{
    auto itPointTypes = road->pointsTypes.find(pointIndex);
    if(itPointTypes == road->pointsTypes.end())
        return 0.0f;

    auto value = getRulesetContext(RoutingRuleset::RoutingObstacles)->evaluateAsFloat(road->subsection->section, *itPointTypes, 0.0f);
    return value;
}
