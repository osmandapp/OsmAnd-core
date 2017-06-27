#ifndef _OSMAND_ROUTE_RESULT_PREPARATION_H
#define _OSMAND_ROUTE_RESULT_PREPARATION_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"

struct RouteSegmentResult;
struct FinalRouteSegment;

void printResults(RoutingContext* ctx, int startX, int startY, int endX, int endY, vector<RouteSegmentResult>& result);
vector<RouteSegmentResult> prepareResult(RoutingContext* ctx, SHARED_PTR<FinalRouteSegment> finalSegment);
vector<RouteSegmentResult> prepareResult(RoutingContext* ctx, vector<RouteSegmentResult>& result);

#endif /*_OSMAND_ROUTE_RESULT_PREPARATION_H*/
