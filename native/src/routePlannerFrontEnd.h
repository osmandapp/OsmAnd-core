#ifndef _OSMAND_ROUTE_PLANNER_FRONT_END_H
#define _OSMAND_ROUTE_PLANNER_FRONT_END_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "routingContext.h"

struct RouteSegmentResult;
struct RouteSegmentPoint;
struct PrecalculatedRouteDirection;
struct RouteSegment;
struct RoutingConfiguration;

class RoutePlannerFrontEnd {
    
    bool useSmartRouteRecalculation;
    
public:
    
    RoutePlannerFrontEnd() : useSmartRouteRecalculation(true) {
    }
    
private:
    vector<SHARED_PTR<RouteSegmentResult> > searchRoute(RoutingContext* ctx,
                                           vector<SHARED_PTR<RouteSegmentPoint>>& points,
                                           SHARED_PTR<PrecalculatedRouteDirection> routeDirection);
    
    vector<SHARED_PTR<RouteSegmentResult> > searchRouteInternalPrepare(RoutingContext* ctx, SHARED_PTR<RouteSegmentPoint> start, SHARED_PTR<RouteSegmentPoint> end, SHARED_PTR<PrecalculatedRouteDirection> routeDirection);
public:
    
    void setUseFastRecalculation(bool use) {
        useSmartRouteRecalculation = use;
    }

    SHARED_PTR<RoutingContext> buildRoutingContext(SHARED_PTR<RoutingConfiguration> config, RouteCalculationMode rm = RouteCalculationMode::NORMAL);
    
    SHARED_PTR<RouteSegment> getRecalculationEnd(RoutingContext* ctx);

    vector<SHARED_PTR<RouteSegmentResult> > searchRoute(SHARED_PTR<RoutingContext> ctx,
                                           int startX, int startY,
                                           int endX, int endY,
                                           vector<int>& intermediatesX, vector<int>& intermediatesY,
                                           SHARED_PTR<PrecalculatedRouteDirection> routeDirection = nullptr);
};

#endif /*_OSMAND_ROUTE_PLANNER_FRONT_END_H*/
