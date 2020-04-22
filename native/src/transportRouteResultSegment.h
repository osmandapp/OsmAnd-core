#ifndef _OSMAND_TRANSPORT_ROUTE_RESULT_SEGMENT_H
#define _OSMAND_TRANSPORT_ROUTE_RESULT_SEGMENT_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"

#define GEOMETRY_WAY_ID -1
#define STOPS_WAY_ID -2

struct TransportRoute;
struct TransportStop;
struct Way;

struct TransportRouteResultSegment
{
private:
	static const bool DISPLAY_FULL_SEGMENT_ROUTE = false;
	static const int DISPLAY_SEGMENT_IND = 0;

public:
	SHARED_PTR<TransportRoute> route;
	double walkTime;
	double travelDistApproximate;
	double travelTime;
	int32_t start;
	int32_t end;
	double walkDist;
	int32_t depTime;

	TransportRouteResultSegment();

	int getArrivalTime();
	double getTravelDist();
	vector<Way> getGeometry();
	SHARED_PTR<TransportStop> getStart();
	SHARED_PTR<TransportStop> getEnd();
	vector<SHARED_PTR<TransportStop>> getTravelStops();
	SHARED_PTR<TransportStop> getStop(int32_t i);
};

#endif /*_OSMAND_TRANSPORT_ROUTE_RESULT_SEGMENT_H*/
