#ifndef _OSMAND_TRANSPORT_ROUTE_PLANNER_H
#define _OSMAND_TRANSPORT_ROUTE_PLANNER_H
#include <queue>

#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "routeCalculationProgress.h"

const bool MEASURE_TIME = false;
const int MIN_DIST_STOP_TO_GEOMETRY = 150;

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

	void buildTransportRoute(unique_ptr<TransportRoutingContext>& ctx, vector<SHARED_PTR<TransportRouteResult>>& res);
	void updateCalculationProgress(unique_ptr<TransportRoutingContext>& ctx, TRANSPORT_SEGMENTS_QUEUE& queue);
	void prepareResults(unique_ptr<TransportRoutingContext>& ctx,
															vector<SHARED_PTR<TransportRouteSegment>>& results,
                                                            vector<SHARED_PTR<TransportRouteResult>>& routes);
	bool includeRoute(TransportRouteResult& fastRoute, TransportRouteResult& testRoute);

   private:
	bool includeRoute(SHARED_PTR<TransportRouteResult>& fastRoute, SHARED_PTR<TransportRouteResult>& testRoute);
	void updateCalculationProgress(unique_ptr<TransportRoutingContext>& ctx,
								   priority_queue<SHARED_PTR<TransportRouteSegment>>& queue);
};

#endif	// _OSMAND_TRANSPORT_ROUTE_PLANNER_H
