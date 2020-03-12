#ifndef _OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
#define _OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "generalRouter.h"
#include <algorithm>

const static string KEY = "public_transport";

struct TransportRoutingConfiguration {
    
    int zoomToLoadTiles; // or ZOOM_TO_LOAD_TILES?
    int walkRadius;
    int walkChangeRadius;
    int maxNumberOfChanges;
    int finishTimeSeconds;
    int maxRouteTime;
    SHARED_PTR<GeneralRouter> router;
    
    float walkSpeed;
    float defaulTravelSpeed;
    int stopTime;
    int changeTime;
    int boardingTime;
    
    bool useSchedule;

    int scheduleTimeOfDay;
    int scheduleMaxTime;
    int scheduleDayNumber;

    MAP_STR_INT rawTypes;
    MAP_STR_FLOAT speed;

    float getSpeedByRouteType(std::string routeType) {
        float sl = speed.find(routeType);
        if (speed.find(routeType) != speed.end)
    };
    
    int getChangeTime() {
        return useSchedule ? 0 : changeTime;
    };

    int getBoardingTime() {
        return boardingTime;
    };

    uint getRawType(string tag, string val) {
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

class TransportRouteConfigurationBuilder() {
    private: 
        SHARED_PTR<GeneralRouter> router;
        MAP_STR_STR attributes;
        GeneralRouter::Rout
    public:
        TransportRouteConfigurationBuilder();
    
    SHARED_PTR<TransportRouteConfiguration> build(SHARED_PTR<GeneralRouter> router, const MAP_STR_STR& params = MAP_STR_STR()) {
        SHARED_PTR<TransportRouteConfiguration> i = std::make_shared<TransportRoutingConfiguration>();
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
    }
}


#endif _OSMAND_TRANSPORT_ROUTING_CONFIGURATION_H