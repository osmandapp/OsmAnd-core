#ifndef _OSMAND_ROUTE_SEGMENT_H
#define _OSMAND_ROUTE_SEGMENT_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "Logging.h"

struct RouteSegment {
    public :
    uint16_t segmentStart;
    SHARED_PTR<RouteDataObject> road;
    // needed to store intersection of routes
    SHARED_PTR<RouteSegment> next;
    SHARED_PTR<RouteSegment>  oppositeDirection ;
    
    // search context (needed for searching route)
    // Initially it should be null (!) because it checks was it segment visited before
    SHARED_PTR<RouteSegment> parentRoute;
    uint16_t parentSegmentEnd;
    
    
    // 1 - positive , -1 - negative, 0 not assigned
    int8_t directionAssgn;
    
    // final route segment
    int8_t reverseWaySearch;
    SHARED_PTR<RouteSegment> opposite;
    
    // distance measured in time (seconds)
    float distanceFromStart;
    float distanceToEnd;
    
    inline bool isFinal() {
        return reverseWaySearch != 0;
    }
    
    inline bool isReverseWaySearch() {
        return reverseWaySearch == 1;
    }
    
    inline uint16_t getSegmentStart() {
        return segmentStart;
    }
    
    inline bool isPositive() {
        return directionAssgn == 1;
    }
    
    inline SHARED_PTR<RouteDataObject> getRoad() {
        return road;
    }
    
    static SHARED_PTR<RouteSegment> initRouteSegment(SHARED_PTR<RouteSegment> th, bool positiveDirection) {
        if(th->segmentStart == 0 && !positiveDirection) {
            return SHARED_PTR<RouteSegment>();
        }
        if(th->segmentStart == th->road->getPointsLength() - 1 && positiveDirection) {
            return SHARED_PTR<RouteSegment>();
        }
        SHARED_PTR<RouteSegment> rs = th;
        if(th->directionAssgn == 0) {
            rs->directionAssgn = positiveDirection ? 1 : -1;
        } else {
            if(positiveDirection != (th->directionAssgn == 1)) {
                if(th->oppositeDirection.get() == NULL) {
                    th->oppositeDirection = SHARED_PTR<RouteSegment>(new RouteSegment(th->road, th->segmentStart));
                    th->oppositeDirection->directionAssgn = positiveDirection ? 1 : -1;
                }
                if ((th->oppositeDirection->directionAssgn == 1) != positiveDirection) {
                    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "Alert failed - directionAssgn wrongly");
                }
                rs = th->oppositeDirection;
            }
        }
        return rs;
    }
    
    RouteSegment(SHARED_PTR<RouteDataObject> road, int segmentStart) :
    segmentStart(segmentStart), road(road), next(), oppositeDirection(),
    parentRoute(), parentSegmentEnd(0),
    directionAssgn(0), reverseWaySearch(0), opposite(),
    distanceFromStart(0), distanceToEnd(0) {
    }
    
    ~RouteSegment(){
    }
};

struct RouteSegmentPoint : RouteSegment {
public:
    RouteSegmentPoint(SHARED_PTR<RouteDataObject> road, int segmentStart) : 
				RouteSegment(road, segmentStart) {					
                }
    ~RouteSegmentPoint(){
    }
    double dist;
    int preciseX;
    int preciseY;
    vector< SHARED_PTR<RouteSegmentPoint> > others;
};

struct FinalRouteSegment : RouteSegment {
public:
    FinalRouteSegment(SHARED_PTR<RouteDataObject> road, int segmentStart) : RouteSegment(road, segmentStart) {
    }
        
    ~FinalRouteSegment() {
    }
    bool reverseWaySearch;
    SHARED_PTR<RouteSegment> opposite;
};

#endif /*_OSMAND_ROUTE_SEGMENT_H*/
