#ifndef _OSMAND_ROUTE_SEGMENT_RESULT_H
#define _OSMAND_ROUTE_SEGMENT_RESULT_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "binaryRead.h"
#include <algorithm>
#include "Logging.h"

struct RouteSegmentResult {
	SHARED_PTR<RouteDataObject> object;
	int startPointIndex;
	int endPointIndex;
    float segmentTime;
	float routingTime;
    float distance;
	vector<vector<RouteSegmentResult> > attachedRoutes;

	RouteSegmentResult(SHARED_PTR<RouteDataObject> object, int startPointIndex, int endPointIndex) :
		object(object), startPointIndex(startPointIndex), endPointIndex (endPointIndex), routingTime(0) {
	}
};

#endif /*_OSMAND_ROUTE_SEGMENT_RESULT_H*/