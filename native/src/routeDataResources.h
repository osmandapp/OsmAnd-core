#ifndef _OSMAND_ROUTE_DATA_RESOURCES_H
#define _OSMAND_ROUTE_DATA_RESOURCES_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "routeTypeRule.h"
#include "binaryRead.h"

struct RouteDataResources {
private:
    int currentLocation;
public:
    UNORDERED_map<RouteTypeRule, uint32_t> rules;
    vector<pair<double, double>> locations;
    UNORDERED_map<SHARED_PTR<RouteDataObject>, vector<vector<uint32_t>>> pointNamesMap;
    
    RouteDataResources(vector<pair<double, double>> locations);
    
    pair<double, double> getLocation(int index);
    void incrementCurrentLocation(int index);
};

#endif /*_OSMAND_ROUTE_DATA_RESOURCES_H*/
