#ifndef _OSMAND_ROUTING_CONTEXT_H
#define _OSMAND_ROUTING_CONTEXT_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "binaryRead.h"
#include "routingConfiguration.h"
#include "routeCalculationProgress.h"
#include "routeSegment.h"
#include "routeSegmentResult.h"
#include "precalculatedRouteDirection.h"
#include <algorithm>
#include "Logging.h"
#include <ctime>

enum class RouteCalculationMode {
    BASE,
    NORMAL,
    COMPLEX
};

struct RoutingSubregionTile {
	RouteSubregion subregion;
	// make it without get/set for fast access
	int access;
	int loaded;
	uint size ;
	UNORDERED(map)<int64_t, SHARED_PTR<RouteSegment> > routes;
	UNORDERED(set)<int64_t > excludedIds;

	RoutingSubregionTile(RouteSubregion& sub) : subregion(sub), access(0), loaded(0) {
		size = sizeof(RoutingSubregionTile);
	}
	~RoutingSubregionTile() {
	}
	bool isLoaded() {
		return loaded > 0;
	}

	void setLoaded() {
		loaded = abs(loaded) + 1;
	}

	void unload() {
		routes.clear();
		size = 0;
		loaded = - abs(loaded);
	}

	int getUnloadCount() {
		return abs(loaded);
	}

	int getSize() {
		return size + routes.size() * sizeof(std::pair<int64_t, SHARED_PTR<RouteSegment> >);
	}

	void add(SHARED_PTR<RouteDataObject>& o) {
		size += o->getSize() + sizeof(RouteSegment) * o->pointsX.size();
		for (uint i = 0; i < o->pointsX.size(); i++) {
			uint64_t x31 = o->pointsX[i];
			uint64_t y31 = o->pointsY[i];
			uint64_t l = (((uint64_t) x31) << 31) + (uint64_t) y31;
            SHARED_PTR<RouteSegment> segment = std::make_shared<RouteSegment>(o, i);
			if (routes[l].get() == NULL) {
				routes[l] = segment;
			} else {
				SHARED_PTR<RouteSegment> orig = routes[l];
				int cnt = 0;
				while (orig->next.get() != NULL) {
					orig = orig->next;
					cnt++;
				}
				orig->next = segment;
			}
		}
	}
};

static int64_t calcRouteId(SHARED_PTR<RouteDataObject>& o, int ind) {
    return ((int64_t) o->id << 10) + ind;
}

inline int intpow(int base, int pw) {
    int r = 1;
    for (int i = 0; i < pw; i++) {
        r *= base;
    }
    return r;
}

inline int compareRoutingSubregionTile(SHARED_PTR<RoutingSubregionTile>& o1, SHARED_PTR<RoutingSubregionTile>& o2) {
    int v1 = (o1->access + 1) * intpow(10, o1->getUnloadCount() - 1);
    int v2 = (o2->access + 1) * intpow(10, o2->getUnloadCount() - 1);
    return v1 < v2;
}

struct RoutingContext {
	typedef UNORDERED(map)<int64_t, SHARED_PTR<RoutingSubregionTile> > MAP_SUBREGION_TILES;

    RouteCalculationMode calculationMode;
    float routingTime;
	int visitedSegments;
	int loadedTiles;
	OsmAnd::ElapsedTimer timeToLoad;
	OsmAnd::ElapsedTimer timeToCalculate;
	int firstRoadDirection;
	int64_t firstRoadId;
	SHARED_PTR<RoutingConfiguration> config;
	SHARED_PTR<RouteCalculationProgress> progress;
    bool leftSideNavigation;

	int gcCollectIterations;

	int startX;
	int startY;
	bool startTransportStop;
	int targetX;
	int targetY;
	bool targetTransportStop;
	bool publicTransport;
	bool basemap;

	time_t conditionalTime;
	tm conditionalTimeStr;

    vector<SHARED_PTR<RouteSegmentResult> > previouslyCalculatedRoute;
    SHARED_PTR<PrecalculatedRouteDirection> precalcRoute;
	SHARED_PTR<RouteSegment> finalRouteSegment;

	vector<SHARED_PTR<RouteSegment> > segmentsToVisitNotForbidden;
	vector<SHARED_PTR<RouteSegment> > segmentsToVisitPrescripted;

	MAP_SUBREGION_TILES subregionTiles;
	UNORDERED(map)<int64_t, std::vector<SHARED_PTR<RoutingSubregionTile> > > indexedSubregions;

    RoutingContext(RoutingContext* cp) {
        this->config = cp->config;
        this->calculationMode = cp->calculationMode;
        this->leftSideNavigation = cp->leftSideNavigation;
        this->startTransportStop = cp->startTransportStop;
        this->targetTransportStop = cp->targetTransportStop;
        this->publicTransport = cp->publicTransport;
        this->conditionalTime = cp->conditionalTime;
        this->basemap = cp->basemap;
        // copy local data and clear caches
        auto it = cp->subregionTiles.begin();
        for(;it != cp->subregionTiles.end(); it++) {
            auto tl = it->second;
            if (tl->isLoaded()) {
                subregionTiles[it->first] = tl;
                auto itr = tl->routes.begin();
                for(;itr != tl->routes.end(); itr++) {
                    auto s = itr->second;
                    while (s) {
                        s->parentRoute = nullptr;
                        s->parentSegmentEnd = 0;
                        s->distanceFromStart = 0;
                        s->distanceToEnd = 0;
                        s = s->next;
                    }
                }
            }
        }
    }
    
    RoutingContext(SHARED_PTR<RoutingConfiguration> config, RouteCalculationMode calcMode = RouteCalculationMode::NORMAL)
		: calculationMode(calcMode)
		, routingTime(0)
		, visitedSegments(0)
		, loadedTiles(0)
		, firstRoadDirection(0)
		, firstRoadId(0)
		, config(config)
		, leftSideNavigation(false)
		, startTransportStop(false)
		, targetTransportStop(false)
		, publicTransport(false)
		, conditionalTime(0)
		, precalcRoute(new PrecalculatedRouteDirection()) {
            this->basemap = RouteCalculationMode::BASE == calcMode;
	}

    void unloadAllData(RoutingContext* except = NULL) {
        auto it = subregionTiles.begin();
        for (;it != subregionTiles.end(); it++) {
            auto tl = it->second;
            if (tl->isLoaded()) {
                if (except == NULL || except->searchSubregionTile(tl->subregion) < 0) {
                    tl->unload();
                }
            }
        }
        subregionTiles.clear();
        indexedSubregions.clear();
    }

    void setConditionalTime(time_t tm) {
    	conditionalTime = tm;
    	if(conditionalTime != 0) {
    		conditionalTimeStr = *localtime(&conditionalTime);
    	}
    }
    
    int searchSubregionTile(RouteSubregion& subregion) {
        auto it = subregionTiles.begin();
        int i = 0;
        int ind = -1;
        for (;it != subregionTiles.end(); it++, i++) {
            auto tl = it->second;
            if (ind == -1 && tl->subregion.left == subregion.left) {
                ind = i;
            }
            if (ind >= 0) {
                if (i == subregionTiles.size() || tl->subregion.left > subregion.left) {
                    ind = -i - 1;
                    return ind;
                }
                if (tl->subregion.filePointer == subregion.filePointer &&
                    tl->subregion.mapDataBlock == subregion.mapDataBlock) {
                    return i;
                }
            }
        }
        return ind;
    }
    
	bool acceptLine(SHARED_PTR<RouteDataObject>& r) {
		return config->router->acceptLine(r);
	}

	int getSize() {
		// multiply 2 for to maps
		int sz = subregionTiles.size() * sizeof(pair< int64_t, SHARED_PTR<RoutingSubregionTile> >)  * 2;
		MAP_SUBREGION_TILES::iterator it = subregionTiles.begin();
		for (;it != subregionTiles.end(); it++) {
			sz += it->second->getSize();
		}
		return sz;
	}

	void unloadUnusedTiles(int memoryLimit) {
		int sz = getSize();
		float critical = 0.9f * memoryLimit * 1024 * 1024;
		if (sz < critical) {
			return;
		}
		float occupiedBefore = sz / (1024. * 1024.);
		float desirableSize = memoryLimit * 0.7f * 1024 * 1024;
		vector<SHARED_PTR<RoutingSubregionTile> > list;
		MAP_SUBREGION_TILES::iterator it = subregionTiles.begin();
		int loaded = 0;
		int unloadedTiles = 0;
		for (;it != subregionTiles.end(); it++) {
			if (it->second->isLoaded()) {
				list.push_back(it->second);
				loaded++;
			}
		}
		sort(list.begin(), list.end(), compareRoutingSubregionTile);
		uint i = 0;
		while (sz >= desirableSize && i < list.size()) {
			SHARED_PTR<RoutingSubregionTile> unload = list[i];
			i++;
			sz -= unload->getSize();
			unload->unload();
			unloadedTiles ++;
		}
		for (i = 0; i < list.size(); i++) {
			list[i]->access /= 3;
		}
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Run GC (before %f Mb after %f Mb) unload %d of %d tiles",
				occupiedBefore, getSize() / (1024.0*1024.0),
				unloadedTiles, loaded);
	}

	void loadHeaderObjects(int64_t tileId) {
        const auto itSubregions = indexedSubregions.find(tileId);
        if (itSubregions == indexedSubregions.end())
            return;
        auto& subregions = itSubregions->second;
        bool gc = false;
		for (uint j = 0; j < subregions.size(); j++) {
			if (!subregions[j]->isLoaded()) {
				gc = true;
                break;
			}
		}
		if (gc) {
			unloadUnusedTiles(config->memoryLimitation);
		}
		bool load = false;
		for (uint j = 0; j < subregions.size(); j++) {
			if (!subregions[j]->isLoaded()) {
				load = true;
                break;
			}
		}
		if (load) {
			UNORDERED(set)<int64_t > excludedIds;
			for (uint j = 0; j < subregions.size(); j++) {
				if (!subregions[j]->isLoaded()) {
					loadedTiles++;
					subregions[j]->setLoaded();
					SearchQuery q;
					vector<RouteDataObject*> res;
					searchRouteDataForSubRegion(&q, res, &subregions[j]->subregion);
					vector<RouteDataObject*>::iterator i = res.begin();
					for (;i != res.end(); i++) {
						if (*i != NULL) {
							SHARED_PTR<RouteDataObject> o(*i);
							if(conditionalTime != 0) {
								o->processConditionalTags(conditionalTimeStr);
							}
							if (acceptLine(o)) {
								if (excludedIds.find(o->getId()) == excludedIds.end()) {
									subregions[j]->add(o);
								}
							} else if (o->getId() > 0) {
								excludedIds.insert(o->getId());
								subregions[j]->excludedIds.insert(o->getId());
							}
						}
					}
				} else {
					excludedIds.insert(
						subregions[j]->excludedIds.begin(), subregions[j]->excludedIds.end());
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
	}


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

	// void searchRouteRegion(SearchQuery* q, std::vector<RouteDataObject*>& list, RoutingIndex* rs, RouteSubregion* sub)
	SHARED_PTR<RouteSegment> loadRouteSegment(int x31, int y31) {
		int z  = config->zoomToLoad;
		int64_t xloc = x31 >> (31 - z);
		int64_t yloc = y31 >> (31 - z);
		uint64_t l = (((uint64_t) x31) << 31) + (uint64_t) y31;
		int64_t tileId = (xloc << z) + yloc;
		loadHeaders(xloc, yloc);
        const auto itSubregions = indexedSubregions.find(tileId);
        if(itSubregions == indexedSubregions.end())
            return SHARED_PTR<RouteSegment>();
        auto& subregions = itSubregions->second;
		UNORDERED(map)<int64_t, SHARED_PTR<RouteDataObject> > excludeDuplications;
		SHARED_PTR<RouteSegment> original;
		for(uint j = 0; j<subregions.size(); j++) {
			if(subregions[j]->isLoaded()) {
				SHARED_PTR<RouteSegment> segment = subregions[j]->routes[l];
				subregions[j]->access++;
				while (segment.get() != NULL) {
					SHARED_PTR<RouteDataObject> ro = segment->road;
					SHARED_PTR<RouteDataObject> toCmp = excludeDuplications[calcRouteId(ro, segment->getSegmentStart())];
					if (toCmp.get() == NULL || toCmp->pointsX.size() < ro->pointsX.size()) {
						excludeDuplications[calcRouteId(ro, segment->getSegmentStart())] =  ro;
						SHARED_PTR<RouteSegment> s = std::make_shared<RouteSegment>(ro, segment->getSegmentStart());
						s->next = original;
						original = 	s;
					}
					segment = segment->next;
				}
			}
		}

		return original;
	}

	bool isInterrupted() {
        return progress != nullptr ? progress->isCancelled() : false;
	}
    
	float getHeuristicCoefficient() {
		return config->heurCoefficient;
	}

	bool planRouteIn2Directions() {
		return getPlanRoadDirection() == 0;
	}
    
	int getPlanRoadDirection() {
		return config->planRoadDirection;
	}
    
    void initTargetPoint(SHARED_PTR<RouteSegment>& end) {
        targetX = end->road->pointsX[end->segmentStart];
        targetY = end->road->pointsY[end->segmentStart];
    }
    
    void initStartAndTargetPoints(SHARED_PTR<RouteSegment> start, SHARED_PTR<RouteSegment> end) {
        initTargetPoint(end);
        startX = start->road->pointsX[start->segmentStart];
        startY = start->road->pointsY[start->segmentStart];
    }
};

#endif /*_OSMAND_ROUTING_CONTEXT_H*/
