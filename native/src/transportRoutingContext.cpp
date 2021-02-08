#ifndef _OSMAND_TRANSPORT_ROUTING_CONTEXT_CPP
#define _OSMAND_TRANSPORT_ROUTING_CONTEXT_CPP
#include "transportRoutingContext.h"

#include "Logging.h"
#include "binaryRead.h"
#include "routeCalculationProgress.h"
#include "transportRoutePlanner.h"
#include "transportRouteSegment.h"
#include "transportRouteStopsReader.h"
#include "transportRoutingConfiguration.h"
#include "transportRoutingObjects.h"

TransportRoutingContext::TransportRoutingContext(SHARED_PTR<TransportRoutingConfiguration> &cfg_) {
	cfg = cfg_;
	walkRadiusIn31 = (cfg->walkRadius / getTileDistanceWidth(31));
	walkChangeRadiusIn31 = (cfg->walkChangeRadius / getTileDistanceWidth(31));
	vector<BinaryMapFile *> files = getOpenMapFiles();
	transportStopsReader = std::make_shared<TransportRouteStopsReader>(files);

	startCalcTime = 0;
	visitedRoutesCount = 0;
	visitedStops = 0;
	wrongLoadedWays = 0;
	loadedWays = 0;
}

void TransportRoutingContext::calcLatLons() {
	startLat = get31LatitudeY(startY);
	startLon = get31LongitudeX(startX);
	endLat = get31LatitudeY(targetY);
	endLon = get31LongitudeX(targetX);
}

void TransportRoutingContext::getTransportStops(int32_t sx, int32_t sy, bool change,
												vector<SHARED_PTR<TransportRouteSegment>> &res) {
	loadTime.Start();
	int32_t d = change ? walkChangeRadiusIn31 : walkRadiusIn31;
	int32_t lx = (sx - d) >> (31 - cfg->zoomToLoadTiles);
	int32_t rx = (sx + d) >> (31 - cfg->zoomToLoadTiles);
	int32_t ty = (sy - d) >> (31 - cfg->zoomToLoadTiles);
	int32_t by = (sy + d) >> (31 - cfg->zoomToLoadTiles);
	for (int32_t x = lx; x <= rx; x++) {
		for (int32_t y = ty; y <= by; y++) {
			int64_t tileId = (((int64_t)x) << (cfg->zoomToLoadTiles + 1)) + y;
			vector<SHARED_PTR<TransportRouteSegment>> list;
			auto it = quadTree.find(tileId);

			if (it == quadTree.end()) {
				list = loadTile(x, y);
				quadTree.insert({tileId, list});
			} else {
				list = it->second;
			}
			for (SHARED_PTR<TransportRouteSegment> &it : list) {
				SHARED_PTR<TransportStop> st = it->getStop(it->segStart);
				if (abs(st->x31 - sx) > walkRadiusIn31 || abs(st->y31 - sy) > walkRadiusIn31) {
					wrongLoadedWays++;
				} else {
					loadedWays++;
					res.push_back(it);
				}
			}
		}
	}
	loadTime.Pause();
}

void TransportRoutingContext::buildSearchTransportRequest(SearchQuery *q, int sleft, int sright, int stop, int sbottom,
														  int limit, vector<SHARED_PTR<TransportStop>> &stops) {
	q->transportResults = stops;
	q->left = sleft >> (31 - TRANSPORT_STOP_ZOOM);
	q->right = sright >> (31 - TRANSPORT_STOP_ZOOM);
	q->top = stop >> (31 - TRANSPORT_STOP_ZOOM);
	q->bottom = sbottom >> (31 - TRANSPORT_STOP_ZOOM);
	q->limit = limit;
}

std::vector<SHARED_PTR<TransportRouteSegment>> TransportRoutingContext::loadTile(uint32_t x, uint32_t y) {
	readTime.Start();
	vector<SHARED_PTR<TransportRouteSegment>> lst;
	int pz = (31 - cfg->zoomToLoadTiles);
	vector<SHARED_PTR<TransportStop>> stops;
	SearchQuery q;
	buildSearchTransportRequest(&q, (x << pz), ((x + 1) << pz), (y << pz), ((y + 1) << pz), -1, stops);

	stops = transportStopsReader->readMergedTransportStops(&q);
	loadTransportSegments(stops, lst);
	readTime.Pause();
	return lst;
}

void TransportRoutingContext::loadTransportSegments(vector<SHARED_PTR<TransportStop>> &stops,
													vector<SHARED_PTR<TransportRouteSegment>> &lst) {
	loadSegmentsTime.Start();
	for (SHARED_PTR<TransportStop> &s : stops) {
		if (s->isDeleted() || s->routes.size() == 0) {
			continue;
		}
		for (SHARED_PTR<TransportRoute> &route : s->routes) {
			int stopIndex = -1;
			double dist = SAME_STOP;
			for (int k = 0; k < route->forwardStops.size(); k++) {
				SHARED_PTR<TransportStop> st = route->forwardStops[k];
				if (st->id == s->id) {
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
					lst.push_back(make_shared<TransportRouteSegment>(route, stopIndex));
				}
			} else {
				OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error,
								  "Routing error: missing stop '%s' in route '%s' id: %d", s->name.c_str(),
								  route->ref.c_str(), route->id / 2);
			}
		}
	}
	loadSegmentsTime.Pause();
}

void TransportRoutingContext::loadScheduleRouteSegment(std::vector<SHARED_PTR<TransportRouteSegment>> &lst,
													   SHARED_PTR<TransportRoute> &route, int32_t stopIndex) {
	// if (route->schedule != nullptr) {
	vector<int32_t> ti = route->schedule.tripIntervals;
	int32_t cnt = ti.size();
	int32_t t = 0;
	// improve by using exact data
	int32_t stopTravelTime = 0;
	vector<int32_t> avgStopIntervals = route->schedule.avgStopIntervals;
	for (int32_t i = 0; i < stopIndex; i++) {
		if (avgStopIntervals.size() > i) {
			stopTravelTime += avgStopIntervals[i];
		}
	}
	for (int32_t i = 0; i < cnt; i++) {
		t += ti[i];
		int32_t startTime = t + stopTravelTime;
		if (startTime >= cfg->scheduleTimeOfDay && startTime <= cfg->scheduleTimeOfDay + cfg->scheduleMaxTime) {
			lst.push_back(make_shared<TransportRouteSegment>(route, stopIndex, startTime));
		}
	}
	// }
}

#endif	//_OSMAND_TRANSPORT_ROUTING_CONTEXT_CPP
