#ifndef _OSMAND_BINARY_ROUTE_PLANNER_H
#define _OSMAND_BINARY_ROUTE_PLANNER_H
#include "Common.h"
#include "common2.h"
#include "binaryRead.h"
#include <algorithm>
#include "Logging.h"
#include "generalRouter.h"

typedef UNORDERED(map)<string, float> MAP_STR_FLOAT;
typedef UNORDERED(map)<string, string> MAP_STR_STR;

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

struct RouteSegmentResult {
	SHARED_PTR<RouteDataObject> object;
	int startPointIndex;
	int endPointIndex;
	float routingTime;
	vector<vector<RouteSegmentResult> > attachedRoutes;
	RouteSegmentResult(SHARED_PTR<RouteDataObject> object, int startPointIndex, int endPointIndex) :
		object(object), startPointIndex(startPointIndex), endPointIndex (endPointIndex), routingTime(0) {

	}
};


struct RoutingSubregionTile {
	RouteSubregion subregion;
	// make it without get/set for fast access
	int access;
	int loaded;
	uint size ;
	UNORDERED(map)<int64_t, SHARED_PTR<RouteSegment> > routes;

	RoutingSubregionTile(RouteSubregion& sub) : subregion(sub), access(0), loaded(0) {
		size = sizeof(RoutingSubregionTile);
	}
	~RoutingSubregionTile(){
	}
	bool isLoaded(){
		return loaded > 0;
	}

	void setLoaded(){
		loaded = abs(loaded) + 1;
	}

	void unload(){
		routes.clear();
		size = 0;
		loaded = - abs(loaded);
	}

	int getUnloadCount() {
		return abs(loaded);
	}

	int getSize(){
		return size + routes.size() * sizeof(std::pair<int64_t, SHARED_PTR<RouteSegment> >);
	}

	void add(SHARED_PTR<RouteDataObject> o) {
		size += o->getSize() + sizeof(RouteSegment)* o->pointsX.size();
		for (uint i = 0; i < o->pointsX.size(); i++) {
			uint64_t x31 = o->pointsX[i];
			uint64_t y31 = o->pointsY[i];
			uint64_t l = (((uint64_t) x31) << 31) + (uint64_t) y31;
			SHARED_PTR<RouteSegment> segment =  SHARED_PTR<RouteSegment>(new RouteSegment(o, i));
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
static int64_t calcRouteId(SHARED_PTR<RouteDataObject> o, int ind) {
	return ((int64_t) o->id << 10) + ind;
}

typedef std::pair<int, std::pair<string, string> > ROUTE_TRIPLE;
struct RoutingConfiguration {
	GeneralRouter router;

	int memoryLimitation;
	float initialDirection;

	int zoomToLoad;
	float heurCoefficient;
	int planRoadDirection;
	string routerName;
	
	
	void initParams(MAP_STR_STR& attributes) {
		planRoadDirection = (int) parseFloat(attributes, "planRoadDirection", 0);
		heurCoefficient = parseFloat(attributes, "heuristicCoefficient", 1);
		// don't use file limitations?
		memoryLimitation = (int)parseFloat(attributes, "nativeMemoryLimitInMB", memoryLimitation);
		zoomToLoad = (int)parseFloat(attributes, "zoomToLoadTiles", 16);
		routerName = parseString(attributes, "name", "default");
		// routerProfile = parseString(attributes, "baseProfile", "car");
	}

	RoutingConfiguration(float initDirection = -360, int memLimit = 48) :
			memoryLimitation(memLimit), initialDirection(initDirection) {
	}

};

bool compareRoutingSubregionTile(SHARED_PTR<RoutingSubregionTile> o1, SHARED_PTR<RoutingSubregionTile> o2);

class RouteCalculationProgress {
protected:
	int segmentNotFound ;
	float distanceFromBegin;
	int directSegmentQueueSize;
	float distanceFromEnd;
	int reverseSegmentQueueSize;

	bool cancelled;
public:
	RouteCalculationProgress() : segmentNotFound(-1), distanceFromBegin(0),
		directSegmentQueueSize(0), distanceFromEnd(0),  reverseSegmentQueueSize(0), cancelled(false){
	}

	virtual bool isCancelled(){
		return cancelled;
	}

	virtual void setSegmentNotFound(int s){
		segmentNotFound = s;
	}

	virtual void updateStatus(float distanceFromBegin,	int directSegmentQueueSize,	float distanceFromEnd,
			int reverseSegmentQueueSize) {
		this->distanceFromBegin = max(distanceFromBegin, this->distanceFromBegin );
		this->distanceFromEnd = max(distanceFromEnd,this->distanceFromEnd);
		this->directSegmentQueueSize = directSegmentQueueSize;
		this->reverseSegmentQueueSize = reverseSegmentQueueSize;
	}

};

struct PrecalculatedRouteDirection {
	vector<uint32_t> pointsX;
	vector<uint32_t> pointsY;
	vector<float> times;
	float speed;
	static int SHIFT;
	static int SHIFTS[];
	bool empty;

	uint64_t startPoint;
	uint64_t endPoint;
	quad_tree<int> quadTree;

 	inline uint64_t calc(int x31, int y31) {
		return (((uint64_t) x31) << 32l) + ((uint64_t)y31);
	}

	float getDeviationDistance(int x31, int y31, int ind);
	float getDeviationDistance(int x31, int y31);
	int getIndex(int x31, int y31);
	float timeEstimate(int begX, int begY, int endX, int endY);

};


struct RoutingContext {
	typedef UNORDERED(map)<int64_t, SHARED_PTR<RoutingSubregionTile> > MAP_SUBREGION_TILES;

	int visitedSegments;
	int loadedTiles;
	OsmAnd::ElapsedTimer timeToLoad;
	OsmAnd::ElapsedTimer timeToCalculate;
	int firstRoadDirection;
	int64_t firstRoadId;
	RoutingConfiguration* config;
	SHARED_PTR<RouteCalculationProgress> progress;

	int gcCollectIterations;

	int startX;
	int startY;
	int targetX;
	int targetY;
	bool basemap;

	PrecalculatedRouteDirection precalcRoute;
	SHARED_PTR<RouteSegment> finalRouteSegment;

	vector<SHARED_PTR<RouteSegment> > segmentsToVisitNotForbidden;
	vector<SHARED_PTR<RouteSegment> > segmentsToVisitPrescripted;

	MAP_SUBREGION_TILES subregionTiles;
	UNORDERED(map)<int64_t, std::vector<SHARED_PTR<RoutingSubregionTile> > > indexedSubregions;

	RoutingContext(RoutingConfiguration* config) : 
		visitedSegments(0), loadedTiles(0),
		firstRoadDirection(0), firstRoadId(0),
		config(config) {
			precalcRoute.empty = true;
	}

	bool acceptLine(SHARED_PTR<RouteDataObject> r) {
		return config->router.acceptLine(r);
	}

	int getSize() {
		// multiply 2 for to maps
		int sz = subregionTiles.size() * sizeof(pair< int64_t, SHARED_PTR<RoutingSubregionTile> >)  * 2;
		MAP_SUBREGION_TILES::iterator it = subregionTiles.begin();
		for(;it != subregionTiles.end(); it++) {
			sz += it->second->getSize();
		}
		return sz;
	}

	void unloadUnusedTiles(int memoryLimit) {
		int sz = getSize();
		float critical = 0.9f * memoryLimit * 1024 * 1024;
		if(sz < critical) {
			return;
		}
		float occupiedBefore = sz / (1024. * 1024.);
		float desirableSize = memoryLimit * 0.7f * 1024 * 1024;
		vector<SHARED_PTR<RoutingSubregionTile> > list;
		MAP_SUBREGION_TILES::iterator it = subregionTiles.begin();
		int loaded = 0;
		int unloadedTiles = 0;
		for(;it != subregionTiles.end(); it++) {
			if(it->second->isLoaded()) {
				list.push_back(it->second);
				loaded++;
			}
		}
		sort(list.begin(), list.end(), compareRoutingSubregionTile);
		uint i = 0;
		while(sz >= desirableSize && i < list.size()) {
			SHARED_PTR<RoutingSubregionTile> unload = list[i];
			i++;
			sz -= unload->getSize();
			unload->unload();
			unloadedTiles ++;
		}
		for(i = 0; i<list.size(); i++) {
			list[i]->access /= 3;
		}
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Run GC (before %f Mb after %f Mb) unload %d of %d tiles",
				occupiedBefore, getSize() / (1024.0*1024.0),
				unloadedTiles, loaded);
	}

	void loadHeaderObjects(int64_t tileId) {
		vector<SHARED_PTR<RoutingSubregionTile> >& subregions = indexedSubregions[tileId];
		bool gc = false;
		for(uint j = 0; j<subregions.size() && !gc; j++) {
			if(!subregions[j]->isLoaded()) {
				gc = true;
			}
		}
		if(gc) {
			unloadUnusedTiles(config->memoryLimitation);
		}
		for(uint j = 0; j<subregions.size(); j++) {
			if(!subregions[j]->isLoaded()) {
				loadedTiles++;
				subregions[j]->setLoaded();
				SearchQuery q;
				vector<RouteDataObject*> res;
				searchRouteDataForSubRegion(&q, res, &subregions[j]->subregion);
				vector<RouteDataObject*>::iterator i = res.begin();
				for(;i!=res.end(); i++) {
					if(*i != NULL) {
						SHARED_PTR<RouteDataObject> o(*i);
						if(acceptLine(o)) {
							subregions[j]->add(o);
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
			for(uint i = 0; i<tempResult.size(); i++) {
				RouteSubregion& rs = tempResult[i];
				int64_t key = ((int64_t)rs.left << 31)+ rs.filePointer;
				if(subregionTiles.find(key) == subregionTiles.end()) {
					subregionTiles[key] = SHARED_PTR<RoutingSubregionTile>(new RoutingSubregionTile(rs));
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
		if(t <= 0) {
			t = 1;
			coordinatesShift = (1 << (31 - zoomAround));
		} else {
			t = 1 << t;
		}
		UNORDERED(set)<int64_t> ids;
		int z  = config->zoomToLoad;
		for(int i = -t; i <= t; i++) {
			for(int j = -t; j <= t; j++) {
				uint32_t xloc = (x31 + i*coordinatesShift) >> (31 - z);
				uint32_t yloc = (y31+j*coordinatesShift) >> (31 - z);
				int64_t tileId = (xloc << z) + yloc;
				loadHeaders(xloc, yloc);
				vector<SHARED_PTR<RoutingSubregionTile> >& subregions = indexedSubregions[tileId];
				for(uint j = 0; j<subregions.size(); j++) {
					if(subregions[j]->isLoaded()) {
						UNORDERED(map)<int64_t, SHARED_PTR<RouteSegment> >::iterator s = subregions[j]->routes.begin();
						while(s != subregions[j]->routes.end()) {
							SHARED_PTR<RouteSegment> seg = s->second;
							if(seg.get() != NULL) {
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
		vector<SHARED_PTR<RoutingSubregionTile> >& subregions = indexedSubregions[tileId];
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
						SHARED_PTR<RouteSegment> s = SHARED_PTR<RouteSegment>(new RouteSegment(ro, segment->getSegmentStart()));
						s->next = original;
						original = 	s;
					}
					segment = segment->next;
				}
			}
		}

		return original;
	}


	bool isInterrupted(){
		return false;
	}
	float getHeuristicCoefficient(){
		return config->heurCoefficient;
	}

	bool planRouteIn2Directions() {
		return getPlanRoadDirection() == 0;
	}
	int getPlanRoadDirection() {
		return config->planRoadDirection;
	}

};


vector<RouteSegmentResult> searchRouteInternal(RoutingContext* ctx, bool leftSideNavigation);
#endif /*_OSMAND_BINARY_ROUTE_PLANNER_H*/
