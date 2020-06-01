#ifndef _OSMAND_TRANSPORT_ROUTE_PLANNER_CPP
#define _OSMAND_TRANSPORT_ROUTE_PLANNER_CPP
#include "transportRoutePlanner.h"
#include "transportRouteResult.h"
#include "transportRouteResultSegment.h"
#include "transportRouteSegment.h"
#include "transportRoutingConfiguration.h"
#include "transportRoutingContext.h"
#include "transportRoutingObjects.h"
#include "Logging.h"

struct TransportSegmentsComparator
	: public std::binary_function<SHARED_PTR<TransportRouteSegment>&, SHARED_PTR<TransportRouteSegment>&, bool> {
	TransportSegmentsComparator() {}
	bool operator()(const SHARED_PTR<TransportRouteSegment>& lhs, const SHARED_PTR<TransportRouteSegment>& rhs) const {
		int cmp;
		if (lhs->distFromStart == rhs->distFromStart) {
			cmp = 0;
		} else {
			cmp = lhs->distFromStart < rhs->distFromStart ? -1 : 1;
		}
		return cmp > 0;
	}
};

TransportRoutePlanner::TransportRoutePlanner() {}

TransportRoutePlanner::~TransportRoutePlanner() {}

bool TransportRoutePlanner::includeRoute(SHARED_PTR<TransportRouteResult>& fastRoute,
										 SHARED_PTR<TransportRouteResult>& testRoute) {
	if (testRoute->segments.size() < fastRoute->segments.size()) {
		return false;
	}
	int32_t j = 0;
	for (int32_t i = 0; i < fastRoute->segments.size(); i++, j++) {
		SHARED_PTR<TransportRouteResultSegment>& fs = fastRoute->segments.at(i);
		while (j < testRoute->segments.size()) {
			SHARED_PTR<TransportRouteResultSegment>& ts = testRoute->segments[j];
			if (fs->route->id != ts->route->id) {
				j++;
			} else {
				break;
			}
		}
		if (j >= testRoute->segments.size()) {
			return false;
		}
	}
	return true;
}

void TransportRoutePlanner::prepareResults(
	unique_ptr<TransportRoutingContext>& ctx, vector<SHARED_PTR<TransportRouteSegment>>& results,
    vector<SHARED_PTR<TransportRouteResult>>& routes) {
	sort(results.begin(), results.end(),
		 [](const SHARED_PTR<TransportRouteSegment>& lhs, const SHARED_PTR<TransportRouteSegment>& rhs) {
			 return lhs->distFromStart < rhs->distFromStart;
		 });
    
	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info,
					  "Found %d results, visited %d routes / %d stops, loaded "
					  "%d tiles, loaded ways %d (%d wrong)",
					  results.size(), ctx->visitedRoutesCount, ctx->visitedStops, ctx->quadTree.size(), ctx->loadedWays,
					  ctx->wrongLoadedWays);

	for (SHARED_PTR<TransportRouteSegment>& res : results) {
		if (ctx->calculationProgress.get() && ctx->calculationProgress->isCancelled()) {
			return;
		}
        SHARED_PTR<TransportRouteResult> route(new TransportRouteResult(ctx->cfg));
		route->routeTime = res->distFromStart;
		route->finishWalkDist = res->walkDist;
		SHARED_PTR<TransportRouteSegment> p = res;
		while (p != nullptr) {
			if (ctx->calculationProgress != nullptr && ctx->calculationProgress->isCancelled()) {
				return;
			}
			if (p->parentRoute != nullptr) {
				unique_ptr<TransportRouteResultSegment> sg(new TransportRouteResultSegment());
				sg->route = p->parentRoute->road;
				sg->start = p->parentRoute->segStart;
				sg->end = p->parentStop;
				sg->walkDist = p->parentRoute->walkDist;
				sg->walkTime = sg->walkDist / ctx->cfg->walkSpeed;
				sg->depTime = p->departureTime;
				sg->travelDistApproximate = p->parentTravelDist;
				sg->travelTime = p->parentTravelTime;
				route->segments.insert(route->segments.begin(), std::move(sg));
			}
			p = p->parentRoute;
		}
		// test if faster routes fully included
		bool include = false;
		for (SHARED_PTR<TransportRouteResult>& s : routes) {
			if (ctx->calculationProgress.get() && ctx->calculationProgress->isCancelled()) {
				return;
			}
			if (includeRoute(s, route)) {
				include = true;
				route->toString();
				break;
			}
		}
		if (!include) {
			routes.push_back(std::move(route));
			// System.out.println(route.toString());
		}
	}
}

void TransportRoutePlanner::buildTransportRoute(unique_ptr<TransportRoutingContext>& ctx,
    											vector<SHARED_PTR<TransportRouteResult>>& res) {
	
	OsmAnd::ElapsedTimer pt_timer;
	pt_timer.Start();
	ctx->loadTime.Enable();
	ctx->searchTransportIndexTime.Enable();
	ctx->readTime.Enable();
	
	double totalDistance = getDistance(ctx->startLat, ctx->startLon, ctx->endLat, ctx->endLon);

	TransportSegmentsComparator trSegmComp;
	TRANSPORT_SEGMENTS_QUEUE queue(trSegmComp);
	vector<SHARED_PTR<TransportRouteSegment>> startStops;
	vector<SHARED_PTR<TransportRouteSegment>> endStops;
	UNORDERED(map)<int64_t, SHARED_PTR<TransportRouteSegment>> endSegments;
	vector<SHARED_PTR<TransportRouteSegment>> results;
	
	ctx->getTransportStops(ctx->startX, ctx->startY, false, startStops);
	ctx->getTransportStops(ctx->targetX, ctx->targetY, false, endStops);
	ctx->calcLatLons();
	
	for (SHARED_PTR<TransportRouteSegment>& s : endStops) {
		endSegments.insert({s->getId(), s});
	}

	if (startStops.size() == 0) { return; }

	for (SHARED_PTR<TransportRouteSegment>& r : startStops) {
		r->walkDist = getDistance(r->getLocationLat(), r->getLocationLon(), ctx->startLat, ctx->startLon);
		r->distFromStart = r->walkDist / ctx->cfg->walkSpeed;
		queue.push(r);
	}

	double finishTime = ctx->cfg->maxRouteTime;
	ctx->finishTimeSeconds = ctx->cfg->finishTimeSeconds;
	if (totalDistance > ctx->cfg->maxNumberOfChanges && ctx->cfg->maxRouteIncreaseSpeed > 0) {
		int increaseTime = (int) ((totalDistance - ctx->cfg->maxRouteDistance) 
			* 3.6 / ctx->cfg->maxRouteIncreaseSpeed);
		finishTime += increaseTime;
		ctx->finishTimeSeconds += increaseTime / 6;
	}
	
	double maxTravelTimeCmpToWalk = getDistance(ctx->startLat, ctx->startLon, ctx->endLat, ctx->endLon) 
									/ ctx->cfg->walkSpeed - ctx->cfg->changeTime / 2;
	
	while (queue.size() > 0) {
		if (ctx->calculationProgress != nullptr && ctx->calculationProgress->isCancelled()) {
			ctx->calculationProgress->setSegmentNotFound(0);
			return;
		}

		SHARED_PTR<TransportRouteSegment> segment = queue.top();
		queue.pop();
		SHARED_PTR<TransportRouteSegment> ex;
		
		if (ctx->visitedSegments.find(segment->getId()) != ctx->visitedSegments.end()) {
			ex = ctx->visitedSegments.find(segment->getId())->second;
			if (ex->distFromStart > segment->distFromStart) {
				OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "%.1f (%s) > %.1f (%s)", ex->distFromStart,
								  ex->to_string().c_str(), segment->distFromStart, segment->to_string().c_str());
			}
			continue;
		}
		ctx->visitedRoutesCount++;
		ctx->visitedSegments.insert({segment->getId(), segment});

		if (segment->getDepth() > ctx->cfg->maxNumberOfChanges + 1) {
			continue;
		}

		if (segment->distFromStart > finishTime + ctx->finishTimeSeconds ||
			segment->distFromStart > maxTravelTimeCmpToWalk) {
			break;
		}

		int64_t segmentId = segment->getId();
		SHARED_PTR<TransportRouteSegment> finish = nullptr;
		int64_t minDist = 0;
		int64_t travelDist = 0;
		double travelTime = 0;
		const float routeTravelSpeed = ctx->cfg->getSpeedByRouteType(segment->road->type);

		if (routeTravelSpeed == 0) {
			continue;
		}
		SHARED_PTR<TransportStop> prevStop = segment->getStop(segment->segStart);
		vector<SHARED_PTR<TransportRouteSegment>> sgms;

		for (int32_t ind = 1 + segment->segStart; ind < segment->getLength(); ind++) {
			if (ctx->calculationProgress != nullptr && ctx->calculationProgress->isCancelled()) {
				return;
			}
			segmentId++;
			ctx->visitedSegments.insert({segmentId, segment});
			SHARED_PTR<TransportStop> stop = segment->getStop(ind);
			double segmentDist = getDistance(prevStop->lat, prevStop->lon, stop->lat, stop->lon);
			travelDist += segmentDist;

			if (ctx->cfg->useSchedule) {
				// TransportSchedule& sc = segment->road->schedule;
				int interval = segment->road->schedule.avgStopIntervals.at(ind - 1);
				travelTime += interval * 10;
			} else {
				travelTime += ctx->cfg->stopTime + segmentDist / routeTravelSpeed;
			}
			if (segment->distFromStart + travelTime > finishTime + ctx->finishTimeSeconds) {
				break;
			}
			sgms.clear();
			ctx->getTransportStops(stop->x31, stop->y31, true, sgms);
			ctx->visitedStops++;
			for (SHARED_PTR<TransportRouteSegment>& sgm : sgms) {
				if (ctx->calculationProgress != nullptr && ctx->calculationProgress->isCancelled()) {
					return;
				}
				if (segment->wasVisited(sgm)) {
					continue;
				}
				SHARED_PTR<TransportRouteSegment> nextSegment = make_shared<TransportRouteSegment>(sgm);
				nextSegment->parentRoute = segment;
				nextSegment->parentStop = ind;
				nextSegment->walkDist =
					getDistance(nextSegment->getLocationLat(), nextSegment->getLocationLon(), stop->lat, stop->lon);
				nextSegment->parentTravelTime = travelTime;
				nextSegment->parentTravelDist = travelDist;
				double walkTime = nextSegment->walkDist / ctx->cfg->walkSpeed + ctx->cfg->getChangeTime() +
								  ctx->cfg->getBoardingTime();
				nextSegment->distFromStart = segment->distFromStart + travelTime + walkTime;
				if (ctx->cfg->useSchedule) {
					int tm = (sgm->departureTime - ctx->cfg->scheduleTimeOfDay) * 10;
					if (tm >= nextSegment->distFromStart) {
						nextSegment->distFromStart = tm;
						queue.push(nextSegment);
					}
				} else {
					queue.push(nextSegment);
				}
			}
			SHARED_PTR<TransportRouteSegment> finalSegment = nullptr;
			if (endSegments.find(segmentId) != endSegments.end()) {
				finalSegment = endSegments[segmentId];
			}
			double distToEnd = getDistance(stop->lat, stop->lon, ctx->endLat, ctx->endLon);

			if (finalSegment != nullptr && distToEnd < ctx->cfg->walkRadius) {
				if (finish == nullptr || minDist > distToEnd) {
					minDist = distToEnd;
					finish = make_shared<TransportRouteSegment>(finalSegment);
					finish->parentRoute = segment;
					finish->parentStop = ind;
					finish->walkDist = distToEnd;
					finish->parentTravelTime = travelTime;
					finish->parentTravelDist = travelDist;

					double walkTime = distToEnd / ctx->cfg->walkSpeed;
					finish->distFromStart = segment->distFromStart + travelTime + walkTime;
				}
			}
			prevStop = stop;
		}
		if (finish != nullptr) {
			if (finishTime > finish->distFromStart) {
				finishTime = finish->distFromStart;
			}
			if (finish->distFromStart < finishTime + ctx->finishTimeSeconds &&
				(finish->distFromStart < maxTravelTimeCmpToWalk || results.size() == 0)) {
				results.push_back(finish);
			}
		}

		if (ctx->calculationProgress != nullptr && ctx->calculationProgress->isCancelled()) {
			OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Route calculation interrupted");
			return;
		}

		// updateCalculationProgress(ctx, queue);
	}
	pt_timer.Pause();
	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info,
					  "[NATIVE] PT calculation took %.3f s, loading tiles "
					  "(overall): %.3f s, readTime : %.3f s",
					  (double)pt_timer.GetElapsedMs() / 1000.0, (double)ctx->loadTime.GetElapsedMs() / 1000.0,
					  (double)ctx->readTime.GetElapsedMs() / 1000.0);

	prepareResults(ctx, results, res);
}

void TransportRoutePlanner::updateCalculationProgress(unique_ptr<TransportRoutingContext>& ctx,
													  priority_queue<SHARED_PTR<TransportRouteSegment>>& queue) {
	if (ctx->calculationProgress.get()) {
		ctx->calculationProgress->directSegmentQueueSize = queue.size();
		if (queue.size() > 0) {
			SHARED_PTR<TransportRouteSegment> peek = queue.top();
			ctx->calculationProgress->distanceFromBegin =
				(int64_t)fmax(peek->distFromStart, ctx->calculationProgress->distanceFromBegin);
		}
	}
}

#endif	//_OSMAND_TRANSPORT_ROUTE_PLANNER_CPP
