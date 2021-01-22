#ifndef _OSMAND_ROUTE_DATA_RESOURCES_H
#define _OSMAND_ROUTE_DATA_RESOURCES_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "routeTypeRule.h"
#include "binaryRead.h"

struct Location {
  public:
	long time;
	double latitude;
	double longitude;
	double altitude;
	double speed;
	double bearing;
	double accuracy;
	double verticalAccuracy;
	
	Location();
	Location(double latitude, double longitude);
	
	bool isInitialized();
};

struct RouteDataResources {
private:
    int currentLocation;
public:
    UNORDERED_map<RouteTypeRule, uint32_t> rules;
    vector<Location> locations;
    UNORDERED_map<SHARED_PTR<RouteDataObject>, vector<vector<uint32_t>>> pointNamesMap;
    
    RouteDataResources();
    RouteDataResources(vector<Location> locations);
    
    Location getLocation(int index);
    void incrementCurrentLocation(int index);
};

#endif /*_OSMAND_ROUTE_DATA_RESOURCES_H*/
