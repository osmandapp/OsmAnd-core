#ifndef _OSMAND_TRANSPORT_ROUTE_PLANNER_H
#define _OSMAND_TRANSPORT_ROUTE_PLANNER_H
#include "transportRoute.h"
#include "routeCalculationProgress.h"
#include "binaryRead.h"
#include "commonOsmAndCore.h"
#include "common.cpp"
#include "ElapsedTimer.h"
#include "Logging.h"
#include "transportRoutingContext.h"
#include "transportRouteConfiguration.h"

static const bool MEASURE_TIME = false;
static const int64_t GEOMETRY_WAY_ID = -1;
static const int64_t STOPS_WAY_ID = -2;



struct TransportRouteResultSegment{
    private:
        static const bool DOSPLAY_FULL_SEGMENT_ROUTE = false;
        static const int  DISPLAY_SEGMENT_IND = 0;

    public:
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
                vector<int32_t> intervals = route->schedule.avgStopIntervals;
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
            route->forwardStops.at(start);
        }

        SHARED_PTR<TransportStop> getEnd() {
            route->forwardStops.at(end);
        }

        //sublist?
        vector<TransportStop> getTravelStops() {
            return vector<TransportStop> ts(route->forwardStops.at(start), route->forwardStops.at(end));
        };

        QuadRect getSegmentRect(); // seems like it is needed for ui only
        vector<Node> getNodes(); //same
        vector<Way> getGeometry(); //same
        
        //todo need to import MapObject?
        double getTravelDist() {
            double d = 0;
            for (int32_t k = start; k < end; k++) {
                // d += getDistance(
                //     route->forwardStops.at(k).location.lat, route->forwardStops.at(k).location.lon,
                //     route->forwardStops.at(k + 1).location.lat, route->forwardStops.at(k + 1).location.lon
                // )
            }
        };

        SHARED_PTR<TransportStop> getStop(int32_t i) {
            return route->forwardStops.at(i);
        };
}

struct TransportRouteResult {
    vector<SHARED_PTR<TransportRouteResultSegment>> segments;
    double finishWalkDist;
    double routeTime;
    SHARED_PTR<TransportRoutingConfiguration> cfg; 

    TransportRouteResult(TransportRoutingContext& ctx) {
        cfg = make_shared<TransportRoutingConfiguration>(ctx->cfg);
    }
    
    //ui/logging
    double getWalkDist() {
        double d = finishWalkDist;
        for (vector<TransportRouteResultSegment>::iterator it = segments.begin(); it != segments.end(); it++) {
            d += *it->walkDist;
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
        for (vector<TransportRouteResultSegment>::iterator it = segments.begin(); it != segments.end(); it++) {
            stops += (*it->end - *it->start);
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
    }

    //for ui/logs
    double getTravelTime() {
        double t = 0;
        for (SHARED_PTR<TransportRouteResultSegment>& it : segments) {
            if (cfg->useSchedule) {
                TransportSchedule& sts = it->route->schedule;
                for (int k = it->start; k < it->end; k++) {
                    t += sts.avgStopIntervals.at(k) * 10;
                }
            } else {
                t += cfg->getBoardingTime();
                t += it->travelTime;
            }
        }
        return t;
    }

    //for ui/logs
    double getWalkTime() {
        return getWalkDist() / cfg->walkSpeed;
    }
    //for ui/logs
    double getChangeTime() {
        return cfg->changeTime;
    }
    //for ui/logs
    double getBoardingTime() {
        return cfg->boardingTime;
    }
    //for ui/logs
    int getChanges() {
        return segments.size() - 1;
    }

    string to_string() {
        //todo add logs
    }
}

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

    TransportRouteSegment(TransportRoute road_, int32_t stopIndex) {
        road = make_shared<TransportRoute>(road_);
        segStart = stopIndex;
        departureTime = -1;
    }

    TransportRouteSegment(TransportRoute road_, int32_t stopIndex_, int32_t depTime_) {
        road = make_shared<TransportRoute>(road_);
        segStart = stopIndex_;
        departureTime = depTime_;
    }

    TransportRouteSegment(TransportRouteSegment& s) {
        road = s->road;
        segStart = s->segStart;
        departureTime = s->departureTime;      
    }

    bool wasVisited(TransportRouteSegment& rrs) {
        if (rrs->road->id == road->id && rrs->departureTime == departureTime) {
            return true;
        } 
        if (hasParentRoute) {
            return parentRoute->wasVisited(rrs);
        }
        return false;
    }

    SHARED_PTR<TransportStop> getStop(int i) {
        road->forwardStops.at(i);
    }

    pair<double, double> getLocation() {
        return pair<double, double>(road->forwardStops.at(segStart).lat, road->forwardStops.at(segStart).lon);
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

    string to_string() {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Route: %s, stop: %s %s", road->name, road->);
    }
}

// struct TransportRoutingContext {
//     RouteCalculationProgress calculationProgress;
//     UNORDERED_map<int64_t, SHARED_PTR<TransportRouteSegment>> visitedSegments;
//     SHARED_PTR<TransportRouteConfiguration> cfg;

//     map<int64_t, vector<SHARED_PTR<TransportRouteSegment>>> quadTree;
//     //Need BinaryMapIndexReader!!! and linked hashmap implementation/
//     const map<BinaryMapIndexReader, UNORDERED_map<int64_t, SHARED_PTR<TransportRoute>>> routeMap;

//     int64_t startCalcTime;
//     int32_t visitedRoutesCount;
//     int32_t visitedStops;
//     int32_t wrongLoadedWays;
//     int32_t loadedWays;
//     OsmAnd::ElapsedTimer loadTime;
//     OsmAnd::ElapsedTimer readTime;

//     const int32_t walkRadiusIn31;
//     const int32_t walkChangeRadiusIn31; 

//     TransportRoutingContext(TransportRoutingConfiguration cfg_, vector<BinaryMapIndexReader>& readers) {
//         cfg = make_shared<TransportRoutingConfiguration>(cfg_);
//         walkRadiusIn31 = (int) (cfg.walkRadius / getTileDistanceWidth(31));
//         walkChangeRadiusIn31 = (int) (cfg.walkChangeRadius / getTileDistanceWidth(31));
//         for (BinaryMapIndexReader) //.... need to read map data.
//     }

//     vector<SHARED_PTR<TransportRouteSegment>> getTransportStops(double lat, double lon) {
//         int32_t y = get31TileNumberY(lat);
//         int32_t x = get31TileNumberX(lon);
//         return getTransportStops(x, y, false, std::vector<<SHARED_PTR<TransportRouteSegment>>()); //how to pass new empty vector as arg?
//     }

//     vector<SHARED_PTR<TransportRouteSegment>> getTransportStops(int32_t sx, int32_t sy, bool change, vector<SHARED_PTR<TransportRouteSegment>> res) {
//         loadTime.Start();
//         int32_t d = change ? walkChangeRadiusIn31 : walkRadiusIn31;
//         int32_t lx = (sx - d) >> (31 - cfg.zoomToLoadTiles);
//         int32_t rx = (sx + d ) >> (31 - cfg.zoomToLoadTiles);
//         int32_t ty = (sy - d ) >> (31 - cfg.zoomToLoadTiles);
//         int32_t by = (sy + d ) >> (31 - cfg.zoomToLoadTiles);
//         for(int32_t x = lx; x <= rx; x++) {
//             for(int32_t y = ty; y <= by; y++) {
//                 int64_t tileId = (((int64_t) x) << (cfg.zoomToLoadTiles + 1)) + y;
//                 vector<SHARED_PTR<TransportRouteSegment>>& list = quadTree.find(tileId);
//                 if (list.size() == 0) {
//                     list = loadTile(x, y);
//                     quadTree.insert(std::pair<int64_t, vector<SHARED_PTR<TransportRouteSegment>>>(tileId, list));
//                 }
//                 for (SHARED_PTR<TransportRouteSegment>& it : list.begin) {
//                     TransportStop& st = it->getStop(it->segStart);
//                     if (abs(st.x31 - sx) > walkRadiusIn31 || abs(st.y31 - sy) > walkRadiusIn31) {
//                         wrongLoadedWays++;
//                     } else {
//                         loadedWays++;
//                         res.push_back(*st);
//                     }
//                 }
//             }
//         }
//         loadTime.Pause();
//         return res;
//     }

//     vector<SHARED_PTR<TransportRouteSegment>> loadTile(int x, int y) {
//     //need another way?
//     }

//     vector<TransportStop> mergeTransportStops(
//     //todo change for native loading mechanic
//         BinaryMapIndexReader& reader, 
//         UNORDERED_map<int64_t, SHARED_PTR<TransportStop>>& loadedTransportStops,
//         vector<SHARED_PTR<TransportStop>>& stops,
//         UNORDERED_map<int64_t, SHARED_PTR<TransportRoute>>& localFileRoutes,
//         UNORDERED_map<int64_t, SHARED_PTR<TransportRoute>>& loadedRoutes) {

//         vector<int32_t> routesToLoad;
//         vector<int32_t> localRoutesToLoad;
        
//         vector<int32_t>::iterator it = stops->begin();
//         while(it != stops.end()) {
//             int64_t stopId = *it->id;
//             localRoutesToLoad.clear();
//             SHARED_PTR<TransportStop> multifileStop = loadedTransportStops->find(stopId);
//             vector<int64_t> routesIds = *it->routesIds;
//             vector<int64_t> delRIds = *it->deletedRoutesIds;
//             if (loadedTransportStops->find(stopId) == loadedTransportStops->end()) {
//                 loadedTransportStops->insert(std::pair<int64_t, SHARED_PTR<TransportStop>>(stopId, *it));
//                 multifileStop = *it;
//                 if (!(*it->isDeleted())) {
//                     localRoutesToLoad.insert(localRoutesToLoad.end(), *it->referencesToRoutes.begin(), *it->referencesToRoutes.end());
//                 }
//             } else if (multifileStop->isDeleted()) {
//                 it = stops->erase(it);
//             } else {
//                 if (delRIds.size > 0) {
//                     for (vector<int64_t>::iterator it = delRIds.begin(); it != delRIds.end(); it++) {
//                         multifileStop->deletedRoutesIds.push_back(*it);
//                     }
//                 }
//                 if (routesIds.size > 0) {
//                     vector<int32_t> refs = *it->referencesToRoutes;
//                     for (int32_t i = 0; i < routesIds.size(); i++) {
//                         int64_t routeId = routesIds.at(i);
//                         if (multifileStop->routesIds.find(routeId) == multifileStop->routesIds.end() 
//                         && multifileStop->isRouteDeleted(routeId)) {
//                             localRoutesToLoad.push_back(refs.at(i));
//                         }
//                     }
//                 } else {
//                     if (*it->hasReferencesToRoutes()) {
//                         localRoutesToLoad.insert(localRoutesToLoad.end(), *it->referencesToRoutes.stabeginrt(), *it->referencesToRoutes.end());
//                     } else {
//                         it = stops.erase(it);
//                     }
//                 }
//             }
//             routesToLoad.insert(routesToLoad.end(), localRoutesToLoad.begin(), localRoutesToLoad.end());
            
//         //TODO get name of file. 
//             multifileStop->putReferenceToRoutes("placeholder_filename", localRoutesToLoad);
//             it++;
//         }

//         if (routesToLoad.size() > 0) {
//             sort(routesToLoad.begin(), routesToLoad.end());
//             vector<int32_t> referencesToLoad;
//             vector<int32_t>::iterator itr = routesToLoad.begin();
//             int32_t p = routesToLoad.at(0) + 1;
//             while (itr != routesToLoad.end()) {
//                 int nxt = *itr;
//                 if (p != nxt) {
//                     if (loadedRoutes.find(nxt) != loadedRoutes.end()) {
//                         localFileRoutes.insert(std::pair<int64_t, SHARED_PTR<TransportRoute>>(nxt, loadedRoutes->find(nxt)));
//                     } else {
//                         referencesToLoad.add(nxt);
//                     }
//                 }
//                 itr++;
//             }
//         //todo: what to use?
//             reader->loadTransportRoutes(referencesToLoad, localFileRoutes);
//             loadedRoutes.insert(localFileRoutes.begin(), localFileRoutes.end());
//         }

//         return stops;

//     }

//     void loadTransportSegments(vector<TransportStops>& stops, vector<TransportRouteSegment>& lst) {
        
//     }

//     void loadScheduleRouteSegment(vector<TransportRouteSegment> lst, TransportRoute route, int stopIndex) {
//     }
// }

vector<SHARED_PTR<TransportRouteResult>> buildRoute(TransportRoutingContext& ctx, double startLat, double startLon, double endLat, double endLon);
void updateCalculationProgress(TransportRoutingContext& ctx, TRANSPORT_SEGMENTS_QUEUE& queue);
vector<SHARED_PTR<TransportRouteResult>> prepareResults(TransportRoutingContext& ctx, vector<SHARED_PTR<TransportRouteSegment>>& results);
bool includeRoute(TransportRouteResult& fastRoute, TransportRouteResult& testRoute);


#endif _OSMAND_TRANSPORT_ROUTE_PLANNER_H