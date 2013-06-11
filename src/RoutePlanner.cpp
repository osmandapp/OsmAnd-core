#include "RoutePlanner.h"
#include "RoutePlannerRouteAnalyzer.h"

#include <queue>

#include <QtCore>

#include "ObfReader.h"
#include "Common.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::RoutePlanner::RoutePlanner()
{
}

OsmAnd::RoutePlanner::~RoutePlanner()
{
}

bool OsmAnd::RoutePlanner::findClosestRoadPoint(
    OsmAnd::RoutePlannerContext* context,
    double latitude, double longitude,
    std::shared_ptr<OsmAnd::Model::Road>* closestRoad /*= nullptr*/,
    uint32_t* closestPointIndex /*= nullptr*/,
    double* sqDistanceToClosestPoint /*= nullptr*/,
    uint32_t* _rx31 /*= nullptr*/, uint32_t* _ry31 /*= nullptr*/ )
{
    const auto x31 = Utilities::get31TileNumberX(longitude);
    const auto y31 = Utilities::get31TileNumberY(latitude);

    QList< std::shared_ptr<Model::Road> > roads;
    loadRoads(context, x31, y31, 17, roads);
    if(roads.isEmpty())
        loadRoads(context, x31, y31, 15, roads);

    std::shared_ptr<OsmAnd::Model::Road> minDistanceRoad;
    uint32_t minDistancePointIdx;
    double minSqDistance = std::numeric_limits<double>::max();
    uint32_t min31x, min31y;
    for(auto itRoad = roads.begin(); itRoad != roads.end(); ++itRoad)
    {
        auto road = *itRoad;
        if(road->points.size() <= 1)
            continue;

        for(auto idx = 1; idx < road->points.size(); idx++)
        {
            const auto& cpx31 = road->points[idx].x;
            const auto& cpy31 = road->points[idx].y;
            const auto& ppx31 = road->points[idx - 1].x;
            const auto& ppy31 = road->points[idx - 1].y;

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
        if(closestRoad)
            *closestRoad = minDistanceRoad;
        if(closestPointIndex)
            *closestPointIndex = minDistancePointIdx;
        if(sqDistanceToClosestPoint)
            *sqDistanceToClosestPoint = minSqDistance;
        if(_rx31)
            *_rx31 = min31x;
        if(_ry31)
            *_ry31 = min31y;
        return true;
    }
    return false;
}

bool OsmAnd::RoutePlanner::findClosestRouteSegment( OsmAnd::RoutePlannerContext* context, double latitude, double longitude, std::shared_ptr<OsmAnd::RoutePlannerContext::RouteCalculationSegment>& routeSegment )
{
    std::shared_ptr<OsmAnd::Model::Road> closestRoad;
    uint32_t closestPointIndex;
    uint32_t rx31, ry31;

    if(!findClosestRoadPoint(context, latitude, longitude, &closestRoad, &closestPointIndex, nullptr, &rx31, &ry31))
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
    if(!context->profileContext->acceptsRoad(road.get()))
        return;

    for(auto itPoint = road->points.begin(); itPoint != road->points.end(); ++itPoint)
    {
        const auto& px31 = itPoint->x;
        const auto& py31 = itPoint->y;

        const auto tileId = getRoutingTileId(context, px31, py31, true);

        auto itCache = context->_cachedRoadsInTiles.find(tileId);
        if(itCache == context->_cachedRoadsInTiles.end())
            itCache = context->_cachedRoadsInTiles.insert(tileId, QList< std::shared_ptr<Model::Road> >());

        if(!itCache->contains(road))
            itCache->push_back(road);
    }
}

void OsmAnd::RoutePlanner::loadRoads( RoutePlannerContext* context, uint32_t x31, uint32_t y31, uint32_t zoomAround, QList< std::shared_ptr<Model::Road> >& roads )
{
    auto coordinatesShift = 1 << (31 - context->_roadTilesLoadingZoomLevel);
    uint32_t t;
    if(zoomAround >= context->_roadTilesLoadingZoomLevel)
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
            if(processedTiles.contains(tileId))
                continue;
            loadRoadsFromTile(context, tileId, roads);
            processedTiles.insert(tileId);
        }
    }
}

void OsmAnd::RoutePlanner::loadRoadsFromTile( RoutePlannerContext* context, uint64_t tileId, QList< std::shared_ptr<Model::Road> >& roads )
{
    QMap<uint64_t, std::shared_ptr<Model::Road> > duplicates;

    auto itRoadsInTile = context->_cachedRoadsInTiles.find(tileId);
    if (itRoadsInTile != context->_cachedRoadsInTiles.end())
    {
        for(auto itRoad = itRoadsInTile->begin(); itRoad != itRoadsInTile->end(); ++itRoad)
        {
            auto road = *itRoad;
            if(duplicates.contains(road->id))
                continue;

            duplicates.insert(road->id, road);
            roads.push_back(road);
        }
    }

    auto itIndexedSubsectionContexts = context->_indexedSubsectionsContexts.find(tileId);
    assert(itIndexedSubsectionContexts != context->_indexedSubsectionsContexts.end());
    for(auto itSubsectionContext = itIndexedSubsectionContexts->begin(); itSubsectionContext != itIndexedSubsectionContexts->end(); ++itSubsectionContext)
    {
        auto subsectionContext = *itSubsectionContext;

        subsectionContext->collectRoads(roads, &duplicates);
    }
}

uint64_t OsmAnd::RoutePlanner::getRoutingTileId( RoutePlannerContext* context, uint32_t x31, uint32_t y31, bool dontLoad )
{
    auto xTileId = x31 >> (31 - context->_roadTilesLoadingZoomLevel);
    auto yTileId = y31 >> (31 - context->_roadTilesLoadingZoomLevel);
    uint64_t tileId = (xTileId << context->_roadTilesLoadingZoomLevel) + yTileId;
    if(dontLoad)
        return tileId;
    
    //TODO: perform GC if estimated size is close to limit
    /*if( memoryLimit == 0){
        memoryLimit = config.memoryLimitation;
    }
    if (getCurrentEstimatedSize() > 0.9 * memoryLimit) {
        int sz1 = getCurrentEstimatedSize();
        long h1 = 0;
        if (SHOW_GC_SIZE && sz1 > 0.7 * memoryLimit) {
            runGCUsedMemory();
            h1 = runGCUsedMemory();
        }
        int clt = getCurrentlyLoadedTiles();
        long us1 = (Runtime.getRuntime().totalMemory() - Runtime.getRuntime().freeMemory());
        unloadUnusedTiles(memoryLimit);
        if (h1 != 0 && getCurrentlyLoadedTiles() != clt) {
            int sz2 = getCurrentEstimatedSize();
            runGCUsedMemory();
            long h2 = runGCUsedMemory();
            float mb = (1 << 20);
            log.warn("Unload tiles :  estimated " + (sz1 - sz2) / mb + " ?= " + (h1 - h2) / mb + " actual");
            log.warn("Used after " + h2 / mb + " of " + Runtime.getRuntime().totalMemory() / mb + " max "
                + maxMemory() / mb);
        } else {
            float mb = (1 << 20);
            int sz2 = getCurrentEstimatedSize();
            log.warn("Unload tiles :  occupied before " + sz1 / mb + " Mb - now  " + sz2 / mb + "MB " + 
                memoryLimit/mb + " limit MB " + config.memoryLimitation/mb);
            long us2 = (Runtime.getRuntime().totalMemory() - Runtime.getRuntime().freeMemory());
            log.warn("Used memory before " + us1 / mb + "after " + us1 / mb + " of max " + maxMemory() / mb);
        }
    }*/

    auto itIndexedSubsectionContexts = context->_indexedSubsectionsContexts.find(tileId);
    if(itIndexedSubsectionContexts == context->_indexedSubsectionsContexts.end())
    {
        QList< std::shared_ptr<RoutePlannerContext::RoutingSubsectionContext> > subsectionContexts;
        loadTileHeader(context, x31, y31, subsectionContexts);
        itIndexedSubsectionContexts = context->_indexedSubsectionsContexts.insert(tileId, subsectionContexts);
    }

    assert(itIndexedSubsectionContexts != context->_indexedSubsectionsContexts.end());
    for(auto itSubsectionContext = itIndexedSubsectionContexts->begin(); itSubsectionContext != itIndexedSubsectionContexts->end(); ++itSubsectionContext)
    {
        auto subsectionContext = *itSubsectionContext;

        if(subsectionContext->isLoaded())continue;

        loadSubregionContext(subsectionContext.get());
    }

    return tileId;
}

void OsmAnd::RoutePlanner::loadTileHeader( RoutePlannerContext* context, uint32_t x31, uint32_t y31, QList< std::shared_ptr<RoutePlannerContext::RoutingSubsectionContext> >& subsectionsContexts )
{
    const auto zoomToLoad = 31 - context->_roadTilesLoadingZoomLevel;
    auto xTileId = x31 >> (31 - context->_roadTilesLoadingZoomLevel);
    auto yTileId = y31 >> (31 - context->_roadTilesLoadingZoomLevel);

    QueryFilter filter;
    AreaI bbox31;
    filter._bbox31 = &bbox31;
    bbox31.left = xTileId << zoomToLoad;
    bbox31.right = (xTileId + 1) << zoomToLoad;
    bbox31.top = yTileId << zoomToLoad;
    bbox31.bottom = (yTileId + 1) << zoomToLoad;

    for(auto itSource = context->sources.cbegin(); itSource != context->sources.cend(); ++itSource)
    {
        auto source = *itSource;
        
        for(auto itRoutingSection = source->routingSections.cbegin(); itRoutingSection != source->routingSections.cend(); ++itRoutingSection)
        {
            auto routingSection = *itRoutingSection;

            QList< std::shared_ptr<ObfRoutingSection::Subsection> > subsections;
            ObfRoutingSection::querySubsections(
                source.get(),
                context->_useBasemap ? routingSection->_baseSubsections : routingSection->_subsections,
                &subsections,
                &filter,
                [=] (std::shared_ptr<OsmAnd::ObfRoutingSection::Subsection> subsection)
                {
                    return subsection->containsData();
                }
            );
            for(auto itSubsection = subsections.begin(); itSubsection != subsections.end(); ++itSubsection)
            {
                auto itSubsectionContext = context->_subsectionsContextsLUT.find(itSubsection->get());
                if(itSubsectionContext == context->_subsectionsContextsLUT.end())
                {
                    std::shared_ptr<RoutePlannerContext::RoutingSubsectionContext> subsectionContext(new RoutePlannerContext::RoutingSubsectionContext(context, source, *itSubsection));
                    itSubsectionContext = context->_subsectionsContextsLUT.insert(itSubsection->get(), subsectionContext);
                    context->_subsectionsContexts.push_back(subsectionContext);
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
    /*TODO:
    long now = System.nanoTime();
    */
    context->markLoaded();
    ObfRoutingSection::loadSubsectionData(context->origin.get(), context->subsection, nullptr, nullptr, nullptr,
        [=] (std::shared_ptr<OsmAnd::Model::Road> road)
        {
            if(!context->owner->profileContext->acceptsRoad(road.get()))
                return false;

            context->registerRoad(road);
            return false;
        }
    );
    /*TODO:
    timeToLoad += (System.nanoTime() - now);
    loadedTiles++;
    if (wasUnloaded) {
        if(ucount == 1) {
            loadedPrevUnloadedTiles++;
        }
    } else {
        if(global != null) {
            global.allRoutes += ts.tileStatistics.allRoutes;
            global.coordinates += ts.tileStatistics.coordinates;
        }
        distinctLoadedTiles++;
    }
    global.size += ts.tileStatistics.size;
    */
}

bool OsmAnd::RoutePlanner::calculateRoute(
    OsmAnd::RoutePlannerContext* context,
    const QList< std::pair<double, double> >& points,
    bool leftSideNavigation,
    IQueryController* controller /*= nullptr*/,
    QList< std::shared_ptr<RouteSegment> >* outResult /*= nullptr*/ )
{
    assert(context != nullptr);
    assert(points.size() >= 2);

    /*
    if(ctx.calculationProgress == null) {
        ctx.calculationProgress = new RouteCalculationProgress();
    }
    */

    QList< std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> > routeCalculationSegments;
    for(auto itPoint = points.begin(); itPoint != points.end(); ++itPoint)
    {
        std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> segment;
        if(!findClosestRouteSegment(context, itPoint->first, itPoint->second, segment))
            return false;
        routeCalculationSegments.push_back(segment);
    }
    
    if(routeCalculationSegments.size() > 2)
    {
//        std::shared_ptr< QList< std::shared_ptr<RouteSegment> > > firstPartRecalculatedRoute;
//        QList< std::shared_ptr<RouteSegment> > restPartRecalculatedRoute;
//
//        if(context->_previouslyCalculatedRoute)
//        {
//            const auto id = routeCalculationSegments[1]->road->id;
//            const auto segmentStart = routeCalculationSegments[1]->pointIndex;
//            
//            int prevRouteSegmentIdx = 0;
//            for(auto itPrevRouteSegment = context->_previouslyCalculatedRoute->begin(); itPrevRouteSegment != context->_previouslyCalculatedRoute->end(); ++itPrevRouteSegment, prevRouteSegmentIdx++)
//            {
//                auto prevRouteSegment = *itPrevRouteSegment;
//                if(id != prevRouteSegment->getObject().getId() || segmentStart != prevRouteSegment->getEndPointIndex())
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
//        for(auto itRouteCalculationSegment = routeCalculationSegments.begin(); itRouteCalculationSegment != routeCalculationSegments.end(); ++itRouteCalculationSegment)
//        {
//            std::unique_ptr<RoutePlannerContext::CalculationContext> calculationContext(new RoutePlannerContext::CalculationContext(context));
//
//            if(itRouteCalculationSegment == routeCalculationSegments.begin() && firstPartRecalculatedRoute)
//                calculationContext->_previouslyCalculatedRoute = firstPartRecalculatedRoute;
//
////            local.calculationProgress = ctx.calculationProgress;
//            routeExists = calculateRoute(calculationContext.get(), *itRouteCalculationSegment, *(itRouteCalculationSegment + 1), leftSideNavigation, controller, outResult);
//            if(!routeExists)
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
//            if(context->_previouslyCalculatedRoute)
//            {
//                if(outResult)
//                    outResult->append(restPartRecalculatedRoute);
//                break;
//            }
//        }
//        //TODO: ctx.unloadAllData();
//        return routeExists;
        int i = 5;
    }

    std::unique_ptr<RoutePlannerContext::CalculationContext> calculationContext(new RoutePlannerContext::CalculationContext(context));
    return calculateRoute(calculationContext.get(), routeCalculationSegments[0], routeCalculationSegments[1], leftSideNavigation, controller, outResult);
}

bool OsmAnd::RoutePlanner::calculateRoute(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& from,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& to_,
    bool leftSideNavigation,
    IQueryController* controller /*= nullptr*/,
    QList< std::shared_ptr<RouteSegment> >* outResult /*= nullptr*/)
{
    /*
    if(ctx.SHOW_GC_SIZE){
        long h1 = ctx.runGCUsedMemory();
        float mb = (1 << 20);
        log.warn("Used before routing " + h1 / mb+ " actual");
    }
    */
    context->_startPoint = from->road->points[from->pointIndex];
    context->_targetPoint = to_->road->points[to_->pointIndex];
    /*
    refreshProgressDistance(ctx);
    */
    /*
    // measure time
    ctx.timeToLoad = 0;
    ctx.visitedSegments = 0;
    ctx.memoryOverhead  = 1000;
    ctx.timeToCalculate = System.nanoTime();
    */

    if (!qIsNaN(context->owner->_initialHeading))
    {
        // Mark here as positive for further check
        context->_entranceRoadId = encodeRoutePointId(from->road, from->pointIndex, true);

        auto roadDirectionDelta = from->road->getDirectionDelta(from->pointIndex, true);
        auto delta = roadDirectionDelta - context->owner->_initialHeading;

        if(qAbs(Utilities::normalizedAngleRadians(delta)) <= M_PI / 3.0)
            context->_entranceRoadDirection = 1;
        else if(qAbs(Utilities::normalizedAngleRadians(delta - M_PI)) <= M_PI / 3.0)
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

#if DEBUG_ROUTING || TRACE_ROUTING
    uint64_t iterations = 0;
#endif
    std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> finalSegment;
    while (!pGraphSegments->empty())
    {
#if TRACE_DUMP_QUEUE
        LogPrintf(LogSeverityLevel::Debug, "----------------------------------------\n");
        LogPrintf(LogSeverityLevel::Debug, "%s-Queue (%d):\n", reverseSearch ? "R" : "D", pGraphSegments->size());
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

        /*
        // use accumulative approach
        ctx.memoryOverhead = (visitedDirectSegments.size() + visitedOppositeSegments.size()) * STANDARD_ROAD_VISITED_OVERHEAD + 
            (graphDirectSegments.size() +
            graphReverseSegments.size()) * STANDARD_ROAD_IN_QUEUE_OVERHEAD;
*/
        
        if(dynamic_cast<RoutePlannerContext::RouteCalculationFinalSegment*>(segment.get()))
        {
            /*if(RoutingContext.SHOW_GC_SIZE){
                log.warn("Estimated overhead " + (ctx.memoryOverhead / (1<<20))+ " mb");
                printMemoryConsumption("Memory occupied after calculation : ");
            }*/
            finalSegment = segment;
            break;
        }

#if DEBUG_ROUTING || TRACE_ROUTING
        iterations++;
#endif

        /*if (ctx.memoryOverhead > ctx.config.memoryLimitation * 0.95 && RoutingContext.SHOW_GC_SIZE) {
            printMemoryConsumption("Memory occupied before exception : ");
        }
        if(ctx.memoryOverhead > ctx.config.memoryLimitation * 0.95) {
            throw new IllegalStateException("There is no enough memory " + ctx.config.memoryLimitation/(1<<20) + " Mb");
        }
        ctx.visitedSegments++;*/

#if TRACE_ROUTING
        LogPrintf(LogSeverityLevel::Debug, "\tFwd-search:\n");
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
        LogPrintf(LogSeverityLevel::Debug, "\tRev-search:\n");
#endif
        calculateRouteSegment(
            context,
            reverseSearch,
            reverseSearch ? graphReverseSegments : graphDirectSegments,
            reverseSearch ? visitedOppositeSegments : visitedDirectSegments,
            segment,
            reverseSearch ? visitedDirectSegments : visitedOppositeSegments,
            false);
        
        /*
        updateCalculationProgress(ctx, graphDirectSegments, graphReverseSegments);
        */

        if(graphDirectSegments.empty() || graphReverseSegments.empty())
            return false;
        
        if(runRecalculation)
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
        if(controller && controller->isAborted())
            return false;
    }

    if(!finalSegment)
        return false;
#if DEBUG_ROUTING || TRACE_ROUTING
    LogPrintf(LogSeverityLevel::Debug, "%llu iterations\n", iterations);
    LogPrintf(LogSeverityLevel::Debug, "D-Queue size %d, R-Queue size %d\n", graphDirectSegments.size(), graphReverseSegments.size());
#endif
    /*
    printDebugMemoryInformation(ctx, graphDirectSegments, graphReverseSegments, visitedDirectSegments, visitedOppositeSegments);
    */
    

    return OsmAnd::RoutePlannerAnalyzer::prepareResult(context, finalSegment, outResult, leftSideNavigation);
}

void OsmAnd::RoutePlanner::loadBorderPoints( OsmAnd::RoutePlannerContext::CalculationContext* context )
{
    QueryFilter filter;
    AreaI bbox31;
    filter._bbox31 = &bbox31;
    bbox31.left = std::min(context->_startPoint.x, context->_targetPoint.x);
    bbox31.right = std::max(context->_startPoint.x, context->_targetPoint.x);
    bbox31.top = std::min(context->_startPoint.y, context->_targetPoint.y);
    bbox31.bottom = std::max(context->_startPoint.y, context->_targetPoint.y);
    
    //NOTE: one tile of 12th zoom around (?)
    const uint32_t zoomAround = 10;
    const uint32_t distAround = 1 << (31 - zoomAround);
    const auto leftBorderBoundary = bbox31.left - distAround;
    const auto rightBorderBoundary = bbox31.right + distAround;
    
    QMap<uint32_t, std::shared_ptr<RoutePlannerContext::BorderLine> > borderLinesLUT;
    for(auto itEntry = context->owner->_subsectionsContextsLUT.begin(); itEntry != context->owner->_subsectionsContextsLUT.end(); ++itEntry)
    {
        auto subsection = itEntry.key();
        auto source = context->owner->_sourcesLUT[subsection->section.get()];

        ObfRoutingSection::loadSubsectionBorderBoxLinesPoints(source.get(), subsection->section.get(), nullptr, &filter, nullptr,
            [&] (std::shared_ptr<OsmAnd::ObfRoutingSection::BorderLinePoint> point)
            {
                if(point->location.x <= leftBorderBoundary || point->location.x >= rightBorderBoundary)
                    return false;

                if(!context->owner->profileContext->acceptsBorderLinePoint(subsection->section.get(), point.get()))
                    return false;

                auto itBorderLine = borderLinesLUT.find(point->location.y);
                if(itBorderLine == borderLinesLUT.end())
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
    for(auto itLine = context->_borderLines.begin(); itLine != context->_borderLines.end(); ++itLine)
    {
        auto line = *itLine;

        // FIXME borders approach
        // not less then 14th zoom
        /*TODO:if(i > 0 && borderLineCoordinates[i - 1] >> 17 == borderLineCoordinates[i] >> 17) {
            throw new IllegalStateException();
        }*/

        context->_borderLinesY31.push_back(line->_y31);
    }

    updateDistanceForBorderPoints(context, context->_startPoint, true);
    updateDistanceForBorderPoints(context, context->_targetPoint, false);
}

uint64_t OsmAnd::RoutePlanner::encodeRoutePointId( const std::shared_ptr<Model::Road>& road, uint64_t pointIndex, bool positive )
{
    assert((pointIndex >> RoutePointsBitSpace) == 0);
    return (road->id << RoutePointsBitSpace) | (pointIndex << 1) | (positive ? 1 : 0);
}

uint64_t OsmAnd::RoutePlanner::encodeRoutePointId( const std::shared_ptr<Model::Road>& road, uint64_t pointIndex)
{
    return (road->id << 10) | pointIndex;
}

float OsmAnd::RoutePlanner::estimateTimeDistance( OsmAnd::RoutePlannerContext::CalculationContext* context, const PointI& from, const PointI& to )
{
    auto distance = Utilities::distance31(from.x, from.y, to.x, to.y);
    return distance / context->owner->profileContext->profile->maxDefaultSpeed;
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
    if(segment->parent && directionAllowed)
    {
        obstaclesTime = calculateTurnTime(context,
            segment, forwardDirection ? segment->road->points.size() - 1 : 0,  
            segment->parent, segment->parentEndPointIndex);
    }

    if(context->_entranceRoadId == encodeRoutePointId(segment->road, segment->pointIndex, true))
    {
        if( (forwardDirection && context->_entranceRoadDirection < 0) || (!forwardDirection && context->_entranceRoadDirection > 0) )
            obstaclesTime += 500;
    }

    float segmentDist = 0;
    // +/- diff from middle point
    auto segmentEnd = segment->pointIndex;
    while (directionAllowed)
    {
        if((segmentEnd == 0 && !forwardDirection) || (segmentEnd + 1 >= segment->road->points.size() && forwardDirection))
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
        // if(point == prevPoint) continue;
        
        /*TODO:
        if(USE_BORDER_LINES) {
            int st = ctx.searchBorderLineIndex(y);
            int tt = ctx.searchBorderLineIndex(prevy);
            if(st != tt){
                //					System.out.print(" " + st + " != " + tt + " " + road.id + " ? ");
                for(int i = Math.min(st, tt); i < Math.max(st, tt) & i < ctx.borderLines.length ; i++) {
                    Iterator<RouteDataBorderLinePoint> pnts = ctx.borderLines[i].borderPoints.iterator();
                    boolean changed = false;
                    while(pnts.hasNext()) {
                        RouteDataBorderLinePoint o = pnts.next();
                        if(o.id == road.id) {
                            //								System.out.println("Point removed !");
                            pnts.remove();
                            changed = true;
                        }
                    }
                    if(changed){
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
        auto obstacleTime = context->owner->profileContext->getRoutingObstaclesExtraTime(segment->road.get(), segmentEnd);
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
        if(!nextSegment) 
            continue;
        if( (nextSegment == segment || nextSegment->road->id == segment->road->id) && !nextSegment->next )
            continue;
        
        // check if there are outgoing connections in that case we need to stop processing
        bool outgoingConnections = false;
        auto otherSegment = nextSegment;
        while(otherSegment)
        {
            if(otherSegment->road->id != segment->road->id || otherSegment->pointIndex != 0 || otherSegment->road->getDirection() != Model::Road::OneWayForward)
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
    if(initDirectionAllowed && ctx.visitor != null){
        ctx.visitor.visitSegment(segment, segmentEnd, true);
    }
    */
}

float OsmAnd::RoutePlanner::calculateTurnTime(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& a, uint32_t aEndPointIndex,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& b, uint32_t bEndPointIndex )
{
    auto itPointTypesB = b->road->pointsTypes.find(bEndPointIndex);
    if(itPointTypesB != b->road->pointsTypes.end())
    {
        const auto& pointTypesB = *itPointTypesB;

        // Check that there are no traffic signals, since they don't add turn info
        for(auto itPointType = pointTypesB.begin(); itPointType != pointTypesB.end(); ++itPointType)
        {
            auto rule = b->road->subsection->section->_encodingRules[*itPointType];
            if(rule->_tag == "highway" && rule->_value == "traffic_signals")
                return 0;
        }
    }

    auto roundaboutTurnTime = context->owner->profileContext->profile->roundaboutTurn;
    if(roundaboutTurnTime > 0 && !b->road->isRoundabout() && a->road->isRoundabout())
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
    const std::shared_ptr<Model::Road>& road )
{
    bool directionAllowed;

    const auto middle = segment->pointIndex;
    const auto direction = context->owner->profileContext->getDirection(road.get());

    // use positive direction as agreed
    if (!reverseWaySearch)
    {
        if(forwardDirection)
            directionAllowed = (direction == Model::Road::Direction::TwoWay || direction == Model::Road::Direction::OneWayReverse);
        else
            directionAllowed = (direction == Model::Road::Direction::TwoWay || direction == Model::Road::Direction::OneWayForward);
    }
    else
    {
        if(forwardDirection)
            directionAllowed = (direction == Model::Road::Direction::TwoWay || direction == Model::Road::Direction::OneWayForward);
        else
            directionAllowed = (direction == Model::Road::Direction::TwoWay || direction == Model::Road::Direction::OneWayReverse);
    }
    if(forwardDirection)
    {
        if(middle == road->points.size() - 1 || visitedSegments.contains(encodeRoutePointId(road, middle, true)) || segment->_allowedDirection == -1)
        {
            directionAllowed = false;
        }
    }
    else
    {
        if(middle == 0 || visitedSegments.contains(encodeRoutePointId(road, middle - 1, false)) || segment->_allowedDirection == 1)
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
    const std::shared_ptr<Model::Road>& road,
    uint32_t segmentEnd,
    bool forwardDirection,
    uint32_t intervalId,
    float segmentDist,
    float obstaclesTime)
{
    const auto id = encodeRoutePointId(road, intervalId, !forwardDirection);

    auto itOppositeSegment = oppositeSegments.find(id);
    if(itOppositeSegment == oppositeSegments.end())
        return false;

    auto oppositeSegment = *itOppositeSegment;
    if(oppositeSegment->pointIndex != segmentEnd)
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
    const std::shared_ptr<Model::Road>& road,
    float distOnRoadToPass,
    float obstaclesTime)
{
    auto priority = context->owner->profileContext->getSpeedPriority(road.get());
    auto speed = context->owner->profileContext->getSpeed(road.get()) * priority;
    if(qFuzzyCompare(speed, 0.0f))
        speed = context->owner->profileContext->profile->minDefaultSpeed * priority;

    // Speed can not exceed max default speed according to A*
    if(speed > context->owner->profileContext->profile->maxDefaultSpeed)
        speed = context->owner->profileContext->profile->maxDefaultSpeed;

    auto distStartObstacles = obstaclesTime + distOnRoadToPass / speed;
    return distStartObstacles;
}

bool OsmAnd::RoutePlanner::processRestrictions(
    OsmAnd::RoutePlannerContext::CalculationContext* context,
    QList< std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& prescripted,
    const std::shared_ptr<Model::Road>& road,
    const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& inputNext,
    bool reverseWay)
{
    QList< std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> > notForbidden;

    auto exclusiveRestriction = false;
    auto next = inputNext;
    if (!reverseWay && road->restrictions.size() == 0)
        return false;
    
    if(!context->owner->profileContext->profile->restrictionsAware)
        return false;
    
    while(next)
    {
        Model::Road::Restriction type = Model::Road::Invalid;
        if (!reverseWay)
        {
            auto itRestriction = road->restrictions.find(next->road->id);
            if(itRestriction != road->restrictions.end())
                type = *itRestriction;
        }
        else
        {
            for(auto itRestriction = next->road->restrictions.begin(); itRestriction != next->road->restrictions.end(); ++itRestriction)
            {
                auto restrictedTo = itRestriction.key();
                auto crt = itRestriction.value();

                if (restrictedTo == road->id)
                {
                    type = crt;
                    break;
                }

                // Check if there is restriction only to the other than current road
                if (crt == Model::Road::OnlyRightTurn || crt == Model::Road::OnlyLeftTurn || crt == Model::Road::OnlyStraightOn)
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
                        type = Model::Road::Special_ReverseWayOnly; // special constant
                }
            }
        }

        if (type == Model::Road::Special_ReverseWayOnly)
        {
            // next = next.next; continue;
        }
        else if (type == Model::Road::Invalid && exclusiveRestriction)
        {
            // next = next.next; continue;
        }
        else if (type == Model::Road::NoLeftTurn || type == Model::Road::NoRightTurn || type == Model::Road::NoUTurn || type == Model::Road::NoStraightOn)
        {
            // next = next.next; continue;
        }
        else if (type == Model::Road::Invalid)
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

    prescripted.append(notForbidden);
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
    auto itPrescripted = prescripted.begin();

#if TRACE_ROUTING
    if(restrictionsPresent)
        LogPrintf(LogSeverityLevel::Debug, "\t[Restrictions present]\n");
#endif

    // Calculate possible ways to put into priority queue
    if(restrictionsPresent ? itPrescripted == prescripted.end() : !inputNext)
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
            if(current->_assignedDirection == -searchDirection)
                current.reset(new RoutePlannerContext::RouteCalculationSegment(current->road, current->pointIndex));

            if(!current->parent ||
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

                        if(segment == current)
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
                if(sameRoadFutureDirection)
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
        else if(!sameRoadFutureDirection)
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
                    //						ctx.visitor.visitSegment(next, false);
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
        if(restrictionsPresent)
        {
            ++itPrescripted;
            if(itPrescripted == prescripted.end())
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
    if(context->owner->_previouslyCalculatedRoute.isEmpty() || qFuzzyCompare(context->owner->_partialRecalculationDistanceLimit, 0))
        return false;

    QList< std::shared_ptr<RouteSegment> > filteredPreviousRoute;
    auto threshold = 0.0f;
    for(auto itPrevRouteSegment = context->owner->_previouslyCalculatedRoute.begin(); itPrevRouteSegment != context->owner->_previouslyCalculatedRoute.end(); ++itPrevRouteSegment)
    {
        auto prevRouteSegment = *itPrevRouteSegment;

        threshold += prevRouteSegment->distance;

        if(threshold > context->owner->_partialRecalculationDistanceLimit)
            filteredPreviousRoute.push_back(prevRouteSegment);
    }

    if(filteredPreviousRoute.isEmpty())
        return false;
    
    return false;
    /*
    TODO:
    std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> previousSegment;
    for(auto itPrevRouteSegment = filteredPreviousRoute.begin(); itPrevRouteSegment != filteredPreviousRoute.end(); ++itPrevRouteSegment)
    {
        auto prevRouteSegment = *itPrevRouteSegment;

        std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> segment(new RoutePlannerContext::RouteCalculationSegment(prevRouteSegment.getObject(), prevRouteSegment.getEndPointIndex()));
        
        if(previousSegment)
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

    /*TODO:if(borderLines.length > 0 && !plus && sy< borderLines[borderLines.length - 1].borderLine){
        throw new IllegalStateException();
    }*/

    auto itLine = positive ? context->_borderLines.begin() : context->_borderLines.end() - 1;
    auto itPrevLine = itLine;
    for(auto c = 0; c < context->_borderLines.size(); c++, itPrevLine = positive ? itLine++ : itLine--)
    {
        auto line = *itLine;

        for(auto itPoint = line->_borderPoints.begin(); itPoint != line->_borderPoints.end(); ++itPoint)
        {
            auto borderPoint = *itPoint;

            auto distance = Utilities::distance31(sPoint.x, sPoint.y, borderPoint->location.x, borderPoint->location.y);

            if(itLine != itPrevLine)
            {
                auto prevLine = *itPrevLine;

                for(auto itPrevLinePoint = prevLine->_borderPoints.begin(); itPrevLinePoint != prevLine->_borderPoints.end(); ++itPrevLinePoint)
                {
                    auto prevLinePoint = *itPrevLinePoint;

                    auto d = Utilities::distance31(prevLinePoint->location.x, prevLinePoint->location.y, borderPoint->location.x, borderPoint->location.y) +
                        isDistanceToStart ? prevLinePoint->distanceToStartPoint : prevLinePoint->distanceToEndPoint;

                    if(d < distance)
                        distance = d;
                }
            }

            if(isDistanceToStart)
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

    QMap<uint64_t, std::shared_ptr<Model::Road> > processed;
    std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> original;

    const auto& itCachedRoads = context->_cachedRoadsInTiles.find(tileId);
    if(itCachedRoads != context->_cachedRoadsInTiles.end())
    {
        const auto& cachedRoads = *itCachedRoads;
        for(auto itRoad = cachedRoads.begin(); itRoad != cachedRoads.end(); ++itRoad)
        {
            auto road = *itRoad;

            uint32_t pointIdx = 0;
            for(auto itPoint = road->points.begin(); itPoint != road->points.end(); ++itPoint, pointIdx++)
            {
                const auto& point = *itPoint;
                auto id = encodeRoutePointId(road, pointIdx);
                if(point.x != x31 || point.y != y31 || processed.contains(id))
                    continue;

                processed.insert(id, road);

                std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> segment(new RoutePlannerContext::RouteCalculationSegment(road, pointIdx));
                segment->_next = original;
                original = segment;
            }
        }
    }

    auto itSubregionsContexts = context->_indexedSubsectionsContexts.find(tileId);
    if(itSubregionsContexts != context->_indexedSubsectionsContexts.end())
    {
        const auto& subregionsContexts = *itSubregionsContexts;
        for(auto itSubregionsContext = subregionsContexts.begin(); itSubregionsContext != subregionsContexts.end(); ++itSubregionsContext)
        {
            auto subregionsContext = *itSubregionsContext;
            original = subregionsContext->loadRouteCalculationSegment(x31, y31, processed, original);
        }
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
            if(endEqStart || endEqTarget) {
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
            } else if(beginEqStart || beginEqTarget) {
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
            if(res > 0) {
                if(res < distToFinalPoint - 0.01) {
                    throw new IllegalStateException("Estimated distance " + res + " > " + distToFinalPoint);
                }
                //					if(endEqStart && res - distToFinalPoint > 13000) {
                //						System.out.println(" Res="+res + " dist=" +distToFinalPoint);
                //					}
                distToFinalPoint = res;

            } else {
                // FIXME put penalty
                //					distToFinalPoint = distToFinalPoint;
            }
        }
    }
    */

    auto res = distanceToFinalPoint / context->owner->profileContext->profile->maxDefaultSpeed;
    return res;
}
