#ifndef _OSMAND_TRANSPORT_ROUTE_PLANNER_H
#define _OSMAND_TRANSPORT_ROUTE_PLANNER_H
#include <queue>

#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "routeCalculationProgress.h"

const bool MEASURE_TIME = false;

struct TransportSegmentsComparator;
struct TransportRouteSegment;
struct TransportRoutingContext;
struct TransportRouteResult;

typedef priority_queue<SHARED_PTR<TransportRouteSegment>, vector<SHARED_PTR<TransportRouteSegment>>,
					   TransportSegmentsComparator>
	TRANSPORT_SEGMENTS_QUEUE;

class TransportRoutePlanner {
   public:
	TransportRoutePlanner();
	~TransportRoutePlanner();

	vector<unique_ptr<TransportRouteResult>> buildTransportRoute(SHARED_PTR<TransportRoutingContext>& ctx);
	void updateCalculationProgress(SHARED_PTR<TransportRoutingContext>& ctx, TRANSPORT_SEGMENTS_QUEUE& queue);
	vector<unique_ptr<TransportRouteResult>> prepareResults(SHARED_PTR<TransportRoutingContext>& ctx,
															vector<SHARED_PTR<TransportRouteSegment>>& results);
	bool includeRoute(TransportRouteResult& fastRoute, TransportRouteResult& testRoute);

   private:
	bool includeRoute(unique_ptr<TransportRouteResult>& fastRoute, unique_ptr<TransportRouteResult>& testRoute);
	void updateCalculationProgress(SHARED_PTR<TransportRoutingContext>& ctx,
								   priority_queue<SHARED_PTR<TransportRouteSegment>>& queue);
};

#endif	// _OSMAND_TRANSPORT_ROUTE_PLANNER_H
