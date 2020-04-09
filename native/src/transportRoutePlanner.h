#ifndef _OSMAND_TRANSPORT_ROUTE_PLANNER_H
#define _OSMAND_TRANSPORT_ROUTE_PLANNER_H
#include "CommonCollections.h"
#include "routeCalculationProgress.h"
#include "commonOsmAndCore.h"

#include <queue>

const bool MEASURE_TIME = false;

inline int TransportSegmentPriorityComparator(double o1DistFromStart, double o2DistFromStart) {
    if(o1DistFromStart == o2DistFromStart) {
        return 0;
    }
    return o1DistFromStart < o2DistFromStart ? -1 : 1;
}

struct TransportSegmentsComparator;
struct TransportRouteSegment;
struct TransportRoutingContext;
struct TransportRouteResult;

typedef priority_queue<SHARED_PTR<TransportRouteSegment>, vector<SHARED_PTR<TransportRouteSegment>>, TransportSegmentsComparator> TRANSPORT_SEGMENTS_QUEUE;

class TransportRoutePlanner
{
public:
    TransportRoutePlanner();
    ~TransportRoutePlanner();
    
    vector<SHARED_PTR<TransportRouteResult>> buildTransportRoute(SHARED_PTR<TransportRoutingContext>& ctx);
    void updateCalculationProgress(SHARED_PTR<TransportRoutingContext>& ctx, TRANSPORT_SEGMENTS_QUEUE& queue);
    vector<SHARED_PTR<TransportRouteResult>> prepareResults(SHARED_PTR<TransportRoutingContext>& ctx, vector<SHARED_PTR<TransportRouteSegment>>& results);
    bool includeRoute(TransportRouteResult& fastRoute, TransportRouteResult& testRoute);
private:
    bool includeRoute(SHARED_PTR<TransportRouteResult>& fastRoute, SHARED_PTR<TransportRouteResult>& testRoute);
    void updateCalculationProgress(SHARED_PTR<TransportRoutingContext>& ctx, priority_queue<SHARED_PTR<TransportRouteSegment>>& queue);
};

#endif // _OSMAND_TRANSPORT_ROUTE_PLANNER_H
