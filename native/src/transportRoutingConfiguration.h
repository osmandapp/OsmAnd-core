#ifndef _OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
#define _OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "generalRouter.h"
#include <algorithm>

struct TransportRoutingConfiguration {
    
    const string KEY = "public_transport";
    int32_t zoomToLoadTiles;
    int32_t walkRadius;
    int32_t walkChangeRadius;
    int32_t maxNumberOfChanges;
    int32_t finishTimeSeconds;
    int32_t maxRouteTime = 60 * 60 * 10;
    SHARED_PTR<GeneralRouter> router;
    
    float walkSpeed;
    float defaultTravelSpeed;
    int32_t stopTime;
    int32_t changeTime;
    int32_t boardingTime;
    
    bool useSchedule;

    int32_t scheduleTimeOfDay;
    int32_t scheduleMaxTime;
    int32_t scheduleDayNumber;

    MAP_STR_INT rawTypes;
    MAP_STR_FLOAT speed;

    float getSpeedByRouteType(std::string routeType)
    {
        float sl = speed[routeType];
        if (speed.find(routeType) != speed.end())
        {
            std::string routeStr = std::string("route");
            dynbitset bs = getRawBitset(routeStr, routeType);
            sl = router->getObjContext(RouteDataObjectAttribute::ROAD_SPEED).evaluateFloat(bs, defaultTravelSpeed);
        }
    };
    
    dynbitset getRawBitset(std::string& tg, std::string& vl)
    {
        dynbitset bs;
        int rawType = getRawType(tg, vl);
        bs.set(rawType);
        return bs;
    }
    
    int32_t getRawType(std::string& tg, std::string& vl)
    {
        std::string key = tg.append("$").append(vl);
        if(rawTypes.find(key) == rawTypes.end())
        {
            uint at = router->registerTagValueAttribute(tag_value(tg, vl));
            rawTypes.insert(std::pair<string, int>(key, at));
        }
        return rawTypes[key];
    }
    
    int32_t getChangeTime() {
        return useSchedule ? 0 : changeTime;
    };

    int32_t getBoardingTime() {
        return boardingTime;
    };
};

class TransportRoutingConfigurationBuilder {

    SHARED_PTR<GeneralRouter> router;
    MAP_STR_STR attributes;
    
    SHARED_PTR<TransportRoutingConfiguration> build(SHARED_PTR<GeneralRouter> router, const MAP_STR_STR& params = MAP_STR_STR()) {
        SHARED_PTR<TransportRoutingConfiguration> i = std::make_shared<TransportRoutingConfiguration>();
        i->router = router->build(params);
        i->walkRadius = i->router->getIntAttribute("walkRadius", 1500.f);
        i->walkChangeRadius = i->router->getIntAttribute("walkChangeRadius", 300.f);
        i->zoomToLoadTiles = i->router->getIntAttribute("zoomToLoadTiles", 15.f);
        i->finishTimeSeconds = i->router->getIntAttribute("delayForAlternativesRoutes", 1200.f);
        i->walkSpeed = i->router->getIntAttribute("minDefaultSpeed", 3.6f) / 3.6f;
        i->maxRouteTime = i->router->getIntAttribute("maxRouteTime", 60 * 60 * 10);
        i->defaultTravelSpeed = i->router->getIntAttribute("maxDefaultSpeed", 60) / 3.6f;
        string mn = i->router->getAttribute("max_num_changes");
        int maxNumOfChanges = 3;
        try
        {
            maxNumOfChanges = stoi(mn);
        } catch (...)
        {
            // Ignore
        }
        i->maxNumberOfChanges = maxNumOfChanges;
        
        RouteAttributeContext& obstacles = i->router->getObjContext(RouteDataObjectAttribute::ROUTING_OBSTACLES);
        string tag("time");
        string value("stop");
        i->stopTime = obstacles.evaluateInt(i->getRawBitset(tag, value), 30);
        value = "change";
        i->changeTime = obstacles.evaluateInt(i->getRawBitset(tag, value), 180);
        value = "boarding";
        i->boardingTime = obstacles.evaluateInt(i->getRawBitset(tag, value), 180);
        
        tag = "route";
        value = "walk";
        RouteAttributeContext& spds = i->router->getObjContext(RouteDataObjectAttribute::ROAD_SPEED);
        dynbitset bs = i->getRawBitset(tag, value);
        i->walkSpeed = spds.evaluateFloat(bs, 3.6f) / 3.6f;

        return i;
    }
};


#endif //_OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
