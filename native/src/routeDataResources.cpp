#ifndef _OSMAND_ROUTE_DATA_RESOURCES_CPP
#define _OSMAND_ROUTE_DATA_RESOURCES_CPP
#include "routeDataResources.h"

Location::Location()
: latitude(NAN)
, longitude(NAN)
, altitude(NAN)
, speed(NAN)
, bearing(NAN)
, accuracy(NAN)
, verticalAccuracy(NAN) {
}

Location::Location(double latitude, double longitude)
: latitude(latitude)
, longitude(longitude)
, altitude(NAN)
, speed(NAN)
, bearing(NAN)
, accuracy(NAN)
, verticalAccuracy(NAN) {
}

bool Location::isInitialized() {
	return !isnan(latitude) && !isnan(longitude);
}

RouteDataResources::RouteDataResources() : currentLocation(0) {
}

RouteDataResources::RouteDataResources(vector<Location> locations) : currentLocation(0), locations(locations) {
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
