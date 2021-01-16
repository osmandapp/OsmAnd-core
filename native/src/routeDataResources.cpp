#ifndef _OSMAND_ROUTE_DATA_RESOURCES_CPP
#define _OSMAND_ROUTE_DATA_RESOURCES_CPP
#include "routeDataResources.h"

Location::Location()
: latitude(0)
, longitude(0)
, altitude(0)
, speed(0)
, bearing(0)
, accuracy(0)
, verticalAccuracy(0) {
}

Location::Location(double latitude, double longitude)
: latitude(latitude)
, longitude(longitude)
, altitude(0)
, speed(0)
, bearing(0)
, accuracy(0)
, verticalAccuracy(0) {
}

bool Location::isInitialized() {
	return latitude != 0 && longitude != 0;
}

RouteDataResources::RouteDataResources() {
}

RouteDataResources::RouteDataResources(vector<Location> locations) : locations(locations) {
}

Location RouteDataResources::getLocation(int index) {
	index += currentLocation;
	if (index < locations.size()) {
		return locations[index];
	}
	return Location();
}
void RouteDataResources::incrementCurrentLocation(int index) {
	currentLocation += index;
}

#endif /*_OSMAND_ROUTE_DATA_RESOURCES_CPP*/
