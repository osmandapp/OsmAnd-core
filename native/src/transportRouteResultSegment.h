#ifndef _OSMAND_TRANSPORT_ROUTE_RESULT_SEGMENT_H
#define _OSMAND_TRANSPORT_ROUTE_RESULT_SEGMENT_H
#include "transportRoutingObjects.h"



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

#endif _OSMAND_TRANSPORT_ROUTE_RESULT_SEGMENT_H