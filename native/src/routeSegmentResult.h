#ifndef _OSMAND_ROUTE_SEGMENT_RESULT_H
#define _OSMAND_ROUTE_SEGMENT_RESULT_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "binaryRead.h"
#include "turnType.h"
#include <algorithm>
#include "routeDataResources.h"
#include <routeTypeRule.h>

// this should be bigger (50-80m) but tests need to be fixed first
#define DIST_BEARING_DETECT 5

#define DIST_BEARING_DETECT_UNMATCHED 50

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
	vector<vector<SHARED_PTR<RouteSegmentResult> > > attachedRoutes;
    vector<vector<SHARED_PTR<RouteSegmentResult> > > attachedRoutesFrontEnd;
    vector<vector<SHARED_PTR<RouteSegmentResult> > > preAttachedRoutes;

    string description;
    
    // this make not possible to make turns in between segment result for now
    SHARED_PTR<TurnType> turnType;

	RouteSegmentResult(SHARED_PTR<RouteDataObject> object, int startPointIndex, int endPointIndex) :
		startPointIndex(startPointIndex), endPointIndex(endPointIndex), object(object), segmentTime(0), segmentSpeed(0), routingTime(0), distance(0), description("") {
            updateCapacity();
	}
    
    void attachRoute(int roadIndex, SHARED_PTR<RouteSegmentResult> r);
    void copyPreattachedRoutes(SHARED_PTR<RouteSegmentResult> toCopy, int shift);
    vector<SHARED_PTR<RouteSegmentResult> > getPreAttachedRoutes(int routeInd);
    vector<SHARED_PTR<RouteSegmentResult> > getAttachedRoutes(int routeInd);
    void fillNames(SHARED_PTR<RouteDataResources> resources);

    inline void updateCapacity() {
        int capacity = abs(endPointIndex - startPointIndex) + 1;
        int oldLength = attachedRoutesFrontEnd.size();
        attachedRoutesFrontEnd.resize(min(oldLength, capacity));
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
        return getBearingBegin(startPointIndex, DIST_BEARING_DETECT);
    }

    inline float getBearingEnd() {
        return getBearingEnd(endPointIndex, DIST_BEARING_DETECT);
    }
    
    inline float getBearingBegin(int point, float dist) {
        return getBearing(point, true, dist);
    }
    
    inline float getBearingEnd(int point, float dist) {
        return getBearing(point, false, dist);
    }
    
    inline float getBearing(int point, bool begin, float dist) {
        if (begin) {
            return (float) (object->directionRoute(point, startPointIndex < endPointIndex) / M_PI * 180);
        } else {
            double dr = object->directionRoute(point, startPointIndex > endPointIndex, dist);
            return (float) (alignAngleDifference(dr - M_PI) / M_PI * 180);
        }
    }

    inline float getDistance(int point, bool plus) {
        return (float) (plus ? object->distance(point, endPointIndex) : object->distance(startPointIndex, point));
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

        if (reverse)
        {
            for (int k = 1; k <= sz / 2; k++)
            {
                int ind = (2 * (end--));
                if (ind < pf.size() && k < sz / 2)
                {
                    res[2 * k] = pf[ind];
                }
                if (ind < pf.size())
                {
                    res[2 * (k - 1) + 1] = pf[ind + 1];
                }
            }
        }
        else
        {
            for (int k = 0; k < sz / 2; k++)
            {
                int ind = (2 * (st + k));
                if (k > 0 && ind < pf.size())
                {
                    res[2 * k] = pf[ind];
                }
                if (ind < pf.size())
                {
                    res[2 * k + 1] = pf[ind + 1];
                }
            }
        }
        return res;
    }
};

#endif /*_OSMAND_ROUTE_SEGMENT_RESULT_H*/
