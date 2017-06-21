#ifndef _OSMAND_ROUTE_PLANNER_FRONT_END_H
#define _OSMAND_ROUTE_PLANNER_FRONT_END_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"

struct RouteSegmentResult;
struct RoutingContext;
struct RouteSegmentPoint;
struct PrecalculatedRouteDirection;

class RoutePlannerFrontEnd {
    
    bool useSmartRouteRecalculation;
    
public:
    
    RoutePlannerFrontEnd() : useSmartRouteRecalculation(true) {
    }
    
private:
    vector<RouteSegmentResult> searchRoute(RoutingContext* ctx,
                                           vector<SHARED_PTR<RouteSegmentPoint>>& points,
                                           SHARED_PTR<PrecalculatedRouteDirection> routeDirection);
    
public:
    
    void setUseFastRecalculation(bool use) {
        useSmartRouteRecalculation = use;
    }

    vector<RouteSegmentResult> searchRoute(SHARED_PTR<RoutingContext> ctx,
                                           int startX, int startY,
                                           int endX, int endY,
                                           vector<int> intermediatesX, vector<int> intermediatesY,
                                           SHARED_PTR<PrecalculatedRouteDirection> routeDirection = nullptr);
};

#endif /*_OSMAND_ROUTE_PLANNER_FRONT_END_H*/
