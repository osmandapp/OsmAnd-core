#ifndef _OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
#define _OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
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

    MAP_STR_INT rawTypes;
    MAP_STR_FLOAT speed;
    
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
        int rawType = getRawType(tg, vl);
        dynbitset bs(rawType);
        return bs;
    }
    
    int32_t getRawType(std::string tg, std::string vl)
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

//    SHARED_PTR<GeneralRouter> router;
//    MAP_STR_STR attributes;
//
//    SHARED_PTR<TransportRoutingConfiguration> build(SHARED_PTR<GeneralRouter> router, const MAP_STR_STR& params = MAP_STR_STR()) {
//        SHARED_PTR<TransportRoutingConfiguration> i = std::make_shared<TransportRoutingConfiguration>();
//        i->router = router->build(params);
//        i->walkRadius = i->router->getIntAttribute("walkRadius", 1500.f);
//        i->walkChangeRadius = i->router->getIntAttribute("walkChangeRadius", 300.f);
//        i->zoomToLoadTiles = i->router->getIntAttribute("zoomToLoadTiles", 15.f);
//        i->finishTimeSeconds = i->router->getIntAttribute("delayForAlternativesRoutes", 1200.f);
//        i->walkSpeed = i->router->getIntAttribute("minDefaultSpeed", 3.6f) / 3.6f;
//        i->maxRouteTime = i->router->getIntAttribute("maxRouteTime", 60 * 60 * 10);
//        i->defaultTravelSpeed = i->router->getIntAttribute("maxDefaultSpeed", 60) / 3.6f;
//        string mn = i->router->getAttribute("max_num_changes");
//        int maxNumOfChanges = 3;
//        try
//        {
//            maxNumOfChanges = stoi(mn);
//        } catch (...)
//        {
//            // Ignore
//        }
//        i->maxNumberOfChanges = maxNumOfChanges;
//
//        RouteAttributeContext& obstacles = i->router->getObjContext(RouteDataObjectAttribute::ROUTING_OBSTACLES);
//        dynbitset bs = i->getRawBitset("time", "stop");
//        i->stopTime = obstacles.evaluateInt(bs, 30);
//        bs = i->getRawBitset("time", "change");
//        i->changeTime = obstacles.evaluateInt(bs, 180);
//        bs = i->getRawBitset("time", "boarding");
//        i->boardingTime = obstacles.evaluateInt(bs, 180);
//
//        RouteAttributeContext& spds = i->router->getObjContext(RouteDataObjectAttribute::ROAD_SPEED);
//        bs = i->getRawBitset("route", "walk");
//        i->walkSpeed = spds.evaluateFloat(bs, 3.6f) / 3.6f;
//
//        return i;
//    }
};


#endif //_OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
