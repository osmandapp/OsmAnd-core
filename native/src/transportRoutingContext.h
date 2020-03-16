#ifndef _OSMAND_TRANSPORT_ROUTING_CONTEXT_H
#define _OSMAND_TRANSPORT_ROUTING_CONTEXT_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "binaryRead.h"
#include "transportRouteConfiguration.h"
#include "transportRoute.h"
#include "routeCalculationProgress.h"
#include "common.cpp"
#include "ElapsedTimer.h"
#include "Logging.h"


struct TransportRoutingContext {
    SHARED_PTR<RouteCalculationProgress> calculationProgress;
    UNORDERED_map<int64_t, SHARED_PTR<TransportRouteSegment>> visitedSegments;
    SHARED_PTR<TransportRouteConfiguration> cfg;

    map<int64_t, vector<SHARED_PTR<TransportRouteSegment>>> quadTree;
    //Need BinaryMapIndexReader!!! and linked hashmap implementation/
    const map<BinaryMapIndexReader, UNORDERED_map<int64_t, SHARED_PTR<TransportRoute>>> routeMap;

    int64_t startCalcTime;
    int32_t visitedRoutesCount;
    int32_t visitedStops;
    int32_t wrongLoadedWays;
    int32_t loadedWays;
    OsmAnd::ElapsedTimer loadTime;
    OsmAnd::ElapsedTimer readTime;

    const int32_t walkRadiusIn31;
    const int32_t walkChangeRadiusIn31; 

    TransportRoutingContext(TransportRoutingConfiguration cfg_, vector<BinaryMapIndexReader>& readers) {
        cfg = make_shared<TransportRoutingConfiguration>(cfg_);
        walkRadiusIn31 = (int) (cfg.walkRadius / getTileDistanceWidth(31));
        walkChangeRadiusIn31 = (int) (cfg.walkChangeRadius / getTileDistanceWidth(31));
        //for (BinaryMapIndexReader) //.... need to read map data.
    }

    vector<SHARED_PTR<TransportRouteSegment>> getTransportStops(double lat, double lon) {
        int32_t y = get31TileNumberY(lat);
        int32_t x = get31TileNumberX(lon);
        return getTransportStops(x, y, false, std::vector<<SHARED_PTR<TransportRouteSegment>>()); //how to pass new empty vector as arg?
    }

    vector<SHARED_PTR<TransportRouteSegment>> getTransportStops(int32_t sx, int32_t sy, bool change, vector<SHARED_PTR<TransportRouteSegment>> res) {
        loadTime.Start();
        int32_t d = change ? walkChangeRadiusIn31 : walkRadiusIn31;
        int32_t lx = (sx - d) >> (31 - cfg.zoomToLoadTiles);
        int32_t rx = (sx + d ) >> (31 - cfg.zoomToLoadTiles);
        int32_t ty = (sy - d ) >> (31 - cfg.zoomToLoadTiles);
        int32_t by = (sy + d ) >> (31 - cfg.zoomToLoadTiles);
        for(int32_t x = lx; x <= rx; x++) {
            for(int32_t y = ty; y <= by; y++) {
                int64_t tileId = (((int64_t) x) << (cfg.zoomToLoadTiles + 1)) + y;
                vector<SHARED_PTR<TransportRouteSegment>>& list = quadTree.find(tileId);
                if (list.size() == 0) {
                    list = loadTile(x, y);
                    quadTree.insert(std::pair<int64_t, vector<SHARED_PTR<TransportRouteSegment>>>(tileId, list));
                }
                for (SHARED_PTR<TransportRouteSegment>& it : list.begin) {
                    TransportStop& st = it->getStop(it->segStart);
                    if (abs(st.x31 - sx) > walkRadiusIn31 || abs(st.y31 - sy) > walkRadiusIn31) {
                        wrongLoadedWays++;
                    } else {
                        loadedWays++;
                        res.push_back(*st);
                    }
                }
            }
        }
        loadTime.Pause();
        return res;
    }

    vector<SHARED_PTR<TransportRouteSegment>> loadTile(int x, int y) {
    //need another way?
    }

    //todo change for native loading mechanic
    vector<TransportStop> mergeTransportStops( BinaryMapIndexReader& reader, 
        UNORDERED_map<int64_t, SHARED_PTR<TransportStop>>& loadedTransportStops,
        vector<SHARED_PTR<TransportStop>>& stops,
        UNORDERED_map<int64_t, SHARED_PTR<TransportRoute>>& localFileRoutes,
        UNORDERED_map<int64_t, SHARED_PTR<TransportRoute>>& loadedRoutes) {

        vector<int32_t> routesToLoad;
        vector<int32_t> localRoutesToLoad;
        
        vector<int32_t>::iterator it = stops->begin();
        while(it != stops.end()) {
            int64_t stopId = *it->id;
            localRoutesToLoad.clear();
            SHARED_PTR<TransportStop> multifileStop = loadedTransportStops->find(stopId);
            vector<int64_t> routesIds = *it->routesIds;
            vector<int64_t> delRIds = *it->deletedRoutesIds;
            if (loadedTransportStops->find(stopId) == loadedTransportStops->end()) {
                loadedTransportStops->insert(std::pair<int64_t, SHARED_PTR<TransportStop>>(stopId, *it));
                multifileStop = *it;
                if (!(*it->isDeleted())) {
                    localRoutesToLoad.insert(localRoutesToLoad.end(), *it->referencesToRoutes.begin(), *it->referencesToRoutes.end());
                }
            } else if (multifileStop->isDeleted()) {
                it = stops->erase(it);
            } else {
                if (delRIds.size > 0) {
                    for (vector<int64_t>::iterator it = delRIds.begin(); it != delRIds.end(); it++) {
                        multifileStop->deletedRoutesIds.push_back(*it);
                    }
                }
                if (routesIds.size > 0) {
                    vector<int32_t> refs = *it->referencesToRoutes;
                    for (int32_t i = 0; i < routesIds.size(); i++) {
                        int64_t routeId = routesIds.at(i);
                        if (multifileStop->routesIds.find(routeId) == multifileStop->routesIds.end() 
                        && multifileStop->isRouteDeleted(routeId)) {
                            localRoutesToLoad.push_back(refs.at(i));
                        }
                    }
                } else {
                    if (*it->hasReferencesToRoutes()) {
                        localRoutesToLoad.insert(localRoutesToLoad.end(), *it->referencesToRoutes.stabeginrt(), *it->referencesToRoutes.end());
                    } else {
                        it = stops.erase(it);
                    }
                }
            }
            routesToLoad.insert(routesToLoad.end(), localRoutesToLoad.begin(), localRoutesToLoad.end());
            
        //TODO get name of file. 
            multifileStop->putReferenceToRoutes("placeholder_filename", localRoutesToLoad);
            it++;
        }

        if (routesToLoad.size() > 0) {
            sort(routesToLoad.begin(), routesToLoad.end());
            vector<int32_t> referencesToLoad;
            vector<int32_t>::iterator itr = routesToLoad.begin();
            int32_t p = routesToLoad.at(0) + 1;
            while (itr != routesToLoad.end()) {
                int nxt = *itr;
                if (p != nxt) {
                    if (loadedRoutes.find(nxt) != loadedRoutes.end()) {
                        localFileRoutes.insert(std::pair<int64_t, SHARED_PTR<TransportRoute>>(nxt, loadedRoutes->find(nxt)));
                    } else {
                        referencesToLoad.add(nxt);
                    }
                }
                itr++;
            }
        //todo: what to use?
            reader->loadTransportRoutes(referencesToLoad, localFileRoutes);
            loadedRoutes.insert(localFileRoutes.begin(), localFileRoutes.end());
        }

        return stops;

    }

    void loadTransportSegments(vector<TransportStops>& stops, vector<TransportRouteSegment>& lst) {
        
    }

    void loadScheduleRouteSegment(vector<TransportRouteSegment> lst, TransportRoute route, int stopIndex) {
    }
}

#endif _OSMAND_TRANSPORT_ROUTING_CONTEXT_H