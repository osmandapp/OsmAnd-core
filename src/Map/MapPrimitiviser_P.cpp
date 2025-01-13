#include "MapPrimitiviser_P.h"
#include "MapPrimitiviser.h"

#include "stdlib_common.h"
#include <proper/future.h>
#include <set>
#include <algorithm>
#include <memory>

#include "QtExtensions.h"
#include "QtCommon.h"

#include "Nullable.h"
#include "ICU.h"
#include "MapStyleEvaluator.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleBuiltinValueDefinitions.h"
#include "ObfMapSectionInfo.h"
#include "MapObject.h"
#include "BinaryMapObject.h"
#include "Road.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "QKeyValueIterator.h"
#include "QCachingIterator.h"
#include "Logging.h"
#include "multipolygons.h"

//#define OSMAND_VERBOSE_MAP_PRIMITIVISER 1
#if !defined(OSMAND_VERBOSE_MAP_PRIMITIVISER)
#   define OSMAND_VERBOSE_MAP_PRIMITIVISER 0
#endif // !defined(OSMAND_VERBOSE_MAP_PRIMITIVISER)

// Most polylines width is under 50 meters
#define MAX_ENLARGE_PRIMITIVIZED_AREA_METERS 50
#define ENLARGE_PRIMITIVIZED_AREA_COEFF 0.2f

#define DEFAULT_TEXT_MIN_DISTANCE 150

// If object's primitive is polygon and order is > 10, add it to polylines list
// If object's primitive is polyline and order < 11, add it to polygons list
#define MAX_POLYGON_ORDER_IN_POLYGONS_LIST 10
#define MIN_POLYLINE_ORDER_IN_POLYLINES_LIST (MAX_POLYGON_ORDER_IN_POLYGONS_LIST + 1)

// Filtering grid dimension in cells per tile side
#define GRID_CELLS_PER_TILESIDE 32.0

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
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric)
{
    return primitiviseAllMapObjects(
        PointD(-1.0, -1.0),
        zoom,
        TileId::fromXY(-1, -1),
        objects,
        cache,
        queryController,
        metric);
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseAllMapObjects(
    const ZoomLevel zoom,
    const TileId tileId,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric)
{
    return primitiviseAllMapObjects(
        PointD(-1.0, -1.0),
        zoom,
        tileId,
        objects,
        cache,
        queryController,
        metric);
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseAllMapObjects(
    const PointD scaleDivisor31ToPixel,
    const ZoomLevel zoom,
    const TileId tileId,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
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
    MapStyleEvaluationResult evaluationResult(context.env->mapStyle->getValueDefinitionsCount());

    const Stopwatch obtainPrimitivesStopwatch(metric != nullptr);
    obtainPrimitives(context, primitivisedObjects, objects, evaluationResult, cache, queryController, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitives += obtainPrimitivesStopwatch.elapsed();
    
    if (queryController && queryController->isAborted())
        return nullptr;

    // Sort and filter primitives
    const Stopwatch sortAndFilterPrimitivesStopwatch(metric != nullptr);
    sortAndFilterPrimitives(context, primitivisedObjects, metric);
    if (metric)
        metric->elapsedTimeForSortingAndFilteringPrimitives += sortAndFilterPrimitivesStopwatch.elapsed();

    // Obtain symbols from primitives
    const Stopwatch obtainPrimitivesSymbolsStopwatch(metric != nullptr);
    obtainPrimitivesSymbols(context, primitivisedObjects, tileId, evaluationResult, cache, queryController, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesSymbols += obtainPrimitivesSymbolsStopwatch.elapsed();

    // Cleanup if aborted
    if (queryController && queryController->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    return primitivisedObjects;
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseWithSurface(
    const AreaI area31,
    const PointI areaSizeInPixels,
    const ZoomLevel zoom,
    const TileId tileId,
    const MapSurfaceType surfaceType_,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitiviseWithSurface* const metric)
{
    const Stopwatch totalStopwatch(metric != nullptr);
    const Context context(owner->environment, zoom);
    const std::shared_ptr<PrimitivisedObjects> primitivisedObjects(new PrimitivisedObjects(
        owner->environment,
        cache,
        zoom,
        Utilities::getScaleDivisor31ToPixel(areaSizeInPixels, zoom)));

    const Stopwatch objectsSortingStopwatch(metric != nullptr);

    // Split input map objects to object, coastline, basemapObjects and basemapCoastline
    QList< std::shared_ptr<const MapObject> > detailedmapMapObjects;
    QList< std::shared_ptr<const MapObject> > detailedmapCoastlineObjects;
    QList< std::shared_ptr<const MapObject> > basemapMapObjects;
    QList< std::shared_ptr<const MapObject> > basemapCoastlineObjects;
    QList< std::shared_ptr<const MapObject> > extraCoastlineObjects;
    bool detailedBinaryMapObjectsPresent = false;
    bool roadsPresent = false;
    int contourLinesObjectsCount = 0;

    // Enlarge area in which objects will be primitivised. It allows to properly draw polylines on tile bounds
    const auto enlarge31X = qMin(
        Utilities::metersToX31(MAX_ENLARGE_PRIMITIVIZED_AREA_METERS),
        static_cast<int64_t>(area31.width() * ENLARGE_PRIMITIVIZED_AREA_COEFF));
    const auto enlarge31Y = qMin(
        Utilities::metersToY31(MAX_ENLARGE_PRIMITIVIZED_AREA_METERS),
        static_cast<int64_t>(area31.height() * ENLARGE_PRIMITIVIZED_AREA_COEFF));
    const auto enlargedArea31 = area31.getEnlargedBy(PointI(enlarge31X, enlarge31Y));

    for (const auto& mapObject : constOf(objects))
    {
        if (queryController && queryController->isAborted())
            break;

        // Check overscaled coastline objects of 13 zoom
        if (mapObject->isCoastline)
        {
            const auto binaryMapObject = std::dynamic_pointer_cast<const BinaryMapObject>(mapObject);
            if (binaryMapObject)
            {
                extraCoastlineObjects.push_back(mapObject);
            }
            continue;
        }
        
        if(!mapObject->intersectedOrContainedBy(enlargedArea31) &&
           !mapObject->containsAttribute(mapObject->attributeMapping->naturalCoastlineAttributeId))
        {
            continue;
        }

        // Check origin of map object
        auto isBasemapObject = false;
        auto isContourLinesObject = false;
        if (const auto binaryMapObject = std::dynamic_pointer_cast<const BinaryMapObject>(mapObject))
        {
            isBasemapObject = binaryMapObject->section->isBasemap;
            isContourLinesObject = binaryMapObject->section->isContourLines;
            if(!isBasemapObject) 
            {
                detailedBinaryMapObjectsPresent = true;
            }
        }
        else if (const auto road = std::dynamic_pointer_cast<const Road>(mapObject))
        {
            roadsPresent = true;
        }

        if (mapObject->containsAttribute(mapObject->attributeMapping->naturalCoastlineAttributeId))
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
            {
                detailedmapMapObjects.push_back(mapObject);
                if (isContourLinesObject)
                    contourLinesObjectsCount++;
            }
        }
    }
    
    bool hasContourLinesObjectOnly = contourLinesObjectsCount == detailedmapMapObjects.size();
    
    if (queryController && queryController->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTimeForSortingObjects += objectsSortingStopwatch.elapsed();

    const Stopwatch polygonizeCoastlinesStopwatch(metric != nullptr);

    // Polygonize coastlines
    QList< std::shared_ptr<const MapObject> > polygonizedCoastlineObjects;
    const auto basemapCoastlinesPresent = !basemapCoastlineObjects.isEmpty();
    const auto detailedmapCoastlinesPresent = !detailedmapCoastlineObjects.isEmpty();
    const auto detailedLandDataPresent = detailedBinaryMapObjectsPresent && !hasContourLinesObjectOnly;
    auto fillEntireArea = true;
    bool detailCoastlineBroken = false;
    
    if (detailedmapCoastlinesPresent && zoom >= MapPrimitiviser::DetailedLandDataMinZoom)
    {
        const bool coastlinesWereAdded = polygonizeCoastlines(
            getWidenArea(area31, zoom),
            zoom,
            detailedmapCoastlineObjects,
            polygonizedCoastlineObjects);
        fillEntireArea = !coastlinesWereAdded;
        
        if (!coastlinesWereAdded)
        {
            //detect if broken coastline inside area31 (tile bbox)
            for (auto obj : detailedmapCoastlineObjects)
            {
                for (auto point : obj->points31)
                {
                    if (area31.contains(point))
                    {
                        detailCoastlineBroken = true;
                        break;
                    }
                }
                if (detailCoastlineBroken)
                    break;
            }
        }
    }
    
    
    bool hasExtraCoastlines = !extraCoastlineObjects.isEmpty();
    bool shouldAddBasemapCoastlines = !detailedmapCoastlinesPresent
                                 && !hasExtraCoastlines
                                 //&& !detailedLandDataPresent
                                 && basemapCoastlinesPresent;
    
    if (detailCoastlineBroken && basemapCoastlinesPresent)
    {
        shouldAddBasemapCoastlines = true;
        polygonizedCoastlineObjects.clear();
    }
    
    shouldAddBasemapCoastlines = shouldAddBasemapCoastlines || zoom < MapPrimitiviser::DetailedLandDataMinZoom;
    
    if (shouldAddBasemapCoastlines)
    {
        const bool coastlinesWereAdded = polygonizeCoastlines(
            area31,
            zoom,
            basemapCoastlineObjects,
            polygonizedCoastlineObjects);
        fillEntireArea = !coastlinesWereAdded && fillEntireArea;
    }

    // In case zoom is higher than ObfMapSectionLevel::MaxBasemapZoomLevel and coastlines were not used
    // due to none of them intersect current zoom tile edge, look for the nearest coastline segment
    // to determine use FullLand or FullWater as surface type
    auto surfaceType = surfaceType_;
    if (zoom > ObfMapSectionLevel::MaxBasemapZoomLevel && fillEntireArea)
    {
        const auto center = area31.center();
        assert(area31.contains(center));
        bool isDeterminedSurfaceType = false;
        
        if (hasExtraCoastlines)
        {
            AreaI bboxZoom13 = Utilities::roundBoundingBox31(area31, ZoomLevel::ZoomLevel14);
            bboxZoom13.right()++;
            bboxZoom13.bottom()++;
            bboxZoom13 = bboxZoom13.getEnlargedBy(bboxZoom13.width() / 2);
            bboxZoom13.right()--;
            bboxZoom13.bottom()--;
            MapSurfaceType surfaceTypeOverscaled = MapSurfaceType::Undefined;
            QList< std::shared_ptr<const MapObject> > polygonizedCoastlines;
            polygonizeCoastlines(
                bboxZoom13,
                ZoomLevel::ZoomLevel13,
                extraCoastlineObjects,
                polygonizedCoastlines);
            surfaceTypeOverscaled = determineSurfaceType(area31, polygonizedCoastlines);
            if (surfaceTypeOverscaled != MapSurfaceType::Undefined)
            {
                isDeterminedSurfaceType = true;
                surfaceType = surfaceTypeOverscaled;
            }
        }
        
        if (!isDeterminedSurfaceType && basemapCoastlinesPresent)
        {
            ZoomLevel basemapZoom = static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel);
            AreaI bboxBasemap = Utilities::roundBoundingBox31(area31, basemapZoom);
            QList< std::shared_ptr<const MapObject> > polygonizedCoastlines;
            MapSurfaceType surfaceTypeBasemap = MapSurfaceType::Undefined;
            polygonizeCoastlines(
                bboxBasemap,
                basemapZoom,
                basemapCoastlineObjects,
                polygonizedCoastlines);
            surfaceTypeBasemap = determineSurfaceType(area31, polygonizedCoastlines);
            if (surfaceTypeBasemap != MapSurfaceType::Undefined)
            {
                isDeterminedSurfaceType = true;
                surfaceType = surfaceTypeBasemap;
            }
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
            bgMapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalCoastlineAttributeId);
        else if (surfaceType == MapSurfaceType::FullLand || surfaceType == MapSurfaceType::Mixed)
            bgMapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalLandAttributeId);
        else // if (surfaceType == SurfaceType::Undefined)
        {
#if OSMAND_VERBOSE_MAP_PRIMITIVISER
            LogPrintf(LogSeverityLevel::Warning, "Area [%d, %d, %d, %d]@%d has undefined surface type",
                area31.top(),
                area31.left(),
                area31.bottom(),
                area31.right(),
                zoom);
#endif // OSMAND_VERBOSE_MAP_PRIMITIVISER

            bgMapObject->isArea = false;
            bgMapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalCoastlineBrokenAttributeId);
        }
        bgMapObject->additionalAttributeIds.push_back(MapObject::defaultAttributeMapping->layerLowestAttributeId);

        assert(bgMapObject->isClosedFigure());
        if (bgMapObject->isArea)
            polygonizedCoastlineObjects.push_back(qMove(bgMapObject));
    }

    // Obtain primitives
    const bool detailedDataMissing =
        (zoom > static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap)) &&
        (detailedmapMapObjects.isEmpty() || hasContourLinesObjectOnly) &&
        detailedmapCoastlineObjects.isEmpty();

    // Check if there is no data to primitivise. Report, clean-up and exit
    const auto mapObjectsCount =
        detailedmapMapObjects.size() +
        ((zoom <= static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap) || detailedDataMissing)
            ? basemapMapObjects.size()
            : 0) +
        polygonizedCoastlineObjects.size();
    if (mapObjectsCount == 0)
    {
        // Empty area
        assert(primitivisedObjects->isEmpty());
        return primitivisedObjects;
    }

    // Obtain primitives:
    MapStyleEvaluationResult evaluationResult(context.env->mapStyle->getValueDefinitionsCount());

    const Stopwatch obtainPrimitivesFromDetailedmapStopwatch(metric != nullptr);
    obtainPrimitives(
        context,
        primitivisedObjects,
        detailedmapMapObjects,
        evaluationResult,
        cache,
        queryController,
        metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesFromDetailedmap += obtainPrimitivesFromDetailedmapStopwatch.elapsed();

    if ((zoom <= static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap)) || detailedDataMissing)
    {
        const Stopwatch obtainPrimitivesFromBasemapStopwatch(metric != nullptr);
        obtainPrimitives(
            context,
            primitivisedObjects,
            basemapMapObjects,
            evaluationResult,
            cache,
            queryController,
            metric);
        if (metric)
            metric->elapsedTimeForObtainingPrimitivesFromBasemap += obtainPrimitivesFromBasemapStopwatch.elapsed();
    }

#if OSMAND_VERBOSE_MAP_PRIMITIVISER
    debugCoastline(area31, polygonizedCoastlineObjects);
#endif
    
    const Stopwatch obtainPrimitivesFromCoastlinesStopwatch(metric != nullptr);
    obtainPrimitives(
        context,
        primitivisedObjects,
        polygonizedCoastlineObjects,
        evaluationResult,
        cache,
        queryController,
        metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesFromCoastlines += obtainPrimitivesFromCoastlinesStopwatch.elapsed();

    if (queryController && queryController->isAborted())
        return nullptr;

    // Sort and filter primitives
    const Stopwatch sortAndFilterPrimitivesStopwatch(metric != nullptr);
    sortAndFilterPrimitives(context, primitivisedObjects, metric);
    if (metric)
        metric->elapsedTimeForSortingAndFilteringPrimitives += sortAndFilterPrimitivesStopwatch.elapsed();

    // Obtain symbols from primitives
    const Stopwatch obtainPrimitivesSymbolsStopwatch(metric != nullptr);
    obtainPrimitivesSymbols(context, primitivisedObjects, tileId, evaluationResult, cache, queryController, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesSymbols += obtainPrimitivesSymbolsStopwatch.elapsed();

    // Cleanup if aborted
    if (queryController && queryController->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    return primitivisedObjects;
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseWithoutSurface(
    const PointD scaleDivisor31ToPixel,
    const ZoomLevel zoom,
    const TileId tileId,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
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
        if (queryController && queryController->isAborted())
            break;

        // Check if this map object is from basemap
        auto isBasemapObject = false;
        if (const auto possiblyBasemapObject = std::dynamic_pointer_cast<const BinaryMapObject>(mapObject))
        {
            isBasemapObject = possiblyBasemapObject->section->isBasemap;
            if (zoom < static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap) && !isBasemapObject)
                continue;
        }

        if (!mapObject->containsAttribute(mapObject->attributeMapping->naturalCoastlineAttributeId))
        {
            if (isBasemapObject)
                basemapMapObjects.push_back(mapObject);
            else
                detailedmapMapObjects.push_back(mapObject);
        }
    }
    if (queryController && queryController->isAborted())
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
    MapStyleEvaluationResult evaluationResult(context.env->mapStyle->getValueDefinitionsCount());

    const Stopwatch obtainPrimitivesFromDetailedmapStopwatch(metric != nullptr);
    obtainPrimitives(context, primitivisedObjects, detailedmapMapObjects, evaluationResult, cache, queryController, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesFromDetailedmap += obtainPrimitivesFromDetailedmapStopwatch.elapsed();

    if ((zoom <= static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap)) || detailedDataMissing)
    {
        const Stopwatch obtainPrimitivesFromBasemapStopwatch(metric != nullptr);
        obtainPrimitives(context, primitivisedObjects, basemapMapObjects, evaluationResult, cache, queryController, metric);
        if (metric)
            metric->elapsedTimeForObtainingPrimitivesFromBasemap += obtainPrimitivesFromBasemapStopwatch.elapsed();
    }

    if (queryController && queryController->isAborted())
        return nullptr;

    // Sort and filter primitives
    const Stopwatch sortAndFilterPrimitivesStopwatch(metric != nullptr);
    sortAndFilterPrimitives(context, primitivisedObjects, metric);
    if (metric)
        metric->elapsedTimeForSortingAndFilteringPrimitives += sortAndFilterPrimitivesStopwatch.elapsed();

    // Obtain symbols from primitives
    const Stopwatch obtainPrimitivesSymbolsStopwatch(metric != nullptr);
    obtainPrimitivesSymbols(context, primitivisedObjects, tileId, evaluationResult, cache, queryController, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesSymbols += obtainPrimitivesSymbolsStopwatch.elapsed();

    // Cleanup if aborted
    if (queryController && queryController->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    return primitivisedObjects;
}

void OsmAnd::MapPrimitiviser_P::obtainPrimitives(
    const Context& context,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
    const QList< std::shared_ptr<const OsmAnd::MapObject> >& source,
    MapStyleEvaluationResult& evaluationResult,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& env = context.env;
    const auto zoom = primitivisedObjects->zoom;

    const Stopwatch obtainPrimitivesStopwatch(metric != nullptr);

    // Initialize shared settings for order evaluation
    MapStyleEvaluator orderEvaluator(env->mapStyle, env->displayDensityFactor * env->mapScaleFactor);
    env->applyTo(orderEvaluator);
    orderEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    orderEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    // Initialize shared settings for polygon evaluation
    MapStyleEvaluator polygonEvaluator(env->mapStyle, env->displayDensityFactor * env->mapScaleFactor);
    env->applyTo(polygonEvaluator);
    polygonEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    polygonEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    // Initialize shared settings for polyline evaluation
    MapStyleEvaluator polylineEvaluator(env->mapStyle, env->displayDensityFactor * env->mapScaleFactor);
    env->applyTo(polylineEvaluator);
    polylineEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    polylineEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    // Initialize shared settings for point evaluation
    MapStyleEvaluator pointEvaluator(env->mapStyle, env->displayDensityFactor * env->mapScaleFactor);
    env->applyTo(pointEvaluator);
    pointEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    pointEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    const auto pSharedPrimitivesGroups = cache ? cache->getPrimitivesGroupsPtr(zoom) : nullptr;
    QList< proper::shared_future< std::shared_ptr<const PrimitivesGroup> > > futureSharedPrimitivesGroups;
    for (const auto& mapObject : constOf(source))
    {
        //////////////////////////////////////////////////////////////////////////
        //if (mapObject->toString().contains("1333827773"))
        //{
        //    const auto t = mapObject->toString();
        //    int i = 5;
        //}
        //////////////////////////////////////////////////////////////////////////

        if (queryController && queryController->isAborted())
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
            evaluationResult,
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

    int failCounter = 0;
    // Wait for future primitives groups
    Stopwatch futureSharedPrimitivesGroupsStopwatch(metric != nullptr);
    for (auto& futureSharedGroup : futureSharedPrimitivesGroups)
    {
        auto timeout = proper::chrono::system_clock::now() + proper::chrono::seconds(3);
        if (proper::future_status::ready == futureSharedGroup.wait_until(timeout))
        {
            auto group = futureSharedGroup.get();

            // Add polygons, polylines and points from group to current context
            primitivisedObjects->polygons.append(group->polygons);
            primitivisedObjects->polylines.append(group->polylines);
            primitivisedObjects->points.append(group->points);

            // Add shared group to current context
            primitivisedObjects->primitivesGroups.push_back(qMove(group));
        }
        else
        {
            failCounter++;
            LogPrintf(LogSeverityLevel::Error, "Get futureSharedGroup timeout exceed = %d", failCounter);
        }
        if (failCounter >= 3)
        {
            LogPrintf(LogSeverityLevel::Error, "Get futureSharedGroup timeout exceed");
            break;
        }
    }
    if (metric)
        metric->elapsedTimeForFutureSharedPrimitivesGroups += futureSharedPrimitivesGroupsStopwatch.elapsed();

    if (metric)
        metric->elapsedTimeForPrimitives += obtainPrimitivesStopwatch.elapsed();
}

std::shared_ptr<const OsmAnd::MapPrimitiviser_P::PrimitivesGroup> OsmAnd::MapPrimitiviser_P::obtainPrimitivesGroup(
    const Context& context,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
    const std::shared_ptr<const MapObject>& mapObject,
    MapStyleEvaluationResult& evaluationResult,
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
    //if (mapObject->toString().contains("47894962"))
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    bool skipUnclosedPolygon = !mapObject->containsTag("boundary");
    double doubledPolygonArea31 = -1.0;
    Nullable<bool> rejectByArea;

    // Setup mapObject-specific input data
    const auto layerType = mapObject->getLayerType();
    orderEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_LAYER, static_cast<int>(layerType));
    orderEvaluator.setBooleanValue(env->styleBuiltinValueDefs->id_INPUT_AREA, mapObject->isArea);
    orderEvaluator.setBooleanValue(env->styleBuiltinValueDefs->id_INPUT_POINT, mapObject->points31.size() == 1);
    orderEvaluator.setBooleanValue(env->styleBuiltinValueDefs->id_INPUT_CYCLE, mapObject->isClosedFigure());
    polylineEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_LAYER, static_cast<int>(layerType));

    const auto& decRules = mapObject->attributeMapping->decodeMap;
    auto pAttributeId = mapObject->attributeIds.constData();
    const auto attributeIdsCount = mapObject->attributeIds.size();
    for (auto attributeIdIndex = 0; attributeIdIndex < attributeIdsCount; attributeIdIndex++, pAttributeId++)
    {
        const auto& decodedAttribute = decRules[*pAttributeId];

        //////////////////////////////////////////////////////////////////////////
        //if (mapObject->toString().contains("49048972"))
        //{
        //    const auto t = mapObject->toString();
        //    int i = 5;
        //}
        //////////////////////////////////////////////////////////////////////////

        const Stopwatch orderEvaluationStopwatch(metric != nullptr);

        // Setup tag+value-specific input data
        orderEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedAttribute.tag);
        orderEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedAttribute.value);

        evaluationResult.clear();
        ok = orderEvaluator.evaluate(mapObject, MapStyleRulesetType::Order, &evaluationResult);

        if (metric)
        {
            metric->elapsedTimeForOrderEvaluation += orderEvaluationStopwatch.elapsed();
            metric->orderEvaluations++;
        }

        const Stopwatch orderProcessingStopwatch(metric != nullptr);

        // If evaluation failed, skip
        if (!ok)
        {
            if (metric)
            {
                metric->elapsedTimeForOrderProcessing += orderProcessingStopwatch.elapsed();
                metric->orderRejects++;
            }

            continue;
        }

        int objectType_;
        if (!evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_OBJECT_TYPE, objectType_))
        {
            if (metric)
            {
                metric->elapsedTimeForOrderProcessing += orderProcessingStopwatch.elapsed();
                metric->orderRejects++;
            }

            continue;
        }
        const auto objectType = static_cast<PrimitiveType>(objectType_);

        int zOrder = -1;
        if (!evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_ORDER, zOrder) || zOrder < 0)
        {
            if (metric)
            {
                metric->elapsedTimeForOrderProcessing += orderProcessingStopwatch.elapsed();
                metric->orderRejects++;
            }

            continue;
        }

        int shadowLevel = 0;
        evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_LEVEL, shadowLevel);

        if (objectType == PrimitiveType::Polygon)
        {
            const Stopwatch polygonProcessingStopwatch(metric != nullptr);

            // Perform checks on data
            if (mapObject->points31.size() <= 2)
            {
                if (metric)
                    metric->elapsedTimeForPolygonProcessing += polygonProcessingStopwatch.elapsed();

#if OSMAND_VERBOSE_MAP_PRIMITIVISER
                LogPrintf(LogSeverityLevel::Warning,
                    "MapObject %s primitive is processed as polygon, but has only %d point(s)",
                    qPrintable(mapObject->toString()),
                    mapObject->points31.size());
#endif // OSMAND_VERBOSE_MAP_PRIMITIVISER
                continue;
            }
            if (skipUnclosedPolygon && !mapObject->isClosedFigure())
            {
                if (metric)
                    metric->elapsedTimeForPolygonProcessing += polygonProcessingStopwatch.elapsed();

#if OSMAND_VERBOSE_MAP_PRIMITIVISER
                LogPrintf(LogSeverityLevel::Warning,
                    "MapObject %s primitive is processed as polygon, but isn't closed",
                    qPrintable(mapObject->toString()));
#endif // OSMAND_VERBOSE_MAP_PRIMITIVISER
                continue;
            }
            if (skipUnclosedPolygon && !mapObject->isClosedFigure(true))
            {
                if (metric)
                    metric->elapsedTimeForPolygonProcessing += polygonProcessingStopwatch.elapsed();

#if OSMAND_VERBOSE_MAP_PRIMITIVISER
                LogPrintf(LogSeverityLevel::Warning,
                    "MapObject %s primitive is processed as polygon, but isn't closed (inner)",
                    qPrintable(mapObject->toString()));
#endif // OSMAND_VERBOSE_MAP_PRIMITIVISER
                continue;
            }

            //////////////////////////////////////////////////////////////////////////
            //if ((mapObject->id >> 1) == 266877135u)
            //{
            //    int i = 5;
            //}
            //////////////////////////////////////////////////////////////////////////

            // Check size of polygon
            auto ignorePolygonArea = false;
            evaluationResult.getBooleanValue(
                env->styleBuiltinValueDefs->id_OUTPUT_IGNORE_POLYGON_AREA,
                ignorePolygonArea);

            auto ignorePolygonAsPointArea = false;
            evaluationResult.getBooleanValue(
                env->styleBuiltinValueDefs->id_OUTPUT_IGNORE_POLYGON_AS_POINT_AREA,
                ignorePolygonAsPointArea);

            if (doubledPolygonArea31 < 0.0)
                doubledPolygonArea31 = Utilities::doubledPolygonArea(mapObject->points31);

            if ((!ignorePolygonArea || !ignorePolygonAsPointArea) && !rejectByArea.isSet())
            {
                const auto polygonArea31 = static_cast<double>(doubledPolygonArea31)* 0.5;
                const auto areaScaleDivisor31ToPixel =
                    primitivisedObjects->scaleDivisor31ToPixel.x * primitivisedObjects->scaleDivisor31ToPixel.y;
                const auto polygonAreaInPixels = polygonArea31 / areaScaleDivisor31ToPixel;
                const auto polygonAreaInAbstractPixels =
                    polygonAreaInPixels / (env->displayDensityFactor * env->displayDensityFactor);
                rejectByArea =
                    primitivisedObjects->scaleDivisor31ToPixel.x >= 0.0 &&
                    primitivisedObjects->scaleDivisor31ToPixel.y >= 0.0 &&
                    polygonAreaInAbstractPixels <= context.polygonAreaMinimalThreshold;
            }

            if (!rejectByArea.isSet() || *rejectByArea.getValuePtrOrNullptr() == false || ignorePolygonArea)
            {
                const Stopwatch polygonEvaluationStopwatch(metric != nullptr);

                // Setup tag+value-specific input data (for Polygon)
                polygonEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedAttribute.tag);
                polygonEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedAttribute.value);

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
                        attributeIdIndex,
                        evaluationResult));
                    primitive->zOrder = (std::dynamic_pointer_cast<const SurfaceMapObject>(mapObject) || std::dynamic_pointer_cast<const CoastlineMapObject>(mapObject))
                        ? std::numeric_limits<int>::min()
                        : zOrder;
                    primitive->doubledArea = doubledPolygonArea31;

                    // Accept this primitive
                    if (zOrder <= MAX_POLYGON_ORDER_IN_POLYGONS_LIST)
                        constructedGroup->polygons.push_back(qMove(primitive));
                    else
                        // Add to polylines to respect order while rastering                
                        constructedGroup->polylines.push_back(qMove(primitive));

                    // Update metric
                    if (metric)
                        metric->polygonPrimitives++;
                }
                else
                {
                    if (metric)
                        metric->polygonRejects++;
                }
            }
            else
            {
                if (metric)
                    metric->polygonsRejectedByArea++;
            }

            if (metric)
                metric->elapsedTimeForPolygonProcessing += polygonProcessingStopwatch.elapsed();

            if (!rejectByArea.isSet() || *rejectByArea.getValuePtrOrNullptr() == false || ignorePolygonAsPointArea)
            {
                const Stopwatch pointEvaluationStopwatch(metric != nullptr);

                // Setup tag+value-specific input data (for Point)
                pointEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedAttribute.tag);
                pointEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedAttribute.value);

                // Icon also depends on native text length
                const auto& nativeCaption = mapObject->getCaptionInNativeLanguage();
                int nativeCaptionLength = nativeCaption.isNull() ? 0 : nativeCaption.length();
                pointEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_TEXT_LENGTH, nativeCaptionLength);

                // Evaluate Point rules
                evaluationResult.clear();
                const auto hasIcon = pointEvaluator.evaluate(mapObject, MapStyleRulesetType::Point, &evaluationResult);

                // Update metric
                if (metric)
                {
                    metric->elapsedTimeForPointEvaluation += pointEvaluationStopwatch.elapsed();
                    metric->pointEvaluations++;
                }

                const Stopwatch pointProcessingStopwatch(metric != nullptr);

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
                            attributeIdIndex,
                            evaluationResult));
                    }
                    else
                    {
                        pointPrimitive.reset(new Primitive(
                            group,
                            PrimitiveType::Point,
                            attributeIdIndex));
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

                if (metric)
                    metric->elapsedTimeForPointProcessing += pointProcessingStopwatch.elapsed();
            }
        }
        else if (objectType == PrimitiveType::Polyline)
        {
            // Perform checks on data
            if (mapObject->points31.size() < 2)
            {
#if OSMAND_VERBOSE_MAP_PRIMITIVISER
                LogPrintf(LogSeverityLevel::Warning,
                    "MapObject %s is processed as polyline, but has %d point(s)",
                    qPrintable(mapObject->toString()),
                    mapObject->points31.size());
#endif // OSMAND_VERBOSE_MAP_PRIMITIVISER
                continue;
            }

            const Stopwatch polylineEvaluationStopwatch(metric != nullptr);

            // Setup tag+value-specific input data
            polylineEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedAttribute.tag);
            polylineEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedAttribute.value);

            // Evaluate style for this primitive to check if it passes
            evaluationResult.clear();
            ok = polylineEvaluator.evaluate(mapObject, MapStyleRulesetType::Polyline, &evaluationResult);

            if (metric)
            {
                metric->elapsedTimeForPolylineEvaluation += polylineEvaluationStopwatch.elapsed();
                metric->polylineEvaluations++;
            }

            const Stopwatch polylineProcessingStopwatch(metric != nullptr);

            // If evaluation failed, skip
            if (!ok)
            {
                if (metric)
                {
                    metric->elapsedTimeForPolylineProcessing += polylineProcessingStopwatch.elapsed();
                    metric->polylineRejects++;
                }

                continue;
            }

            // Create new primitive
            const std::shared_ptr<Primitive> primitive(new Primitive(
                group,
                objectType,
                attributeIdIndex,
                evaluationResult));
            primitive->zOrder = zOrder;

            // Accept this primitive
            if (zOrder >= MIN_POLYLINE_ORDER_IN_POLYLINES_LIST)
                constructedGroup->polylines.push_back(qMove(primitive));
            else
                // Add to polygons to respect order while rasterizing
                constructedGroup->polygons.push_back(qMove(primitive));

            // Update metric
            if (metric)
            {
                metric->elapsedTimeForPolylineProcessing += polylineProcessingStopwatch.elapsed();
                metric->polylinePrimitives++;
            }
        }
        else if (objectType == PrimitiveType::Point)
        {
            // Perform checks on data
            if (mapObject->points31.size() < 1)
            {
#if OSMAND_VERBOSE_MAP_PRIMITIVISER
                LogPrintf(LogSeverityLevel::Warning,
                    "MapObject %s is processed as point, but has no point",
                    qPrintable(mapObject->toString()));
#endif // OSMAND_VERBOSE_MAP_PRIMITIVISER
                continue;
            }

            const Stopwatch pointEvaluationStopwatch(metric != nullptr);

            // Setup tag+value-specific input data
            pointEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedAttribute.tag);
            pointEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedAttribute.value);

            // Icon also depends on native text length
            const auto& nativeCaption = mapObject->getCaptionInNativeLanguage();
            int nativeCaptionLength = nativeCaption.isNull() ? 0 : nativeCaption.length();
            pointEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_TEXT_LENGTH, nativeCaptionLength);

            // Evaluate Point rules
            evaluationResult.clear();
            const bool hasIcon = pointEvaluator.evaluate(mapObject, MapStyleRulesetType::Point, &evaluationResult);

            // Update metric
            if (metric)
            {
                metric->elapsedTimeForPointEvaluation += pointEvaluationStopwatch.elapsed();
                metric->pointEvaluations++;
            }

            const Stopwatch pointProcessingStopwatch(metric != nullptr);

            // Create point primitive only in case polygon has any content
            if (mapObject->captions.isEmpty() && !hasIcon)
            {
                if (metric)
                {
                    metric->elapsedTimeForPointProcessing += pointProcessingStopwatch.elapsed();
                    metric->pointRejects++;
                }

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
                    attributeIdIndex,
                    evaluationResult));
            }
            else
            {
                primitive.reset(new Primitive(
                    group,
                    PrimitiveType::Point,
                    attributeIdIndex));
            }
            primitive->zOrder = zOrder;

            // Accept this primitive
            constructedGroup->points.push_back(qMove(primitive));

            // Update metric
            if (metric)
            {
                metric->elapsedTimeForPointProcessing += pointProcessingStopwatch.elapsed();
                metric->pointPrimitives++;
            }
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
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
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
            if (l->attributeIdIndex != r->attributeIdIndex && l->sourceObject == r->sourceObject)
            {
                if (l->type == PrimitiveType::Polygon)
                    return l->attributeIdIndex > r->attributeIdIndex;
                return l->attributeIdIndex < r->attributeIdIndex;
            }

            // Then sort by number of points
            const auto lPointsCount = l->sourceObject->points31.size();
            const auto rPointsCount = r->sourceObject->points31.size();
            if (lPointsCount != rPointsCount)
                return lPointsCount < rPointsCount;

            // Sort by map object sorting key, if map objects are equal
            return mapObjectsComparator(l->sourceObject, r->sourceObject);
        };

    std::sort(primitivisedObjects->polygons, privitivesSort);
    std::sort(primitivisedObjects->polylines, privitivesSort);
    filterOutHighwaysByDensity(context, primitivisedObjects, metric);
    std::sort(primitivisedObjects->points, privitivesSort);
}

void OsmAnd::MapPrimitiviser_P::filterOutHighwaysByDensity(
    const Context& context,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    // Check if any filtering needed
    if (context.roadDensityZoomTile == 0 || context.roadsDensityLimitPerTile == 0)
        return;

    const auto dZ = primitivisedObjects->zoom + context.roadDensityZoomTile;
    QHash< uint64_t, std::pair<uint32_t, double> > densityMap;

    auto itPolyline = mutableIteratorOf(primitivisedObjects->polylines);
    itPolyline.toBack();
    while (itPolyline.hasPrevious())
    {
        const auto& polyline = itPolyline.previous();
        const auto& sourceObject = polyline->sourceObject;

        // If polyline is not road, it should be accepted
        const auto pPrimitiveAttribute = sourceObject->resolveAttributeByIndex(polyline->attributeIdIndex);
        if (!pPrimitiveAttribute)
            continue;
        if (pPrimitiveAttribute->tag != QLatin1String("highway"))
            continue;

        auto accept = false;
        uint64_t prevId = 0;
        const auto pointsCount = sourceObject->points31.size();
        auto pPoint = sourceObject->points31.constData();
        for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
        {
            auto x = pPoint->x >> (MaxZoomLevel - dZ);
            auto y = pPoint->y >> (MaxZoomLevel - dZ);
            uint64_t id = (static_cast<uint64_t>(x) << dZ) | y;
            if (prevId != id)
            {
                prevId = id;

                auto& mapEntry = densityMap[id];
                if (mapEntry.first < context.roadsDensityLimitPerTile /*&& p.second > o */)
                {
                    accept = true;
                    mapEntry.first += 1;
                    mapEntry.second = polyline->zOrder;
                }
            }
        }
        if (!accept)
        {
            if (metric)
                metric->polylineRejectedByDensity++;

            itPolyline.remove();
        }
    }
}

void OsmAnd::MapPrimitiviser_P::obtainPrimitivesSymbols(
    const Context& context,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
    const TileId tileId,
    MapStyleEvaluationResult& evaluationResult,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& env = context.env;
    const auto zoom = primitivisedObjects->zoom;

    // Initialize shared settings for text evaluation
    MapStyleEvaluator textEvaluator(env->mapStyle, env->displayDensityFactor * env->symbolsScaleFactor);
    env->applyTo(textEvaluator);
    textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    // Use coarse grid of tile symbols for preliminary overlap filtering
    QHash<int, std::shared_ptr<const GridSymbolsGroup>> tileGrid;

    //NOTE: Em, I'm not sure this is still true
    //NOTE: Since 2 tiles with same MapObject may have different set of polylines, generated from it,
    //NOTE: then set of symbols also should differ, but it won't.
    const auto pSharedSymbolGroups = cache ? cache->getSymbolsGroupsPtr(primitivisedObjects->zoom) : nullptr;
    QList< proper::shared_future< std::shared_ptr<const SymbolsGroup> > > futureSharedSymbolGroups;
    for (const auto& primitivesGroup : constOf(primitivisedObjects->primitivesGroups))
    {
        if (queryController && queryController->isAborted())
            return;

        // If using shared context is allowed, check if this group was already processed
        // (using shared cache is only allowed for non-generated MapObjects),
        // then symbols group can be shared
        MapObject::SharingKey sharingKey;
        auto canBeShared = primitivesGroup->sourceObject->obtainSharingKey(sharingKey);

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

        const Stopwatch symbolsGroupsProcessingStopwatch(metric != nullptr);

        // Create a symbols group
        const std::shared_ptr<SymbolsGroup> group(new SymbolsGroup(
            primitivesGroup->sourceObject));

        // For each primitive if primitive group, collect symbols from it
        collectSymbolsFromPrimitives(
            context,
            primitivisedObjects,
            primitivesGroup->polygons,
            evaluationResult,
            textEvaluator,
            group->symbols,
            queryController,
            metric);

        collectSymbolsFromPrimitives(
            context,
            primitivisedObjects,
            primitivesGroup->polylines,
            evaluationResult,
            textEvaluator,
            group->symbols,
            queryController,
            metric);

        const auto prevSize = group->symbols.size();

        collectSymbolsFromPrimitives(
            context,
            primitivisedObjects,
            primitivesGroup->points,
            evaluationResult,
            textEvaluator,
            group->symbols,
            queryController,
            metric);

        // Filter out overlapping labels of points by placing them on the coarse grid
        if (tileId.x >= 0 && tileId.y >= 0 && group->symbols.size() == 1 && prevSize == 0
            && primitivisedObjects->zoom >= MinZoomLevel && primitivisedObjects->zoom <= MaxZoomLevel)
        {
            if (const auto& textSymbol = std::dynamic_pointer_cast<const TextSymbol>(group->symbols.back()))
            {
                const auto minCell = -GRID_CELLS_PER_TILESIDE;
                const auto maxCell = GRID_CELLS_PER_TILESIDE * 2.0;
                const auto zoomShift = MaxZoomLevel - primitivisedObjects->zoom;
                PointD filterCoords(textSymbol->location31);
                filterCoords -= PointD(
                    static_cast<double>(tileId.x << zoomShift), static_cast<double>(tileId.y << zoomShift));
                filterCoords *= GRID_CELLS_PER_TILESIDE / static_cast<double>(1u << zoomShift);
                filterCoords.x = filterCoords.x > minCell && filterCoords.x < maxCell ? filterCoords.x : minCell;
                filterCoords.y = filterCoords.y > minCell && filterCoords.y < maxCell ? filterCoords.y : minCell;
                const auto gridCode = static_cast<int>(std::floor(filterCoords.y)) * 10000
                    + static_cast<int>(std::floor(filterCoords.x));
                bool shouldInsert = true;
                const auto& citGroup = tileGrid.constFind(gridCode);
                if (citGroup != tileGrid.cend())
                {
                    const auto& groupOnGrid = *citGroup;
                    shouldInsert = textSymbol->order < groupOnGrid->symbolsGroup->symbols.back()->order;
                    if (pSharedSymbolGroups)
                    {
                        group->symbols.clear();
                        if (shouldInsert && groupOnGrid->canBeShared)
                            pSharedSymbolGroups->fulfilPromiseAndReference(groupOnGrid->sharingKey, group);
                        if (!shouldInsert && canBeShared)
                            pSharedSymbolGroups->fulfilPromiseAndReference(sharingKey, group);
                    }
                }
                if (shouldInsert)
                {
                    const std::shared_ptr<SymbolsGroup> preGroup(new SymbolsGroup(primitivesGroup->sourceObject));
                    preGroup->symbols.push_back(qMove(textSymbol));
                    const std::shared_ptr<GridSymbolsGroup> groupOnGrid(
                        new GridSymbolsGroup(preGroup, canBeShared, sharingKey));
                    tileGrid.insert(gridCode, qMove(groupOnGrid));
                }
                group->symbols.clear();
                canBeShared = false;
            }
        }

        // Update metric
        if (metric)
        {
            metric->elapsedTimeForSymbolsGroupsProcessing += symbolsGroupsProcessingStopwatch.elapsed();
            metric->symbolsGroupsProcessed++;
        }

        // Add this group to shared cache
        if (pSharedSymbolGroups && canBeShared)
            pSharedSymbolGroups->fulfilPromiseAndReference(sharingKey, group);

        // Empty groups are also inserted, to indicate that they are empty
        assert(!primitivisedObjects->symbolsGroups.contains(group->sourceObject));
        primitivisedObjects->symbolsGroups.insert(group->sourceObject, group);
    }

    // Keep only those symbols, that left on the filter grid
    for (const auto& groupOnGrid : constOf(tileGrid))
    {
        const auto& lastGroup = qMove(groupOnGrid->symbolsGroup);
        if (pSharedSymbolGroups && groupOnGrid->canBeShared)
            pSharedSymbolGroups->fulfilPromiseAndReference(groupOnGrid->sharingKey, lastGroup);
        primitivisedObjects->symbolsGroups.insert(lastGroup->sourceObject, lastGroup);
    }

    int failCounter = 0;
    for (auto& futureGroup : futureSharedSymbolGroups)
    {
        auto timeout = proper::chrono::system_clock::now() + proper::chrono::seconds(3);
        if (proper::future_status::ready == futureGroup.wait_until(timeout))
        {
            const auto group = futureGroup.get();

            // Add shared group to current context
            assert(!primitivisedObjects->symbolsGroups.contains(group->sourceObject));
            primitivisedObjects->symbolsGroups.insert(group->sourceObject, group);
        }
        else
        {
            failCounter++;
            LogPrintf(LogSeverityLevel::Error, "Get futureGroup timeout exceed = %d", failCounter);
        }
        if (failCounter >= 3)
        {
            LogPrintf(LogSeverityLevel::Error, "Get futureGroup timeout exceed");
            break;
        }
    }
}

void OsmAnd::MapPrimitiviser_P::collectSymbolsFromPrimitives(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
    const PrimitivesCollection& primitives,
    MapStyleEvaluationResult& evaluationResult,
    MapStyleEvaluator& textEvaluator,
    SymbolsCollection& outSymbols,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    for (const auto& primitive : constOf(primitives))
    {
        //////////////////////////////////////////////////////////////////////////
        //if (primitive->sourceObject->toString().contains("49048972"))
        //{
        //    const auto t = primitive->sourceObject->toString();
        //    int i = 5;
        //}
        //else
        //    return;
        //////////////////////////////////////////////////////////////////////////

        if (queryController && queryController->isAborted())
            return;

        //NOTE: Each polygon that has icon or text is also added as point. So there's no need to process polygons
        if (primitive->type == PrimitiveType::Polyline)
        {
            obtainSymbolsFromPolyline(
                context,
                primitivisedObjects,
                primitive,
                evaluationResult,
                textEvaluator,
                outSymbols,
                metric);
        }
        else if (primitive->type == PrimitiveType::Point)
        {
            const auto& attributeMapping = primitive->sourceObject->attributeMapping;
            const auto attributeId = primitive->sourceObject->attributeIds[primitive->attributeIdIndex];
            const auto& decodedAttribute = attributeMapping->decodeMap[attributeId];
            if (!context.env->disabledAttributes.contains(decodedAttribute.value))
            {
                obtainSymbolsFromPoint(
                    context,
                    primitivisedObjects,
                    primitive,
                    evaluationResult,
                    textEvaluator,
                    outSymbols,
                    metric);
            }
        }
    }
}

void OsmAnd::MapPrimitiviser_P::obtainSymbolsFromPolygon(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
    const std::shared_ptr<const Primitive>& primitive,
    MapStyleEvaluationResult& evaluationResult,
    MapStyleEvaluator& textEvaluator,
    SymbolsCollection& outSymbols,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& points31 = primitive->sourceObject->points31;

    assert(points31.size() > 2);
    // assert(primitive->sourceObject->isClosedFigure());
    // assert(primitive->sourceObject->isClosedFigure(true));

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
        evaluationResult,
        textEvaluator,
        outSymbols,
        metric);
}

void OsmAnd::MapPrimitiviser_P::obtainSymbolsFromPolyline(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
    const std::shared_ptr<const Primitive>& primitive,
    MapStyleEvaluationResult& evaluationResult,
    MapStyleEvaluator& textEvaluator,
    SymbolsCollection& outSymbols,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
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
        evaluationResult,
        textEvaluator,
        outSymbols,
        metric);
}

void OsmAnd::MapPrimitiviser_P::obtainSymbolsFromPoint(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
    const std::shared_ptr<const Primitive>& primitive,
    MapStyleEvaluationResult& evaluationResult,
    MapStyleEvaluator& textEvaluator,
    SymbolsCollection& outSymbols,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    //////////////////////////////////////////////////////////////////////////
    //if (primitive->sourceObject->toString().contains("49048972"))
    //{
    //    const auto t = primitive->sourceObject->toString();
    //    int i = 5;
    //}
    //else
    //    return;
    //////////////////////////////////////////////////////////////////////////

    const auto& points31 = primitive->sourceObject->points31;

    assert(points31.size() > 0);

    // Depending on type of point, center is determined differently
    PointI center;
    if (primitive->sourceObject->isLabelCoordinatesSpecified())
    {
        center.x = primitive->sourceObject->getLabelCoordinateX();
        center.y = primitive->sourceObject->getLabelCoordinateY();
    }
    else if (points31.size() == 1)
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

    obtainPrimitiveIcon(
        context,
        primitive,
        center,
        evaluationResult,
        outSymbols,
        metric);

    obtainPrimitiveTexts(
        context,
        primitivisedObjects,
        primitive,
        center,
        evaluationResult,
        textEvaluator,
        outSymbols,
        metric);
}

void OsmAnd::MapPrimitiviser_P::obtainPrimitiveTexts(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
    const std::shared_ptr<const Primitive>& primitive,
    const PointI& location,
    MapStyleEvaluationResult& evaluationResult,
    MapStyleEvaluator& textEvaluator,
    SymbolsCollection& outSymbols,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& mapObject = primitive->sourceObject;
    const auto& env = context.env;

    //////////////////////////////////////////////////////////////////////////
    //if (primitive->sourceObject->toString().contains("47894962"))
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    // Text symbols can only be obtained from captions
    if (mapObject->captions.isEmpty())
        return;

    const auto& attributeMapping = mapObject->attributeMapping;
    const auto attributeId = mapObject->attributeIds[primitive->attributeIdIndex];
    const auto& decodedAttribute = attributeMapping->decodeMap[attributeId];

    // Set common evaluator settings
    textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedAttribute.tag);
    textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedAttribute.value);

    // Get captions and their order
    auto captions = mapObject->captions;
    auto captionsOrder = mapObject->captionsOrder;

    // Process captions to find out what names are present and modify that if needed
    bool hasNativeName = false;
    bool hasLocalizedName = false;
    bool hasEnglishName = false;
    uint32_t localizedNameRuleId = std::numeric_limits<uint32_t>::max();
    {
        const auto citCaptionsEnd = captions.cend();

        // Look for localized name
        const auto& localeLanguageId = env->getLocaleLanguageId().toLower();
        const auto citLocalizedNameRuleId = attributeMapping->localizedNameAttributes.constFind(&localeLanguageId);
        if (citLocalizedNameRuleId != attributeMapping->localizedNameAttributes.cend())
            localizedNameRuleId = *citLocalizedNameRuleId;
        
        // sort captionsOrder by textOrder property
        QHash<uint32_t, int> textOrderMap;
        for (const auto& captionAttributeId : constOf(captionsOrder))
        {
            const auto& caption = constOf(captions)[captionAttributeId];
            textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_TEXT_LENGTH, caption.length());
            QString captionAttributeTag;
            if (captionAttributeId != attributeMapping->nativeNameAttributeId && captionAttributeId != localizedNameRuleId)
                captionAttributeTag = attributeMapping->decodeMap[captionAttributeId].tag;
            textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_NAME_TAG, captionAttributeTag);
            evaluationResult.clear();
            textEvaluator.evaluate(mapObject, MapStyleRulesetType::Text, &evaluationResult);
            int textOrder = 100;
            evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_ORDER, textOrder);
            textOrderMap.insert(captionAttributeId, textOrder);
        }
        std::sort(captionsOrder.begin(), captionsOrder.end(), [&textOrderMap] (uint32_t c1, uint32_t c2) {
            return textOrderMap.value(c1) < textOrderMap.value(c2);
        });

        // Look for native name
        auto citNativeName =
            (attributeMapping->nativeNameAttributeId == std::numeric_limits<uint32_t>::max())
            ? citCaptionsEnd
            : captions.constFind(attributeMapping->nativeNameAttributeId);
        hasNativeName = (citNativeName != citCaptionsEnd);
        auto nativeNameOrder = hasNativeName
            ? captionsOrder.indexOf(citNativeName.key())
            : -1;
        
        auto citLocalizedName =
            (localizedNameRuleId == std::numeric_limits<uint32_t>::max())
            ? citCaptionsEnd
            : captions.constFind(localizedNameRuleId);
        hasLocalizedName = (citLocalizedName != citCaptionsEnd);
        auto localizedNameOrder = hasLocalizedName
            ? captionsOrder.indexOf(citLocalizedName.key())
            : -1;
        
        auto citEnglishName =
            (attributeMapping->enNameAttributeId == std::numeric_limits<uint32_t>::max())
            ? citCaptionsEnd
            : captions.constFind(attributeMapping->enNameAttributeId);
        hasEnglishName = (citEnglishName != citCaptionsEnd);
        

        // According to presentation settings, adjust set of captions
        switch (env->getLanguagePreference())
        {
            case MapPresentationEnvironment::LanguagePreference::NativeOnly:
            {
                // Remove localized name if such exists
                if (hasLocalizedName)
                {
                    captions.remove(localizedNameRuleId);
                    captionsOrder.removeOne(localizedNameRuleId);
                    hasLocalizedName = false;
                    citLocalizedName = citCaptionsEnd;
                    localizedNameOrder = -1;
                }
                break;
            }

            case MapPresentationEnvironment::LanguagePreference::LocalizedOrNative:
            {
                // Only one should be shown, thus remove native if localized exist
                if (hasLocalizedName && hasNativeName)
                {
                    captions.remove(attributeMapping->nativeNameAttributeId);
                    captionsOrder.removeOne(attributeMapping->nativeNameAttributeId);
                    hasNativeName = false;
                    citNativeName = citCaptionsEnd;
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
                    const auto latinNameValue = (hasEnglishName) ? citEnglishName.value() : ICU::transliterateToLatin(citNativeName.value());

                    citLocalizedName = captions.insert(localizedNameRuleId, latinNameValue);
                    localizedNameOrder = captionsOrder.indexOf(attributeMapping->nativeNameAttributeId) + 1;
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
                    const auto latinNameValue = (hasEnglishName) ? citEnglishName.value() : ICU::transliterateToLatin(citNativeName.value());

                    citLocalizedName = captions.insert(localizedNameRuleId, latinNameValue);
                    localizedNameOrder = captionsOrder.indexOf(attributeMapping->nativeNameAttributeId);
                    captionsOrder.insert(localizedNameOrder, localizedNameRuleId);
                    hasLocalizedName = true;
                }
                break;
            }

            case MapPresentationEnvironment::LanguagePreference::LocalizedOrTransliterated:
            {
                // If there's no localized name, transliterate native name (if exists)
                if (!hasLocalizedName && hasNativeName)
                {
                    const auto latinNameValue = (hasEnglishName) ? citEnglishName.value() : ICU::transliterateToLatin(citNativeName.value());

                    citLocalizedName = captions.insert(localizedNameRuleId, latinNameValue);
                    localizedNameOrder = captionsOrder.indexOf(attributeMapping->nativeNameAttributeId);
                    captionsOrder.insert(localizedNameOrder, localizedNameRuleId);
                    hasLocalizedName = true;
                }
                
                // Only one should be shown, thus remove native if localized exist
                if (hasLocalizedName && hasNativeName)
                {
                    captions.remove(attributeMapping->nativeNameAttributeId);
                    captionsOrder.removeOne(attributeMapping->nativeNameAttributeId);
                    hasNativeName = false;
                    citNativeName = citCaptionsEnd;
                    nativeNameOrder = -1;
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
            assert(citNativeName != citCaptionsEnd);
            const auto pureNativeName = ICU::stripAccentsAndDiacritics(citNativeName.value());

            assert(citLocalizedName != citCaptionsEnd);
            const auto pureLocalizedName = ICU::stripAccentsAndDiacritics(citLocalizedName.value());

            if (pureNativeName.compare(pureLocalizedName, Qt::CaseInsensitive) == 0)
            {
                // Keep first one
                if (nativeNameOrder < localizedNameOrder)
                {
                    captions.remove(localizedNameRuleId);
                    captionsOrder.removeOne(localizedNameRuleId);
                    hasLocalizedName = false;
                    citLocalizedName = citCaptionsEnd;
                    localizedNameOrder = -1;
                }
                else // if (localizedNameOrder < nativeNameOrder)
                {
                    captions.remove(attributeMapping->nativeNameAttributeId);
                    captionsOrder.removeOne(attributeMapping->nativeNameAttributeId);
                    hasNativeName = false;
                    citNativeName = citCaptionsEnd;
                    nativeNameOrder = -1;
                }
            }
        }
    }

    // Process captions in order how captions where declared in OBF source (what is being controlled by 'rendering_types.xml')
    bool ok;
    bool extraCaptionTextAdded = false;
    uint32_t extraCaptionRuleId = std::numeric_limits<uint32_t>::max();
    for (const auto& captionAttributeId : constOf(captionsOrder))
    {
        const auto& caption = constOf(captions)[captionAttributeId];

        // If it's empty, it can not be primitivised
        if (caption.isEmpty())
            continue;

        // In case this tag was already processed as extra caption, no need to duplicate it
        if (extraCaptionTextAdded && captionAttributeId == extraCaptionRuleId)
            continue;

        // Skip captions that are from 'name[:*]'-tags but not of 'native' or 'localized' language
        if (captionAttributeId != attributeMapping->nativeNameAttributeId &&
            captionAttributeId != localizedNameRuleId &&
            attributeMapping->nameAttributeIds.contains(captionAttributeId))
        {
            continue;
        }

        const Stopwatch textEvaluationStopwatch(metric != nullptr);

        // Evaluate style to obtain text parameters
        textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_TEXT_LENGTH, caption.length());

        // Get tag of the attribute, in case caption is not from a 'name[:*]'-attribute
        QString captionAttributeTag;
        if (captionAttributeId != attributeMapping->nativeNameAttributeId && captionAttributeId != localizedNameRuleId)
            captionAttributeTag = attributeMapping->decodeMap[captionAttributeId].tag;
        textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_NAME_TAG, captionAttributeTag);

        evaluationResult.clear();
        ok = textEvaluator.evaluate(mapObject, MapStyleRulesetType::Text, &evaluationResult);

        if (metric)
        {
            metric->elapsedTimeForTextSymbolsEvaluation += textEvaluationStopwatch.elapsed();
            metric->textSymbolsEvaluations++;
        }

        if (!ok)
            continue;

        // Skip text that doesn't have valid size
        int textSize = 0;
        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_SIZE, textSize);
        if (!ok || textSize == 0)
            continue;

        const Stopwatch textProcessingStopwatch(metric != nullptr);

        // Determine language of this text
        auto languageId = LanguageId::Invariant;
        if (hasNativeName && captionAttributeId == attributeMapping->nativeNameAttributeId)
            languageId = LanguageId::Native;
        else if (hasLocalizedName && captionAttributeId == localizedNameRuleId)
            languageId = LanguageId::Localized;

        // Create primitive
        const std::shared_ptr<TextSymbol> text(new TextSymbol(primitive));
        text->location31 = location;
        text->languageId = languageId;
        text->baseValue = caption;
        text->value = caption;
        text->scaleFactor = env->symbolsScaleFactor;

        TextSymbol::Placement placement = TextSymbol::Placement::Default;
        if (primitive->type == PrimitiveType::Point || primitive->type == PrimitiveType::Polygon)
        {
            QString placementsString;
            bool ok = evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_PLACEMENT, placementsString);
            if (ok)
            {
                const auto placementsList = placementsString.split('|', Qt::SkipEmptyParts);
                for (int i = 0; i < placementsList.size(); i++)
                {
                    const auto& placementName = placementsList[i];
                    const auto parsedPlacement = TextSymbol::placementFromString(placementName);
                    
                    if (i == 0)
                        placement = parsedPlacement;
                    else if (parsedPlacement != placement)
                        text->additionalPlacements.push_back(parsedPlacement);
                }
            }
        }
        text->placement = placement;

        bool textPositioningTaken = std::any_of(outSymbols,
            [text]
            (const std::shared_ptr<const Symbol>& otherSymbol) -> bool
            {
                if (const auto otherTextSymbol = std::dynamic_pointer_cast<const TextSymbol>(otherSymbol))
                {
                    bool bothAlongPath = text->drawAlongPath && otherTextSymbol->drawAlongPath;
                    bool bothOnPath = text->drawOnPath && otherTextSymbol->drawOnPath;
                    bool bothWithoutPath = !text->drawAlongPath
                        && !text->drawOnPath 
                        && !otherTextSymbol->drawAlongPath
                        && !otherTextSymbol->drawOnPath;
                    bool equalPlacement = text->placement == otherTextSymbol->placement;

                    return bothAlongPath || bothOnPath || bothWithoutPath && equalPlacement;
                }
                return false;
            });
        if (textPositioningTaken)
            return;

        // Get additional text from nameTag2 if present (and not yet added)
        if (!extraCaptionTextAdded)
        {
            QString nameTag2;
            ok = evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_NAME_TAG2, nameTag2);
            if (ok && !nameTag2.isEmpty())
            {
                const auto citNameTag2AttributesGroup = attributeMapping->encodeMap.constFind(&nameTag2);
                if (citNameTag2AttributesGroup != attributeMapping->encodeMap.constEnd())
                {
                    const auto& nameTag2AttributesGroup = *citNameTag2AttributesGroup;
                    for (const auto& nameTag2AttributeEntry : rangeOf(constOf(nameTag2AttributesGroup)))
                    {
                        const auto attributeId = nameTag2AttributeEntry.value();
                        const auto citExtraCaption = mapObject->captions.constFind(attributeId);

                        if (citExtraCaption == mapObject->captions.constEnd())
                            continue;
                        const auto& extraCaption = *citExtraCaption;
                        if (extraCaption.isEmpty())
                            continue;

                        // Skip extra caption in case it's same as caption, to avoid "X (X)" cases
                        if (extraCaption == caption)
                            continue;

                        text->value += QString(QLatin1String(" (%1)")).arg(extraCaption);

                        extraCaptionTextAdded = true;
                        extraCaptionRuleId = attributeId;
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

        if (primitive->type == PrimitiveType::Point || primitive->type == PrimitiveType::Polygon)
            evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_DY, text->verticalOffset);

        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_COLOR, text->color.argb);
        if (!ok || text->color == ColorARGB::fromSkColor(SK_ColorTRANSPARENT))
            text->color = ColorARGB::fromSkColor(SK_ColorBLACK);

        text->size = textSize;

        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_HALO_RADIUS, text->shadowRadius);
        if (ok && text->shadowRadius > 0)
        {
            text->shadowRadius += 2; // + 2px
            //NOTE: ^^^ This is same as specifying 'x:2' in style, but due to backward compatibility with Android, leave as-is
        }

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

        ok = evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_MIN_DISTANCE, text->minDistance);
        if (!ok || qFuzzyIsNull(text->minDistance))
            text->minDistance = DEFAULT_TEXT_MIN_DISTANCE * env->displayDensityFactor * env->symbolsScaleFactor;

        evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_SHIELD, text->shieldResourceName);
        text->shieldResourceName = prepareIconValue(primitive->sourceObject,text->shieldResourceName);

        QString iconResourceName;
        if (evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON, iconResourceName))
            text->underlayIconResourceNames.push_back(prepareIconValue(mapObject, iconResourceName));
        if (evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_2, iconResourceName))
            text->underlayIconResourceNames.push_back(prepareIconValue(mapObject, iconResourceName));
        
        QString intersectsWith;
        ok = evaluationResult.getStringValue(
            env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTS_WITH,
            intersectsWith);
        if (!ok)
            intersectsWith = QLatin1String("text"); // To simulate original behavior, texts should intersect only other texts
        text->intersectsWith = intersectsWith.split(QLatin1Char(','), QString::SkipEmptyParts).toSet();

        float intersectionSizeFactor = std::numeric_limits<float>::quiet_NaN();
        ok = evaluationResult.getFloatValue(
            env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTION_SIZE_FACTOR,
            intersectionSizeFactor);
        if (ok)
            text->intersectionSizeFactor = intersectionSizeFactor;

        float intersectionSize = std::numeric_limits<float>::quiet_NaN();
        ok = !ok && evaluationResult.getFloatValue(
            env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTION_SIZE,
            intersectionSize);
        if (ok)
            text->intersectionSize = intersectionSize;

        float intersectionMargin = std::numeric_limits<float>::quiet_NaN();
        ok = !ok && evaluationResult.getFloatValue(
            env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTION_MARGIN,
            intersectionMargin);
        if (ok)
            text->intersectionMargin = intersectionMargin;

        // In case new text symbol has a twin that was already added, ignore this one
        const auto hasTwin = std::any_of(outSymbols,
            [text]
            (const std::shared_ptr<const Symbol>& otherSymbol) -> bool
            {
                if (const auto otherText = std::dynamic_pointer_cast<const TextSymbol>(otherSymbol))
                {
                    return text->hasSameContentAs(*otherText);
                }

                return false;
            });

        if (hasTwin)
        {
            if (metric)
            {
                metric->elapsedTimeForTextSymbolsProcessing += textProcessingStopwatch.elapsed();
                metric->rejectedTextSymbols++;
            }

            continue;
        }

        outSymbols.push_back(qMove(text));
        if (metric)
        {
            metric->elapsedTimeForTextSymbolsProcessing += textProcessingStopwatch.elapsed();
            metric->obtainedTextSymbols++;
        }
    }
}

QString OsmAnd::MapPrimitiviser_P::prepareIconValue(
    const std::shared_ptr<const MapObject>& object,
    const QString& genTagVal)
{
    return object->formatWithAdditionalAttributes(genTagVal);
}

void OsmAnd::MapPrimitiviser_P::obtainPrimitiveIcon(
    const Context& context,
    const std::shared_ptr<const Primitive>& primitive,
    const PointI& location,
    MapStyleEvaluationResult& evaluationResult,
    SymbolsCollection& outSymbols,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& env = context.env;

    //////////////////////////////////////////////////////////////////////////
    //if (primitive->sourceObject->toString().contains("1333827773"))
    //{
    //    int i = 5;
    //}
    //else
    //    return;
    //////////////////////////////////////////////////////////////////////////

    if (primitive->evaluationResult.isEmpty())
        return;

    bool ok;

    const Stopwatch iconProcessingStopwatch(metric != nullptr);

    QString iconResourceName;
    ok = primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON, iconResourceName);
    if (!ok || iconResourceName.isEmpty())
    {
        if (metric)
            metric->elapsedTimeForIconSymbolsProcessing += iconProcessingStopwatch.elapsed();

        return;
    }

    const std::shared_ptr<IconSymbol> icon(new IconSymbol(primitive));
    icon->drawAlongPath = (primitive->type == PrimitiveType::Polyline);
    icon->location31 = location;
    icon->scaleFactor = env->symbolsScaleFactor;
    
    icon->resourceName = prepareIconValue(primitive->sourceObject, iconResourceName);
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON__3, iconResourceName))
        icon->underlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON__2, iconResourceName))
        icon->underlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON__1, iconResourceName))
        icon->underlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_2, iconResourceName))
        icon->overlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_3, iconResourceName))
        icon->overlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_4, iconResourceName))
        icon->overlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_5, iconResourceName))
        icon->overlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));

    //NOTE: Also divide by 2, since for some reason factor is calculated using half-size, not size
    if (primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_SHIFT_PX, icon->offsetFactor.x))
        icon->offsetFactor.x *= 0.5f;
    if (primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_SHIFT_PY, icon->offsetFactor.y))
        icon->offsetFactor.y *= 0.5f;

    icon->order = 100;
    primitive->evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_ORDER, icon->order);
    //NOTE: a magic shifting of icon order. This is needed to keep icons less important than anything else
    icon->order += 1000000;

    evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_MIN_DISTANCE, icon->minDistance);

    primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_SHIELD, icon->shieldResourceName);

    QString intersectsWith;
    ok = primitive->evaluationResult.getStringValue(
        env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTS_WITH,
        intersectsWith);
    if (!ok)
        intersectsWith = QLatin1String("icon"); // To simulate original behavior, icons should intersect only other icons
    icon->intersectsWith = intersectsWith.split(QLatin1Char(','), QString::SkipEmptyParts).toSet();

    float intersectionSizeFactor = std::numeric_limits<float>::quiet_NaN();
    ok = primitive->evaluationResult.getFloatValue(
        env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTION_SIZE_FACTOR,
        intersectionSizeFactor);
    if (ok)
        icon->intersectionSizeFactor = intersectionSizeFactor;

    float intersectionSize = std::numeric_limits<float>::quiet_NaN();
    ok = !ok && primitive->evaluationResult.getFloatValue(
        env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTION_SIZE,
        intersectionSize);
    if (ok)
        icon->intersectionSize = intersectionSize;

    float intersectionMargin = std::numeric_limits<float>::quiet_NaN();
    ok = !ok && primitive->evaluationResult.getFloatValue(
        env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTION_MARGIN,
        intersectionMargin);
    if (ok)
        icon->intersectionMargin = intersectionMargin;

    //TODO: Obsolete
    if (!ok)
    {
        float iconIntersectionSize = std::numeric_limits<float>::quiet_NaN();
        ok = primitive->evaluationResult.getFloatValue(
            env->styleBuiltinValueDefs->id_OUTPUT_ICON_INTERSECTION_SIZE,
            iconIntersectionSize);
        if (ok)
            icon->intersectionSize = iconIntersectionSize;
    }

    // In case new icon symbol has a twin that was already added, ignore this one
    const auto hasTwin = std::any_of(outSymbols,
        [icon]
        (const std::shared_ptr<const Symbol>& otherSymbol) -> bool
        {
            if (const auto otherIcon = std::dynamic_pointer_cast<const IconSymbol>(otherSymbol))
            {
                return icon->hasSameContentAs(*otherIcon);
            }

            return false;
        });
    if (hasTwin)
    {
        if (metric)
        {
            metric->elapsedTimeForIconSymbolsProcessing += iconProcessingStopwatch.elapsed();
            metric->rejectedIconSymbols++;
        }

        return;
    }

    // Icons are always first
    outSymbols.prepend(qMove(icon));
    if (metric)
    {
        metric->elapsedTimeForIconSymbolsProcessing += iconProcessingStopwatch.elapsed();
        metric->obtainedIconSymbols++;
    }
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
    defaultSymbolPathSpacing = env->getDefaultSymbolPathSpacing();
    defaultBlockPathSpacing = env->getDefaultBlockPathSpacing();
}

bool OsmAnd::MapPrimitiviser_P::polygonizeCoastlines(
    const AreaI area31,
    const ZoomLevel zoom,
    const QList< std::shared_ptr<const MapObject> >& coastlines,
    QList< std::shared_ptr<const MapObject> >& outVectorized)
{
    std::vector<FoundMapDataObject> legacyCoastlineObjects;
    QList<shared_ptr<MapDataObject>> legacyObjectsHolder;
    for (const auto & c : constOf(coastlines))
    {
        auto dataObj = std::make_shared<MapDataObject>(convertToLegacy(*c));
        legacyObjectsHolder.push_back(dataObj);
        FoundMapDataObject legacyObj(dataObj.get());
        legacyCoastlineObjects.push_back(legacyObj);
    }
    std::vector<FoundMapDataObject> legacyCoastlineObjectsResult;
    // Get polygons of coastlines circumcised by sides of area31 or isolated islands/lakes
    bool processed = processCoastlines(legacyCoastlineObjects, (int) area31.topLeft.x, (int) area31.bottomRight.x, (int) area31.bottomRight.y,
                      (int) area31.topLeft.y, static_cast<int>(zoom), false, false, legacyCoastlineObjectsResult);
    if (processed)
    {
        int size = 0;
        for (const auto & legacyCoastline : constOf(legacyCoastlineObjectsResult))
        {
            const auto & mapObj = convertFromLegacy(legacyCoastline.obj);
            size += mapObj->points31.size();
            outVectorized.push_back(mapObj);
        }
        if (size < 3)
        {
            // Polygon needs mimimum 3 points (bug with part of coastline on the border of tile)
            processed = false;
        }
    }
    return processed;
}

MapDataObject OsmAnd::MapPrimitiviser_P::convertToLegacy(const MapObject & coreObj)
{
    MapDataObject legacyObj;
    for (const auto & p : coreObj.points31)
    {
        legacyObj.points.push_back(std::make_pair(p.x, p.y));
    }
    for (const uint32_t & a : coreObj.attributeIds)
    {
        auto tagVal = coreObj.attributeMapping->decodeMap[a];
        legacyObj.types.push_back(std::make_pair(tagVal.tag.toStdString(), tagVal.value.toStdString()));
    }
    for (const uint32_t & a : coreObj.additionalAttributeIds)
    {
        auto tagVal = coreObj.attributeMapping->decodeMap[a];
        legacyObj.additionalTypes.push_back(std::make_pair(tagVal.tag.toStdString(), tagVal.value.toStdString()));
    }
    legacyObj.polygonInnerCoordinates.reserve(coreObj.innerPolygonsPoints31.size());
    for (const auto & innerVector : coreObj.innerPolygonsPoints31)
    {
        std::vector<pair<int, int> > coordinates;
        for (const auto & c : innerVector)
        {
            coordinates.push_back(std::make_pair(c.x, c.y));
        }
        legacyObj.polygonInnerCoordinates.push_back(coordinates);
    }
    legacyObj.area = coreObj.isArea;
    return legacyObj;
}

const std::shared_ptr<OsmAnd::MapObject> OsmAnd::MapPrimitiviser_P::convertFromLegacy(const MapDataObject * legacyObj)
{
    const std::shared_ptr<MapObject> mapObject(new CoastlineMapObject());
    for (auto const & point : constOf(legacyObj->points))
    {
        PointI p(point.first, point.second);
        mapObject->points31.push_back(p);
    }
    mapObject->isArea = legacyObj->area;
    for (const auto & t : constOf(legacyObj->types))
    {
        if (t.first == "natural")
        {
            if (t.second == "coastline")
                mapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalCoastlineAttributeId);
            if (t.second == "land")
                mapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalLandAttributeId);
            if (t.second == "coastline_line")
                mapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalCoastlineLineAttributeId);
            if (t.second == "coastline_broken")
                mapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalCoastlineBrokenAttributeId);
        }
    }
    return mapObject;
}


// Try to draw lines through of the center of area and find intersection with closest coastline
OsmAnd::MapSurfaceType OsmAnd::MapPrimitiviser_P::determineSurfaceType(AreaI area31, QList< std::shared_ptr<const MapObject> >& coastlines)
{
    const PointI center = area31.center();
    int minDist = INT_MAX;
    std::pair<PointI, PointI> foundSegment;
    bool isOcean = false;
    OsmAnd::MapSurfaceType surfaceType = MapSurfaceType::Undefined;
    for (const auto & mapObj : constOf(coastlines))
    {
        if (mapObj->points31.size() < 2)
            continue;
        
        for (int i = 0; i < mapObj->points31.size() - 1; i++)
        {
            const PointI & start = mapObj->points31.at(i);
            const PointI & end = mapObj->points31.at(i + 1);
            int intersectX = ray_intersect_x(start.x, start.y, end.x, end.y, center.y);
            if (intersectX != INT_MAX && intersectX != INT_MIN)
            {
                if (abs(intersectX - center.x) < minDist)
                {
                    // previous segment further than found
                    minDist = abs(intersectX - center.x);
                    
                    //water is always in right side
                    if ((intersectX < center.x && end.x > center.x) || (intersectX > center.x && end.x < center.x))
                    {
                        //the excluding when coastline is too big and "end" point in other quater
                        isOcean = (start.y - center.y > 0 && start.x - center.x < 0) || (start.y - center.y < 0 && start.x - center.x > 0);
                    }
                    else
                    {
                        //(end.y - center.y) * (end.x - center.x) - overflow INT
                        isOcean = (end.y - center.y < 0 && end.x - center.x < 0) || (end.y - center.y > 0 && end.x - center.x > 0);
                    }
                    
                    if (isOcean)
                    {
                        surfaceType = MapSurfaceType::FullWater;
                    }
                    else
                    {
                        surfaceType = MapSurfaceType::FullLand;
                    }
                }
            }
            int intersectY = ray_intersect_y(start.x, start.y, end.x, end.y, center.x);
            if (intersectY != INT_MAX && intersectY != INT_MIN)
            {
                if (abs(intersectY - center.y) < minDist)
                {
                    // previous segment further than found
                    minDist = abs(intersectY - center.y);
                    
                    //water is always in right side
                    if ((intersectY < center.y && end.y > center.y) || (intersectY > center.y && end.y < center.y))
                    {
                        //the excluding when coastline is too big and "end" point in other quater
                        isOcean =
                            (start.x > center.x && start.y > center.y) || (start.x < center.x && start.y < center.y);
                    }
                    else
                    {
                        isOcean = (end.x < center.x && end.y > center.y) || (end.x > center.x && end.y < center.y);
                    }
                    
                    if (isOcean)
                    {
                        surfaceType = MapSurfaceType::FullWater;
                    }
                    else
                    {
                        surfaceType = MapSurfaceType::FullLand;
                    }
                }
            }
        }
    }
    return surfaceType;
}

// Increase area on few points for avoid lines on the borders of tile
const OsmAnd::AreaI OsmAnd::MapPrimitiviser_P::getWidenArea(const AreaI & area31, const ZoomLevel & zoom) const
{
    if (zoom > ZoomLevel::ZoomLevel14)
    {
        int widen = zoom - ZoomLevel14;
        const AreaI & area = area31.getEnlargedBy(widen);
        return area;
    }
    return area31;
}

void OsmAnd::MapPrimitiviser_P::debugCoastline(const AreaI & area31, const QList< std::shared_ptr<const MapObject>> & coastlines) const
{
    LogPrintf(LogSeverityLevel::Debug, "area \n topLeft: [%d %d] [%f %f] \n bottomRight: [%d %d] [%f %f]",
              area31.top(), area31.left(),
              Utilities::get31LatitudeY(area31.topLeft.y),
              Utilities::get31LongitudeX(area31.topLeft.x),
              area31.bottom(), area31.right(),
              Utilities::get31LatitudeY(area31.bottomRight.y),
              Utilities::get31LongitudeX(area31.bottomRight.x));
    
    for (auto & c : coastlines)
    {
        LogPrintf(LogSeverityLevel::Debug, "LATITUDE, LONGITUDE");
        for (auto & p : c->points31)
        {
            LogPrintf(LogSeverityLevel::Debug, "%f, %f", Utilities::get31LatitudeY(p.y), Utilities::get31LongitudeX(p.x));
        }
    }
}
