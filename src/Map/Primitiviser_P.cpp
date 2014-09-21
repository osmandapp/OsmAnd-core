#include "Primitiviser_P.h"
#include "Primitiviser.h"

#include "stdlib_common.h"
#include <proper/future.h>
#include <set>

#include "QtExtensions.h"
#include "QtCommon.h"

#include "ICU.h"
#include "MapStyleEvaluator.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleBuiltinValueDefinitions.h"
#include "ObfMapSectionInfo.h"
#include "BinaryMapObject.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "QKeyValueIterator.h"
#include "QCachingIterator.h"
#include "Logging.h"

OsmAnd::Primitiviser_P::Primitiviser_P(Primitiviser* const owner_)
    : owner(owner_)
{
}

OsmAnd::Primitiviser_P::~Primitiviser_P()
{
}

std::shared_ptr<const OsmAnd::Primitiviser_P::PrimitivisedArea> OsmAnd::Primitiviser_P::primitivise(
    const AreaI area31,
    const PointI sizeInPixels,
    const ZoomLevel zoom,
    const MapFoundationType foundation,
    const QList< std::shared_ptr<const Model::BinaryMapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const IQueryController* const controller,
    Primitiviser_Metrics::Metric_primitivise* const metric)
{
    const Stopwatch totalStopwatch(metric != nullptr);

    const std::shared_ptr<PrimitivisedArea> primitivisedArea(new PrimitivisedArea(area31, sizeInPixels, zoom, cache, owner->environment));
    applyEnvironment(owner->environment, primitivisedArea);
    
    const Stopwatch objectsSortingStopwatch(metric != nullptr);

    // Split input map objects to object, coastline, basemapObjects and basemapCoastline
    QList< std::shared_ptr<const Model::BinaryMapObject> > detailedmapMapObjects, detailedmapCoastlineObjects, basemapMapObjects, basemapCoastlineObjects;
    QList< std::shared_ptr<const Model::BinaryMapObject> > polygonizedCoastlineObjects;
    for (const auto& mapObject : constOf(objects))
    {
        if (controller && controller->isAborted())
            break;

        const auto& mapObjectSection = mapObject->section;
        const auto isFromBasemap = mapObjectSection->isBasemap;
        if (zoom < static_cast<ZoomLevel>(BasemapZoom) && !isFromBasemap)
            continue;

        if (mapObject->containsType(mapObjectSection->encodingDecodingRules->naturalCoastline_encodingRuleId))
        {
            if (isFromBasemap)
                basemapCoastlineObjects.push_back(mapObject);
            else
                detailedmapCoastlineObjects.push_back(mapObject);
        }
        else
        {
            if (isFromBasemap)
                basemapMapObjects.push_back(mapObject);
            else
                detailedmapMapObjects.push_back(mapObject);
        }
    }
    if (controller && controller->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTimeForSortingObjects += objectsSortingStopwatch.elapsed();

    const Stopwatch polygonizeCoastlinesStopwatch(metric != nullptr);

    // Polygonize coastlines
    bool fillEntireArea = true;
    bool addBasemapCoastlines = true;
    const bool detailedLandData = (zoom >= static_cast<ZoomLevel>(DetailedLandDataZoom)) && !detailedmapMapObjects.isEmpty();
    if (!detailedmapCoastlineObjects.empty())
    {
        const bool coastlinesWereAdded = polygonizeCoastlines(
            owner->environment,
            primitivisedArea,
            detailedmapCoastlineObjects,
            polygonizedCoastlineObjects,
            !basemapCoastlineObjects.isEmpty(),
            true);
        fillEntireArea = !coastlinesWereAdded && fillEntireArea;
        addBasemapCoastlines = (!coastlinesWereAdded && !detailedLandData) || zoom <= static_cast<ZoomLevel>(BasemapZoom);
    }
    else
    {
        addBasemapCoastlines = !detailedLandData;
    }
    if (addBasemapCoastlines)
    {
        const bool coastlinesWereAdded = polygonizeCoastlines(
            owner->environment,
            primitivisedArea,
            basemapCoastlineObjects,
            polygonizedCoastlineObjects,
            false,
            true);
        fillEntireArea = !coastlinesWereAdded && fillEntireArea;
    }

    if (metric)
    {
        metric->elapsedTimeForPolygonizingCoastlines += polygonizeCoastlinesStopwatch.elapsed();
        metric->polygonizedCoastlines += polygonizedCoastlineObjects.size();
    }

    if (basemapMapObjects.isEmpty() && detailedmapMapObjects.isEmpty() && foundation == MapFoundationType::Undefined)
    {
        // Empty area
        assert(primitivisedArea->isEmpty());
        return primitivisedArea;
    }

    if (fillEntireArea)
    {
        const auto& encDecRules = owner->environment->dummyMapSection->encodingDecodingRules;
        const std::shared_ptr<Model::BinaryMapObject> bgMapObject(new Model::BinaryMapObject(owner->environment->dummyMapSection, nullptr));
        bgMapObject->_isArea = true;
        bgMapObject->_points31.push_back(qMove(PointI(area31.left(), area31.top())));
        bgMapObject->_points31.push_back(qMove(PointI(area31.right(), area31.top())));
        bgMapObject->_points31.push_back(qMove(PointI(area31.right(), area31.bottom())));
        bgMapObject->_points31.push_back(qMove(PointI(area31.left(), area31.bottom())));
        bgMapObject->_points31.push_back(bgMapObject->_points31.first());
        if (foundation == MapFoundationType::FullWater)
            bgMapObject->_typesRuleIds.push_back(encDecRules->naturalCoastline_encodingRuleId);
        else if (foundation == MapFoundationType::FullLand || foundation == MapFoundationType::Mixed)
            bgMapObject->_typesRuleIds.push_back(encDecRules->naturalLand_encodingRuleId);
        else // if (foundation == MapFoundationType::Undefined)
        {
            LogPrintf(LogSeverityLevel::Warning, "Area [%d, %d, %d, %d]@%d has undefined foundation type",
                area31.top(),
                area31.left(),
                area31.bottom(),
                area31.right(),
                zoom);

            bgMapObject->_isArea = false;
            bgMapObject->_typesRuleIds.push_back(encDecRules->naturalCoastlineBroken_encodingRuleId);
        }
        bgMapObject->_extraTypesRuleIds.push_back(encDecRules->layerLowest_encodingRuleId);

        assert(bgMapObject->isClosedFigure());
        polygonizedCoastlineObjects.push_back(qMove(bgMapObject));
    }

    // Obtain primitives
    const bool detailedDataMissing = (zoom > static_cast<ZoomLevel>(BasemapZoom)) && detailedmapMapObjects.isEmpty() && detailedmapCoastlineObjects.isEmpty();

    // Check if there is no data to render. Report, clean-up and exit
    const auto mapObjectsCount =
        detailedmapMapObjects.size() +
        ((zoom <= static_cast<ZoomLevel>(BasemapZoom) || detailedDataMissing) ? basemapMapObjects.size() : 0) +
        polygonizedCoastlineObjects.size();
    if (mapObjectsCount == 0)
    {
        // Empty area
        assert(primitivisedArea->isEmpty());
        return primitivisedArea;
    }

    // Obtain primitives:
    MapStyleEvaluationResult evaluationResult;

    const Stopwatch obtainPrimitivesFromDetailedmapStopwatch(metric != nullptr);
    obtainPrimitives(owner->environment, primitivisedArea, detailedmapMapObjects, qMove(evaluationResult), cache, controller, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesFromDetailedmap += obtainPrimitivesFromDetailedmapStopwatch.elapsed();

    if ((zoom <= static_cast<ZoomLevel>(BasemapZoom)) || detailedDataMissing)
    {
        const Stopwatch obtainPrimitivesFromBasemapStopwatch(metric != nullptr);
        obtainPrimitives(owner->environment, primitivisedArea, basemapMapObjects, qMove(evaluationResult), cache, controller, metric);
        if (metric)
            metric->elapsedTimeForObtainingPrimitivesFromBasemap += obtainPrimitivesFromBasemapStopwatch.elapsed();
    }

    const Stopwatch obtainPrimitivesFromCoastlinesStopwatch(metric != nullptr);
    obtainPrimitives(owner->environment, primitivisedArea, polygonizedCoastlineObjects, qMove(evaluationResult), cache, controller, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesFromCoastlines += obtainPrimitivesFromCoastlinesStopwatch.elapsed();

    if (controller && controller->isAborted())
        return nullptr;

    // Sort and filter primitives
    const Stopwatch sortAndFilterPrimitivesStopwatch(metric != nullptr);
    sortAndFilterPrimitives(primitivisedArea);
    if (metric)
        metric->elapsedTimeForSortingAndFilteringPrimitives += sortAndFilterPrimitivesStopwatch.elapsed();

    // Obtain symbols from primitives
    const Stopwatch obtainPrimitivesSymbolsStopwatch(metric != nullptr);
    obtainPrimitivesSymbols(owner->environment, primitivisedArea, qMove(evaluationResult), cache, controller);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesSymbols += obtainPrimitivesSymbolsStopwatch.elapsed();

    // Cleanup if aborted
    if (controller && controller->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    return primitivisedArea;
}

void OsmAnd::Primitiviser_P::applyEnvironment(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<PrimitivisedArea>& primitivisedArea)
{
    primitivisedArea->defaultBackgroundColor = env->getDefaultBackgroundColor(primitivisedArea->zoom);
    env->obtainShadowRenderingOptions(primitivisedArea->zoom, primitivisedArea->shadowRenderingMode, primitivisedArea->shadowRenderingColor);
    primitivisedArea->polygonAreaMinimalThreshold = env->getPolygonAreaMinimalThreshold(primitivisedArea->zoom);
    primitivisedArea->roadDensityZoomTile = env->getRoadDensityZoomTile(primitivisedArea->zoom);
    primitivisedArea->roadsDensityLimitPerTile = env->getRoadsDensityLimitPerTile(primitivisedArea->zoom);
    primitivisedArea->shadowLevelMin = MapPresentationEnvironment::DefaultShadowLevelMin;
    primitivisedArea->shadowLevelMax = MapPresentationEnvironment::DefaultShadowLevelMax;
    const auto tileDivisor = Utilities::getPowZoom(31 - primitivisedArea->zoom);
    primitivisedArea->scale31ToPixelDivisor.x = tileDivisor / static_cast<double>(primitivisedArea->sizeInPixels.x);
    primitivisedArea->scale31ToPixelDivisor.y = tileDivisor / static_cast<double>(primitivisedArea->sizeInPixels.y);
}

bool OsmAnd::Primitiviser_P::polygonizeCoastlines(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<const PrimitivisedArea>& primitivisedArea,
    const QList< std::shared_ptr<const Model::BinaryMapObject> >& coastlines,
    QList< std::shared_ptr<const Model::BinaryMapObject> >& outVectorized,
    bool abortIfBrokenCoastlinesExist,
    bool includeBrokenCoastlines)
{
    QList< QVector< PointI > > closedPolygons;
    QList< QVector< PointI > > coastlinePolylines; // Broken == not closed in this case

    // Align area to 32: this fixes coastlines and specifically Antarctica
    const auto area31 = primitivisedArea->area31;
    auto alignedArea31 = primitivisedArea->area31;
    alignedArea31.top() &= ~((1u << 5) - 1);
    alignedArea31.left() &= ~((1u << 5) - 1);
    alignedArea31.bottom() &= ~((1u << 5) - 1);
    alignedArea31.right() &= ~((1u << 5) - 1);

    uint64_t osmId = 0;
    QVector< PointI > linePoints31;
    for (const auto& coastline : constOf(coastlines))
    {
        if (coastline->points31.size() < 2)
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Map object #%" PRIu64 " (%" PRIi64 ") is primitivised as coastline, but has %d vertices",
                coastline->id >> 1, static_cast<int64_t>(coastline->id) / 2,
                coastline->points31.size());
            continue;
        }

        osmId = coastline->id >> 1;
        linePoints31.clear();
        auto itPoint = coastline->points31.cbegin();
        auto pp = *itPoint;
        auto cp = pp;
        auto prevInside = alignedArea31.contains(cp);
        if (prevInside)
            linePoints31.push_back(cp);
        const auto itEnd = coastline->points31.cend();
        for (++itPoint; itPoint != itEnd; ++itPoint)
        {
            cp = *itPoint;

            const auto inside = alignedArea31.contains(cp);
            const auto lineEnded = buildCoastlinePolygonSegment(env, primitivisedArea, inside, cp, prevInside, pp, linePoints31);
            if (lineEnded)
            {
                appendCoastlinePolygons(closedPolygons, coastlinePolylines, linePoints31);

                // Create new line if it goes outside
                linePoints31.clear();
            }

            pp = cp;
            prevInside = inside;
        }

        appendCoastlinePolygons(closedPolygons, coastlinePolylines, linePoints31);
    }

    if (closedPolygons.isEmpty() && coastlinePolylines.isEmpty())
        return false;

    // Draw coastlines
    for (const auto& polyline : constOf(coastlinePolylines))
    {
        const std::shared_ptr<Model::BinaryMapObject> mapObject(new Model::BinaryMapObject(env->dummyMapSection, nullptr));
        mapObject->_isArea = false;
        mapObject->_points31 = polyline;
        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastlineLine_encodingRuleId);

        outVectorized.push_back(qMove(mapObject));
    }

    const bool coastlineCrossesBounds = !coastlinePolylines.isEmpty();
    if (!coastlinePolylines.isEmpty())
    {
        // Add complete water tile with holes
        const std::shared_ptr<Model::BinaryMapObject> mapObject(new Model::BinaryMapObject(env->dummyMapSection, nullptr));
        mapObject->_points31.push_back(qMove(PointI(area31.left(), area31.top())));
        mapObject->_points31.push_back(qMove(PointI(area31.right(), area31.top())));
        mapObject->_points31.push_back(qMove(PointI(area31.right(), area31.bottom())));
        mapObject->_points31.push_back(qMove(PointI(area31.left(), area31.bottom())));
        mapObject->_points31.push_back(mapObject->_points31.first());
        convertCoastlinePolylinesToPolygons(env, primitivisedArea, coastlinePolylines, mapObject->_innerPolygonsPoints31, osmId);

        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
        mapObject->_id = osmId;
        mapObject->_isArea = true;

        assert(mapObject->isClosedFigure());
        assert(mapObject->isClosedFigure(true));
        outVectorized.push_back(qMove(mapObject));
    }

    if (!coastlinePolylines.isEmpty())
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Invalid polylines found during primitivisation of coastlines in area [%d, %d, %d, %d]@%d",
            primitivisedArea->area31.top(),
            primitivisedArea->area31.left(),
            primitivisedArea->area31.bottom(),
            primitivisedArea->area31.right(),
            primitivisedArea->zoom);
    }

    if (includeBrokenCoastlines)
    {
        for (const auto& polygon : constOf(coastlinePolylines))
        {
            const std::shared_ptr<Model::BinaryMapObject> mapObject(new Model::BinaryMapObject(env->dummyMapSection, nullptr));
            mapObject->_isArea = false;
            mapObject->_points31 = polygon;
            mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastlineBroken_encodingRuleId);

            outVectorized.push_back(qMove(mapObject));
        }
    }

    // Draw coastlines
    for (const auto& polygon : constOf(closedPolygons))
    {
        const std::shared_ptr<Model::BinaryMapObject> mapObject(new Model::BinaryMapObject(env->dummyMapSection, nullptr));
        mapObject->_isArea = false;
        mapObject->_points31 = polygon;
        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastlineLine_encodingRuleId);

        outVectorized.push_back(qMove(mapObject));
    }

    if (abortIfBrokenCoastlinesExist && !coastlinePolylines.isEmpty())
        return false;

    auto fullWaterObjects = 0u;
    auto fullLandObjects = 0u;
    for (const auto& polygon : constOf(closedPolygons))
    {
        // If polygon has less than 4 points, it's invalid
        if (polygon.size() < 4)
            continue;

        bool clockwise = isClockwiseCoastlinePolygon(polygon);

        const std::shared_ptr<Model::BinaryMapObject> mapObject(new Model::BinaryMapObject(env->dummyMapSection, nullptr));
        mapObject->_points31 = qMove(polygon);
        if (clockwise)
        {
            mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
            fullWaterObjects++;
        }
        else
        {
            mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalLand_encodingRuleId);
            fullLandObjects++;
        }
        mapObject->_id = osmId;
        mapObject->_isArea = true;

        assert(mapObject->isClosedFigure());
        outVectorized.push_back(qMove(mapObject));
    }

    if (fullWaterObjects == 0u && !coastlineCrossesBounds)
    {
        LogPrintf(LogSeverityLevel::Warning, "Isolated islands found during primitivisation of coastlines in area [%d, %d, %d, %d]@%d",
            primitivisedArea->area31.top(),
            primitivisedArea->area31.left(),
            primitivisedArea->area31.bottom(),
            primitivisedArea->area31.right(),
            primitivisedArea->zoom);

        // Add complete water tile
        const std::shared_ptr<Model::BinaryMapObject> mapObject(new Model::BinaryMapObject(env->dummyMapSection, nullptr));
        mapObject->_points31.push_back(qMove(PointI(area31.left(), area31.top())));
        mapObject->_points31.push_back(qMove(PointI(area31.right(), area31.top())));
        mapObject->_points31.push_back(qMove(PointI(area31.right(), area31.bottom())));
        mapObject->_points31.push_back(qMove(PointI(area31.left(), area31.bottom())));
        mapObject->_points31.push_back(mapObject->_points31.first());

        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
        mapObject->_id = osmId;
        mapObject->_isArea = true;

        assert(mapObject->isClosedFigure());
        outVectorized.push_back(qMove(mapObject));
    }

    return true;
}

bool OsmAnd::Primitiviser_P::buildCoastlinePolygonSegment(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<const PrimitivisedArea>& primitivisedArea,
    bool currentInside,
    const PointI& currentPoint31,
    bool prevInside,
    const PointI& previousPoint31,
    QVector< PointI >& segmentPoints)
{
    bool lineEnded = false;

    // Align area to 32: this fixes coastlines and specifically Antarctica
    auto alignedArea31 = primitivisedArea->area31;
    alignedArea31.top() &= ~((1u << 5) - 1);
    alignedArea31.left() &= ~((1u << 5) - 1);
    alignedArea31.bottom() &= ~((1u << 5) - 1);
    alignedArea31.right() &= ~((1u << 5) - 1);

    auto point = currentPoint31;
    if (prevInside)
    {
        if (!currentInside)
        {
            bool hasIntersection = calculateIntersection(currentPoint31, previousPoint31, alignedArea31, point);
            if (!hasIntersection)
                point = previousPoint31;
            segmentPoints.push_back(point);
            lineEnded = true;
        }
        else
        {
            segmentPoints.push_back(point);
        }
    }
    else
    {
        bool hasIntersection = calculateIntersection(currentPoint31, previousPoint31, alignedArea31, point);
        if (currentInside)
        {
            assert(hasIntersection);
            segmentPoints.push_back(point);
            segmentPoints.push_back(currentPoint31);
        }
        else if (hasIntersection)
        {
            segmentPoints.push_back(point);
            calculateIntersection(currentPoint31, point, alignedArea31, point);
            segmentPoints.push_back(point);
            lineEnded = true;
        }
    }

    return lineEnded;
}

bool OsmAnd::Primitiviser_P::calculateIntersection(const PointI& p1, const PointI& p0, const AreaI& bbox, PointI& pX)
{
    // Calculates intersection between line and bbox in clockwise manner.

    // Well, since Victor said not to touch this thing, I will replace only output writing,
    // and will even retain old variable names.
    const auto& px = p0.x;
    const auto& py = p0.y;
    const auto& x = p1.x;
    const auto& y = p1.y;
    const auto& leftX = bbox.left();
    const auto& rightX = bbox.right();
    const auto& topY = bbox.top();
    const auto& bottomY = bbox.bottom();

    // firstly try to search if the line goes in
    if (py < topY && y >= topY) {
        int tx = (int)(px + ((double)(x - px) * (topY - py)) / (y - py));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;//b.first = tx;
            pX.y = topY;//b.second = topY;
            return true;
        }
    }
    if (py > bottomY && y <= bottomY) {
        int tx = (int)(px + ((double)(x - px) * (py - bottomY)) / (py - y));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;//b.first = tx;
            pX.y = bottomY;//b.second = bottomY;
            return true;
        }
    }
    if (px < leftX && x >= leftX) {
        int ty = (int)(py + ((double)(y - py) * (leftX - px)) / (x - px));
        if (ty >= topY && ty <= bottomY) {
            pX.x = leftX;//b.first = leftX;
            pX.y = ty;//b.second = ty;
            return true;
        }

    }
    if (px > rightX && x <= rightX) {
        int ty = (int)(py + ((double)(y - py) * (px - rightX)) / (px - x));
        if (ty >= topY && ty <= bottomY) {
            pX.x = rightX;//b.first = rightX;
            pX.y = ty;//b.second = ty;
            return true;
        }

    }

    // try to search if point goes out
    if (py > topY && y <= topY) {
        int tx = (int)(px + ((double)(x - px) * (topY - py)) / (y - py));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;//b.first = tx;
            pX.y = topY;//b.second = topY;
            return true;
        }
    }
    if (py < bottomY && y >= bottomY) {
        int tx = (int)(px + ((double)(x - px) * (py - bottomY)) / (py - y));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;//b.first = tx;
            pX.y = bottomY;//b.second = bottomY;
            return true;
        }
    }
    if (px > leftX && x <= leftX) {
        int ty = (int)(py + ((double)(y - py) * (leftX - px)) / (x - px));
        if (ty >= topY && ty <= bottomY) {
            pX.x = leftX;//b.first = leftX;
            pX.y = ty;//b.second = ty;
            return true;
        }

    }
    if (px < rightX && x >= rightX) {
        int ty = (int)(py + ((double)(y - py) * (px - rightX)) / (px - x));
        if (ty >= topY && ty <= bottomY) {
            pX.x = rightX;//b.first = rightX;
            pX.y = ty;//b.second = ty;
            return true;
        }

    }

    if (px == rightX || px == leftX || py == topY || py == bottomY) {
        pX = p0;//b.first = px; b.second = py;
        //		return true;
        // Is it right? to not return anything?
    }
    return false;
}

void OsmAnd::Primitiviser_P::appendCoastlinePolygons(
    QList< QVector< PointI > >& closedPolygons,
    QList< QVector< PointI > >& coastlinePolylines,
    QVector< PointI >& polyline)
{
    if (polyline.isEmpty())
        return;

    if (polyline.first() == polyline.last())
    {
        closedPolygons.push_back(polyline);
        return;
    }

    bool add = true;

    for (auto itPolygon = cachingIteratorOf(coastlinePolylines); itPolygon;)
    {
        auto& polygon = *itPolygon;

        bool remove = false;

        if (polyline.first() == polygon.last())
        {
            polygon.reserve(polygon.size() + polyline.size() - 1);
            polygon.pop_back();
            polygon += polyline;
            remove = true;
            polyline = polygon;
        }
        else if (polyline.last() == polygon.first())
        {
            polyline.reserve(polyline.size() + polygon.size() - 1);
            polyline.pop_back();
            polyline += polygon;
            remove = true;
        }

        if (remove)
        {
            itPolygon.set(coastlinePolylines.erase(itPolygon.current));
            itPolygon.update(coastlinePolylines);
        }
        else
        {
            ++itPolygon;
        }

        if (polyline.first() == polyline.last())
        {
            closedPolygons.push_back(polyline);
            add = false;
            break;
        }
    }

    if (add)
    {
        coastlinePolylines.push_back(polyline);
    }
}

void OsmAnd::Primitiviser_P::convertCoastlinePolylinesToPolygons(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<const PrimitivisedArea>& primitivisedArea,
    QList< QVector< PointI > >& coastlinePolylines,
    QList< QVector< PointI > >& coastlinePolygons,
    uint64_t osmId)
{
    // Align area to 32: this fixes coastlines and specifically Antarctica
    auto alignedArea31 = primitivisedArea->area31;
    alignedArea31.top() &= ~((1u << 5) - 1);
    alignedArea31.left() &= ~((1u << 5) - 1);
    alignedArea31.bottom() &= ~((1u << 5) - 1);
    alignedArea31.right() &= ~((1u << 5) - 1);

    QList< QVector< PointI > > validPolylines;

    // Check if polylines has been cut by primitivisation area
    auto itPolyline = mutableIteratorOf(coastlinePolylines);
    while (itPolyline.hasNext())
    {
        const auto& polyline = itPolyline.next();
        assert(!polyline.isEmpty());

        const auto& head = polyline.first();
        const auto& tail = polyline.last();

        // This curve has not been cut by primitivisation area, so it's
        // impossible to fix it
        if (!alignedArea31.isOnEdge(head) || !alignedArea31.isOnEdge(tail))
            continue;

        validPolylines.push_back(polyline);
        itPolyline.remove();
    }

    std::set< QList< QVector< PointI > >::iterator > processedPolylines;
    while (processedPolylines.size() != validPolylines.size())
    {
        for (auto itPolyline = cachingIteratorOf(validPolylines); itPolyline; ++itPolyline)
        {
            // If this polyline was already processed, skip it
            if (processedPolylines.find(itPolyline.current) != processedPolylines.end())
                continue;

            // Start from tail of the polyline and search for it's continuation in CCV order
            auto& polyline = *itPolyline;
            const auto& tail = polyline.last();
            auto tailEdge = Edge::Invalid;
            alignedArea31.isOnEdge(tail, &tailEdge);
            auto itNearestPolyline = itPolyline.getEnd();
            auto firstIteration = true;
            for (int idx = static_cast<int>(tailEdge)+4; (idx >= static_cast<int>(tailEdge)) && (!itNearestPolyline); idx--, firstIteration = false)
            {
                const auto currentEdge = static_cast<Edge>(idx % 4);

                for (auto itOtherPolyline = cachingIteratorOf(validPolylines); itOtherPolyline; ++itOtherPolyline)
                {
                    // If this polyline was already processed, skip it
                    if (processedPolylines.find(itOtherPolyline.current) != processedPolylines.end())
                        continue;

                    // Skip polylines that are on other edges
                    const auto& otherHead = itOtherPolyline->first();
                    auto otherHeadEdge = Edge::Invalid;
                    alignedArea31.isOnEdge(otherHead, &otherHeadEdge);
                    if (otherHeadEdge != currentEdge)
                        continue;

                    // Skip polyline that is not next in CCV order
                    if (firstIteration)
                    {
                        bool isNextByCCV = false;
                        if (currentEdge == Edge::Top)
                            isNextByCCV = (otherHead.x <= tail.x);
                        else if (currentEdge == Edge::Right)
                            isNextByCCV = (otherHead.y <= tail.y);
                        else if (currentEdge == Edge::Bottom)
                            isNextByCCV = (tail.x <= otherHead.x);
                        else if (currentEdge == Edge::Left)
                            isNextByCCV = (tail.y <= otherHead.y);
                        if (!isNextByCCV)
                            continue;
                    }

                    // If nearest was not yet set, set this
                    if (!itNearestPolyline)
                    {
                        itNearestPolyline = itOtherPolyline;
                        continue;
                    }

                    // Check if current polyline's head is closer (by CCV) that previously selected
                    const auto& previouslySelectedHead = itNearestPolyline->first();
                    bool isCloserByCCV = false;
                    if (currentEdge == Edge::Top)
                        isCloserByCCV = (otherHead.x > previouslySelectedHead.x);
                    else if (currentEdge == Edge::Right)
                        isCloserByCCV = (otherHead.y > previouslySelectedHead.y);
                    else if (currentEdge == Edge::Bottom)
                        isCloserByCCV = (otherHead.x < previouslySelectedHead.x);
                    else if (currentEdge == Edge::Left)
                        isCloserByCCV = (otherHead.y < previouslySelectedHead.y);

                    // If closer-by-CCV, then select this
                    if (isCloserByCCV)
                        itNearestPolyline = itOtherPolyline;
                }
            }
            assert(itNearestPolyline /* means '!= end' */);

            // Get edge of nearest-by-CCV head
            auto nearestHeadEdge = Edge::Invalid;
            const auto& nearestHead = itNearestPolyline->first();
            alignedArea31.isOnEdge(nearestHead, &nearestHeadEdge);

            // Fill by edges of area, if required
            int loopShift = 0;
            if (static_cast<int>(tailEdge)-static_cast<int>(nearestHeadEdge) < 0)
                loopShift = 4;
            else if (tailEdge == nearestHeadEdge)
            {
                bool skipAddingSides = false;
                if (tailEdge == Edge::Top)
                    skipAddingSides = (tail.x >= nearestHead.x);
                else if (tailEdge == Edge::Right)
                    skipAddingSides = (tail.y >= nearestHead.y);
                else if (tailEdge == Edge::Bottom)
                    skipAddingSides = (tail.x <= nearestHead.x);
                else if (tailEdge == Edge::Left)
                    skipAddingSides = (tail.y <= nearestHead.y);

                if (!skipAddingSides)
                    loopShift = 4;
            }
            for (int idx = static_cast<int>(tailEdge)+loopShift; idx > static_cast<int>(nearestHeadEdge); idx--)
            {
                const auto side = static_cast<Edge>(idx % 4);
                PointI p;

                if (side == Edge::Top)
                {
                    p.y = alignedArea31.top();
                    p.x = alignedArea31.left();
                }
                else if (side == Edge::Right)
                {
                    p.y = alignedArea31.top();
                    p.x = alignedArea31.right();
                }
                else if (side == Edge::Bottom)
                {
                    p.y = alignedArea31.bottom();
                    p.x = alignedArea31.right();
                }
                else if (side == Edge::Left)
                {
                    p.y = alignedArea31.bottom();
                    p.x = alignedArea31.left();
                }

                polyline.push_back(qMove(p));
            }

            // If nearest-by-CCV is head of current polyline, cap it and add to polygons, ...
            if (itNearestPolyline == itPolyline)
            {
                polyline.push_back(polyline.first());
                coastlinePolygons.push_back(polyline);
            }
            // ... otherwise join them. Joined will never be visited, and current will remain unmarked as processed
            else
            {
                const auto& otherPolyline = *itNearestPolyline;
                polyline << otherPolyline;
            }

            // After we've selected nearest-by-CCV polyline, mark it as processed
            processedPolylines.insert(itNearestPolyline.current);
        }
    }
}

bool OsmAnd::Primitiviser_P::isClockwiseCoastlinePolygon(const QVector< PointI > & polygon)
{
    if (polygon.isEmpty())
        return true;

    // calculate middle Y
    int64_t middleY = 0;
    for (const auto& vertex : constOf(polygon))
        middleY += vertex.y;
    middleY /= polygon.size();

    double clockwiseSum = 0;

    bool firstDirectionUp = false;
    int previousX = INT_MIN;
    int firstX = INT_MIN;

    auto itPrevVertex = polygon.cbegin();
    auto itVertex = itPrevVertex + 1;
    for (const auto itEnd = polygon.cend(); itVertex != itEnd; itPrevVertex = itVertex, ++itVertex)
    {
        const auto& vertex0 = *itPrevVertex;
        const auto& vertex1 = *itVertex;

        int32_t rX;
        if (!Utilities::rayIntersectX(vertex0, vertex1, middleY, rX))
            continue;

        bool skipSameSide = (vertex1.y <= middleY) == (vertex0.y <= middleY);
        if (skipSameSide)
            continue;

        bool directionUp = vertex0.y >= middleY;
        if (firstX == INT_MIN) {
            firstDirectionUp = directionUp;
            firstX = rX;
        }
        else {
            bool clockwise = (!directionUp) == (previousX < rX);
            if (clockwise) {
                clockwiseSum += abs(previousX - rX);
            }
            else {
                clockwiseSum -= abs(previousX - rX);
            }
        }
        previousX = rX;
    }

    if (firstX != INT_MIN) {
        bool clockwise = (!firstDirectionUp) == (previousX < firstX);
        if (clockwise) {
            clockwiseSum += qAbs(previousX - firstX);
        }
        else {
            clockwiseSum -= qAbs(previousX - firstX);
        }
    }

    return clockwiseSum >= 0;
}

void OsmAnd::Primitiviser_P::obtainPrimitives(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<PrimitivisedArea>& primitivisedArea,
    const QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> >& source,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    const std::shared_ptr<Cache>& cache,
    const IQueryController* const controller,
    Primitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto zoom = primitivisedArea->zoom;

    // Initialize shared settings for order evaluation
    MapStyleEvaluator orderEvaluator(env->resolvedStyle, env->displayDensityFactor);
    env->applyTo(orderEvaluator);
    orderEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    orderEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    // Initialize shared settings for polygon evaluation
    MapStyleEvaluator polygonEvaluator(env->resolvedStyle, env->displayDensityFactor);
    env->applyTo(polygonEvaluator);
    polygonEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    polygonEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    // Initialize shared settings for polyline evaluation
    MapStyleEvaluator polylineEvaluator(env->resolvedStyle, env->displayDensityFactor);
    env->applyTo(polylineEvaluator);
    polylineEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    polylineEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    // Initialize shared settings for point evaluation
    MapStyleEvaluator pointEvaluator(env->resolvedStyle, env->displayDensityFactor);
    env->applyTo(pointEvaluator);
    pointEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    pointEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    const auto pSharedPrimitivesGroups = cache ? cache->getPrimitivesGroupsPtr(zoom) : nullptr;
    QList< proper::shared_future< std::shared_ptr<const PrimitivesGroup> > > futureSharedPrimitivesGroups;
    for (const auto& mapObject : constOf(source))
    {
        if (controller && controller->isAborted())
            return;

        const auto canBeShared = (mapObject->section != env->dummyMapSection);

        // If group can be shared, use already-processed or reserve pending
        if (pSharedPrimitivesGroups && canBeShared)
        {
            // If this group was already processed, use that
            std::shared_ptr<const PrimitivesGroup> group;
            proper::shared_future< std::shared_ptr<const PrimitivesGroup> > futureGroup;
            if (pSharedPrimitivesGroups->obtainReferenceOrFutureReferenceOrMakePromise(mapObject->id, group, futureGroup))
            {
                if (group)
                {
                    // Add polygons, polylines and points from group to current context
                    primitivisedArea->polygons.append(group->polygons);
                    primitivisedArea->polylines.append(group->polylines);
                    primitivisedArea->points.append(group->points);
                    
                    // Add shared group to current context
                    primitivisedArea->primitivesGroups.push_back(qMove(group));
                }
                else
                {
                    futureSharedPrimitivesGroups.push_back(qMove(futureGroup));
                }

                continue;
            }
        }

        // Create a primitives group
        const Stopwatch obtainPrimitivesGroupStopwatch(metric != nullptr);
        const auto group = obtainPrimitivesGroup(
            env,
            primitivisedArea,
            mapObject,
            qMove(evaluationResult),
            orderEvaluator,
            polygonEvaluator,
            polylineEvaluator,
            pointEvaluator,
            metric);
        if (metric)
            metric->elapsedTimeForObtainingPrimitivesGroups += obtainPrimitivesGroupStopwatch.elapsed();

        // Add this group to shared cache
        if (pSharedPrimitivesGroups && canBeShared)
            pSharedPrimitivesGroups->fulfilPromiseAndReference(mapObject->id, group);

        // Add polygons, polylines and points from group to current context
        primitivisedArea->polygons.append(group->polygons);
        primitivisedArea->polylines.append(group->polylines);
        primitivisedArea->points.append(group->points);

        // Empty groups are also inserted, to indicate that they are empty
        primitivisedArea->primitivesGroups.push_back(qMove(group));
    }

    // Wait for future primitives groups
    Stopwatch futureSharedPrimitivesGroupsStopwatch(metric != nullptr);
    for (auto& futureSharedGroup : futureSharedPrimitivesGroups)
    {
        auto group = futureSharedGroup.get();

        // Add polygons, polylines and points from group to current context
        primitivisedArea->polygons.append(group->polygons);
        primitivisedArea->polylines.append(group->polylines);
        primitivisedArea->points.append(group->points);

        // Add shared group to current context
        primitivisedArea->primitivesGroups.push_back(qMove(group));
    }
    if (metric)
        metric->elapsedTimeForFutureSharedPrimitivesGroups += futureSharedPrimitivesGroupsStopwatch.elapsed();
}

std::shared_ptr<const OsmAnd::Primitiviser_P::PrimitivesGroup> OsmAnd::Primitiviser_P::obtainPrimitivesGroup(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<PrimitivisedArea>& primitivisedArea,
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    MapStyleEvaluator& orderEvaluator,
    MapStyleEvaluator& polygonEvaluator,
    MapStyleEvaluator& polylineEvaluator,
    MapStyleEvaluator& pointEvaluator,
    Primitiviser_Metrics::Metric_primitivise* const metric)
{
    bool ok;

    //////////////////////////////////////////////////////////////////////////
    //if ((mapObject->id >> 1) == 95692962u)
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    const auto constructedGroup = new PrimitivesGroup(mapObject);
    std::shared_ptr<const PrimitivesGroup> group(constructedGroup);

    uint32_t typeRuleIdIndex = 0;
    const auto& decRules = mapObject->section->encodingDecodingRules->decodingRules;
    for (auto itTypeRuleId = cachingIteratorOf(constOf(mapObject->typesRuleIds)); itTypeRuleId; ++itTypeRuleId, typeRuleIdIndex++)
    {
        const auto& decodedType = decRules[*itTypeRuleId];

        const Stopwatch orderEvaluationStopwatch(metric != nullptr);

        // Setup mapObject-specific input data
        orderEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
        orderEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);
        orderEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_LAYER, mapObject->getSimpleLayerValue());
        orderEvaluator.setBooleanValue(env->styleBuiltinValueDefs->id_INPUT_AREA, mapObject->isArea);
        orderEvaluator.setBooleanValue(env->styleBuiltinValueDefs->id_INPUT_POINT, mapObject->points31.size() == 1);
        orderEvaluator.setBooleanValue(env->styleBuiltinValueDefs->id_INPUT_CYCLE, mapObject->isClosedFigure());

        evaluationResult.clear();
        ok = orderEvaluator.evaluate(mapObject, MapStyleRulesetType::Order, &evaluationResult);

        if (metric)
        {
            metric->elapsedTimeForOrderEvaluation += orderEvaluationStopwatch.elapsed();
            metric->orderEvaluations++;
        }

        // If evaluation failed, skip
        if (!ok)
        {
            if (metric)
                metric->orderRejects++;

            continue;
        }

        int objectType_;
        if (!evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_OBJECT_TYPE, objectType_))
        {
            if (metric)
                metric->orderRejects++;

            continue;
        }
        const auto objectType = static_cast<PrimitiveType>(objectType_);

        int zOrder;
        if (!evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_ORDER, zOrder) || zOrder < 0)
        {
            if (metric)
                metric->orderRejects++;

            continue;
        }

        int shadowLevel = 0;
        evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_LEVEL, shadowLevel);

        if (objectType == PrimitiveType::Polygon)
        {
            // Perform checks on data
            if (mapObject->points31.size() <= 2)
            {
                LogPrintf(LogSeverityLevel::Warning,
                    "Map object #%" PRIu64 " (%" PRIi64 ") primitive is processed as polygon, but has only %d point(s)",
                    mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2,
                    mapObject->points31.size());
                continue;
            }
            if (!mapObject->isClosedFigure())
            {
                LogPrintf(LogSeverityLevel::Warning,
                    "Map object #%" PRIu64 " (%" PRIi64 ") primitive is processed as polygon, but isn't closed",
                    mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2);
                continue;
            }
            if (!mapObject->isClosedFigure(true))
            {
                LogPrintf(LogSeverityLevel::Warning,
                    "Map object #%" PRIu64 " (%" PRIi64 ") primitive is processed as polygon, but isn't closed (inner)",
                    mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2);
                continue;
            }

            //////////////////////////////////////////////////////////////////////////
            //if ((mapObject->id >> 1) == 266877135u)
            //{
            //    int i = 5;
            //}
            //////////////////////////////////////////////////////////////////////////

            // Check size of polygon
            const auto polygonArea31 = Utilities::polygonArea(mapObject->points31);
            const auto polygonAreaInPixels = polygonArea31 / (primitivisedArea->scale31ToPixelDivisor.x * primitivisedArea->scale31ToPixelDivisor.y);
            const auto polygonAreaInAbstractPixels = polygonAreaInPixels / (env->displayDensityFactor * env->displayDensityFactor);
            if (polygonAreaInAbstractPixels <= primitivisedArea->polygonAreaMinimalThreshold)
            {
                if (metric)
                    metric->polygonsRejectedByArea++;

                continue;
            }
            double adjustedZOrder = zOrder;
            if (mapObject->section != env->dummyMapSection)
                adjustedZOrder += (1.0 / polygonArea31);
            else
                adjustedZOrder = 0;

            const Stopwatch polygonEvaluationStopwatch(metric != nullptr);

            // Setup mapObject-specific input data (for Polygon)
            polygonEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
            polygonEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);

            // Evaluate style for this primitive to check if it passes (for Polygon)
            evaluationResult.clear();
            ok = polygonEvaluator.evaluate(mapObject, MapStyleRulesetType::Polygon, &evaluationResult);

            if (metric)
            {
                metric->elapsedTimeForPolygonEvaluation += polygonEvaluationStopwatch.elapsed();
                metric->polygonEvaluations++;
            }

            // Add as polygon if accepted as polygon
            if (ok)
            {
                // Create new primitive
                const std::shared_ptr<Primitive> primitive(new Primitive(
                    group,
                    objectType,
                    typeRuleIdIndex,
                    qMove(evaluationResult)));
                primitive->zOrder = adjustedZOrder;

                // Accept this primitive
                constructedGroup->polygons.push_back(qMove(primitive));

                // Update metric
                if (metric)
                    metric->polygonPrimitives++;
            }
            else
            {
                if (metric)
                    metric->polygonRejects++;
            }

            const Stopwatch pointEvaluationStopwatch(metric != nullptr);

            // Setup mapObject-specific input data (for Point)
            pointEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
            pointEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);

            // Evaluate Point rules
            evaluationResult.clear();
            const auto hasIcon = pointEvaluator.evaluate(mapObject, MapStyleRulesetType::Point, &evaluationResult);

            // Update metric
            if (metric)
            {
                metric->elapsedTimeForPointEvaluation += pointEvaluationStopwatch.elapsed();
                metric->pointEvaluations++;
            }

            // Create point primitive only in case:
            //  - there's text and currently processing first tag=value pair
            //  - there's icon
            if ((typeRuleIdIndex == 0 && !mapObject->captions.isEmpty()) || hasIcon)
            {
                // Duplicate primitive as point
                std::shared_ptr<Primitive> pointPrimitive;
                if (hasIcon)
                {
                    // Point evaluation is a bit special, it's success only indicates that point has an icon
                    pointPrimitive.reset(new Primitive(
                        group,
                        PrimitiveType::Point,
                        typeRuleIdIndex,
                        qMove(evaluationResult)));
                }
                else
                {
                    pointPrimitive.reset(new Primitive(
                        group,
                        PrimitiveType::Point,
                        typeRuleIdIndex));
                }
                pointPrimitive->zOrder = adjustedZOrder;

                constructedGroup->points.push_back(qMove(pointPrimitive));

                // Update metric
                if (metric)
                    metric->pointPrimitives++;
            }
        }
        else if (objectType == PrimitiveType::Polyline)
        {
            // Perform checks on data
            if (mapObject->points31.size() < 2)
            {
                LogPrintf(LogSeverityLevel::Warning,
                    "Map object #%" PRIu64 " (%" PRIi64 ") is processed as polyline, but has %d point(s)",
                    mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2,
                    mapObject->_points31.size());
                continue;
            }

            const Stopwatch polylineEvaluationStopwatch(metric != nullptr);

            // Setup mapObject-specific input data
            polylineEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
            polylineEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);
            polylineEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_LAYER, mapObject->getSimpleLayerValue());

            // Evaluate style for this primitive to check if it passes
            evaluationResult.clear();
            ok = polylineEvaluator.evaluate(mapObject, MapStyleRulesetType::Polyline, &evaluationResult);

            if (metric)
            {
                metric->elapsedTimeForPolylineEvaluation += polylineEvaluationStopwatch.elapsed();
                metric->polylineEvaluations++;
            }

            // If evaluation failed, skip
            if (!ok)
            {
                if (metric)
                    metric->polylineRejects++;

                continue;
            }

            // Create new primitive
            const std::shared_ptr<Primitive> primitive(new Primitive(
                group, 
                objectType,
                typeRuleIdIndex,
                qMove(evaluationResult)));
            primitive->zOrder = zOrder;

            // Accept this primitive
            constructedGroup->polylines.push_back(qMove(primitive));

            // Update metric
            if (metric)
                metric->polylinePrimitives++;
        }
        else if (objectType == PrimitiveType::Point)
        {
            // Perform checks on data
            if (mapObject->points31.size() < 1)
            {
                LogPrintf(LogSeverityLevel::Warning,
                    "Map object #%" PRIu64 " (%" PRIi64 ") is processed as point, but has no point",
                    mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2);
                continue;
            }

            const Stopwatch pointEvaluationStopwatch(metric != nullptr);

            // Setup mapObject-specific input data
            pointEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
            pointEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);

            // Evaluate Point rules
            evaluationResult.clear();
            const bool hasIcon = pointEvaluator.evaluate(mapObject, MapStyleRulesetType::Point, &evaluationResult);

            // Update metric
            if (metric)
            {
                metric->elapsedTimeForPointEvaluation += pointEvaluationStopwatch.elapsed();
                metric->pointEvaluations++;
            }

            // Create point primitive only in case:
            //  - there's text and currently processing first tag=value pair
            //  - there's icon
            const auto hasContent = (typeRuleIdIndex == 0 && !mapObject->captions.isEmpty()) || hasIcon;
            if (!hasContent)
            {
                if (metric)
                    metric->pointRejects++;

                continue;
            }

            // Create new primitive
            std::shared_ptr<Primitive> primitive;
            if (hasIcon)
            {
                // Point evaluation is a bit special, it's success only indicates that point has an icon
                primitive.reset(new Primitive(
                    group,
                    PrimitiveType::Point,
                    typeRuleIdIndex,
                    qMove(evaluationResult)));
            }
            else
            {
                primitive.reset(new Primitive(
                    group,
                    PrimitiveType::Point,
                    typeRuleIdIndex));
            }
            primitive->zOrder = zOrder;

            // Accept this primitive
            constructedGroup->points.push_back(qMove(primitive));

            // Update metric
            if (metric)
                metric->pointPrimitives++;
        }
        else
        {
            assert(false);
            continue;
        }

        if (shadowLevel > 0)
        {
            primitivisedArea->shadowLevelMin = qMin(primitivisedArea->shadowLevelMin, static_cast<int>(zOrder));
            primitivisedArea->shadowLevelMax = qMax(primitivisedArea->shadowLevelMax, static_cast<int>(zOrder));
        }
    }

    return group;
}

void OsmAnd::Primitiviser_P::sortAndFilterPrimitives(
    const std::shared_ptr<PrimitivisedArea>& primitivisedArea)
{
    const auto privitivesSort =
        []
        (const std::shared_ptr<const Primitive>& l, const std::shared_ptr<const Primitive>& r) -> bool
        {
            if (qFuzzyCompare(l->zOrder, r->zOrder))
            {
                if (l->typeRuleIdIndex == r->typeRuleIdIndex)
                    return l->sourceObject->points31.size() < r->sourceObject->points31.size();
                return l->typeRuleIdIndex < r->typeRuleIdIndex;
            }
            return l->zOrder < r->zOrder;
        };

    qSort(primitivisedArea->polygons.begin(), primitivisedArea->polygons.end(), privitivesSort);
    qSort(primitivisedArea->polylines.begin(), primitivisedArea->polylines.end(), privitivesSort);
    filterOutHighwaysByDensity(primitivisedArea);
    qSort(primitivisedArea->points.begin(), primitivisedArea->points.end(), privitivesSort);
}

void OsmAnd::Primitiviser_P::filterOutHighwaysByDensity(
    const std::shared_ptr<PrimitivisedArea>& primitivisedArea)
{
    // Check if any filtering needed
    if (primitivisedArea->roadDensityZoomTile == 0 || primitivisedArea->roadsDensityLimitPerTile == 0)
        return;

    const auto dZ = primitivisedArea->zoom + primitivisedArea->roadDensityZoomTile;
    QHash< uint64_t, std::pair<uint32_t, double> > densityMap;

    auto itLine = mutableIteratorOf(primitivisedArea->polylines);
    itLine.toBack();
    while (itLine.hasPrevious())
    {
        const auto& line = itLine.previous();

        auto accept = true;
        if (line->sourceObject->_typesRuleIds[line->typeRuleIdIndex] == line->sourceObject->section->encodingDecodingRules->highway_encodingRuleId)
        {
            accept = false;

            uint64_t prevId = 0;
            const auto pointsCount = line->sourceObject->points31.size();
            auto pPoint = line->sourceObject->points31.constData();
            for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
            {
                auto x = pPoint->x >> (31 - dZ);
                auto y = pPoint->y >> (31 - dZ);
                uint64_t id = (static_cast<uint64_t>(x) << dZ) | y;
                if (prevId != id)
                {
                    prevId = id;

                    auto& mapEntry = densityMap[id];
                    if (mapEntry.first < primitivisedArea->roadsDensityLimitPerTile /*&& p.second > o */)
                    {
                        accept = true;
                        mapEntry.first += 1;
                        mapEntry.second = line->zOrder;
                    }
                }
            }
        }

        if (!accept)
            itLine.remove();
    }
}

void OsmAnd::Primitiviser_P::obtainPrimitivesSymbols(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<PrimitivisedArea>& primitivisedArea,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    const std::shared_ptr<Cache>& cache,
    const IQueryController* const controller)
{
    //NOTE: Em, I'm not sure this is still true
    //NOTE: Since 2 tiles with same BinaryMapObject may have different set of polylines, generated from it,
    //NOTE: then set of symbols also should differ, but it won't.
    const auto pSharedSymbolGroups = cache ? cache->getSymbolsGroupsPtr(primitivisedArea->zoom) : nullptr;
    QList< proper::shared_future< std::shared_ptr<const SymbolsGroup> > > futureSharedSymbolGroups;
    for (const auto& primitivesGroup : constOf(primitivisedArea->primitivesGroups))
    {
        if (controller && controller->isAborted())
            return;

        // If using shared context is allowed, check if this group was already processed
        // (using shared cache is only allowed for non-generated MapObjects),
        // then symbols group can be shared
        const auto canBeShared = (primitivesGroup->sourceObject->section != env->dummyMapSection);

        //////////////////////////////////////////////////////////////////////////
        //if ((primitivesGroup->sourceObject->id >> 1) == 1937897178u)
        //{
        //    int i = 5;
        //}
        //////////////////////////////////////////////////////////////////////////

        if (pSharedSymbolGroups && canBeShared)
        {
            // If this group was already processed, use that
            std::shared_ptr<const SymbolsGroup> group;
            proper::shared_future< std::shared_ptr<const SymbolsGroup> > futureGroup;
            if (pSharedSymbolGroups->obtainReferenceOrFutureReferenceOrMakePromise(primitivesGroup->sourceObject->id, group, futureGroup))
            {
                if (group)
                {
                    // Add shared group to current context
                    assert(!primitivisedArea->symbolsGroups.contains(group->sourceObject));
                    primitivisedArea->symbolsGroups.insert(group->sourceObject, qMove(group));
                }
                else
                {
                    futureSharedSymbolGroups.push_back(qMove(futureGroup));
                }

                continue;
            }
        }

        // Create a symbols group
        const auto constructedGroup = new SymbolsGroup(primitivesGroup->sourceObject);
        std::shared_ptr<const SymbolsGroup> group(constructedGroup);

        // For each primitive if primitive group, collect symbols from it
        //NOTE: Each polygon that has icon or text is also added as point. So there's no need to process polygons
        /*
        collectSymbolsFromPrimitives(
            env,
            primitivisedArea,
            primitivesGroup->polygons,
            PrimitivesType::Polygons,
            qMove(evaluationResult),
            constructedGroup->symbols,
            controller);
        */
        collectSymbolsFromPrimitives(
            env,
            primitivisedArea,
            primitivesGroup->polylines,
            PrimitivesType::Polylines,
            qMove(evaluationResult),
            constructedGroup->symbols,
            controller);
        collectSymbolsFromPrimitives(
            env,
            primitivisedArea,
            primitivesGroup->points,
            PrimitivesType::Points,
            qMove(evaluationResult),
            constructedGroup->symbols,
            controller);

        // Add this group to shared cache
        if (pSharedSymbolGroups && canBeShared)
            pSharedSymbolGroups->fulfilPromiseAndReference(primitivesGroup->sourceObject->id, group);

        // Empty groups are also inserted, to indicate that they are empty
        assert(!primitivisedArea->symbolsGroups.contains(group->sourceObject));
        primitivisedArea->symbolsGroups.insert(group->sourceObject, qMove(group));
    }

    for (auto& futureGroup : futureSharedSymbolGroups)
    {
        auto group = futureGroup.get();

        // Add shared group to current context
        assert(!primitivisedArea->symbolsGroups.contains(group->sourceObject));
        primitivisedArea->symbolsGroups.insert(group->sourceObject, qMove(group));
    }
}

void OsmAnd::Primitiviser_P::collectSymbolsFromPrimitives(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<const PrimitivisedArea>& primitivisedArea,
    const PrimitivesCollection& primitives,
    const PrimitivesType type,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    SymbolsCollection& outSymbols,
    const IQueryController* const controller)
{
    assert(type != PrimitivesType::Polylines_ShadowOnly);

    for (const auto& primitive : constOf(primitives))
    {
        if (controller && controller->isAborted())
            return;

        if (type == PrimitivesType::Polygons)
        {
            obtainSymbolsFromPolygon(env, primitivisedArea, primitive, qMove(evaluationResult), outSymbols);
        }
        else if (type == PrimitivesType::Polylines)
        {
            obtainSymbolsFromPolyline(env, primitivisedArea, primitive, qMove(evaluationResult), outSymbols);
        }
        else if (type == PrimitivesType::Points)
        {
            obtainSymbolsFromPoint(env, primitivisedArea, primitive, qMove(evaluationResult), outSymbols);
        }
    }
}

void OsmAnd::Primitiviser_P::obtainSymbolsFromPolygon(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<const PrimitivisedArea>& primitivisedArea,
    const std::shared_ptr<const Primitive>& primitive,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    SymbolsCollection& outSymbols)
{
    const auto& points31 = primitive->sourceObject->points31;

    assert(points31.size() > 2);
    assert(primitive->sourceObject->isClosedFigure());
    assert(primitive->sourceObject->isClosedFigure(true));

    // Get center of polygon, since all symbols of polygon are related to it's center
    PointI64 center;
    const auto pointsCount = points31.size();
    auto pPoint = points31.constData();
    for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        center.x += pPoint->x;
        center.y += pPoint->y;
    }
    center.x /= pointsCount;
    center.y /= pointsCount;

    // Obtain texts for this symbol
    obtainPrimitiveTexts(env,
        primitivisedArea,
        primitive,
        Utilities::normalizeCoordinates(center, ZoomLevel31),
        qMove(evaluationResult),
        outSymbols);
}

void OsmAnd::Primitiviser_P::obtainSymbolsFromPolyline(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<const PrimitivisedArea>& primitivisedArea,
    const std::shared_ptr<const Primitive>& primitive,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    SymbolsCollection& outSymbols)
{
    const auto& points31 = primitive->sourceObject->points31;

    assert(points31.size() >= 2);

    // Symbols for polyline are always related to it's "middle" point
    const auto center = points31[points31.size() >> 1];

    // Obtain texts for this symbol
    obtainPrimitiveTexts(
        env,
        primitivisedArea,
        primitive,
        center,
        qMove(evaluationResult),
        outSymbols);
}

void OsmAnd::Primitiviser_P::obtainSymbolsFromPoint(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<const PrimitivisedArea>& primitivisedArea,
    const std::shared_ptr<const Primitive>& primitive,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    SymbolsCollection& outSymbols)
{
    //////////////////////////////////////////////////////////////////////////
    //if ((primitive->sourceObject->id >> 1) == 1937897178u)
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    const auto& points31 = primitive->sourceObject->points31;

    assert(points31.size() > 0);

    // Depending on type of point, center is determined differently
    PointI center;
    if (points31.size() == 1)
    {
        // Regular point
        center = points31.first();
    }
    else
    {
        // Point represents center of polygon
        PointI64 center_;
        const auto pointsCount = points31.size();
        auto pPoint = points31.constData();
        for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
        {
            center_.x += pPoint->x;
            center_.y += pPoint->y;
        }
        center_.x /= pointsCount;
        center_.y /= pointsCount;

        center = Utilities::normalizeCoordinates(center_, ZoomLevel31);
    }

    // Obtain icon for this symbol
    obtainPrimitiveIcon(
        env,
        primitive,
        center,
        qMove(evaluationResult),
        outSymbols);

    // Obtain texts for this symbol (only in case it's first tag)
    if (primitive->typeRuleIdIndex == 0)
    {
        obtainPrimitiveTexts(
            env,
            primitivisedArea,
            primitive,
            center,
            qMove(evaluationResult),
            outSymbols);
    }
}

void OsmAnd::Primitiviser_P::obtainPrimitiveTexts(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<const PrimitivisedArea>& primitivisedArea,
    const std::shared_ptr<const Primitive>& primitive,
    const PointI& location,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    SymbolsCollection& outSymbols)
{
    const auto& mapObject = primitive->sourceObject;

    // Text symbols can only be obtained from captions
    if (mapObject->captions.isEmpty())
        return;

    const auto& encDecRules = mapObject->section->encodingDecodingRules;
    const auto typeRuleId = mapObject->_typesRuleIds[primitive->typeRuleIdIndex];
    const auto& decodedType = encDecRules->decodingRules[typeRuleId];

    MapStyleEvaluator textEvaluator(env->resolvedStyle, env->displayDensityFactor);
    env->applyTo(textEvaluator);

    // Get captions and their order
    auto captions = mapObject->captions;
    auto captionsOrder = mapObject->captionsOrder;

    // Process captions to find out what names are present and modify that if needed
    bool hasNativeName = false;
    bool hasLocalizedName = false;
    uint32_t localizedNameRuleId = std::numeric_limits<uint32_t>::max();
    {
        const auto citCaptionsEnd = captions.cend();

        // Look for native name
        const auto citNativeName =
            (encDecRules->name_encodingRuleId == std::numeric_limits<uint32_t>::max())
            ? citCaptionsEnd
            : captions.constFind(encDecRules->name_encodingRuleId);
        auto hasNativeName = (citNativeName != citCaptionsEnd);

        // Look for localized name
        const auto citLocalizedNameRuleId = encDecRules->localizedName_encodingRuleIds.constFind(env->localeLanguageId);
        if (citLocalizedNameRuleId != encDecRules->localizedName_encodingRuleIds.cend())
            localizedNameRuleId = *citLocalizedNameRuleId;
        const auto citLocalizedName =
            (localizedNameRuleId == std::numeric_limits<uint32_t>::max())
            ? citCaptionsEnd
            : captions.constFind(localizedNameRuleId);
        auto hasLocalizedName = (citLocalizedName != citCaptionsEnd);

        // Post-process names-only captions
        if (hasNativeName && hasLocalizedName)
        {
            const auto& nativeNameValue = citNativeName.value();
            const auto& localizedNameValue = citLocalizedName.value();

            // If mapObject has both native and localized names and they are equal - prefer native
            if (nativeNameValue.compare(localizedNameValue, Qt::CaseInsensitive) == 0)
            {
                captions.remove(localizedNameRuleId);
                captionsOrder.removeOne(localizedNameRuleId);
                hasLocalizedName = false;
            }
        }
        else if (hasNativeName && !hasLocalizedName)
        {
            const auto& nativeNameValue = citNativeName.value();

            // If mapObject has native name, but doesn't have localized name - create such (as a Latin)
            const auto latinNameValue = ICU::transliterateToLatin(nativeNameValue);

            // If transliterated name differs from native name, add it to captions as 'localized' name
            if (nativeNameValue.compare(latinNameValue, Qt::CaseInsensitive) != 0)
            {
                captions.insert(localizedNameRuleId, latinNameValue);
                // 'Localized' name should go after native name, according to following content from 'rendering_types.xml':
                // <type tag="name" additional="text" order="40"/>
                // <type tag="name:*" additional="text" order="45" />
                captionsOrder.insert(captionsOrder.indexOf(encDecRules->name_encodingRuleId) + 1, localizedNameRuleId);
                hasLocalizedName = true;
            }
        }
    }
    
    // Process captions in order how captions where declared in OBF source (what is being controlled by 'rendering_types.xml')
    bool ok;
    for (const auto& captionRuleId : constOf(captionsOrder))
    {
        const auto& caption = constOf(captions)[captionRuleId];

        // If it's empty, it can not be primitivised
        if (caption.isEmpty())
            continue;

        // Skip captions that are names but not of 'native' or 'localized' language
        if (captionRuleId != encDecRules->name_encodingRuleId &&
            captionRuleId != localizedNameRuleId &&
            encDecRules->namesRuleId.contains(captionRuleId))
        {
            continue;
        }

        // Evaluate style to obtain text parameters
        textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
        textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);
        textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, primitivisedArea->zoom);
        textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, primitivisedArea->zoom);
        textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_TEXT_LENGTH, caption.length());

        // Get tag of rule, in case caption is not a name
        QString captionRuleTag;
        if (captionRuleId != encDecRules->name_encodingRuleId && captionRuleId != localizedNameRuleId)
            captionRuleTag = encDecRules->decodingRules[captionRuleId].tag;
        textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_NAME_TAG, captionRuleTag);

        evaluationResult.clear();
        if (!textEvaluator.evaluate(mapObject, MapStyleRulesetType::Text, &evaluationResult))
            continue;

        // Skip text that doesn't have valid size
        int textSize = 0;
        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_SIZE, textSize);
        if (!ok || textSize == 0)
            continue;

        // Determine language of this text
        auto languageId = LanguageId::Invariant;
        if (hasLocalizedName && hasNativeName)
        {
            if (captionRuleId == encDecRules->name_encodingRuleId)
                languageId = LanguageId::Native;
            else if (captionRuleId == localizedNameRuleId)
                languageId = LanguageId::Localized;
        }

        // Create primitive
        const std::shared_ptr<TextSymbol> text(new TextSymbol(primitive));
        text->location31 = location;
        text->languageId = languageId;
        text->value = caption;

        text->drawOnPath = false;
        text->drawAlongPath = false;
        if (primitive->type == PrimitiveType::Polyline)
        {
            evaluationResult.getBooleanValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_ON_PATH, text->drawOnPath);
            if (!text->drawOnPath)
                text->drawAlongPath = (primitive->type == PrimitiveType::Polyline);
        }
        
        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_ORDER, text->order);
        if (!ok)
            continue;
        //NOTE: a magic shifting of text order. This is needed to keep text more important than anything else
        text->order += 100000;

        text->verticalOffset = 0;
        evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_DY, text->verticalOffset);

        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_COLOR, text->color.argb);
        if (!ok || text->color == ColorARGB::fromSkColor(SK_ColorTRANSPARENT))
            text->color = ColorARGB::fromSkColor(SK_ColorBLACK);

        text->size = textSize;

        text->shadowRadius = 0;
        evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_HALO_RADIUS, text->shadowRadius);

        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_HALO_COLOR, text->shadowColor.argb);
        if (!ok || text->shadowColor == ColorARGB::fromSkColor(SK_ColorTRANSPARENT))
            text->shadowColor = ColorARGB::fromSkColor(SK_ColorWHITE);

        text->wrapWidth = 0;
        evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_WRAP_WIDTH, text->wrapWidth);
        if (!text->drawOnPath && text->wrapWidth == 0)
        {
            // Default wrapping width (in characters)
            text->wrapWidth = DefaultTextLabelWrappingLengthInCharacters;
        }

        text->isBold = false;
        evaluationResult.getBooleanValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_BOLD, text->isBold);

        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_MIN_DISTANCE_X, text->minDistance.x);
        if (ok && (text->minDistance.x > 0 || text->minDistance.y > 0))
        {
            text->minDistance.y = 15.0f*env->displayDensityFactor; /* 15dip */
            const auto minDistanceX = 5.0f*env->displayDensityFactor; /* 5dip */
            if (text->minDistance.x < minDistanceX)
                text->minDistance.x = minDistanceX;
        }

        evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_SHIELD, text->shieldResourceName);

        QString intersectsWith;
        ok = primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_INTERSECTS_WITH, intersectsWith);
        if (!ok)
            intersectsWith = QLatin1String("text"); // To simulate original behavior, texts should intersect only other texts
        text->intersectsWith = intersectsWith.split(QLatin1Char(','), QString::SkipEmptyParts).toSet();

        outSymbols.push_back(qMove(text));
    }
}

void OsmAnd::Primitiviser_P::obtainPrimitiveIcon(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const std::shared_ptr<const Primitive>& primitive,
    const PointI& location,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    SymbolsCollection& outSymbols)
{
    if (primitive->evaluationResult.isEmpty())
        return;

    bool ok;

    QString iconResourceName;
    ok = primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON, iconResourceName);

    if (!ok || iconResourceName.isEmpty())
        return;

    const std::shared_ptr<IconSymbol> icon(new IconSymbol(primitive));
    icon->drawAlongPath = (primitive->type == PrimitiveType::Polyline);
    icon->location31 = location;

    icon->resourceName = qMove(iconResourceName);

    icon->order = 100;
    primitive->evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_ORDER, icon->order);

    primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_SHIELD, icon->shieldResourceName);

    icon->intersectionSize = 0.0f; // 0.0 means 'Excluded from intersection check'
    ok = primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_INTERSECTION_SIZE, icon->intersectionSize);
    if (!ok)
        icon->intersectionSize = -1.0f; // < 0 means 'Determined by it's real size'

    QString intersectsWith;
    ok = primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_INTERSECTS_WITH, intersectsWith);
    if (!ok)
        intersectsWith = QLatin1String("icon"); // To simulate original behavior, icons should intersect only other icons
    icon->intersectsWith = intersectsWith.split(QLatin1Char(','), QString::SkipEmptyParts).toSet();

    outSymbols.push_back(qMove(icon));
}
