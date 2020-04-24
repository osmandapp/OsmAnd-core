#ifndef _OSMAND_TRANSPORT_ROUTE_SEGMENT_H
#define _OSMAND_TRANSPORT_ROUTE_SEGMENT_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"

struct TransportRoute;
struct TransportStop;

struct TransportRouteSegment {
	static const int32_t SHIFT = 10;		  // assume less than 1024 stops
	static const int32_t SHIFT_DEPTIME = 14;  // assume less than 1024 stops

	const int32_t segStart;
	SHARED_PTR<TransportRoute> road;

	int32_t departureTime;
	SHARED_PTR<TransportRouteSegment> parentRoute;
	bool hasParentRoute = false;
	int32_t parentStop;		  // last stop to exit for parent route
	double parentTravelTime;  // travel time for parent route
	double parentTravelDist;  // travel distance for parent route (inaccurate)
	// walk distance to start route location (or finish in case last segment)
	double walkDist = 0;
	// main field accumulated all time spent from beginning of journey
	double distFromStart = 0;

	TransportRouteSegment(SHARED_PTR<TransportRoute> road_, int32_t stopIndex);
	TransportRouteSegment(SHARED_PTR<TransportRoute> road_, int32_t stopIndex_,
						  int32_t depTime_);
	TransportRouteSegment(SHARED_PTR<TransportRouteSegment> s);

	bool wasVisited(SHARED_PTR<TransportRouteSegment>& rrs);
	SHARED_PTR<TransportStop> getStop(int i);
	pair<double, double> getLocation();
	double getLocationLat();
	double getLocationLon();
	int32_t getLength();
	int64_t getId();
	int32_t getDepth();
	string to_string();

	// static inline string formatTransporTime(int32_t i)
	// {
	//     int32_t h = i / 60 / 6;
	//     int32_t mh = i - h * 60 * 6;
	//     int32_t m = mh / 6;
	//     int32_t s = (mh - m * 6) * 10;
	//     boost::format tm = boost::format("%02d:%02d:%02d ") % h % m % s;
	//     return tm.str();
	// }
};

#endif /*_OSMAND_TRANSPORT_ROUTE_SEGMENT_H*/
