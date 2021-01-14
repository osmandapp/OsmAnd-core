#ifndef _OSMAND_ROUTE_DATA_RESOURCES_CPP
#define _OSMAND_ROUTE_DATA_RESOURCES_CPP
#include "routeDataResources.h"

RouteDataResources::RouteDataResources(vector<pair<double, double>> locations) : locations(locations) {
}

pair<double, double> RouteDataResources::getLocation(int index) {
	index += currentLocation;
	if (index < locations.size()) {
		return locations[index];
	}
	return {NAN, NAN};
}
void RouteDataResources::incrementCurrentLocation(int index) {
	currentLocation += index;
}

#endif /*_OSMAND_ROUTE_DATA_RESOURCES_CPP*/
