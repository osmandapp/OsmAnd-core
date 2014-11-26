#include "MapPrimitiviser_P.h"
#include "MapPrimitiviser.h"

#include "stdlib_common.h"
#include <proper/future.h>
#include <set>
#include <algorithm>

#include "QtExtensions.h"
#include "QtCommon.h"

#include "ICU.h"
#include "MapStyleEvaluator.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleBuiltinValueDefinitions.h"
#include "ObfMapSectionInfo.h"
#include "MapObject.h"
#include "BinaryMapObject.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "QKeyValueIterator.h"
#include "QCachingIterator.h"
#include "Logging.h"

OsmAnd::MapPrimitiviser_P::MapPrimitiviser_P(MapPrimitiviser* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapPrimitiviser_P::~MapPrimitiviser_P()
{
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseAllMapObjects(
    const ZoomLevel zoom,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const IQueryController* const controller,
    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric)
{
    return primitiviseAllMapObjects(
        PointD(-1.0, -1.0),
        zoom,
        objects,
        cache,
        controller,
        metric);
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseAllMapObjects(
    const PointD scaleDivisor31ToPixel,
    const ZoomLevel zoom,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const IQueryController* const controller,
    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric)
{
    const Stopwatch totalStopwatch(metric != nullptr);

    const Context context(owner->environment, zoom);
    const std::shared_ptr<PrimitivisedObjects> primitivisedObjects(new PrimitivisedObjects(
        owner->environment,
        cache,
        zoom,
        scaleDivisor31ToPixel));

    // Check if there are map objects for primitivisation
    if (objects.isEmpty())
    {
        // Empty area
        assert(primitivisedObjects->isEmpty());
        return primitivisedObjects;
    }

    // Obtain primitives:
    MapStyleEvaluationResult evaluationResult;

    const Stopwatch obtainPrimitivesStopwatch(metric != nullptr);
    obtainPrimitives(context, primitivisedObjects, objects, qMove(evaluationResult), cache, controller, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitives += obtainPrimitivesStopwatch.elapsed();
    
    if (controller && controller->isAborted())
        return nullptr;

    // Sort and filter primitives
    const Stopwatch sortAndFilterPrimitivesStopwatch(metric != nullptr);
    sortAndFilterPrimitives(context, primitivisedObjects);
    if (metric)
        metric->elapsedTimeForSortingAndFilteringPrimitives += sortAndFilterPrimitivesStopwatch.elapsed();

    // Obtain symbols from primitives
    const Stopwatch obtainPrimitivesSymbolsStopwatch(metric != nullptr);
    obtainPrimitivesSymbols(context, primitivisedObjects, qMove(evaluationResult), cache, controller);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesSymbols += obtainPrimitivesSymbolsStopwatch.elapsed();

    // Cleanup if aborted
    if (controller && controller->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    return primitivisedObjects;
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseWithSurface(
    const AreaI area31,
    const PointI areaSizeInPixels,
    const ZoomLevel zoom,
    const MapSurfaceType surfaceType_,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const IQueryController* const controller,
    MapPrimitiviser_Metrics::Metric_primitiviseWithSurface* const metric)
{
    const Stopwatch totalStopwatch(metric != nullptr);

    //////////////////////////////////////////////////////////////////////////
    //if (area31 == Utilities::tileBoundingBox31(TileId::fromXY(2204, 1370), ZoomLevel12))
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    const Context context(owner->environment, zoom);
    const std::shared_ptr<PrimitivisedObjects> primitivisedObjects(new PrimitivisedObjects(
        owner->environment,
        cache,
        zoom,
        Utilities::getScaleDivisor31ToPixel(areaSizeInPixels, zoom)));

    const Stopwatch objectsSortingStopwatch(metric != nullptr);

    // Split input map objects to object, coastline, basemapObjects and basemapCoastline
    QList< std::shared_ptr<const MapObject> > detailedmapMapObjects, detailedmapCoastlineObjects, basemapMapObjects, basemapCoastlineObjects;
    QList< std::shared_ptr<const MapObject> > polygonizedCoastlineObjects;
    for (const auto& mapObject : constOf(objects))
    {
        if (controller && controller->isAborted())
            break;

        // Check if this map object is from basemap
        auto isBasemapObject = false;
        if (const auto possiblyBasemapObject = std::dynamic_pointer_cast<const BinaryMapObject>(mapObject))
            isBasemapObject = possiblyBasemapObject->section->isBasemap;

        if (mapObject->containsType(mapObject->encodingDecodingRules->naturalCoastline_encodingRuleId))
        {
            if (isBasemapObject)
                basemapCoastlineObjects.push_back(mapObject);
            else
                detailedmapCoastlineObjects.push_back(mapObject);
        }
        else
        {
            if (isBasemapObject)
                basemapMapObjects.push_back(mapObject);
            else
                detailedmapMapObjects.push_back(mapObject);
        }
    }
    if (controller && controller->isAborted())
        return nullptr;

    //////////////////////////////////////////////////////////////////////////
    //if (area31 == Utilities::tileBoundingBox31(TileId::fromXY(2204, 1370), ZoomLevel12))
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    if (metric)
        metric->elapsedTimeForSortingObjects += objectsSortingStopwatch.elapsed();

    const Stopwatch polygonizeCoastlinesStopwatch(metric != nullptr);

    // Polygonize coastlines
    auto surfaceType = surfaceType_;
    const auto basemapCoastlinesPresent = !basemapCoastlineObjects.isEmpty();
    const auto detailedmapCoastlinesPresent = !detailedmapCoastlineObjects.isEmpty();
    const auto detailedLandDataPresent = (zoom >= MapPrimitiviser::DetailedLandDataZoom) && !detailedmapMapObjects.isEmpty();
    auto fillEntireArea = true;
    auto addBasemapCoastlines = true;
    if (detailedmapCoastlinesPresent)
    {
        const bool coastlinesWereAdded = polygonizeCoastlines(
            area31,
            zoom,
            primitivisedObjects,
            detailedmapCoastlineObjects,
            polygonizedCoastlineObjects,
            basemapCoastlinesPresent,
            true);
        fillEntireArea = !coastlinesWereAdded && fillEntireArea;
        addBasemapCoastlines = (!coastlinesWereAdded && !detailedLandDataPresent) || zoom <= static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap);
    }
    else
    {
        addBasemapCoastlines = !detailedLandDataPresent;
    }
    if (addBasemapCoastlines)
    {
        const bool coastlinesWereAdded = polygonizeCoastlines(
            area31,
            zoom,
            primitivisedObjects,
            basemapCoastlineObjects,
            polygonizedCoastlineObjects,
            false,
            true);
        fillEntireArea = !coastlinesWereAdded && fillEntireArea;
    }

    // In case zoom is higher than ObfMapSectionLevel::MaxBasemapZoomLevel and coastlines were not used
    // due to none of them intersect current zoom tile edge, look for the nearest coastline segment
    // to determine use FullLand or FullWater as surface type
    if (zoom > ObfMapSectionLevel::MaxBasemapZoomLevel && basemapCoastlinesPresent && fillEntireArea)
    {
        const auto center = area31.center();
        assert(area31.contains(center));

        std::shared_ptr<const MapObject> neareastCoastlineMapObject;
        PointI nearestCoastlineSegment0;
        PointI nearestCoastlineSegment1;
        double squaredMinDistance = std::numeric_limits<double>::max();

        for (const auto& coastlineMapObject : constOf(basemapCoastlineObjects))
        {
            int segmentIndex0 = -1;
            int segmentIndex1 = -1;
            const auto squaredDistance = Utilities::minimalSquaredDistanceToLineSegmentFromPoint(
                coastlineMapObject->points31,
                center,
                &segmentIndex0,
                &segmentIndex1);
            if (segmentIndex0 != -1 && segmentIndex1 != -1 && squaredDistance < squaredMinDistance)
            {
                const auto pPoints = coastlineMapObject->points31.constData();
                nearestCoastlineSegment0 = pPoints[segmentIndex0];
                nearestCoastlineSegment1 = pPoints[segmentIndex1];
                squaredMinDistance = squaredDistance;
                neareastCoastlineMapObject = coastlineMapObject;
            }
        }

        // If nearest coastline was found, determine FullLand or FullWater using direction of the nearest segment
        // Rule: Water is always on the right along the direction of coastline segment.
        if (neareastCoastlineMapObject)
        {
            const auto sign = crossProductSign(nearestCoastlineSegment0, nearestCoastlineSegment1, center);
            surfaceType = (sign >= 0) ? MapSurfaceType::FullLand : MapSurfaceType::FullWater;
        }
    }

    if (metric)
    {
        metric->elapsedTimeForPolygonizingCoastlines += polygonizeCoastlinesStopwatch.elapsed();
        metric->polygonizedCoastlines += polygonizedCoastlineObjects.size();
    }

    if (basemapMapObjects.isEmpty() && detailedmapMapObjects.isEmpty() && surfaceType == MapSurfaceType::Undefined)
    {
        // Empty area
        assert(primitivisedObjects->isEmpty());
        return primitivisedObjects;
    }

    if (fillEntireArea)
    {
        const std::shared_ptr<MapObject> bgMapObject(new SurfaceMapObject());
        bgMapObject->isArea = true;
        bgMapObject->points31.push_back(qMove(PointI(area31.left(), area31.top())));
        bgMapObject->points31.push_back(qMove(PointI(area31.right(), area31.top())));
        bgMapObject->points31.push_back(qMove(PointI(area31.right(), area31.bottom())));
        bgMapObject->points31.push_back(qMove(PointI(area31.left(), area31.bottom())));
        bgMapObject->points31.push_back(bgMapObject->points31.first());
        bgMapObject->bbox31 = area31;
        if (surfaceType == MapSurfaceType::FullWater)
            bgMapObject->typesRuleIds.push_back(MapObject::defaultEncodingDecodingRules->naturalCoastline_encodingRuleId);
        else if (surfaceType == MapSurfaceType::FullLand || surfaceType == MapSurfaceType::Mixed)
            bgMapObject->typesRuleIds.push_back(MapObject::defaultEncodingDecodingRules->naturalLand_encodingRuleId);
        else // if (surfaceType == SurfaceType::Undefined)
        {
            LogPrintf(LogSeverityLevel::Warning, "Area [%d, %d, %d, %d]@%d has undefined surface type",
                area31.top(),
                area31.left(),
                area31.bottom(),
                area31.right(),
                zoom);

            bgMapObject->isArea = false;
            bgMapObject->typesRuleIds.push_back(MapObject::defaultEncodingDecodingRules->naturalCoastlineBroken_encodingRuleId);
        }
        bgMapObject->additionalTypesRuleIds.push_back(MapObject::defaultEncodingDecodingRules->layerLowest_encodingRuleId);

        assert(bgMapObject->isClosedFigure());
        polygonizedCoastlineObjects.push_back(qMove(bgMapObject));
    }

    // Obtain primitives
    const bool detailedDataMissing = (zoom > static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap)) && detailedmapMapObjects.isEmpty() && detailedmapCoastlineObjects.isEmpty();

    // Check if there is no data to primitivise. Report, clean-up and exit
    const auto mapObjectsCount =
        detailedmapMapObjects.size() +
        ((zoom <= static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap) || detailedDataMissing) ? basemapMapObjects.size() : 0) +
        polygonizedCoastlineObjects.size();
    if (mapObjectsCount == 0)
    {
        // Empty area
        assert(primitivisedObjects->isEmpty());
        return primitivisedObjects;
    }

    // Obtain primitives:
    MapStyleEvaluationResult evaluationResult;

    const Stopwatch obtainPrimitivesFromDetailedmapStopwatch(metric != nullptr);
    obtainPrimitives(context, primitivisedObjects, detailedmapMapObjects, qMove(evaluationResult), cache, controller, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesFromDetailedmap += obtainPrimitivesFromDetailedmapStopwatch.elapsed();

    if ((zoom <= static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap)) || detailedDataMissing)
    {
        const Stopwatch obtainPrimitivesFromBasemapStopwatch(metric != nullptr);
        obtainPrimitives(context, primitivisedObjects, basemapMapObjects, qMove(evaluationResult), cache, controller, metric);
        if (metric)
            metric->elapsedTimeForObtainingPrimitivesFromBasemap += obtainPrimitivesFromBasemapStopwatch.elapsed();
    }

    const Stopwatch obtainPrimitivesFromCoastlinesStopwatch(metric != nullptr);
    obtainPrimitives(context, primitivisedObjects, polygonizedCoastlineObjects, qMove(evaluationResult), cache, controller, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesFromCoastlines += obtainPrimitivesFromCoastlinesStopwatch.elapsed();

    if (controller && controller->isAborted())
        return nullptr;

    // Sort and filter primitives
    const Stopwatch sortAndFilterPrimitivesStopwatch(metric != nullptr);
    sortAndFilterPrimitives(context, primitivisedObjects);
    if (metric)
        metric->elapsedTimeForSortingAndFilteringPrimitives += sortAndFilterPrimitivesStopwatch.elapsed();

    // Obtain symbols from primitives
    const Stopwatch obtainPrimitivesSymbolsStopwatch(metric != nullptr);
    obtainPrimitivesSymbols(context, primitivisedObjects, qMove(evaluationResult), cache, controller);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesSymbols += obtainPrimitivesSymbolsStopwatch.elapsed();

    // Cleanup if aborted
    if (controller && controller->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    return primitivisedObjects;
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseWithoutSurface(
    const PointD scaleDivisor31ToPixel,
    const ZoomLevel zoom,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const IQueryController* const controller,
    MapPrimitiviser_Metrics::Metric_primitiviseWithoutSurface* const metric)
{
    const Stopwatch totalStopwatch(metric != nullptr);

    const Context context(owner->environment, zoom);
    const std::shared_ptr<PrimitivisedObjects> primitivisedObjects(new PrimitivisedObjects(
        owner->environment,
        cache, 
        zoom,
        scaleDivisor31ToPixel));

    const Stopwatch objectsSortingStopwatch(metric != nullptr);

    // Split input map objects
    QList< std::shared_ptr<const MapObject> > detailedmapMapObjects, basemapMapObjects;
    for (const auto& mapObject : constOf(objects))
    {
        if (controller && controller->isAborted())
            break;

        // Check if this map object is from basemap
        auto isBasemapObject = false;
        if (const auto possiblyBasemapObject = std::dynamic_pointer_cast<const BinaryMapObject>(mapObject))
        {
            isBasemapObject = possiblyBasemapObject->section->isBasemap;
            if (zoom < static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap) && !isBasemapObject)
                continue;
        }

        if (!mapObject->containsType(mapObject->encodingDecodingRules->naturalCoastline_encodingRuleId))
        {
            if (isBasemapObject)
                basemapMapObjects.push_back(mapObject);
            else
                detailedmapMapObjects.push_back(mapObject);
        }
    }
    if (controller && controller->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTimeForSortingObjects += objectsSortingStopwatch.elapsed();

    // Obtain primitives
    const bool detailedDataMissing = (zoom > static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap)) && detailedmapMapObjects.isEmpty();

    // Check if there is no data to primitivise. Report, clean-up and exit
    const auto mapObjectsCount =
        detailedmapMapObjects.size() +
        ((zoom <= static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap) || detailedDataMissing) ? basemapMapObjects.size() : 0);
    if (mapObjectsCount == 0)
    {
        // Empty area
        assert(primitivisedObjects->isEmpty());
        return primitivisedObjects;
    }

    // Obtain primitives:
    MapStyleEvaluationResult evaluationResult;

    const Stopwatch obtainPrimitivesFromDetailedmapStopwatch(metric != nullptr);
    obtainPrimitives(context, primitivisedObjects, detailedmapMapObjects, qMove(evaluationResult), cache, controller, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesFromDetailedmap += obtainPrimitivesFromDetailedmapStopwatch.elapsed();

    if ((zoom <= static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap)) || detailedDataMissing)
    {
        const Stopwatch obtainPrimitivesFromBasemapStopwatch(metric != nullptr);
        obtainPrimitives(context, primitivisedObjects, basemapMapObjects, qMove(evaluationResult), cache, controller, metric);
        if (metric)
            metric->elapsedTimeForObtainingPrimitivesFromBasemap += obtainPrimitivesFromBasemapStopwatch.elapsed();
    }

    if (controller && controller->isAborted())
        return nullptr;

    // Sort and filter primitives
    const Stopwatch sortAndFilterPrimitivesStopwatch(metric != nullptr);
    sortAndFilterPrimitives(context, primitivisedObjects);
    if (metric)
        metric->elapsedTimeForSortingAndFilteringPrimitives += sortAndFilterPrimitivesStopwatch.elapsed();

    // Obtain symbols from primitives
    const Stopwatch obtainPrimitivesSymbolsStopwatch(metric != nullptr);
    obtainPrimitivesSymbols(context, primitivisedObjects, qMove(evaluationResult), cache, controller);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesSymbols += obtainPrimitivesSymbolsStopwatch.elapsed();

    // Cleanup if aborted
    if (controller && controller->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    return primitivisedObjects;
}

OsmAnd::AreaI OsmAnd::MapPrimitiviser_P::alignAreaForCoastlines(const AreaI& area31)
{
    const auto tilesCount = static_cast<int32_t>(1u << static_cast<unsigned int>(ZoomLevel5));
    const auto alignMask = static_cast<uint32_t>(tilesCount - 1);

    // Align area to 32: this fixes coastlines and specifically Antarctica
    AreaI alignedArea31;

    alignedArea31.top() = area31.top() & ~alignMask;

    alignedArea31.left() = area31.left() & ~alignMask;

    alignedArea31.bottom() = area31.bottom() & ~alignMask;

    alignedArea31.right() = area31.right() & ~alignMask;

    return alignedArea31;
}

bool OsmAnd::MapPrimitiviser_P::polygonizeCoastlines(
    const AreaI area31,
    const ZoomLevel zoom,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
    const QList< std::shared_ptr<const MapObject> >& coastlines,
    QList< std::shared_ptr<const MapObject> >& outVectorized,
    bool abortIfBrokenCoastlinesExist,
    bool includeBrokenCoastlines)
{
    QList< QVector< PointI > > closedPolygons;
    QList< QVector< PointI > > coastlinePolylines; // Broken == not closed in this case

    // Align area to 32: this fixes coastlines and specifically Antarctica
    const auto alignedArea31 = alignAreaForCoastlines(area31);

    QVector< PointI > linePoints31;
    for (const auto& coastline : constOf(coastlines))
    {
        if (coastline->points31.size() < 2)
        {
            LogPrintf(LogSeverityLevel::Warning,
                "MapObject %s is primitivised as coastline, but has %d vertices",
                qPrintable(coastline->toString()),
                coastline->points31.size());
            continue;
        }

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
            const auto lineEnded = buildCoastlinePolygonSegment(area31, inside, cp, prevInside, pp, linePoints31);
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
        const std::shared_ptr<MapObject> mapObject(new CoastlineMapObject());
        mapObject->isArea = false;
        mapObject->points31 = polyline;
        mapObject->typesRuleIds.push_back(MapObject::defaultEncodingDecodingRules->naturalCoastlineLine_encodingRuleId);

        outVectorized.push_back(qMove(mapObject));
    }

    const bool coastlineCrossesBounds = !coastlinePolylines.isEmpty();
    if (!coastlinePolylines.isEmpty())
    {
        // Add complete water tile with holes
        const std::shared_ptr<MapObject> mapObject(new CoastlineMapObject());
        mapObject->points31.push_back(qMove(PointI(area31.left(), area31.top())));
        mapObject->points31.push_back(qMove(PointI(area31.right(), area31.top())));
        mapObject->points31.push_back(qMove(PointI(area31.right(), area31.bottom())));
        mapObject->points31.push_back(qMove(PointI(area31.left(), area31.bottom())));
        mapObject->points31.push_back(mapObject->points31.first());
        mapObject->bbox31 = area31;
        convertCoastlinePolylinesToPolygons(area31, coastlinePolylines, mapObject->innerPolygonsPoints31);

        mapObject->typesRuleIds.push_back(MapObject::defaultEncodingDecodingRules->naturalCoastline_encodingRuleId);
        mapObject->isArea = true;

        assert(mapObject->isClosedFigure());
        assert(mapObject->isClosedFigure(true));
        outVectorized.push_back(qMove(mapObject));
    }

    if (!coastlinePolylines.isEmpty())
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Invalid polylines found during primitivisation of coastlines in area [%d, %d, %d, %d]@%d",
            area31.top(),
            area31.left(),
            area31.bottom(),
            area31.right(),
            zoom);
    }

    if (includeBrokenCoastlines)
    {
        for (const auto& polygon : constOf(coastlinePolylines))
        {
            const std::shared_ptr<MapObject> mapObject(new CoastlineMapObject());
            mapObject->isArea = false;
            mapObject->points31 = polygon;
            mapObject->typesRuleIds.push_back(MapObject::defaultEncodingDecodingRules->naturalCoastlineBroken_encodingRuleId);

            outVectorized.push_back(qMove(mapObject));
        }
    }

    // Draw coastlines
    for (const auto& polygon : constOf(closedPolygons))
    {
        const std::shared_ptr<MapObject> mapObject(new CoastlineMapObject());
        mapObject->isArea = false;
        mapObject->points31 = polygon;
        mapObject->typesRuleIds.push_back(MapObject::defaultEncodingDecodingRules->naturalCoastlineLine_encodingRuleId);

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

        const std::shared_ptr<MapObject> mapObject(new CoastlineMapObject());
        mapObject->points31 = qMove(polygon);
        if (clockwise)
        {
            mapObject->typesRuleIds.push_back(MapObject::defaultEncodingDecodingRules->naturalCoastline_encodingRuleId);
            fullWaterObjects++;
        }
        else
        {
            mapObject->typesRuleIds.push_back(MapObject::defaultEncodingDecodingRules->naturalLand_encodingRuleId);
            fullLandObjects++;
        }
        mapObject->isArea = true;

        assert(mapObject->isClosedFigure());
        outVectorized.push_back(qMove(mapObject));
    }

    if (fullWaterObjects == 0u && !coastlineCrossesBounds)
    {
        LogPrintf(LogSeverityLevel::Warning, "Isolated islands found during primitivisation of coastlines in area [%d, %d, %d, %d]@%d",
            area31.top(),
            area31.left(),
            area31.bottom(),
            area31.right(),
            zoom);

        // Add complete water tile
        const std::shared_ptr<MapObject> mapObject(new CoastlineMapObject());
        mapObject->points31.push_back(qMove(PointI(area31.left(), area31.top())));
        mapObject->points31.push_back(qMove(PointI(area31.right(), area31.top())));
        mapObject->points31.push_back(qMove(PointI(area31.right(), area31.bottom())));
        mapObject->points31.push_back(qMove(PointI(area31.left(), area31.bottom())));
        mapObject->points31.push_back(mapObject->points31.first());
        mapObject->bbox31 = area31;

        mapObject->typesRuleIds.push_back(MapObject::defaultEncodingDecodingRules->naturalCoastline_encodingRuleId);
        mapObject->isArea = true;

        assert(mapObject->isClosedFigure());
        outVectorized.push_back(qMove(mapObject));
    }

    return true;
}

bool OsmAnd::MapPrimitiviser_P::buildCoastlinePolygonSegment(
    const AreaI area31,
    bool currentInside,
    const PointI& currentPoint31,
    bool prevInside,
    const PointI& previousPoint31,
    QVector< PointI >& segmentPoints)
{
    bool lineEnded = false;

    const auto alignedArea31 = alignAreaForCoastlines(area31);

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

bool OsmAnd::MapPrimitiviser_P::calculateIntersection(const PointI& p1, const PointI& p0, const AreaI& bbox, PointI& pX)
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

void OsmAnd::MapPrimitiviser_P::appendCoastlinePolygons(
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

void OsmAnd::MapPrimitiviser_P::convertCoastlinePolylinesToPolygons(
    const AreaI area31,
    QList< QVector< PointI > >& coastlinePolylines,
    QList< QVector< PointI > >& coastlinePolygons)
{
    const auto alignedArea31 = alignAreaForCoastlines(area31);

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

bool OsmAnd::MapPrimitiviser_P::isClockwiseCoastlinePolygon(const QVector< PointI > & polygon)
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

void OsmAnd::MapPrimitiviser_P::obtainPrimitives(
    const Context& context,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
    const QList< std::shared_ptr<const OsmAnd::MapObject> >& source,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    const std::shared_ptr<Cache>& cache,
    const IQueryController* const controller,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& env = context.env;
    const auto zoom = primitivisedObjects->zoom;

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

        MapObject::SharingKey sharingKey;
        const auto isShareable = mapObject->obtainSharingKey(sharingKey);

        // If group can be shared, use already-processed or reserve pending
        if (pSharedPrimitivesGroups && isShareable)
        {
            // If this group was already processed, use that
            std::shared_ptr<const PrimitivesGroup> group;
            proper::shared_future< std::shared_ptr<const PrimitivesGroup> > futureGroup;
            if (pSharedPrimitivesGroups->obtainReferenceOrFutureReferenceOrMakePromise(sharingKey, group, futureGroup))
            {
                if (group)
                {
                    // Add polygons, polylines and points from group to current context
                    primitivisedObjects->polygons.append(group->polygons);
                    primitivisedObjects->polylines.append(group->polylines);
                    primitivisedObjects->points.append(group->points);

                    // Add shared group to current context
                    primitivisedObjects->primitivesGroups.push_back(qMove(group));
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
            context,
            primitivisedObjects,
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
        if (pSharedPrimitivesGroups && isShareable)
            pSharedPrimitivesGroups->fulfilPromiseAndReference(sharingKey, group);

        // Add polygons, polylines and points from group to current context
        primitivisedObjects->polygons.append(group->polygons);
        primitivisedObjects->polylines.append(group->polylines);
        primitivisedObjects->points.append(group->points);

        // Empty groups are also inserted, to indicate that they are empty
        primitivisedObjects->primitivesGroups.push_back(qMove(group));
    }

    // Wait for future primitives groups
    Stopwatch futureSharedPrimitivesGroupsStopwatch(metric != nullptr);
    for (auto& futureSharedGroup : futureSharedPrimitivesGroups)
    {
        auto group = futureSharedGroup.get();

        // Add polygons, polylines and points from group to current context
        primitivisedObjects->polygons.append(group->polygons);
        primitivisedObjects->polylines.append(group->polylines);
        primitivisedObjects->points.append(group->points);

        // Add shared group to current context
        primitivisedObjects->primitivesGroups.push_back(qMove(group));
    }
    if (metric)
        metric->elapsedTimeForFutureSharedPrimitivesGroups += futureSharedPrimitivesGroupsStopwatch.elapsed();
}

std::shared_ptr<const OsmAnd::MapPrimitiviser_P::PrimitivesGroup> OsmAnd::MapPrimitiviser_P::obtainPrimitivesGroup(
    const Context& context,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
    const std::shared_ptr<const MapObject>& mapObject,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    MapStyleEvaluator& orderEvaluator,
    MapStyleEvaluator& polygonEvaluator,
    MapStyleEvaluator& polylineEvaluator,
    MapStyleEvaluator& pointEvaluator,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& env = context.env;

    bool ok;

    const auto constructedGroup = new PrimitivesGroup(mapObject);
    std::shared_ptr<const PrimitivesGroup> group(constructedGroup);

    //////////////////////////////////////////////////////////////////////////
    //if ((mapObject->id >> 1) == 192124575u)
    //{
    //    int i = 5;
    //}
    //else
    //    return group;
    //////////////////////////////////////////////////////////////////////////

    uint32_t typeRuleIdIndex = 0;
    const auto& decRules = mapObject->encodingDecodingRules->decodingRules;
    for (auto itTypeRuleId = cachingIteratorOf(constOf(mapObject->typesRuleIds)); itTypeRuleId; ++itTypeRuleId, typeRuleIdIndex++)
    {
        const auto& decodedType = decRules[*itTypeRuleId];

        //////////////////////////////////////////////////////////////////////////
        //if ((mapObject->id >> 1) == 9223371929479601886)
        //{
        //    if (decodedType.tag != QLatin1String("building"))
        //        continue;
        //    int i = 5;
        //}
        //if (decodedType.value != QLatin1String("pedestrian"))
        //    continue;
        //if (decodedType.tag == QLatin1String("barrier") && decodedType.value == QLatin1String("city_wall"))
        //{
        //    int i = 5;
        //}
        //////////////////////////////////////////////////////////////////////////

        const Stopwatch orderEvaluationStopwatch(metric != nullptr);

        // Setup mapObject-specific input data
        orderEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
        orderEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);
        orderEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_LAYER, static_cast<int>(mapObject->getLayerType()));
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

        int zOrder = -1;
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
                    "MapObject %s primitive is processed as polygon, but has only %d point(s)",
                    qPrintable(mapObject->toString()),
                    mapObject->points31.size());
                continue;
            }
            if (!mapObject->isClosedFigure())
            {
                LogPrintf(LogSeverityLevel::Warning,
                    "MapObject %s primitive is processed as polygon, but isn't closed",
                    qPrintable(mapObject->toString()));
                continue;
            }
            if (!mapObject->isClosedFigure(true))
            {
                LogPrintf(LogSeverityLevel::Warning,
                    "MapObject %s primitive is processed as polygon, but isn't closed (inner)",
                    qPrintable(mapObject->toString()));
                continue;
            }

            //////////////////////////////////////////////////////////////////////////
            //if ((mapObject->id >> 1) == 266877135u)
            //{
            //    int i = 5;
            //}
            //////////////////////////////////////////////////////////////////////////

            // Check size of polygon
            const auto doubledPolygonArea31 = Utilities::doubledPolygonArea(mapObject->points31);
            const auto polygonArea31 = static_cast<double>(doubledPolygonArea31)* 0.5;
            const auto polygonAreaInPixels = polygonArea31 / (primitivisedObjects->scaleDivisor31ToPixel.x * primitivisedObjects->scaleDivisor31ToPixel.y);
            const auto polygonAreaInAbstractPixels = polygonAreaInPixels / (env->displayDensityFactor * env->displayDensityFactor);
            if (primitivisedObjects->scaleDivisor31ToPixel.x >= 0.0 &&
                primitivisedObjects->scaleDivisor31ToPixel.y >= 0.0 &&
                polygonAreaInAbstractPixels <= context.polygonAreaMinimalThreshold)
            {
                if (metric)
                    metric->polygonsRejectedByArea++;

                continue;
            }

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
                primitive->zOrder = (std::dynamic_pointer_cast<const SurfaceMapObject>(mapObject) || std::dynamic_pointer_cast<const CoastlineMapObject>(mapObject))
                    ? std::numeric_limits<int>::min()
                    : zOrder;
                primitive->doubledArea = doubledPolygonArea31;

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

            // Create point primitive only in case polygon has any content
            if (!mapObject->captions.isEmpty() || hasIcon)
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
                pointPrimitive->zOrder = (std::dynamic_pointer_cast<const SurfaceMapObject>(mapObject) || std::dynamic_pointer_cast<const CoastlineMapObject>(mapObject))
                    ? std::numeric_limits<int>::min()
                    : zOrder;
                pointPrimitive->doubledArea = doubledPolygonArea31;

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
                    "MapObject %s is processed as polyline, but has %d point(s)",
                    qPrintable(mapObject->toString()),
                    mapObject->points31.size());
                continue;
            }

            const Stopwatch polylineEvaluationStopwatch(metric != nullptr);

            // Setup mapObject-specific input data
            polylineEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
            polylineEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);
            polylineEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_LAYER, static_cast<int>(mapObject->getLayerType()));

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
                    "MapObject %s is processed as point, but has no point",
                    qPrintable(mapObject->toString()));
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

            // Create point primitive only in case polygon has any content
            if (mapObject->captions.isEmpty() && !hasIcon)
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

        //if (shadowLevel > 0)
        //{
        //    primitivisedObjects->shadowLevelMin = qMin(primitivisedObjects->shadowLevelMin, static_cast<int>(zOrder));
        //    primitivisedObjects->shadowLevelMax = qMax(primitivisedObjects->shadowLevelMax, static_cast<int>(zOrder));
        //}
    }

    return group;
}

void OsmAnd::MapPrimitiviser_P::sortAndFilterPrimitives(
    const Context& context,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects)
{
    const MapObject::Comparator mapObjectsComparator;
    const auto privitivesSort =
        [mapObjectsComparator]
        (const std::shared_ptr<const Primitive>& l, const std::shared_ptr<const Primitive>& r) -> bool
        {
            // Sort by zOrder first
            if (l->zOrder != r->zOrder)
                return l->zOrder < r->zOrder;

            // Then sort by area
            if (l->doubledArea != r->doubledArea)
                return l->doubledArea > r->doubledArea;

            // Then sort by tag=value ordering (this is possible only inside same object)
            if (l->typeRuleIdIndex != r->typeRuleIdIndex && l->sourceObject == r->sourceObject)
            {
                if (l->type == PrimitiveType::Polygon)
                    return l->typeRuleIdIndex > r->typeRuleIdIndex;
                return l->typeRuleIdIndex < r->typeRuleIdIndex;
            }

            // Then sort by number of points
            const auto lPointsCount = l->sourceObject->points31.size();
            const auto rPointsCount = r->sourceObject->points31.size();
            if (lPointsCount != rPointsCount)
                return lPointsCount < rPointsCount;

            // Sort by map object sorting key, if map objects are equal
            return mapObjectsComparator(l->sourceObject, r->sourceObject);
        };

    qSort(primitivisedObjects->polygons.begin(), primitivisedObjects->polygons.end(), privitivesSort);
    qSort(primitivisedObjects->polylines.begin(), primitivisedObjects->polylines.end(), privitivesSort);
    filterOutHighwaysByDensity(context, primitivisedObjects);
    qSort(primitivisedObjects->points.begin(), primitivisedObjects->points.end(), privitivesSort);
}

void OsmAnd::MapPrimitiviser_P::filterOutHighwaysByDensity(
    const Context& context,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects)
{
    // Check if any filtering needed
    if (context.roadDensityZoomTile == 0 || context.roadsDensityLimitPerTile == 0)
        return;

    const auto dZ = primitivisedObjects->zoom + context.roadDensityZoomTile;
    QHash< uint64_t, std::pair<uint32_t, double> > densityMap;

    auto itLine = mutableIteratorOf(primitivisedObjects->polylines);
    itLine.toBack();
    while (itLine.hasPrevious())
    {
        const auto& line = itLine.previous();

        auto accept = true;
        if (line->sourceObject->typesRuleIds[line->typeRuleIdIndex] == line->sourceObject->encodingDecodingRules->highway_encodingRuleId)
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
                    if (mapEntry.first < context.roadsDensityLimitPerTile /*&& p.second > o */)
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

void OsmAnd::MapPrimitiviser_P::obtainPrimitivesSymbols(
    const Context& context,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    const std::shared_ptr<Cache>& cache,
    const IQueryController* const controller)
{
    //NOTE: Em, I'm not sure this is still true
    //NOTE: Since 2 tiles with same MapObject may have different set of polylines, generated from it,
    //NOTE: then set of symbols also should differ, but it won't.
    const auto pSharedSymbolGroups = cache ? cache->getSymbolsGroupsPtr(primitivisedObjects->zoom) : nullptr;
    QList< proper::shared_future< std::shared_ptr<const SymbolsGroup> > > futureSharedSymbolGroups;
    for (const auto& primitivesGroup : constOf(primitivisedObjects->primitivesGroups))
    {
        if (controller && controller->isAborted())
            return;

        // If using shared context is allowed, check if this group was already processed
        // (using shared cache is only allowed for non-generated MapObjects),
        // then symbols group can be shared
        MapObject::SharingKey sharingKey;
        const auto canBeShared = primitivesGroup->sourceObject->obtainSharingKey(sharingKey);

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
            if (pSharedSymbolGroups->obtainReferenceOrFutureReferenceOrMakePromise(sharingKey, group, futureGroup))
            {
                if (group)
                {
                    // Add shared group to current context
                    assert(!primitivisedObjects->symbolsGroups.contains(group->sourceObject));
                    primitivisedObjects->symbolsGroups.insert(group->sourceObject, qMove(group));
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
            context,
            primitivisedObjects,
            primitivesGroup->polygons,
            PrimitivesType::Polygons,
            qMove(evaluationResult),
            constructedGroup->symbols,
            controller);
        */
        collectSymbolsFromPrimitives(
            context,
            primitivisedObjects,
            primitivesGroup->polylines,
            PrimitivesType::Polylines,
            qMove(evaluationResult),
            constructedGroup->symbols,
            controller);
        collectSymbolsFromPrimitives(
            context,
            primitivisedObjects,
            primitivesGroup->points,
            PrimitivesType::Points,
            qMove(evaluationResult),
            constructedGroup->symbols,
            controller);

        // Add this group to shared cache
        if (pSharedSymbolGroups && canBeShared)
            pSharedSymbolGroups->fulfilPromiseAndReference(sharingKey, group);

        // Empty groups are also inserted, to indicate that they are empty
        assert(!primitivisedObjects->symbolsGroups.contains(group->sourceObject));
        primitivisedObjects->symbolsGroups.insert(group->sourceObject, qMove(group));
    }

    for (auto& futureGroup : futureSharedSymbolGroups)
    {
        auto group = futureGroup.get();

        // Add shared group to current context
        assert(!primitivisedObjects->symbolsGroups.contains(group->sourceObject));
        primitivisedObjects->symbolsGroups.insert(group->sourceObject, qMove(group));
    }
}

void OsmAnd::MapPrimitiviser_P::collectSymbolsFromPrimitives(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
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
            obtainSymbolsFromPolygon(
                context,
                primitivisedObjects,
                primitive,
                qMove(evaluationResult),
                outSymbols);
        }
        else if (type == PrimitivesType::Polylines)
        {
            obtainSymbolsFromPolyline(
                context,
                primitivisedObjects,
                primitive,
                qMove(evaluationResult),
                outSymbols);
        }
        else if (type == PrimitivesType::Points)
        {
            obtainSymbolsFromPoint(
                context, 
               primitivisedObjects,
               primitive,
               qMove(evaluationResult),
               outSymbols);
        }
    }
}

void OsmAnd::MapPrimitiviser_P::obtainSymbolsFromPolygon(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
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
    obtainPrimitiveTexts(
        context,
        primitivisedObjects,
        primitive,
        Utilities::normalizeCoordinates(center, ZoomLevel31),
        qMove(evaluationResult),
        outSymbols);
}

void OsmAnd::MapPrimitiviser_P::obtainSymbolsFromPolyline(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
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
        context,
        primitivisedObjects,
        primitive,
        center,
        qMove(evaluationResult),
        outSymbols);
}

void OsmAnd::MapPrimitiviser_P::obtainSymbolsFromPoint(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
    const std::shared_ptr<const Primitive>& primitive,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    SymbolsCollection& outSymbols)
{
    //////////////////////////////////////////////////////////////////////////
    //if ((primitive->sourceObject->id >> 1) == 9223090561878064380u)
    //{
    //    int i = 5;
    //}
    //else
    //    return;
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

    // Obtain icon for this symbol (only if there's no icon yet)
    const auto alreadyContainsIcon = std::find_if(outSymbols.cbegin(), outSymbols.cend(),
        []
    (const std::shared_ptr<const Symbol>& symbol) -> bool
    {
        if (std::dynamic_pointer_cast<const IconSymbol>(symbol))
            return true;
        return false;
    }) != outSymbols.cend();
    if (!alreadyContainsIcon)
    {
        obtainPrimitiveIcon(
            context,
            primitive,
            center,
            qMove(evaluationResult),
            outSymbols);
    }

    // Obtain texts for this symbol
    //HACK: (only in case it's first tag)
    if (primitive->typeRuleIdIndex == 0)
    {
        obtainPrimitiveTexts(
            context,
            primitivisedObjects,
            primitive,
            center,
            qMove(evaluationResult),
            outSymbols);
    }
}

void OsmAnd::MapPrimitiviser_P::obtainPrimitiveTexts(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
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
    const auto& env = context.env;

    //////////////////////////////////////////////////////////////////////////
    //if (primitive->sourceObject->toString() == "OSM-node 1705951317 [3411902634]")
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    // Text symbols can only be obtained from captions
    if (mapObject->captions.isEmpty())
        return;

    const auto& encDecRules = mapObject->encodingDecodingRules;
    const auto typeRuleId = mapObject->typesRuleIds[primitive->typeRuleIdIndex];
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
        hasNativeName = (citNativeName != citCaptionsEnd);
        auto nativeNameOrder = hasNativeName
            ? captionsOrder.indexOf(citNativeName.key())
            : -1;

        // Look for localized name
        const auto citLocalizedNameRuleId = encDecRules->localizedName_encodingRuleIds.constFind(env->localeLanguageId);
        if (citLocalizedNameRuleId != encDecRules->localizedName_encodingRuleIds.cend())
            localizedNameRuleId = *citLocalizedNameRuleId;
        const auto citLocalizedName =
            (localizedNameRuleId == std::numeric_limits<uint32_t>::max())
            ? citCaptionsEnd
            : captions.constFind(localizedNameRuleId);
        hasLocalizedName = (citLocalizedName != citCaptionsEnd);
        auto localizedNameOrder = hasLocalizedName
            ? captionsOrder.indexOf(citLocalizedName.key())
            : -1;

        // According to presentation settings, adjust set of captions
        switch (env->languagePreference)
        {
            case MapPresentationEnvironment::LanguagePreference::NativeOnly:
            {
                // Remove localized name if such exists
                if (hasLocalizedName)
                {
                    captions.remove(localizedNameRuleId);
                    captionsOrder.removeOne(localizedNameRuleId);
                    hasLocalizedName = false;
                    localizedNameOrder = -1;
                }
                break;
            }

            case MapPresentationEnvironment::LanguagePreference::LocalizedOrNative:
            {
                // Only one should be shown, thus remove native if localized exist
                if (hasLocalizedName && hasNativeName)
                {
                    captions.remove(encDecRules->name_encodingRuleId);
                    captionsOrder.removeOne(encDecRules->name_encodingRuleId);
                    hasNativeName = false;
                    nativeNameOrder = -1;
                }
                break;
            }

            case MapPresentationEnvironment::LanguagePreference::NativeAndLocalized:
            {
                // Both should be shown if available, and in correct order
                if (hasNativeName && hasLocalizedName && localizedNameOrder < nativeNameOrder)
                {
                    captionsOrder.swap(localizedNameOrder, nativeNameOrder);
                    qSwap(localizedNameOrder, nativeNameOrder);
                }
                break;
            }

            case MapPresentationEnvironment::LanguagePreference::NativeAndLocalizedOrTransliterated:
            {
                // Both should be present, so add transliterated in case localized is missing, and in correct order
                if (hasNativeName && hasLocalizedName && localizedNameOrder < nativeNameOrder)
                {
                    captionsOrder.swap(localizedNameOrder, nativeNameOrder);
                    qSwap(localizedNameOrder, nativeNameOrder);
                }
                else if (hasNativeName && !hasLocalizedName)
                {
                    const auto latinNameValue = ICU::transliterateToLatin(citNativeName.value());

                    captions.insert(localizedNameRuleId, latinNameValue);
                    localizedNameOrder = captionsOrder.indexOf(encDecRules->name_encodingRuleId) + 1;
                    captionsOrder.insert(localizedNameOrder, localizedNameRuleId);
                    hasLocalizedName = true;
                }
                break;
            }

            case MapPresentationEnvironment::LanguagePreference::LocalizedAndNative:
            {
                // Both should be shown if available, and in correct order
                if (hasLocalizedName && hasNativeName && nativeNameOrder < localizedNameOrder)
                {
                    captionsOrder.swap(nativeNameOrder, localizedNameOrder);
                    qSwap(nativeNameOrder, localizedNameOrder);
                }
                break;
            }

            case MapPresentationEnvironment::LanguagePreference::LocalizedOrTransliteratedAndNative:
            {
                // Both should be present, so add transliterated in case localized is missing, and in correct order
                if (hasLocalizedName && hasNativeName && nativeNameOrder < localizedNameOrder)
                {
                    captionsOrder.swap(nativeNameOrder, localizedNameOrder);
                    qSwap(nativeNameOrder, localizedNameOrder);
                }
                else if (!hasLocalizedName && hasNativeName)
                {
                    const auto latinNameValue = ICU::transliterateToLatin(citNativeName.value());

                    captions.insert(localizedNameRuleId, latinNameValue);
                    localizedNameOrder = captionsOrder.indexOf(encDecRules->name_encodingRuleId);
                    captionsOrder.insert(localizedNameOrder, localizedNameRuleId);
                    hasLocalizedName = true;
                }
                break;
            }

            default:
                assert(false);
                return;
        }

        // In case both languages are present, but they are equal (without accents and diacritics)
        if (hasNativeName && hasLocalizedName)
        {
            const auto pureNativeName = ICU::stripAccentsAndDiacritics(citNativeName.value());
            const auto pureLocalizedName = ICU::stripAccentsAndDiacritics(citLocalizedName.value());

            if (pureNativeName.compare(pureLocalizedName, Qt::CaseInsensitive) == 0)
            {
                // Keep first one
                if (nativeNameOrder < localizedNameOrder)
                {
                    captions.remove(localizedNameRuleId);
                    captionsOrder.removeOne(localizedNameRuleId);
                    hasLocalizedName = false;
                    localizedNameOrder = -1;
                }
                else // if (localizedNameOrder < nativeNameOrder)
                {
                    captions.remove(encDecRules->name_encodingRuleId);
                    captionsOrder.removeOne(encDecRules->name_encodingRuleId);
                    hasNativeName = false;
                    nativeNameOrder = -1;
                }
            }
        }
    }

    // Process captions in order how captions where declared in OBF source (what is being controlled by 'rendering_types.xml')
    bool ok;
    bool extraCaptionTextAdded = false;
    for (const auto& captionRuleId : constOf(captionsOrder))
    {
        const auto& caption = constOf(captions)[captionRuleId];

        // If it's empty, it can not be primitivised
        if (caption.isEmpty())
            continue;

        // Skip captions that are from 'name[:*]'-tags but not of 'native' or 'localized' language
        if (captionRuleId != encDecRules->name_encodingRuleId &&
            captionRuleId != localizedNameRuleId &&
            encDecRules->namesRuleId.contains(captionRuleId))
        {
            continue;
        }

        // Evaluate style to obtain text parameters
        textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
        textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);
        textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, primitivisedObjects->zoom);
        textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, primitivisedObjects->zoom);
        textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_TEXT_LENGTH, caption.length());

        // Get tag of rule, in case caption is not from a 'name[:*]'-tag
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
        if (hasNativeName && captionRuleId == encDecRules->name_encodingRuleId)
            languageId = LanguageId::Native;
        else if (hasLocalizedName && captionRuleId == localizedNameRuleId)
            languageId = LanguageId::Localized;

        // Create primitive
        const std::shared_ptr<TextSymbol> text(new TextSymbol(primitive));
        text->location31 = location;
        text->languageId = languageId;
        text->value = caption;

        // Get additional text from nameTag2 if present (and not yet added)
        if (!extraCaptionTextAdded)
        {
            QString nameTag2;
            ok = evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_NAME_TAG2, nameTag2);
            if (ok && !nameTag2.isEmpty())
            {
                const auto citNameTag2RulesGroup = encDecRules->encodingRuleIds.constFind(nameTag2);
                if (citNameTag2RulesGroup != encDecRules->encodingRuleIds.constEnd())
                {
                    const auto& nameTag2RulesGroup = *citNameTag2RulesGroup;
                    for (const auto& nameTag2RuleEntry : rangeOf(constOf(nameTag2RulesGroup)))
                    {
                        const auto& ruleId = nameTag2RuleEntry.value();
                        const auto citExtraCaption = mapObject->captions.constFind(ruleId);

                        if (citExtraCaption == mapObject->captions.constEnd())
                            continue;
                        const auto& extraCaption = *citExtraCaption;
                        if (extraCaption.isEmpty())
                            continue;

                        text->value += QString(QLatin1String(" (%1)")).arg(extraCaption);

                        extraCaptionTextAdded = true;
                        break;
                    }
                }
            }
        }

        // In case text is from polyline primitive, it's drawn along-path if not on-path
        if (primitive->type == PrimitiveType::Polyline)
        {
            evaluationResult.getBooleanValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_ON_PATH, text->drawOnPath);
            text->drawAlongPath = !text->drawOnPath;
        }

        // By default, text order is treated as 100
        text->order = 100;
        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_ORDER, text->order);
        //NOTE: a magic shifting of text order. This is needed to keep text more important than anything else
        text->order += 100000;

        if (primitive->type == PrimitiveType::Point ||
            primitive->type == PrimitiveType::Polygon)
        {
            evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_DY, text->verticalOffset);
        }

        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_COLOR, text->color.argb);
        if (!ok || text->color == ColorARGB::fromSkColor(SK_ColorTRANSPARENT))
            text->color = ColorARGB::fromSkColor(SK_ColorBLACK);

        text->size = textSize;

        evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_HALO_RADIUS, text->shadowRadius);

        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_HALO_COLOR, text->shadowColor.argb);
        if (!ok || text->shadowColor == ColorARGB::fromSkColor(SK_ColorTRANSPARENT))
            text->shadowColor = ColorARGB::fromSkColor(SK_ColorWHITE);

        evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_WRAP_WIDTH, text->wrapWidth);
        if (!text->drawOnPath && text->wrapWidth == 0)
        {
            // Default wrapping width (in characters)
            text->wrapWidth = MapPrimitiviser::DefaultTextLabelWrappingLengthInCharacters;
        }

        evaluationResult.getBooleanValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_BOLD, text->isBold);
        evaluationResult.getBooleanValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_ITALIC, text->isItalic);

        evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_MIN_DISTANCE, text->minDistance);

        evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_SHIELD, text->shieldResourceName);

        QString intersectsWith;
        ok = evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_INTERSECTS_WITH, intersectsWith);
        if (!ok)
            intersectsWith = QLatin1String("text"); // To simulate original behavior, texts should intersect only other texts
        text->intersectsWith = intersectsWith.split(QLatin1Char(','), QString::SkipEmptyParts).toSet();

        float intersectionSizeFactor = std::numeric_limits<float>::quiet_NaN();
        ok = evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_INTERSECTION_SIZE_FACTOR, intersectionSizeFactor);
        if (ok)
            text->intersectionSizeFactor = intersectionSizeFactor;

        float intersectionSize = std::numeric_limits<float>::quiet_NaN();
        ok = !ok && evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_INTERSECTION_SIZE, intersectionSize);
        if (ok)
            text->intersectionSize = intersectionSize;

        float intersectionMargin = std::numeric_limits<float>::quiet_NaN();
        ok = !ok && evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_INTERSECTION_MARGIN, intersectionMargin);
        if (ok)
            text->intersectionMargin = intersectionMargin;

        text->pathPaddingLeft = context.defaultPathPaddingLeft;
        text->pathPaddingRight = context.defaultPathPaddingRight;
        float pathPadding = 0.0f;
        ok = evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_PATH_PADDING, pathPadding);
        if (ok)
            text->pathPaddingLeft = text->pathPaddingRight = pathPadding;
        evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_PATH_PADDING_LEFT, text->pathPaddingLeft);
        evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_PATH_PADDING_RIGHT, text->pathPaddingRight);

        outSymbols.push_back(qMove(text));
    }
}

void OsmAnd::MapPrimitiviser_P::obtainPrimitiveIcon(
    const Context& context,
    const std::shared_ptr<const Primitive>& primitive,
    const PointI& location,
#ifdef Q_COMPILER_RVALUE_REFS
    MapStyleEvaluationResult&& evaluationResult,
#else
    MapStyleEvaluationResult& evaluationResult,
#endif // Q_COMPILER_RVALUE_REFS
    SymbolsCollection& outSymbols)
{
    const auto& env = context.env;

    //////////////////////////////////////////////////////////////////////////
    //if (primitive->sourceObject->toString().contains("2380290737"))
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

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

    evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_MIN_DISTANCE, icon->minDistance);

    primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_SHIELD, icon->shieldResourceName);

    QString intersectsWith;
    ok = primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_INTERSECTS_WITH, intersectsWith);
    if (!ok)
        intersectsWith = QLatin1String("icon"); // To simulate original behavior, icons should intersect only other icons
    icon->intersectsWith = intersectsWith.split(QLatin1Char(','), QString::SkipEmptyParts).toSet();

    float intersectionSizeFactor = std::numeric_limits<float>::quiet_NaN();
    ok = primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_INTERSECTION_SIZE_FACTOR, intersectionSizeFactor);
    if (ok)
        icon->intersectionSizeFactor = intersectionSizeFactor;

    float intersectionSize = std::numeric_limits<float>::quiet_NaN();
    ok = !ok && primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_INTERSECTION_SIZE, intersectionSize);
    if (ok)
        icon->intersectionSize = intersectionSize;

    float intersectionMargin = std::numeric_limits<float>::quiet_NaN();
    ok = !ok && primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_INTERSECTION_MARGIN, intersectionMargin);
    if (ok)
        icon->intersectionMargin = intersectionMargin;

    //TODO: Obsolete
    if (!ok)
    {
        float iconIntersectionSize = std::numeric_limits<float>::quiet_NaN();
        ok = primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_INTERSECTION_SIZE, iconIntersectionSize);
        if (ok)
            icon->intersectionSize = iconIntersectionSize;
    }

    icon->pathPaddingLeft = context.defaultPathPaddingLeft;
    icon->pathPaddingRight = context.defaultPathPaddingRight;
    float pathPadding = 0.0f;
    ok = primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_PATH_PADDING, pathPadding);
    if (ok)
        icon->pathPaddingLeft = icon->pathPaddingRight = pathPadding;
    primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_PATH_PADDING_LEFT, icon->pathPaddingLeft);
    primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_OR_ICON_PATH_PADDING_RIGHT, icon->pathPaddingRight);

    // Icons are always first
    outSymbols.prepend(qMove(icon));
}

OsmAnd::MapPrimitiviser_P::Context::Context(
    const std::shared_ptr<const MapPresentationEnvironment>& env_,
    const ZoomLevel zoom_)
    : env(env_)
    , zoom(zoom_)
{
    polygonAreaMinimalThreshold = env->getPolygonAreaMinimalThreshold(zoom);
    roadDensityZoomTile = env->getRoadDensityZoomTile(zoom);
    roadsDensityLimitPerTile = env->getRoadsDensityLimitPerTile(zoom);
    env->obtainDefaultPathPadding(defaultPathPaddingLeft, defaultPathPaddingRight);
}
