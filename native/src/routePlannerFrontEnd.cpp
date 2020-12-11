#ifndef _OSMAND_ROUTE_PLANNER_FRONT_END_CPP
#define _OSMAND_ROUTE_PLANNER_FRONT_END_CPP
#include "routePlannerFrontEnd.h"
#include "binaryRoutePlanner.h"
#include "routingConfiguration.h"
#include "routeSegment.h"
#include "routeSegmentResult.h"
#include "routeResultPreparation.h"

SHARED_PTR<RoutingContext> RoutePlannerFrontEnd::buildRoutingContext(SHARED_PTR<RoutingConfiguration> config, RouteCalculationMode rm /*= RouteCalculationMode::NORMAL*/) {
	return SHARED_PTR<RoutingContext>(new RoutingContext(config, rm));
}

SHARED_PTR<RouteSegment> RoutePlannerFrontEnd::getRecalculationEnd(RoutingContext* ctx) {
	SHARED_PTR<RouteSegment> recalculationEnd;
	bool runRecalculation = ctx->previouslyCalculatedRoute.size() > 0 && ctx->config->recalculateDistance != 0;
	if (runRecalculation) {
		vector<SHARED_PTR<RouteSegmentResult> > rlist;
		float distanceThreshold = ctx->config->recalculateDistance;
		float threshold = 0;
		for (auto rr : ctx->previouslyCalculatedRoute) {
			threshold += rr->distance;
			if (threshold > distanceThreshold) {
				rlist.push_back(rr);
			}
		}
		runRecalculation = rlist.size() > 0;
		if (rlist.size() > 0) {
			SHARED_PTR<RouteSegment> previous;
			for (int i = 0; i <= rlist.size() - 1; i++) {
				auto rr = rlist[i];
				SHARED_PTR<RouteSegment> segment = std::make_shared<RouteSegment>(rr->object, rr->getEndPointIndex());
				if (previous) {
					previous->parentRoute = segment;
					previous->parentSegmentEnd = rr->getStartPointIndex();
				} else {
					recalculationEnd = segment;
				}
				previous = segment;
			}
		}
	}
	return recalculationEnd;
}

void refreshProgressDistance(RoutingContext* ctx) {
	if (ctx->progress) {
		ctx->progress->distanceFromBegin = 0;
		ctx->progress->distanceFromEnd = 0;
		ctx->progress->reverseSegmentQueueSize = 0;
		ctx->progress->directSegmentQueueSize = 0;
		float rd = (float) squareRootDist31(ctx->startX, ctx->startY, ctx->targetX, ctx->targetY);
		float speed = 0.9f * ctx->config->router->maxSpeed;
		ctx->progress->totalEstimatedDistance = (float) (rd / speed);
	}
}

double projectDistance(vector<SHARED_PTR<RouteSegmentResult> >& res, int k, int px, int py) {
	auto sr = res[k];
	auto r = sr->object;
	std::pair<int, int> pp = getProjectionPoint(px, py, r->pointsX[sr->getStartPointIndex()], r->pointsY[sr->getStartPointIndex()], r->pointsX[sr->getEndPointIndex()], r->pointsY[sr->getEndPointIndex()]);
	double currentsDist = squareRootDist31(pp.first, pp.second, px, py);
	return currentsDist;
}

void updateResult(SHARED_PTR<RouteSegmentResult>& routeSegmentResult, int px, int py, bool st) {
	int pind = st ? routeSegmentResult->getStartPointIndex() : routeSegmentResult->getEndPointIndex();
	
	auto r = routeSegmentResult->object;
	std::pair<int, int>* before = NULL;
	std::pair<int, int>* after = NULL;
	if (pind > 0) {
		before = new std::pair<int, int>(getProjectionPoint(px, py, r->pointsX[pind - 1], r->pointsY[pind - 1], r->pointsX[pind], r->pointsY[pind]));
	}
	if (pind < r->getPointsLength() - 1) {
		after = new std::pair<int, int>(getProjectionPoint(px, py, r->pointsX[pind + 1], r->pointsY[pind + 1], r->pointsX[pind], r->pointsY[pind]));
	}
	int insert = 0;
	double dd = measuredDist31(px, py, r->pointsX[pind], r->pointsY[pind]);
	double ddBefore = std::numeric_limits<double>::infinity();
	double ddAfter = std::numeric_limits<double>::infinity();
	std::pair<int, int>* i = NULL;
	if (before != NULL) {
		ddBefore = measuredDist31(px, py, before->first, before->second);
		if (ddBefore < dd) {
			insert = -1;
			i = before;
		}
	}
	
	if (after != NULL) {
		ddAfter = measuredDist31(px, py, after->first, after->second);
		if (ddAfter < dd && ddAfter < ddBefore) {
			insert = 1;
			i = after;
		}
	}
	
	if (insert != 0) {
		if (st && routeSegmentResult->getStartPointIndex() < routeSegmentResult->getEndPointIndex()) {
			routeSegmentResult->setEndPointIndex(routeSegmentResult->getEndPointIndex() + 1);
		}
		if (!st && routeSegmentResult->getStartPointIndex() > routeSegmentResult->getEndPointIndex()) {
			routeSegmentResult->setStartPointIndex(routeSegmentResult->getStartPointIndex() + 1);
		}
		if (insert > 0) {
			r->insert(pind + 1, i->first, i->second);
			if (st) {
				routeSegmentResult->setStartPointIndex(routeSegmentResult->getStartPointIndex() + 1);
			}
			if (!st) {
				routeSegmentResult->setEndPointIndex(routeSegmentResult->getEndPointIndex() + 1);
			}
		} else {
			r->insert(pind, i->first, i->second);
		}
	}
	if (before != NULL) {
		delete before;
	}
	if (after != NULL) {
		delete after;
	}
}


bool addSegment(int x31, int y31, RoutingContext* ctx, int indexNotFound, vector<SHARED_PTR<RouteSegmentPoint>>& res, bool transportStop) {
	auto f = findRouteSegment(x31, y31, ctx, transportStop);
	if (!f) {
		ctx->progress->segmentNotFound = indexNotFound;
		return false;
	} else {
		//OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "[Native] Route segment found %d %s", f->getRoad()->getId(), f->getRoad()->getName().c_str());
		res.push_back(f);
		return true;
	}
}

void makeStartEndPointsPrecise(vector<SHARED_PTR<RouteSegmentResult> >& res, int startX, int startY, int endX, int endY, vector<int> intermediatesX, vector<int> intermediatesY) {
	if (res.size() > 0) {
		updateResult(res[0], startX, startY, true);
		updateResult(res[res.size() - 1], endX, endY, false);
		if (!intermediatesX.empty()) {
			int k = 1;
			for (int i = 0; i < intermediatesX.size(); i++) {
				int px = intermediatesX[i];
				int py = intermediatesY[i];
				for (; k < res.size(); k++) {
					double currentsDist = projectDistance(res, k, px, py);
					if (currentsDist < 500 * 500) {
						for (int k1 = k + 1; k1 < res.size(); k1++) {
							double c2 = projectDistance(res, k1, px, py);
							if (c2 < currentsDist) {
								k = k1;
								currentsDist = c2;
							} else if (k1 - k > 15) {
								break;
							}
						}
						updateResult(res[k], px, py, false);
						if (k < res.size() - 1) {
							updateResult(res[k + 1], px, py, true);
						}
						break;
					}
				}
			}
		}
	}
}

vector<SHARED_PTR<RouteSegmentResult> > runRouting(RoutingContext* ctx, SHARED_PTR<RouteSegment> recalculationEnd) {
	refreshProgressDistance(ctx);
	
	OsmAnd::ElapsedTimer timer;
	timer.Start();

	vector<SHARED_PTR<RouteSegmentResult> > result = searchRouteInternal(ctx, false);
	
	timer.Pause();
	// OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "[Native] routing took %.3f seconds", (double)timer.GetElapsedMs() / 1000.0);

	if (recalculationEnd) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "[Native] use precalculated route");
		SHARED_PTR<RouteSegment> current = recalculationEnd;
		while (current->parentRoute) {
			SHARED_PTR<RouteSegment> pr = current->parentRoute;
			auto segmentResult = std::make_shared<RouteSegmentResult>(pr->road, current->parentSegmentEnd, pr->segmentStart);
			result.push_back(segmentResult);
			current = pr;
		}
	}
	if (!result.empty()) {
		for (auto seg : result) {
			seg->preAttachedRoutes = seg->attachedRoutes;
		}
	}
	if (ctx->finalRouteSegment && ctx->progress) {
		ctx->progress->routingCalculatedTime += ctx->finalRouteSegment->distanceFromStart;
	}

	return prepareResult(ctx, result);
}

vector<SHARED_PTR<RouteSegmentResult> > RoutePlannerFrontEnd::searchRouteInternalPrepare(RoutingContext* ctx, SHARED_PTR<RouteSegmentPoint> start, SHARED_PTR<RouteSegmentPoint> end, SHARED_PTR<PrecalculatedRouteDirection> routeDirection) {
	auto recalculationEnd = getRecalculationEnd(ctx);
	if (recalculationEnd) {
		ctx->initStartAndTargetPoints(start, recalculationEnd);
	} else {
		ctx->initStartAndTargetPoints(start, end);
	}
	if (routeDirection) {
		ctx->precalcRoute = routeDirection->adopt(ctx);
	}
	return runRouting(ctx, recalculationEnd);
}

vector<SHARED_PTR<RouteSegmentResult> > RoutePlannerFrontEnd::searchRoute(RoutingContext* ctx, vector<SHARED_PTR<RouteSegmentPoint>>& points, SHARED_PTR<PrecalculatedRouteDirection> routeDirection) {
	if (points.size() <= 2) {
		if (!useSmartRouteRecalculation) {
			ctx->previouslyCalculatedRoute.clear();
		}
		return searchRouteInternalPrepare(ctx, points[0], points[1], routeDirection);
	}
	
	vector<SHARED_PTR<RouteSegmentResult> > firstPartRecalculatedRoute;
	vector<SHARED_PTR<RouteSegmentResult> > restPartRecalculatedRoute;
	if (!ctx->previouslyCalculatedRoute.empty()) {
		auto prev = ctx->previouslyCalculatedRoute;
		int64_t id = points[1]->getRoad()->getId();
		uint16_t ss = points[1]->getSegmentStart();
		int px = points[1]->getRoad()->pointsX[ss];
		int py = points[1]->getRoad()->pointsY[ss];
		for (int i = 0; i < prev.size(); i++) {
			auto rsr = prev[i];
			if (id == rsr->object->getId()) {
				if (measuredDist31(rsr->object->pointsX[rsr->getEndPointIndex()], rsr->object->pointsY[rsr->getEndPointIndex()], px, py) < 50) {
					firstPartRecalculatedRoute.clear();
					restPartRecalculatedRoute.clear();
					for (int k = 0; k < prev.size(); k++) {
						if (k <= i) {
							firstPartRecalculatedRoute.push_back(prev[k]);
						} else {
							restPartRecalculatedRoute.push_back(prev[k]);
						}
					}
					OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "[Native] Recalculate only first part of the route");
					break;
				}
			}
		}
	}
	vector<SHARED_PTR<RouteSegmentResult> > results;
	for (int i = 0; i < points.size() - 1; i++) {
		RoutingContext local(ctx);
		if (i == 0) {
			if (useSmartRouteRecalculation) {
				local.previouslyCalculatedRoute = firstPartRecalculatedRoute;
			}
		}
		local.progress = ctx->progress;
		auto res = searchRouteInternalPrepare(&local, points[i], points[i + 1], routeDirection);
		
		results.insert(results.end(), res.begin(), res.end());
		
		local.unloadAllData(ctx);
		if (!restPartRecalculatedRoute.empty()) {
			results.insert(results.end(), restPartRecalculatedRoute.begin(), restPartRecalculatedRoute.end());
			break;
		}
	}
	ctx->unloadAllData();
	return results;
}

vector<SHARED_PTR<RouteSegmentResult> > RoutePlannerFrontEnd::searchRoute(SHARED_PTR<RoutingContext> ctx, int startX, int startY, int endX, int endY, vector<int>& intermediatesX, vector<int>& intermediatesY, SHARED_PTR<PrecalculatedRouteDirection> routeDirection) {
	
	if (!ctx->progress) {
		ctx->progress = std::make_shared<RouteCalculationProgress>();
	}
	bool intermediatesEmpty = intermediatesX.empty();
	/* TODO missing functionality for private access recalculation
	List<LatLon> targets = new ArrayList<>();
	targets.add(end);
	if (!intermediatesEmpty) {
		targets.addAll(intermediates);
	}
	if (needRequestPrivateAccessRouting(ctx, targets)) {
		ctx.calculationProgress.requestPrivateAccessRouting = true;
	}
	 */
	double maxDistance = measuredDist31(startX, startY, endX, endY);
	if (!intermediatesEmpty) {
		int x31 = startX;
		int y31 = startY;
		int ix31 = 0;
		int iy31 = 0;
		for (int i = 0; i < intermediatesX.size(); i++) {
			ix31 = intermediatesX[i];
			iy31 = intermediatesY[i];
			maxDistance = max(measuredDist31(x31, y31, ix31, iy31), maxDistance);
			x31 = ix31;
			y31 = iy31;
		}
	}
	if (ctx->calculationMode == RouteCalculationMode::COMPLEX && !routeDirection && maxDistance > ctx->config->DEVIATION_RADIUS * 6) {
		SHARED_PTR<RoutingContext> nctx = buildRoutingContext(ctx->config, RouteCalculationMode::BASE);
		nctx->progress = ctx->progress;
		vector<SHARED_PTR<RouteSegmentResult> > ls = searchRoute(nctx, startX, startY, endX, endY, intermediatesX, intermediatesY);
		routeDirection = PrecalculatedRouteDirection::build(ls, ctx->config->DEVIATION_RADIUS, ctx->config->router->maxSpeed);
	}
	
	if (intermediatesEmpty) {
		ctx->startX = startX;
		ctx->startY = startY;
		ctx->targetX = endX;
		ctx->targetY = endY;
		SHARED_PTR<RouteSegment> recalculationEnd = getRecalculationEnd(ctx.get());
		if (recalculationEnd) {
			ctx->initTargetPoint(recalculationEnd);
		}
		if (routeDirection) {
			ctx->precalcRoute = routeDirection->adopt(ctx.get());
		}
		auto res = runRouting(ctx.get(), recalculationEnd);
		if (!res.empty()) {
			printResults(ctx.get(), startX, startY, endX, endY, res);
		}
		makeStartEndPointsPrecise(res, startX, startY, endX, endY, intermediatesX, intermediatesY);
		return res;
	}
	int indexNotFound = 0;
	vector<SHARED_PTR<RouteSegmentPoint> > points;
	if (!addSegment(startX, startY, ctx.get(), indexNotFound++, points, ctx->startTransportStop)) {
		return vector<SHARED_PTR<RouteSegmentResult> >();
	}
	if (!intermediatesX.empty()) {
		for (int i = 0; i < intermediatesX.size(); i++) {
			int x31 = intermediatesX[i];
			int y31 = intermediatesY[i];
			if (!addSegment(x31, y31, ctx.get(), indexNotFound++, points, false)) {
				return vector<SHARED_PTR<RouteSegmentResult> >();
			}
		}
	}
	if (!addSegment(endX, endY, ctx.get(), indexNotFound++, points, ctx->targetTransportStop)) {
		return vector<SHARED_PTR<RouteSegmentResult> >();
	}
	auto res = searchRoute(ctx.get(), points, routeDirection);
	// make start and end more precise
	makeStartEndPointsPrecise(res, startX, startY, endX, endY, intermediatesX, intermediatesY);
	if (!res.empty()) {
		printResults(ctx.get(), startX, startY, endX, endY, res);
	}
	return res;
}

SHARED_PTR<RouteSegmentResult> RoutePlannerFrontEnd::generateStraightLineSegment(float averageSpeed, std::vector<pair<double, double>> points)
{
    RoutingIndex reg;
    reg.initRouteEncodingRule(0, "highway", "unmatched");
    SHARED_PTR<RouteDataObject> rdo;
    rdo->region = &reg;
    unsigned long size = points.size();
    
    vector<uint32_t> x(size);
    vector<uint32_t> y(size);
    double distance = 0;
    double distOnRoadToPass = 0;
    pair<double, double> prev = {NAN, NAN};
    for (int i = 0; i < size; i++) {
        const auto& l = points[i];
        if (l.first != NAN && l.second != NAN) {
            x.push_back(get31TileNumberX(l.second));
            y.push_back(get31TileNumberY(l.first));
            if (prev.first != NAN && prev.second != NAN)
            {
                double d = getDistance(l.first, l.second, prev.first, prev.second);
                distance += d;
                distOnRoadToPass += d / averageSpeed;
            }
        }
        prev = l;
    }
    rdo->pointsX = x;
    rdo->pointsY = y;
    rdo->types = { 0 };
    rdo->id = -1;
    SHARED_PTR<RouteSegmentResult> segment = make_shared<RouteSegmentResult>(rdo, 0, rdo->getPointsLength() - 1);
    segment->segmentTime = (float) distOnRoadToPass;
    segment->segmentSpeed = (float) averageSpeed;
    segment->distance = (float) distance;
    segment->turnType = TurnType::ptrStraight();
    return segment;
}

#endif /*_OSMAND_ROUTE_PLANNER_FRONT_END_CPP*/
