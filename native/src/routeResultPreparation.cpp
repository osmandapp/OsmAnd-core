#ifndef _OSMAND_ROUTE_RESULT_PREPARATION_CPP
#define _OSMAND_ROUTE_RESULT_PREPARATION_CPP
#include "routingContext.h"
#include "routeSegment.h"
#include "routeSegmentResult.h"
#include "turnType.h"
#include "binaryRoutePlanner.h"
#include "multipolygons.h"

const int MAX_SPEAK_PRIORITY = 5;
const float TURN_DEGREE_MIN = 45;
const float UNMATCHED_TURN_DEGREE_MINIMUM = 45;
const float SPLIT_TURN_DEGREE_NOT_STRAIGHT = 100;
const string UNMATCHED_HIGHWAY_TYPE = "unmatched";

struct RoadSplitStructure {
    bool keepLeft = false;
    bool keepRight = false;
    bool speak = false;
    vector<vector<int> > leftLanesInfo;
    int leftLanes = 0;
    vector<vector<int> > rightLanesInfo;
    int rightLanes = 0;
    int roadsOnLeft = 0;
    int addRoadsOnLeft = 0;
    int roadsOnRight = 0;
    int addRoadsOnRight = 0;
};

struct MergeTurnLaneTurn {
    SHARED_PTR<TurnType> turn;
    vector<int> originalLanes;
    vector<int> disabledLanes;
    int activeStartIndex;
    int activeEndIndex;
    int activeLen;
    
    MergeTurnLaneTurn(SHARED_PTR<RouteSegmentResult>& segment) : activeStartIndex(-1), activeEndIndex(-1), activeLen(0) {
        turn = segment->turnType;
        if (turn) {
            originalLanes = turn->getLanes();
        }
        if (!originalLanes.empty()) {
            disabledLanes.clear();
            disabledLanes.resize(originalLanes.size());
            for (int i = 0; i < originalLanes.size(); i++) {
                int ln = originalLanes[i];
                disabledLanes[i] = ln & ~1;
                if ((ln & 1) > 0) {
                    if (activeStartIndex == -1) {
                        activeStartIndex = i;
                    }
                    activeEndIndex = i;
                    activeLen++;
                }
            }
        }
    }
    
    inline bool isActiveTurnMostLeft() {
        return activeStartIndex == 0;
    }
    inline bool isActiveTurnMostRight() {
        return activeEndIndex == originalLanes.size() - 1;
    }
};

struct CombineAreaRoutePoint {
    int x31;
    int y31;
    int originalIndex;
};

vector<int> getPossibleTurns(vector<int>& oLanes, bool onlyPrimary, bool uniqueFromActive);

long getPoint(SHARED_PTR<RouteDataObject>& road, int pointInd) {
    return (((long) road->pointsX[pointInd]) << 31) + (long) road->pointsY[pointInd];
}

bool isMotorway(SHARED_PTR<RouteSegmentResult>& s) {
    string h = s->object->getHighway();
    return "motorway" == h || "motorway_link" == h  || "trunk" == h || "trunk_link" == h;
}

void ignorePrecedingStraightsOnSameIntersection(bool leftside, vector<SHARED_PTR<RouteSegmentResult> >& result) {
    //Issue 2571: Ignore TurnType::C if immediately followed by another turn in non-motorway cases, as these likely belong to the very same intersection
    SHARED_PTR<RouteSegmentResult> nextSegment = nullptr;
    double distanceToNextTurn = 999999;
    for (int i = (int)result.size() - 1; i >= 0; i--) {
        // Mark next "real" turn
        if (nextSegment && nextSegment->turnType &&
            nextSegment->turnType->getValue() != TurnType::C && !isMotorway(nextSegment)) {
            if (distanceToNextTurn == 999999) {
                distanceToNextTurn = 0;
            }
        }
        auto currentSegment = result[i];
        // Identify preceding goStraights within distance limit and suppress
        if (currentSegment) {
            distanceToNextTurn += currentSegment->distance;
            if (currentSegment->turnType &&
                currentSegment->turnType->getValue() == TurnType::C && distanceToNextTurn <= 100) {
                result[i]->turnType->setSkipToSpeak(true);
            } else {
                nextSegment = currentSegment;
                distanceToNextTurn = 999999;
            }
        }
    }
}

void validateAllPointsConnected(vector<SHARED_PTR<RouteSegmentResult> >& result) {
    for (int i = 1; i < result.size(); i++) {
        auto rr = result[i];
        auto pr = result[i - 1];
        double d = measuredDist31(pr->object->pointsX[pr->getEndPointIndex()], pr->object->pointsY[pr->getEndPointIndex()], rr->object->pointsX[rr->getStartPointIndex()], rr->object->pointsY[rr->getStartPointIndex()]);
        if (d > 0) {
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Points are not connected: %p(%d) -> %p(%d) %f meters", pr->object.get(), pr->getEndPointIndex(), rr->object.get(), rr->getStartPointIndex(), d);
        }
    }
}

// try to attach all segments except with current id
void attachSegments(RoutingContext* ctx, SHARED_PTR<RouteSegment>& routeSegment, SHARED_PTR<RouteDataObject>& road, SHARED_PTR<RouteSegmentResult>& rr, long previousRoadId, int pointInd, long prevL, long nextL) {
    if (routeSegment->road->getId() != road->getId() && routeSegment->road->getId() != previousRoadId) {
        auto addRoad = routeSegment->road;
        //checkAndInitRouteRegion(ctx, addRoad); ? TODO
        // Future: restrictions can be considered as well
        int oneWay = ctx->config->router->isOneWay(addRoad);
        if (oneWay >= 0 && routeSegment->getSegmentStart() < addRoad->getPointsLength() - 1) {
            long pointL = getPoint(addRoad, routeSegment->getSegmentStart() + 1);
            if (pointL != nextL && pointL != prevL) {
                // if way contains same segment (nodes) as different way (do not attach it)
                auto rsr = std::make_shared<RouteSegmentResult>(addRoad, routeSegment->getSegmentStart(), addRoad->getPointsLength() - 1);
                rr->attachRoute(pointInd, rsr);
            }
        }
        if (oneWay <= 0 && routeSegment->getSegmentStart() > 0) {
            long pointL = getPoint(addRoad, routeSegment->getSegmentStart() - 1);
            // if way contains same segment (nodes) as different way (do not attach it)
            if (pointL != nextL && pointL != prevL) {
                auto rsr = std::make_shared<RouteSegmentResult>(addRoad, routeSegment->getSegmentStart(), 0);
                rr->attachRoute(pointInd, rsr);
            }
        }
    }
}

void attachRoadSegments(RoutingContext* ctx, vector<SHARED_PTR<RouteSegmentResult> >& result, int routeInd, int pointInd, bool plus) {
    auto rr = result[routeInd];
    auto road = rr->object;
    long nextL = pointInd < road->getPointsLength() - 1 ? getPoint(road, pointInd + 1) : 0;
    long prevL = pointInd > 0 ? getPoint(road, pointInd - 1) : 0;
    
    // attach additional roads to represent more information about the route
    SHARED_PTR<RouteSegmentResult> previousResult = nullptr;
    
    // by default make same as this road id
    long previousRoadId = road->getId();
    if (pointInd == rr->getStartPointIndex() && routeInd > 0) {
        previousResult = result[routeInd - 1];
        previousRoadId = previousResult->object->getId();
        if (previousRoadId != road->getId()) {
            if (previousResult->getStartPointIndex() < previousResult->getEndPointIndex() && previousResult->getEndPointIndex() < previousResult->object->getPointsLength() - 1) {
                auto segResult = std::make_shared<RouteSegmentResult>(previousResult->object, previousResult->getEndPointIndex(), previousResult->object->getPointsLength() - 1);
                rr->attachRoute(pointInd, segResult);
            } else if (previousResult->getStartPointIndex() > previousResult->getEndPointIndex() && previousResult->getEndPointIndex() > 0) {
                auto segResult = std::make_shared<RouteSegmentResult>(previousResult->object, previousResult->getEndPointIndex(), 0);
                rr->attachRoute(pointInd, segResult);
            }
        }
    }
    if (!rr->getPreAttachedRoutes(pointInd).empty()) {
        const auto& list = rr->getPreAttachedRoutes(pointInd);
        for (auto r : list) {
            auto rs = std::make_shared<RouteSegment>(r->object, r->getStartPointIndex());
            attachSegments(ctx, rs, road, rr, previousRoadId, pointInd, prevL, nextL);
        }
    } else {
        auto rt = ctx->loadRouteSegment(road->pointsX[pointInd], road->pointsY[pointInd]);
        if (rt) {
            auto rs = rt->next;
            while (rs) {
                attachSegments(ctx, rs, road, rr, previousRoadId, pointInd, prevL, nextL);
                rs = rs->next;
            }
        }
    }
}

void splitRoadsAndAttachRoadSegments(RoutingContext* ctx, vector<SHARED_PTR<RouteSegmentResult> >& result) {
    for (int i = 0; i < result.size(); i++) {
        //ctx->unloadUnusedTiles(ctx->config->memoryLimitation);
        
        auto rr = result[i];
        auto road = rr->object;
        //checkAndInitRouteRegion(ctx, road); ? TODO
        bool plus = rr->getStartPointIndex() < rr->getEndPointIndex();
        int next;
        bool unmatched = UNMATCHED_HIGHWAY_TYPE == rr->object->getHighway();
        for (int j = rr->getStartPointIndex(); j != rr->getEndPointIndex(); j = next) {
            next = plus ? j + 1 : j - 1;
            if (j == rr->getStartPointIndex()) {
                attachRoadSegments(ctx, result, i, j, plus);
            }
            if (next != rr->getEndPointIndex()) {
                attachRoadSegments(ctx, result, i, next, plus);
            }
            const auto& attachedRoutes = rr->getAttachedRoutes(next);
            bool tryToSplit = next != rr->getEndPointIndex() && !rr->object->roundabout();
            if (rr->getDistance(next, plus ) == 0) {
                // same point will be processed next step
                tryToSplit = false;
            }
            if (tryToSplit) {
                float distBearing = unmatched ? DIST_BEARING_DETECT_UNMATCHED : DIST_BEARING_DETECT;
                // avoid small zigzags
                float before = rr->getBearingEnd(next, distBearing);
                float after = rr->getBearingBegin(next, distBearing);
                if(rr->getDistance(next, plus) < distBearing) {
                    after = before;
                } else if(rr->getDistance(next, !plus) < distBearing) {
                    before = after;
                }
                double contAngle = abs(degreesDiff(before, after));
                bool straight = contAngle < TURN_DEGREE_MIN;
                bool isSplit = false;
                
                if (unmatched && abs(contAngle) >= UNMATCHED_TURN_DEGREE_MINIMUM) {
                    isSplit = true;
                }
                // split if needed
                for (auto rs : attachedRoutes) {
                    double diff = degreesDiff(before, rs->getBearingBegin());
                    if (abs(diff) <= TURN_DEGREE_MIN) {
                        isSplit = true;
                    } else if (!straight && abs(diff) < SPLIT_TURN_DEGREE_NOT_STRAIGHT) {
                        isSplit = true;
                    }
                }
                if (isSplit) {
                    int endPointIndex = rr->getEndPointIndex();
                    auto split = std::make_shared<RouteSegmentResult>(rr->object, next, endPointIndex);
                    split->copyPreattachedRoutes(rr, abs(next - rr->getStartPointIndex()));
                    rr->setEndPointIndex(next);
                    result.insert(result.begin() + i + 1, split);
                    i++;
                    // switch current segment to the splitted
                    rr = split;
                }
            }
        }
    }
}

void calculateTimeSpeed(RoutingContext* ctx, vector<SHARED_PTR<RouteSegmentResult> >& result) {
    // for Naismith
    bool usePedestrianHeight = ctx->config->router->getProfile() == GeneralRouterProfile::PEDESTRIAN && ctx->config->router->heightObstacles;
    
    for (int i = 0; i < result.size(); i++) {
        auto rr = result[i];
        auto road = rr->object;
        double distOnRoadToPass = 0;
        double speed = ctx->config->router->defineVehicleSpeed(road);
        if (speed == 0) {
            speed = ctx->config->router->getDefaultSpeed();
        } else {
            if (speed > 15) {
                // decrease speed proportionally from 15ms (50kmh)
                // reference speed 30ms (108kmh) - 2ms (7kmh)
                speed = speed - (speed / 15.f - 1) * 2.f;
            }
        }
        bool plus = rr->getStartPointIndex() < rr->getEndPointIndex();
        int next;
        double distance = 0;
        
        //for Naismith
        float prevHeight = -99999.0f;
        vector<double> heightDistanceArray;
        if (usePedestrianHeight) {
            road->calculateHeightArray();
            heightDistanceArray = road->heightDistanceArray;
        }
        
        for (int j = rr->getStartPointIndex(); j != rr->getEndPointIndex(); j = next) {
            next = plus ? j + 1 : j - 1;
            double d = measuredDist31(road->pointsX[j], road->pointsY[j], road->pointsX[next], road->pointsY[next]);
            distance += d;
            double obstacle = ctx->config->router->defineObstacle(road, j, rr->getStartPointIndex() < rr->getEndPointIndex());
            if (obstacle < 0) {
                obstacle = 0;
            }
            distOnRoadToPass += d / speed + obstacle;
            
            //for Naismith
            if (usePedestrianHeight) {
                int heightIndex = 2 * j + 1;
                if (!heightDistanceArray.empty() && heightIndex < heightDistanceArray.size()) {
                    float height = heightDistanceArray[heightIndex];
                    if (prevHeight != -99999.0f) {
                        float heightDiff = height - prevHeight;
                        if (heightDiff > 0) {  //ascent only
                            distOnRoadToPass += heightDiff * 6.0f;  //Naismith's rule: add 1 hour per every 600m of ascent
                        }
                    }
                    prevHeight = height;
                }
            }
        }
        // last point turn time can be added
        // if(i + 1 < result.size()) { distOnRoadToPass += ctx.getRouter().calculateTurnTime(); }
        rr->segmentTime = (float) distOnRoadToPass;
        rr->segmentSpeed = (float) speed;
        rr->distance = (float) distance;
    }
}

SHARED_PTR<TurnType> processRoundaboutTurn(vector<SHARED_PTR<RouteSegmentResult> >& result, int i, bool leftSide, SHARED_PTR<RouteSegmentResult>& prev, SHARED_PTR<RouteSegmentResult>& rr) {
    int exit = 1;
    auto last = rr;
    auto firstRoundabout = rr;
    auto lastRoundabout = rr;
    for (int j = i; j < result.size(); j++) {
        auto rnext = result[j];
        last = rnext;
        if (rnext->object->roundabout()) {
            lastRoundabout = rnext;
            bool plus = rnext->getStartPointIndex() < rnext->getEndPointIndex();
            int k = rnext->getStartPointIndex();
            if (j == i) {
                // first exit could be immediately after roundabout enter
                //					k = plus ? k + 1 : k - 1;
            }
            while (k != rnext->getEndPointIndex()) {
                int attachedRoads = (int)rnext->getAttachedRoutes(k).size();
                if (attachedRoads > 0) {
                    exit++;
                }
                k = plus ? k + 1 : k - 1;
            }
        } else {
            break;
        }
    }
    // combine all roundabouts
    auto t = TurnType::getPtrExitTurn(exit, 0, leftSide);
    // usually covers more than expected
    float turnAngleBasedOnOutRoads = (float) degreesDiff(last->getBearingBegin(), prev->getBearingEnd());
    // usually covers less than expected
    float turnAngleBasedOnCircle = (float) -degreesDiff(firstRoundabout->getBearingBegin(), lastRoundabout->getBearingEnd() + 180);
    if (abs(turnAngleBasedOnOutRoads - turnAngleBasedOnCircle) > 180) {
        t->setTurnAngle(turnAngleBasedOnCircle);
    } else {
        t->setTurnAngle((turnAngleBasedOnCircle + turnAngleBasedOnOutRoads) / 2) ;
    }
    return t;
}

string getTurnLanesString(SHARED_PTR<RouteSegmentResult>& segment) {
    if (segment->object->getOneway() == 0) {
        if (segment->isForwardDirection()) {
            return segment->object->getValue("turn:lanes:forward");
        } else {
            return segment->object->getValue("turn:lanes:backward");
        }
    } else {
        return segment->object->getValue("turn:lanes");
    }
}

vector<int> calculateRawTurnLanes(string turnLanes, int calcTurnType) {
    vector<string> splitLaneOptions = split_string(turnLanes, "|");
    vector<int> lanes(splitLaneOptions.size());
    for (int i = 0; i < splitLaneOptions.size(); i++) {
        vector<string> laneOptions = split_string(splitLaneOptions[i], ";");
        for (int j = 0; j < laneOptions.size(); j++) {
            int turn = TurnType::convertType(laneOptions[j]);
            int primary = TurnType::getPrimaryTurn(lanes[i]);
            if (primary == 0) {
                TurnType::setPrimaryTurnAndReset(lanes, i, turn);
            } else {
                if (turn == calcTurnType ||
                    (TurnType::isRightTurn(calcTurnType) && TurnType::isRightTurn(turn)) ||
                    (TurnType::isLeftTurn(calcTurnType) && TurnType::isLeftTurn(turn))
                    ) {
                    TurnType::setPrimaryTurnShiftOthers(lanes, i, turn);
                } else if (TurnType::getSecondaryTurn(lanes[i]) == 0) {
                    TurnType::setSecondaryTurn(lanes, i, turn);
                } else if (TurnType::getTertiaryTurn(lanes[i]) == 0) {
                    TurnType::setTertiaryTurn(lanes, i, turn);
                } else {
                    // ignore
                }
            }
        }
    }
    return lanes;
}

bool setAllowedLanes(int mainTurnType, vector<int>& lanesArray) {
    bool turnSet = false;
    for (int i = 0; i < lanesArray.size(); i++) {
        if (TurnType::getPrimaryTurn(lanesArray[i]) == mainTurnType) {
            lanesArray[i] |= 1;
            turnSet = true;
        }
    }
    return turnSet;
}

vector<int> getTurnLanesInfo(SHARED_PTR<RouteSegmentResult>& prevSegm, int mainTurnType) {
    string turnLanes = getTurnLanesString(prevSegm);
    vector<int> lanesArray;
    if (turnLanes.empty()) {
        if(prevSegm->turnType && !prevSegm->turnType->getLanes().empty() && prevSegm->distance < 100) {
            const auto& lns = prevSegm->turnType->getLanes();
            vector<int> lst;
            for(int i = 0; i < lns.size(); i++) {
                if (lns[i] % 2 == 1) {
                    lst.push_back((lns[i] >> 1) << 1);
                }
            }
            if (lst.empty()) {
                return lanesArray;
            }
            lanesArray = lst;
        } else {
            return lanesArray;
        }
    } else {
        lanesArray = calculateRawTurnLanes(turnLanes, mainTurnType);
    }
    // Manually set the allowed lanes.
    bool isSet = setAllowedLanes(mainTurnType, lanesArray);
    if (!isSet && lanesArray.size() > 0) {
        // In some cases (at least in the US), the rightmost lane might not have a right turn indicated as per turn:lanes,
        // but is allowed and being used here. This section adds in that indicator.  The same applies for where leftSide is true.
        bool leftTurn = TurnType::isLeftTurn(mainTurnType);
        int ind = leftTurn? 0 : (int)lanesArray.size() - 1;
        int primaryTurn = TurnType::getPrimaryTurn(lanesArray[ind]);
        int st = TurnType::getSecondaryTurn(lanesArray[ind]);
        if (leftTurn) {
            if (!TurnType::isLeftTurn(primaryTurn)) {
                // This was just to make sure that there's no bad data.
                TurnType::setPrimaryTurnAndReset(lanesArray, ind, TurnType::TL);
                TurnType::setSecondaryTurn(lanesArray, ind, primaryTurn);
                TurnType::setTertiaryTurn(lanesArray, ind, st);
                primaryTurn = TurnType::TL;
                lanesArray[ind] |= 1;
            }
        } else {
            if (!TurnType::isRightTurn(primaryTurn)) {
                // This was just to make sure that there's no bad data.
                TurnType::setPrimaryTurnAndReset(lanesArray, ind, TurnType::TR);
                TurnType::setSecondaryTurn(lanesArray, ind, primaryTurn);
                TurnType::setTertiaryTurn(lanesArray, ind, st);
                primaryTurn = TurnType::TR;
                lanesArray[ind] |= 1;
            }
        }
        setAllowedLanes(primaryTurn, lanesArray);
    }
    return lanesArray;
}

int highwaySpeakPriority(const string& highway) {
    if (highway == "" || endsWith(highway, "track") || endsWith(highway, "services") || endsWith(highway, "service")
       || endsWith(highway, "path")) {
        return MAX_SPEAK_PRIORITY;
    }
    if (endsWith(highway, "_link")  || endsWith(highway, "unclassified") || endsWith(highway, "road")
        || endsWith(highway, "living_street") || endsWith(highway, "residential") || endsWith(highway, "tertiary"))  {
        return 1;
    }
    return 0;
}

void filterMinorStops(SHARED_PTR<RouteSegmentResult> seg)
{
    std::vector<uint32_t> stops;
    bool plus = seg->getStartPointIndex() < seg->getEndPointIndex();
    int next;

    for (int i = seg->getStartPointIndex(); i != seg->getEndPointIndex(); i = next)
    {
        next = plus ? i + 1 : i - 1;
        std::vector<uint32_t> pointTypes = seg->object->pointTypes[i];
        if (pointTypes.size() > 0)
        {
            for (int j = 0; j < pointTypes.size(); j++)
            {
                if (pointTypes[j] == seg->object->region->stopMinor)
                {
                    stops.push_back(i);
                }
            }
        }
    }

    for (uint32_t stop : stops)
    {
        std::vector<SHARED_PTR<RouteSegmentResult>> attachedRoutes = seg->getAttachedRoutes(stop);
        std::vector<SHARED_PTR<RouteSegmentResult>>::iterator it = attachedRoutes.begin();
        while (it != attachedRoutes.end())
        {
            const std::string highway = (*it)->object->getHighway();
            const std::string segHighway = seg->object->getHighway();
            int attStopPriority = highwaySpeakPriority(highway);
            int segStopPriority = highwaySpeakPriority(segHighway);
            if (segStopPriority < attStopPriority)
            {
                seg->object->removePointType(stop, seg->object->region->stopSign);
                break;
            }
            it++;
        }
    }
}

int countOccurrences(const string& haystack, char needle) {
    int count = 0;
    for (int i = 0; i < haystack.size(); i++) {
        if (haystack[i] == needle) {
            count++;
        }
    }
    return count;
}

int countLanesMinOne(SHARED_PTR<RouteSegmentResult>& attached) {
    bool oneway = attached->object->getOneway() != 0;
    int lns = attached->object->getLanes();
    if (lns == 0) {
        string tls = getTurnLanesString(attached);
        if (tls != "") {
            return max(1, countOccurrences(tls, '|'));
        }
    }
    if (oneway) {
        return max(1, lns);
    }
    if (attached->isForwardDirection() && attached->object->getValue("lanes:forward") != "") {
        int val = -1;
        if (sscanf(attached->object->getValue("lanes:forward").c_str(), "%d", &val) != EOF) {
            return val;
        }
    } else if (!attached->isForwardDirection() && attached->object->getValue("lanes:backward") != "") {
        int val = -1;
        if (sscanf(attached->object->getValue("lanes:backward").c_str(), "%d", &val) != EOF) {
            return val;
        }
    }
    return max(1, (lns + 1) / 2);
}

vector<int> parseTurnLanes(const SHARED_PTR<RouteDataObject>& ro, double dirToNorthEastPi) {
    string turnLanes;
    if (ro->getOneway() == 0) {
        // we should get direction to detect forward or backward
        double cmp = ro->directionRoute(0, true);
        if (abs(alignAngleDifference(dirToNorthEastPi -cmp)) < M_PI / 2) {
            turnLanes = ro->getValue("turn:lanes:forward");
        } else {
            turnLanes = ro->getValue("turn:lanes:backward");
        }
    } else {
        turnLanes = ro->getValue("turn:lanes");
    }
    if (turnLanes.empty()) {
        return vector<int>();
    }
    return calculateRawTurnLanes(turnLanes, 0);
}

vector<int> parseLanes(const SHARED_PTR<RouteDataObject>& ro, double dirToNorthEastPi) {
    int lns = 0;
    if (ro->getOneway() == 0) {
        // we should get direction to detect forward or backward
        double cmp = ro->directionRoute(0, true);
        
        if (abs(alignAngleDifference(dirToNorthEastPi -cmp)) < M_PI / 2) {
            if (!ro->getValue("lanes:forward").empty()) {
                lns = atoi(ro->getValue("lanes:forward").c_str());
            }
        } else {
            if (!ro->getValue("lanes:backward").empty()) {
                lns = atoi(ro->getValue("lanes:backward").c_str());
            }
        }
        if (lns == 0 && !ro->getValue("lanes").empty()) {
            lns = atoi(ro->getValue("lanes").c_str()) / 2;
        }
    } else {
        lns = atoi(ro->getValue("lanes").c_str());
    }
    if (lns > 0 ) {
        return vector<int>(lns);
    }
    return vector<int>();
}

RoadSplitStructure calculateRoadSplitStructure(SHARED_PTR<RouteSegmentResult>& prevSegm, SHARED_PTR<RouteSegmentResult>& currentSegm, vector<SHARED_PTR<RouteSegmentResult> >& attachedRoutes) {
    RoadSplitStructure rs;
    int speakPriority = max(highwaySpeakPriority(prevSegm->object->getHighway()), highwaySpeakPriority(currentSegm->object->getHighway()));
    for (auto attached : attachedRoutes) {
        bool restricted = false;
        for(int k = 0; k < prevSegm->object->getRestrictionLength(); k++) {
            if(prevSegm->object->getRestrictionId(k) == attached->object->getId() &&
               prevSegm->object->getRestrictionType(k) <= RESTRICTION_NO_STRAIGHT_ON) {
                restricted = true;
                break;
            }
        }
        if (restricted) {
            continue;
        }
        double ex = degreesDiff(attached->getBearingBegin(), currentSegm->getBearingBegin());
        double mpi = abs(degreesDiff(prevSegm->getBearingEnd(), attached->getBearingBegin()));
        int rsSpeakPriority = highwaySpeakPriority(attached->object->getHighway());
        int lanes = countLanesMinOne(attached);
        const auto& turnLanes = parseTurnLanes(attached->object, attached->getBearingBegin() * M_PI / 180);
        bool smallStraightVariation = mpi < TURN_DEGREE_MIN;
        bool smallTargetVariation = abs(ex) < TURN_DEGREE_MIN;
        bool attachedOnTheRight = ex >= 0;
        if (attachedOnTheRight) {
            rs.roadsOnRight++;
        } else {
            rs.roadsOnLeft++;
        }
        if (rsSpeakPriority != MAX_SPEAK_PRIORITY || speakPriority == MAX_SPEAK_PRIORITY) {
            if (smallTargetVariation || smallStraightVariation) {
                if (attachedOnTheRight) {
                    rs.keepLeft = true;
                    rs.rightLanes += lanes;
                    if (!turnLanes.empty()) {
                        rs.rightLanesInfo.push_back(turnLanes);
                    }
                } else {
                    rs.keepRight = true;
                    rs.leftLanes += lanes;
                    if (!turnLanes.empty()) {
                        rs.leftLanesInfo.push_back(turnLanes);
                    }
                }
                rs.speak = rs.speak || rsSpeakPriority <= speakPriority;
            } else {
                if (attachedOnTheRight) {
                    rs.addRoadsOnRight++;
                } else {
                    rs.addRoadsOnLeft++;
                }
            }
        }
    }
    return rs;
}

int findActiveIndex(vector<int>& rawLanes, vector<string>& splitLaneOptions, int lanes, bool left,
                              vector<vector<int> >& lanesInfo, int roads, int addRoads) {
    int activeStartIndex = -1;
    bool lookupSlightTurn = addRoads > 0;
    set<int> addedTurns;
    // if we have information increase number of roads per each turn direction
    int diffTurnRoads = roads;
    int increaseTurnRoads = 0;
    for (auto li : lanesInfo) {
        set<int> set;
        if (!li.empty()) {
            for(int k = 0; k < li.size(); k++) {
                TurnType::collectTurnTypes(li[k], set);
            }
        }
        increaseTurnRoads = max((int)set.size() - 1, 0);
    }
    
    for (int i = 0; i < rawLanes.size(); i++) {
        int ind = left ? i : (int)rawLanes.size() - i - 1;
        if (!lookupSlightTurn || TurnType::hasAnySlightTurnLane(rawLanes[ind])) {
            vector<string> laneTurns = split_string(splitLaneOptions[ind], ";");
            int cnt = 0;
            for(auto& lTurn : laneTurns) {
                auto added = addedTurns.insert(TurnType::convertType(lTurn));
                if (added.second) {
                    cnt++;
                    diffTurnRoads --;
                }
            }
            lanes -= cnt;
            //lanes--;
            // we already found slight turn others are turn in different direction
            lookupSlightTurn = false;
        }
        if (lanes < 0 || diffTurnRoads + increaseTurnRoads < 0) {
            activeStartIndex = ind;
            break;
        } else if (diffTurnRoads < 0 && activeStartIndex < 0) {
            activeStartIndex = ind;
        }
    }
    return activeStartIndex;
}

SHARED_PTR<TurnType> createSimpleKeepLeftRightTurn(bool leftSide, SHARED_PTR<RouteSegmentResult>& prevSegm,
                                                 SHARED_PTR<RouteSegmentResult>& currentSegm, RoadSplitStructure& rs) {
    double devation = abs(degreesDiff(prevSegm->getBearingEnd(), currentSegm->getBearingBegin()));
    bool makeSlightTurn = devation > 5 && (!isMotorway(prevSegm) || !isMotorway(currentSegm));
    SHARED_PTR<TurnType> t = nullptr;
    int laneType = TurnType::C;
    if (rs.keepLeft && rs.keepRight) {
        t = TurnType::ptrValueOf(TurnType::C, leftSide);
    } else if (rs.keepLeft) {
        t = TurnType::ptrValueOf(makeSlightTurn ? TurnType::TSLL : TurnType::KL, leftSide);
        if (makeSlightTurn) {
            laneType = TurnType::TSLL;
        }
    } else if (rs.keepRight) {
        t = TurnType::ptrValueOf(makeSlightTurn ? TurnType::TSLR : TurnType::KR, leftSide);
        if (makeSlightTurn) {
            laneType = TurnType::TSLR;
        }
    } else {
        return nullptr;
    }
    int current = countLanesMinOne(currentSegm);
    int ls = current + rs.leftLanes + rs.rightLanes;
    vector<int> lanes(ls);
    for (int it = 0; it < ls; it++) {
        if (it < rs.leftLanes || it >= rs.leftLanes + current) {
            lanes[it] = TurnType::C << 1;
        } else {
            lanes[it] = (laneType << 1) + 1;
        }
    }
    // sometimes links are
    if ((current <= rs.leftLanes + rs.rightLanes) && (rs.leftLanes > 1 || rs.rightLanes > 1)) {
        rs.speak = true;
    }
    t->setSkipToSpeak(!rs.speak);
    t->setLanes(lanes);
    return t;
}

int inferSlightTurnFromActiveLanes(vector<int>& oLanes, bool mostLeft, bool mostRight) {
    const auto possibleTurns = getPossibleTurns(oLanes, false, true);
    if (possibleTurns.size() == 0) {
        // No common turns, so can't determine anything.
        return 0;
    }
    int infer = 0;
    if (possibleTurns.size() == 1) {
        infer = possibleTurns[0];
    } else {
        if (mostLeft && !mostRight) {
            infer = possibleTurns[0];
        } else if (mostRight && !mostLeft) {
            infer = possibleTurns[possibleTurns.size() - 1];
        } else {
            infer = possibleTurns[1];
        }
    }
    return infer;
}

SHARED_PTR<TurnType> createKeepLeftRightTurnBasedOnTurnTypes(RoadSplitStructure& rs, SHARED_PTR<RouteSegmentResult>& prevSegm,
                                                           SHARED_PTR<RouteSegmentResult>& currentSegm, string turnLanes, bool leftSide) {
    // Maybe going straight at a 90-degree intersection
    auto t = TurnType::ptrValueOf(TurnType::C, leftSide);
    auto rawLanes = calculateRawTurnLanes(turnLanes, TurnType::C);
    bool possiblyLeftTurn = rs.roadsOnLeft == 0;
    bool possiblyRightTurn = rs.roadsOnRight == 0;
    for (int k = 0; k < rawLanes.size(); k++) {
        int turn = TurnType::getPrimaryTurn(rawLanes[k]);
        int sturn = TurnType::getSecondaryTurn(rawLanes[k]);
        int tturn = TurnType::getTertiaryTurn(rawLanes[k]);
        if (turn == TurnType::TU || sturn == TurnType::TU || tturn == TurnType::TU) {
            possiblyLeftTurn = true;
        }
        if (turn == TurnType::TRU || sturn == TurnType::TRU || tturn == TurnType::TRU) {
            possiblyRightTurn = true;
        }
    }
    
    if (rs.keepLeft || rs.keepRight) {
        vector<string> splitLaneOptions = split_string(turnLanes, "|");
        int activeBeginIndex = findActiveIndex(rawLanes, splitLaneOptions, rs.leftLanes, true,
                                               rs.leftLanesInfo, rs.roadsOnLeft, rs.addRoadsOnLeft);
        int activeEndIndex = findActiveIndex(rawLanes, splitLaneOptions, rs.rightLanes, false,
                                             rs.rightLanesInfo, rs.roadsOnRight, rs.addRoadsOnRight);
        if (activeBeginIndex == -1 || activeEndIndex == -1 || activeBeginIndex > activeEndIndex) {
            // something went wrong
            return createSimpleKeepLeftRightTurn(leftSide, prevSegm, currentSegm, rs);
        }
        for (int k = 0; k < rawLanes.size(); k++) {
            if (k >= activeBeginIndex && k <= activeEndIndex) {
                rawLanes[k] |= 1;
            }
        }
        int tp = inferSlightTurnFromActiveLanes(rawLanes, rs.keepLeft, rs.keepRight);
        // Checking to see that there is only one unique turn
        if (tp != 0) {
            // add extra lanes with same turn
            for(int i = 0; i < rawLanes.size(); i++) {
                if(TurnType::getSecondaryTurn(rawLanes[i]) == tp) {
                    TurnType::setSecondaryToPrimary(rawLanes, i);
                    rawLanes[i] |= 1;
                } else if(TurnType::getPrimaryTurn(rawLanes[i]) == tp) {
                    rawLanes[i] |= 1;
                }
            }
        }
        if (tp != t->getValue() && tp != 0) {
            t = TurnType::ptrValueOf(tp, leftSide);
        } else {
            if (rs.keepRight && TurnType::getSecondaryTurn(rawLanes[activeEndIndex]) == 0) {
                t = TurnType::ptrValueOf(TurnType::getPrimaryTurn(rawLanes[activeEndIndex]), leftSide);
            } else if (rs.keepLeft && TurnType::getSecondaryTurn(rawLanes[activeBeginIndex]) == 0) {
                t = TurnType::ptrValueOf(TurnType::getPrimaryTurn(rawLanes[activeBeginIndex]), leftSide);
            }
        }
    } else {
        // case for go straight and identify correct turn:lane to go straight
        const auto possibleTurns = getPossibleTurns(rawLanes, false, false);
        int tp = TurnType::C;
        if (possibleTurns.size() == 1) {
            tp = possibleTurns[0];
        } else if (possibleTurns.size() == 3) {
            if ((!possiblyLeftTurn || !possiblyRightTurn) && TurnType::isSlightTurn(possibleTurns[1])) {
                tp = possibleTurns[1];
                t = TurnType::ptrValueOf(tp, leftSide);
            }
        }
        for (int k = 0; k < rawLanes.size(); k++) {
            int turn = TurnType::getPrimaryTurn(rawLanes[k]);
            int sturn = TurnType::getSecondaryTurn(rawLanes[k]);
            int tturn = TurnType::getTertiaryTurn(rawLanes[k]);
            
            bool active = false;
            // some turns go through many segments (to turn right or left)
            // so on one first segment the lane could be available and may be only 1 possible
            // all undesired lanes will be disabled through the 2nd pass
            if((TurnType::isRightTurn(sturn) && possiblyRightTurn) ||
               (TurnType::isLeftTurn(sturn) && possiblyLeftTurn)) {
                // we can't predict here whether it will be a left turn or straight on,
                // it could be done during 2nd pass
                TurnType::setSecondaryToPrimary(rawLanes, k);
                active = true;
            } else if((TurnType::isRightTurn(tturn) && possiblyRightTurn) ||
                      (TurnType::isLeftTurn(tturn) && possiblyLeftTurn)) {
                TurnType::setTertiaryToPrimary(rawLanes, k);
                active = true;
            } else if((TurnType::isRightTurn(turn) && possiblyRightTurn) ||
                      (TurnType::isLeftTurn(turn) && possiblyLeftTurn)) {
                active = true;
            } else if (turn == tp) {
                active = true;
            }
            if (active) {
                rawLanes[k] |= 1;
            }
        }
    }
    t->setSkipToSpeak(!rs.speak);
    t->setLanes(rawLanes);
    t->setPossibleLeftTurn(possiblyLeftTurn);
    t->setPossibleRightTurn(possiblyRightTurn);
    return t;
}

SHARED_PTR<TurnType> attachKeepLeftInfoAndLanes(bool leftSide, SHARED_PTR<RouteSegmentResult>& prevSegm, SHARED_PTR<RouteSegmentResult>& currentSegm) {
    auto attachedRoutes = currentSegm->getAttachedRoutes(currentSegm->getStartPointIndex());
    if (attachedRoutes.empty()) {
        return nullptr;
    }
    // keep left/right
    RoadSplitStructure rs = calculateRoadSplitStructure(prevSegm, currentSegm, attachedRoutes);
    if(rs.roadsOnLeft  + rs.roadsOnRight == 0) {
        return nullptr;
    }
    
    // turn lanes exist
    string turnLanes = getTurnLanesString(prevSegm);
    if (!turnLanes.empty()) {
        return createKeepLeftRightTurnBasedOnTurnTypes(rs, prevSegm, currentSegm, turnLanes, leftSide);
    }
    
    // turn lanes don't exist
    if (rs.keepLeft || rs.keepRight) {
        return createSimpleKeepLeftRightTurn(leftSide, prevSegm, currentSegm, rs);
    }
    return nullptr;
}

SHARED_PTR<TurnType> getTurnInfo(vector<SHARED_PTR<RouteSegmentResult> >& result, int i, bool leftSide) {
    if (i == 0) {
        return TurnType::ptrValueOf(TurnType::C, false);
    }
    auto prev = result[i - 1];
    if (prev->object->roundabout()) {
        // already analyzed!
        return nullptr;
    }
    auto rr = result[i];
    if (rr->object->roundabout()) {
        return processRoundaboutTurn(result, i, leftSide, prev, rr);
    }
    SHARED_PTR<TurnType> t = nullptr;
	if (prev != nullptr)
	{
		// add description about turn
		// avoid small zigzags is covered at (search for "zigzags")
		float bearingDist = DIST_BEARING_DETECT;
		// could be || noAttachedRoads, boolean noAttachedRoads = rr.getAttachedRoutes(rr.getStartPointIndex()).size() == 0;
		if (UNMATCHED_HIGHWAY_TYPE == rr->object->getHighway()) {
			bearingDist = DIST_BEARING_DETECT_UNMATCHED;
		}
		double mpi = degreesDiff(prev->getBearingEnd(prev->getEndPointIndex(), bearingDist), rr->getBearingBegin(rr->getStartPointIndex(), bearingDist));
		if (mpi >= TURN_DEGREE_MIN) {
			if (mpi < TURN_DEGREE_MIN) {
				// Slight turn detection here causes many false positives where drivers would expect a "normal" TL. Best use limit-angle=TURN_DEGREE_MIN, this reduces TSL to the turn-lanes cases.
				t = TurnType::ptrValueOf(TurnType::TSLL, leftSide);
			} else if (mpi < 120) {
				t = TurnType::ptrValueOf(TurnType::TL, leftSide);
			} else if (mpi < 150 || leftSide) {
				t = TurnType::ptrValueOf(TurnType::TSHL, leftSide);
			} else {
				t = TurnType::ptrValueOf(TurnType::TU, leftSide);
			}
			const auto& lanes = getTurnLanesInfo(prev, t->getValue());
			t->setLanes(lanes);
		} else if (mpi < -TURN_DEGREE_MIN) {
			if (mpi > -TURN_DEGREE_MIN) {
				t = TurnType::ptrValueOf(TurnType::TSLR, leftSide);
			} else if (mpi > -120) {
				t = TurnType::ptrValueOf(TurnType::TR, leftSide);
			} else if (mpi > -150 || !leftSide) {
				t = TurnType::ptrValueOf(TurnType::TSHR, leftSide);
			} else {
				t = TurnType::ptrValueOf(TurnType::TRU, leftSide);
			}
			const auto& lanes = getTurnLanesInfo(prev, t->getValue());
			t->setLanes(lanes);
		} else {
			t = attachKeepLeftInfoAndLanes(leftSide, prev, rr);
		}
		if (t) {
			t->setTurnAngle((float) -mpi);
		}
	}

    return t;
}

bool mergeTurnLanes(bool leftSide, SHARED_PTR<RouteSegmentResult>& currentSegment, SHARED_PTR<RouteSegmentResult>& nextSegment) {
    MergeTurnLaneTurn active(currentSegment);
    MergeTurnLaneTurn target(nextSegment);
    if (active.activeLen < 2) {
        return false;
    }
    if (target.activeStartIndex == -1) {
        return false;
    }
    bool changed = false;
    if (target.isActiveTurnMostLeft()) {
        // let only the most left lanes be enabled
        if (target.activeLen < active.activeLen) {
            active.activeEndIndex -= (active.activeLen - target.activeLen);
            changed = true;
        }
    } else if (target.isActiveTurnMostRight()) {
        // next turn is right
        // let only the most right lanes be enabled
        if (target.activeLen < active.activeLen ) {
            active.activeStartIndex += (active.activeLen - target.activeLen);
            changed = true;
        }
    } else {
        // next turn is get through (take out the left and the right turn)
        if (target.activeLen < active.activeLen) {
            if (target.originalLanes.size() == active.activeLen) {
                active.activeEndIndex = active.activeStartIndex + target.activeEndIndex;
                active.activeStartIndex = active.activeStartIndex + target.activeStartIndex;
                changed = true;
            } else {
                int straightActiveLen = 0;
                int straightActiveBegin = -1;
                for (int i = active.activeStartIndex; i <= active.activeEndIndex; i++) {
                    if (TurnType::hasAnyTurnLane(active.originalLanes[i], TurnType::C)) {
                        straightActiveLen++;
                        if(straightActiveBegin == -1) {
                            straightActiveBegin = i;
                        }
                    }
                }
                if (straightActiveBegin != -1 && straightActiveLen <= target.activeLen) {
                    active.activeStartIndex = straightActiveBegin;
                    active.activeEndIndex = straightActiveBegin + straightActiveLen - 1;
                    changed = true;
                } else {
                    // cause the next-turn goes forward exclude left most and right most lane
                    if (active.activeStartIndex == 0) {
                        active.activeStartIndex++;
                        active.activeLen--;
                    }
                    if (active.activeEndIndex == active.originalLanes.size() - 1) {
                        active.activeEndIndex--;
                        active.activeLen--;
                    }
                    float ratio = (active.activeLen - target.activeLen) / 2.f;
                    if (ratio > 0) {
                        active.activeEndIndex = (int) ceil(active.activeEndIndex - ratio);
                        active.activeStartIndex = (int) floor(active.activeStartIndex + ratio);
                    }
                    changed = true;
                }
            }
        }
    }
    if (!changed) {
        return false;
    }
    
    // set the allowed lane bit
    for (int i = 0; i < active.disabledLanes.size(); i++) {
        if (i >= active.activeStartIndex && i <= active.activeEndIndex && 
            active.originalLanes[i] % 2 == 1) {
            active.disabledLanes[i] |= 1;
        }
    }
    const auto& currentTurn = currentSegment->turnType;
    currentTurn->setLanes(active.disabledLanes);
    return true;
}

void inferCommonActiveLane(const SHARED_PTR<TurnType>& currentTurn, const SHARED_PTR<TurnType>& nextTurn) {
    auto& lanes = currentTurn->getLanes();
    set<int> turnSet;
    for (int i = 0; i < lanes.size(); i++) {
        if (lanes[i] % 2 == 1 ) {
            int singleTurn = TurnType::getPrimaryTurn(lanes[i]);
            turnSet.insert(singleTurn);
            if (TurnType::getSecondaryTurn(lanes[i]) != 0) {
                turnSet.insert(TurnType::getSecondaryTurn(lanes[i]));
            }
            if (TurnType::getTertiaryTurn(lanes[i]) != 0) {
                turnSet.insert(TurnType::getTertiaryTurn(lanes[i]));
            }
        }
    }
    int singleTurn = 0;
    if (turnSet.size() == 1) {
        singleTurn = *turnSet.begin();
    } else if (currentTurn->goAhead() && turnSet.find(nextTurn->getValue()) != turnSet.end()) {
        if (currentTurn->isPossibleLeftTurn() && TurnType::isLeftTurn(nextTurn->getValue())) {
            singleTurn = nextTurn->getValue();
        } else if(currentTurn->isPossibleLeftTurn() && TurnType::isLeftTurn(nextTurn->getActiveCommonLaneTurn())) {
            singleTurn = nextTurn->getActiveCommonLaneTurn();
        } else if (currentTurn->isPossibleRightTurn() && TurnType::isRightTurn(nextTurn->getValue())) {
            singleTurn = nextTurn->getValue();
        } else if (currentTurn->isPossibleRightTurn() && TurnType::isRightTurn(nextTurn->getActiveCommonLaneTurn())) {
            singleTurn = nextTurn->getActiveCommonLaneTurn();
        }
    }
    if (singleTurn == 0) {
        singleTurn = currentTurn->getValue();
        if (singleTurn == TurnType::KL || singleTurn == TurnType::KR) {
            return;
        }
    }
    for (int i = 0; i < lanes.size(); i++) {
        if (lanes[i] % 2 == 1 && TurnType::getPrimaryTurn(lanes[i]) != singleTurn) {
            if (TurnType::getSecondaryTurn(lanes[i]) == singleTurn) {
                TurnType::setSecondaryTurn(lanes, i, TurnType::getPrimaryTurn(lanes[i]));
                TurnType::setPrimaryTurn(lanes, i, singleTurn);
            } else if (TurnType::getTertiaryTurn(lanes[i]) == singleTurn) {
                TurnType::setTertiaryTurn(lanes, i, TurnType::getPrimaryTurn(lanes[i]));
                TurnType::setPrimaryTurn(lanes, i, singleTurn);
            } else {
                // disable lane
                lanes[i] = lanes[i] - 1;
            }
        }
    }
}

void inferActiveTurnLanesFromTurn(const SHARED_PTR<TurnType>& tt, int type) {
    bool found = false;
    auto& lanes = tt->getLanes();
    if (tt->getValue() == type && !lanes.empty()) {
        for (int it = 0; it < lanes.size(); it++) {
            int turn = lanes[it];
            if (TurnType::getPrimaryTurn(turn) == type ||
                TurnType::getSecondaryTurn(turn) == type ||
                TurnType::getTertiaryTurn(turn) == type) {
                found = true;
                break;
            }
        }
    }
    if (found) {
        for (int it = 0; it < lanes.size(); it++) {
            int turn = lanes[it];
            if (TurnType::getPrimaryTurn(turn) != type) {
                if (TurnType::getSecondaryTurn(turn) == type) {
                    int st = TurnType::getSecondaryTurn(turn);
                    TurnType::setSecondaryTurn(lanes, it, TurnType::getPrimaryTurn(turn));
                    TurnType::setPrimaryTurn(lanes, it, st);
                } else if (TurnType::getTertiaryTurn(turn) == type) {
                    int st = TurnType::getTertiaryTurn(turn);
                    TurnType::setTertiaryTurn(lanes, it, TurnType::getPrimaryTurn(turn));
                    TurnType::setPrimaryTurn(lanes, it, st);
                } else {
                    lanes[it] = turn & (~1);
                }
            }
        }
    }
}

bool sortArray(int o1, int o2) {
    return TurnType::orderFromLeftToRight(o1) < TurnType::orderFromLeftToRight(o2);
}

vector<int> getPossibleTurns(vector<int>& oLanes, bool onlyPrimary, bool uniqueFromActive) {
    set<int> possibleTurns;
    set<int> upossibleTurns;
    for (int i = 0; i < oLanes.size(); i++) {
        // Nothing is in the list to compare to, so add the first elements
        upossibleTurns.clear();
        upossibleTurns.insert(TurnType::getPrimaryTurn(oLanes[i]));
        if (!onlyPrimary && TurnType::getSecondaryTurn(oLanes[i]) != 0) {
            upossibleTurns.insert(TurnType::getSecondaryTurn(oLanes[i]));
        }
        if (!onlyPrimary && TurnType::getTertiaryTurn(oLanes[i]) != 0) {
            upossibleTurns.insert(TurnType::getTertiaryTurn(oLanes[i]));
        }
        if (!uniqueFromActive) {
            possibleTurns.insert(upossibleTurns.begin(), upossibleTurns.end());
            //                if (!possibleTurns.isEmpty()) {
            //                    possibleTurns.retainAll(upossibleTurns);
            //                    if(possibleTurns.isEmpty()) {
            //                        break;
            //                    }
            //                } else {
            //                    possibleTurns.addAll(upossibleTurns);
            //                }
        } else if ((oLanes[i] & 1) == 1) {
            if (possibleTurns.size() > 0) {
                auto it = possibleTurns.begin();
                while(it != possibleTurns.end()) {
                    auto current = it++;
                    if (upossibleTurns.find(*current) == upossibleTurns.end())
                        possibleTurns.erase(current);
                }
                if(possibleTurns.size() == 0) {
                    break;
                }
            } else {
                possibleTurns.insert(upossibleTurns.begin(), upossibleTurns.end());
            }
        }
    }
    // Remove all turns from lanes not selected...because those aren't it
    if (uniqueFromActive) {
        for (int i = 0; i < oLanes.size(); i++) {
            if ((oLanes[i] & 1) == 0) {
                possibleTurns.erase(TurnType::getPrimaryTurn(oLanes[i]));
                if (TurnType::getSecondaryTurn(oLanes[i]) != 0) {
                    possibleTurns.erase(TurnType::getSecondaryTurn(oLanes[i]));
                }
                if (TurnType::getTertiaryTurn(oLanes[i]) != 0) {
                    possibleTurns.erase(TurnType::getTertiaryTurn(oLanes[i]));
                }
            }
        }
    }
    vector<int> array(possibleTurns.begin(), possibleTurns.end());
    sort(array.begin(), array.end(), sortArray);
    return array;
}

vector<int> getPossibleTurnsFromActiveLanes(vector<int>& oLanes, bool onlyPrimary)
{
    return getPossibleTurns(oLanes, onlyPrimary, true);
}

void determineTurnsToMerge(bool leftside, vector<SHARED_PTR<RouteSegmentResult> >& result) {
    SHARED_PTR<RouteSegmentResult> nextSegment = nullptr;
    double dist = 0;
    for (int i = (int)result.size() - 1; i >= 0; i--) {
        auto currentSegment = result[i];
        const auto& currentTurn = currentSegment->turnType;
        dist += currentSegment->distance;
        if (!currentTurn || currentTurn->getLanes().empty()) {
            // skip
        } else {
            bool merged = false;
            if (nextSegment) {
                string hw = currentSegment->object->getHighway();
                double mergeDistance = 200;
                if (startsWith(hw, "trunk") || startsWith(hw, "motorway")) {
                    mergeDistance = 400;
                }
                if (dist < mergeDistance) {
                    mergeTurnLanes(leftside, currentSegment, nextSegment);
                    const auto& turnType = currentSegment->turnType;
                    const auto possibleTurn = getPossibleTurnsFromActiveLanes(turnType->getLanes(), true);
                    if (possibleTurn.size() == 1) {
                        auto tt = TurnType::ptrValueOf(possibleTurn[0], currentSegment->turnType->isLeftSide());
                        tt->setLanes(turnType->getLanes());
                        tt->setSkipToSpeak(turnType->isSkipToSpeak());
                        currentSegment->turnType = tt;
                    }
                    inferCommonActiveLane(currentSegment->turnType, nextSegment->turnType);
                    merged = true;
                }
            }
            if (!merged) {
                const auto& tt = currentSegment->turnType;
                inferActiveTurnLanesFromTurn(tt, tt->getValue());
            }
            nextSegment = currentSegment;
            dist = 0;
        }
    }
}

string getStreetName(vector<SHARED_PTR<RouteSegmentResult> >& result, int i, bool dir) {
    string nm = result[i]->object->getName();
    if (nm.empty()) {
        if (!dir) {
            if (i > 0) {
                nm = result[i - 1]->object->getName();
            }
        } else {
            if(i < result.size() - 1) {
                nm = result[i + 1]->object->getName();
            }
        }
    }
    return nm;
}

SHARED_PTR<TurnType> justifyUTurn(bool leftside, vector<SHARED_PTR<RouteSegmentResult> >& result, int i, const SHARED_PTR<TurnType>& t) {
    bool tl = TurnType::isLeftTurnNoUTurn(t->getValue());
    bool tr = TurnType::isRightTurnNoUTurn(t->getValue());
    if (tl || tr) {
        const auto& tnext = result[i + 1]->turnType;
        if (tnext && result[i]->distance < 50) {
            bool ut = true;
            if (i > 0) {
                double uTurn = degreesDiff(result[i - 1]->getBearingEnd(), result[i + 1]->getBearingBegin());
                if (abs(uTurn) < 120) {
                    ut = false;
                }
            }
            //				String highway = result->get(i).getObject().getHighway();
            //				if(highway == null || highway.endsWith("track") || highway.endsWith("services") || highway.endsWith("service")
            //						|| highway.endsWith("path")) {
            //					ut = false;
            //				}
            if (result[i - 1]->object->getOneway() == 0 || result[i + 1]->object->getOneway() == 0) {
                ut = false;
            }
            if (getStreetName(result, i - 1, false) != getStreetName(result, i + 1, true)) {
                ut = false;
            }
            if (ut) {
                tnext->setSkipToSpeak(true);
                if (tl && TurnType::isLeftTurnNoUTurn(tnext->getValue())) {
                    auto tt = TurnType::ptrValueOf(TurnType::TU, false);
                    tt->setLanes(t->getLanes());
                    return tt;
                } else if (tr && TurnType::isRightTurnNoUTurn(tnext->getValue())) {
                    auto tt = TurnType::ptrValueOf(TurnType::TU, true);
                    tt->setLanes(t->getLanes());
                    return tt;
                }
            }
        }
    }
    return nullptr;
}

void justifyUTurns(bool leftSide, vector<SHARED_PTR<RouteSegmentResult> >& result) {
    int next;
    for (int i = 1; i < ((int) result.size() - 1); i = next) {
        next = i + 1;
        const auto& t = result[i]->turnType;
        // justify turn
        if (t) {
            const auto& jt = justifyUTurn(leftSide, result, i, t);
            if (jt) {
                result[i]->turnType = jt;
                next = i + 2;
            }
        }
    }
}

void addTurnInfoDescriptions(vector<SHARED_PTR<RouteSegmentResult> >& result) {
    int prevSegment = -1;
    float dist = 0;
    char distStr[32];
    for (int i = 0; i <= result.size(); i++) {
        if (i == result.size() || result[i]->turnType) {
            if (prevSegment >= 0) {
                string turn = result[prevSegment]->turnType->toString();
                sprintf(distStr, "%.2f", dist);
                result[prevSegment]->description = turn + " and go " + distStr + " meters";
                if (result[prevSegment]->turnType->isSkipToSpeak()) {
                    result[prevSegment]->description = "-*" + result[prevSegment]->description;
                }
            }
            prevSegment = i;
            dist = 0;
        }
        if (i < result.size()) {
            dist += result[i]->distance;
        }
    }
}

float calcRoutingTime(float parentRoutingTime, const SHARED_PTR<RouteSegment>& finalSegment, const SHARED_PTR<RouteSegment>& segment, SHARED_PTR<RouteSegmentResult>& res) {
    if (segment != finalSegment) {
        if (parentRoutingTime != -1) {
            res->routingTime = parentRoutingTime - segment->distanceFromStart;
        }
        parentRoutingTime = segment->distanceFromStart;
    }
    return parentRoutingTime;
}

bool combineTwoSegmentResult(SHARED_PTR<RouteSegmentResult>& toAdd, SHARED_PTR<RouteSegmentResult>& previous, bool reverse) {
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

void addRouteSegmentToResult(RoutingContext* ctx, vector<SHARED_PTR<RouteSegmentResult> >& result, SHARED_PTR<RouteSegmentResult>& res, bool reverse) {
    if (res->getStartPointIndex() != res->getEndPointIndex()) {
        if (result.size() > 0) {
            auto last = result.back();
            if (last->object->id == res->object->id && ctx->calculationMode != RouteCalculationMode::BASE) {
                if (combineTwoSegmentResult(res, last, reverse)) {
                    return;
                }
            }
        }
        result.push_back(res);
    }
}

vector<SHARED_PTR<RouteSegmentResult> > convertFinalSegmentToResults(RoutingContext* ctx, const SHARED_PTR<FinalRouteSegment>& finalSegment) {
    vector<SHARED_PTR<RouteSegmentResult> > result;
    if (finalSegment) {
        if (ctx->progress)
            ctx->progress->routingCalculatedTime += finalSegment->distanceFromStart;

        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Routing calculated time distance %f", finalSegment->distanceFromStart);
        // Get results from opposite direction roads
        auto segment = finalSegment->reverseWaySearch ? finalSegment : finalSegment->opposite->parentRoute.lock();
        int parentSegmentStart = finalSegment->reverseWaySearch ? finalSegment->opposite->getSegmentStart() :
        finalSegment->opposite->parentSegmentEnd;
        float parentRoutingTime = -1;
        while (segment) {
            auto res = std::make_shared<RouteSegmentResult>(segment->road, parentSegmentStart, segment->getSegmentStart());
            parentRoutingTime = calcRoutingTime(parentRoutingTime, finalSegment, segment, res);
            parentSegmentStart = segment->parentSegmentEnd;
            segment = segment->parentRoute.lock();
            addRouteSegmentToResult(ctx, result, res, false);
        }
        // reverse it just to attach good direction roads
        std::reverse(result.begin(), result.end());
        
        segment = finalSegment->reverseWaySearch ? finalSegment->opposite->parentRoute.lock() : finalSegment;
        int parentSegmentEnd = finalSegment->reverseWaySearch ? finalSegment->opposite->parentSegmentEnd : finalSegment->opposite->getSegmentStart();
        parentRoutingTime = -1;
        while (segment) {
            auto res = std::make_shared<RouteSegmentResult>(segment->road, segment->getSegmentStart(), parentSegmentEnd);
            parentRoutingTime = calcRoutingTime(parentRoutingTime, finalSegment, segment, res);
            parentSegmentEnd = segment->parentSegmentEnd;
            segment = segment->parentRoute.lock();
            // happens in smart recalculation
            addRouteSegmentToResult(ctx, result, res, true);
        }
        std::reverse(result.begin(), result.end());
        // checkTotalRoutingTime(result);
    }
    return result;
}

void printAdditionalPointInfo(SHARED_PTR<RouteSegmentResult>& res) {
    bool plus = res->getStartPointIndex() < res->getEndPointIndex();
    for(int k = res->getStartPointIndex(); k != res->getEndPointIndex(); ) {
        if (res->object->pointTypes.size() > k || res->object->pointNameTypes.size() > k) {
            string bld;
            bld.append("<point ").append(std::to_string(k));
            if (res->object->pointTypes.size() > k) {
                auto& tp = res->object->pointTypes[k];
                for (int t = 0; t < tp.size(); t++) {
                    auto& rr = res->object->region->quickGetEncodingRule(tp[t]);
                    bld.append(" ").append(rr.getTag()).append("=\"").append(rr.getValue()).append("\"");
                }
            }
            if (res->object->pointNameTypes.size() > k && res->object->pointNames.size() > k) {
                auto& pointNames = res->object->pointNames[k];
                auto& pointNameTypes = res->object->pointNameTypes[k];
                for (int t = 0; t < pointNameTypes.size(); t++) {
                    auto& rr = res->object->region->quickGetEncodingRule(pointNameTypes[t]);
                    bld.append(" ").append(rr.getTag()).append("=\"").append(pointNames[t]).append("\"");
                }
            }
            bld.append("/>");
            //OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "\t%s", bld.c_str());
        }
        if (plus) {
            k++;
        } else {
            k--;
        }
    }
}

const static bool PRINT_TO_CONSOLE_ROUTE_INFORMATION_TO_TEST = false;

void printResults(RoutingContext* ctx, int startX, int startY, int endX, int endY, vector<SHARED_PTR<RouteSegmentResult> >& result) {
    float completeTime = 0;
    float completeDistance = 0;
    for (auto r : result) {
        completeTime += r->segmentTime;
        completeDistance += r->distance;
    }
    
    double startLat = get31LatitudeY(startY);
    double startLon = get31LongitudeX(startX);
    double endLat = get31LatitudeY(endY);
    double endLon = get31LongitudeX(endX);
    if (ctx->progress)
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "ROUTE : <test regions=\"\" description=\"\" best_percent=\"\" vehicle=\"%s\" \n    start_x=\"%f\" start_y=\"%f\" target_x=\"%f\" target_y=\"%f\" loadedTiles = \"%d\" visitedSegments = \"%d\" complete_distance = \"%f\" complete_time = \"%f\" routing_time = \"%f\">",
                          ctx->config->routerName.c_str(), startLat, startLon, endLat, endLon,
                          ctx->progress->loadedTiles, ctx->progress->visitedSegments, completeDistance, completeTime, ctx->progress->routingCalculatedTime);
    
    if (PRINT_TO_CONSOLE_ROUTE_INFORMATION_TO_TEST) {
        
        string xmlStr;
        xmlStr.append("\n<trk>\n");
        xmlStr.append("<trkseg>\n");
        
        double lastHeight = -180;
        for (auto& res : result) {
            string trkSeg;
            string name = res->object->getName();
            string lang = "";
            string ref = res->object->getRef(lang, false, res->isForwardDirection());
            if (!ref.empty()) {
                name += " (" + ref + ") ";
            }
            string additional;
            additional.append("time = \"").append(std::to_string(res->segmentTime)).append("\" ");
            additional.append("rtime = \"").append(std::to_string(res->routingTime)).append("\" ");
            additional.append("name = \"").append(name).append("\" ");
            //				float ms = res->getSegmentSpeed();
            float ms = res->object->getMaximumSpeed(res->isForwardDirection());
            if (ms > 0) {
                additional.append("maxspeed = \"").append(std::to_string(ms * 3.6f)).append("\" ").append(res->object->getHighway()).append(" ");
            }
            additional.append("distance = \"").append(std::to_string(res->distance)).append("\" ");
            if (res->turnType) {
                additional.append("turn = \"").append(res->turnType->toString()).append("\" ");
                additional.append("turn_angle = \"").append(std::to_string(res->turnType->getTurnAngle())).append("\" ");
                if (!res->turnType->getLanes().empty()) {
                    additional.append("lanes = \"");
                    additional.append("[");
                    string lanes;
                    for (auto l : res->turnType->getLanes()) {
                        if (!lanes.empty()) {
                            lanes.append(", ");
                        }
                        lanes.append(std::to_string(l));
                    }
                    additional.append(lanes).append("]").append("\" ");;
                }
            }
            additional.append("start_bearing = \"").append(std::to_string(res->getBearingBegin())).append("\" ");
            additional.append("end_bearing = \"").append(std::to_string(res->getBearingEnd())).append("\" ");
            additional.append("height = \"");
            additional.append("[");
            string hs;
            const auto& heights = res->getHeightValues();
            for (auto h : heights) {
                if (!hs.empty()) {
                    hs.append(", ");
                }
                hs.append(std::to_string(h));
            }
            additional.append(hs).append("]").append("\" ");
            additional.append("description = \"").append(res->description).append("\" ");
            
            //OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "\t<segment id=\"%d\" oid=\"%lld\" start=\"%d\" end=\"%d\" %s/>", (res->object->getId() >> SHIFT_ID), res->object->getId(), res->getStartPointIndex(), res->getEndPointIndex(), additional.c_str());
            int inc = res->getStartPointIndex() < res->getEndPointIndex() ? 1 : -1;
            int indexnext = res->getStartPointIndex();
            int prevX = 0;
            int prevY = 0;
            for (int index = res->getStartPointIndex() ; index != res->getEndPointIndex(); ) {
                index = indexnext;
                indexnext += inc;
                
                int x = res->object->pointsX[index];
                int y = res->object->pointsY[index];
                double lat = get31LatitudeY(y);
                double lon = get31LongitudeX(x);
                
                trkSeg.append("<trkpt");
                trkSeg.append(" lat=\"").append(std::to_string(lat)).append("\"");
                trkSeg.append(" lon=\"").append(std::to_string(lon)).append("\"");
                trkSeg.append(">\n");
                auto& vls = res->object->heightDistanceArray;
                double dist = prevX == 0 ? 0 : measuredDist31(x, y, prevX, prevY);
                if (index * 2 + 1 < vls.size()) {
                    auto h = vls[2 * index + 1];
                    trkSeg.append("<ele>");
                    trkSeg.append(std::to_string(h));
                    trkSeg.append("</ele>\n");
                    if (lastHeight != -180 && dist > 0) {
                        trkSeg.append("<cmt>");
                        trkSeg.append(std::to_string((float) ((h - lastHeight)/ dist * 100))).append("%% degree ").append(std::to_string((float) atan(((h - lastHeight) / dist)) / M_PI * 180)).append(" asc ").append(std::to_string((float) (h - lastHeight))).append(" dist ").append(std::to_string((float) dist));
                        trkSeg.append("</cmt>\n");
                        trkSeg.append("<slope>");
                        trkSeg.append(std::to_string((h - lastHeight)/ dist * 100));
                        trkSeg.append("</slope>\n");
                    }
                    trkSeg.append("<desc>");
                    trkSeg.append(std::to_string(res->object->getId() >> SHIFT_ID )).append(" ").append(std::to_string(index));
                    trkSeg.append("</desc>\n");
                    lastHeight = h;
                } else if (lastHeight != -180) {
                    //								serializer.startTag("","ele");
                    //								serializer.text(lastHeight +"");
                    //								serializer.endTag("","ele");
                }
                trkSeg.append("</trkpt>\n");
                prevX = x;
                prevY = y;
            }
            xmlStr.append(trkSeg);
            printAdditionalPointInfo(res);
        }
        xmlStr.append("</trkseg>\n");
        xmlStr.append("</trk>\n");
        //OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, xmlStr.c_str());
    }
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "</test>");
}

bool segmentLineBelongsToPolygon(CombineAreaRoutePoint& p, CombineAreaRoutePoint& n,
        vector<CombineAreaRoutePoint>& originalWay) {
    int intersections = 0;
    int mx = p.x31 / 2 + n.x31 / 2;
    int my = p.y31 / 2 + n.y31 / 2;
    for(int i = 1; i < originalWay.size(); i++) {
        CombineAreaRoutePoint& p2 = originalWay[i - 1];
        CombineAreaRoutePoint& n2 = originalWay[i];
        if(p.originalIndex != i && p.originalIndex != i - 1) {
            if(n.originalIndex != i && n.originalIndex != i - 1) {
                if(linesIntersect(p.x31, p.y31, n.x31, n.y31, p2.x31, p2.y31, n2.x31, n2.y31)) {
                    return false;
                }
            }
        }
        int fx = ray_intersect_x(p2.x31, p2.y31, n2.x31, n2.y31, my);
        if (INT_MIN != fx && mx >= fx) {
            intersections++;
        }
    }
    return intersections % 2 == 1;
}

void simplifyAreaRouteWay(vector<CombineAreaRoutePoint>& routeWay, vector<CombineAreaRoutePoint>& originalWay) {
    bool changed = true;
    while (changed) {
        changed = false;
        uint64_t connectStart = -1;
        uint64_t connectLen = 0;
        double dist = 0;
        uint64_t length = routeWay.size() - 1;
        while (length > 0 && connectLen == 0) {
            for (int i = 0; i < routeWay.size() - length; i++) {
                CombineAreaRoutePoint& p = routeWay[i];
                CombineAreaRoutePoint& n = routeWay[i + length];
                if (segmentLineBelongsToPolygon(p, n, originalWay)) {
                    double ndist = squareRootDist31(p.x31, p.y31, n.x31, n.y31);
                    if (ndist > dist) {
                        ndist = dist;
                        connectStart = i;
                        connectLen = length;
                    }
                }
            }
            length--;
        }
        while (connectLen > 1) {
            routeWay.erase(routeWay.begin() + connectStart + 1);
            connectLen--;
            changed = true;
        }
    }
}

void combineWayPointsForAreaRouting(RoutingContext* ctx, vector<SHARED_PTR<RouteSegmentResult> >& result) {
    for(int i = 0; i < result.size(); i++) {
        const auto& rsr = result[i];
        auto& obj = rsr->object;
        bool area = false;
        if(obj->pointsX[0] == obj->pointsX[obj->getPointsLength() - 1] &&
                obj->pointsY[0] == obj->pointsY[obj->getPointsLength() - 1]) {
            area = true;
        }
        if(!area || !ctx->config->router->isArea(obj)) {
            continue;
        }
        vector<CombineAreaRoutePoint> originalWay;
        vector<CombineAreaRoutePoint> routeWay;
        for(int j = 0;  j < obj->getPointsLength(); j++) {
            CombineAreaRoutePoint pnt;
            pnt.x31 = obj->pointsX[j];
            pnt.y31 = obj->pointsY[j];
            pnt.originalIndex = j;

            originalWay.push_back(pnt);
            if(j >= rsr->getStartPointIndex() && j <= rsr->getEndPointIndex()) {
                routeWay.push_back(pnt);
            } else if(j <= rsr->getStartPointIndex() && j >= rsr->getEndPointIndex()) {
                routeWay.insert(routeWay.begin(), pnt);
            }
        }
        uint64_t originalSize = routeWay.size();
        simplifyAreaRouteWay(routeWay, originalWay);
        uint64_t newsize = routeWay.size();
        if (routeWay.size() != originalSize) {
            SHARED_PTR<RouteDataObject> nobj = std::make_shared<RouteDataObject>(obj);
            nobj->pointsX.clear();
            nobj->pointsY.clear();
            nobj->pointsX.resize(newsize);
            nobj->pointsY.resize(newsize);
            for (int k = 0; k < newsize; k++) {
                nobj->pointsX[k] = routeWay[k].x31;
                nobj->pointsY[k] = routeWay[k].y31;
            }
            // in future point names might be used
            nobj->restrictions.clear();
//            nobj->restrictionsVia.clear();
            nobj->pointTypes.clear();
            nobj->pointNames.clear();
            nobj->pointNameTypes.clear();
            auto nrsr = std::make_shared<RouteSegmentResult>(nobj, 0, newsize - 1);
            result[i] = nrsr;
        }
    }
}

void prepareTurnResults(RoutingContext* ctx, vector<SHARED_PTR<RouteSegmentResult> >& result) {
    for (int i = 0; i < result.size(); i ++) {
        const auto& turnType = getTurnInfo(result, i, ctx->leftSideNavigation);
        result[i]->turnType = turnType;
    }
    
    determineTurnsToMerge(ctx->leftSideNavigation, result);
    ignorePrecedingStraightsOnSameIntersection(ctx->leftSideNavigation, result);
    justifyUTurns(ctx->leftSideNavigation, result);
    addTurnInfoDescriptions(result);
}

vector<SHARED_PTR<RouteSegmentResult> > prepareResult(RoutingContext* ctx, vector<SHARED_PTR<RouteSegmentResult> >& result) {
    combineWayPointsForAreaRouting(ctx, result);
    validateAllPointsConnected(result);
    splitRoadsAndAttachRoadSegments(ctx, result);
	for (int i = 0; i < result.size(); i++) {
		filterMinorStops(result[i]);
	}
    calculateTimeSpeed(ctx, result);
	prepareTurnResults(ctx, result);
    return result;
}

vector<SHARED_PTR<RouteSegmentResult> > prepareResult(RoutingContext* ctx, const SHARED_PTR<FinalRouteSegment>& finalSegment) {
    auto result = convertFinalSegmentToResults(ctx, finalSegment);
    prepareResult(ctx, result);
    return result;
}

#endif /*_OSMAND_ROUTE_RESULT_PREPARATION_CPP*/
