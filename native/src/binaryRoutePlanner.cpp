#include "Common.h"
#include "common2.h"
#include <queue>
#include "binaryRead.h"
#include "binaryRoutePlanner.h"
#include <functional>

#include "Logging.h"

static bool PRINT_TO_CONSOLE_ROUTE_INFORMATION_TO_TEST = true;
static const int REVERSE_WAY_RESTRICTION_ONLY = 1024;

static const int ROUTE_POINTS = 11;
static const float TURN_DEGREE_MIN = 45;
static const short RESTRICTION_NO_RIGHT_TURN = 1;
static const short RESTRICTION_NO_LEFT_TURN = 2;
static const short RESTRICTION_NO_U_TURN = 3;
static const short RESTRICTION_NO_STRAIGHT_ON = 4;
static const short RESTRICTION_ONLY_RIGHT_TURN = 5;
static const short RESTRICTION_ONLY_LEFT_TURN = 6;
static const short RESTRICTION_ONLY_STRAIGHT_ON = 7;
static const bool TRACE_ROUTING = true;

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

void printRoad(const char* prefix, SHARED_PTR<RouteSegment> segment) {
	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "%s Road id=%lld dir=%d ind=%d ds=%f es=%f pend=%d parent=%lld ",
		prefix, segment->road->id, 
		segment->directionAssgn, segment->getSegmentStart(),
		segment->distanceFromStart, segment->distanceToEnd, 
		segment->parentRoute.get() != NULL? segment->parentSegmentEnd : 0,
		segment->parentRoute.get() != NULL? segment->parentRoute->road->id : 0);	
}

// translate into meters
static double squareRootDist(int x1, int y1, int x2, int y2) {
	double dy = convert31YToMeters(y1, y2);
	double dx = convert31XToMeters(x1, x2);
	return sqrt(dx * dx + dy * dy);
//		return measuredDist(x1, y1, x2, y2);
}

static double squareDist(int x1, int y1, int x2, int y2) {
	// translate into meters
	double dy = convert31YToMeters(y1, y2);
	double dx = convert31XToMeters(x1, x2);
	return dx * dx + dy * dy;
}

int64_t calculateRoutePointId(SHARED_PTR<RouteDataObject> road, int intervalId, bool positive) {
	return (road->id << ROUTE_POINTS) + (intervalId << 1) + (positive ? 1 : 0);
}

int64_t calculateRoutePointId(SHARED_PTR<RouteSegment> segm, bool direction) {
	if(segm->getSegmentStart() == 0 && !direction) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Assert failed route point id  0");
	}
	if(segm->getSegmentStart() == segm->getRoad()->getPointsLength() - 1 && direction) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Assert failed route point length");
	}
	return calculateRoutePointId(segm->getRoad(),
				direction ? segm->getSegmentStart() : segm->getSegmentStart() - 1, direction);
}

static double h(RoutingContext* ctx, int begX, int begY, int endX, int endY) {
	double distToFinalPoint = squareRootDist(begX, begY,  endX, endY);
	double result = distToFinalPoint /  ctx->config.getMaxDefaultSpeed();
	if(!ctx.precalculatedRouteDirection.empty){
		// TODO
		// float te = ctx.precalculatedRouteDirection.timeEstimate(begX, begY,  endX, endY);
		// if(te > 0) return te;
	}
	return result;
}

struct SegmentsComparator: public std::binary_function<SHARED_PTR<RouteSegment>, SHARED_PTR<RouteSegment>, bool>
{
	RoutingContext* ctx;
	SegmentsComparator(RoutingContext* c) : ctx(c) {

	}
	bool operator()(const SHARED_PTR<RouteSegment> lhs, const SHARED_PTR<RouteSegment> rhs) const
	{
		int cmp = roadPriorityComparator(lhs.get()->distanceFromStart, lhs.get()->distanceToEnd, rhs.get()->distanceFromStart,
		    			rhs.get()->distanceToEnd, ctx->getHeuristicCoefficient());
    	return cmp > 0;
    }
};
struct NonHeuristicSegmentsComparator: public std::binary_function<SHARED_PTR<RouteSegment>, SHARED_PTR<RouteSegment>, bool>
{
	bool operator()(const SHARED_PTR<RouteSegment> lhs, const SHARED_PTR<RouteSegment> rhs) const
	{
		return roadPriorityComparator(lhs.get()->distanceFromStart, lhs.get()->distanceToEnd, rhs.get()->distanceFromStart, rhs.get()->distanceToEnd, 0.5) > 0;
	}
};

typedef UNORDERED(map)<int64_t, SHARED_PTR<RouteSegment> > VISITED_MAP;
typedef priority_queue<SHARED_PTR<RouteSegment>, vector<SHARED_PTR<RouteSegment> >, SegmentsComparator > SEGMENTS_QUEUE;
void processRouteSegment(RoutingContext* ctx, bool reverseWaySearch, SEGMENTS_QUEUE& graphSegments,
		VISITED_MAP& visitedSegments, SHARED_PTR<RouteSegment> segment, 
		VISITED_MAP& oppositeSegments, bool direction);

void processIntersections(RoutingContext* ctx, SEGMENTS_QUEUE& graphSegments, VISITED_MAP& visitedSegments,
		double distFromStart, SHARED_PTR<RouteSegment> segment,int segmentPoint, SHARED_PTR<RouteSegment> inputNext,
		bool reverseWaySearch, bool doNotAddIntersections);

void processOneRoadIntersection(RoutingContext* ctx, SEGMENTS_QUEUE& graphSegments,
			VISITED_MAP& visitedSegments, double distFromStart, double distanceToEnd,  
			SHARED_PTR<RouteSegment> segment, int segmentPoint, SHARED_PTR<RouteSegment> next);


int calculateSizeOfSearchMaps(SEGMENTS_QUEUE graphDirectSegments, SEGMENTS_QUEUE graphReverseSegments,
		VISITED_MAP visitedDirectSegments, VISITED_MAP visitedOppositeSegments) {
	int sz = visitedDirectSegments.size() * sizeof(pair<int64_t, SHARED_PTR<RouteSegment> > );
	sz += visitedOppositeSegments.size()*sizeof(pair<int64_t, SHARED_PTR<RouteSegment> >);
	sz += graphDirectSegments.size()*sizeof(SHARED_PTR<RouteSegment>);
	sz += graphReverseSegments.size()*sizeof(SHARED_PTR<RouteSegment>);
	return sz;
}

void initQueuesWithStartEnd(RoutingContext* ctx,  SHARED_PTR<RouteSegment> start, SHARED_PTR<RouteSegment> end, 
			PriorityQueue<RouteSegment> graphDirectSegments, PriorityQueue<RouteSegment> graphReverseSegments) {
		SHARED_PTR<RouteSegment> startPos = start->initRouteSegment(true);
		SHARED_PTR<RouteSegment> startNeg = start->initRouteSegment(false);
		SHARED_PTR<RouteSegment> endPos = end->initRouteSegment(true);
		SHARED_PTR<RouteSegment> endNeg = end->initRouteSegment(false);


		// for start : f(start) = g(start) + h(start) = 0 + h(start) = h(start)
		if(ctx->config.initialDirection > -180 && ctx->config.initialDirection < 180) {
			ctx->firstRoadId = (start->road->id << ROUTE_POINTS) + start->getSegmentStart();
			double plusDir = start->road->directionRoute(start->getSegmentStart(), true);
			double diff = plusDir - ctx->config.initialDirection;
			if(abs(alignAngleDifference(diff)) <= M_PI / 3) {
				if(startNeg.get() != NULL) {
					startNeg->distanceFromStart += 500;
				}
			} else if(abs(alignAngleDifference(diff - M_PI )) <= M_PI / 3) {
				if(startPos.get() != NULL) {
					startPos->distanceFromStart += 500;
				}
			}
		}
		int targetEndX = end->road->pointsX[end->segmentStart];
		int targetEndY = end->road->pointsY[end->segmentStart];
		int startX = start->road->pointsX[start->segmentStart];
		int startY = start->road->pointsY[start->segmentStart];
	
		float estimatedDistance = (float) h(ctx, targetEndX, targetEndY, startX, startY);
		if(startPos.get() != NULL) {
			startPos->distanceToEnd = estimatedDistance;
			graphDirectSegments.add(startPos);
		}
		if(startNeg.get() != NULL) {
			startNeg->distanceToEnd = estimatedDistance;
			graphDirectSegments.add(startNeg);
		}
		if(endPos.get() != NULL) {
			endPos->distanceToEnd = estimatedDistance;
			graphReverseSegments.add(endPos);
		}
		if(endNeg.get() != NULL) {
			endNeg->distanceToEnd = estimatedDistance;
			graphReverseSegments.add(endNeg);
		}
}

/**
 * Calculate route between start.segmentEnd and end.segmentStart (using A* algorithm)
 * return list of segments
 */
SHARED_PTR<RouteSegment> searchRouteInternal(RoutingContext* ctx, SHARED_PTR<RouteSegment> start, SHARED_PTR<RouteSegment> end, bool leftSideNavigation) {
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
			printRoad(">", segment);
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
		if(ctx->getPlanRoadDirection() <= 0 && graphReverseSegments.size() == 0){
			OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Route is not found to selected target point.");
			return finalSegment;
		}
		if(ctx->getPlanRoadDirection() >= 0 && graphDirectSegments.size() == 0){
			OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Route is not found from selected start point.");
			return finalSegment;
		}
		if (ctx->planRouteIn2Directions()) {
			forwardSearch = !nonHeuristicSegmentsComparator(graphDirectSegments.top(), graphReverseSegments.top());
			if (graphDirectSegments.size() * 1.3 > graphReverseSegments.size()) {
				forwardSearch = false;
			} else if (graphDirectSegments.size() < 1.3 * graphReverseSegments.size()) {
				forwardSearch = true;
			}
		} else {
			// different strategy : use onedirectional graph
			inverse = ctx->getPlanRoadDirection() < 0;
		}
		if (inverse) {
			graphSegments = &graphReverseSegments;
		} else {
			graphSegments = &graphDirectSegments;
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
			VISITED_MAP& visitedSegments, SHARED_PTR<RouteSegment> segment, SHARED_PTR<RouteDataObject> road) {
	bool directionAllowed;
	int middle = segment->segmentStart;
	int oneway = ctx->config.isOneWay(road);
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
	}
	if(directionAllowed && visitedSegments.find(calculateRoutePointId(segment, segment->isPositive()))
				 != visitedSegments.end() ) {
		directionAllowed = false;
	}

	return directionAllowed;
}


float calculateTimeWithObstacles(RoutingContext* ctx, SHARED_PTR<RouteDataObject> road, float distOnRoadToPass, float obstaclesTime) {
	float priority = ctx->config.defineSpeedPriority(road);
	float speed = ctx->config.defineSpeed(road) * priority;
	if (speed == 0) {
		speed = ctx->config.getMinDefaultSpeed() * priority;
	}
	// speed can not exceed max default speed according to A*
	if(speed > ctx->config.getMaxDefaultSpeed()) {
		speed = ctx->config.getMaxDefaultSpeed();
	}
	return obstaclesTime + distOnRoadToPass / speed;
}

bool checkIfOppositieSegmentWasVisited(RoutingContext* ctx, bool reverseWaySearch, SEGMENTS_QUEUE& graphSegments,
		SHARED_PTR<RouteSegment> segment, VISITED_MAP& oppositeSegments, 
		 int segmentPoint, float segmentDist, float obstaclesTime) {
	SHARED_PTR<RouteDataObject> road = segment -> getRoad();
	int64_t opp = calculateRoutePointId(road, segment->isPositive() ? segmentPoint - 1 : segmentPoint, !segment->isPositive());
	SHARED_PTR<RouteSegment> opposite = oppositeSegments[opp];
	if (opposite.get() != NULL) {
		SHARED_PTR<RouteSegment> frs = SHARED_PTR<RouteSegment>(new RouteSegment(road, segmentPoint));
		float distStartObstacles = segment->distanceFromStart + calculateTimeWithObstacles(ctx, road, segmentDist , obstaclesTime);
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
	return false;
}

void processRouteSegment(RoutingContext* ctx, bool reverseWaySearch, SEGMENTS_QUEUE& graphSegments,
		VISITED_MAP& visitedSegments, SHARED_PTR<RouteSegment> segment, 
		VISITED_MAP& oppositeSegments, bool doNotAddIntersections) {		
	SHARED_PTR<RouteDataObject> road = segment->road;
	bool initDirectionAllowed = checkIfInitialMovementAllowedOnSegment(ctx, reverseWaySearch, visitedSegments, segment, direction, road);	
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
	boolean dir = segment.isPositive();
	while (directionAllowed) {
		// mark previous interval as visited and move to next intersection
		int prevInd = segmentPoint;
		if(dir) {
			segmentEnd ++;
		} else {
			segmentEnd --;
		}
		if (segmentEnd < 0 || segmentEnd >= road->pointsX.size()) {
			directionAllowed = false;
			continue;
		}
		visitedSegments[calculateRoutePointId(segment->getRoad(), segment->isPositive() ? segmentPoint - 1 : segmentPoint, 
					segment->isPositive())]=segment;
		int x = road->pointsX[segmentPoint];
		int y = road->pointsY[segmentPoint];
		int prevx = road->pointsX[prevInd];
		int prevy = road->pointsY[prevInd];
		if(x == prevx && y == prevy) {
			continue;
		}
			
		// 2. calculate point and try to load neighbor ways if they are not loaded
		segmentDist  += squareRootDist(x, y,  prevx, prevy);
			
		// 2.1 calculate possible obstacle plus time
		double obstacle = ctx->config.defineRoutingObstacle(road, segmentEnd);
		if (obstacle < 0) {
			directionAllowed = false;
			continue;
		}
		obstaclesTime += obstacle;
		
		bool alreadyVisited = checkIfOppositieSegmentWasVisited(ctx, reverseWaySearch, graphSegments, segment, oppositeSegments,
				segmentPoint,  segmentDist, obstaclesTime);
		if (alreadyVisited) {
			directionAllowed = false;
			continue;
		}
		// correct way of handling precalculatedRouteDirection 
		if(!ctx->precalcRoute.empty) {
//				long nt = System.nanoTime();
//				float devDistance = ctx.precalculatedRouteDirection.getDeviationDistance(x, y);
//				// 1. linear method
//				// segmentDist = segmentDist * (1 + ctx.precalculatedRouteDirection.getDeviationDistance(x, y) / ctx.config.DEVIATION_RADIUS);
//				// 2. exponential method
//				segmentDist = segmentDist * (float) Math.pow(1.5, devDistance / 500);
//				ctx.timeNanoToCalcDeviation += (System.nanoTime() - nt);
		}
		// could be expensive calculation
		// 3. get intersected ways
		SHARED_PTR<RouteSegment> next = ctx->loadRouteSegment(x, y); // ctx.config.memoryLimitation - ctx.memoryOverhead
		float distStartObstacles = segment->distanceFromStart + calculateTimeWithObstacles(ctx, road, segmentDist , obstaclesTime);
		// We don't check if there are outgoing connections
		bool processFurther = processIntersections(ctx, graphSegments, visitedSegments, distStartObstacles,
					segment, segmentPoint, roadNext, reverseWaySearch, doNotAddIntersections);
		if (!processFurther) {
			directionAllowed = false;
			continue;
		}
	}
	//if(initDirectionAllowed && ctx.visitor != null){
	//	ctx.visitor.visitSegment(segment, segmentEnd, true);
	//}

}

bool proccessRestrictions(RoutingContext* ctx, SHARED_PTR<RouteDataObject> road, SHARED_PTR<RouteSegment> inputNext, bool reverseWay) {
	ctx->segmentsToVisitPrescripted.clear();
	ctx->segmentsToVisitNotForbidden.clear();
	bool exclusiveRestriction = false;
	SHARED_PTR<RouteSegment> next = inputNext;

	if (!reverseWay && road->restrictions.size() == 0) {
		return false;
	}
	if(!ctx->config.restrictionsAware()) {
		return false;
	}
	while (next.get() != NULL) {
		int type = -1;
		if (!reverseWay) {
			for (int i = 0; i < road->restrictions.size(); i++) {
				if ((road->restrictions[i] >> 3) == next->road->id) {
					type = road->restrictions[i] & 7;
					break;
				}
			}
		} else {
			for (int i = 0; i < next->road->restrictions.size(); i++) {
				int rt = next->road->restrictions[i] & 7;
				int64_t restrictedTo = next->road->restrictions[i] >> 3;
				if (restrictedTo == road->id) {
					type = rt;
					break;
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
		} else if (type == -1) {
			// case no restriction
			ctx->segmentsToVisitNotForbidden.push_back(next);
		} else {
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
		next = next->next;
	}
	ctx->segmentsToVisitPrescripted.insert(ctx->segmentsToVisitPrescripted.end(), ctx->segmentsToVisitNotForbidden.begin(), ctx->segmentsToVisitNotForbidden.end());
	return true;
}


void processIntersections(RoutingContext* ctx, SEGMENTS_QUEUE& graphSegments, VISITED_MAP& visitedSegments,
		double distFromStart, SHARED_PTR<RouteSegment> segment,int segmentPoint, SHARED_PTR<RouteSegment> inputNext,
		bool reverseWaySearch, bool doNotAddIntersections) {
	bool thereAreRestrictions ;
	bool processFurther = true;
	if(inputNext.get() != NULL && inputNext->getRoad()->getId() == segment->getRoad()->getId() && 
		inputNext->next.get() ==NULL) {
		thereAreRestrictions = false;
	} else {
		thereAreRestrictions = proccessRestrictions(ctx, segment->road, inputNext, reverseWay);
		vector<SHARED_PTR<RouteSegment> >::iterator nextIterator;
		if (thereAreRestrictions) {
			nextIterator = ctx->segmentsToVisitPrescripted.begin();
			if(TRACE_ROUTING) {
		 		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "  >> There are restrictions");
		 	}
		}
	}

	int targetEndX = reverseWaySearch ? ctx.startX : ctx.targetX;
	int targetEndY = reverseWaySearch ? ctx.startY : ctx.targetY;
	float distanceToEnd = h(ctx, segment->road->pointsX[segmentPoint],
					segment->road->pointsY[segmentPoint], targetEndX, targetEndY);
	// Calculate possible ways to put into priority queue
	SHARED_PTR<RouteSegment> next = inputNext;
	bool hasNext = !thereAreRestrictions || nextIterator != ctx->segmentsToVisitPrescripted.end();
	while (hasNext) {
		if (thereAreRestrictions) {
			next = *nextIterator;
		}
		if (next->getSegmentStart() == segmentPoint && next->road->getId() == segment->road->getId()) {
			// find segment itself  
			// (and process it as other with small exception that we don't add to graph segments and process immediately)
			SHARED_PTR<RouteSegment> itself = next->initRouteSegment(segment->isPositive());
			if(itself.get() == NULL) {
				// do nothing
			} else if (itself->getParentRoute().get() == NULL
				|| roadPriorityComparator(itself->distanceFromStart, itself->distanceToEnd, distFromStart,
								distanceToEnd) > 0) {
				itself->distanceFromStart = distFromStart;
				itself->distanceToEnd = distanceToEnd;
				itself->parentRoute = segment;
				itself->parentSegmentEnd = segmentPoint;
			} else {
				// we already processed that segment earlier or it is in graph segments
				// and we had better results (so we shouldn't process)
				processFurther = false;
			}
		} else if(!doNotAddIntersections) {
			SHARED_PTR<RouteSegment> nextPos = next->initRouteSegment(true);
			SHARED_PTR<RouteSegment> nextNeg = next->initRouteSegment(false);
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
	return processFurther;
}

SHARED_PTR<RouteSegment> findRouteSegment(int px, int py, RoutingContext* ctx) {
	vector<SHARED_PTR<RouteDataObject> > dataObjects;
	ctx->loadTileData(px, py, 17, dataObjects);
	if (dataObjects.size() == 0) {
		ctx->loadTileData(px, py, 15, dataObjects);
	}
	SHARED_PTR<RouteSegment> road;
	double sdist = 0;
	int foundx = 0;
	int foundy = 0;
	vector<SHARED_PTR<RouteDataObject> >::iterator it = dataObjects.begin();
	for (; it!= dataObjects.end(); it++) {
		SHARED_PTR<RouteDataObject> r = *it;
		if (r->pointsX.size() > 1) {
			for (int j = 1; j < r->pointsX.size(); j++) {
				double mDist = squareRootDist(r->pointsX[j -1 ], r->pointsY[j-1], r->pointsX[j], r->pointsY[j]);
				int prx = r->pointsX[j];
				int pry = r->pointsY[j];
				double projection = calculateProjection31TileMetric(r->pointsX[j -1 ], r->pointsY[j-1], r->pointsX[j], r->pointsY[j],
						px, py);
				if (projection < 0) {
					prx = r->pointsX[j - 1];
					pry = r->pointsY[j - 1];
				} else if (projection >= mDist * mDist) {
					prx = r->pointsX[j ];
					pry = r->pointsY[j ];
				} else {
					double c = projection / (mDist * mDist);
					prx = (int) ((double)r->pointsX[j - 1] + ((double)r->pointsX[j] - r->pointsX[j - 1]) * c);
					pry = (int) ((double)r->pointsY[j - 1] + ((double)r->pointsY[j] - r->pointsY[j - 1]) * c);
				}
				double currentsDist = squareDist31TileMetric(prx, pry, px, py);
				if (road.get() == NULL || currentsDist < sdist) {
					road = SHARED_PTR<RouteSegment>(new RouteSegment(r, j));
					foundx = prx;
					foundy = pry;
					sdist = currentsDist;
				}
			}
		}
	}
	if (road.get() != NULL) {
		// make copy before
		SHARED_PTR<RouteDataObject> r = road->road;
		int index = road->getSegmentStart();
		r->pointsX.insert(r->pointsX.begin() + index, foundx);
		r->pointsY.insert(r->pointsY.begin() + index, foundy);
		if(r->pointTypes.size() > index) {
			r->pointTypes.insert(r->pointTypes.begin() + index, std::vector<uint32_t>());
		}
	}
	return road;
}

bool combineTwoSegmentResult(RouteSegmentResult& toAdd, RouteSegmentResult& previous, bool reverse) {
	bool ld = previous.endPointIndex > previous.startPointIndex;
	bool rd = toAdd.endPointIndex > toAdd.startPointIndex;
	if (rd == ld) {
		if (toAdd.startPointIndex == previous.endPointIndex && !reverse) {
			previous.endPointIndex = toAdd.endPointIndex;
			return true;
		} else if (toAdd.endPointIndex == previous.startPointIndex && reverse) {
			previous.startPointIndex = toAdd.startPointIndex;
			return true;
		}
	}
	return false;
}

void addRouteSegmentToResult(vector<RouteSegmentResult>& result, RouteSegmentResult& res, bool reverse) {
	if (res.endPointIndex != res.startPointIndex) {
		if (result.size() > 0) {
			RouteSegmentResult last = result[result.size() - 1];
			if (last.object->id == res.object->id) {
				if (combineTwoSegmentResult(res, last, reverse)) {
					return;
				}
			}
		}
		result.push_back(res);
	}
}

void attachConnectedRoads(RoutingContext* ctx, vector<RouteSegmentResult>& res) {
	vector<RouteSegmentResult>::iterator it = res.begin();
	for (; it != res.end(); it++) {
		bool plus = it->startPointIndex < it->endPointIndex;
		int j = it->startPointIndex;
		do {
			SHARED_PTR<RouteSegment> s = ctx->loadRouteSegment(it->object->pointsX[j], it->object->pointsY[j]);
			vector<RouteSegmentResult> r;
			RouteSegment* rs = s.get();
			while(rs != NULL) {
				RouteSegmentResult res(rs->road, rs->getSegmentStart(), rs->getSegmentStart());
				r.push_back(res);
				rs = rs->next.get();
			}
			it->attachedRoutes.push_back(r);
			j = plus ? j + 1 : j - 1;
		}while(j != it->endPointIndex);
	}

}

void processOneRoadIntersection(RoutingContext* ctx, SEGMENTS_QUEUE& graphSegments,
			VISITED_MAP& visitedSegments, double distFromStart, double distanceToEnd,  
			SHARED_PTR<RouteSegment> segment, int segmentPoint, SHARED_PTR<RouteSegment> next) {
	if (next.get() != null) {
		double obstaclesTime = ctx->getRouter()->calculateTurnTime(next, next->isPositive()? 
				next->road->getPointsLength() - 1 : 0,  
				segment, segmentPoint);
		distFromStart += obstaclesTime;
		if (visitedSegments.find(calculateRoutePointId(next, next->isPositive())) == visitedSegments.end()) {
			if (next->parentRoute.get() == NULL
				|| roadPriorityComparator(next->distanceFromStart, next->distanceToEnd,
								distFromStart, distanceToEnd) > 0) {
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
			if (distFromStart < next.distanceFromStart) {
				if (ctx->config->heuristicCoefficient <= 1) {
					OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, 
						"! Alert distance from start %llf < %llf id=%lld", 
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

vector<RouteSegmentResult> convertFinalSegmentToResults(RoutingContext* ctx, SHARED_PTR<RouteSegment> finalSegment) {
	vector<RouteSegmentResult> result;
	if (finalSegment.get() != NULL) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Routing calculated time distance %f", finalSegment->distanceFromStart);
		// Get results from opposite direction roads
		SHARED_PTR<RouteSegment> segment = finalSegment->reverseWaySearch ? finalSegment : finalSegment->opposite->parentRoute;
		int parentSegmentStart =
				finalSegment->reverseWaySearch ?
						finalSegment->opposite->getSegmentStart() : finalSegment->opposite->parentSegmentEnd;
		while (segment.get() != NULL) {
			RouteSegmentResult res(segment->road, parentSegmentStart, segment->getSegmentStart());
			parentSegmentStart = segment->parentSegmentEnd;
			segment = segment->parentRoute;
			addRouteSegmentToResult(result, res, false);
		}
		// reverse it just to attach good direction roads
		std::reverse(result.begin(), result.end());

		segment = finalSegment->reverseWaySearch ? finalSegment->opposite->parentRoute : finalSegment;
		int parentSegmentEnd =
				finalSegment->reverseWaySearch ?
						finalSegment->opposite->parentSegmentEnd : finalSegment->opposite->getSegmentStart();

		while (segment.get() != NULL) {
			RouteSegmentResult res(segment->road, segment->getSegmentStart(), parentSegmentEnd);
			parentSegmentEnd = segment->parentSegmentEnd;
			segment = segment->parentRoute;
			// happens in smart recalculation
			addRouteSegmentToResult(result, res, true);
		}
		std::reverse(result.begin(), result.end());

	}
	return result;
}

vector<RouteSegmentResult> searchRouteInternal(RoutingContext* ctx, bool leftSideNavigation) {
	SHARED_PTR<RouteSegment> start = findRouteSegment(ctx->startX, ctx->startY, ctx);
	if(start.get() == NULL) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Start point was not found [Native]");
		if(ctx->progress.get()) {
			ctx->progress->setSegmentNotFound(0);
		}
		return vector<RouteSegmentResult>();
	} else {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Start point was found %lld [Native]", start->road->id);
	}
	SHARED_PTR<RouteSegment> end = findRouteSegment(ctx->endX, ctx->endY, ctx);
	if(end.get() == NULL) {
		if(ctx->progress.get()) {
			ctx->progress->setSegmentNotFound(1);
		}
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "End point was not found [Native]");
		return vector<RouteSegmentResult>();
	} else {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "End point was found %lld [Native]", end->road->id);
	}
	SHARED_PTR<RouteSegment> finalSegment = searchRouteInternal(ctx, start, end, leftSideNavigation);
	vector<RouteSegmentResult> res = convertFinalSegmentToResults(ctx, finalSegment);
	attachConnectedRoads(ctx, res);
	return res;
}

bool compareRoutingSubregionTile(SHARED_PTR<RoutingSubregionTile> o1, SHARED_PTR<RoutingSubregionTile> o2) {
	int v1 = (o1->access + 1) * pow((float)10, o1->getUnloadCount() -1);
	int v2 = (o2->access + 1) * pow((float)10, o2->getUnloadCount() -1);
	return v1 < v2;
}
