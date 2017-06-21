#ifndef _OSMAND_BINARY_ROUTE_PLANNER_H
#define _OSMAND_BINARY_ROUTE_PLANNER_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"

struct RoutingContext;
struct RouteSegmentResult;
struct RouteSegmentPoint;

//typedef std::pair<int, std::pair<string, string> > ROUTE_TRIPLE;

std::pair<int, int> getProjectionPoint(int px, int py, int xA, int yA, int xB, int yB);

SHARED_PTR<RouteSegmentPoint> findRouteSegment(int px, int py, RoutingContext* ctx);

vector<RouteSegmentResult> searchRouteInternal(RoutingContext* ctx, bool leftSideNavigation);

#endif /*_OSMAND_BINARY_ROUTE_PLANNER_H*/
