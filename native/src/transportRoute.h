#ifndef _OSMAND_TRANSPORT_ROUTE_H
#define _OSMAND_TRANSPORT_ROUTE_H
#include <string>
#include <vector>
using namespace std;

//TODO MapDataObject is not java's MapObject! I need to implement it here to? 
//need latlon, id, 

struct MapObject {
    int64_t id;
    double lat;
    double lon;
    string name;
    string enName;
}

struct TransportRoute : public MapObject {
    vector<SHARED_PTR<TransportStop>> forwardStops;
    string ref;
    string operator;
    string type;
    int32_t dist;
    string color;
    vector<SHARED_PTR<Way>> forwardWays; //todo is there a Way or analogue?
    SHARED_PTR<TransportSchedule> schedule;
    static const double SAVE_STOP = 40; 
    
    TransportRoute() {
        dist = -1;
    }

    void mergeForwardWays() {
        //todo
    }

    void addWay(Way w) {

    } 


}

struct TransportStop : public MapObject {
    const static int32_t DELETED_STOP = -1;

    vector<int32_t> referencesToRoutes;
    vector<int64_t> deletedRoutesIds;
    vector<int64_t> routesIds;
    int32_t distance;
    int32_t x31;
    int32_t y31;
    vector<TransportStopExit> exits;
    vector<SHARED_PTR<TransportRoute>> routes;
    map<string, vector<int32_t>> referencesToRoutesMap; //add linked realizations?
    
    bool isDeleted() {
        return referenceToRoutes.size() == 1 && referenceToRoutes.at(0) ==  DELETED_STOP;
    }

    bool isRouteDeleted(int64_t routeId) {
        return deletedRoutesIds.find(routeId) != deletedRoutesIds.end();
    }

    bool hasReferencesToRoutes() {
        return !isDeleted() && referencesToRoutes.size() > 0;
    }

    void putReferenceToRoutes(string repositoryFileName, vector<int32_t> referencesToRoutes) {
        referencesToRoutesMap.insert(std:pair<string, vector<int32_t>>(repositoryFileName, referencesToRoutes));
    }

}

struct TransportStopExit {

}

struct TransportSchedule {
    vector<int32_t> tripIntervals;
    vector<int32_t> avgStopIntervals;
    vector<int32_t> avgWaitIntervals;

    bool compareSchedule (const TransportSchedule& thatObj) {
        return tripIntervals == thatObj->tripIntervals 
        && avgStopIntervals == thatObj->avgStopIntervals
        && avgWaitIntervals == thatObj->avgWaitIntervals;
    }
}



#endif _OSMAND_TRANSPORT_ROUTE_H
