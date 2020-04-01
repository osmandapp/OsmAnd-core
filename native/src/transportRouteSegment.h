#ifndef _OSMAND_TRANSPORT_ROUTE_SEGMENT_H
#define _OSMAND_TRANSPORT_ROUTE_SEGMENT_H
#include "transportRoutingObjects.h"
#include "Logging.h"
// #include <boost/format.hpp>

struct TransportRouteSegment {
    static const int32_t SHIFT = 10; // assume less than 1024 stops
    static const int32_t SHIFT_DEPTIME = 14; // assume less than 1024 stops
    
    const int32_t segStart;
    SHARED_PTR<TransportRoute> road;
    
    int32_t departureTime;
    SHARED_PTR<TransportRouteSegment> parentRoute;
    bool hasParentRoute = false;
    int32_t parentStop; // last stop to exit for parent route
    double parentTravelTime; // travel time for parent route
    double parentTravelDist; // travel distance for parent route (inaccurate)
    // walk distance to start route location (or finish in case last segment)
    double walkDist = 0;
    // main field accumulated all time spent from beginning of journey
    double distFromStart = 0;

    TransportRouteSegment(SHARED_PTR<TransportRoute>& road_, int32_t stopIndex)
    : segStart(stopIndex)
    {
        road = road_;
        departureTime = -1;
    }

    TransportRouteSegment(SHARED_PTR<TransportRoute>& road_, int32_t stopIndex_, int32_t depTime_)
    : segStart(stopIndex_)
    {
        road = road_;
        departureTime = depTime_;
    }

    TransportRouteSegment(SHARED_PTR<TransportRouteSegment>& s)
    : segStart(s->segStart)
    {
        road = s->road;
        departureTime = s->departureTime;
    }

    bool wasVisited(SHARED_PTR<TransportRouteSegment>& rrs) {
        if (rrs->road->id == road->id && rrs->departureTime == departureTime) {
            return true;
        }
        if (hasParentRoute) {
            return parentRoute->wasVisited(rrs);
        }
        return false;
    }

    SHARED_PTR<TransportStop> getStop(int i) {
        return road->forwardStops.at(i);
    }

    pair<double, double> getLocation() {
        return pair<double, double>(road->forwardStops[segStart]->lat, road->forwardStops[segStart]->lon);
    }
    
    double getLocationLat() {
        return road->forwardStops[segStart]->lat;
    }

    double getLocationLon() {
        return road->forwardStops[segStart]->lon;
    }

    int32_t getLength() {
        return road->forwardStops.size();
    }

    int64_t getId() {
        int64_t l = road->id;
        l = l << SHIFT_DEPTIME;

        if (departureTime >= (1 << SHIFT_DEPTIME)) {
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "too long dep time %d", departureTime);
            return -1;
        }

        l += (departureTime + 1);
        l = l << SHIFT;
        if (segStart >= (1 << SHIFT)) {
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "too many stops roadId: %d, start: %d", road->id, segStart);
            return -1;
        }

        l += segStart;

        if (l < 0) {
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "too long id: %d", road->id);
            return -1;
        }
        return l;
    }

    int32_t getDepth() {
        if (hasParentRoute) {
            return parentRoute->getDepth() + 1;
        }
        return 1;
    }
    
    // static inline string formatTransporTime(int32_t i)
    // {
    //     int32_t h = i / 60 / 6;
    //     int32_t mh = i - h * 60 * 6;
    //     int32_t m = mh / 6;
    //     int32_t s = (mh - m * 6) * 10;
    //     boost::format tm = boost::format("%02d:%02d:%02d ") % h % m % s;
    //     return tm.str();
    // }

    string to_string() {
//        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Route: %s, stop: %s %s", road->name, road->forwardStops[segStart]->name, departureTime == -1 ? "" : formatTransporTime(departureTime));
        return "";
    }
};

#endif /*_OSMAND_TRANSPORT_ROUTE_SEGMENT_H*/
