#ifndef _OSMAND_ROUTE_SEGMENT_RESULT_CPP
#define _OSMAND_ROUTE_SEGMENT_RESULT_CPP
#include "routeSegmentResult.h"

void RouteSegmentResult::attachRoute(int roadIndex, RouteSegmentResult& r) {
    int st = abs(roadIndex - startPointIndex);
    if (st >= attachedRoutesFrontEnd.size()) {
        attachedRoutesFrontEnd.resize(st + 1);
    }
    attachedRoutesFrontEnd[st].push_back(r);
}

void RouteSegmentResult::copyPreattachedRoutes(RouteSegmentResult& toCopy, int shift) {
    if (!toCopy.preAttachedRoutes.empty()) {
        preAttachedRoutes.clear();
        preAttachedRoutes.insert(preAttachedRoutes.end(), toCopy.preAttachedRoutes.begin() + shift, toCopy.preAttachedRoutes.end());
    }
}

vector<RouteSegmentResult> RouteSegmentResult::getPreAttachedRoutes(int routeInd) {
    int st = abs(routeInd - startPointIndex);
    if (!preAttachedRoutes.empty() && st < preAttachedRoutes.size()) {
        return preAttachedRoutes[st];
    }
    return vector<RouteSegmentResult>();
}

vector<RouteSegmentResult> RouteSegmentResult::getAttachedRoutes(int routeInd) {
    int st = abs(routeInd - startPointIndex);
    if (st >= attachedRoutesFrontEnd.size()) {
        return vector<RouteSegmentResult>();
    } else {
        return attachedRoutesFrontEnd[st];
    }
}

#endif /*_OSMAND_ROUTE_SEGMENT_RESULT_CPP*/
