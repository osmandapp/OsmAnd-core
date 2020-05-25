#ifndef _OSMAND_TRANSPORT_ROUTING_CONTEXT_H
#define _OSMAND_TRANSPORT_ROUTING_CONTEXT_H

#include "CommonCollections.h"
#include "ElapsedTimer.h"
#include "commonOsmAndCore.h"

class RouteCalculationProgress;
struct BinaryMapFile;
struct SearchQuery;
struct TransportRoutingConfiguration;
struct TransportRouteSegment;
struct TransportStop;
struct TransportRoute;
struct TransportRouteStopsReader;

struct TransportRoutingContext {
	SHARED_PTR<RouteCalculationProgress> calculationProgress;
	UNORDERED(map)<int64_t, SHARED_PTR<TransportRouteSegment>> visitedSegments;
	SHARED_PTR<TransportRoutingConfiguration> cfg;
	UNORDERED(map)<int64_t, SHARED_PTR<TransportRoute>> combinedRouteCache;
	UNORDERED(map)<SHARED_PTR<TransportStop>, vector<SHARED_PTR<TransportRoute>>> missingStopsCache;
	UNORDERED(map)<int64_t, std::vector<SHARED_PTR<TransportRouteSegment>>> quadTree;

	TransportRouteStopsReader* transportStopsReader;

	int32_t startX;
	int32_t startY;
	int32_t targetX;
	int32_t targetY;

	double startLat;
	double startLon;
	double endLat;
	double endLon;

	int64_t startCalcTime;
	int32_t visitedRoutesCount;
	int32_t visitedStops;
	int32_t wrongLoadedWays;
	int32_t loadedWays;

	OsmAnd::ElapsedTimer loadTime;
	OsmAnd::ElapsedTimer searchTransportIndexTime;
	OsmAnd::ElapsedTimer loadSegmentsTime;
	OsmAnd::ElapsedTimer readTime;

	int32_t walkRadiusIn31;
	int32_t walkChangeRadiusIn31;
	int32_t finishTimeSeconds; 

	TransportRoutingContext(SHARED_PTR<TransportRoutingConfiguration>& cfg_);

	inline static double getTileDistanceWidth(float zoom) {
		double lat1 = 30;
		double lon1 = getLongitudeFromTile(zoom, 0);
		double lat2 = 30;
		double lon2 = getLongitudeFromTile(zoom, 1);
		return getDistance(lat1, lon1, lat2, lon2);
	}

	void calcLatLons();
	void getTransportStops(int32_t sx, int32_t sy, bool change,
						   vector<SHARED_PTR<TransportRouteSegment>> &res);
	void buildSearchTransportRequest(SearchQuery *q, int sleft, int sright,
									 int stop, int sbottom, int limit,
									 vector<SHARED_PTR<TransportStop>> &stops);
	std::vector<SHARED_PTR<TransportRouteSegment>> loadTile(uint32_t x, uint32_t y);
	void loadTransportSegments(vector<SHARED_PTR<TransportStop>> &stops,
							   vector<SHARED_PTR<TransportRouteSegment>> &lst);
	void loadScheduleRouteSegment(
		std::vector<SHARED_PTR<TransportRouteSegment>> &lst,
		SHARED_PTR<TransportRoute> &route, int32_t stopIndex);
};

#endif	// _OSMAND_TRANSPORT_ROUTING_CONTEXT_H
