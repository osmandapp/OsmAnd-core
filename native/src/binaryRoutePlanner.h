#ifndef _OSMAND_BINARY_ROUTE_PLANNER_H
#define _OSMAND_BINARY_ROUTE_PLANNER_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"

static const int REVERSE_WAY_RESTRICTION_ONLY = 1024;

static const int ROUTE_POINTS = 11;
// static const float TURN_DEGREE_MIN = 45;
static const short RESTRICTION_NO_RIGHT_TURN = 1;
static const short RESTRICTION_NO_LEFT_TURN = 2;
static const short RESTRICTION_NO_U_TURN = 3;
static const short RESTRICTION_NO_STRAIGHT_ON = 4;
static const short RESTRICTION_ONLY_RIGHT_TURN = 5;
static const short RESTRICTION_ONLY_LEFT_TURN = 6;
static const short RESTRICTION_ONLY_STRAIGHT_ON = 7;

struct RoutingContext;
struct RouteSegmentResult;
struct RouteSegmentPoint;

//typedef std::pair<int, std::pair<string, string> > ROUTE_TRIPLE;

SHARED_PTR<RouteSegmentPoint> findRouteSegment(int px, int py, RoutingContext* ctx);

vector<SHARED_PTR<RouteSegmentResult> > searchRouteInternal(RoutingContext* ctx, bool leftSideNavigation);

#endif /*_OSMAND_BINARY_ROUTE_PLANNER_H*/
