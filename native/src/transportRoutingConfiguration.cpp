#ifndef _OSMAND_TRANSPORT_ROUTE_CONFIGUERATION_CPP
#define _OSMAND_TRANSPORT_ROUTE_CONFIGUERATION_CPP
#include "transportRoutingConfiguration.h"
#include "generalRouter.h"
#include <stdlib.h>


TransportRoutingConfiguration::TransportRoutingConfiguration() : router(new GeneralRouter()) {}

TransportRoutingConfiguration::TransportRoutingConfiguration(SHARED_PTR<GeneralRouter> prouter, MAP_STR_STR params)
{
	if (prouter != nullptr)
	{
		this->router = prouter->build(params);
		walkRadius = router->getIntAttribute("walkRadius", walkRadius);
		walkChangeRadius = router->getIntAttribute("walkChangeRadius", walkChangeRadius);
		zoomToLoadTiles = router->getIntAttribute("zoomToLoadTiles", zoomToLoadTiles);
		maxNumberOfChanges = router->getIntAttribute("maxNumberOfChanges", maxNumberOfChanges);
		maxRouteTime = router->getIntAttribute("maxRouteTime", maxRouteTime);
		finishTimeSeconds = router->getIntAttribute("delayForAlternativesRoutes", finishTimeSeconds);
		string mn = router->getAttribute("max_num_changes");
		int maxNumOfChanges = 3;
		try
		{
			maxNumOfChanges = atoi(mn.c_str());
		}
		catch (...)
		{
			// Ignore
		}
		maxNumberOfChanges = maxNumOfChanges;

		walkSpeed = router->getFloatAttribute("minDefaultSpeed", walkSpeed * 3.6f) / 3.6f;
		defaultTravelSpeed = router->getFloatAttribute("maxDefaultSpeed", defaultTravelSpeed * 3.6f) / 3.6f;

		RouteAttributeContext &obstacles = router->getObjContext(RouteDataObjectAttribute::ROUTING_OBSTACLES);
		dynbitset bs = getRawBitset("time", "stop");
		stopTime = obstacles.evaluateInt(bs, stopTime);
		bs = getRawBitset("time", "change");
		changeTime = obstacles.evaluateInt(bs, changeTime);
		bs = getRawBitset("time", "boarding");
		boardingTime = obstacles.evaluateInt(bs, boardingTime);

		RouteAttributeContext &spds = router->getObjContext(RouteDataObjectAttribute::ROAD_SPEED);
		bs = getRawBitset("route", "walk");
		walkSpeed = spds.evaluateFloat(bs, walkSpeed);
	}
}

float TransportRoutingConfiguration::getSpeedByRouteType(std::string routeType)
{
	const auto it = speed.find(routeType);
	float sl = defaultTravelSpeed;
	if (it == speed.end())
	{
		dynbitset bs = getRawBitset("route", routeType);
		sl = router->getObjContext(RouteDataObjectAttribute::ROAD_SPEED).evaluateFloat(bs, defaultTravelSpeed);
		speed[routeType] = sl;
	}
	else
	{
		sl = it->second;
	}
	return sl;
}

dynbitset TransportRoutingConfiguration::getRawBitset(std::string tg, std::string vl)
{
	uint id = getRawType(tg, vl);
	dynbitset bs(router->getBitSetSize());
	bs.set(id);
	return bs;
}

uint TransportRoutingConfiguration::getRawType(string &tg, string &vl)
{
	string key = tg + "$" + vl;
	if (rawTypes.find(key) == rawTypes.end())
	{
		uint at = router->registerTagValueAttribute(tag_value(tg, vl));
		rawTypes[key] = at;
	}
	return rawTypes[key];
}

int32_t TransportRoutingConfiguration::getChangeTime()
{
	return useSchedule ? 0 : changeTime;
};

int32_t TransportRoutingConfiguration::getBoardingTime()
{
	return boardingTime;
};

#endif