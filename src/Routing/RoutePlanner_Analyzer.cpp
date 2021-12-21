#include "RoutePlanner.h"

#include <queue>

#include <OsmAndCore/QtExtensions.h>
#include <QtCore>
#include <QtMath>

#include "ObfReader.h"
#include "Road.h"
#include "Common.h"
#include "Logging.h"
#include "Utilities.h"
#include "TurnInfo.h"
#include "QCachingIterator.h"

OsmAnd::RouteCalculationResult OsmAnd::RoutePlanner::prepareResult(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> finalSegment,
    bool leftSideNavigation)
{
    // Prepare result
    QVector< std::shared_ptr<RouteSegment> > route;
    auto pFinalSegment = dynamic_cast<RoutePlannerContext::RouteCalculationFinalSegment*>(finalSegment.get());

    // Get results from opposite direction roads
    auto segment = pFinalSegment->_reverseWaySearch ? finalSegment : pFinalSegment->opposite->parent;
    auto parentSegmentStart = pFinalSegment->_reverseWaySearch ? pFinalSegment->opposite->pointIndex : pFinalSegment->opposite->parentEndPointIndex;
    int i = 0;
    while (segment)
    {
        std::shared_ptr<RouteSegment> routeSegment(new RouteSegment(segment->road, parentSegmentStart, segment->pointIndex));
        parentSegmentStart = segment->parentEndPointIndex;
        segment = segment->parent;

        addRouteSegmentToRoute(route, routeSegment, false);
        i++;
    }

    // Reverse it just to attach good direction roads
    std::reverse(route.begin(), route.end());
    i = 0;
    segment = pFinalSegment->_reverseWaySearch ? pFinalSegment->opposite->parent : finalSegment;
    auto parentSegmentEnd = pFinalSegment->_reverseWaySearch ? pFinalSegment->opposite->parentEndPointIndex : pFinalSegment->opposite->pointIndex;
    while (segment)
    {
        std::shared_ptr<RouteSegment> routeSegment(new RouteSegment(segment->road, segment->pointIndex, parentSegmentEnd));
        parentSegmentEnd = segment->parentEndPointIndex;
        segment = segment->parent;
        addRouteSegmentToRoute(route, routeSegment, true);
        i++;
    }
    std::reverse(route.begin(), route.end());

    if (!validateAllPointsConnected(route))
        return OsmAnd::RouteCalculationResult("Calculated route has broken paths");
    splitRoadsAndAttachRoadSegments(context, route);
    calculateTimeSpeedInRoute(context, route);

    addTurnInfoToRoute(leftSideNavigation, route);

    printRouteInfo(route);
    OsmAnd::RouteCalculationResult result;
    result.list = route.toList();
    context->owner->_previouslyCalculatedRoute = result.list;
    return result;
}

void OsmAnd::RoutePlanner::printRouteInfo(QVector< std::shared_ptr<RouteSegment> >& route) {
    float completeDist = 0;
    float completeTime = 0;
    for(const auto& segment : constOf(route))
    {
        completeDist += segment->distance;
        completeTime += segment->time;
#ifdef DEBUG_ROUTING
        LogPrintf(LogSeverityLevel::Debug, "Segment : %llu %u %u  time %f speed %f dist %f",
                  segment->road->id, segment->startPointIndex,  segment->endPointIndex,
                  segment->_time, segment->_speed, segment->_distance);
#endif
    }

    LogPrintf(LogSeverityLevel::Debug, "%u segments in route", route.size());
    LogPrintf(LogSeverityLevel::Debug, "Complete road time %f, complete distance %f", completeTime, completeDist);
    LogFlush();
}

void OsmAnd::RoutePlanner::splitRoadsAndAttachRoadSegments( OsmAnd::RoutePlannerContext::CalculationContext* context, QVector< std::shared_ptr<RouteSegment> >& route )
{
    for(auto itSegment = iteratorOf(route); itSegment; ++itSegment)
    {
        auto segment = *itSegment;
        /*TODO:GC
        if (ctx.checkIfMemoryLimitCritical(ctx.config.memoryLimitation)) {
            ctx.unloadUnusedTiles(ctx.config.memoryLimitation);
        }
        */
        //TODO:GC:checkAndInitRouteRegion(context, segment->road);

        const bool isIncrement = segment->startPointIndex < segment->endPointIndex;
        for(auto pointIdx = segment->startPointIndex; pointIdx != segment->endPointIndex; isIncrement ? pointIdx++ : pointIdx--)
        {
            const auto nextIdx = (isIncrement ? pointIdx + 1 : pointIdx - 1);

            if (pointIdx == segment->startPointIndex)
                attachRouteSegments(context, route, itSegment.current, pointIdx, isIncrement);

            if (nextIdx != segment->endPointIndex)
                attachRouteSegments(context, route, itSegment.current, nextIdx, isIncrement);

            if (nextIdx < 0 || nextIdx >= segment->attachedRoutes.size())
                continue;
            if (nextIdx >= 0 && nextIdx < segment->attachedRoutes.size() && nextIdx != segment->endPointIndex && !segment->road->isRoundabout())
            {
                const auto& attachedRoutes = segment->attachedRoutes[qAbs(static_cast<int64_t>(nextIdx) - segment->startPointIndex)];

                auto before = segment->getBearing(nextIdx, !isIncrement);
                auto after = segment->getBearing(nextIdx, isIncrement);
                auto straight = qAbs(Utilities::normalizedAngleDegrees(before + 180.0 - after)) < (double)MinTurnAngle;
                auto isSplit = false;

                // split if needed
                for(const auto& attachedSegment : constOf(attachedRoutes))
                {
                    auto diff = Utilities::normalizedAngleDegrees(before + 180.0 - attachedSegment->getBearingBegin());
                    if (qAbs(diff) <= (double)MinTurnAngle)
                        isSplit = true;
                    else if (!straight && qAbs(diff) < 100.0)
                        isSplit = true;
                }

                if (isSplit)
                {
                    std::shared_ptr<RouteSegment> split(new RouteSegment(segment->road, nextIdx, segment->endPointIndex));
                    //TODO:split.copyPreattachedRoutes(segment, qAbs(nextIdx - segment->startPointIndex));?
                    segment->_endPointIndex = nextIdx;
                    segment->_attachedRoutes.resize(qAbs(static_cast<int64_t>(segment->_endPointIndex) - static_cast<int64_t>(segment->_startPointIndex)) + 1);

                    itSegment.set(route.insert((++itSegment).current, split));
                    itSegment.update(route);
                    
                    // switch current segment to the splited
                    segment = split;
                }
            }
        }
    }
}

void OsmAnd::RoutePlanner::attachRouteSegments(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    const QVector< std::shared_ptr<RouteSegment> >& route,
    const QVector< std::shared_ptr<RouteSegment> >::const_iterator& itSegment,
    uint32_t pointIdx, bool isIncrement)
{
    const auto& segment = *itSegment;
    const auto& nextL = pointIdx < segment->road->points.size() - 1
        ? segment->road->points[pointIdx + 1]
        : PointI();
    const auto& prevL = pointIdx > 0
        ? segment->road->points[pointIdx - 1]
        : PointI();

    // by default make same as this road id
    auto previousRoadId = segment->road->id;
    if (pointIdx == segment->startPointIndex && itSegment != route.cbegin())
    {
        // attach additional roads to represent more information about the route
        const auto& previousResult = *(itSegment - 1);
        previousRoadId = previousResult->road->id;
        if (previousRoadId != segment->road->id)
        {
            std::shared_ptr<RouteSegment> attachedSegment;

            if (previousResult->startPointIndex < previousResult->endPointIndex &&
                previousResult->endPointIndex < previousResult->road->points.size() - 1)
                attachedSegment.reset(new RouteSegment(previousResult->road, previousResult->endPointIndex, previousResult->road->points.size() - 1));
            else if (previousResult->startPointIndex > previousResult->endPointIndex && previousResult->endPointIndex > 0)
                attachedSegment.reset(new RouteSegment(previousResult->road, previousResult->endPointIndex, 0));

            if (attachedSegment)
                segment->_attachedRoutes[qAbs(static_cast<int64_t>(pointIdx) - static_cast<int64_t>(segment->_startPointIndex))].push_back(qMove(attachedSegment));
        }
    }

    // Try to attach all segments except with current id
    const auto& p31 = segment->road->points[pointIdx];
    auto rt = OsmAnd::RoutePlanner::loadRouteCalculationSegment(context->owner, p31.x, p31.y);
    while(rt)
    {
        if (rt->road->id != segment->road->id && rt->road->id != previousRoadId)
        {
            std::shared_ptr<RouteSegment> attachedSegment;
            //TODO:GC:checkAndInitRouteRegion(ctx, rt->road);
            // TODO restrictions can be considered as well
            auto direction = context->owner->profileContext->getDirection(rt->road);
            if ((direction == Model::RoadDirection::TwoWay || direction == Model::RoadDirection::OneWayReverse) && rt->pointIndex < rt->road->points.size() - 1)
            {
                const auto& otherPoint = rt->road->points[rt->pointIndex + 1];
                if (otherPoint != nextL && otherPoint != prevL)
                {
                    // if way contains same segment (nodes) as different way (do not attach it)
                    attachedSegment.reset(new RouteSegment(rt->road, rt->pointIndex, rt->road->points.size() - 1));
                }
            }
            if ((direction == Model::RoadDirection::TwoWay || direction == Model::RoadDirection::OneWayForward) && rt->pointIndex > 0)
            {
                const auto& otherPoint = rt->road->points[rt->pointIndex - 1];
                if (otherPoint != nextL && otherPoint != prevL)
                {
                    // if way contains same segment (nodes) as different way (do not attach it)
                    attachedSegment.reset(new RouteSegment(rt->road, rt->pointIndex, 0));
                }
            }

            if (attachedSegment)
                segment->_attachedRoutes[qAbs(static_cast<int64_t>(pointIdx) - static_cast<int64_t>(segment->_startPointIndex))].push_back(qMove(attachedSegment));
        }

        rt = rt->next;
    }
}

void OsmAnd::RoutePlanner::calculateTimeSpeedInRoute( OsmAnd::RoutePlannerContext::CalculationContext* context, QVector< std::shared_ptr<RouteSegment> >& route )
{
    for(const auto& segment : constOf(route))
    {
        float distOnRoadToPass = 0;
        float speed = context->owner->profileContext->getSpeed(segment->road);
        if (qFuzzyCompare(speed, 0))
            speed = context->owner->profileContext->profile->defaultSpeed;

        const auto isIncrement = segment->startPointIndex < segment->endPointIndex;
        
        float distanceSum = 0;
        for(auto pointIdx = segment->startPointIndex; pointIdx != segment->endPointIndex; isIncrement ? pointIdx++ : pointIdx--)
        {
            const auto& point1 = segment->road->points[pointIdx];
            const auto& point2 = segment->road->points[pointIdx + (isIncrement ? +1 : -1)];
            auto distance = Utilities::distance(
                Utilities::get31LongitudeX(point1.x), Utilities::get31LatitudeY(point1.y),
                Utilities::get31LongitudeX(point2.x), Utilities::get31LatitudeY(point2.y)
            );
            
            distanceSum += distance;
            auto obstacle = context->owner->profileContext->getObstaclesExtraTime(segment->road, pointIdx);
            if (obstacle < 0)
                obstacle = 0;
            distOnRoadToPass += distance / speed + obstacle;
        }

        // last point turn time can be added
        // if (i + 1 < result.size()) { distOnRoadToPass += ctx.getRouter().calculateTurnTime(); }
        segment->_time = distOnRoadToPass;
        segment->_speed = speed;
        segment->_distance = distanceSum;
    }
}
bool OsmAnd::RoutePlanner::validateAllPointsConnected( const QVector< std::shared_ptr<RouteSegment> >& route )
{
    assert(route.size() > 1);

    bool res = true;    
    for(int i = 1, count = route.size(); i < count; i++)
    {
        auto prevSegment = route[i-1];
        auto segment = route[i];

        const auto& point1 = prevSegment->road->points[prevSegment->endPointIndex];
        const auto& point2 = segment->road->points[segment->startPointIndex];
        auto distance = Utilities::distance(
            Utilities::get31LongitudeX(point1.x), Utilities::get31LatitudeY(point1.y),
            Utilities::get31LongitudeX(point2.x), Utilities::get31LatitudeY(point2.y)
        );
        if (distance > 0)
        {
            res = false;

            LogPrintf(LogSeverityLevel::Error, "!! Points are not connected : %llu(%u) -> %llu(%u) %f meters",
                prevSegment->road->id, prevSegment->endPointIndex, segment->road->id, segment->startPointIndex, distance);
        }
    }

    return res;
}

void OsmAnd::RoutePlanner::addRouteSegmentToRoute( QVector< std::shared_ptr<RouteSegment> >& route, const std::shared_ptr<RouteSegment>& segment, bool reverse )
{
    if (segment->startPointIndex == segment->endPointIndex)
        return;

    if (route.size() > 0)
    {
        auto last = route.back();
        if (last->road->id == segment->road->id)
        {
            if (combineTwoSegmentResult(segment, last, reverse))
                return;
        }
    }
    route.push_back(segment);
}

bool OsmAnd::RoutePlanner::combineTwoSegmentResult( const std::shared_ptr<RouteSegment>& toAdd, const std::shared_ptr<RouteSegment>& previous, bool reverse )
{
    auto ld = previous->endPointIndex > previous->startPointIndex;
    auto rd = toAdd->endPointIndex > toAdd->startPointIndex;
    if (rd != ld)
        return false;
     
    if (toAdd->startPointIndex == previous->endPointIndex && !reverse)
    {
        previous->_endPointIndex = toAdd->endPointIndex;
        previous->_attachedRoutes.resize(qAbs(static_cast<int64_t>(previous->_endPointIndex) - static_cast<int64_t>(previous->_startPointIndex)) + 1);
        return true;
    }
    else if (toAdd->endPointIndex == previous->startPointIndex && reverse)
    {
        previous->_startPointIndex = toAdd->startPointIndex;
        previous->_attachedRoutes.resize(qAbs(static_cast<int64_t>(previous->_endPointIndex) - static_cast<int64_t>(previous->_startPointIndex)) + 1);
        return true;
    }

    return false;
}

static const int MAX_SPEAK_PRIORITY = 5;
int highwaySpeakPriority(QString highway) {
    if (highway == "" || highway.endsWith("track") || highway.endsWith("services") || highway.endsWith("service")
            || highway.endsWith("path")) {
        return MAX_SPEAK_PRIORITY;
    }
    if (highway.endsWith("_link")  || highway.endsWith("unclassified") || highway.endsWith("road")
            || highway.endsWith("living_street") || highway.endsWith("residential") )  {
        return 1;
    }
    return 0;
}

bool isMotorway(std::shared_ptr<OsmAnd::RouteSegment> s){
    QString h = s->road->getHighway();
    return "motorway"==h || "motorway_link"==h  ||
            "trunk"==h || "trunk_link"==h;

}


OsmAnd::TurnInfo attachKeepLeftInfoAndLanes(bool leftSide,
                                            std::shared_ptr<OsmAnd::RouteSegment>  prevSegm, std::shared_ptr<OsmAnd::RouteSegment>  currentSegm, OsmAnd::TurnInfo t) {
    // keep left/right
    QVector<int> lanes ;
    bool kl = false;
    bool kr = false;

    QList<std::shared_ptr<OsmAnd::RouteSegment> > attachedRoutes = currentSegm->attachedRoutes[0];
    int ls = prevSegm->road->getLanes();
    if (ls >= 0 && prevSegm->road->getDirection() == OsmAnd::Model::RoadDirection::TwoWay) {
        ls = (ls + 1) / 2;
    }
    int left = 0;
    int right = 0;
    bool speak = false;
    int speakPriority = qMax(highwaySpeakPriority(prevSegm->road->getHighway()), highwaySpeakPriority(currentSegm->road->getHighway()));
    if (!attachedRoutes.isEmpty()) {
        for (std::shared_ptr<OsmAnd::RouteSegment> attached : attachedRoutes) {
            double ex = OsmAnd::Utilities::degreesDiff(attached->getBearingBegin(), currentSegm->getBearingBegin());
            double mpi = abs(OsmAnd::Utilities::degreesDiff(prevSegm->getBearingEnd(), attached->getBearingBegin()));
            int rsSpeakPriority = highwaySpeakPriority(attached->road->getHighway());
            if (rsSpeakPriority != MAX_SPEAK_PRIORITY || speakPriority == MAX_SPEAK_PRIORITY) {
                if ((ex < OsmAnd::RoutePlanner::MinTurnAngle || mpi < OsmAnd::RoutePlanner::MinTurnAngle) && ex >= 0) {
                    kl = true;
                    int lns = attached->road->getLanes();
                    if (attached->road->getDirection() == OsmAnd::Model::RoadDirection::TwoWay) {
                        lns = (lns + 1) / 2;
                    }
                    if (lns > 0) {
                        right += lns;
                    }
                    speak = speak || rsSpeakPriority <= speakPriority;
                } else if ((ex > -OsmAnd::RoutePlanner::MinTurnAngle || mpi < OsmAnd::RoutePlanner::MinTurnAngle) && ex <= 0) {
                    kr = true;
                    int lns = attached->road->getLanes();
                    if (attached->road->getDirection() == OsmAnd::Model::RoadDirection::TwoWay) {
                        lns = (lns + 1) / 2;
                    }
                    if (lns > 0) {
                        left += lns;
                    }
                    speak = speak || rsSpeakPriority <= speakPriority;
                }
            }
        }
    }
    if (kr && left == 0) {
        left = 1;
    } else if (kl && right == 0) {
        right = 1;
    }
    int current = currentSegm->road->getLanes();
    if (currentSegm->road->getDirection() == OsmAnd::Model::RoadDirection::TwoWay) {
        current = (current + 1) / 2;
    }
    if (current <= 0) {
        current = 1;
    }
//        if (ls >= 0 /*&& current + left + right >= ls*/){
        lanes.resize(current + left + right);
        ls = current + left + right;
        for(int it=0; it< ls; it++) {
            if (it < left || it >= left + current) {
                lanes[it] = 0;
            } else {
                lanes[it] = 1;
            }
        }
        // sometimes links are
        if ((current <= left + right) && (left > 1 || right > 1)) {
            speak = true;
        }
//        }

    double devation = abs(OsmAnd::Utilities::degreesDiff(prevSegm->getBearingEnd(), currentSegm->getBearingBegin()));
    bool makeSlightTurn = devation > 5 && (!isMotorway(prevSegm) || !isMotorway(currentSegm));
    if (kl) {
        t = OsmAnd::TurnInfo(devation > 5 ? OsmAnd::TurnType::TSLL : OsmAnd::TurnType::KL);
        t.setSkipToSpeak(!speak);
    }
    if (kr) {
        t = OsmAnd::TurnInfo(devation > 5 ? OsmAnd::TurnType::TSLR : OsmAnd::TurnType::KR);
        t.setSkipToSpeak(!speak);
    }
    t.setLanes(lanes);
    return t;
}

OsmAnd::TurnInfo processRoundaboutTurn(QVector<std::shared_ptr<OsmAnd::RouteSegment> >& result, int i, bool leftSide,
                               std::shared_ptr<OsmAnd::RouteSegment> prev,std::shared_ptr<OsmAnd::RouteSegment>  rr) {
    int exit = 1;
    std::shared_ptr<OsmAnd::RouteSegment> last = rr;
    for (int j = i; j < result.size(); j++) {
        std::shared_ptr<OsmAnd::RouteSegment> rnext = result[j];
        last = rnext;
        if (rnext->road->isRoundabout()) {
            bool plus = rnext->startPointIndex < rnext->endPointIndex;
            int k = rnext->startPointIndex;
            if (j == i) {
                // first exit could be immediately after roundabout enter
//                    k = plus ? k + 1 : k - 1;
            }
            while (k != rnext->endPointIndex) {
                int attachedRoads = rnext->attachedRoutes[qAbs(static_cast<int64_t>(k) - rnext->startPointIndex)].size();
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
    OsmAnd::TurnInfo t = OsmAnd::TurnInfo(leftSide? OsmAnd::RD_LEFT : OsmAnd::RD_RIGHT, exit);
    t.setTurnAngle((float) OsmAnd::Utilities::degreesDiff(last->getBearingBegin(), prev->getBearingEnd())) ;
    return t;
}




OsmAnd::TurnInfo getTurnInfo(QVector<std::shared_ptr<OsmAnd::RouteSegment> >& result, int i, bool leftSide) {
    if (i == 0) {
        return OsmAnd::TurnInfo::straight();
    }
    std::shared_ptr<OsmAnd::RouteSegment> prev = result[i - 1];
    if (prev->road->isRoundabout()) {
        // already analyzed!
        return OsmAnd::TurnInfo();
    }
    std::shared_ptr<OsmAnd::RouteSegment> rr = result[i];
    if (rr->road->isRoundabout()) {
        return processRoundaboutTurn(result, i, leftSide, prev, rr);
    }
    OsmAnd::TurnInfo t;
    if (prev != nullptr) {
        bool noAttachedRoads = rr->attachedRoutes[0].size() == 0;
        // add description about turn
        double mpi = OsmAnd::Utilities::degreesDiff(prev->getBearingEnd(), rr->getBearingBegin());
        if (noAttachedRoads){
            // TODO VICTOR : look at the comment inside direction route
//                double begin = rr.getObject().directionRoute(rr.getStartPointIndex(), rr.getStartPointIndex() <
//                        rr.getEndPointIndex(), 25);
//                mpi = MapUtils.degreesDiff(prev.getBearingEnd(), begin);
        }
        if (mpi >= OsmAnd::RoutePlanner::MinTurnAngle) {
            if (mpi < 60) {
                t = OsmAnd::TurnInfo(OsmAnd::TurnType::TSLL);
            } else if (mpi < 120) {
                t = OsmAnd::TurnInfo(OsmAnd::TurnType::TL);
            } else if (mpi < 135 || leftSide) {
                t = OsmAnd::TurnInfo(OsmAnd::TurnType::TSHL);
            } else {
                t = OsmAnd::TurnInfo(OsmAnd::TurnType::TU);
            }
        } else if (mpi < -OsmAnd::RoutePlanner::MinTurnAngle) {
            if (mpi > -60) {
                t = OsmAnd::TurnInfo(OsmAnd::TurnType::TSLR);
            } else if (mpi > -120) {
                t = OsmAnd::TurnInfo(OsmAnd::TurnType::TR);
            } else if (mpi > -135 || !leftSide) {
                t = OsmAnd::TurnInfo(OsmAnd::TurnType::TSHR);
            } else {
                t = OsmAnd::TurnInfo(OsmAnd::TurnType::TRU);
            }
        } else {
            t = attachKeepLeftInfoAndLanes(leftSide, prev, rr, t);
        }
        if (t.getType() != OsmAnd::UKNOWN) {
            t.setTurnAngle((float) -mpi);
        }
    }
    return t;
}


void OsmAnd::RoutePlanner::addTurnInfoToRoute( bool leftside, QVector< std::shared_ptr<RouteSegment> >& result ) {
    int prevSegment = -1;
    float dist = 0;
    int next = 1;
    for (int i = 0; i <= result.size(); i = next) {
        TurnInfo t;
        next = i + 1;
        if (i < result.size()) {
            t = getTurnInfo(result, i, leftside);
            // justify turn
            if (t.getType() != UKNOWN && i < result.size() - 1) {
                bool tl = OsmAnd::TurnType::TL == t.getType();
                bool tr = OsmAnd::TurnType::TR == t.getType();
                if (tl || tr) {
                    TurnInfo tnext = getTurnInfo(result, i + 1, leftside);
                    if (tnext.getType() != OsmAnd::UKNOWN && result[i]->distance < 35) {
                        if (tl && TurnType::TL == tnext.getType() ) {
                            next = i + 2;
                            t = TurnInfo(OsmAnd::TurnType::TU);
                        } else if (tr && OsmAnd::TurnType::TR == tnext.getType() ) {
                            next = i + 2;
                            t = TurnInfo(OsmAnd::TurnType::TRU);
                        }
                    }
                }
            }
            result[i]->_turnType = t;
        }
        if (t.getType() != UKNOWN || i == result.size()) {
            if (prevSegment >= 0) {
                QString turn = result[prevSegment]->turnInfo.toString();
                if (result[prevSegment]->turnInfo.getLanes().size() > 0) {
                    turn += "{";
                    for(int li : result[prevSegment]->turnInfo.getLanes()) {
                        turn += QString::number(li) + " ";
                    }
                    turn += "} ";
                }
                result[prevSegment]->_description =  turn + " and go " + QString::number(dist) + " meters";
                if (result[prevSegment]->turnInfo.isSkipToSpeak()) {
                    result[prevSegment]->_description = "-*"+result[prevSegment]->description;
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
