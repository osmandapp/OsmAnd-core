#include "RoutePlanner.h"
#include "RoutePlannerRouteAnalyzer.h"

#include <queue>

#include <QtCore>

#include "ObfReader.h"
#include "Common.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::RoutePlannerAnalyzer::RoutePlannerAnalyzer()
{
}

OsmAnd::RoutePlannerAnalyzer::~RoutePlannerAnalyzer()
{
}
bool OsmAnd::RoutePlannerAnalyzer::prepareResult(OsmAnd::RoutePlannerContext::CalculationContext* context,
                                         std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> finalSegment,
                                         QList< std::shared_ptr<RouteSegment> >* outResult,
                                         bool leftSideNavigation){
    // Prepare result
    QList< std::shared_ptr<RouteSegment> > route;
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


    if(!validateAllPointsConnected(route))
        return false;
    splitRoadsAndAttachRoadSegments(context, route);
    calculateTimeSpeedInRoute(context, route);

    addTurnInfoToRoute(leftSideNavigation, route);

#if DEBUG_ROUTING || TRACE_ROUTING
    LogPrintf(LogSeverityLevel::Debug, "%llu segments in route", route.size());
#endif

    if(outResult)
        (*outResult) = route;
    context->owner->_previouslyCalculatedRoute = route;
    /*
    if (RoutingContext.SHOW_GC_SIZE) {
        int sz = ctx.global.size;
        log.warn("Subregion size " + ctx.subregionTiles.size() + " " + " tiles " + ctx.indexedSubregions.size());
        ctx.runGCUsedMemory();
        long h1 = ctx.runGCUsedMemory();
        ctx.unloadAllData();
        ctx.runGCUsedMemory();
        long h2 = ctx.runGCUsedMemory();
        float mb = (1 << 20);
        log.warn("Unload context :  estimated " + sz / mb + " ?= " + (h1 - h2) / mb + " actual");
    }*/
    return true;
}

void OsmAnd::RoutePlannerAnalyzer::splitRoadsAndAttachRoadSegments( OsmAnd::RoutePlannerContext::CalculationContext* context, QList< std::shared_ptr<RouteSegment> >& route )
{
    for(auto itSegment = route.begin(); itSegment != route.end(); ++itSegment)
    {
        /*TODO:GC
        if (ctx.checkIfMemoryLimitCritical(ctx.config.memoryLimitation)) {
            ctx.unloadUnusedTiles(ctx.config.memoryLimitation);
        }
        */
        auto segment = *itSegment;
        //TODO:GC:checkAndInitRouteRegion(context, segment->road);

        const bool isIncrement = segment->startPointIndex < segment->endPointIndex;
        for(auto pointIdx = segment->startPointIndex; pointIdx != segment->endPointIndex; isIncrement ? pointIdx++ : pointIdx--)
        {
            const auto nextIdx = (isIncrement ? pointIdx + 1 : pointIdx - 1);

            if (pointIdx == segment->startPointIndex)
                attachRouteSegments(context, route, itSegment, pointIdx, isIncrement);

            if (nextIdx != segment->endPointIndex)
                attachRouteSegments(context, route, itSegment, nextIdx, isIncrement);

            if(nextIdx < 0 || nextIdx >= segment->attachedRoutes.size())
                continue;
            if(nextIdx >= 0 && nextIdx < segment->attachedRoutes.size() && nextIdx != segment->endPointIndex && !segment->road->isRoundabout())
            {
                const auto& attachedRoutes = segment->attachedRoutes[nextIdx];

                auto before = segment->getBearing(nextIdx, !isIncrement);
                auto after = segment->getBearing(nextIdx, isIncrement);
                auto straight = qAbs(Utilities::normalizedAngleDegrees(before + 180.0 - after)) < (double)MinTurnAngle;
                auto isSplit = false;

                // split if needed
                for(auto itAttachedSegment = attachedRoutes.begin(); itAttachedSegment != attachedRoutes.end(); ++itAttachedSegment)
                {
                    auto attachedSegment = *itAttachedSegment;

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

                    itSegment = route.insert(++itSegment, split);
                    
                    // switch current segment to the splited
                    segment = split;
                }
            }
        }
    }
}

void OsmAnd::RoutePlannerAnalyzer::attachRouteSegments(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    QList< std::shared_ptr<RouteSegment> >& route,
    const QList< std::shared_ptr<RouteSegment> >::iterator& itSegment,
    uint32_t pointIdx, bool isIncrement)
{
    auto segment = *itSegment;
    const auto& nextL = pointIdx < segment->road->points.size() - 1
        ? segment->road->points[pointIdx + 1]
        : PointI();
    const auto& prevL = pointIdx > 0
        ? segment->road->points[pointIdx - 1]
        : PointI();

    // attach additional roads to represent more information about the route
    std::shared_ptr<RouteSegment> previousResult;

    // by default make same as this road id
    auto previousRoadId = segment->road->id;
    if (pointIdx == segment->startPointIndex && itSegment != route.begin())
    {
        previousResult = *(itSegment - 1);
        previousRoadId = previousResult->road->id;
        if (previousRoadId != segment->road->id)
        {
            std::shared_ptr<RouteSegment> attachedSegment;

            if (previousResult->startPointIndex < previousResult->endPointIndex &&
                previousResult->endPointIndex < previousResult->road->points.size() - 1)
                attachedSegment.reset(new RouteSegment(previousResult->road, previousResult->endPointIndex, previousResult->road->points.size() - 1));
            else if (previousResult->startPointIndex > previousResult->endPointIndex && previousResult->endPointIndex > 0)
                attachedSegment.reset(new RouteSegment(previousResult->road, previousResult->endPointIndex, 0));

            if(attachedSegment)
                segment->_attachedRoutes[qAbs(static_cast<int64_t>(pointIdx) - static_cast<int64_t>(segment->_startPointIndex))].push_back(attachedSegment);
        }
    }

    /*TODO:
    Iterator<RouteSegment> it;
    if(segment->getPreAttachedRoutes(pointInd) != null) {
        final RouteSegmentResult[] list = segment->getPreAttachedRoutes(pointInd);
        it = new Iterator<BinaryRoutePlanner.RouteSegment>() {
            int i = 0;
            @Override
                public boolean hasNext() {
                    return i < list.length;
            }

            @Override
                public RouteSegment next() {
                    RouteSegmentResult r = list[i++];
                    return new RouteSegment(r.getObject(), r.getStartPointIndex());
            }

            @Override
                public void remove() {
            }
        };	
    } else {
        RouteSegment rt = ctx.loadRouteSegment(road.getPoint31XTile(pointInd), road.getPoint31YTile(pointInd), ctx.config.memoryLimitation);
        it = rt == null ? null : rt.getIterator();
    }*/

    // Try to attach all segments except with current id
    const auto& p31 = segment->road->points[pointIdx];
    auto rt = OsmAnd::RoutePlanner::loadRouteCalculationSegment(context->owner, p31.x, p31.y);
    while(rt)
    {
        if(rt->road->id != segment->road->id && rt->road->id != previousRoadId)
        {
            std::shared_ptr<RouteSegment> attachedSegment;
            //TODO:GC:checkAndInitRouteRegion(ctx, rt->road);
            // TODO restrictions can be considered as well
            auto direction = context->owner->profileContext->getDirection(rt->road.get());
            if ((direction == Model::Road::TwoWay || direction == Model::Road::OneWayReverse) && rt->pointIndex < rt->road->points.size() - 1)
            {
                const auto& otherPoint = rt->road->points[rt->pointIndex + 1];
                if(otherPoint != nextL && otherPoint != prevL)
                {
                    // if way contains same segment (nodes) as different way (do not attach it)
                    attachedSegment.reset(new RouteSegment(rt->road, rt->pointIndex, rt->road->points.size() - 1));
                }
            }
            if ((direction == Model::Road::TwoWay || direction == Model::Road::OneWayForward) && rt->pointIndex > 0)
            {
                const auto& otherPoint = rt->road->points[rt->pointIndex - 1];
                if(otherPoint != nextL && otherPoint != prevL)
                {
                    // if way contains same segment (nodes) as different way (do not attach it)
                    attachedSegment.reset(new RouteSegment(rt->road, rt->pointIndex, 0));
                }
            }

            if(attachedSegment)
                segment->_attachedRoutes[qAbs(static_cast<int64_t>(pointIdx) - static_cast<int64_t>(segment->_startPointIndex))].push_back(attachedSegment);
        }

        rt = rt->next;
    }
}

void OsmAnd::RoutePlannerAnalyzer::calculateTimeSpeedInRoute( OsmAnd::RoutePlannerContext::CalculationContext* context, QList< std::shared_ptr<RouteSegment> >& route )
{
    for(auto itSegment = route.begin(); itSegment != route.end(); ++itSegment)
    {
        auto segment = *itSegment;

        float distOnRoadToPass = 0;
        float speed = context->owner->profileContext->getSpeed(segment->road.get());
        if (qFuzzyCompare(speed, 0))
            speed = context->owner->profileContext->profile->minDefaultSpeed;

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
            auto obstacle = context->owner->profileContext->getObstaclesExtraTime(segment->road.get(), pointIdx);
            if (obstacle < 0)
                obstacle = 0;
            distOnRoadToPass += distance / speed + obstacle;
        }

        // last point turn time can be added
        // if(i + 1 < result.size()) { distOnRoadToPass += ctx.getRouter().calculateTurnTime(); }
        segment->_time = distOnRoadToPass;
        segment->_speed = speed;
        segment->_distance = distanceSum;
    }
}

void OsmAnd::RoutePlannerAnalyzer::addTurnInfoToRoute( bool leftSideNavigation, QList< std::shared_ptr<RouteSegment> >& route )
{
    /*
    int prevSegment = -1;
    float dist = 0;
    int next = 1;
    for (int i = 0; i <= result.size(); i = next) {
        TurnType t = null;
        next = i + 1;
        if (i < result.size()) {
            t = getTurnInfo(result, i, leftside);
            // justify turn
            if(t != null && i < result.size() - 1) {
                boolean tl = TurnType.TL.equals(t.getValue());
                boolean tr = TurnType.TR.equals(t.getValue());
                if(tl || tr) {
                    TurnType tnext = getTurnInfo(result, i + 1, leftside);
                    if(tnext != null && result.get(i).getDistance() < 35) {
                        if(tl && TurnType.TL.equals(tnext.getValue()) ) {
                            next = i + 2;
                            t = TurnType.valueOf(TurnType.TU, false);
                        } else if(tr && TurnType.TR.equals(tnext.getValue()) ) {
                            next = i + 2;
                            t = TurnType.valueOf(TurnType.TU, true);
                        }
                    }
                }
            }
            result.get(i).setTurnType(t);
        }
        if (t != null || i == result.size()) {
            if (prevSegment >= 0) {
                String turn = result.get(prevSegment).getTurnType().toString();
                if (result.get(prevSegment).getTurnType().getLanes() != null) {
                    turn += Arrays.toString(result.get(prevSegment).getTurnType().getLanes());
                }
                result.get(prevSegment).setDescription(turn + MessageFormat.format(" and go {0,number,#.##} meters", dist));
                if(result.get(prevSegment).getTurnType().isSkipToSpeak()) {
                    result.get(prevSegment).setDescription("-*"+result.get(prevSegment).getDescription());
                }
            }
            prevSegment = i;
            dist = 0;
        }
        if (i < result.size()) {
            dist += result.get(i).getDistance();
        }
    }
    */
}

bool OsmAnd::RoutePlannerAnalyzer::validateAllPointsConnected( const QList< std::shared_ptr<RouteSegment> >& route )
{
    assert(route.size() > 1);

    auto itPrevSegment = route.begin();
    bool res = true;
    LogPrintf(LogSeverityLevel::Debug, "Segment : %llu %u %u ", (*itPrevSegment)->road->id, (*itPrevSegment)->startPointIndex,  (*itPrevSegment)->endPointIndex);
    for(auto itSegment = ++route.begin(); itSegment != route.end(); ++itSegment, ++itPrevSegment)
    {
        auto prevSegment = *itPrevSegment;
        auto segment = *itSegment;

        const auto& point1 = prevSegment->road->points[prevSegment->endPointIndex];
        const auto& point2 = segment->road->points[segment->startPointIndex];
        auto distance = Utilities::distance(
            Utilities::get31LongitudeX(point1.x), Utilities::get31LatitudeY(point1.y),
            Utilities::get31LongitudeX(point2.x), Utilities::get31LatitudeY(point2.y)
        );
        LogPrintf(LogSeverityLevel::Debug, "Segment : %llu %u %u  ", segment->road->id, segment->startPointIndex,  segment->endPointIndex);
        if(distance > 0)
        {
            res = false;

            LogPrintf(LogSeverityLevel::Error, "!! Points are not connected : %llu(%u) -> %llu(%u) %f meters",
                prevSegment->road->id, prevSegment->endPointIndex, segment->road->id, segment->startPointIndex, distance);
        }
    }

    return res;
}

void OsmAnd::RoutePlannerAnalyzer::addRouteSegmentToRoute( QList< std::shared_ptr<RouteSegment> >& route, const std::shared_ptr<RouteSegment>& segment, bool reverse )
{
    if(segment->startPointIndex == segment->endPointIndex)
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

bool OsmAnd::RoutePlannerAnalyzer::combineTwoSegmentResult( const std::shared_ptr<RouteSegment>& toAdd, const std::shared_ptr<RouteSegment>& previous, bool reverse )
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
