#ifndef _OSMAND_ROUTE_SEGMENT_RESULT_H
#define _OSMAND_ROUTE_SEGMENT_RESULT_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "binaryRead.h"
#include "turnType.h"
#include <algorithm>
#include "Logging.h"

struct RouteSegmentResult {
private:
    int startPointIndex;
    int endPointIndex;

public:
    SHARED_PTR<RouteDataObject> object;
    float segmentTime;
    float segmentSpeed;
	float routingTime;
    float distance;
	vector<vector<RouteSegmentResult> > attachedRoutes;
    vector<vector<RouteSegmentResult> > preAttachedRoutes;

    string description;
    // this make not possible to make turns in between segment result for now
    SHARED_PTR<TurnType> turnType;

	RouteSegmentResult(SHARED_PTR<RouteDataObject> object, int startPointIndex, int endPointIndex) :
		object(object), startPointIndex(startPointIndex), endPointIndex (endPointIndex), routingTime(0), description("") {
            updateCapacity();
	}
    
    void attachRoute(int roadIndex, RouteSegmentResult& r);
    void copyPreattachedRoutes(RouteSegmentResult& toCopy, int shift);
    vector<RouteSegmentResult> getPreAttachedRoutes(int routeInd);
    vector<RouteSegmentResult> getAttachedRoutes(int routeInd);

    inline void updateCapacity() {
        int capacity = abs(endPointIndex - startPointIndex) + 1;
        attachedRoutes.resize(capacity);
    }

    inline int getStartPointIndex() const {
        return startPointIndex;
    }
    
    inline int getEndPointIndex() const {
        return endPointIndex;
    }
    
    inline void setEndPointIndex(int endPointIndex) {
        this->endPointIndex = endPointIndex;
        updateCapacity();
    }
    
    inline void setStartPointIndex(int startPointIndex) {
        this->startPointIndex = startPointIndex;
        updateCapacity();
    }
    
    inline bool isForwardDirection() {
        return endPointIndex - startPointIndex > 0;
    }
    
    inline float getBearingBegin() {
        return (float) (object->directionRoute(startPointIndex, startPointIndex < endPointIndex) / M_PI * 180);
    }

    inline float getBearingEnd() {
        return (float) (alignAngleDifference(object->directionRoute(endPointIndex, startPointIndex > endPointIndex) - M_PI) / M_PI * 180);
    }
    
    inline float getBearing(int point, bool plus) {
        return (float) (object->directionRoute(point, plus) / M_PI * 180);
    }

    inline float getDistance(int point, bool plus) {
        return (float) (plus ? object->distance(point, endPointIndex): object->distance(startPointIndex, point));
    }

    inline std::vector<double> getHeightValues() {
        const auto& pf = object->calculateHeightArray();
        if (pf.empty()) {
            return std::vector<double>();
        }
        bool reverse = startPointIndex > endPointIndex;
        int st = min(startPointIndex, endPointIndex);
        int end = max(startPointIndex, endPointIndex);
        int sz = (end - st + 1) * 2;
        std::vector<double> res(sz);
        for (int k = 0; k < sz / 2; k++) {
            int ind = reverse ? (2 * (end - k)) : (2 * (k + st));
            if (k == 0) {
                res[2 * k] = 0;
            } else {
                if (ind < pf.size()) {
                    res[2 * k] = pf[k];
                }
            }
            if (ind < pf.size()) {
                res[2 * k + 1] = pf[ind + 1];
            }
        }
        return res;
    }
};

#endif /*_OSMAND_ROUTE_SEGMENT_RESULT_H*/
