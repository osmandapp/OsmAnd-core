#ifndef _OSMAND_TRANSPORT_ROUTE_RESULT_H
#define _OSMAND_TRANSPORT_ROUTE_RESULT_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"

struct TransportRouteResultSegment;
struct TransportRoutingContext;
struct TransportRoutingConfiguration;

struct TransportRouteResult {
	vector<unique_ptr<TransportRouteResultSegment>> segments;
	double finishWalkDist;
	double routeTime;
	TransportRoutingConfiguration *config;

	TransportRouteResult(unique_ptr<TransportRoutingConfiguration>& cfg);

	double getWalkDist();
	float getWalkSpeed();
	int getStops();
	// bool isRouteStop(TransportStop stop);
	double getTravelDist();
	double getTravelTime();
	double getWalkTime();
	double getChangeTime();
	double getBoardingTime();
	int getChanges();
	void toString();
};

#endif /*_OSMAND_TRANSPORT_ROUTE_RESULT_H*/
