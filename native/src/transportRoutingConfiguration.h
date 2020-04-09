#ifndef _OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
#define _OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
#include "CommonCollections.h"
#include "generalRouter.h"
#include <algorithm>

struct TransportRoutingConfiguration {
    
    const string KEY = "public_transport";
    int32_t zoomToLoadTiles = 15;
    int32_t walkRadius = 1500;
    int32_t walkChangeRadius = 300;
    int32_t maxNumberOfChanges = 3;
    int32_t finishTimeSeconds = 1200;
    int32_t maxRouteTime = 60 * 60 * 10;
    SHARED_PTR<GeneralRouter> router;
    
    float walkSpeed = (3.6 / 3.6);
    float defaultTravelSpeed = (60 / 3.6);
    int32_t stopTime = 30;
    int32_t changeTime = 180;
    int32_t boardingTime = 180;
    
    bool useSchedule = false;

    int32_t scheduleTimeOfDay = 12 * 60 * 6;
    int32_t scheduleMaxTime = 50 * 6;
//    int32_t scheduleDayNumber; Unused
    MAP_STR_FLOAT speed;

    TransportRoutingConfiguration() : router(new GeneralRouter()) {}
    
    TransportRoutingConfiguration(SHARED_PTR<GeneralRouter> prouter, MAP_STR_STR params) {
        if(prouter != nullptr) {
            this->router = prouter->build(params);
            walkRadius =  router->getIntAttribute("walkRadius", walkRadius);
            walkChangeRadius =  router->getIntAttribute("walkChangeRadius", walkChangeRadius);
            zoomToLoadTiles =  router->getIntAttribute("zoomToLoadTiles", zoomToLoadTiles);
            maxNumberOfChanges =  router->getIntAttribute("maxNumberOfChanges", maxNumberOfChanges);
            maxRouteTime =  router->getIntAttribute("maxRouteTime", maxRouteTime);
            finishTimeSeconds =  router->getIntAttribute("delayForAlternativesRoutes", finishTimeSeconds);
            string mn = router->getAttribute("max_num_changes");
            int maxNumOfChanges = 3;
            try
            {
                maxNumOfChanges = stoi(mn);
            } catch (...)
            {
                // Ignore
            }
            maxNumberOfChanges = maxNumOfChanges;
            
            walkSpeed = router->getFloatAttribute("minDefaultSpeed", walkSpeed * 3.6f) / 3.6f;
            defaultTravelSpeed = router->getFloatAttribute("maxDefaultSpeed", defaultTravelSpeed * 3.6f) / 3.6f;
            
            RouteAttributeContext& obstacles = router->getObjContext(RouteDataObjectAttribute::ROUTING_OBSTACLES);
            dynbitset bs = getRawBitset("time", "stop");
            stopTime = obstacles.evaluateInt(bs, stopTime);
            bs = getRawBitset("time", "change");
            changeTime = obstacles.evaluateInt(bs, changeTime);
            bs = getRawBitset("time", "boarding");
            boardingTime = obstacles.evaluateInt(bs, boardingTime);
            
            RouteAttributeContext& spds = router->getObjContext(RouteDataObjectAttribute::ROAD_SPEED);
            bs = getRawBitset("route", "walk");
            walkSpeed = spds.evaluateFloat(bs, walkSpeed);
        }
    }

    float getSpeedByRouteType(std::string routeType)
    {
        float sl = speed[routeType];
        if (speed.find(routeType) != speed.end())
        {
            std::string routeStr = std::string("route");
            dynbitset bs = getRawBitset(routeStr, routeType);
            sl = router->getObjContext(RouteDataObjectAttribute::ROAD_SPEED).evaluateFloat(bs, defaultTravelSpeed);
        }
        return sl;
    }
    
    dynbitset getRawBitset(std::string tg, std::string vl)
    {
        dynbitset bs;
        std::string key = tg + "$" + vl;
        router->registerTagValueAttribute(tag_value(tg, vl), bs);
        return bs;
    }
    
    int32_t getChangeTime() {
        return useSchedule ? 0 : changeTime;
    };

    int32_t getBoardingTime() {
        return boardingTime;
    };
};

#endif //_OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
