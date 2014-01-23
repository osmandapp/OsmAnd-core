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
static const bool TRACE_ROUTING = false;

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

static double h(RoutingContext* ctx, float distanceToFinalPoint, SHARED_PTR<RouteSegment> next) {
	return distanceToFinalPoint / ctx->config.getMaxDefaultSpeed();

}
static double h(RoutingContext* ctx, int targetEndX, int targetEndY, int startX, int startY) {
	double distance = squareRootDist(startX, startY, targetEndX, targetEndY);
	return distance / ctx->config.getMaxDefaultSpeed();
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
		double distFromStart, SHARED_PTR<RouteSegment> segment,int segmentEnd, SHARED_PTR<RouteSegment> inputNext,
		bool reverseWay, bool addSameRoadFutureDirection);

int calculateSizeOfSearchMaps(SEGMENTS_QUEUE graphDirectSegments, SEGMENTS_QUEUE graphReverseSegments,
		VISITED_MAP visitedDirectSegments, VISITED_MAP visitedOppositeSegments) {
	int sz = visitedDirectSegments.size() * sizeof(pair<int64_t, SHARED_PTR<RouteSegment> > );
	sz += visitedOppositeSegments.size()*sizeof(pair<int64_t, SHARED_PTR<RouteSegment> >);
	sz += graphDirectSegments.size()*sizeof(SHARED_PTR<RouteSegment>);
	sz += graphReverseSegments.size()*sizeof(SHARED_PTR<RouteSegment>);
	return sz;
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
	if(ctx->config.initialDirection > -180 && ctx->config.initialDirection < 180) {
		ctx->firstRoadId = (start->road->id << ROUTE_POINTS) + start->getSegmentStart();
		double plusDir = start->road->directionRoute(start->getSegmentStart(), true);
		double diff = plusDir - ctx->config.initialDirection;
		if(abs(alignAngleDifference(diff)) <= M_PI / 3) {
			ctx->firstRoadDirection = 1;
		} else if(abs(alignAngleDifference(diff - M_PI )) <= M_PI / 3) {
			ctx->firstRoadDirection = -1;
		}

	}

	SegmentsComparator sgmCmp(ctx);
	SEGMENTS_QUEUE graphDirectSegments(sgmCmp);
	SEGMENTS_QUEUE graphReverseSegments(sgmCmp);

	// Set to not visit one segment twice (stores road.id << X + segmentStart)
	VISITED_MAP visitedDirectSegments;
	VISITED_MAP visitedOppositeSegments;

	
	// for start : f(start) = g(start) + h(start) = 0 + h(start) = h(start)
	int targetEndX = end->road->pointsX[end->segmentStart];
	int targetEndY = end->road->pointsY[end->segmentStart];
	int startX = start->road->pointsX[start->segmentStart];
	int startY = start->road->pointsY[start->segmentStart];
	float estimatedDistance = (float) h(ctx, targetEndX, targetEndY, startX, startY);
	end->distanceToEnd = start->distanceToEnd = estimatedDistance;

	graphDirectSegments.push(start);
	graphReverseSegments.push(end);

	// Extract & analyze segment with min(f(x)) from queue while final segment is not found
	bool inverse = false;
	bool init = false;

	NonHeuristicSegmentsComparator nonHeuristicSegmentsComparator;
	SEGMENTS_QUEUE * graphSegments;
	if(inverse) {
		graphSegments = &graphReverseSegments;
	} else {
		graphSegments = &graphDirectSegments;
	}
	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "Start");
	SHARED_PTR<RouteSegment> finalSegment;
	while (graphSegments->size() > 0) {
		SHARED_PTR<RouteSegment> segment = graphSegments->top();
		graphSegments->pop();
		// Memory management
		// ctx.memoryOverhead = calculateSizeOfSearchMaps(graphDirectSegments, graphReverseSegments, visitedDirectSegments, visitedOppositeSegments);	

		if(segment->isFinal()) {
			finalSegment = segment;
			ctx->finalRouteSegment = segment;
			break;
		}

		
		if (!inverse) {
			processRouteSegment(ctx, false, graphDirectSegments, visitedDirectSegments, 
						segment, visitedOppositeSegments, true);
			processRouteSegment(ctx, false, graphDirectSegments, visitedDirectSegments, 
						segment, visitedOppositeSegments, false);
		} else {
			processRouteSegment(ctx, true, graphReverseSegments, visitedOppositeSegments, segment,
						visitedDirectSegments, true);
			processRouteSegment(ctx, true, graphReverseSegments, visitedOppositeSegments,segment,
						visitedDirectSegments, false);
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
		if (graphReverseSegments.size() == 0 || graphDirectSegments.size() == 0) {
			break;
		}
		if (!init) {
			inverse = !inverse;
			init = true;
		} else if (ctx->planRouteIn2Directions()) {
			inverse = !nonHeuristicSegmentsComparator(graphDirectSegments.top(), graphReverseSegments.top());
			if (graphDirectSegments.size() * 1.3 > graphReverseSegments.size()) {
				inverse = true;
			} else if (graphDirectSegments.size() < 1.3 * graphReverseSegments.size()) {
				inverse = false;
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
			VISITED_MAP& visitedSegments, SHARED_PTR<RouteSegment> segment, bool direction, 
			SHARED_PTR<RouteDataObject> road) {
	bool directionAllowed;
	int middle = segment->segmentStart;
	int oneway = ctx->config.isOneWay(road);
	// use positive direction as agreed
	if (!reverseWaySearch) {
		if(direction){
			directionAllowed = oneway >= 0;
		} else {
			directionAllowed = oneway <= 0;
		}
	} else {
		if(direction){
			directionAllowed = oneway <= 0;
		} else {
			directionAllowed = oneway >= 0;
		}
	}
	if(direction) {
		if(middle == road->pointsX.size() - 1 ||
				visitedSegments.find(calculateRoutePointId(road, middle, true)) != visitedSegments.end() || 
				segment->allowedDirection == -1) {
			directionAllowed = false;
		}	
	} else {
		if(middle == 0 || 
				visitedSegments.find(calculateRoutePointId(road, middle - 1, false)) != visitedSegments.end() || 
				segment->allowedDirection == 1) {
			directionAllowed = false;
		}	
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
		SHARED_PTR<RouteDataObject> road, int segmentEnd, bool positive, int intervalId, 
		float segmentDist, float obstaclesTime) {
	int64_t opp = calculateRoutePointId(road, intervalId, !positive);
	SHARED_PTR<RouteSegment> opposite = oppositeSegments[opp];
	if (opposite.get() != NULL && opposite->getSegmentStart() == segmentEnd) {
		SHARED_PTR<RouteSegment> frs = SHARED_PTR<RouteSegment>(new RouteSegment(road, segment->getSegmentStart()));
		float distStartObstacles = segment->distanceFromStart + calculateTimeWithObstacles(ctx, road, segmentDist , obstaclesTime);
		frs->parentRoute = segment->parentRoute;
		frs->parentSegmentEnd = segment -> parentSegmentEnd;
		frs->reverseWaySearch = reverseWaySearch? 1 : -1;
		frs->distanceFromStart = opposite->distanceFromStart + distStartObstacles;
		frs->distanceToEnd = 0;
		frs->opposite = opposite;
		graphSegments.push(frs);
		return true;			
	}
	return false;
}

void processRouteSegment(RoutingContext* ctx, bool reverseWaySearch, SEGMENTS_QUEUE& graphSegments,
		VISITED_MAP& visitedSegments, SHARED_PTR<RouteSegment> segment, 
		VISITED_MAP& oppositeSegments, bool direction) {
	
	SHARED_PTR<RouteDataObject> road = segment->road;
	bool initDirectionAllowed = checkIfInitialMovementAllowedOnSegment(ctx, reverseWaySearch, visitedSegments, segment, direction, road);
	bool directionAllowed = initDirectionAllowed; 
	// Go through all point of the way and find ways to continue
	// ! Actually there is small bug when there is restriction to move forward on the way (it doesn't take into account)
	float obstaclesTime = 0;
	if(segment->parentRoute.get() != NULL) {
		obstaclesTime = ctx->config.calculateTurnTime(segment, direction? segment->road->pointsX.size() - 1 : 0,
					segment->parentRoute, segment->parentSegmentEnd);		
	}
	if(ctx->firstRoadId == calculateRoutePointId(road, segment->getSegmentStart(), true) ) {
		if(ctx->firstRoadDirection < 0) {
			obstaclesTime += 500;
		} else if(ctx->firstRoadDirection > 0) {
			obstaclesTime += 500;
		}
	}
	float segmentDist = 0;
	// +/- diff from middle point
	int segmentEnd = segment->getSegmentStart();
	while (directionAllowed) {
		int prevInd = segmentEnd;
		if(direction) {
			segmentEnd ++;
		} else {
			segmentEnd --;
		}
		if (segmentEnd < 0 || segmentEnd >= road->pointsX.size()) {
			directionAllowed = false;
			continue;
		}
		int intervalId = direction ? segmentEnd - 1 : segmentEnd;
		visitedSegments[calculateRoutePointId(road, intervalId, direction)]=segment;
		int x = road->pointsX[segmentEnd];
		int y = road->pointsY[segmentEnd];
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
		
		bool alreadyVisited = checkIfOppositieSegmentWasVisited(ctx, reverseWaySearch, graphSegments, segment, oppositeSegments, road,
				segmentEnd, direction, intervalId, segmentDist, obstaclesTime);
		if (alreadyVisited) {
			directionAllowed = false;
			continue;
		}
			
		// could be expensive calculation
		// 3. get intersected ways
		SHARED_PTR<RouteSegment> next = ctx->loadRouteSegment(x, y); // ctx.config.memoryLimitation - ctx.memoryOverhead
		if(next.get() != NULL && 
				!((next.get() == segment.get() || next->road->id == road->id) && next->next.get() == NULL)) {
			// check if there are outgoing connections in that case we need to stop processing
			bool outgoingConnections = false;
			SHARED_PTR<RouteSegment> r = next;
			while(r.get() != NULL && !outgoingConnections) {
				// potential roundtrip
				if(r->road->id != road->id || r->getSegmentStart() != 0 /*|| !ctx->config.isOneWay(road)*/){
					outgoingConnections = true;
				}
				r = r->next;
			}
			if (outgoingConnections) {
				directionAllowed = false;
			}
			
			float distStartObstacles = segment->distanceFromStart + calculateTimeWithObstacles(ctx, road, segmentDist , obstaclesTime);
			processIntersections(ctx, graphSegments, visitedSegments, 
					distStartObstacles, segment, segmentEnd, 
					next, reverseWaySearch, outgoingConnections);
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

void printRoad(const char* prefix, SHARED_PTR<RouteSegment> segment) {
	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "%s road=%lld ind=%d ds=%f es=%f pend=%d parent=%lld ",
		prefix, segment->road->id, segment->getSegmentStart(), segment->distanceFromStart,
		segment->distanceToEnd, 
		segment->parentRoute.get() != NULL? segment->parentSegmentEnd : 0,
		segment->parentRoute.get() != NULL? segment->parentRoute->road->id : 0);	
}

void processIntersections(RoutingContext* ctx, SEGMENTS_QUEUE& graphSegments, VISITED_MAP& visitedSegments,
		double distFromStart, SHARED_PTR<RouteSegment> segment,int segmentEnd, SHARED_PTR<RouteSegment> inputNext,
		bool reverseWay, bool addSameRoadFutureDirection) {
	
	int8_t searchDirection = reverseWay ? (int8_t)-1 : (int8_t)1;
	bool thereAreRestrictions = proccessRestrictions(ctx, segment->road, inputNext, reverseWay);
	vector<SHARED_PTR<RouteSegment> >::iterator nextIterator;
	if (thereAreRestrictions) {
		nextIterator = ctx->segmentsToVisitPrescripted.begin();
		if(TRACE_ROUTING) {
		 	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "  >> There are restrictions");
		 }
	}

	// Calculate possible ways to put into priority queue
	SHARED_PTR<RouteSegment> next = inputNext;
	bool hasNext = !thereAreRestrictions || nextIterator != ctx->segmentsToVisitPrescripted.end();
	while (hasNext) {
		if (thereAreRestrictions) {
			next = *nextIterator;
		}
		bool nextPlusNotAllowed = (next->getSegmentStart() == next->road->pointsX.size() - 1) ||
					visitedSegments.find(calculateRoutePointId(next->road, next->getSegmentStart(), true)) != visitedSegments.end();
		bool nextMinusNotAllowed = (next->getSegmentStart() == 0) ||
					visitedSegments.find(calculateRoutePointId(next->road, next->getSegmentStart() - 1, false)) != visitedSegments.end();
		bool sameRoadFutureDirection = next->road->id == segment->road->id && next->getSegmentStart() == segmentEnd;
		// road.id could be equal on roundabout, but we should accept them
		bool alreadyVisited = nextPlusNotAllowed && nextMinusNotAllowed;
		bool skipRoad = sameRoadFutureDirection && !addSameRoadFutureDirection;

		
		if (!alreadyVisited && !skipRoad) {
			int targetEndX = reverseWay? ctx->startX : ctx->endX;
			int targetEndY = reverseWay? ctx->startY : ctx->endY;
			float distanceToEnd = h(ctx, segment->road->pointsX[segmentEnd],
					segment->road->pointsY[segmentEnd], targetEndX, targetEndY);
			// assigned to wrong direction
			if(next->directionAssgn == -searchDirection){
				next = SHARED_PTR<RouteSegment> (new RouteSegment(next->road, next->getSegmentStart()));
			}
				 
			if (next->parentRoute.get() == NULL
					|| roadPriorityComparator(next->distanceFromStart, next->distanceToEnd, distFromStart, distanceToEnd,
						ctx->getHeuristicCoefficient()) > 0) {
				if (next->parentRoute.get() != NULL) {
					//if (!graphSegments.remove(next)) {
						OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Should be handled by direction flag");
					//throw new IllegalStateException("Should be handled by direction flag");
					//} 
				} 
				next->directionAssgn = searchDirection;
				next->distanceFromStart = distFromStart;
				next->distanceToEnd = distanceToEnd;
				if(sameRoadFutureDirection) {
					next->allowedDirection = (uint8_t) (segment->getSegmentStart() < next->getSegmentStart() ? 1 : - 1);
				}
				if(TRACE_ROUTING) {
					printRoad("  >>", next);
				}
				// put additional information to recover whole route after
				next->parentRoute = segment;
				next->parentSegmentEnd = segmentEnd ;
				
				graphSegments.push(next);
			}
			
			
		} else if(!sameRoadFutureDirection){
			// the segment was already visited! We need to follow better route if it exists
			// that is very strange situation and almost exception (it can happen when we underestimate distnceToEnd)

			if (next->directionAssgn == searchDirection && 
					distFromStart < next->distanceFromStart && next->road->id != segment->road->id) {
				if(ctx->getHeuristicCoefficient() <= 1) {
					OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Direction update distance from start %f < %f", 
					distFromStart, next->distanceFromStart );
				//	throw new IllegalStateException("distance from start " + distFromStart + " < " +next.distanceFromStart); 
				}
				// That code is incorrect (when segment is processed itself,
				// then it tries to make wrong u-turn) -
				// this situation should be very carefully checked in future (seems to be fixed)
				next->distanceFromStart = distFromStart;
				next->parentRoute = segment;
				next->parentSegmentEnd = segmentEnd;
			}
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
