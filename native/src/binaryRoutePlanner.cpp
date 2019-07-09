#include <queue>
#include <iterator>
#include "binaryRead.h"
#include "binaryRoutePlanner.h"
#include "routingContext.h"
#include <functional>

#include "Logging.h"

//	static bool PRINT_TO_CONSOLE_ROUTE_INFORMATION_TO_TEST = true;

static const bool TRACE_ROUTING = false;
static const double GPS_POSSIBLE_ERROR = 10;

inline int roadPriorityComparator(float o1DistanceFromStart, float o1DistanceToEnd, float o2DistanceFromStart,
		float o2DistanceToEnd, float heuristicCoefficient) {
	// f(x) = g(x) + h(x)  --- g(x) - distanceFromStart, h(x) - distanceToEnd (not exact)
	float f1 = o1DistanceFromStart + heuristicCoefficient * o1DistanceToEnd;
	float f2 = o2DistanceFromStart + heuristicCoefficient * o2DistanceToEnd;
	if (f1 == f2) {
		return 0;
	}
	return f1 < f2 ? -1 : 1;
}

void printRoad(const char* prefix, SHARED_PTR<RouteSegment>& segment) {
	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "%s Road id=%lld dir=%d ind=%d ds=%f es=%f pend=%d parent=%lld",
		prefix, segment->road->id / 64, 
		segment->directionAssgn, segment->getSegmentStart(),
		segment->distanceFromStart, segment->distanceToEnd, 
		segment->parentRoute.get() != NULL? segment->parentSegmentEnd : 0,
		segment->parentRoute.get() != NULL? segment->parentRoute->road->id : 0);	
}

int64_t calculateRoutePointId(SHARED_PTR<RouteDataObject>& road, int intervalId, bool positive) {
	return (road->id << ROUTE_POINTS) + (intervalId << 1) + (positive ? 1 : 0);
}

int64_t calculateRoutePointId(SHARED_PTR<RouteSegment>& segm, bool direction) {
	if(segm->getSegmentStart() == 0 && !direction) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Assert failed route point id  0");
	}
	if(segm->getSegmentStart() == segm->road->getPointsLength() - 1 && direction) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Assert failed route point length");
	}
	return calculateRoutePointId(segm->getRoad(),
				direction ? segm->getSegmentStart() : segm->getSegmentStart() - 1, direction);
}

static double h(RoutingContext* ctx, int begX, int begY, int endX, int endY) {
	double distToFinalPoint = squareRootDist31(begX, begY,  endX, endY);
	double result = distToFinalPoint /  ctx->config->router->getMaxSpeed();
	if(ctx->precalcRoute != nullptr){
		float te = ctx->precalcRoute->timeEstimate(begX, begY,  endX, endY);
		if(te > 0) return te;
	}
	return result;
}

struct SegmentsComparator: public std::binary_function<SHARED_PTR<RouteSegment>&, SHARED_PTR<RouteSegment>&, bool>
{
	RoutingContext* ctx;
	SegmentsComparator(RoutingContext* c) : ctx(c) {

	}
	bool operator()(const SHARED_PTR<RouteSegment>& lhs, const SHARED_PTR<RouteSegment>& rhs) const
	{
		int cmp = roadPriorityComparator(lhs.get()->distanceFromStart, lhs.get()->distanceToEnd, rhs.get()->distanceFromStart,
		    			rhs.get()->distanceToEnd, ctx->getHeuristicCoefficient());
    	return cmp > 0;
    }
};
struct NonHeuristicSegmentsComparator: public std::binary_function<SHARED_PTR<RouteSegment>&, SHARED_PTR<RouteSegment>&, bool>
{
	bool operator()(const SHARED_PTR<RouteSegment>& lhs, const SHARED_PTR<RouteSegment>& rhs) const
	{
		return roadPriorityComparator(lhs.get()->distanceFromStart, lhs.get()->distanceToEnd, rhs.get()->distanceFromStart, rhs.get()->distanceToEnd, 0.5) > 0;
	}
};

typedef UNORDERED(map)<int64_t, SHARED_PTR<RouteSegment> > VISITED_MAP;
typedef priority_queue<SHARED_PTR<RouteSegment>, vector<SHARED_PTR<RouteSegment> >, SegmentsComparator > SEGMENTS_QUEUE;
void processRouteSegment(RoutingContext* ctx, bool reverseWaySearch, SEGMENTS_QUEUE& graphSegments,
		VISITED_MAP& visitedSegments, SHARED_PTR<RouteSegment>& segment,
		VISITED_MAP& oppositeSegments, bool direction);

SHARED_PTR<RouteSegment> processIntersections(RoutingContext* ctx, SEGMENTS_QUEUE& graphSegments, VISITED_MAP& visitedSegments,
		double distFromStart, SHARED_PTR<RouteSegment>& segment,int segmentPoint, SHARED_PTR<RouteSegment>& inputNext,
		bool reverseWaySearch, bool doNotAddIntersections, bool* processFurther);

void processOneRoadIntersection(RoutingContext* ctx, SEGMENTS_QUEUE& graphSegments,
			VISITED_MAP& visitedSegments, double distFromStart, double distanceToEnd,  
			SHARED_PTR<RouteSegment>& segment, int segmentPoint, SHARED_PTR<RouteSegment>& next);


int calculateSizeOfSearchMaps(SEGMENTS_QUEUE& graphDirectSegments, SEGMENTS_QUEUE& graphReverseSegments,
		VISITED_MAP& visitedDirectSegments, VISITED_MAP& visitedOppositeSegments) {
	int sz = visitedDirectSegments.size() * sizeof(pair<int64_t, SHARED_PTR<RouteSegment> > );
	sz += visitedOppositeSegments.size()*sizeof(pair<int64_t, SHARED_PTR<RouteSegment> >);
	sz += graphDirectSegments.size()*sizeof(SHARED_PTR<RouteSegment>);
	sz += graphReverseSegments.size()*sizeof(SHARED_PTR<RouteSegment>);
	return sz;
}

SHARED_PTR<RouteSegment> loadSameSegment(RoutingContext* ctx, SHARED_PTR<RouteSegment> segment, int ind) {
	int x31 = segment->getRoad()->pointsX[ind];
	int y31 = segment->getRoad()->pointsY[ind];
	SHARED_PTR<RouteSegment> s = ctx->loadRouteSegment(x31, y31);
	while(s.get() != NULL) {
		if(s->getRoad()->getId() == segment->getRoad()->getId()) {
			segment = s;
			break;
		}
		s = s->next;
	}
	return segment;
}


SHARED_PTR<RouteSegment> initRouteSegment(RoutingContext* ctx, SHARED_PTR<RouteSegment> segment, bool positiveDirection) {
	if(segment->getSegmentStart() == 0 && !positiveDirection && segment->getRoad()->getPointsLength() > 0) {
		segment = loadSameSegment(ctx, segment, 1);
	// } else if(segment->getSegmentStart() == segment->getRoad()->getPointsLength() -1 && positiveDirection && segment->getSegmentStart() > 0) {
	// assymetric cause we calculate initial point differently (segmentStart means that point is between ]segmentStart-1, segmentStart]
	} else if(segment->getSegmentStart() > 0 && positiveDirection) {
		segment = loadSameSegment(ctx, segment, segment->getSegmentStart() -1);
	}
	if(segment.get() == NULL) {
		return segment;
	}
	return RouteSegment::initRouteSegment(segment, positiveDirection);
}



void initQueuesWithStartEnd(RoutingContext* ctx,  SHARED_PTR<RouteSegment> start, SHARED_PTR<RouteSegment> end,
			SEGMENTS_QUEUE& graphDirectSegments, SEGMENTS_QUEUE& graphReverseSegments) {
		SHARED_PTR<RouteSegment> startPos = initRouteSegment(ctx, start, true);
		SHARED_PTR<RouteSegment> startNeg = initRouteSegment(ctx, start, false);
		SHARED_PTR<RouteSegment> endPos = initRouteSegment(ctx, end, true);
		SHARED_PTR<RouteSegment> endNeg = initRouteSegment(ctx, end, false);


		// for start : f(start) = g(start) + h(start) = 0 + h(start) = h(start)
		if (ctx->config->initialDirection > -180 && ctx->config->initialDirection < 180) {
			ctx->firstRoadId = (start->road->id << ROUTE_POINTS) + start->getSegmentStart();
			double plusDir = start->road->directionRoute(start->getSegmentStart(), true);
			double diff = plusDir - ctx->config->initialDirection;
			if (abs(alignAngleDifference(diff)) <= M_PI / 3) {
				if (startNeg.get() != NULL) {
					startNeg->distanceFromStart += 500;
				}
			} else if (abs(alignAngleDifference(diff - M_PI )) <= M_PI / 3) {
				if (startPos.get() != NULL) {
					startPos->distanceFromStart += 500;
				}
			}
		}
		//int targetEndX = end->road->pointsX[end->segmentStart];
		//int targetEndY = end->road->pointsY[end->segmentStart];
		//int startX = start->road->pointsX[start->segmentStart];
		//int startY = start->road->pointsY[start->segmentStart];
	
		float estimatedDistance = (float) h(ctx, ctx->startX, ctx->startY, ctx->targetX, ctx->targetY);
		if(startPos.get() != NULL) {
			startPos->distanceToEnd = estimatedDistance;
			graphDirectSegments.push(startPos);
		}
		if(startNeg.get() != NULL) {
			startNeg->distanceToEnd = estimatedDistance;
			graphDirectSegments.push(startNeg);
		}
		if(endPos.get() != NULL) {
			endPos->distanceToEnd = estimatedDistance;
			graphReverseSegments.push(endPos);
		}
		if(endNeg.get() != NULL) {
			endNeg->distanceToEnd = estimatedDistance;
			graphReverseSegments.push(endNeg);
		}
}


bool checkIfGraphIsEmpty(RoutingContext* ctx, bool allowDirection, 
			SEGMENTS_QUEUE& graphSegments,  SHARED_PTR<RouteSegmentPoint>& pnt,  VISITED_MAP& visited, string msg) {
		if (allowDirection && graphSegments.size() == 0) {
			if (pnt->others.size() > 0) {
				vector<SHARED_PTR<RouteSegmentPoint> >::iterator pntIterator = pnt->others.begin();
				while (pntIterator != pnt->others.end()) {
					SHARED_PTR<RouteSegment> next = *pntIterator;
					bool visitedAlready = false;
					if (next->getSegmentStart() > 0 && visited.find(calculateRoutePointId(next, false)) != visited.end()) {
						visitedAlready = true;
					} else if (next->getSegmentStart() < next->getRoad()->getPointsLength() - 1
							&& visited.find(calculateRoutePointId(next, true)) != visited.end()) {
						visitedAlready = true;
					}
					pntIterator = pnt->others.erase(pntIterator);
					if (!visitedAlready) {
						float estimatedDistance = (float) h(ctx, ctx->startX, ctx->startY, ctx->targetX, ctx->targetY);
						SHARED_PTR<RouteSegment> pos = RouteSegment::initRouteSegment(next, true);
						SHARED_PTR<RouteSegment> neg = RouteSegment::initRouteSegment(next, false);
						if (pos.get() != NULL) {
							pos->distanceToEnd = estimatedDistance;
							graphSegments.push(pos);
						}
						if (neg.get() != NULL) {
							neg->distanceToEnd = estimatedDistance;
							graphSegments.push(neg);
						}
						OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Reiterate point with new start/destination ");						
						printRoad("Reiterate point ", next);
						break;
					}
				}
				if (graphSegments.size() == 0) {
					OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Route is not found to selected target point.");
					return true;
				}
			}
		}
		return false;
	}

/**
 * Calculate route between start.segmentEnd and end.segmentStart (using A* algorithm)
 * return list of segments
 */
SHARED_PTR<RouteSegment> searchRouteInternal(RoutingContext* ctx, SHARED_PTR<RouteSegmentPoint>& start, SHARED_PTR<RouteSegmentPoint>& end, bool leftSideNavigation) {
	// FIXME intermediate points
	// measure time
	ctx->visitedSegments = 0;
	int iterationsToUpdate = 0;
	ctx->timeToCalculate.Start();
	

	SegmentsComparator sgmCmp(ctx);
	NonHeuristicSegmentsComparator nonHeuristicSegmentsComparator;
	SEGMENTS_QUEUE graphDirectSegments(sgmCmp);
	SEGMENTS_QUEUE graphReverseSegments(sgmCmp);

	// Set to not visit one segment twice (stores road.id << X + segmentStart)
	VISITED_MAP visitedDirectSegments;
	VISITED_MAP visitedOppositeSegments;

	initQueuesWithStartEnd(ctx, start, end, graphDirectSegments, graphReverseSegments);

	// Extract & analyze segment with min(f(x)) from queue while final segment is not found
	bool forwardSearch = true;
	
	SEGMENTS_QUEUE * graphSegments = &graphDirectSegments;;	
	bool onlyBackward = ctx->getPlanRoadDirection() < 0;
	bool onlyForward = ctx->getPlanRoadDirection() > 0;

	SHARED_PTR<RouteSegment> finalSegment;
	while (graphSegments->size() > 0) {
		SHARED_PTR<RouteSegment> segment = graphSegments->top();
		graphSegments->pop();
		// Memory management
		// ctx.memoryOverhead = calculateSizeOfSearchMaps(graphDirectSegments, graphReverseSegments, visitedDirectSegments, visitedOppositeSegments);	
		if(TRACE_ROUTING){
			printRoad(forwardSearch?"F>" :"B>", segment);
		}
		if(segment->isFinal()) {
			finalSegment = segment;
			ctx->finalRouteSegment = segment;
			if(TRACE_ROUTING) {
				OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "Final segment found");
			}
			break;
		}

		ctx->visitedSegments++;		
		if (forwardSearch) {
			bool doNotAddIntersections = onlyBackward;
			processRouteSegment(ctx, false, graphDirectSegments, visitedDirectSegments, 
						segment, visitedOppositeSegments, doNotAddIntersections);			
		} else {
			bool doNotAddIntersections = onlyForward;
			processRouteSegment(ctx, true, graphReverseSegments, visitedOppositeSegments, segment,
						visitedDirectSegments, doNotAddIntersections);
		}
		if(ctx->progress.get() && iterationsToUpdate-- < 0) {
			iterationsToUpdate = 100;
			ctx->progress->updateStatus(graphDirectSegments.empty()? 0 :graphDirectSegments.top()->distanceFromStart,
					graphDirectSegments.size(),
					graphReverseSegments.empty()? 0 :graphReverseSegments.top()->distanceFromStart,
					graphReverseSegments.size());
			if(ctx->progress->isCancelled()) {
				break;
			}
		}
		if(checkIfGraphIsEmpty(ctx, ctx->getPlanRoadDirection() <= 0, graphReverseSegments, end, visitedOppositeSegments,
					"Route is not found to selected target point.")) {
			return finalSegment;
		}
		if(checkIfGraphIsEmpty(ctx, ctx->getPlanRoadDirection() >= 0, graphDirectSegments, start, visitedDirectSegments,
					"Route is not found from selected start point.")) {
			return finalSegment;
		}		
		if (ctx->planRouteIn2Directions()) {
			forwardSearch = !nonHeuristicSegmentsComparator(graphDirectSegments.top(), graphReverseSegments.top());
			//if (graphDirectSegments.size() * 2 > graphReverseSegments.size()) {
			//	forwardSearch = false;
			//} else if (graphDirectSegments.size() < 2 * graphReverseSegments.size()) {
			//	forwardSearch = true;
			//}
		} else {

			// different strategy : use onedirectional graph
			forwardSearch = onlyForward;
			if(onlyBackward && graphDirectSegments.size() > 0) {
				forwardSearch = true;
			}
			if(onlyForward && graphReverseSegments.size() > 0) {
				forwardSearch = false;
			}
		}
		if (forwardSearch) {
			graphSegments = &graphDirectSegments;
		} else {
			graphSegments = &graphReverseSegments;
		}

		// check if interrupted
		if(ctx->isInterrupted()) {
			return finalSegment;
		}
	}
	ctx->timeToCalculate.Pause();
	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "[Native] Result visited (visited roads %d, visited segments %d / %d , queue sizes %d / %d ) ",
			ctx-> visitedSegments, visitedDirectSegments.size(), visitedOppositeSegments.size(),
			graphDirectSegments.size(),graphReverseSegments.size());
	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "[Native] Result timing (time to load %d, time to calc %d, loaded tiles %d) ", (int)ctx->timeToLoad.GetElapsedMs()
			, (int)ctx->timeToCalculate.GetElapsedMs(), ctx->loadedTiles);
	int sz = calculateSizeOfSearchMaps(graphDirectSegments, graphReverseSegments, visitedDirectSegments, visitedOppositeSegments);
	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "[Native] Memory occupied (Routing context %d Kb, search %d Kb)", ctx->getSize()/ 1024, sz/1024);
	return finalSegment;
}

bool checkIfInitialMovementAllowedOnSegment(RoutingContext* ctx, bool reverseWaySearch,
			VISITED_MAP& visitedSegments, SHARED_PTR<RouteSegment>& segment, SHARED_PTR<RouteDataObject>& road) {
	bool directionAllowed;
	int oneway = ctx->config->router->isOneWay(road);
	// use positive direction as agreed
	if (!reverseWaySearch) {
			if(segment->isPositive()){
				directionAllowed = oneway >= 0;
			} else {
				directionAllowed = oneway <= 0;
			}
	} else {
		if(segment->isPositive()){
			directionAllowed = oneway <= 0;
		} else {
			directionAllowed = oneway >= 0;
		}
	}
	VISITED_MAP::iterator mit = visitedSegments.find(calculateRoutePointId(segment, segment->isPositive()));
	if(directionAllowed && mit != visitedSegments.end() && mit->second.get() != NULL) {
		if(mit->second.get()->distanceFromStart <= segment->distanceFromStart) {
			directionAllowed = false;
		}
	}

	return directionAllowed;
}


float calculateTimeWithObstacles(RoutingContext* ctx, SHARED_PTR<RouteDataObject>& road, float distOnRoadToPass, float obstaclesTime) {
	float priority = ctx->config->router->defineSpeedPriority(road);
	float speed = ctx->config->router->defineRoutingSpeed(road) * priority;
	if (speed == 0) {
		speed = ctx->config->router->getDefaultSpeed();
		if(priority > 0) {
			speed *= priority;
		}
	}
	// speed can not exceed max default speed according to A*
	if(speed > ctx->config->router->getMaxSpeed()) {
		speed = ctx->config->router->getMaxSpeed();
	}
	return obstaclesTime + distOnRoadToPass / speed;
}

bool checkViaRestrictions(SHARED_PTR<RouteSegment>& from, SHARED_PTR<RouteSegment>& to) {
    if(from.get() != NULL && to.get() != NULL) {
        int64_t fid = to->getRoad()->getId();
        for(uint i = 0; i < from->getRoad()->restrictions.size(); i++) {
            int64_t id = from->getRoad()->restrictions[i].to;
            if(fid == id) {
                int tp = from->getRoad()->restrictions[i].type;
                if(tp == RESTRICTION_NO_LEFT_TURN || 
                   tp == RESTRICTION_NO_RIGHT_TURN || 
                   tp == RESTRICTION_NO_STRAIGHT_ON || 
                   tp == RESTRICTION_NO_U_TURN) {
                   return false;
                }
                break;
            }
        }
    }
    return true;
}

SHARED_PTR<RouteSegment> getParentDiffId(SHARED_PTR<RouteSegment> s) {
    while(s->parentRoute.get() != NULL && s->parentRoute->getRoad()->id == s->getRoad()->id) {
            s = s->parentRoute;
    }
    return s->parentRoute;
}
               

bool checkIfOppositieSegmentWasVisited(RoutingContext* ctx, bool reverseWaySearch, SEGMENTS_QUEUE& graphSegments,
		SHARED_PTR<RouteSegment>& segment, VISITED_MAP& oppositeSegments,
		 int segmentPoint, float segmentDist, float obstaclesTime) {
	SHARED_PTR<RouteDataObject> road = segment -> getRoad();
	int64_t opp = calculateRoutePointId(road, segment->isPositive() ? segmentPoint - 1 : segmentPoint, !segment->isPositive());
	VISITED_MAP::iterator opIt = oppositeSegments.find(opp);
	if (opIt != oppositeSegments.end() && opIt->second.get() != NULL ) {
		SHARED_PTR<RouteSegment> opposite = opIt->second;
		SHARED_PTR<RouteSegment> to = reverseWaySearch ? getParentDiffId(segment) : getParentDiffId(opposite);
        SHARED_PTR<RouteSegment> from = !reverseWaySearch ? getParentDiffId(segment) : getParentDiffId(opposite);
        if (checkViaRestrictions(from, to)) {			
            SHARED_PTR<RouteSegment> frs = std::make_shared<RouteSegment>(road, segmentPoint);
			float distStartObstacles = segment->distanceFromStart + 
						calculateTimeWithObstacles(ctx, road, segmentDist, obstaclesTime);
			frs->parentRoute = segment;
			frs->parentSegmentEnd = segmentPoint;
			frs->reverseWaySearch = reverseWaySearch? 1 : -1;
			frs->distanceFromStart = opposite->distanceFromStart + distStartObstacles;
			frs->distanceToEnd = 0;
			frs->opposite = opposite;
			graphSegments.push(frs);
			if(TRACE_ROUTING){
				printRoad("  >> Final segment : ", frs);
			}
			return true;			
		}
	}
	return false;
}

void processRouteSegment(RoutingContext* ctx, bool reverseWaySearch, SEGMENTS_QUEUE& graphSegments,
		VISITED_MAP& visitedSegments, SHARED_PTR<RouteSegment>& segment,
		VISITED_MAP& oppositeSegments, bool doNotAddIntersections) {		
	SHARED_PTR<RouteDataObject> road = segment->road;
	bool initDirectionAllowed = checkIfInitialMovementAllowedOnSegment(ctx, 
		reverseWaySearch, visitedSegments, segment, road);	
	bool directionAllowed = initDirectionAllowed;
	if(!directionAllowed) {
		if(TRACE_ROUTING) {
			OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug,"  >> Already visited");
		}
		return;
	}
	float obstaclesTime = 0;
	float segmentDist = 0;
	int segmentPoint = segment->getSegmentStart();
	bool dir = segment->isPositive();
	SHARED_PTR<RouteSegment> prev = segment;
	while (directionAllowed) {
		// mark previous interval as visited and move to next intersection
		int prevInd = segmentPoint;
		if(dir) {
			segmentPoint ++;
		} else {
			segmentPoint --;
		}
		if (segmentPoint < 0 || segmentPoint >= road->getPointsLength()) {
			directionAllowed = false;
			continue;
		}
		visitedSegments[calculateRoutePointId(segment->getRoad(), segment->isPositive() ? segmentPoint - 1 : segmentPoint, 
					segment->isPositive())]= prev.get() != NULL ? prev : segment;
		int x = road->pointsX[segmentPoint];
		int y = road->pointsY[segmentPoint];
		int prevx = road->pointsX[prevInd];
		int prevy = road->pointsY[prevInd];
		if(x == prevx && y == prevy) {
			continue;
		}
			
		// 2. calculate point and try to load neighbor ways if they are not loaded
		segmentDist  += squareRootDist31(x, y,  prevx, prevy);
			
		// 2.1 calculate possible obstacle plus time
		double obstacle = ctx->config->router->defineRoutingObstacle(road, segmentPoint);
		if (obstacle < 0) {
			directionAllowed = false;
			continue;
		}
		double heightObstacle = ctx->config->router->defineHeightObstacle(road, !reverseWaySearch ? prevInd : segmentPoint, 
 					!reverseWaySearch ? segmentPoint : prevInd);
		if (heightObstacle < 0) {
			directionAllowed = false;
			continue;
		}
		bool alreadyVisited = checkIfOppositieSegmentWasVisited(ctx, reverseWaySearch, graphSegments, segment, oppositeSegments,
				segmentPoint,  segmentDist, obstaclesTime);
		obstaclesTime += obstacle;
		obstaclesTime += heightObstacle;
		if (alreadyVisited) {
			directionAllowed = false;
			continue;
		}
		// correct way of handling precalculatedRouteDirection 
		if(ctx->precalcRoute != nullptr) {
//				long nt = System.nanoTime();
//				float devDistance = ctx.precalculatedRouteDirection.getDeviationDistance(x, y);
//				// 1. linear method
//				// segmentDist = segmentDist * (1 + ctx.precalculatedRouteDirection.getDeviationDistance(x, y) / ctx.config->DEVIATION_RADIUS);
//				// 2. exponential method
//				segmentDist = segmentDist * (float) Math.pow(1.5, devDistance / 500);
//				ctx.timeNanoToCalcDeviation += (System.nanoTime() - nt);
		}
		// could be expensive calculation
		// 3. get intersected ways
		SHARED_PTR<RouteSegment> roadNext = ctx->loadRouteSegment(x, y); // ctx.config->memoryLimitation - ctx.memoryOverhead
		float distStartObstacles = segment->distanceFromStart + calculateTimeWithObstacles(ctx, road, segmentDist , obstaclesTime);
		if(ctx->precalcRoute != nullptr && ctx->precalcRoute->followNext) {
			//distStartObstacles = 0;
			distStartObstacles = ctx->precalcRoute->getDeviationDistance(x, y) / ctx->precalcRoute->maxSpeed;
		}
		// We don't check if there are outgoing connections
		bool processFurther = true;
		prev = processIntersections(ctx, graphSegments, visitedSegments, distStartObstacles,
					segment, segmentPoint, roadNext, reverseWaySearch, doNotAddIntersections, &processFurther);
		if (!processFurther) {
			directionAllowed = false;
			continue;
		}
	}
	//if(initDirectionAllowed && ctx.visitor != null){
	//	ctx.visitor.visitSegment(segment, segmentEnd, true);
	//}

}

void processRestriction(RoutingContext* ctx, SHARED_PTR<RouteSegment>& inputNext, bool reverseWay, int64_t viaId,
			SHARED_PTR<RouteDataObject>& road) {
	bool via = viaId != 0;
	SHARED_PTR<RouteSegment> next = inputNext;
	bool exclusiveRestriction = false;
	
	while (next.get() != NULL) {
		int type = -1;
		if (!reverseWay) {
			for (uint i = 0; i < road->restrictions.size(); i++) {
				if (road->restrictions[i].to == next->road->id) {
					if(!via || road->restrictions[i].via == viaId) {
						type = road->restrictions[i].type;
						break;
					}
				}
			}
		} else {
			for (uint i = 0; i < next->road->restrictions.size(); i++) {
				int rt = next->road->restrictions[i].type;
				int64_t restrictedTo = next->road->restrictions[i].to;
				if (restrictedTo == road->id) {			
					if(!via || next->road->restrictions[i].via == viaId) {		
						type = rt;
						break;
					}
				}

				// Check if there is restriction only to the other than current road
				if (rt == RESTRICTION_ONLY_RIGHT_TURN || rt == RESTRICTION_ONLY_LEFT_TURN
				|| rt == RESTRICTION_ONLY_STRAIGHT_ON) {
					// check if that restriction applies to considered junk
					SHARED_PTR<RouteSegment> foundNext = inputNext;
					while (foundNext.get() != NULL) {
						if (foundNext->road->id == restrictedTo) {
							break;
						}
						foundNext = foundNext->next;
					}
					if (foundNext.get() != NULL) {
						type = REVERSE_WAY_RESTRICTION_ONLY; // special constant
					}
				}
			}
		}
		if (type == REVERSE_WAY_RESTRICTION_ONLY) {
			// next = next.next; continue;
		} else if (type == -1 && exclusiveRestriction) {
			// next = next.next; continue;
		} else if (type == RESTRICTION_NO_LEFT_TURN || type == RESTRICTION_NO_RIGHT_TURN
		|| type == RESTRICTION_NO_STRAIGHT_ON || type == RESTRICTION_NO_U_TURN) {
			// next = next.next; continue;
			if(via) {
				vector<SHARED_PTR<RouteSegment> >::iterator it;
				for(it = ctx->segmentsToVisitPrescripted.begin(); it != ctx->segmentsToVisitPrescripted.end();
					it++) {
					if((*it)->road->id == next->road->id) {
						ctx->segmentsToVisitPrescripted.erase(it);
						break;
					}
					
				}
				
			}
		} else if (type == -1) {
			// case no restriction
			ctx->segmentsToVisitNotForbidden.push_back(next);
		} else {
			if (!via) {
				// case exclusive restriction (only_right, only_straight, ...)
				// 1. in case we are going backward we should not consider only_restriction
				// as exclusive because we have many "in" roads and one "out"
				// 2. in case we are going forward we have one "in" and many "out"
				if (!reverseWay) {
					exclusiveRestriction = true;
					ctx->segmentsToVisitNotForbidden.clear();
					ctx->segmentsToVisitPrescripted.push_back(next);
				} else {
					ctx->segmentsToVisitNotForbidden.push_back(next);
				}
			}
		}
		next = next->next;
	}
	if(!via) {
		ctx->segmentsToVisitPrescripted.insert(ctx->segmentsToVisitPrescripted.end(), ctx->segmentsToVisitNotForbidden.begin(), ctx->segmentsToVisitNotForbidden.end());
	}
}

bool proccessRestrictions(RoutingContext* ctx, SHARED_PTR<RouteSegment>& segment, SHARED_PTR<RouteSegment>& inputNext, bool reverseWay) {
	
	if(!ctx->config->router->restrictionsAware()) {
		return false;
	}
	SHARED_PTR<RouteDataObject> road = segment->getRoad();
	SHARED_PTR<RouteSegment> parent = getParentDiffId(segment);
		
	if (!reverseWay && road->restrictions.size() == 0 && 
			(parent.get() == NULL || parent->road->restrictions.size() == 0)) {
		return false;
	}
	ctx->segmentsToVisitPrescripted.clear();
	ctx->segmentsToVisitNotForbidden.clear();
	processRestriction(ctx, inputNext, reverseWay, 0, road);
	if(parent.get() != NULL) {
		processRestriction(ctx, inputNext, reverseWay, segment->road->id, parent->road);
	}
	return true;
}


SHARED_PTR<RouteSegment> processIntersections(RoutingContext* ctx, SEGMENTS_QUEUE& graphSegments, VISITED_MAP& visitedSegments,
		double distFromStart, SHARED_PTR<RouteSegment>& segment,int segmentPoint, SHARED_PTR<RouteSegment>& inputNext,
		bool reverseWaySearch, bool doNotAddIntersections, bool* processFurther) {
	bool thereAreRestrictions ;
	SHARED_PTR<RouteSegment> itself;
	vector<SHARED_PTR<RouteSegment> >::iterator nextIterator;
	if(inputNext.get() != NULL && inputNext->getRoad()->getId() == segment->getRoad()->getId() && 
		inputNext->next.get() == NULL) {
		thereAreRestrictions = false;
	} else {
		thereAreRestrictions = proccessRestrictions(ctx, segment, inputNext, reverseWaySearch);
		if (thereAreRestrictions) {
			nextIterator = ctx->segmentsToVisitPrescripted.begin();
			if(TRACE_ROUTING) {
		 		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "  >> There are restrictions");
		 	}
		}
	}

	int targetEndX = reverseWaySearch ? ctx->startX : ctx->targetX;
	int targetEndY = reverseWaySearch ? ctx->startY : ctx->targetY;
	float distanceToEnd = h(ctx, segment->road->pointsX[segmentPoint],
					segment->road->pointsY[segmentPoint], targetEndX, targetEndY);
	// Calculate possible ways to put into priority queue
	SHARED_PTR<RouteSegment> next = inputNext;
	bool hasNext = !thereAreRestrictions? next.get() != NULL : nextIterator != ctx->segmentsToVisitPrescripted.end();
	while (hasNext && !ctx->isInterrupted()) {
		if (thereAreRestrictions) {
			next = *nextIterator;
		}		
		if (next->getSegmentStart() == segmentPoint && next->road->getId() == segment->road->getId()) {
			// find segment itself  
			// (and process it as other with small exception that we don't add to graph segments and process immediately)
			itself = RouteSegment::initRouteSegment(next, segment->isPositive());
			if(itself.get() == NULL) {
				// do nothing
			} else if (itself->parentRoute.get() == NULL
				|| roadPriorityComparator(itself->distanceFromStart, itself->distanceToEnd, distFromStart,
								distanceToEnd, ctx->getHeuristicCoefficient()) > 0) {
				itself->distanceFromStart = distFromStart;
				itself->distanceToEnd = distanceToEnd;
				itself->parentRoute = segment;
				itself->parentSegmentEnd = segmentPoint;
			} else {
				// we already processed that segment earlier or it is in graph segments
				// and we had better results (so we shouldn't process)
				*processFurther = false;
			}
		} else if(!doNotAddIntersections) {
			SHARED_PTR<RouteSegment> nextPos = RouteSegment::initRouteSegment(next, true);
			SHARED_PTR<RouteSegment> nextNeg = RouteSegment::initRouteSegment(next, false);
			processOneRoadIntersection(ctx, graphSegments, visitedSegments, distFromStart, distanceToEnd, segment, segmentPoint,
					nextPos);
			processOneRoadIntersection(ctx, graphSegments, visitedSegments, distFromStart, distanceToEnd, segment, segmentPoint,
					nextNeg);

		}
		
		// iterate to next road
		if (thereAreRestrictions) {
			nextIterator++;
			hasNext = nextIterator != ctx->segmentsToVisitPrescripted.end();
		} else {
			next = next->next;
			hasNext = next.get() != NULL;
		}
	}
    if (ctx->isInterrupted())
        *processFurther = false;
    
	return itself;
}

bool sortRoutePoints (const SHARED_PTR<RouteSegmentPoint>& i,const SHARED_PTR<RouteSegmentPoint>& j) { return (i->dist<j->dist); }

SHARED_PTR<RouteSegmentPoint> findRouteSegment(int px, int py, RoutingContext* ctx) {
	return findRouteSegment(px, py, ctx, false);
}

SHARED_PTR<RouteSegmentPoint> findRouteSegment(int px, int py, RoutingContext* ctx, bool transportStop) {
	vector<SHARED_PTR<RouteDataObject> > dataObjects;
	ctx->loadTileData(px, py, 17, dataObjects);
	if (dataObjects.size() == 0) {
		ctx->loadTileData(px, py, 15, dataObjects);
	}
	if (dataObjects.size() == 0) {
		ctx->loadTileData(px, py, 14, dataObjects);
	}	
	
	vector<SHARED_PTR<RouteSegmentPoint> > list ;
	vector<SHARED_PTR<RouteDataObject> >::iterator it = dataObjects.begin();
	for (; it!= dataObjects.end(); it++) {
		SHARED_PTR<RouteDataObject> r = *it;
		if (r->pointsX.size() > 1) {
			SHARED_PTR<RouteSegmentPoint> road;
			for (uint j = 1; j < r->pointsX.size(); j++) {
				std::pair<int, int> p = getProjectionPoint(px, py, 
						r->pointsX[j -1 ], r->pointsY[j-1], r->pointsX[j], r->pointsY[j]);
				int prx = p.first;
				int pry = p.second;
				double currentsDist = squareDist31TileMetric(prx, pry, px, py);
				if (road.get() == NULL || currentsDist < road->dist) {
					road = std::make_shared<RouteSegmentPoint>(r, j);
					road->preciseX = prx;
					road->preciseY = pry;
					road->dist = currentsDist;
				}
			}
			if (road.get() != NULL) {
				float prio = ctx->config->router->defineSpeedPriority(road->road);
				if (prio > 0) {
					road->dist = (road->dist + GPS_POSSIBLE_ERROR * GPS_POSSIBLE_ERROR) / (prio * prio);
					list.push_back(road);
				}
				
			}
		}		
	}	
	sort(list.begin(), list.end(), sortRoutePoints);
	if(list.size() > 0) {
		SHARED_PTR<RouteSegmentPoint> ps = nullptr;
		int i = 0;
		if (ctx->publicTransport) {
			for (auto p : list) {
				if (transportStop && p->dist > 100) {
					break;
				}
				bool platform = p->road->platform();
				if (transportStop && platform) {
					ps = p;
					break;
				}
				if (!transportStop && !platform) {
					ps = p;
					break;
				}
			}
			i++;
		}
		if (ps == nullptr) {
			ps = list[0];
			list.erase(list.begin());
		} else {
			list.erase(list.begin(), list.begin() + i + 1);
		}
		ps->others = list;
		return ps;
	}
	return NULL;
}

bool combineTwoSegmentResultPlanner(SHARED_PTR<RouteSegmentResult>& toAdd, SHARED_PTR<RouteSegmentResult>& previous, bool reverse) {
	bool ld = previous->getEndPointIndex() > previous->getStartPointIndex();
	bool rd = toAdd->getEndPointIndex() > toAdd->getStartPointIndex();
	if (rd == ld) {
		if (toAdd->getStartPointIndex() == previous->getEndPointIndex() && !reverse) {
			previous->setEndPointIndex(toAdd->getEndPointIndex());
			previous->routingTime = previous->routingTime + toAdd->routingTime;
			return true;
		} else if (toAdd->getEndPointIndex() == previous->getStartPointIndex() && reverse) {
			previous->setStartPointIndex(toAdd->getStartPointIndex());
			previous->routingTime = previous->routingTime + toAdd->routingTime;
			return true;
		}
	}
	return false;
}

void addRouteSegmentToResult(vector<SHARED_PTR<RouteSegmentResult> >& result, SHARED_PTR<RouteSegmentResult>& res, bool reverse) {
	if (res->getEndPointIndex() != res->getStartPointIndex()) {
		if (result.size() > 0) {
			auto last = result.back();
			if (last->object->id == res->object->id) {
				if (combineTwoSegmentResultPlanner(res, last, reverse)) {
					return;
				}
			}
		}
		result.push_back(res);
	}
}

void attachConnectedRoads(RoutingContext* ctx, vector<SHARED_PTR<RouteSegmentResult> >& res) {
    for (auto it : res) {
		bool plus = it->getStartPointIndex() < it->getEndPointIndex();
		int j = it->getStartPointIndex();
		do {
			SHARED_PTR<RouteSegment> s = ctx->loadRouteSegment(it->object->pointsX[j], it->object->pointsY[j]);
			vector<SHARED_PTR<RouteSegmentResult> > r;
			RouteSegment* rs = s.get();
			while(rs != NULL) {
                auto res = std::make_shared<RouteSegmentResult>(rs->road, rs->getSegmentStart(), rs->getSegmentStart());
				r.push_back(res);
				rs = rs->next.get();
			}
			it->attachedRoutes.push_back(r);
			j = plus ? j + 1 : j - 1;
            
		} while (j != it->getEndPointIndex());
	}
}

void processOneRoadIntersection(RoutingContext* ctx, SEGMENTS_QUEUE& graphSegments,
			VISITED_MAP& visitedSegments, double distFromStart, double distanceToEnd,  
			SHARED_PTR<RouteSegment>& segment, int segmentPoint, SHARED_PTR<RouteSegment>& next) {
	if (next.get() != NULL) {
		double obstaclesTime = ctx->config->router->calculateTurnTime(next, next->isPositive()? 
				next->road->getPointsLength() - 1 : 0,  
				segment, segmentPoint);
		distFromStart += obstaclesTime;
		VISITED_MAP::iterator visIt = visitedSegments.find(calculateRoutePointId(next, next->isPositive()));
		if (visIt == visitedSegments.end() || visIt->second.get() == NULL) {
			if (next->parentRoute.get() == NULL
				|| roadPriorityComparator(next->distanceFromStart, next->distanceToEnd,
								distFromStart, distanceToEnd, ctx->getHeuristicCoefficient()) > 0) {
				next->distanceFromStart = distFromStart;
				next->distanceToEnd = distanceToEnd;
				if (TRACE_ROUTING) {
					printRoad("  >>", next);
				}
					// put additional information to recover whole route after
				next->parentRoute = segment;
				next->parentSegmentEnd = segmentPoint;
				graphSegments.push(next);
			}
		} else {
			// the segment was already visited! We need to follow better route if it exists
			// that is very exceptional situation and almost exception, it can happen 
			// 1. when we underestimate distnceToEnd - wrong h()
			// 2. because we process not small segments but the whole road, it could be that 
			// deviation from the road is faster than following the whole road itself!
			if (distFromStart < next->distanceFromStart) {
				if (ctx->getHeuristicCoefficient() <= 1) {
					OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, 
						"! Alert distance from start %f < %f id=%lld", 
						 distFromStart, next->distanceFromStart, next->getRoad()->getId());
				}
				// A: we can't change parent route just here, because we need to update visitedSegments
				// presumably we can do visitedSegments.put(calculateRoutePointId(next), next);
//				next.distanceFromStart = distFromStart;
//				next.setParentRoute(segment);
//				next.setParentSegmentEnd(segmentPoint);
				//if (ctx.visitor != null) {
					// ctx.visitor.visitSegment(next, false);
				//}
			}
		}
	}
}

float calcRoutingTime(float parentRoutingTime, SHARED_PTR<RouteSegment>& finalSegment,
	SHARED_PTR<RouteSegment>& segment, SHARED_PTR<RouteSegmentResult>& res) {
	if (segment.get() != finalSegment.get()) {
		if (parentRoutingTime != -1) {
			res->routingTime = parentRoutingTime - segment->distanceFromStart;
		}
		parentRoutingTime = segment->distanceFromStart;
	}
	return parentRoutingTime;
}

vector<SHARED_PTR<RouteSegmentResult> > convertFinalSegmentToResults(RoutingContext* ctx, SHARED_PTR<RouteSegment>& finalSegment) {
	vector<SHARED_PTR<RouteSegmentResult> > result;
	if (finalSegment.get() != NULL) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Routing calculated time distance %f", finalSegment->distanceFromStart);
		// Get results from opposite direction roads
		SHARED_PTR<RouteSegment> segment = finalSegment->isReverseWaySearch() ? finalSegment : 
					finalSegment->opposite->parentRoute;
		int parentSegmentStart = finalSegment->isReverseWaySearch() ? finalSegment->opposite->getSegmentStart() : 
					finalSegment->opposite->parentSegmentEnd;
		float parentRoutingTime = -1;
		while (segment.get() != NULL) {
            auto res = std::make_shared<RouteSegmentResult>(segment->road, parentSegmentStart, segment->getSegmentStart());
			parentRoutingTime = calcRoutingTime(parentRoutingTime, finalSegment, segment, res);
			parentSegmentStart = segment->parentSegmentEnd;
			segment = segment->parentRoute;
			addRouteSegmentToResult(result, res, false);
		}
		// reverse it just to attach good direction roads
		std::reverse(result.begin(), result.end());

		segment = finalSegment->isReverseWaySearch() ? finalSegment->opposite->parentRoute : finalSegment;
		int parentSegmentEnd =
				finalSegment->isReverseWaySearch() ?
						finalSegment->opposite->parentSegmentEnd : finalSegment->opposite->getSegmentStart();
		parentRoutingTime = -1;
		while (segment.get() != NULL) {
			auto res = std::make_shared<RouteSegmentResult>(segment->road, segment->getSegmentStart(), parentSegmentEnd);
			parentRoutingTime = calcRoutingTime(parentRoutingTime, finalSegment, segment, res);
			parentSegmentEnd = segment->parentSegmentEnd;
			segment = segment->parentRoute;
			// happens in smart recalculation
			addRouteSegmentToResult(result, res, true);
		}
		std::reverse(result.begin(), result.end());

	}
	return result;
}

vector<SHARED_PTR<RouteSegmentResult> > searchRouteInternal(RoutingContext* ctx, bool leftSideNavigation) {
	SHARED_PTR<RouteSegmentPoint> start = findRouteSegment(ctx->startX, ctx->startY, ctx, ctx->publicTransport && ctx->startTransportStop);
	if(start.get() == NULL) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Start point was not found [Native]");
		if(ctx->progress.get()) {
			ctx->progress->setSegmentNotFound(0);
		}
		return vector<SHARED_PTR<RouteSegmentResult> >();
	} else {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Start point was found %lld [Native]", start->road->id / 64);
	}
	SHARED_PTR<RouteSegmentPoint> end = findRouteSegment(ctx->targetX, ctx->targetY, ctx, ctx->publicTransport && ctx->targetTransportStop);
	if(end.get() == NULL) {
		if(ctx->progress.get()) {
			ctx->progress->setSegmentNotFound(1);
		}
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "End point was not found [Native]");
		return vector<SHARED_PTR<RouteSegmentResult> >();
	} else {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "End point was found %lld [Native]", end->road->id / 64);
	}
	SHARED_PTR<RouteSegment> finalSegment = searchRouteInternal(ctx, start, end, leftSideNavigation);
	vector<SHARED_PTR<RouteSegmentResult> > res = convertFinalSegmentToResults(ctx, finalSegment);
	attachConnectedRoads(ctx, res);
	return res;
}


