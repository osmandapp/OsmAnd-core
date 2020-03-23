#ifndef _OSMAND_TRANSPORT_ROUTING_CONTEXT_H
#define _OSMAND_TRANSPORT_ROUTING_CONTEXT_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "binaryRead.h"
#include "transportRoutingConfiguration.h"
#include "transportRoutingObjects.h"
#include "routeCalculationProgress.h"
#include "common.cpp"
#include "ElapsedTimer.h"
#include "Logging.h"


struct TransportRoutingContext {
    struct map<

    SHARED_PTR<RouteCalculationProgress> calculationProgress;
    UNORDERED_map<int64_t, SHARED_PTR<TransportRouteSegment>> visitedSegments;
    SHARED_PTR<TransportRoutingConfiguration> cfg;

    map<int64_t, vector<SHARED_PTR<TransportRouteSegment>>> quadTree;
    //Need BinaryMapIndexReader!!! and linked hashmap implementation/
    map<TransportIndex, map<int64_t, SHARED_PTR<TransportRoute>>> routeMap;

    int32_t startX;
    int32_t startY;
    int32_t targetX;
    int32_t targetY;

    int64_t startCalcTime;
    int32_t visitedRoutesCount;
    int32_t visitedStops;
    int32_t wrongLoadedWays;
    int32_t loadedWays;
    OsmAnd::ElapsedTimer loadTime;
    OsmAnd::ElapsedTimer readTime;

    const int32_t walkRadiusIn31;
    const int32_t walkChangeRadiusIn31; 

    TransportRoutingContext(TransportRoutingConfiguration cfg_, vector<SHARED_PTR<TransportIndex>> indexes) {
        cfg = make_shared<TransportRoutingConfiguration>(cfg_);
        walkRadiusIn31 = (int) (cfg.walkRadius / getTileDistanceWidth(31));
        walkChangeRadiusIn31 = (int) (cfg.walkChangeRadius / getTileDistanceWidth(31));
        for (SHARED_PTR<TransportIndex>& i : indexes)  {
            routeMap.insert({i, map<int64_t, SHARED_PTR<TransportRoute>>()});
        }
    }

    // vector<SHARED_PTR<TransportRouteSegment>> getTransportStops(double lat, double lon) {
    //     int32_t y = get31TileNumberY(lat);
    //     int32_t x = get31TileNumberX(lon);
    //     return getTransportStops(x, y, false, vector<<SHARED_PTR<TransportRouteSegment>>()); 
    // }

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
                if ( quadTree.find(tileId) = quadTree.end()) {
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

    vector<SHARED_PTR<TransportRouteSegment>> loadTile(uint32_t x, uint32_t y) {
        	
        vector<SHARED_PTR<TransportRouteSegment>> lst;
        int z = cfg->zoomToLoadTiles;
        int pz = (31 - z);
        int64_t tileId = (x << z) + y;
        
        SearchQuery q((uint32_t) (x << pz), (uint32_t) ((x + 1) << pz), (uint32_t)(y << pz), (uint32_t)((y+1) << pz)); 
        map<int64_t, SHARED_PTR<TransportStop>> loadedTransportStops;
        map<int64_t, SHARED_PTR<TransportRoute>> localFileRoutes;


        vector<SHARED_PTR<TransportStop>> stops = r.searchTransportIndex(&q, lst);

        localFileRoutes.clear();
        mergeTransportStops(r, loadedTransportStops, stops, localFileRoutes, routeMap.at(r));

        for (SHARED_PTR<TransportStop>& stop : stops) {
            int64_t stopId = stop->id;
            SHARED_PTR<TransportStop> multifileStop = loadedTransportStops.get(stopId);
            vector<int32_t> rrs = stop->referencesToRoutes;
            if (multifileStop == stop) {
                // clear up so it won't be used as it is multi file stop
                stop->referencesToRoutes.clear();
            } else {
                // add other routes
                stop->referencesToRoutes.clear();
            }
            if (rrs.size() > 0 && !multifileStop->isDeleted()) {
                for (int32_t& rr : rrs) {
                    SHARED_PTR<TransportRoute> route = localFileRoutes.at(rr);
                    if (!route.get()) {
                        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Something went wrong by loading route %d for stop %s", rr, stop));
                    } else if (multifileStop == stop ||
                            (!multifileStop->hasRoute(route.getId()) &&
                                    !multifileStop.isRouteDeleted(route.getId()))) {
                        // duplicates won't be added
                        multifileStop.addRouteId(route.getId());
                        multifileStop.addRoute(route);
                    }
                }
            }
        }
			
			loadTransportSegments(loadedTransportStops.valueCollection(), lst);
			
			// readTime += System.nanoTime() - nanoTime;
			return lst;
    }
    
    
    // vector<SHARED_PTR<TransportRouteSegment>> loadTile(int32_t x, int32_t y) {
    //     	// long nanoTime = System.nanoTime();
	// 		vector<SHARED_PTR<TransportRouteSegment>> lst;
	// 		int32_t pz = (31 - cfg->zoomToLoadTiles);
	// 		// SearchRequest<TransportStop> sr = BinaryMapIndexReader.buildSearchTransportRequest(x << pz, (x + 1) << pz,
	// 		// 		y << pz, (y + 1) << pz, -1, null);
			
			
	// 		UNORDERED_map<int64_t, SHARED_PTR<TransportStop>> loadedTransportStops;
	// 		UNORDERED_map<int64_t, SHARED_PTR<TransportRoute>> localFileRoutes;
	// 		for (BinaryMapIndexReader r : routeMap.keySet()) {
    //             // 	sr.clearSearchResults();
    //             vector<SHARED_PTR<TransportStop>> stops = r.searchTransportIndex(sr);

    //             localFileRoutes.clear();
    //             // mergeTransportStops(r, loadedTransportStops, stops, localFileRoutes, routeMap.at(r));

    //             for (SHARED_PTR<TransportStop>& stop : stops) {
    //                 int64_t stopId = stop->id;
    //                 SHARED_PTR<TransportStop> multifileStop = loadedTransportStops.get(stopId);
    //                 vector<int32_t> rrs = stop->referencesToRoutes;
    //                 if (multifileStop == stop) {
    //                     // clear up so it won't be used as it is multi file stop
    //                     stop->referencesToRoutes.clear();
    //                 } else {
    //                     // add other routes
    //                     stop->referencesToRoutes.clear();
    //                 }
    //                 if (rrs.size() > 0 && !multifileStop->isDeleted()) {
    //                     for (int32_t& rr : rrs) {
    //                         SHARED_PTR<TransportRoute> route = localFileRoutes.at(rr);
    //                         if (!route.get()) {
    //                             OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Something went wrong by loading route %d for stop %s", rr, stop));
    //                         } else if (multifileStop == stop ||
    //                                 (!multifileStop->hasRoute(route.getId()) &&
    //                                         !multifileStop.isRouteDeleted(route.getId()))) {
    //                             // duplicates won't be added
    //                             multifileStop.addRouteId(route.getId());
    //                             multifileStop.addRoute(route);
    //                         }
    //                     }
    //                 }
    //             }
    //         }
			
	// 		loadTransportSegments(loadedTransportStops.valueCollection(), lst);
			
	// 		// readTime += System.nanoTime() - nanoTime;
	// 		return lst;
    // }
    vector<TransportStop> mergeTransportStops( 
        //TODO change for native loading mechanic
        BinaryMapIndexReader& reader, 
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

    void loadTransportSegments(vector<SHARED_PTR<TransportStops>>& stops, vector<SHARED_PTR<TransportRouteSegment>>& lst) {
        for(SHARED_PTR<TransportStop> &s : stops) {
            if (s->isDeleted() || s->routes.size() == 0) {
                continue;
            }
            for (SHARED_PTR<TransportRoute>& route : s->routes) {
                int stopIndex = -1;
                double dist = TransportRoute->SAME_STOP;
                for (int k = 0; k < route.getForwardStops().size(); k++) {
                    SHARED_PTR<TransportStop> st = route->forwardStops.at(k);
                    if(st->id == s->id ) {
                        stopIndex = k;
                        break;
                    }
                    double d = getDistance(st->lat, st->lon, s->lat, s->lon);
                    if (d < dist) {
                        stopIndex = k;
                        dist = d;
                    }
                }
                if (stopIndex != -1) {
                    if (cfg->useSchedule) {
                        loadScheduleRouteSegment(lst, route, stopIndex);
                    } else {
                        SHARED_PTR<TransportRouteSegment> segment = make_shared<TransportRouteSegment>(TransportRouteSegment(route, stopIndex));
                        lst.push_back(segment);
                    }
                } else {
                    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Routing error: missing stop '%s' in route '%s' id: %d", s->toString(), route->ref, route->id / 2));
                }
            }
		}
    }


    void loadScheduleRouteSegment(vector<TransportRouteSegment> lst, TransportRoute route, int stopIndex) {
    }
//----------------------------------------------
    void loadTileData(int x31, int y31, int zoomAround, vector<SHARED_PTR<RouteDataObject> >& dataObjects ) {
		int t = config->zoomToLoad - zoomAround;
		int coordinatesShift = (1 << (31 - config->zoomToLoad));
		if (t <= 0) {
			t = 1;
			coordinatesShift = (1 << (31 - zoomAround));
		} else {
			t = 1 << t;
		}
		UNORDERED(set)<int64_t> ids;
		int z  = config->zoomToLoad;
		for (int i = -t; i <= t; i++) {
			for (int j = -t; j <= t; j++) {
				uint32_t xloc = (x31 + i*coordinatesShift) >> (31 - z);
				uint32_t yloc = (y31+j*coordinatesShift) >> (31 - z);
				int64_t tileId = (xloc << z) + yloc;
				loadHeaders(xloc, yloc);
                const auto itSubregions = indexedSubregions.find(tileId);
                if (itSubregions == indexedSubregions.end())
                    continue;
                auto& subregions = itSubregions->second;
				for (uint j = 0; j<subregions.size(); j++) {
					if(subregions[j]->isLoaded()) {
						UNORDERED(map)<int64_t, SHARED_PTR<RouteSegment> >::iterator s = subregions[j]->routes.begin();
						while(s != subregions[j]->routes.end()) {
							SHARED_PTR<RouteSegment> seg = s->second;
							while(seg.get() != NULL) {
								if(ids.find(seg->road->id) == ids.end()) {
									dataObjects.push_back(seg->road);
									ids.insert(seg->road->id);
								}
								seg = seg->next;
							}
							s++;
						}
					}
				}
			}
		}
    }

    void loadHeaders(uint32_t xloc, uint32_t yloc) {
		timeToLoad.Start();
		int z  = config->zoomToLoad;
		int tz = 31 - z;
		int64_t tileId = (xloc << z) + yloc;
		if (indexedSubregions.find(tileId) == indexedSubregions.end()) {
			SearchQuery q((uint32_t) (xloc << tz),
							(uint32_t) ((xloc + 1) << tz), (uint32_t) (yloc << tz), (uint32_t) ((yloc + 1) << tz));
			std::vector<RouteSubregion> tempResult;
			searchRouteSubregions(&q, tempResult, basemap);
			std::vector<SHARED_PTR<RoutingSubregionTile> > collection;
			for (uint i = 0; i < tempResult.size(); i++) {
				RouteSubregion& rs = tempResult[i];
				int64_t key = ((int64_t)rs.left << 31)+ rs.filePointer;
				if (subregionTiles.find(key) == subregionTiles.end()) {
					subregionTiles[key] = std::make_shared<RoutingSubregionTile>(rs);
				}
				collection.push_back(subregionTiles[key]);
			}
			indexedSubregions[tileId] = collection;
		}
		loadHeaderObjects(tileId);
		timeToLoad.Pause();

        //-------------------------------------
}

#endif _OSMAND_TRANSPORT_ROUTING_CONTEXT_H