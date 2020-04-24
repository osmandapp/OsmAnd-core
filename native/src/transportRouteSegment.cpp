#ifndef _OSMAND_TRANSPORT_ROUTE_SEGMENT_CPP
#define _OSMAND_TRANSPORT_ROUTE_SEGMENT_CPP
#include "transportRouteSegment.h"
#include "Logging.h"
#include "transportRoutingObjects.h"

TransportRouteSegment::TransportRouteSegment(SHARED_PTR<TransportRoute> road_,
											 int32_t stopIndex)
	: segStart(stopIndex) {
	road = road_;
	departureTime = -1;
}

TransportRouteSegment::TransportRouteSegment(SHARED_PTR<TransportRoute> road_,
											 int32_t stopIndex_,
											 int32_t depTime_)
	: segStart(stopIndex_) {
	road = road_;
	departureTime = depTime_;
}

TransportRouteSegment::TransportRouteSegment(
	SHARED_PTR<TransportRouteSegment> s)
	: segStart(s->segStart) {
	road = s->road;
	departureTime = s->departureTime;
}

bool TransportRouteSegment::wasVisited(SHARED_PTR<TransportRouteSegment>& rrs) {
	if (rrs->road->id == road->id && rrs->departureTime == departureTime) {
		return true;
	}
	if (hasParentRoute) {
		return parentRoute->wasVisited(rrs);
	}
	return false;
}

SHARED_PTR<TransportStop> TransportRouteSegment::getStop(int i) {
	return road->forwardStops.at(i);
}

pair<double, double> TransportRouteSegment::getLocation() {
	return pair<double, double>(road->forwardStops[segStart]->lat,
								road->forwardStops[segStart]->lon);
}

double TransportRouteSegment::getLocationLat() {
	return road->forwardStops[segStart]->lat;
}

double TransportRouteSegment::getLocationLon() {
	return road->forwardStops[segStart]->lon;
}

int32_t TransportRouteSegment::getLength() { return road->forwardStops.size(); }

int64_t TransportRouteSegment::getId() {
	int64_t l = road->id;
	l = l << SHIFT_DEPTIME;

	if (departureTime >= (1 << SHIFT_DEPTIME)) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error,
						  "too long dep time %d", departureTime);
		return -1;
	}

	l += (departureTime + 1);
	l = l << SHIFT;
	if (segStart >= (1 << SHIFT)) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error,
						  "too many stops roadId: %d, start: %d", road->id,
						  segStart);
		return -1;
	}

	l += segStart;

	if (l < 0) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "too long id: %d",
						  road->id);
		return -1;
	}
	return l;
}

int32_t TransportRouteSegment::getDepth() {
	if (hasParentRoute) {
		return parentRoute->getDepth() + 1;
	}
	return 1;
}

string TransportRouteSegment::to_string() {
	return "Route: " + road->name +
		   ", stop: " + road->forwardStops[segStart]->name;
}

#endif	//_OSMAND_TRANSPORT_ROUTE_SEGMENT_CPP
