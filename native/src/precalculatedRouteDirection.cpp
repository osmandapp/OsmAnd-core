#ifndef _OSMAND_PRECALCULATED_ROUTE_DIRECTION_CPP
#define _OSMAND_PRECALCULATED_ROUTE_DIRECTION_CPP
#include "precalculatedRouteDirection.h"
#include "routingContext.h"

PrecalculatedRouteDirection::PrecalculatedRouteDirection(vector<SHARED_PTR<RouteSegmentResult> >& ls, float maxSpeed) {
    init();
    
    this->maxSpeed = maxSpeed;
    init(ls);
}

PrecalculatedRouteDirection::PrecalculatedRouteDirection(vector<int>& x31, vector<int>& y31, float maxSpeed) {
    init();

    this->maxSpeed = maxSpeed;
    init(x31, y31);
}

PrecalculatedRouteDirection::PrecalculatedRouteDirection(PrecalculatedRouteDirection& parent, int s1, int s2) {
    init();

    this->minSpeed = parent.minSpeed;
    this->maxSpeed = parent.maxSpeed;
    bool inverse = false;
    if (s1 > s2) {
        int tmp = s1;
        s1 = s2;
        s2 = tmp;
        inverse = true;
    }
    times.assign(s2 - s1 + 1, .0f);
    pointsX.assign(s2 - s1 + 1, 0);
    pointsY.assign(s2 - s1 + 1, 0);
    for (int i = s1; i <= s2; i++) {
        int shiftInd = i - s1;
        pointsX[shiftInd] = parent.pointsX[i];
        pointsY[shiftInd] = parent.pointsY[i];
        //			indexedPoints.registerObjectXY(parent.pointsX.get(i), parent.pointsY.get(i), pointsX.size() - 1);
        SkRect rct = SkRect::MakeLTRB(parent.pointsX[i], parent.pointsY[i], parent.pointsX[i], parent.pointsY[i]);
        quadTree.insert(shiftInd, rct);
        times[shiftInd] = parent.times[i] - parent.times[inverse ? s1 : s2];
    }
}

void PrecalculatedRouteDirection::init() {
    minSpeed = 0.f;
    maxSpeed = 0.f;
    startFinishTime = 0.f;
    endFinishTime = 0.f;
    followNext = false;
    startPoint = 0;
    endPoint = 0;
}

SHARED_PTR<PrecalculatedRouteDirection> PrecalculatedRouteDirection::build(vector<SHARED_PTR<RouteSegmentResult> >& ls, float cutoffDistance, float maxSpeed) {
    int begi = 0;
    float d = cutoffDistance;
    for (; begi < ls.size(); begi++) {
        d -= ls[begi]->distance;
        if (d < 0) {
            break;
        }
    }
    int endi = (int)ls.size();
    d = cutoffDistance;
    for (; endi > 0; endi--) {
        d -= ls[endi - 1]->distance;
        if (d < 0) {
            break;
        }
    }
    if (begi < endi) {
        vector<SHARED_PTR<RouteSegmentResult> > sublist(ls.begin() + begi, ls.begin() + endi);
        return SHARED_PTR<PrecalculatedRouteDirection>(new PrecalculatedRouteDirection(sublist, maxSpeed));
    }
    return nullptr;
}

SHARED_PTR<PrecalculatedRouteDirection> PrecalculatedRouteDirection::build(vector<int>& x31, vector<int>& y31, float maxSpeed) {
    return SHARED_PTR<PrecalculatedRouteDirection>(new PrecalculatedRouteDirection(x31, y31, maxSpeed));
}

void PrecalculatedRouteDirection::init(vector<int>& x31, vector<int>& y31) {
    vector<float> speedSegments;
    for (int i = 0; i < x31.size(); i++ ) {
        float routeSpd = maxSpeed; // (s.getDistance() / s.getRoutingTime())
        speedSegments.push_back(routeSpd);
    }
    init(x31, y31, speedSegments);
}

void PrecalculatedRouteDirection::init(vector<int>& x31, vector<int>& y31, vector<float>& speedSegments) {
    float totaltm = 0;
    vector<float> times;
    for (int i = 0; i < x31.size(); i++) {
        // MapUtils.measuredDist31 vs BinaryRoutePlanner.squareRootDist
        // use measuredDist31 because we use precise s.getDistance() to calculate routeSpd
        int ip = i == 0 ? 0 : i - 1;
        float dist = (float) measuredDist31(x31[ip], y31[ip], x31[i], y31[i]);
        float tm = dist / speedSegments[i];// routeSpd;
        times.push_back(tm);
        SkRect rct = SkRect::MakeLTRB(x31[i], y31[i], x31[i], y31[i]);
        quadTree.insert(i, rct);
        // indexedPoints.registerObjectXY();
        totaltm += tm;
    }
    pointsX.clear();
    pointsY.clear();
    this->times.clear();
    pointsX.insert(pointsX.end(), x31.begin(), x31.end());
    pointsY.insert(pointsY.end(), y31.begin(), y31.end());
    float totDec = totaltm;
    for(int i = 0; i < times.size(); i++) {
        totDec -= times[i];
        this->times.push_back(totDec);
    }
}

void PrecalculatedRouteDirection::init(vector<SHARED_PTR<RouteSegmentResult> >& ls) {
    vector<int> x31;
    vector<int> y31;
    vector<float> speedSegments;
    for (auto s : ls) {
        bool plus = s->getStartPointIndex() < s->getEndPointIndex();
        int i = s->getStartPointIndex();
        SHARED_PTR<RouteDataObject> obj = s->object;
        float routeSpd = (s->routingTime == 0 || s->distance == 0) ? maxSpeed : (s->distance / s->routingTime);
        while (true) {
            i = plus? i + 1 : i -1;
            x31.push_back(obj->pointsX[i]);
            y31.push_back(obj->pointsY[i]);
            speedSegments.push_back(routeSpd);
            if (i == s->getEndPointIndex()) {
                break;
            }
        }
    }
    init(x31, y31, speedSegments);
}

SHARED_PTR<PrecalculatedRouteDirection> PrecalculatedRouteDirection::adopt(RoutingContext* ctx) {
    int ind1 = getIndex(ctx->startX, ctx->startY);
    int ind2 = getIndex(ctx->targetX, ctx->targetY);
    minSpeed = ctx->config->router->minSpeed;
    defaultSpeed = ctx->config->router->defaultSpeed;
    maxSpeed = ctx->config->router->maxSpeed;
    if (ind1 == -1) {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "! Illegal argument ");
        return nullptr;
    }
    if (ind2 == -1) {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "! Illegal argument ");
        return nullptr;
    }
    SHARED_PTR<PrecalculatedRouteDirection> routeDirection = std::make_shared<PrecalculatedRouteDirection>(*this, ind1, ind2);
    routeDirection->startPoint = calc(ctx->startX, ctx->startY);
    routeDirection->startFinishTime = (float) squareRootDist31(pointsX[ind1], pointsY[ind1], ctx->startX, ctx->startY) / maxSpeed;
    //		routeDirection.startX31 = ctx.startX;
    //		routeDirection.startY31 = ctx.startY;
    routeDirection->endPoint = calc(ctx->targetX, ctx->targetY);
    routeDirection->endFinishTime = (float) squareRootDist31(pointsX[ind2], pointsY[ind2], ctx->targetX, ctx->targetY) / maxSpeed;
    routeDirection->followNext = followNext;
    
    //		routeDirection.endX31 = ctx.targetX;
    //		routeDirection.endY31 = ctx.targetY;
    return routeDirection;
}

float PrecalculatedRouteDirection::getDeviationDistance(int x31, int y31) {
	int ind = getIndex(x31, y31);
	if(ind == -1) {
		return 0;
	}
	return getDeviationDistance(x31, y31, ind);
}

float PrecalculatedRouteDirection::getDeviationDistance(int x31, int y31, int ind) {
	float distToPoint = 0; //squareRootDist(x31, y31, pointsX.get(ind), pointsY.get(ind));
	if(ind < (int)pointsX.size() - 1 && ind != 0) {
		double nx = squareRootDist31(x31, y31, pointsX[ind + 1], pointsY[ind + 1]);
		double pr = squareRootDist31(x31, y31, pointsX[ind - 1], pointsY[ind - 1]);
		int nind =  nx > pr ? ind -1 : ind +1;
		std::pair<int, int> proj = getProjectionPoint(x31, y31, pointsX[ind], pointsY[ind], pointsX[nind], pointsX[nind]);
		distToPoint = (float) squareRootDist31(x31, y31, (int)proj.first, (int)proj.second) ;
	}
	return distToPoint;
}

int PrecalculatedRouteDirection::SHIFT = (1 << (31 - 17));
int PrecalculatedRouteDirection::SHIFTS[] = {1 << (31 - 15), 1 << (31 - 13), 1 << (31 - 12), 
		1 << (31 - 11), 1 << (31 - 7)};
int PrecalculatedRouteDirection::getIndex(int x31, int y31) {
	int ind = -1;
	vector<int> cachedS;
	SkRect rct = SkRect::MakeLTRB(x31 - SHIFT, y31 - SHIFT, x31 + SHIFT, y31 + SHIFT);
	quadTree.query_in_box(rct, cachedS);
	if (cachedS.size() == 0) {
		for (uint k = 0; k < 5 /* SHIFTS.size()*/; k++) {
			rct = SkRect::MakeLTRB(x31 - SHIFTS[k], y31 - SHIFTS[k], x31 + SHIFTS[k], y31 + SHIFTS[k]);
			quadTree.query_in_box(rct, cachedS);
			if (cachedS.size() != 0) {
				break;
			}
		}
		if (cachedS.size() == 0) {
			return -1;
		}
	}
	double minDist = 0;
	for (uint i = 0; i < cachedS.size(); i++) {
		int n = cachedS[i];
		double ds = squareRootDist31(x31, y31, pointsX[n], pointsY[n]);
		if (ds < minDist || i == 0) {
			ind = n;
			minDist = ds;
		}
	}
	return ind;
}

float PrecalculatedRouteDirection::timeEstimate(int sx31, int sy31, int ex31, int ey31) {
	uint64_t l1 = calc(sx31, sy31);
	uint64_t l2 = calc(ex31, ey31);
	int x31 = sx31;
	int y31 = sy31;
	bool start = false;
	if(l1 == startPoint || l1 == endPoint) {
		start = l1 == startPoint;
		x31 = ex31;
		y31 = ey31;
	} else if(l2 == startPoint || l2 == endPoint) {
		start = l2 == startPoint;
		x31 = sx31;
		y31 = sy31;
	} else {
		//OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "! Alert unsupported time estimate ");
		return -2;
	}
	int ind = getIndex(x31, y31);
	if(ind == -1) {
		return -1;
	}
	if((ind == 0 && start) || 
			(ind == (int)pointsX.size() - 1 && !start)) {
		return -1;
	}
	float distToPoint = getDeviationDistance(x31, y31, ind);
	float deviationPenalty = distToPoint / minSpeed;
    float finishTime = (start? startFinishTime : endFinishTime);
	if(start) {
		return (times[0] - times[ind]) +  deviationPenalty + finishTime;
	} else {
		return times[ind] + deviationPenalty + finishTime;
	}
}

#endif /*_OSMAND_PRECALCULATED_ROUTE_DIRECTION_CPP*/
