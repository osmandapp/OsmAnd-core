#ifndef _OSMAND_TRANSPORT_ROUTE_PLANNER_H
#define _OSMAND_TRANSPORT_ROUTE_PLANNER_H
#include "routeCalculationProgress.h"
#include "binaryRead.h"
#include "commonOsmAndCore.h"
#include "ElapsedTimer.h"
#include "Logging.h"

#include "transportRoutingConfiguration.h"
#include "transportRoutingObjects.h"

// #include <boost/format.hpp>
#include <queue>

const bool MEASURE_TIME = false;
const int64_t GEOMETRY_WAY_ID = -1;
const int64_t STOPS_WAY_ID = -2;

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
        bool noErrors = true;
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

struct TransportRouteResultSegment {
    private:
        static const bool DOSPLAY_FULL_SEGMENT_ROUTE = false;
        static const int  DISPLAY_SEGMENT_IND = 0;

    public:
    
        TransportRouteResultSegment() {
        }
    
        SHARED_PTR<TransportRoute> route;
        double walkTime;
        double travelDistApproximate;
        double travelTime;
        int32_t start;
        int32_t end;
        double walkDist;
        int32_t depTime;

        int getArrivalTime() {
            if (depTime != -1) {
                int32_t tm = depTime;
                vector<int32_t> intervals = route->schedule->avgStopIntervals;
                for (int i = start; i <= end; i++) {
                    if (i == end) {
                        return tm;
                    }
                    if (intervals.size() > 1) {
                        tm += intervals.at(i);
                    } else {
                        break;
                    }
                }
            }
            return -1;
        }
        
        SHARED_PTR<TransportStop> getStart() {
            return route->forwardStops.at(start);
        }

        SHARED_PTR<TransportStop> getEnd() {
            return route->forwardStops.at(end);
        }

        
        vector<SHARED_PTR<TransportStop>> getTravelStops() {
            return vector<SHARED_PTR<TransportStop>>(route->forwardStops.begin() + start, route->forwardStops.begin() + end);
        }

        double getTravelDist() {
            double d = 0;
            for (int32_t k = start; k < end; k++) {
                const auto& stop = route->forwardStops[k];
                const auto& nextStop = route->forwardStops[k + 1];
                d += getDistance(
                              stop->lat, stop->lon,
                              nextStop->lat, nextStop->lon
                              );
            }
            return d;
        }

        SHARED_PTR<TransportStop> getStop(int32_t i) {
            return route->forwardStops.at(i);
        }
};

struct TransportRouteResult {
    vector<SHARED_PTR<TransportRouteResultSegment>> segments;
    double finishWalkDist;
    double routeTime;
    SHARED_PTR<TransportRoutingConfiguration> config;

    TransportRouteResult(TransportRoutingContext* ctx) {
        config = ctx->cfg;
    }
    
    //ui/logging
    double getWalkDist() {
        double d = finishWalkDist;
        for (vector<SHARED_PTR<TransportRouteResultSegment>>::iterator it = segments.begin(); it != segments.end(); it++) {
            d += (*it)->walkDist;
        }
        return d;
    }

    //ui only
    // double getWalkSpeed() {
    //     cfg->walkSpeed;
    // }
    
    //logging only
    int getStops() {
        int stops = 0;
        for (vector<SHARED_PTR<TransportRouteResultSegment>>::iterator it = segments.begin(); it != segments.end(); it++) {
            stops += ((*it)->end - (*it)->start);
        }
        return stops;
    }

    // ui only:
    // bool isRouteStop (TransportStop stop) {
    //     for (vector<TransportRouteResultSegment>::iterator it = segments.begin(); it != segments.end(); it++) {
    //         if (find(*it->getTravelStops().begin(), *it->getTravelStops().end(), stop) != *it->getTravelStops().end()) {
    //             return true;
    //         }
    //     }
    //     return false;
    // }

    //for ui/logs
    double getTravelDist() {
        double d = 0;
        for (SHARED_PTR<TransportRouteResultSegment>& it : segments) {
            d += it->getTravelDist();
        }
        return d;
    }

    //for ui/logs
    double getTravelTime() {
        double t = 0;
        for (SHARED_PTR<TransportRouteResultSegment> seg : segments) {
            if (config->useSchedule) {
                SHARED_PTR<TransportSchedule> sts = seg->route->schedule;
                for (int k = seg->start; k < seg->end; k++) {
                    t += sts->avgStopIntervals[k] * 10;
                }
            } else {
                t += config->getBoardingTime();
                t += seg->travelTime;
            }
        }
        return t;
    }

    //for ui/logs
    double getWalkTime() {
        return getWalkDist() / config->walkSpeed;
    }
    //for ui/logs
    double getChangeTime() {
        return config->changeTime;
    }
    //for ui/logs
    double getBoardingTime() {
        return config->boardingTime;
    }
    //for ui/logs
    int getChanges() {
        return segments.size() - 1;
    }

    string to_string() {
        //todo add logs
        return "";
    }
};





inline int TransportSegmentPriorityComparator(double o1DistFromStart, double o2DistFromStart) {
    if(o1DistFromStart == o2DistFromStart) {
        return 0;
    }
    return o1DistFromStart < o2DistFromStart ? -1 : 1;
}

struct TransportSegmentsComparator;

typedef priority_queue<SHARED_PTR<TransportRouteSegment>, vector<SHARED_PTR<TransportRouteSegment>>, TransportSegmentsComparator> TRANSPORT_SEGMENTS_QUEUE;

vector<SHARED_PTR<TransportRouteResult>> buildRoute(TransportRoutingContext& ctx, double startLat, double startLon, double endLat, double endLon);
void updateCalculationProgress(TransportRoutingContext* ctx, TRANSPORT_SEGMENTS_QUEUE& queue);
vector<SHARED_PTR<TransportRouteResult>> prepareResults(TransportRoutingContext& ctx, vector<SHARED_PTR<TransportRouteSegment>>& results);
bool includeRoute(TransportRouteResult& fastRoute, TransportRouteResult& testRoute);


#endif // _OSMAND_TRANSPORT_ROUTE_PLANNER_H
