#ifndef _OSMAND_TRANSPORT_ROUTE_PLANNER_CPP
#define _OSMAND_TRANSPORT_ROUTE_PLANNER_CPP
#include "transportRouteContext.h"
#include "transportRoutePlanner.h"
#include "transportRoutingObjects.h"

inline int TransportSegmentPriorityComparator(double o1DistFromStart, double o2DistFromStart) {
    if(o1DistFromStart == o2DistFromStart) {
        return 0;
    }
    return f1 < f2 ? -1 : 1;
}

struct TransportSegmentsComparator: public std::binary_function<SHARED_PTR<TransportRouteSegment>&, SHARED_PTR<TransportRouteSegment>&, bool>
{
	TransportRoutingContext* ctx;
	TransportSegmentsComparator(TransportRoutingContext& c) : ctx(c) {}
	bool operator()(const SHARED_PTR<TransportRouteSegment>& lhs, const SHARED_PTR<TransportRouteSegment>& rhs) const
	{
		int cmp = TransportSegmentPriorityComparator(lhs.get()->distanceFromStart, rhs.get()->distanceFromStart);
    	return cmp > 0;
    }
};

typedef priority_queue<SHARED_PTR<TransportRouteSegment>, vector<SHARED_PTR<TransportRouteSegment>>, TransportSegmentsComparator> TRANSPORT_SEGMENTS_QUEUE;

vector<SHARED_PTR<TransportRouteResult>> buildRoute(TransportRoutingContext* ctx) {
    //todo add counter

	TransportSegmentsComparator trSegmComp(ctx);
    TRANSPORT_SEGMENTS_QUEUE queue(trSegmComp);
	
    vector<SHARED_PTR<TransportRouteSegment>> startStops = ctx->getTransportStops(ctx->startX, ctx->startY, false, vector<SHARED_PTR<TransportRouteSegment>>());
    vector<SHARED_PTR<TransportRouteSegment>> endStops = ctx->getTransportStops(ctx->targetX, ctx->targetY, false, vector<SHARED_PTR<TransportRouteSegment>>());
    UNORDERED_map<int64_t, SHARED_PTR<TransportRouteSegment>> endSegments;

	ctx->calcLatLons();

    for (TransportRouteSegment& s : endStops) {
        endSegments.insert({s.getId(), s});
    }
    if (startStops->size() == 0) {
        return vector<TransportRouteResult>();
    }
	
    for (SHARED_PTR<TransportRouteSegment>& r : startStops) {
        r->walkDist = getDistance(r.getLocationLat(), r.getLocationLon(), ctx->startLat, ctx->startLon);
        r->distFromStart = r->walkDist / ctx->cfg.walkSpeed;
        queue.push(r);
    }

    double finishTime = ctx->cfg->maxRouteTime;
    double maxTravelTimeCmpToWalk = getDistance(ctx->startLat, ctx->startLon, ctx->endLat, ctx->endLon) / ctx->cfg->changeTime;
    vector<SHARED_PTR<TransportRouteResult>> results;
	//initProgressBar(ctx, start, end); - ui
    while (queue->size() > 0) {
	// 	long beginMs = MEASURE_TIME ? System.currentTimeMillis() : 0;
        if(ctx->calculationProgress.get() && ctx->calculationProgress->isCancelled()) {
			ctx->calculationProgress->setSegmentNotFound(0);
            return vector<SHARED_PTR<TransportRouteResult>>();
		}
		
		SHARED_PTR<TransportRouteSegment> segment = queue->top();
        queue.pop();	
        SHARED_PTR<TransportRouteSegment> ex ;
        if (ctx->visitedSegments.find(segment->getId()) != ctx->visitedSegments.end()) {
            ex = ctx->visitedSegments.find(segment->getId());
            if (ex->distFromStart > segment->distFromStart) {
                OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "%.1f (%s) > %.1f (%s)", ex->distFromStart, ex->to_string(), segment->distFromStart, segment->to_string());
            }
            continue;
        }
        ctx->visitedRoutesCount++;
        ctx->visitedSegments.insert({segment->getId(), segment});

        if (segment->getDepth() > ctx->cfg->maxNumberOfChanges + 1) {
            continue;
        }

        if (segment->distFromStart > finishTime + ctx->cfg->finishTimeSeconds 
			|| segment->distFromStart > maxTravelTimeCmpToWalk) {
            break;
        }

        int64_t segmentId = segment->getId();
        SHARED_PTR<TransportRouteSegment> finish = NULL;
        int64_t minDist = 0;
        int64_t travelDist = 0;
        double travelTime = 0;
        const float routeTravelSpeed = ctx->cfg->getSpeedByRouteType(segment->road->type);
					
		if(routeTravelSpeed == 0) {
			continue;
		}
		SHARED_PTR<TransportStop> prevStop = segment->getStop(segment->segStart);
		vector<SHARED_PTR<TransportRouteSegment>> sgms = vector<SHARED_PTR<TransportRouteResult>>();
		
		for (int32_t ind = 1 + segment->segStart; ind < segment->getLength(); ind++) {
			if (ctx->calculationProgress.get() && ctx->calculationProgress->isCancelled()) {
				return vector<SHARED_PTR<TransportRouteResult>>();
			}
			segmentId ++;
			ctx->visitedSegments.insert({segmentId, segment});
			SHARED_PTR<TransportStop> stop = segment->getStop(ind);
			double segmentDist = getDistance(prevStop->lat, prevStop->lon, stop->lat, stop->lon);
			travelDist += segmentDist;

			if(ctx->cfg->useSchedule) {
				SHARED_PTR<TransportSchedule> sc = segment->road->schedule;
				int interval = sc->avgStopIntervals.at(ind - 1);
				travelTime += interval * 10;
			} else {
				travelTime += ctx->cfg->stopTime + segmentDist / routeTravelSpeed;
			}
			if(segment->distFromStart + travelTime > finishTime + ctx->cfg->finishTimeSeconds) {
				break;
			}
			sgms.clear();
			sgms = ctx->getTransportStops(stop->x31, stop->y31, true, sgms);
			ctx->visitedStops++;
			for (SHARED_PTR<TransportRouteSegment>& sgm : sgms) {
				if (ctx->calculationProgress.get() && ctx.calculationProgress.isCancelled) {
					return vector<SHARED_PTR<TransportRouteResult>>();
				}
				if (segment->wasVisited(sgm)) {
					continue;
				}
				SHARED_PTR<TransportRouteSegment> nextSegment = make_shared<TransportRouteSegment>(TransportRouteSegment());
				nextSegment->parentRoute = segment;
				nextSegment->parentStop = ind;
				nextSegment->walkDist = MapUtils.getDistance(nextSegment.getLocation(), stop.getLocation());
				nextSegment->parentTravelTime = travelTime;
				nextSegment->parentTravelDist = travelDist;
				double walkTime = nextSegment->walkDist / ctx->cfg->walkSpeed
						+ ctx->cfg->getChangeTime() + ctx->cfg->getBoardingTime();
				nextSegment.distFromStart = segment->distFromStart + travelTime + walkTime;
				if(ctx->cfg->useSchedule) {
					int tm = (sgm.departureTime - ctx->cfg->scheduleTimeOfDay) * 10;
					if(tm >= nextSegment->distFromStart) {
						nextSegment->distFromStart = tm;
						queue.add(nextSegment);
					}
				} else {
					queue.add(nextSegment);
				}
			}
			SHARED_PTR<TransportRouteSegment> finalSegment = endSegments.find(segmentId);
			double dist = getDistance(stop->lat, stop->lon, endLat, endLon);

			if (endSegments.find(segmentId) != endSegments.end() && distToEnd < ctx->cfg->walkRadius) {
				if (finish == NULL || minDist > distToEnd) {
					minDist = distToEnd;
					finish = make_shared<TransportRouteSegment>(finalSegment);
					finish->parentRoute = segment;
					finish->parentStop = ind;
					finish->walkDist = distToEnd;
					finish->parentTravelTime = travelTime;
					finish->parentTravelDist = travelDist;

					double walkTime = distToEnd / ctx->cfg->walkSpeed;
					finish->distFromStart = segment.distFromStart + travelTime + walkTime;

				}
			}	
			prevStop = stop;
		}
		if (finish != NULL) {
			if (finishTime > finish->distFromStart) {
				finishTime = finish->distFromStart;
			}
			if(finish->distFromStart < finishTime + ctx->cfg->finishTimeSeconds && 
					(finish.distFromStart < maxTravelTimeCmpToWalk || results.size() == 0)) {
				results.push_back(finish);
			}
		}
		
		if (ctx->calculationProgress.get() && ctx->calculationProgress->isCancelled()) {
			OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Route calculation interrupted");
			return vector<SHARED_PTR<TransportRouteResult>>();
		}
		// if (MEASURE_TIME) {
		// 	long time = System.currentTimeMillis() - beginMs;
		// 	if (time > 10) {
		// 		System.out.println(String.format("%d ms ref - %s id - %d", time, segment.road.getRef(),
		// 				segment.road.getId()));
		// 	}
		// }
		updateCalculationProgress(ctx, queue);
    }

    return prepareResults(ctx, results);
}

// private void initProgressBar(TransportRoutingContext ctx, LatLon start, LatLon end) {
// 	if (ctx.calculationProgress != null) {
// 		ctx.calculationProgress.distanceFromEnd = 0;
// 		ctx.calculationProgress.reverseSegmentQueueSize = 0;
// 		ctx.calculationProgress.directSegmentQueueSize = 0;
// 		float speed = (float) ctx.cfg.defaultTravelSpeed + 1; // assume
// 		ctx.calculationProgress.totalEstimatedDistance = (float) (MapUtils.getDistance(start, end) / speed);
// 	}
// }

void updateCalculationProgress(TransportRoutingContext* ctx, priority_queue<SHARED_PTR<TransportRouteSegment>>& queue) {
	if (ctx->calculationProgress.get()) {
		ctx->calculationProgress->directSegmentQueueSize = queue.size();
		if (queue.size() > 0) {
			SHARED_PTR<TransportRouteSegment> peek = queue.top(); 
			ctx->calculationProgress->distanceFromBegin = (float) max(peek->distFromStart, ctx->calculationProgress->distanceFromBegin);
		}		
	}
}

vector<SHARED_PTR<TransportRouteResult>> prepareResults(TransportRoutingContext* ctx, vector<TransportRouteSegment>& results) {
	sort(results.begin(), result.end(), TransportSegmentPriorityComparator(ctx));

	vector<SHARED_PTR<TransportRouteResult>> lst;
// 		System.out.println(String.format("Calculated %.1f seconds, found %d results, visited %d routes / %d stops, loaded %d tiles (%d ms read, %d ms total), loaded ways %d (%d wrong)",
// 				(System.currentTimeMillis() - ctx.startCalcTime) / 1000.0, results.size(), 
// 				ctx.visitedRoutesCount, ctx.visitedStops, 
// 				ctx.quadTree.size(), ctx.readTime / (1000 * 1000), ctx.loadTime / (1000 * 1000),
// 				ctx.loadedWays, ctx.wrongLoadedWays));

	for(SHARED_PTR<TransportRouteSegment>& res : results) {
		if (ctx->calculationProgress.get() && ctx->calculationProgress->isCancelled()) {
			return vector<SHARED_PTR<TransportRouteResult>>();
		}
		SHARED_PTR<TransportRouteResult> route = make_shared<TransportRouteResult>(TransportRouteResult(ctx));
		route->routeTime = res->distFromStart;
		route->finishWalkDist = res->walkDist;
		SHARED_PTR<TransportRouteSegment> p = res;
		while (p.get()) {
			if (ctx->calculationProgress.get() && ctx.calculationProgress.isCancelled()) {
				return vector<SHARED_PTR<TransportRouteResult>>();
			}
			if (p.parentRoute.get()) {
				SHARED_PTR<TransportRouteResultSegment> sg = make_shared<TransportRouteResultSegment>(TransportRouteResultSegment());
				sg->route = p->parentRoute->road;
				sg->start = p->parentRoute->segStart;
				sg->end = p->parentStop;
				sg->walkDist = p->parentRoute->walkDist;
				sg->walkTime = sg->walkDist / ctx->cfg->walkSpeed;
				sg->depTime = p->departureTime;
				sg->travelDistApproximate = p->parentTravelDist;
				sg->travelTime = p->parentTravelTime;
				route->segments.insert(0, sg);
			}
			p = p->parentRoute;
		}
		// test if faster routes fully included
		bool include = false;
		for(SHARED_PTR<TransportRouteResult>& s : lst) {
			if (ctx->calculationProgress.get() && ctx->calculationProgress->isCancelled()) {
				return null;
			}
			if(includeRoute(s, route)) {
				include = true;
				break;
			}
		}
		if(!include) {
			lst.push_back(route);
			// System.out.println(route.toString());
		} else {
//				System.err.println(route.toString());
		}
	}
	return lst;
}

//done
bool includeRoute(SHARED_PTR<TransportRouteResult>& fastRoute, SHARED_PTR<TransportRouteResult>& testRoute) {
	if(testRoute->segments.size() < fastRoute->segments.size()) {
		return false;
	}
	int32_t j = 0;
	for(int32_t i = 0; i < fastRoute->segments.size(); i++, j++) {
		SHARED_PTR<TransportRouteResultSegment> fs = fastRoute->segments.at(i);
			while(j < testRoute.segments.size()) {
				TransportRouteResultSegment ts = testRoute.segments.get(j);
				if(fs->route->getId() != ts->route->getId()) {
					j++;	
				} else {
					break;
				}
			}
			if(j >= testRoute->segments.size()) {
				return false;
			}
	}	
	return true;
}

#endif _OSMAND_TRANSPORT_ROUTE_PLANNER_CPP