#include "RoutePlanner.h"

#include <queue>
#include <ctime>

#include <OsmAndCore/QtExtensions.h>
#include <QtCore>

#include "ObfReader.h"
#include "ObfRoutingSectionReader.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionInfo_P.h"
#include "Common.h"
#include "Logging.h"
#include "LoggingAssert.h"
#include "Utilities.h"
#include "PlainQueryFilter.h"
#include "QKeyValueIterator.h"
#include "QCachingIterator.h"

OsmAnd::RoutePlanner::RoutePlanner()
{
}

OsmAnd::RoutePlanner::~RoutePlanner()
{
}

bool OsmAnd::RoutePlanner::findClosestRoadPoint(
    OsmAnd::RoutePlannerContext* context,
    double latitude, double longitude,
    std::shared_ptr<const OsmAnd::Model::Road>* closestRoad /*= nullptr*/,
    uint32_t* closestPointIndex /*= nullptr*/,
    double* sqDistanceToClosestPoint /*= nullptr*/,
    uint32_t* _rx31 /*= nullptr*/, uint32_t* _ry31 /*= nullptr*/ )
{
    const auto x31 = Utilities::get31TileNumberX(longitude);
    const auto y31 = Utilities::get31TileNumberY(latitude);

    QList< std::shared_ptr<const Model::Road> > roads;
    loadRoads(context, x31, y31, 17, roads);
    if (roads.isEmpty())
        loadRoads(context, x31, y31, 15, roads);

    std::shared_ptr<const OsmAnd::Model::Road> minDistanceRoad;
    uint32_t minDistancePointIdx;
    double minSqDistance = std::numeric_limits<double>::max();
    uint32_t min31x, min31y;
    for(const auto& road : constOf(roads))
    {
        const auto& points = road->points;
        if (points.size() <= 1)
            continue;

        for(auto idx = 1, count = points.size(); idx < count; idx++)
        {
            const auto& cpx31 = points[idx].x;
            const auto& cpy31 = points[idx].y;
            const auto& ppx31 = points[idx - 1].x;
            const auto& ppy31 = points[idx - 1].y;

            auto sqDistance = Utilities::squareDistance31(cpx31, cpy31, ppx31, ppy31);

            uint32_t rx31;
            uint32_t ry31;
            auto projection = Utilities::projection31(ppx31, ppy31, cpx31, cpy31, x31, y31);
            if (projection < 0)
            {
                rx31 = ppx31;
                ry31 = ppy31;
            }
            else if (projection >= sqDistance)
            {
                rx31 = cpx31;
                ry31 = cpy31;
            }
            else
            {
                const auto& factor = projection / sqDistance;
                rx31 = ppx31 + (cpx31 - ppx31) * factor;
                ry31 = ppy31 + (cpy31 - ppy31) * factor;
            }
            sqDistance = Utilities::squareDistance31(rx31, ry31, x31, y31);
            if (!road || sqDistance < minSqDistance)
            {
                minDistanceRoad = road;
                minDistancePointIdx = idx;
                minSqDistance = sqDistance;
                min31x = rx31;
                min31y = ry31;
            }
        }
    }

    if (minDistanceRoad)
    {
        if (closestRoad)
            *closestRoad = minDistanceRoad;
        if (closestPointIndex)
            *closestPointIndex = minDistancePointIdx;
        if (sqDistanceToClosestPoint)
            *sqDistanceToClosestPoint = minSqDistance;
        if (_rx31)
            *_rx31 = min31x;
        if (_ry31)
            *_ry31 = min31y;
        return true;
    }
    return false;
}

bool OsmAnd::RoutePlanner::findClosestRouteSegment( OsmAnd::RoutePlannerContext* context, double latitude, double longitude, std::shared_ptr<OsmAnd::RoutePlannerContext::RouteCalculationSegment>& routeSegment )
{
    std::shared_ptr<const OsmAnd::Model::Road> closestRoad;
    uint32_t closestPointIndex;
    uint32_t rx31, ry31;

    if (!findClosestRoadPoint(context, latitude, longitude, &closestRoad, &closestPointIndex, nullptr, &rx31, &ry31))
        return false;

    // will be bug if it is not inserted
    std::shared_ptr<Model::Road> clonedRoad(new Model::Road(closestRoad, closestPointIndex, rx31, ry31));
    routeSegment.reset(new RoutePlannerContext::RouteCalculationSegment(clonedRoad, closestPointIndex));
    // Cache road in tiles it goes through
    cacheRoad(context, clonedRoad);
    
    return true;
}

void OsmAnd::RoutePlanner::cacheRoad( RoutePlannerContext* context, const std::shared_ptr<Model::Road>& road )
{
    if (!context->profileContext->acceptsRoad(road))
        return;

    for(const auto& point : constOf(road->points))
    {
        const auto& px31 = point.x;
        const auto& py31 = point.y;

        const auto tileId = getRoutingTileId(context, px31, py31, true);

        auto itCache = context->_cachedRoadsInTiles.find(tileId);
        if (itCache == context->_cachedRoadsInTiles.end())
            itCache = context->_cachedRoadsInTiles.insert(tileId, QList< std::shared_ptr<Model::Road> >());

        if (!itCache->contains(road))
            itCache->push_back(road);
    }
}

void OsmAnd::RoutePlanner::loadRoads( RoutePlannerContext* context, uint32_t x31, uint32_t y31, uint32_t zoomAround, QList< std::shared_ptr<const Model::Road> >& roads )
{
    auto coordinatesShift = 1 << (31 - context->_roadTilesLoadingZoomLevel);
    uint32_t t;
    if (zoomAround >= context->_roadTilesLoadingZoomLevel)
    {
        t = 1;
        coordinatesShift = 1 << (31 - zoomAround);
    }
    else
    {
        t = 1 << (context->_roadTilesLoadingZoomLevel - zoomAround);
    }

    QSet<uint64_t> processedTiles;
    for(auto i = -(int64_t)t; i <= t; i++)
    {
        for(auto j = -(int64_t)t; j <= t; j++)
        {
            auto tileId = getRoutingTileId(context, x31+i*coordinatesShift, y31+j*coordinatesShift, false);
            if (processedTiles.contains(tileId))
                continue;
            loadRoadsFromTile(context, tileId, roads);
            processedTiles.insert(tileId);
        }
    }
}

void OsmAnd::RoutePlanner::loadRoadsFromTile( RoutePlannerContext* context, uint64_t tileId, QList< std::shared_ptr<const Model::Road> >& roads )
{
    QMap<uint64_t, std::shared_ptr<const Model::Road> > duplicates;

    auto itRoadsInTile = context->_cachedRoadsInTiles.constFind(tileId);
    if (itRoadsInTile != context->_cachedRoadsInTiles.cend())
    {
        for(const auto& road : constOf(*itRoadsInTile))
        {
            if (duplicates.contains(road->id))
                continue;

            duplicates.insert(road->id, road);
            roads.push_back(road);
        }
    }

    auto itIndexedSubsectionContexts = context->_indexedSubsectionsContexts.constFind(tileId);
    assert(itIndexedSubsectionContexts != context->_indexedSubsectionsContexts.cend());
    for(auto& subsectionContext : *itIndexedSubsectionContexts)
        subsectionContext->collectRoads(roads, &duplicates);
}

uint32_t OsmAnd::RoutePlanner::getCurrentEstimatedSize(RoutePlannerContext* context)
{
    // TODO Victor
    return context->getCurrentEstimatedSize(); // + current stack size  * 2000; //+ current queue size
}

uint64_t OsmAnd::RoutePlanner::getRoutingTileId( RoutePlannerContext* context, uint32_t x31, uint32_t y31, bool dontLoad )
{
    auto xTileId = x31 >> (31 - context->_roadTilesLoadingZoomLevel);
    auto yTileId = y31 >> (31 - context->_roadTilesLoadingZoomLevel);
    uint64_t tileId = (xTileId << context->_roadTilesLoadingZoomLevel) + yTileId;
    if (dontLoad)
        return tileId;
    
    if (!dontLoad) {
        auto memoryLimit = context->_memoryUsageLimit;
        uint32_t estimatedSize = getCurrentEstimatedSize(context);
        if ( estimatedSize > 0.9 * memoryLimit) {
            int clt = context->getCurrentlyLoadedTiles();
            context->unloadUnusedTiles(memoryLimit);
            int unloaded = clt - context->getCurrentlyLoadedTiles() ;
            if (unloaded > 0) {
                OsmAnd::LogPrintf(LogSeverityLevel::Warning,"Unload %d tiles :  estimated size %d", unloaded,
                                  (estimatedSize - getCurrentEstimatedSize(context)));
            }
        }
    }

    auto itIndexedSubsectionContexts = context->_indexedSubsectionsContexts.constFind(tileId);
    if (itIndexedSubsectionContexts == context->_indexedSubsectionsContexts.cend())
    {
        QList< std::shared_ptr<RoutePlannerContext::RoutingSubsectionContext> > subsectionContexts;
        loadTileHeader(context, x31, y31, subsectionContexts);
        itIndexedSubsectionContexts = context->_indexedSubsectionsContexts.insert(tileId, subsectionContexts);
    }

    assert(itIndexedSubsectionContexts != context->_indexedSubsectionsContexts.cend());
    for(const auto& subsectionContext : constOf(*itIndexedSubsectionContexts))
    {
        if (subsectionContext->isLoaded())
            continue;

        loadSubregionContext(subsectionContext.get());
    }

    return tileId;
}

void OsmAnd::RoutePlanner::loadTileHeader( RoutePlannerContext* context, uint32_t x31, uint32_t y31, QList< std::shared_ptr<RoutePlannerContext::RoutingSubsectionContext> >& subsectionsContexts )
{
    const auto zoomToLoad = 31 - context->_roadTilesLoadingZoomLevel;
    auto xTileId = x31 >> (31 - context->_roadTilesLoadingZoomLevel);
    auto yTileId = y31 >> (31 - context->_roadTilesLoadingZoomLevel);

    AreaI bbox31;
    bbox31.left = xTileId << zoomToLoad;
    bbox31.right = (xTileId + 1) << zoomToLoad;
    bbox31.top = yTileId << zoomToLoad;
    bbox31.bottom = (yTileId + 1) << zoomToLoad;
    PlainQueryFilter filter(nullptr, &bbox31);

    for(const auto& source : constOf(context->sources))
    {
        const auto& obfInfo = source->obtainInfo();
        for(const auto& routingSection : constOf(obfInfo->routingSections))
        {
            QList< std::shared_ptr<const ObfRoutingSubsectionInfo> > subsections;
            ObfRoutingSectionReader::querySubsections(
                source,
                context->_useBasemap ? routingSection->baseSubsections : routingSection->subsections,
                &subsections,
                &filter,
                [](const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection)
                {
                    return subsection->containsData();
                });
            for(const auto& subsection : constOf(subsections))
            {
                auto itSubsectionContext = context->_subsectionsContextsLUT.constFind(subsection.get());
                if (itSubsectionContext == context->_subsectionsContextsLUT.cend())
                {
                    const std::shared_ptr<RoutePlannerContext::RoutingSubsectionContext> subsectionContext(new RoutePlannerContext::RoutingSubsectionContext(context, source, subsection));
                    itSubsectionContext = context->_subsectionsContextsLUT.insert(subsection.get(), subsectionContext);
                    context->_subsectionsContexts.push_back(qMove(subsectionContext));
                }

                subsectionsContexts.push_back(*itSubsectionContext);
            }
        }
    }

}

void OsmAnd::RoutePlanner::loadSubregionContext( RoutePlannerContext::RoutingSubsectionContext* context )
{
    const auto wasUnloaded = !context->isLoaded();
    const auto loadsCount = context->getLoadsCounter();
    if (context->owner->_routeStatistics) {
        context->owner->_routeStatistics->timeToLoadBegin = std::chrono::steady_clock::now();
    }
    context->markLoaded();
    ObfRoutingSectionReader::loadSubsectionData(context->origin, context->subsection, nullptr, nullptr, nullptr,
        [=] (const std::shared_ptr<const OsmAnd::Model::Road>& road)
        {
            if (!context->owner->profileContext->acceptsRoad(road))
                return false;

            context->registerRoad(road);
            return false;
        }
    );

    if (context->owner->_routeStatistics) {
        context->owner->_routeStatistics->timeToLoad += (uint64_t) (
        std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - context->owner->_routeStatistics->timeToLoadBegin).count());
        context->owner->_routeStatistics->loadedTiles ++;
        context->owner->_loadedTiles++;
        if (wasUnloaded) {
            if (loadsCount == 1) {
                context->owner->_routeStatistics->loadedPrevUnloadedTiles++;
            }
        } else {
            context->owner->_routeStatistics->distinctLoadedTiles++;
        }
    }
}

OsmAnd::RouteCalculationResult OsmAnd::RoutePlanner::calculateRoute(
    OsmAnd::RoutePlannerContext* context,
    const QList< std::pair<double, double> >& points,
    bool leftSideNavigation,
    const IQueryController* const controller /*= nullptr*/)
{
    assert(context != nullptr);
    assert(points.size() >= 2);

    /* TODO VICTOR
    if (ctx.calculationProgress == null) {
        ctx.calculationProgress = new RouteCalculationProgress();
    }
    */

    QList< std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> > routeCalculationSegments;
    for(const auto& point : constOf(points))
    {
        std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> segment;
        if (!findClosestRouteSegment(context, point.first, point.second, segment))
        {
            OsmAnd::RouteCalculationResult r;
            /*if (itPoint == points.cbegin()) {
                return OsmAnd::RouteCalculationResult("Start point was not found");
            } else if (itPoint == points.cend()) {
                return OsmAnd::RouteCalculationResult("End point was not found");
            } else {
                return OsmAnd::RouteCalculationResult("Intermediate point was not found");
            }*/
        }
        routeCalculationSegments.push_back(qMove(segment));
    }
    
    if (routeCalculationSegments.size() > 2)
    {
        // TODO route recalculation
//        std::shared_ptr< QList< std::shared_ptr<RouteSegment> > > firstPartRecalculatedRoute;
//        QList< std::shared_ptr<RouteSegment> > restPartRecalculatedRoute;
//
//        if (context->_previouslyCalculatedRoute)
//        {
//            const auto id = routeCalculationSegments[1]->road->id;
//            const auto segmentStart = routeCalculationSegments[1]->pointIndex;
//            
//            int prevRouteSegmentIdx = 0;
//            for(auto itPrevRouteSegment = context->_previouslyCalculatedRoute->begin(); itPrevRouteSegment != context->_previouslyCalculatedRoute->end(); ++itPrevRouteSegment, prevRouteSegmentIdx++)
//            {
//                auto prevRouteSegment = *itPrevRouteSegment;
//                if (id != prevRouteSegment->getObject().getId() || segmentStart != prevRouteSegment->getEndPointIndex())
//                    continue;
//
//                firstPartRecalculatedRoute.reset(new QList< std::shared_ptr<RouteSegment> >(prevRouteSegmentIdx + 1));
//                restPartRecalculatedRoute.reserve(context->_previouslyCalculatedRoute->size() - prevRouteSegmentIdx);
//                itPrevRouteSegment++;
//                for(auto itRouteSegment = itPrevRouteSegment; itRouteSegment != context->_previouslyCalculatedRoute->end(); ++itRouteSegment)
//                    restPartRecalculatedRoute.push_back(*itRouteSegment);
//                for(auto itRouteSegment = context->_previouslyCalculatedRoute->begin(); itRouteSegment != itPrevRouteSegment; ++itRouteSegment)
//                    firstPartRecalculatedRoute->push_back(*itRouteSegment);
//                break;
//            }
//        }
//
//        bool routeExists = false;
//        for(auto itRouteCalculationSegment = routeCalculationSegments.cbegin(); itRouteCalculationSegment != routeCalculationSegments.cend(); ++itRouteCalculationSegment)
//        {
//            std::unique_ptr<RoutePlannerContext::CalculationContext> calculationContext(new RoutePlannerContext::CalculationContext(context));
//
//            if (itRouteCalculationSegment == routeCalculationSegments.cbegin() && firstPartRecalculatedRoute)
//                calculationContext->_previouslyCalculatedRoute = firstPartRecalculatedRoute;
//
////            local.calculationProgress = ctx.calculationProgress;
//            routeExists = calculateRoute(calculationContext.get(), *itRouteCalculationSegment, *(itRouteCalculationSegment + 1), leftSideNavigation, controller, outResult);
//            if (!routeExists)
//                break;
//
//            /*
//            ctx.distinctLoadedTiles += local.distinctLoadedTiles;
//            ctx.loadedTiles += local.loadedTiles;
//            ctx.visitedSegments += local.visitedSegments;
//            ctx.loadedPrevUnloadedTiles += local.loadedPrevUnloadedTiles;
//            ctx.timeToCalculate += local.timeToCalculate;
//            ctx.timeToLoad += local.timeToLoad;
//            ctx.timeToLoadHeaders += local.timeToLoadHeaders;
//            ctx.relaxedSegments += local.relaxedSegments;
//            ctx.routingTime += local.routingTime;
//            */
//
//            //TODO: unloadAllData of local context in destructor
//            if (context->_previouslyCalculatedRoute)
//            {
//                if (outResult)
//                    outResult->push_back(restPartRecalculatedRoute);
//                break;
//            }
//        }
//        //TODO: ctx.unloadAllData();
//        return routeExists;
        int i = 5;
    }

    std::unique_ptr<RoutePlannerContext::CalculationContext> calculationContext(new RoutePlannerContext::CalculationContext(context));
    return calculateRoute(calculationContext.get(), routeCalculationSegments[0], routeCalculationSegments[1], leftSideNavigation, controller);
}

void OsmAnd::RoutePlanner::printDebugInformation(OsmAnd::RoutePlannerContext::CalculationContext* ctx, int directSegmentSize, int reverseSegmentSize,
           std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> finalSegment) {

    std::shared_ptr<RouteStatistics> st = ctx->owner->_routeStatistics;
    if (st) {
        st->timeToCalculate += (uint64_t) (
                    std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - st->timeToCalculateBegin).count());
        LogPrintf(LogSeverityLevel::Debug, "Time to calculate %llu, time to load %llu ", st->timeToCalculate, st->timeToLoad);
        LogPrintf(LogSeverityLevel::Debug, "Forward iterations %u, backward iterations %u", st->forwardIterations, st->backwardIterations);
        auto maxLoadedTiles = qMax(st->maxLoadedTiles, ctx->owner->getCurrentlyLoadedTiles());
        LogPrintf(LogSeverityLevel::Debug, "Current loaded tiles %d, maximum %d : " , ctx->owner->getCurrentlyLoadedTiles(), st->maxLoadedTiles);
                LogPrintf(LogSeverityLevel::Debug, "Loaded tiles %u (distinct %u), unloaded tiles %u, loaded more than once same tiles %u",
                          st->loadedTiles, st->distinctLoadedTiles, st->unloadedTiles, st->loadedPrevUnloadedTiles);
        LogPrintf(LogSeverityLevel::Debug, "D-Queue size %d, R-Queue size %d", directSegmentSize, reverseSegmentSize);
        LogPrintf(LogSeverityLevel::Debug, "Routing calculated time distance %f", finalSegment->_distanceFromStart);
        LogFlush();
    }
}

OsmAnd::RouteCalculationResult OsmAnd::RoutePlanner::calculateRoute(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& from,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& to_,
    bool leftSideNavigation,
    const IQueryController* const controller /*= nullptr*/)
{

    context->_startPoint = from->road->points[from->pointIndex];
    context->_targetPoint = to_->road->points[to_->pointIndex];

    #ifndef ROUTE_STATISTICS
        context->_routeStatistics = nullptr;
    #endif
    /* TODO VICTOR progress
    refreshProgressDistance(ctx);
    */
    // measure time
    if (context->owner->_routeStatistics) {
       context->owner->_routeStatistics->timeToLoad = 0;
       context->owner->_routeStatistics->timeToCalculate = 0;
       context->owner->_routeStatistics->forwardIterations = 0;
       context->owner->_routeStatistics->backwardIterations = 0;
       context->owner->_routeStatistics->timeToCalculateBegin = std::chrono::steady_clock::now();
    }

    if (!qIsNaN(context->owner->_initialHeading))
    {
        // Mark here as positive for further check
        context->_entranceRoadId = encodeRoutePointId(from->road, from->pointIndex, true);

        auto roadDirectionDelta = from->road->getDirectionDelta(from->pointIndex, true);
        auto delta = roadDirectionDelta - context->owner->_initialHeading;

        if (qAbs(Utilities::normalizedAngleRadians(delta)) <= M_PI / 3.0)
            context->_entranceRoadDirection = 1;
        else if (qAbs(Utilities::normalizedAngleRadians(delta - M_PI)) <= M_PI / 3.0)
            context->_entranceRoadDirection = -1;
    }

    // Initializing priority queue to visit way segments 
    const auto nonHeuristicRoadSegmentsComparator = []( const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& a, const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& b )
    {
        return roadPriorityComparator(
            a->_distanceFromStart, a->_distanceToEnd,
            b->_distanceFromStart, b->_distanceToEnd,
            0.5) > 0;
    };
    const auto roadSegmentsComparator = [context]( const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& a, const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& b )
    {
        return roadPriorityComparator(
            a->_distanceFromStart, a->_distanceToEnd,
            b->_distanceFromStart, b->_distanceToEnd,
            context->owner->_heuristicCoefficient) > 0;
    };
    RoadSegmentsPriorityQueue graphDirectSegments(roadSegmentsComparator);
    RoadSegmentsPriorityQueue graphReverseSegments(roadSegmentsComparator);
    
    // Set to not visit one segment twice (stores road.id << X + segmentStart)
    QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> > visitedDirectSegments;
    QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> > visitedOppositeSegments;
    
    auto to = to_;
    const auto runRecalculation = checkPartialRecalculationPossible(context, visitedOppositeSegments, to);
    
    // for start : f(start) = g(start) + h(start) = 0 + h(start) = h(start)
    auto estimatedDistance = estimateTimeDistance(context, context->_targetPoint, context->_startPoint);
    to->_distanceToEnd = from->_distanceToEnd = estimatedDistance;

#if DEBUG_ROUTING || TRACE_ROUTING
    from->dump("From: ");
    to->dump("To  : ");
#endif

    graphDirectSegments.push(from);
    graphReverseSegments.push(to);

    // Extract & analyze segment with min(f(x)) from queue while final segment is not found
    bool reverseSearch = false;
    bool initialized = false;

    RoadSegmentsPriorityQueue* pGraphSegments = reverseSearch ? &graphReverseSegments : &graphDirectSegments;
    loadBorderPoints(context);


    std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> finalSegment;
    while (!pGraphSegments->empty())
    {
#if TRACE_DUMP_QUEUE
        LogPrintf(LogSeverityLevel::Debug, "---------------------------------------");
        LogPrintf(LogSeverityLevel::Debug, "%s-Queue (%d):", reverseSearch ? "R" : "D", pGraphSegments->size());
        {
            auto queueDuplicate = *pGraphSegments;
            while(queueDuplicate.size() > 0)
            {
                queueDuplicate.top()->dump("\t");
                queueDuplicate.pop();
            }
        }
#endif

        auto segment = pGraphSegments->top();
        pGraphSegments->pop();

#if TRACE_ROUTING
        segment->dump("> ");
#endif

        if (dynamic_cast<RoutePlannerContext::RouteCalculationFinalSegment*>(segment.get()))
        {
            finalSegment = segment;
            break;
        }
        if (context->owner->getCurrentEstimatedSize() > context->owner->_memoryUsageLimit) {
            return OsmAnd::RouteCalculationResult("There is no enough memory " +
                                                  QString::number(context->owner->_memoryUsageLimit/(1<<20)) + " Mb");
        }


#if TRACE_ROUTING
        LogPrintf(LogSeverityLevel::Debug, "\tFwd-search:");
#endif
        calculateRouteSegment(
            context,
            reverseSearch,
            reverseSearch ? graphReverseSegments : graphDirectSegments,
            reverseSearch ? visitedOppositeSegments : visitedDirectSegments,
            segment,
            reverseSearch ? visitedDirectSegments : visitedOppositeSegments,
            true);
#if TRACE_ROUTING
        LogPrintf(LogSeverityLevel::Debug, "\tRev-search:");
#endif
        calculateRouteSegment(
            context,
            reverseSearch,
            reverseSearch ? graphReverseSegments : graphDirectSegments,
            reverseSearch ? visitedOppositeSegments : visitedDirectSegments,
            segment,
            reverseSearch ? visitedDirectSegments : visitedOppositeSegments,
            false);
        
        /* TODO progress
        updateCalculationProgress(ctx, graphDirectSegments, graphReverseSegments);
        */
        if (graphReverseSegments.size() == 0){
            return OsmAnd::RouteCalculationResult("Route is not found to selected target point.");
        }
        if (graphDirectSegments.size() == 0){
            return OsmAnd::RouteCalculationResult("Route is not found from selected start point.");
        }
        if (context->owner->_routeStatistics) {
            if (reverseSearch) {
                context->owner->_routeStatistics->backwardIterations++;
            } else {
                context->owner->_routeStatistics->forwardIterations++;
            }
        }
        
        if (runRecalculation)
        {
            // nothing to do
            reverseSearch = false;
        }
        else if (!initialized)
        {
            reverseSearch = !reverseSearch;
            initialized = true;
        }
        else if (context->owner->_planRoadDirection == 0)
        {
            auto directTop = graphDirectSegments.top();
            auto reverseTop = graphReverseSegments.top();

            reverseSearch = roadPriorityComparator(
                directTop->_distanceFromStart, directTop->_distanceToEnd,
                reverseTop->_distanceFromStart, reverseTop->_distanceToEnd,
                0.5) > 0;
            if (graphDirectSegments.size() * 1.3 > graphReverseSegments.size())
                reverseSearch = true;
            else if (graphDirectSegments.size() < 1.3 * graphReverseSegments.size())
                reverseSearch = false;
        }
        else 
       {
            // different strategy : use unidirectional graph
            reverseSearch = context->owner->_planRoadDirection < 0;
        }

        pGraphSegments = reverseSearch ? &graphReverseSegments : &graphDirectSegments;

        // Check if route calculation has been aborted
        if (controller && controller->isAborted())
            return OsmAnd::RouteCalculationResult("Aborted");
    }

    if (!finalSegment)
        return OsmAnd::RouteCalculationResult("Route could not be calculated");
    printDebugInformation(context, graphDirectSegments.size(), graphReverseSegments.size(), finalSegment);

    return prepareResult(context, finalSegment, leftSideNavigation);
}

void OsmAnd::RoutePlanner::loadBorderPoints( OsmAnd::RoutePlannerContext::CalculationContext* context )
{
    AreaI bbox31;
    bbox31.left = std::min(context->_startPoint.x, context->_targetPoint.x);
    bbox31.right = std::max(context->_startPoint.x, context->_targetPoint.x);
    bbox31.top = std::min(context->_startPoint.y, context->_targetPoint.y);
    bbox31.bottom = std::max(context->_startPoint.y, context->_targetPoint.y);
    PlainQueryFilter filter(nullptr, &bbox31);
    
    //NOTE: one tile of 12th zoom around (?)
    const uint32_t zoomAround = 10;
    const uint32_t distAround = 1 << (31 - zoomAround);
    const auto leftBorderBoundary = bbox31.left - distAround;
    const auto rightBorderBoundary = bbox31.right + distAround;
    
    QMap<uint32_t, std::shared_ptr<RoutePlannerContext::BorderLine> > borderLinesLUT;
    for(const auto& entry : rangeOf(constOf(context->owner->_subsectionsContextsLUT)))
    {
        const auto& subsection = entry.key();
        const auto& source = context->owner->_sourcesLUT[subsection->section.get()];

        ObfRoutingSectionReader::loadSubsectionBorderBoxLinesPoints(source, subsection->section, nullptr, &filter, nullptr,
            [&] (const std::shared_ptr<const OsmAnd::ObfRoutingBorderLinePoint>& point)
            {
                if (point->location.x <= leftBorderBoundary || point->location.x >= rightBorderBoundary)
                    return false;

                if (!context->owner->profileContext->acceptsBorderLinePoint(subsection->section, point))
                    return false;

                auto itBorderLine = borderLinesLUT.constFind(point->location.y);
                if (itBorderLine == borderLinesLUT.cend())
                {
                    std::shared_ptr<RoutePlannerContext::BorderLine> line(new RoutePlannerContext::BorderLine());

                    line->_y31 = point->location.y;
                    
                    auto lft = point->bboxedClone(leftBorderBoundary);
                    line->_borderPoints.push_back(lft);
                    auto rht = point->bboxedClone(rightBorderBoundary);
                    line->_borderPoints.push_back(rht);
                    
                    context->_borderLines.push_back(line);
                    itBorderLine = borderLinesLUT.insert(line->_y31, line);
                }

                (*itBorderLine)->_borderPoints.push_back(point);

                return false;
            }
        );
    }

    qSort(context->_borderLines.begin(), context->_borderLines.end(), [](const std::shared_ptr<RoutePlannerContext::BorderLine>& l, const std::shared_ptr<RoutePlannerContext::BorderLine>& r) -> bool
    {
        return l->_y31 < r->_y31;
    });
    
    context->_borderLinesY31.reserve(context->_borderLines.size());
    for(const auto& line : constOf(context->_borderLines))
    {
        // FIXME borders approach
        // not less then 14th zoom
        /*TODO:if (i > 0 && borderLineCoordinates[i - 1] >> 17 == borderLineCoordinates[i] >> 17) {
            throw new IllegalStateException();
        }*/

        context->_borderLinesY31.push_back(line->_y31);
    }

    updateDistanceForBorderPoints(context, context->_startPoint, true);
    updateDistanceForBorderPoints(context, context->_targetPoint, false);
}

uint64_t OsmAnd::RoutePlanner::encodeRoutePointId( const std::shared_ptr<const Model::Road>& road, uint64_t pointIndex, bool positive )
{
    assert((pointIndex >> RoutePointsBitSpace) == 0);
    return (road->id << RoutePointsBitSpace) | (pointIndex << 1) | (positive ? 1 : 0);
}

uint64_t OsmAnd::RoutePlanner::encodeRoutePointId( const std::shared_ptr<const Model::Road>& road, uint64_t pointIndex)
{
    return (road->id << 10) | pointIndex;
}

float OsmAnd::RoutePlanner::estimateTimeDistance( OsmAnd::RoutePlannerContext::CalculationContext* context, const PointI& from, const PointI& to )
{
    auto distance = Utilities::distance31(from.x, from.y, to.x, to.y);
    return distance / context->owner->profileContext->profile->maxSpeed;
}

int OsmAnd::RoutePlanner::roadPriorityComparator( double aDistanceFromStart, double aDistanceToEnd, double bDistanceFromStart, double bDistanceToEnd, double heuristicCoefficient )
{
    return Utilities::javaDoubleCompare(
        aDistanceFromStart + heuristicCoefficient * aDistanceToEnd,
        bDistanceFromStart + heuristicCoefficient * bDistanceToEnd);
}

void OsmAnd::RoutePlanner::calculateRouteSegment(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    bool reverseWaySearch,
    RoadSegmentsPriorityQueue& graphSegments,
    QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& visitedSegments,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& segment,
    QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& oppositeSegments,
    bool forwardDirection )
{
    const bool initDirectionAllowed = checkIfInitialMovementAllowedOnSegment(context, reverseWaySearch, visitedSegments, segment, forwardDirection, segment->road);
    bool directionAllowed = initDirectionAllowed;
    
    // Go through all point of the way and find ways to continue
    //NOTE: ! Actually there is small bug when there is restriction to move forward on the way (it doesn't take into account)
    float obstaclesTime = 0.0f;
    if (segment->parent && directionAllowed)
    {
        obstaclesTime = calculateTurnTime(context,
            segment, forwardDirection ? segment->road->points.size() - 1 : 0,  
            segment->parent, segment->parentEndPointIndex);
    }

    if (context->_entranceRoadId == encodeRoutePointId(segment->road, segment->pointIndex, true))
    {
        if ( (forwardDirection && context->_entranceRoadDirection < 0) || (!forwardDirection && context->_entranceRoadDirection > 0) )
            obstaclesTime += 500;
    }

    float segmentDist = 0;
    // +/- diff from middle point
    auto segmentEnd = segment->pointIndex;
    while (directionAllowed)
    {
        if ((segmentEnd == 0 && !forwardDirection) || (segmentEnd + 1 >= segment->road->points.size() && forwardDirection))
        {
            directionAllowed = false;
            continue;
        }
        auto prevInd = forwardDirection ? segmentEnd++ : segmentEnd--;
        const auto intervalId = forwardDirection ? segmentEnd - 1 : segmentEnd;

        visitedSegments.insert(encodeRoutePointId(segment->road, intervalId, forwardDirection), segment);

        const auto& point = segment->road->points[segmentEnd];
        const auto& prevPoint = segment->road->points[prevInd];
        
        // causing bugs in first route calculation
        // if (point == prevPoint) continue;
        
        /*TODO:
        if (USE_BORDER_LINES) {
            int st = ctx.searchBorderLineIndex(y);
            int tt = ctx.searchBorderLineIndex(prevy);
            if (st != tt){
                //                    System.out.print(" " + st + " != " + tt + " " + road.id + " ? ");
                for(int i = Math.min(st, tt); i < Math.max(st, tt) & i < ctx.borderLines.length ; i++) {
                    Iterator<RouteDataBorderLinePoint> pnts = ctx.borderLines[i].borderPoints.iterator();
                    boolean changed = false;
                    while(pnts.hasNext()) {
                        RouteDataBorderLinePoint o = pnts.next();
                        if (o.id == road.id) {
                            //                                System.out.println("Point removed !");
                            pnts.remove();
                            changed = true;
                        }
                    }
                    if (changed){
                        ctx.updateDistanceForBorderPoints(ctx.startX, ctx.startY, true);
                        ctx.updateDistanceForBorderPoints(ctx.targetX, ctx.targetY, false);
                    }
                }
            }
        }*/

        // 2. calculate point and try to load neighbor ways if they are not loaded
        segmentDist += Utilities::distance31(
            point.x, point.y,
            prevPoint.x, prevPoint.y);

        // 2.1 calculate possible obstacle plus time
        auto obstacleTime = context->owner->profileContext->getRoutingObstaclesExtraTime(segment->road, segmentEnd);
        if (obstacleTime < 0)
        {
            directionAllowed = false;
            continue;
        }
        obstaclesTime += obstacleTime;

        const auto alreadyVisited = checkIfOppositeSegmentWasVisited(
            context, reverseWaySearch, graphSegments, segment, oppositeSegments, segment->road,
            segmentEnd, forwardDirection, intervalId, segmentDist, obstaclesTime);
        if (alreadyVisited)
        {
            directionAllowed = false;
            continue;
        }

        // could be expensive calculation
        // 3. get intersected ways
        auto nextSegment = loadRouteCalculationSegment(context->owner, point.x, point.y); // ctx.config.memoryLimitation - ctx.memoryOverhead
        if (!nextSegment) 
            continue;
        if ( (nextSegment == segment || nextSegment->road->id == segment->road->id) && !nextSegment->next )
            continue;
        
        // check if there are outgoing connections in that case we need to stop processing
        bool outgoingConnections = false;
        auto otherSegment = nextSegment;
        while(otherSegment)
        {
            if (otherSegment->road->id != segment->road->id || otherSegment->pointIndex != 0 || otherSegment->road->getDirection() != Model::RoadDirection::OneWayForward)
            {
                outgoingConnections = true;
                break;
            }

            otherSegment = otherSegment->next;
        }

        if (outgoingConnections)
            directionAllowed = false;

        float distStartObstacles = segment->_distanceFromStart + calculateTimeWithObstacles(context, segment->road, segmentDist, obstaclesTime);
        processIntersections(context, graphSegments, visitedSegments, 
            distStartObstacles, segment, segmentEnd, 
            nextSegment, reverseWaySearch, outgoingConnections);
    }

    /*
     *TODO
    if (initDirectionAllowed && ctx.visitor != null){
        ctx.visitor.visitSegment(segment, segmentEnd, true);
    }
    */
}

float OsmAnd::RoutePlanner::calculateTurnTime(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& a, uint32_t aEndPointIndex,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& b, uint32_t bEndPointIndex )
{
    auto itPointTypesB = b->road->pointsTypes.constFind(bEndPointIndex);
    if (itPointTypesB != b->road->pointsTypes.cend())
    {
        const auto& pointTypesB = *itPointTypesB;

        // Check that there are no traffic signals, since they don't add turn info
        const auto& encRules = b->road->subsection->section->_p->_encodingRules;
        for(const auto& pointType : constOf(pointTypesB))
        {
            const auto& rule = encRules[pointType];
            if (rule->_tag == "highway" && rule->_value == "traffic_signals")
                return 0;
        }
    }

    auto roundaboutTurnTime = context->owner->profileContext->profile->roundaboutTurn;
    if (roundaboutTurnTime > 0 && !b->road->isRoundabout() && a->road->isRoundabout())
        return roundaboutTurnTime;
    
    if (context->owner->profileContext->profile->leftTurn > 0 || context->owner->profileContext->profile->rightTurn > 0)
    {
        auto a1 = a->road->getDirectionDelta(a->pointIndex, a->pointIndex < aEndPointIndex);
        auto a2 = b->road->getDirectionDelta(bEndPointIndex, bEndPointIndex < b->pointIndex);
        auto diff = qAbs(Utilities::normalizedAngleRadians(a1 - a2 - M_PI));

        // more like UT
        if (diff > 2.0 * M_PI / 3.0)
        {
            return context->owner->profileContext->profile->leftTurn;
        }
        else if (diff > M_PI / 2.0)
        {
            return context->owner->profileContext->profile->rightTurn;
        }
        return 0;
    }

    return 0;
}

bool OsmAnd::RoutePlanner::checkIfInitialMovementAllowedOnSegment(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    bool reverseWaySearch, QMap<uint64_t,
    std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& visitedSegments,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& segment,
    bool forwardDirection,
    const std::shared_ptr<const Model::Road>& road )
{
    bool directionAllowed;

    const auto middle = segment->pointIndex;
    const auto direction = context->owner->profileContext->getDirection(road);

    // use positive direction as agreed
    if (!reverseWaySearch)
    {
        if (forwardDirection)
            directionAllowed = (direction == Model::RoadDirection::TwoWay || direction == Model::RoadDirection::OneWayReverse);
        else
            directionAllowed = (direction == Model::RoadDirection::TwoWay || direction == Model::RoadDirection::OneWayForward);
    }
    else
    {
        if (forwardDirection)
            directionAllowed = (direction == Model::RoadDirection::TwoWay || direction == Model::RoadDirection::OneWayForward);
        else
            directionAllowed = (direction == Model::RoadDirection::TwoWay || direction == Model::RoadDirection::OneWayReverse);
    }
    if (forwardDirection)
    {
        if (middle == road->points.size() - 1 || visitedSegments.contains(encodeRoutePointId(road, middle, true)) || segment->_allowedDirection == -1)
        {
            directionAllowed = false;
        }
    }
    else
    {
        if (middle == 0 || visitedSegments.contains(encodeRoutePointId(road, middle - 1, false)) || segment->_allowedDirection == 1)
        {
            directionAllowed = false;
        }
    }

    return directionAllowed;
}

bool OsmAnd::RoutePlanner::checkIfOppositeSegmentWasVisited(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    bool reverseWaySearch,
    RoadSegmentsPriorityQueue& graphSegments,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& segment,
    QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& oppositeSegments,
    const std::shared_ptr<const Model::Road>& road,
    uint32_t segmentEnd,
    bool forwardDirection,
    uint32_t intervalId,
    float segmentDist,
    float obstaclesTime)
{
    const auto id = encodeRoutePointId(road, intervalId, !forwardDirection);

    auto itOppositeSegment = oppositeSegments.constFind(id);
    if (itOppositeSegment == oppositeSegments.cend())
        return false;

    auto oppositeSegment = *itOppositeSegment;
    if (oppositeSegment->pointIndex != segmentEnd)
        return false;

    auto finalSegment = new RoutePlannerContext::RouteCalculationFinalSegment(road, segment->pointIndex);
    auto distStartObstacles = segment->_distanceFromStart + calculateTimeWithObstacles(context, road, segmentDist, obstaclesTime);
    finalSegment->_parent = segment->_parent;
    finalSegment->_parentEndPointIndex = segment->_parentEndPointIndex;
    finalSegment->_distanceFromStart = oppositeSegment->_distanceFromStart + distStartObstacles;
    finalSegment->_distanceToEnd = 0;
    finalSegment->_reverseWaySearch = reverseWaySearch;
    finalSegment->_opposite = oppositeSegment;

    graphSegments.push(std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>(
        dynamic_cast<RoutePlannerContext::RouteCalculationSegment*>(finalSegment)
    ));
    return true;
}

float OsmAnd::RoutePlanner::calculateTimeWithObstacles(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    const std::shared_ptr<const Model::Road>& road,
    float distOnRoadToPass,
    float obstaclesTime)
{
    auto priority = context->owner->profileContext->getSpeedPriority(road);
    auto speed = context->owner->profileContext->getSpeed(road) * priority;
    if (qFuzzyCompare(speed, 0.0f))
        speed = context->owner->profileContext->profile->defaultSpeed * priority;

    // Speed can not exceed max default speed according to A*
    if (speed > context->owner->profileContext->profile->maxSpeed)
        speed = context->owner->profileContext->profile->maxSpeed;

    auto distStartObstacles = obstaclesTime + distOnRoadToPass / speed;
    return distStartObstacles;
}

bool OsmAnd::RoutePlanner::processRestrictions(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    QList< std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& prescripted,
    const std::shared_ptr<const Model::Road>& road,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& inputNext,
    bool reverseWay)
{
    QList< std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> > notForbidden;

    auto exclusiveRestriction = false;
    auto next = inputNext;
    if (!reverseWay && road->restrictions.size() == 0)
        return false;
    
    if (!context->owner->profileContext->profile->restrictionsAware)
        return false;
    
    while(next)
    {
        Model::RoadRestriction type = Model::RoadRestriction::Invalid;
        if (!reverseWay)
        {
            auto itRestriction = road->restrictions.constFind(next->road->id);
            if (itRestriction != road->restrictions.cend())
                type = *itRestriction;
        }
        else
        {
            for(const auto& restriction : rangeOf(constOf(next->road->restrictions)))
            {
                const auto& restrictedTo = restriction.key();
                const auto& crt = restriction.value();

                if (restrictedTo == road->id)
                {
                    type = crt;
                    break;
                }

                // Check if there is restriction only to the other than current road
                if (crt == Model::RoadRestriction::OnlyRightTurn || crt == Model::RoadRestriction::OnlyLeftTurn || crt == Model::RoadRestriction::OnlyStraightOn)
                {
                    // check if that restriction applies to considered junction
                    auto foundNext = inputNext;
                    while(foundNext)
                    {
                        if (foundNext->road->id == restrictedTo)
                            break;

                        foundNext = foundNext->next;
                    }
                    if (foundNext)
                        type = Model::RoadRestriction::Special_ReverseWayOnly; // special constant
                }
            }
        }

        if (type == Model::RoadRestriction::Special_ReverseWayOnly)
        {
            // next = next.next; continue;
        }
        else if (type == Model::RoadRestriction::Invalid && exclusiveRestriction)
        {
            // next = next.next; continue;
        }
        else if (type == Model::RoadRestriction::NoLeftTurn || type == Model::RoadRestriction::NoRightTurn || type == Model::RoadRestriction::NoUTurn || type == Model::RoadRestriction::NoStraightOn)
        {
            // next = next.next; continue;
        }
        else if (type == Model::RoadRestriction::Invalid)
        {
            // case no restriction
            notForbidden.push_back(next);
        }
        else
        {
            // case exclusive restriction (only_right, only_straight, ...)
            // 1. in case we are going backward we should not consider only_restriction
            // as exclusive because we have many "in" roads and one "out"
            // 2. in case we are going forward we have one "in" and many "out"
            if (!reverseWay)
            {
                exclusiveRestriction = true;
                notForbidden.clear();
                prescripted.push_back(next);
            }
            else
            {
                notForbidden.push_back(next);
            }
        }
        next = next->next;
    }

    prescripted.append(qMove(notForbidden));
    return true;
}

void OsmAnd::RoutePlanner::processIntersections(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    RoadSegmentsPriorityQueue& graphSegments,
    QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& visitedSegments,
    float distFromStart,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& segment, 
    uint32_t segmentEnd,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& inputNext,
    bool reverseWaySearch,
    bool addSameRoadFutureDirection)
{
    auto searchDirection = reverseWaySearch ? -1 : 1;

    QList< std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> > prescripted;
    const auto restrictionsPresent = processRestrictions(context, prescripted, segment->road, inputNext, reverseWaySearch);
    auto itPrescripted = prescripted.cbegin();

#if TRACE_ROUTING
    if (restrictionsPresent)
        LogPrintf(LogSeverityLevel::Debug, "\t[Restrictions present]");
#endif

    // Calculate possible ways to put into priority queue
    if (restrictionsPresent ? itPrescripted == prescripted.cend() : !inputNext)
        return;
    auto current = restrictionsPresent ? *itPrescripted : inputNext;
    while(current)
    {
        auto nextPlusNotAllowed =
            (current->pointIndex == current->road->points.size() - 1) ||
            visitedSegments.contains(encodeRoutePointId(current->road, current->pointIndex, true));

        auto nextMinusNotAllowed =
            (current->pointIndex == 0) ||
            visitedSegments.contains(encodeRoutePointId(current->road, current->pointIndex - 1, false));

        auto sameRoadFutureDirection = current->road->id == segment->road->id && current->pointIndex == segmentEnd;

        // road->id could be equal on roundabout, but we should accept them
        auto alreadyVisited = nextPlusNotAllowed && nextMinusNotAllowed;
        auto skipRoad = sameRoadFutureDirection && !addSameRoadFutureDirection;
        if (!alreadyVisited && !skipRoad)
        {
            auto targetEnd = reverseWaySearch ? context->_startPoint : context->_targetPoint;
            
            auto distanceToEnd = h(context, segment->road->points[segmentEnd], targetEnd, current);
            
            // assigned to wrong direction
            if (current->_assignedDirection == -searchDirection)
                current.reset(new RoutePlannerContext::RouteCalculationSegment(current->road, current->pointIndex));

            if (!current->parent ||
                roadPriorityComparator(
                    current->_distanceFromStart, current->_distanceToEnd,
                    distFromStart, distanceToEnd,
                    context->owner->_heuristicCoefficient) > 0)
            {
                if (current->parent)
                {
                    //NOTE: This should be done somehow else (probably use different priority queue)
                    RoadSegmentsPriorityQueue filteredGraphSegments;
                    bool success = false;
                    while (graphSegments.size() > 0)
                    {
                        auto segment = graphSegments.top();
                        graphSegments.pop();

                        if (segment == current)
                        {
                            success = true;
                            continue;
                        }

                        filteredGraphSegments.push(segment);
                    }
                    OSMAND_ASSERT(success, "Should be handled by direction flag");
                    graphSegments = filteredGraphSegments;
                } 
                current->_assignedDirection = searchDirection;
                current->_distanceFromStart = distFromStart;
                current->_distanceToEnd = distanceToEnd;
                if (sameRoadFutureDirection)
                    current->_allowedDirection = segment->pointIndex < current->pointIndex ? 1 : - 1;

                // put additional information to recover whole route after
                current->_parent = segment;
                current->_parentEndPointIndex = segmentEnd;

#if TRACE_ROUTING
                current->dump("\t>> ");
#endif

                graphSegments.push(current);
            }
#if TRACE_ROUTING
            else
            {
                current->dump("\t!R ");
            }
#endif
            /*if (ctx.visitor != null) {
                //ctx.visitor.visitSegment(next, false);
            }*/
        }
        else if (!sameRoadFutureDirection)
        {
#if TRACE_ROUTING
            current->dump("\t!AlreadyVisited ");
#endif
            // the segment was already visited! We need to follow better route if it exists
            // that is very strange situation and almost exception (it can happen when we underestimate distnceToEnd)
            if (current->_assignedDirection == searchDirection && distFromStart < current->_distanceFromStart && current->road->id != segment->road->id)
            {
                OSMAND_ASSERT(context->owner->_heuristicCoefficient > 1, "distance from start..."); // "distance from start " + distFromStart + " < " + current->distanceFromStart
                
                // That code is incorrect (when segment is processed itself,
                // then it tries to make wrong u-turn) -
                // this situation should be very carefully checked in future (seems to be fixed)
                current->_distanceFromStart = distFromStart;
                current->_parent = segment;
                current->_parentEndPointIndex = segmentEnd;

                /*
                if (ctx.visitor != null) {
                    //                        ctx.visitor.visitSegment(next, false);
                }
                */
            }
        }
#if TRACE_ROUTING
        else
        {
            current->dump("\t!Skip ");
        }
#endif

        // Move to next
        if (restrictionsPresent)
        {
            ++itPrescripted;
            if (itPrescripted == prescripted.cend())
                break;
            current = *itPrescripted;
        }
        else
            current = current->next;
    }
}

bool OsmAnd::RoutePlanner::checkPartialRecalculationPossible(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& visitedOppositeSegments,
    std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& outSegment)
{
    if (context->owner->_previouslyCalculatedRoute.isEmpty() || qFuzzyCompare(context->owner->_partialRecalculationDistanceLimit, 0))
        return false;

    QList< std::shared_ptr<RouteSegment> > filteredPreviousRoute;
    auto threshold = 0.0f;
    for(const auto& prevRouteSegment : constOf(context->owner->_previouslyCalculatedRoute))
    {
        threshold += prevRouteSegment->distance;

        if (threshold > context->owner->_partialRecalculationDistanceLimit)
            filteredPreviousRoute.push_back(prevRouteSegment);
    }

    if (filteredPreviousRoute.isEmpty())
        return false;
    
    return false;
    /*
    TODO:
    std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> previousSegment;
    for(auto itPrevRouteSegment = filteredPreviousRoute.cbegin(); itPrevRouteSegment != filteredPreviousRoute.cend(); ++itPrevRouteSegment)
    {
        auto prevRouteSegment = *itPrevRouteSegment;

        std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> segment(new RoutePlannerContext::RouteCalculationSegment(prevRouteSegment.getObject(), prevRouteSegment.getEndPointIndex()));
        
        if (previousSegment)
        {
            previousSegment.setParentRoute(segment);
            previousSegment.setParentSegmentEnd(prevRouteSegment.getStartPointIndex());
            boolean positive = rr.getStartPointIndex() < rr.getEndPointIndex();
            long t = calculateRoutePointId(rr.getObject(), positive ? rr.getEndPointIndex() - 1 : rr.getEndPointIndex(),
                positive);
            visitedOppositeSegments.put(t, segment);
        }

        previousSegment = segment;
    }
    outSegment = previousSegment;
    return true;*/
}

void OsmAnd::RoutePlanner::updateDistanceForBorderPoints( RoutePlannerContext::CalculationContext* context, const PointI& sPoint, bool isDistanceToStart )
{
    const auto positive = !context->_borderLines.isEmpty() && sPoint.y < context->_borderLines.first()->_y31;

    /*if (borderLines.length > 0 && !plus && sy< borderLines[borderLines.length - 1].borderLine){
        throw new IllegalStateException();
    }*/

    auto itLine = positive ? context->_borderLines.cbegin() : context->_borderLines.cend() - 1;
    auto itPrevLine = itLine;
    for(auto c = 0; c < context->_borderLines.size(); c++, itPrevLine = positive ? itLine++ : itLine--)
    {
        const auto& line = *itLine;

        for(const auto& borderPoint : constOf(line->_borderPoints))
        {
            auto distance = Utilities::distance31(sPoint.x, sPoint.y, borderPoint->location.x, borderPoint->location.y);

            if (itLine != itPrevLine)
            {
                auto prevLine = *itPrevLine;

                for(const auto& prevLinePoint : constOf(prevLine->_borderPoints))
                {
                    auto d = Utilities::distance31(prevLinePoint->location.x, prevLinePoint->location.y, borderPoint->location.x, borderPoint->location.y) +
                        isDistanceToStart ? prevLinePoint->distanceToStartPoint : prevLinePoint->distanceToEndPoint;

                    if (d < distance)
                        distance = d;
                }
            }

            if (isDistanceToStart)
                borderPoint->distanceToStartPoint = distance;
            else
                borderPoint->distanceToEndPoint = distance;
        }
    }
}

std::shared_ptr<OsmAnd::RoutePlannerContext::RouteCalculationSegment> OsmAnd::RoutePlanner::loadRouteCalculationSegment(
    OsmAnd::RoutePlannerContext* context,
    uint32_t x31, uint32_t y31)
{
    auto tileId = getRoutingTileId(context, x31, y31, false);

    QMap<uint64_t, std::shared_ptr<const Model::Road> > processed;
    std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> original;

    const auto& itCachedRoads = context->_cachedRoadsInTiles.constFind(tileId);
    if (itCachedRoads != context->_cachedRoadsInTiles.cend())
    {
        const auto& cachedRoads = *itCachedRoads;
        for(const auto& road : constOf(cachedRoads))
        {
            uint32_t pointIdx = 0;
            for(auto itPoint = iteratorOf(constOf(road->points)); itPoint; ++itPoint, pointIdx++)
            {
                const auto& point = *itPoint;
                auto id = encodeRoutePointId(road, pointIdx);
                if (point.x != x31 || point.y != y31 || processed.contains(id))
                    continue;

                processed.insert(id, road);

                std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> segment(new RoutePlannerContext::RouteCalculationSegment(road, pointIdx));
                segment->_next = original;
                original = segment;
            }
        }
    }

    auto itSubregionsContexts = context->_indexedSubsectionsContexts.constFind(tileId);
    if (itSubregionsContexts != context->_indexedSubsectionsContexts.cend())
    {
        const auto& subregionsContexts = *itSubregionsContexts;
        for(const auto& subregionsContext : constOf(subregionsContexts))
            original = subregionsContext->loadRouteCalculationSegment(x31, y31, processed, original);
    }

    return original;
}

double OsmAnd::RoutePlanner::h(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    const PointI& start, const PointI& end,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& next )
{
    auto distanceToFinalPoint = Utilities::distance31(start.x, start.y, end.x, end.y);
    
    /*TODO:
    if (RoutingContext.USE_BORDER_LINES) {
        int begBorder = ctx.searchBorderLineIndex(begY);
        int endBorder = ctx.searchBorderLineIndex(endY);
        if (begBorder != endBorder) {
            double res = 0;
            boolean plus = begBorder < endBorder;
            boolean beginEqStart = begX == ctx.startX && begY == ctx.startY;
            boolean beginEqTarget = begX == ctx.targetX && begY == ctx.targetY;
            boolean endEqStart = endX == ctx.startX && endY == ctx.startY;
            boolean endEqTarget = endX == ctx.targetX && endY == ctx.targetY;
            if (endEqStart || endEqTarget) {
                // we start from intermediate point and end in target or start
                if (begX > ctx.leftBorderBoundary && begX < ctx.rightBorderBoundary) {
                    List<RouteDataBorderLinePoint> pnts = ctx.borderLines[plus ? begBorder : begBorder - 1].borderPoints;
                    for (RouteDataBorderLinePoint p : pnts) {
                        double f = (endEqTarget ? p.distanceToEndPoint : p.distanceToStartPoint) + squareRootDist(p.x, p.y, begX, begY);
                        if (res > f || res <= 0) {
                            res = f;
                        }
                    }
                }
            } else if (beginEqStart || beginEqTarget) {
                if (endX > ctx.leftBorderBoundary && endX < ctx.rightBorderBoundary) {
                    List<RouteDataBorderLinePoint> pnts = ctx.borderLines[plus ? endBorder - 1 : endBorder].borderPoints;
                    for (RouteDataBorderLinePoint p : pnts) {
                        double f = (beginEqTarget ? p.distanceToEndPoint : p.distanceToStartPoint)
                            + squareRootDist(p.x, p.y, endX, endY);
                        if (res > f || res <= 0) {
                            res = f;
                        }
                    }
                }
            } else { 
                throw new IllegalStateException();
            }
            if (res > 0) {
                if (res < distToFinalPoint - 0.01) {
                    throw new IllegalStateException("Estimated distance " + res + " > " + distToFinalPoint);
                }
                //                    if (endEqStart && res - distToFinalPoint > 13000) {
                //                        System.out.println(" Res="+res + " dist=" +distToFinalPoint);
                //                    }
                distToFinalPoint = res;

            } else {
                // FIXME put penalty
                //                    distToFinalPoint = distToFinalPoint;
            }
        }
    }
    */

    auto res = distanceToFinalPoint / context->owner->profileContext->profile->maxSpeed;
    return res;
}
