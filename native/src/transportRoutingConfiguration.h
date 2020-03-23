#ifndef _OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
#define _OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "generalRouter.h"
#include <algorithm>

struct TransportRoutingConfiguration {
    
    const static string KEY = "public_transport";
    int32_t zoomToLoadTiles;
    int32_t walkRadius;
    int32_t walkChangeRadius;
    int32_t maxNumberOfChanges;
    int32_t finishTimeSeconds;
    int32_t maxRouteTime = 60 * 60 * 10;
    SHARED_PTR<GeneralRouter> router;
    
    float walkSpeed;
    float defaulTravelSpeed;
    int32_t stopTime;
    int32_t changeTime;
    int32_t boardingTime;
    
    bool useSchedule;

    int32_t scheduleTimeOfDay;
    int32_t scheduleMaxTime;
    int32_t scheduleDayNumber;

    MAP_STR_INT rawTypes;
    MAP_STR_FLOAT speed;

    float getSpeedByRouteType(std::string routeType) {
        float sl = speed.find(routeType);
        if (speed.find(routeType) != speed.end) {
            //??????
            sl = router->getObjContext(RouteDataObjectAttribute::ROAD_SPEED).evaluateFloat();
            speed.inser
        }
    };
    
    int32_t getChangeTime() {
        return useSchedule ? 0 : changeTime;
    };

    int32_t getBoardingTime() {
        return boardingTime;
    };

    int32_t getRawType(string tag, string val) {
        tag_value key = make_pair(tag,val);
        if (rawTypes.find(key) == rawTypes.end()) {
            uint at = router->registerTagValuetAttribute(key)
        }
    }

    string getAttribute(SHARED_PTR<GeneralRouter> router, string propertyName) {
        if (router->containsAttribute(propertyName)) {
            return router->getAttribute(propertyName);
        }
        return attributes[propertyName];
    }
}

class TransportRoutingConfigurationBuilder {

    SHARED_PTR<GeneralRouter> router;
    MAP_STR_STR attributes;
    GeneralRouter router;
    
    SHARED_PTR<TransportRoutingConfiguration> build(SHARED_PTR<GeneralRouter> router, const MAP_STR_STR& params = MAP_STR_STR()) {
        SHARED_PTR<TransportRoutingConfiguration> i = std::make_shared<TransportRoutingConfiguration>();
        i->router = router->build(params);
        i->walkRadius = (int) i->router->parseFloat("walkRadius", 1500.f);
        i->walkChangeRadius = (int) i->router->parseFloat("walkChangeRadius", 300.f);
        i->zoomToLoadTiles = (int) i->router->parseFloat("zoomToLoadTiles", 15.f);
        i->maxNumberOfChanges = (int) i->router->parseFloat("maxNumberOfChanges", 3.f);
        i->finishTimeSeconds = (int) i->router->parseFloat("delayForAlternativesRoutes", 1200.f);
        i->maxNumberOfChanges = (int) i->router->parseFloat("max_num_changes", 3.f);
        i->walkSpeed = i->router->parseFloat("minDefaultSpeed", 3.6f) / 3.6f;
        i->defaultTravelSpeed = i->router->parseFloat("maxDefaultSpeed", 60) / 3.6f;
        RouteAttributeContext& obstacles = i->router->getObjContext(RouteDataObjectAttribute::ROUTING_OBSTACLES);
        RouteAttributeContext& spds = i->router->getObjContext(RouteDataObjectAttribute::ROAD_SPEED);
        
//TODO write evaluateInt/Float methods like in java 
        i->stopTime = (int) obstacles.evalueateInt(i->getRawBitset("time", "stop"), i->stopTime);
        i->changeTime = (int) obstacles.evaluateInt(i->getRawBitSet("time", "change"), i->changeTime);
        i->boardingTime = (int) obstacles.evaluateInt(i->getRawBitSet("time", "boarding"), i->boardingTime);
        i->walkSpeed = spds.evaluateFloat(getRawBitset("route", "walk"), i->walkSpeed);

        return i;
    }
};


#endif _OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H