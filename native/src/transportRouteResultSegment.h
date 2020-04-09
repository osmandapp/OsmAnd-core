#ifndef _OSMAND_TRANSPORT_ROUTE_RESULT_SEGMENT_H
#define _OSMAND_TRANSPORT_ROUTE_RESULT_SEGMENT_H
#include "transportRoutingObjects.h"

#define GEOMETRY_WAY_ID -1
#define STOPS_WAY_ID -2

struct TransportRouteResultSegment {
    private:
        static const bool DISPLAY_FULL_SEGMENT_ROUTE = false;
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
    
        vector<SHARED_PTR<Way>> getGeometry() {
            vector<SHARED_PTR<Way>> list;
            route->mergeForwardWays();
            if (DISPLAY_FULL_SEGMENT_ROUTE) {
                if (route->forwardWays.size() > DISPLAY_SEGMENT_IND) {
                    list.push_back(route->forwardWays[DISPLAY_SEGMENT_IND]);
                    return list;
                }
                return route->forwardWays;
            }
            vector<SHARED_PTR<Way>> fw = route->forwardWays;
            double minStart = 150;
            double minEnd = 150;
            
            double startLat = getStart()->lat;
            double startLon = getStart()->lon;
            double endLat = getEnd()->lat;
            double endLon = getEnd()->lon;
            
            int endInd = -1;
            vector<SHARED_PTR<Node>> res;
            for (auto it = fw.begin(); it != fw.end(); ++it) {
                vector<SHARED_PTR<Node>> nodes = (*it)->nodes;
                for (auto nodesIt = nodes.begin(); nodesIt != nodes.end(); ++nodesIt) {
                    const auto n = *nodesIt;
                    double startDist = getDistance(startLat, startLon, n->lat, n->lon);
                    if (startDist < minStart) {
                        minStart = startDist;
                        res.clear();
                    }
                    res.push_back(n);
                    double endDist = getDistance(endLat, endLon, n->lat, n->lon);
                    if (endDist < minEnd) {
                        endInd = res.size();
                        minEnd = endDist;
                    }
                }
            }
            SHARED_PTR<Way> way = nullptr;
            if (res.size() == 0 || endInd == -1) {
                way = make_shared<Way>(STOPS_WAY_ID);
                for (int i = start; i <= end; i++) {
                    double lLat = getStop(i)->lat;
                    double lLon = getStop(i)->lon;
                    const auto n = make_shared<Node>(lLat, lLon, -1);
                    way->addNode(n);
                }
            } else {
                way = make_shared<Way>(GEOMETRY_WAY_ID);
                for(int k = 0; k < res.size() && k < endInd; k++) {
                    way->addNode(res[k]);
                }
            }
            list.push_back(way);
            return list;
        }

        SHARED_PTR<TransportStop> getStop(int32_t i) {
            return route->forwardStops.at(i);
        }
};

#endif /*_OSMAND_TRANSPORT_ROUTE_RESULT_SEGMENT_H*/
