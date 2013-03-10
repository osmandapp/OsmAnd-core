/*
 * RoutingVehicleConfig.cpp
 *
 *  Created on: Mar 4, 2013
 *      Author: victor
 */

#include "RoutingVehicleConfig.h"

namespace OsmAnd {

RoutingVehicleConfig::RoutingVehicleConfig() {
}

RoutingVehicleConfig::~RoutingVehicleConfig() {
}

RouteDataObjectAttribute valueOfRouteDataObjectAttribute(QString& s) {
	if (s == "priority") {
		return ROAD_PRIORITIES;
	} else if (s == "speed") {
		return ROAD_SPEED;
	} else if (s == "access") {
		return ACCESS;
	} else if (s == "obstacle_time") {
		return OBSTACLES;
	} else if (s == "obstacle") {
		return ROUTING_OBSTACLES;
	} else if (s == "oneway") {
		return ONEWAY;
	} else {
		ASSERT(0, "Route data object attribute is unkown " << s.toStdString());
		return UKNOWN;
	}
}


} /* namespace OsmAnd */
