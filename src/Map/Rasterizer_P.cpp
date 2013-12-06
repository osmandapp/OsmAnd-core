#include "Rasterizer_P.h"
#include "Rasterizer.h"
#include "Rasterizer_Metrics.h"

#include <cassert>
#include <set>

#include <QMutableVectorIterator>
#include <QReadWriteLock>

#include "RasterizerEnvironment.h"
#include "RasterizerEnvironment_P.h"
#include "RasterizerContext.h"
#include "RasterizerContext_P.h"
#include "RasterizerSharedContext.h"
#include "RasterizerSharedContext_P.h"
#include "RasterizedSymbol.h"
#include "RasterizedSymbolsGroup.h"
#include "MapStyleEvaluator.h"
#include "MapStyleEvaluationResult.h"
#include "MapTypes.h"
#include "MapObject.h"
#include "ObfMapSectionInfo.h"
#include "IQueryController.h"
#include "Utilities.h"
#include "Logging.h"

#include <SkBitmapDevice.h>
#include <SkBlurDrawLooper.h>
#include <SkColorFilter.h>
#include <SkDashPathEffect.h>
#include <SkBitmapProcShader.h>
#include <SkError.h>

OsmAnd::Rasterizer_P::Rasterizer_P(Rasterizer* const owner_, const RasterizerEnvironment_P& env_, const RasterizerContext_P& context_)
    : owner(owner_)
    , env(env_)
    , context(context_)
{
}

OsmAnd::Rasterizer_P::~Rasterizer_P()
{
}

void OsmAnd::Rasterizer_P::prepareContext(
    const RasterizerEnvironment_P& env,
    RasterizerContext_P& context,
    const AreaI& area31,
    const ZoomLevel zoom,
    const MapFoundationType foundation,
    const QList< std::shared_ptr<const Model::MapObject> >& objects,
    bool* nothingToRasterize,
    const IQueryController* const controller,
    Rasterizer_Metrics::Metric_prepareContext* const metric)
{
    context.clear();

    adjustContextFromEnvironment(env, context, zoom);

    context._tileDivisor = Utilities::getPowZoom(31 - zoom);

    context._zoom = zoom;
    context._area31 = area31;

    // Update metric
    std::chrono::high_resolution_clock::time_point objectsSorting_begin;
    if(metric)
        objectsSorting_begin = std::chrono::high_resolution_clock::now();

    // Split input map objects to object, coastline, basemapObjects and basemapCoastline
    QList< std::shared_ptr<const OsmAnd::Model::MapObject> > detailedmapMapObjects, detailedmapCoastlineObjects, basemapMapObjects, basemapCoastlineObjects;
    QList< std::shared_ptr<const OsmAnd::Model::MapObject> > polygonizedCoastlineObjects;
    for(auto itMapObject = objects.cbegin(); itMapObject != objects.cend(); ++itMapObject)
    {
        if(controller && controller->isAborted())
            break;

        const auto& mapObject = *itMapObject;

        if(zoom < BasemapZoom && !mapObject->section->isBasemap)
            continue;

        if(mapObject->containsType(mapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId))
        {
            if(mapObject->section->isBasemap)
                basemapCoastlineObjects.push_back(mapObject);
            else
                detailedmapCoastlineObjects.push_back(mapObject);
        }
        else
        {
            if(mapObject->section->isBasemap)
                basemapMapObjects.push_back(mapObject);
            else
                detailedmapMapObjects.push_back(mapObject);
        }
    }

    // Cleanup if aborted
    if(controller && controller->isAborted())
    {
        // Update metric
        if(metric)
        {
            const std::chrono::duration<float> objectsSorting_elapsed = std::chrono::high_resolution_clock::now() - objectsSorting_begin;
            metric->elapsedTimeForSortingObjects += objectsSorting_elapsed.count();
        }

        context.clear();
        return;
    }

    // Update metric
    std::chrono::high_resolution_clock::time_point polygonizeCoastlines_begin;
    if(metric)
    {
        const std::chrono::duration<float> objectsSorting_elapsed = std::chrono::high_resolution_clock::now() - objectsSorting_begin;
        metric->elapsedTimeForSortingObjects += objectsSorting_elapsed.count();

        polygonizeCoastlines_begin = std::chrono::high_resolution_clock::now();
    }

    // Polygonize coastlines
    bool fillEntireArea = true;
    bool addBasemapCoastlines = true;
    const bool detailedLandData = zoom >= DetailedLandDataZoom && !detailedmapMapObjects.isEmpty();
    if(!detailedmapCoastlineObjects.empty())
    {
        const bool coastlinesWereAdded = polygonizeCoastlines(env, context,
            detailedmapCoastlineObjects,
            polygonizedCoastlineObjects,
            !basemapCoastlineObjects.isEmpty(),
            true);
        fillEntireArea = !coastlinesWereAdded && fillEntireArea;
        addBasemapCoastlines = (!coastlinesWereAdded && !detailedLandData) || zoom <= BasemapZoom;
    }
    else
    {
        addBasemapCoastlines = !detailedLandData;
    }
    if(addBasemapCoastlines)
    {
        const bool coastlinesWereAdded = polygonizeCoastlines(env, context,
            basemapCoastlineObjects,
            polygonizedCoastlineObjects,
            false,
            true);
        fillEntireArea = !coastlinesWereAdded && fillEntireArea;
    }

    // Update metric
    if(metric)
    {
        const std::chrono::duration<float> polygonizeCoastlines_elapsed = std::chrono::high_resolution_clock::now() - polygonizeCoastlines_begin;
        metric->elapsedTimeForPolygonizingCoastlines += polygonizeCoastlines_elapsed.count();
        metric->polygonizedCoastlines = polygonizedCoastlineObjects.size();
    }

    if(basemapMapObjects.isEmpty() && detailedmapMapObjects.isEmpty() && foundation == MapFoundationType::Undefined)
    {
        // We simply have no data to render. Report, clean-up and exit
        if(nothingToRasterize)
            *nothingToRasterize = true;

        context.clear();
        return;
    }

    if(fillEntireArea)
    {
        assert(foundation != MapFoundationType::Undefined);

        const std::shared_ptr<Model::MapObject> bgMapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        bgMapObject->_isArea = true;
        bgMapObject->_points31.push_back(qMove(PointI(area31.left, area31.top)));
        bgMapObject->_points31.push_back(qMove(PointI(area31.right, area31.top)));
        bgMapObject->_points31.push_back(qMove(PointI(area31.right, area31.bottom)));
        bgMapObject->_points31.push_back(qMove(PointI(area31.left, area31.bottom)));
        bgMapObject->_points31.push_back(bgMapObject->_points31.first());
        if(foundation == MapFoundationType::FullWater)
            bgMapObject->_typesRuleIds.push_back(bgMapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
        else if(foundation == MapFoundationType::FullLand || foundation == MapFoundationType::Mixed)
            bgMapObject->_typesRuleIds.push_back(bgMapObject->section->encodingDecodingRules->naturalLand_encodingRuleId);
        else
        {
            bgMapObject->_isArea = false;
            bgMapObject->_typesRuleIds.push_back(bgMapObject->section->encodingDecodingRules->naturalCoastlineBroken_encodingRuleId);
        }
        bgMapObject->_extraTypesRuleIds.push_back(bgMapObject->section->encodingDecodingRules->layerLowest_encodingRuleId);

        assert(bgMapObject->isClosedFigure());
        polygonizedCoastlineObjects.push_back(qMove(bgMapObject));
    }

    // Obtain primitives
    const bool detailedDataMissing = zoom > BasemapZoom && detailedmapMapObjects.isEmpty() && detailedmapCoastlineObjects.isEmpty();

    // Check if there is no data to render. Report, clean-up and exit
    const auto mapObjectsCount =
        detailedmapMapObjects.size() +
        ((zoom <= BasemapZoom || detailedDataMissing) ? basemapMapObjects.size() : 0) +
        polygonizedCoastlineObjects.size();
    if(mapObjectsCount == 0)
    {
        if(nothingToRasterize)
            *nothingToRasterize = true;

        context.clear();
        return;
    }
    if(nothingToRasterize)
        *nothingToRasterize = false;

    // Update metric
    std::chrono::high_resolution_clock::time_point obtainPrimitives_begin;
    if(metric)
        obtainPrimitives_begin = std::chrono::high_resolution_clock::now();

    // Obtain primitives
    obtainPrimitives(env, context, detailedmapMapObjects, controller, metric);
    if(zoom <= BasemapZoom || detailedDataMissing)
        obtainPrimitives(env, context, basemapMapObjects, controller, metric);
    obtainPrimitives(env, context, polygonizedCoastlineObjects, controller, metric);
    sortAndFilterPrimitives(env, context);
    if(controller && controller->isAborted())
    {
        context.clear();
        return;
    }

    // Update metric
    if(metric)
    {
        const std::chrono::duration<float> obtainPrimitives_elapsed = std::chrono::high_resolution_clock::now() - obtainPrimitives_begin;
        metric->elapsedTimeForObtainingPrimitives += obtainPrimitives_elapsed.count();
    }

    // Update metric
    std::chrono::high_resolution_clock::time_point obtainPrimitivesSymbols_begin;
    if(metric)
        obtainPrimitivesSymbols_begin = std::chrono::high_resolution_clock::now();

    // Obtain symbols from primitives
    obtainPrimitivesSymbols(env, context, controller);

    // Update metric
    if(metric)
    {
        const std::chrono::duration<float> obtainPrimitivesSymbols_elapsed = std::chrono::high_resolution_clock::now() - obtainPrimitivesSymbols_begin;
        metric->elapsedTimeForObtainingPrimitivesSymbols += obtainPrimitivesSymbols_elapsed.count();
    }

    if(controller && controller->isAborted())
    {
        context.clear();
        return;
    }
}

void OsmAnd::Rasterizer_P::obtainPrimitives(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& source,
    const IQueryController* const controller,
    Rasterizer_Metrics::Metric_prepareContext* const metric)
{
    bool ok;

    // Initialize shared settings for order evaluation
    MapStyleEvaluator orderEvaluator(env.owner->style, env.owner->displayDensityFactor);
    env.applyTo(orderEvaluator);
    orderEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MINZOOM, context._zoom);
    orderEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MAXZOOM, context._zoom);

    // Initialize shared settings for polygon evaluation
    MapStyleEvaluator polygonEvaluator(env.owner->style, env.owner->displayDensityFactor);
    env.applyTo(polygonEvaluator);
    polygonEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MINZOOM, context._zoom);
    polygonEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MAXZOOM, context._zoom);

    // Initialize shared settings for polyline evaluation
    MapStyleEvaluator polylineEvaluator(env.owner->style, env.owner->displayDensityFactor);
    env.applyTo(polylineEvaluator);
    polylineEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MINZOOM, context._zoom);
    polylineEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MAXZOOM, context._zoom);

    // Initialize shared settings for point evaluation
    MapStyleEvaluator pointEvaluator(env.owner->style, env.owner->displayDensityFactor);
    env.applyTo(pointEvaluator);
    pointEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MINZOOM, context._zoom);
    pointEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MAXZOOM, context._zoom);

    for(auto itMapObject = source.cbegin(); itMapObject != source.cend(); ++itMapObject)
    {
        if(controller && controller->isAborted())
            return;

        const auto& mapObject = *itMapObject;
        const auto isMapObjectGenerated = (mapObject->section == env.dummyMapSection);
        
        // If using shared context is allowed, check if this group was already processed
        if(!isMapObjectGenerated && context.owner->sharedContext)
        {
            auto& primitivesCacheLevel = context.owner->sharedContext->_d->_primitivesCacheLevels[context._zoom];

            // If this group was already processed, use that
            std::shared_ptr<const PrimitivesGroup> group;
            {
                QReadLocker scopedLocker(&primitivesCacheLevel._lock);

                const auto itProcessedGroup = primitivesCacheLevel._cache.constFind(mapObject->id);
                if(itProcessedGroup != primitivesCacheLevel._cache.cend())
                    group = *itProcessedGroup;
            }

            if(static_cast<bool>(group))
            {
                // Add polygons, polylines and points from group to current context
                for(auto itPrimitive = group->polygons.cbegin(); itPrimitive != group->polygons.cend(); ++itPrimitive)
                    context._polygons.push_back(*itPrimitive);
                for(auto itPrimitive = group->polylines.cbegin(); itPrimitive != group->polylines.cend(); ++itPrimitive)
                    context._polylines.push_back(*itPrimitive);
                for(auto itPrimitive = group->points.cbegin(); itPrimitive != group->points.cend(); ++itPrimitive)
                    context._points.push_back(*itPrimitive);

                // Add shared group to current context
                context._primitivesGroups.push_back(qMove(group));

                continue;
            }
        }

        // Create a primitives group
        const auto constructedGroup = new PrimitivesGroup(mapObject);
        std::shared_ptr<const PrimitivesGroup> group(constructedGroup);

        uint32_t typeRuleIdIndex = 0;
        for(auto itTypeRuleId = mapObject->typesRuleIds.cbegin(); itTypeRuleId != mapObject->typesRuleIds.cend(); ++itTypeRuleId, typeRuleIdIndex++)
        {
            const auto& decodedType = mapObject->section->encodingDecodingRules->decodingRules[*itTypeRuleId];

            // Update metric
            std::chrono::high_resolution_clock::time_point orderEvaluation_begin;
            if(metric)
                orderEvaluation_begin = std::chrono::high_resolution_clock::now();
            
            // Setup mapObject-specific input data
            orderEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
            orderEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);
            orderEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_LAYER, mapObject->getSimpleLayerValue());
            orderEvaluator.setBooleanValue(env.styleBuiltinValueDefs->id_INPUT_AREA, mapObject->isArea);
            orderEvaluator.setBooleanValue(env.styleBuiltinValueDefs->id_INPUT_POINT, mapObject->points31.size() == 1);
            orderEvaluator.setBooleanValue(env.styleBuiltinValueDefs->id_INPUT_CYCLE, mapObject->isClosedFigure());

            MapStyleEvaluationResult orderEvalResult;
            ok = orderEvaluator.evaluate(mapObject, MapStyleRulesetType::Order, &orderEvalResult);

            // Update metric
            if(metric)
            {
                const std::chrono::duration<float> orderEvaluation_elapsed = std::chrono::high_resolution_clock::now() - orderEvaluation_begin;
                metric->elapsedTimeForOrderEvaluation += orderEvaluation_elapsed.count();
                metric->orderEvaluations++;
            }

            // If evaluation failed, skip
            if(!ok)
                continue;

            int objectType;
            if(!orderEvalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_OBJECT_TYPE, objectType))
                continue;
            int zOrder;
            if(!orderEvalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_ORDER, zOrder))
                continue;

            // Create new primitive
            std::shared_ptr<Primitive> primitive(new Primitive(group, mapObject, static_cast<PrimitiveType>(objectType), typeRuleIdIndex));
            primitive->zOrder = zOrder;

            if(objectType == PrimitiveType::Polygon)
            {
                // Perform checks on data
                if(mapObject->points31.size() <= 2)
                {
                    OsmAnd::LogPrintf(LogSeverityLevel::Warning,
                        "Map object #%" PRIu64 " (%" PRIi64 ") primitive is processed as polygon, but has only %d point(s)",
                        mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2,
                        mapObject->points31.size());
                    continue;
                }
                if(!mapObject->isClosedFigure())
                {
                    OsmAnd::LogPrintf(LogSeverityLevel::Warning,
                        "Map object #%" PRIu64 " (%" PRIi64 ") primitive is processed as polygon, but isn't closed",
                        mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2);
                    continue;
                }
                if(!mapObject->isClosedFigure(true))
                {
                    OsmAnd::LogPrintf(LogSeverityLevel::Warning,
                        "Map object #%" PRIu64 " (%" PRIi64 ") primitive is processed as polygon, but isn't closed (inner)",
                        mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2);
                    continue;
                }

                // Update metric
                std::chrono::high_resolution_clock::time_point polygonEvaluation_begin;
                if(metric)
                    polygonEvaluation_begin = std::chrono::high_resolution_clock::now();

                // Setup mapObject-specific input data
                polygonEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
                polygonEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);

                // Evaluate style for this primitive to check if it passes
                std::shared_ptr<MapStyleEvaluationResult> evaluatorState(new MapStyleEvaluationResult());
                ok = polygonEvaluator.evaluate(mapObject, MapStyleRulesetType::Polygon, evaluatorState.get());

                // Update metric
                if(metric)
                {
                    const std::chrono::duration<float> polygonEvaluation_elapsed = std::chrono::high_resolution_clock::now() - polygonEvaluation_begin;
                    metric->elapsedTimeForPolygonEvaluation += polygonEvaluation_elapsed.count();
                    metric->polygonEvaluations++;
                }

                // If evaluation failed, skip
                if(!ok)
                    continue;
                primitive->evaluationResult = evaluatorState;

                // Check size of polygon
                auto polygonArea31 = Utilities::polygonArea(mapObject->points31);
                if(polygonArea31 > PolygonAreaCutoffLowerThreshold)
                {
                    primitive->zOrder += 1.0 / polygonArea31;

                    // Duplicate primitive as point
                    std::shared_ptr<Primitive> pointPrimitive(new Primitive(group, mapObject, PrimitiveType::Point, typeRuleIdIndex));
                    pointPrimitive->zOrder = primitive->zOrder;

                    // Accept this primitive
                    constructedGroup->polygons.push_back(qMove(primitive));

                    // Update metric
                    if(metric)
                        metric->polygonPrimitives++;

                    // Update metric
                    std::chrono::high_resolution_clock::time_point pointEvaluation_begin;
                    if(metric)
                        pointEvaluation_begin = std::chrono::high_resolution_clock::now();

                    // Setup mapObject-specific input data
                    pointEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
                    pointEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);

                    // Evaluate Point rules
                    std::shared_ptr<MapStyleEvaluationResult> pointEvaluatorState(new MapStyleEvaluationResult());
                    ok = pointEvaluator.evaluate(mapObject, MapStyleRulesetType::Point, pointEvaluatorState.get());

                    // Update metric
                    if(metric)
                    {
                        const std::chrono::duration<float> pointEvaluation_elapsed = std::chrono::high_resolution_clock::now() - pointEvaluation_begin;
                        metric->elapsedTimeForPointEvaluation += pointEvaluation_elapsed.count();
                        metric->pointEvaluations++;
                    }
                    
                    // Point evaluation is a bit special, it's success only indicates that point has an icon
                    if(ok)
                        pointPrimitive->evaluationResult = pointEvaluatorState;

                    // Accept also point primitive only if typeIndex == 0 and (there is text or icon)
                    if(pointPrimitive->typeRuleIdIndex == 0 && (!mapObject->names.isEmpty() || ok))
                    {
                        constructedGroup->points.push_back(qMove(pointPrimitive));

                        // Update metric
                        if(metric)
                            metric->pointPrimitives++;
                    }
                }
            }
            else if(objectType == PrimitiveType::Polyline)
            {
                // Perform checks on data
                if(mapObject->points31.size() < 2)
                {
                    OsmAnd::LogPrintf(LogSeverityLevel::Warning,
                        "Map object #%" PRIu64 " (%" PRIi64 ") is processed as polyline, but has %d point(s)",
                        mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2,
                        mapObject->_points31.size());
                    continue;
                }

                // Update metric
                std::chrono::high_resolution_clock::time_point polylineEvaluation_begin;
                if(metric)
                    polylineEvaluation_begin = std::chrono::high_resolution_clock::now();

                // Setup mapObject-specific input data
                polylineEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
                polylineEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);
                polylineEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_LAYER, mapObject->getSimpleLayerValue());

                // Evaluate style for this primitive to check if it passes
                std::shared_ptr<MapStyleEvaluationResult> evaluatorState(new MapStyleEvaluationResult());
                ok = polylineEvaluator.evaluate(mapObject, MapStyleRulesetType::Polyline, evaluatorState.get());

                // Update metric
                if(metric)
                {
                    const std::chrono::duration<float> polylineEvaluation_elapsed = std::chrono::high_resolution_clock::now() - polylineEvaluation_begin;
                    metric->elapsedTimeForPolylineEvaluation += polylineEvaluation_elapsed.count();
                    metric->polylineEvaluations++;
                }

                // If evaluation failed, skip
                if(!ok)
                    continue;
                primitive->evaluationResult = evaluatorState;

                // Accept this primitive
                constructedGroup->polylines.push_back(qMove(primitive));

                // Update metric
                if(metric)
                    metric->polylinePrimitives++;
            }
            else if(objectType == PrimitiveType::Point)
            {
                // Perform checks on data
                if(mapObject->points31.size() < 1)
                {
                    OsmAnd::LogPrintf(LogSeverityLevel::Warning,
                        "Map object #%" PRIu64 " (%" PRIi64 ") is processed as point, but has no point",
                        mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2);
                    continue;
                }

                // Update metric
                std::chrono::high_resolution_clock::time_point pointEvaluation_begin;
                if(metric)
                    pointEvaluation_begin = std::chrono::high_resolution_clock::now();

                // Setup mapObject-specific input data
                pointEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
                pointEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);

                // Evaluate Point rules
                std::shared_ptr<MapStyleEvaluationResult> evaluatorState(new MapStyleEvaluationResult());
                ok = pointEvaluator.evaluate(mapObject, MapStyleRulesetType::Point, evaluatorState.get());

                // Update metric
                if(metric)
                {
                    const std::chrono::duration<float> pointEvaluation_elapsed = std::chrono::high_resolution_clock::now() - pointEvaluation_begin;
                    metric->elapsedTimeForPointEvaluation += pointEvaluation_elapsed.count();
                    metric->pointEvaluations++;
                }

                // Point evaluation is a bit special, it's success only indicates that point has an icon
                if(ok)
                    primitive->evaluationResult = evaluatorState;

                // Skip is possible if typeIndex != 0 or (there is no text and no icon)
                if(primitive->typeRuleIdIndex != 0 || (mapObject->names.isEmpty() && !ok))
                    continue;

                // Accept this primitive
                constructedGroup->points.push_back(qMove(primitive));

                // Update metric
                if(metric)
                    metric->pointPrimitives++;
            }
            else
            {
                assert(false);
                continue;
            }

            int shadowLevel;
            if(orderEvalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_SHADOW_LEVEL, shadowLevel) && shadowLevel > 0)
            {
                context._shadowLevelMin = qMin(context._shadowLevelMin, static_cast<uint32_t>(zOrder));
                context._shadowLevelMax = qMax(context._shadowLevelMax, static_cast<uint32_t>(zOrder));
            }
        }

        // Add this group to shared cache
        if(!isMapObjectGenerated && context.owner->sharedContext)
        {
            auto& primitivesCacheLevel = context.owner->sharedContext->_d->_primitivesCacheLevels[context._zoom];

            QWriteLocker scopedLocker(&primitivesCacheLevel._lock);

            const auto itProcessedGroup = primitivesCacheLevel._cache.constFind(mapObject->id);
            if(itProcessedGroup != primitivesCacheLevel._cache.cend())
            {
                // Replace current group with already available shared one. Unfortunately
                // current group was already duplicate
                group = *itProcessedGroup;
            }
            else
            {
                // Add current group to shared cache
                primitivesCacheLevel._cache.insert(mapObject->id, group);
            }
        }

        // Add polygons, polylines and points from group to current context
        for(auto itPrimitive = group->polygons.cbegin(); itPrimitive != group->polygons.cend(); ++itPrimitive)
            context._polygons.push_back(*itPrimitive);
        for(auto itPrimitive = group->polylines.cbegin(); itPrimitive != group->polylines.cend(); ++itPrimitive)
            context._polylines.push_back(*itPrimitive);
        for(auto itPrimitive = group->points.cbegin(); itPrimitive != group->points.cend(); ++itPrimitive)
            context._points.push_back(*itPrimitive);

        // Empty groups are also inserted, to indicate that they are empty
        context._primitivesGroups.push_back(qMove(group));
    }
}

void OsmAnd::Rasterizer_P::sortAndFilterPrimitives(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context)
{
    const auto privitivesSort = [](const std::shared_ptr<const Primitive>& l, const std::shared_ptr<const Primitive>& r) -> bool
    {
        if(qFuzzyCompare(l->zOrder, r->zOrder))
        {
            if(l->typeRuleIdIndex == r->typeRuleIdIndex)
                return l->mapObject->points31.size() < r->mapObject->points31.size();
            return l->typeRuleIdIndex < r->typeRuleIdIndex;
        }
        return l->zOrder < r->zOrder;
    };

    qSort(context._polygons.begin(), context._polygons.end(), privitivesSort);
    qSort(context._polylines.begin(), context._polylines.end(), privitivesSort);
    filterOutHighwaysByDensity(env, context);
    qSort(context._points.begin(), context._points.end(), privitivesSort);
}

void OsmAnd::Rasterizer_P::filterOutHighwaysByDensity(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context)
{
    // Check if any filtering needed
    if(context._roadDensityZoomTile == 0 || context._roadsDensityLimitPerTile == 0)
        return;

    const auto dZ = context._zoom + context._roadDensityZoomTile;
    QHash< uint64_t, std::pair<uint32_t, double> > densityMap;
    
    QMutableVectorIterator< std::shared_ptr<const Primitive> > itLine(context._polylines);
    itLine.toBack();
    while(itLine.hasPrevious())
    {
        const auto& line = itLine.previous();

        auto accept = true;
        if(line->mapObject->_typesRuleIds[line->typeRuleIdIndex] == line->mapObject->section->encodingDecodingRules->highway_encodingRuleId)
        {
            accept = false;

            uint64_t prevId = 0;
            const auto pointsCount = line->mapObject->points31.size();
            auto pPoint = line->mapObject->points31.constData();
            for(auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
            {
                auto x = pPoint->x >> (31 - dZ);
                auto y = pPoint->y >> (31 - dZ);
                uint64_t id = (static_cast<uint64_t>(x) << dZ) | y;
                if(prevId != id)
                {
                    prevId = id;

                    auto& mapEntry = densityMap[id];
                    if(mapEntry.first < context._roadsDensityLimitPerTile /*&& p.second > o */)
                    {
                        accept = true;
                        mapEntry.first += 1;
                        mapEntry.second = line->zOrder;
                    }
                }
            }
        }

        if(!accept)
            itLine.remove();
    }
}

void OsmAnd::Rasterizer_P::obtainPrimitivesSymbols(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const IQueryController* const controller )
{
    //NOTE: Since 2 tiles with same MapObject may have different set of polylines, generated from it,
    //NOTE: then set of symbols also should differ, but it won't.
    for(auto itPrimitivesGroup = context._primitivesGroups.cbegin(); itPrimitivesGroup != context._primitivesGroups.cend(); ++itPrimitivesGroup)
    {
        if(controller && controller->isAborted())
            return;

        const auto& primitivesGroup = *itPrimitivesGroup;
        const auto isMapObjectGenerated = (primitivesGroup->mapObject->section == env.dummyMapSection);

        // If using shared context is allowed, check if this group was already processed
        // (using shared cache is only allowed for non-generated MapObjects)
        if(!isMapObjectGenerated && context.owner->sharedContext)
        {
            auto& symbolsCacheLevel = context.owner->sharedContext->_d->_symbolsCacheLevels[context._zoom];

            // If this group was already processed, use that
            std::shared_ptr<const SymbolsGroup> group;
            {
                QReadLocker scopedLocker(&symbolsCacheLevel._lock);

                const auto itProcessedGroup = symbolsCacheLevel._cache.constFind(primitivesGroup->mapObject->id);
                if(itProcessedGroup != symbolsCacheLevel._cache.cend())
                    group = *itProcessedGroup;
            }

            if(static_cast<bool>(group))
            {
                // Add symbols from group to current context
                RasterizerContext_P::SymbolsEntry entry;
                entry.first = primitivesGroup->mapObject;
                entry.second.reserve(group->symbols.size());
                for(auto itSymbol = group->symbols.cbegin(); itSymbol != group->symbols.cend(); ++itSymbol)
                    entry.second.push_back(*itSymbol);
                entry.second.squeeze();
                context._symbols.push_back(qMove(entry));

                // Add shared group to current context
                context._symbolsGroups.push_back(qMove(group));

                continue;
            }
        }

        // Create a symbols group
        const auto constructedGroup = new SymbolsGroup(primitivesGroup->mapObject);
        std::shared_ptr<const SymbolsGroup> group(constructedGroup);

        // For each primitive if primitive group, collect symbols from it
        collectSymbolsFromPrimitives(env, context, primitivesGroup->polygons, Polygons, constructedGroup->symbols, controller);
        collectSymbolsFromPrimitives(env, context, primitivesGroup->polylines, Polylines, constructedGroup->symbols, controller);
        collectSymbolsFromPrimitives(env, context, primitivesGroup->points, Points, constructedGroup->symbols, controller);

        // Add this group to shared cache
        // (using shared cache is only allowed for non-generated MapObjects)
        if(!isMapObjectGenerated && context.owner->sharedContext)
        {
            auto& symbolsCacheLevel = context.owner->sharedContext->_d->_symbolsCacheLevels[context._zoom];

            QWriteLocker scopedLocker(&symbolsCacheLevel._lock);

            const auto itProcessedGroup = symbolsCacheLevel._cache.constFind(primitivesGroup->mapObject->id);
            if(itProcessedGroup != symbolsCacheLevel._cache.cend())
            {
                // Replace current group with already available shared one. Unfortunately
                // current group was already duplicate
                group = *itProcessedGroup;
            }
            else
            {
                // Add current group to shared cache
                symbolsCacheLevel._cache.insert(primitivesGroup->mapObject->id, group);
            }
        }

        // Add symbols from group to current context
        RasterizerContext_P::SymbolsEntry entry;
        entry.first = primitivesGroup->mapObject;
        entry.second.reserve(group->symbols.size());
        for(auto itSymbol = group->symbols.cbegin(); itSymbol != group->symbols.cend(); ++itSymbol)
            entry.second.push_back(*itSymbol);
        entry.second.squeeze();
        context._symbols.push_back(qMove(entry));

        // Empty groups are also inserted, to indicate that they are empty
        context._symbolsGroups.push_back(qMove(group));
    }
}

void OsmAnd::Rasterizer_P::collectSymbolsFromPrimitives(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const QVector< std::shared_ptr<const Primitive> >& primitives, const PrimitivesType type,
    QVector< std::shared_ptr<const PrimitiveSymbol> >& outSymbols,
    const IQueryController* const controller)
{
    assert(type != PrimitivesType::Polylines_ShadowOnly);

    for(auto itPrimitive = primitives.cbegin(); itPrimitive != primitives.cend(); ++itPrimitive)
    {
        if(controller && controller->isAborted())
            return;

        const auto& primitive = *itPrimitive;

        if(type == Polygons)
        {
            obtainSymbolsFromPolygon(env, context, primitive, outSymbols);
        }
        else if(type == Polylines)
        {
            obtainSymbolsFromPolyline(env, context, primitive, outSymbols);
        }
        else if(type == Points)
        {
            assert(primitive->typeRuleIdIndex == 0);

            obtainSymbolsFromPoint(env, context, primitive, outSymbols);
        }
    }
}

void OsmAnd::Rasterizer_P::obtainSymbolsFromPolygon(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const std::shared_ptr<const Primitive>& primitive,
    QVector< std::shared_ptr<const PrimitiveSymbol> >& outSymbols)
{
    assert(primitive->mapObject->points31.size() > 2);
    assert(primitive->mapObject->isClosedFigure());
    assert(primitive->mapObject->isClosedFigure(true));

    // Get center of polygon, since all symbols of polygon are related to it's center
    PointI64 center;
    const auto pointsCount = primitive->mapObject->points31.size();
    auto pPoint = primitive->mapObject->points31.constData();
    for(auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        center.x += pPoint->x;
        center.y += pPoint->y;
    }
    center.x /= pointsCount;
    center.y /= pointsCount;

    // Obtain texts for this symbol
    obtainPrimitiveTexts(env, context, primitive, Utilities::normalizeCoordinates(center, ZoomLevel31), outSymbols);
}

void OsmAnd::Rasterizer_P::obtainSymbolsFromPolyline(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const std::shared_ptr<const Primitive>& primitive,
    QVector< std::shared_ptr<const PrimitiveSymbol> >& outSymbols)
{
    assert(primitive->mapObject->points31.size() >= 2);

    // Symbols for polyline are always related to it's "middle" point
    const auto center = primitive->mapObject->points31[primitive->mapObject->points31.size() >> 1];

    // Obtain texts for this symbol
    obtainPrimitiveTexts(env, context, primitive, center, outSymbols);
}

void OsmAnd::Rasterizer_P::obtainSymbolsFromPoint(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const std::shared_ptr<const Primitive>& primitive,
    QVector< std::shared_ptr<const PrimitiveSymbol> >& outSymbols)
{
    assert(primitive->mapObject->points31.size() > 0);

    // Depending on type of point, center is determined differently
    PointI center;
    if(primitive->mapObject->points31.size() == 1)
    {
        // Regular point
        center = primitive->mapObject->points31.first();
    }
    else
    {
        // Point represents center of polygon
        PointI64 center_;
        const auto pointsCount = primitive->mapObject->points31.size();
        auto pPoint = primitive->mapObject->points31.constData();
        for(auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
        {
            center_.x += pPoint->x;
            center_.y += pPoint->y;
        }
        center_.x /= pointsCount;
        center_.y /= pointsCount;

        center = Utilities::normalizeCoordinates(center_, ZoomLevel31);
    }

    // Obtain icon for this symbol
    obtainPrimitiveIcon(env, context, primitive, center, outSymbols);

    // Obtain texts for this symbol
    obtainPrimitiveTexts(env, context, primitive, center, outSymbols);
}

void OsmAnd::Rasterizer_P::obtainPrimitiveTexts(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const std::shared_ptr<const Primitive>& primitive, const PointI& location,
    QVector< std::shared_ptr<const PrimitiveSymbol> >& outSymbols)
{
    const auto typeRuleId = primitive->mapObject->_typesRuleIds[primitive->typeRuleIdIndex];
    const auto& decodedType = primitive->mapObject->section->encodingDecodingRules->decodingRules[typeRuleId];

    MapStyleEvaluationResult textEvalResult;

    MapStyleEvaluator textEvaluator(env.owner->style, env.owner->displayDensityFactor);
    env.applyTo(textEvaluator);

    bool ok;
    for(auto itName = primitive->mapObject->names.cbegin(); itName != primitive->mapObject->names.cend(); ++itName)
    {
        const auto& name = itName.value();

        // Skip empty names
        if(name.isEmpty())
            continue;

        //TODO: reshape name with icu4c

        // Evaluate style to obtain text parameters
        textEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_TAG, decodedType.tag);
        textEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_VALUE, decodedType.value);
        textEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MINZOOM, context._zoom);
        textEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MAXZOOM, context._zoom);
        textEvaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_TEXT_LENGTH, name.length());

        QString nameTag;
        if(itName.key() != primitive->mapObject->section->encodingDecodingRules->name_encodingRuleId)
            nameTag = primitive->mapObject->section->encodingDecodingRules->decodingRules[itName.key()].tag;

        textEvaluator.setStringValue(env.styleBuiltinValueDefs->id_INPUT_NAME_TAG, nameTag);

        textEvalResult.clear();
        if(!textEvaluator.evaluate(primitive->mapObject, MapStyleRulesetType::Text, &textEvalResult))
            continue;

        // Skip text that doesn't have valid size
        int textSize = 0;
        ok = textEvalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_TEXT_SIZE, textSize);
        if(!ok || textSize == 0)
            continue;

        // Create primitive
        const auto text = new PrimitiveSymbol_Text();
        text->primitive = primitive;
        text->location31 = location;
        text->value = name;

        text->drawOnPath = false;
        textEvalResult.getBooleanValue(env.styleBuiltinValueDefs->id_OUTPUT_TEXT_ON_PATH, text->drawOnPath);

        textEvalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_TEXT_ORDER, text->order);

        text->verticalOffset = 0;
        textEvalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_TEXT_DY, text->verticalOffset);

        ok = textEvalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_TEXT_COLOR, text->color);
        if(!ok || !text->color)
            text->color = SK_ColorBLACK;

        text->size = textSize;

        text->shadowRadius = 0;
        textEvalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_TEXT_HALO_RADIUS, text->shadowRadius);

        text->wrapWidth = 0;
        textEvalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_TEXT_WRAP_WIDTH, text->wrapWidth);

        text->isBold = false;
        textEvalResult.getBooleanValue(env.styleBuiltinValueDefs->id_OUTPUT_TEXT_BOLD, text->isBold);

        text->minDistance = 0;
        textEvalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_TEXT_MIN_DISTANCE, text->minDistance);

        textEvalResult.getStringValue(env.styleBuiltinValueDefs->id_OUTPUT_TEXT_SHIELD, text->shieldResourceName);

        outSymbols.push_back(qMove(std::shared_ptr<PrimitiveSymbol>(text)));
    }
}

void OsmAnd::Rasterizer_P::obtainPrimitiveIcon(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const std::shared_ptr<const Primitive>& primitive, const PointI& location,
    QVector< std::shared_ptr<const PrimitiveSymbol> >& outSymbols)
{
    if(!primitive->evaluationResult)
        return;

    bool ok;

    QString iconResourceName;
    ok = primitive->evaluationResult->getStringValue(env.styleBuiltinValueDefs->id_OUTPUT_ICON, iconResourceName);

    if(ok && !iconResourceName.isEmpty())
    {
        const auto icon = new PrimitiveSymbol_Icon();
        icon->primitive = primitive;
        icon->location31 = location;

        icon->resourceName = qMove(iconResourceName);

        icon->order = 100;
        primitive->evaluationResult->getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_ICON, icon->order);

        outSymbols.push_back(qMove(std::shared_ptr<PrimitiveSymbol>(icon)));
    }
}

void OsmAnd::Rasterizer_P::adjustContextFromEnvironment(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const ZoomLevel zoom)
{
    MapStyleEvaluationResult evalResult;

    context._defaultBgColor = env.defaultBgColor;
    if(env.attributeRule_defaultColor)
    {
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor);
        env.applyTo(evaluator);
        evaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        evalResult.clear();
        if(evaluator.evaluate(env.attributeRule_defaultColor, &evalResult))
            evalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_ATTR_COLOR_VALUE, context._defaultBgColor);
    }

    context._shadowRenderingMode = env.shadowRenderingMode;
    context._shadowRenderingColor = env.shadowRenderingColor;
    if(env.attributeRule_shadowRendering)
    {
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor);
        env.applyTo(evaluator);
        evaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        evalResult.clear();
        if(evaluator.evaluate(env.attributeRule_shadowRendering, &evalResult))
        {
            evalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, context._shadowRenderingMode);
            evalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_SHADOW_COLOR, context._shadowRenderingColor);
        }
    }

    context._polygonMinSizeToDisplay = env.polygonMinSizeToDisplay;
    if(env.attributeRule_polygonMinSizeToDisplay)
    {
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor);
        env.applyTo(evaluator);
        evaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        evalResult.clear();
        if(evaluator.evaluate(env.attributeRule_polygonMinSizeToDisplay, &evalResult))
        {
            int polygonMinSizeToDisplay;
            if(evalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, polygonMinSizeToDisplay))
                context._polygonMinSizeToDisplay = polygonMinSizeToDisplay;
        }
    }

    context._roadDensityZoomTile = env.roadDensityZoomTile;
    if(env.attributeRule_roadDensityZoomTile)
    {
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor);
        env.applyTo(evaluator);
        evaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
        
        evalResult.clear();
        if(evaluator.evaluate(env.attributeRule_roadDensityZoomTile, &evalResult))
            evalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, context._roadDensityZoomTile);
    }

    context._roadsDensityLimitPerTile = env.roadsDensityLimitPerTile;
    if(env.attributeRule_roadsDensityLimitPerTile)
    {
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor);
        env.applyTo(evaluator);
        evaluator.setIntegerValue(env.styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
        
        evalResult.clear();
        if(evaluator.evaluate(env.attributeRule_roadsDensityLimitPerTile, &evalResult))
            evalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, context._roadsDensityLimitPerTile);
    }

    context._shadowLevelMin = env.shadowLevelMin;
    context._shadowLevelMax = env.shadowLevelMax;
}

void OsmAnd::Rasterizer_P::rasterizeMap(
    SkCanvas& canvas,
    const bool fillBackground,
    const AreaI* const destinationArea,
    const IQueryController* const controller)
{
    // Deal with background
    if(fillBackground)
    {
        if(destinationArea)
        {
            // If destination area is specified, fill only it with background
            SkPaint bgPaint;
            bgPaint.setColor(context._defaultBgColor);
            bgPaint.setStyle(SkPaint::kFill_Style);
            canvas.drawRectCoords(destinationArea->top, destinationArea->left, destinationArea->right, destinationArea->bottom, bgPaint);
        }
        else
        {
            // Since destination area is not specified, erase whole canvas with specified color
            canvas.clear(context._defaultBgColor);
        }
    }

    // Adopt initial paint setup from environment
    _mapPaint = env.mapPaint;

    // Precalculate values
    if(destinationArea)
    {
        _destinationArea = *destinationArea;
    }
    else
    {
        const auto targetSize = canvas.getDeviceSize();
        _destinationArea = AreaI(0, 0, targetSize.height(), targetSize.width());
    }
    _31toPixelDivisor.x = context._tileDivisor / static_cast<double>(_destinationArea.width());
    _31toPixelDivisor.y = context._tileDivisor / static_cast<double>(_destinationArea.height());

    // Rasterize layers of map:
    rasterizeMapPrimitives(destinationArea, canvas, context._polygons, Polygons, controller);
    if(context._shadowRenderingMode > 1)
        rasterizeMapPrimitives(destinationArea, canvas, context._polylines, Polylines_ShadowOnly, controller);
    rasterizeMapPrimitives(destinationArea, canvas, context._polylines, Polylines, controller);
}

void OsmAnd::Rasterizer_P::rasterizeMapPrimitives(
    const AreaI* const destinationArea,
    SkCanvas& canvas, const QVector< std::shared_ptr<const Primitive> >& primitives, PrimitivesType type, const IQueryController* const controller)
{
    assert(type != PrimitivesType::Points);

    const auto polygonMinSizeToDisplay31 = context._polygonMinSizeToDisplay * (_31toPixelDivisor.x * _31toPixelDivisor.y);
    const auto polygonSizeThreshold = 1.0 / polygonMinSizeToDisplay31;

    for(auto itPrimitive = primitives.cbegin(); itPrimitive != primitives.cend(); ++itPrimitive)
    {
        if(controller && controller->isAborted())
            return;

        const auto& primitive = *itPrimitive;

        if(type == Polygons)
        {
            if(primitive->zOrder > polygonSizeThreshold + static_cast<int>(primitive->zOrder))
                continue;

            rasterizePolygon(destinationArea, canvas, primitive);
        }
        else if(type == Polylines || type == Polylines_ShadowOnly)
        {
            rasterizePolyline(destinationArea, canvas, primitive, type == Polylines_ShadowOnly);
        }
    }
}

bool OsmAnd::Rasterizer_P::updatePaint(
    const MapStyleEvaluationResult& evalResult, const PaintValuesSet valueSetSelector, const bool isArea )
{
    bool ok = true;

    int valueDefId_color = -1;
    int valueDefId_strokeWidth = -1;
    int valueDefId_cap = -1;
    int valueDefId_pathEffect = -1;
    switch(valueSetSelector)
    {
    case PaintValuesSet::Set_0:
        valueDefId_color = env.styleBuiltinValueDefs->id_OUTPUT_COLOR;
        valueDefId_strokeWidth = env.styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH;
        valueDefId_cap = env.styleBuiltinValueDefs->id_OUTPUT_CAP;
        valueDefId_pathEffect = env.styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT;
        break;
    case PaintValuesSet::Set_1:
        valueDefId_color = env.styleBuiltinValueDefs->id_OUTPUT_COLOR_2;
        valueDefId_strokeWidth = env.styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_2;
        valueDefId_cap = env.styleBuiltinValueDefs->id_OUTPUT_CAP_2;
        valueDefId_pathEffect = env.styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT_2;
        break;
    case PaintValuesSet::Set_minus1:
        valueDefId_color = env.styleBuiltinValueDefs->id_OUTPUT_COLOR_0;
        valueDefId_strokeWidth = env.styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_0;
        valueDefId_cap = env.styleBuiltinValueDefs->id_OUTPUT_CAP_0;
        valueDefId_pathEffect = env.styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT_0;
        break;
    case PaintValuesSet::Set_minus2:
        valueDefId_color = env.styleBuiltinValueDefs->id_OUTPUT_COLOR__1;
        valueDefId_strokeWidth = env.styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH__1;
        valueDefId_cap = env.styleBuiltinValueDefs->id_OUTPUT_CAP__1;
        valueDefId_pathEffect = env.styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT__1;
        break;
    case PaintValuesSet::Set_3:
        valueDefId_color = env.styleBuiltinValueDefs->id_OUTPUT_COLOR_3;
        valueDefId_strokeWidth = env.styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_3;
        valueDefId_cap = env.styleBuiltinValueDefs->id_OUTPUT_CAP_3;
        valueDefId_pathEffect = env.styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT_3;
        break;
    }

    if(isArea)
    {
        _mapPaint.setColorFilter(nullptr);
        _mapPaint.setShader(nullptr);
        _mapPaint.setLooper(nullptr);
        _mapPaint.setStyle(SkPaint::kStrokeAndFill_Style);
        _mapPaint.setStrokeWidth(0);
    }
    else
    {
        float stroke;
        ok = evalResult.getFloatValue(valueDefId_strokeWidth, stroke);
        if(!ok || stroke <= 0.0f)
            return false;

        _mapPaint.setColorFilter(nullptr);
        _mapPaint.setShader(nullptr);
        _mapPaint.setLooper(nullptr);
        _mapPaint.setStyle(SkPaint::kStroke_Style);
        _mapPaint.setStrokeWidth(stroke);

        QString cap;
        ok = evalResult.getStringValue(valueDefId_cap, cap);
        if (!ok || cap.isEmpty() || cap == QLatin1String("BUTT"))
            _mapPaint.setStrokeCap(SkPaint::kButt_Cap);
        else if (cap == QLatin1String("ROUND"))
            _mapPaint.setStrokeCap(SkPaint::kRound_Cap);
        else if (cap == QLatin1String("SQUARE"))
            _mapPaint.setStrokeCap(SkPaint::kSquare_Cap);
        else
            _mapPaint.setStrokeCap(SkPaint::kButt_Cap);

        QString encodedPathEffect;
        ok = evalResult.getStringValue(valueDefId_pathEffect, encodedPathEffect);
        if(!ok || encodedPathEffect.isEmpty())
        {
            _mapPaint.setPathEffect(nullptr);
        }
        else
        {
            SkPathEffect* pathEffect = nullptr;
            ok = env.obtainPathEffect(encodedPathEffect, pathEffect);

            if(ok && pathEffect)
                _mapPaint.setPathEffect(pathEffect);
        }
    }

    SkColor color;
    ok = evalResult.getIntegerValue(valueDefId_color, color);
    if(!ok || !color)
        return false;
    _mapPaint.setColor(color);

    if (valueSetSelector == PaintValuesSet::Set_0)
    {
        QString shader;
        ok = evalResult.getStringValue(env.styleBuiltinValueDefs->id_OUTPUT_SHADER, shader);
        if(ok && !shader.isEmpty())
        {
            SkBitmapProcShader* shaderObj = nullptr;
            if(env.obtainBitmapShader(shader, shaderObj) && shaderObj)
            {
                _mapPaint.setShader(static_cast<SkShader*>(shaderObj));
                shaderObj->unref();
            }
        }
    }

    // do not check shadow color here
    if (context._shadowRenderingMode == 1 && valueSetSelector == PaintValuesSet::Set_0)
    {
        int shadowColor;
        ok = evalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_SHADOW_COLOR, shadowColor);
        int shadowRadius;
        evalResult.getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_SHADOW_RADIUS, shadowRadius);
        if(!ok || shadowColor == 0)
            shadowColor = context._shadowRenderingColor;
        if(shadowColor == 0)
            shadowRadius = 0;

        if(shadowRadius > 0)
            _mapPaint.setLooper(new SkBlurDrawLooper(static_cast<SkScalar>(shadowRadius), 0, 0, shadowColor))->unref();
    }

    return true;
}

void OsmAnd::Rasterizer_P::rasterizePolygon(
    const AreaI* const destinationArea,
    SkCanvas& canvas, const std::shared_ptr<const Primitive>& primitive)
{
    assert(primitive->mapObject->points31.size() > 2);
    assert(primitive->mapObject->isClosedFigure());
    assert(primitive->mapObject->isClosedFigure(true));

    if(!updatePaint(*primitive->evaluationResult, PaintValuesSet::Set_0, true))
        return;

    SkPath path;
    bool containsAtLeastOnePoint = false;
    int pointIdx = 0;
    PointF vertex;
    int bounds = 0;
    QVector< PointF > outsideBounds;
    const auto pointsCount = primitive->mapObject->points31.size();
    auto pPoint = primitive->mapObject->points31.constData();
    for(auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        calculateVertex(*pPoint, vertex);

        if(pointIdx == 0)
        {
            path.moveTo(vertex.x, vertex.y);
        }
        else
        {
            path.lineTo(vertex.x, vertex.y);
        }

        if(destinationArea && !containsAtLeastOnePoint)
        {
            if(destinationArea->contains(vertex))
            {
                containsAtLeastOnePoint = true;
            }
            else
            {
                outsideBounds.push_back(qMove(vertex));
            }
            bounds |= (vertex.x < destinationArea->left ? 1 : 0);
            bounds |= (vertex.x > destinationArea->right ? 2 : 0);
            bounds |= (vertex.y < destinationArea->top ? 4 : 0);
            bounds |= (vertex.y > destinationArea->bottom ? 8 : 0);
        }

    }

    if(destinationArea && !containsAtLeastOnePoint)
    {
        // fast check for polygons
        if((bounds & 3) != 3 || (bounds >> 2) != 3)
            return;

        bool ok = true;
        ok = ok || contains(outsideBounds, destinationArea->topLeft);
        ok = ok || contains(outsideBounds, destinationArea->bottomRight);
        ok = ok || contains(outsideBounds, PointF(0, destinationArea->bottom));
        ok = ok || contains(outsideBounds, PointF(destinationArea->right, 0));
        if(!ok)
            return;
    }

    if(!primitive->mapObject->innerPolygonsPoints31.isEmpty())
    {
        path.setFillType(SkPath::kEvenOdd_FillType);
        for(auto itPolygon = primitive->mapObject->innerPolygonsPoints31.cbegin(); itPolygon != primitive->mapObject->innerPolygonsPoints31.cend(); ++itPolygon)
        {
            const auto& polygon = *itPolygon;

            pointIdx = 0;
            for(auto itVertex = polygon.cbegin(); itVertex != polygon.cend(); ++itVertex, pointIdx++)
            {
                const auto& point = *itVertex;
                calculateVertex(point, vertex);

                if(pointIdx == 0)
                {
                    path.moveTo(vertex.x, vertex.y);
                }
                else
                {
                    path.lineTo(vertex.x, vertex.y);
                }
            }
        }
    }

    canvas.drawPath(path, _mapPaint);
    if(updatePaint(*primitive->evaluationResult, PaintValuesSet::Set_1, false))
        canvas.drawPath(path, _mapPaint);
}

void OsmAnd::Rasterizer_P::rasterizePolyline(
    const AreaI* const destinationArea,
    SkCanvas& canvas, const std::shared_ptr<const Primitive>& primitive, bool drawOnlyShadow)
{
    assert(primitive->mapObject->points31.size() >= 2);

    if(!updatePaint(*primitive->evaluationResult, PaintValuesSet::Set_0, false))
        return;

    bool ok;

    int shadowColor;
    ok = primitive->evaluationResult->getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_SHADOW_COLOR, shadowColor);
    if(!ok || shadowColor == 0)
        shadowColor = context._shadowRenderingColor;

    int shadowRadius;
    ok = primitive->evaluationResult->getIntegerValue(env.styleBuiltinValueDefs->id_OUTPUT_SHADOW_RADIUS, shadowRadius);
    if(drawOnlyShadow && (!ok || shadowRadius == 0))
        return;

    const auto typeRuleId = primitive->mapObject->_typesRuleIds[primitive->typeRuleIdIndex];

    int oneway = 0;
    if(context._zoom >= ZoomLevel16 && typeRuleId == primitive->mapObject->section->encodingDecodingRules->highway_encodingRuleId)
    {
        if(primitive->mapObject->containsType(primitive->mapObject->section->encodingDecodingRules->oneway_encodingRuleId, true))
            oneway = 1;
        else if(primitive->mapObject->containsType(primitive->mapObject->section->encodingDecodingRules->onewayReverse_encodingRuleId, true))
            oneway = -1;
    }

    SkPath path;
    int pointIdx = 0;
    bool intersect = false;
    int prevCross = 0;
    PointF vertex, middleVertex;
    const auto pointsCount = primitive->mapObject->points31.size();
    const auto middleIdx = pointsCount / 2;
    auto pPoint = primitive->mapObject->points31.constData();
    for(pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        calculateVertex(*pPoint, vertex);

        if(pointIdx == 0)
        {
            path.moveTo(vertex.x, vertex.y);
        }
        else
        {
            if(pointIdx == middleIdx)
                middleVertex = vertex;

            path.lineTo(vertex.x, vertex.y);
        }

        if(destinationArea && !intersect)
        {
            if(destinationArea->contains(vertex))
            {
                intersect = true;
            }
            else
            {
                int cross = 0;
                cross |= (vertex.x < destinationArea->left ? 1 : 0);
                cross |= (vertex.x > destinationArea->right ? 2 : 0);
                cross |= (vertex.y < destinationArea->top ? 4 : 0);
                cross |= (vertex.y > destinationArea->bottom ? 8 : 0);
                if(pointIdx > 0)
                {
                    if((prevCross & cross) == 0)
                    {
                        intersect = true;
                    }
                }
                prevCross = cross;
            }
        }
    }

    if (destinationArea && !intersect)
        return;

    if (pointIdx > 0)
    {
        if (drawOnlyShadow)
        {
            rasterizeLineShadow(canvas, path, shadowColor, shadowRadius);
        }
        else
        {
            if(updatePaint(*primitive->evaluationResult, PaintValuesSet::Set_minus2, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            if(updatePaint(*primitive->evaluationResult, PaintValuesSet::Set_minus1, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            if(updatePaint(*primitive->evaluationResult, PaintValuesSet::Set_0, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            canvas.drawPath(path, _mapPaint);
            if(updatePaint(*primitive->evaluationResult, PaintValuesSet::Set_1, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            if(updatePaint(*primitive->evaluationResult, PaintValuesSet::Set_3, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            if (oneway && !drawOnlyShadow)
            {
                rasterizeLine_OneWay(canvas, path, oneway);
            }
        }
    }
}

void OsmAnd::Rasterizer_P::rasterizeLineShadow(
    SkCanvas& canvas, const SkPath& path, uint32_t shadowColor, int shadowRadius )
{
    // blurred shadows
    if (context._shadowRenderingMode == 2 && shadowRadius > 0)
    {
        // simply draw shadow? difference from option 3 ?
        // paint->setColor(0xffffffff);
        _mapPaint.setLooper(new SkBlurDrawLooper(shadowRadius, 0, 0, shadowColor))->unref();
        canvas.drawPath(path, _mapPaint);
    }

    // option shadow = 3 with solid border
    if (context._shadowRenderingMode == 3 && shadowRadius > 0)
    {
        _mapPaint.setLooper(nullptr);
        _mapPaint.setStrokeWidth(_mapPaint.getStrokeWidth() + shadowRadius*2);
        //		paint->setColor(0xffbababa);
        _mapPaint.setColorFilter(SkColorFilter::CreateModeFilter(shadowColor, SkXfermode::kSrcIn_Mode))->unref();
        //		paint->setColor(shadowColor);
        canvas.drawPath(path, _mapPaint);
    }
}

void OsmAnd::Rasterizer_P::rasterizeLine_OneWay(
    SkCanvas& canvas, const SkPath& path, int oneway )
{
    if (oneway > 0)
    {
        for(auto itPaint = env.oneWayPaints.cbegin(); itPaint != env.oneWayPaints.cend(); ++itPaint)
        {
            canvas.drawPath(path, *itPaint);
        }
    }
    else
    {
        for(auto itPaint = env.reverseOneWayPaints.cbegin(); itPaint != env.reverseOneWayPaints.cend(); ++itPaint)
        {
            canvas.drawPath(path, *itPaint);
        }
    }
}

void OsmAnd::Rasterizer_P::calculateVertex( const PointI& point31, PointF& vertex )
{
    vertex.x = static_cast<float>(point31.x - context._area31.left) / _31toPixelDivisor.x;
    vertex.y = static_cast<float>(point31.y - context._area31.top) / _31toPixelDivisor.y;
}

bool OsmAnd::Rasterizer_P::contains( const QVector< PointF >& vertices, const PointF& other )
{
    uint32_t intersections = 0;

    auto itPrevVertex = vertices.cbegin();
    auto itVertex = itPrevVertex + 1;
    for(; itVertex != vertices.cend(); itPrevVertex = itVertex, ++itVertex)
    {
        const auto& vertex0 = *itPrevVertex;
        const auto& vertex1 = *itVertex;

        if(Utilities::rayIntersect(vertex0, vertex1, other))
            intersections++;
    }

    // special handling, also count first and last, might not be closed, but
    // we want this!
    const auto& vertex0 = vertices.first();
    const auto& vertex1 = vertices.last();
    if(Utilities::rayIntersect(vertex0, vertex1, other))
        intersections++;

    return intersections % 2 == 1;
}

bool OsmAnd::Rasterizer_P::polygonizeCoastlines(
    const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
    const QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& coastlines,
    QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& outVectorized,
    bool abortIfBrokenCoastlinesExist,
    bool includeBrokenCoastlines )
{
    QList< QVector< PointI > > closedPolygons;
    QList< QVector< PointI > > coastlinePolylines; // Broken == not closed in this case

    // Align area to 32: this fixes coastlines and specifically Antarctica
    auto alignedArea31 = context._area31;
    alignedArea31.top &= ~((1u << 5) - 1);
    alignedArea31.left &= ~((1u << 5) - 1);
    alignedArea31.bottom &= ~((1u << 5) - 1);
    alignedArea31.right &= ~((1u << 5) - 1);

    uint64_t osmId = 0;
    QVector< PointI > linePoints31;
    for(auto itCoastline = coastlines.cbegin(); itCoastline != coastlines.cend(); ++itCoastline)
    {
        const auto& coastline = *itCoastline;

        if(coastline->_points31.size() < 2)
        {
            OsmAnd::LogPrintf(LogSeverityLevel::Warning,
                "Map object #%" PRIu64 " (%" PRIi64 ") is polygonized as coastline, but has %d vertices",
                coastline->id >> 1, static_cast<int64_t>(coastline->id) / 2,
                coastline->_points31.size());
            continue;
        }

        osmId = coastline->id >> 1;
        linePoints31.clear();
        auto itPoint = coastline->_points31.cbegin();
        auto pp = *itPoint;
        auto cp = pp;
        auto prevInside = alignedArea31.contains(cp);
        if(prevInside)
            linePoints31.push_back(cp);
        for(++itPoint; itPoint != coastline->_points31.cend(); ++itPoint)
        {
            cp = *itPoint;

            const auto inside = alignedArea31.contains(cp);
            const auto lineEnded = buildCoastlinePolygonSegment(env, context, inside, cp, prevInside, pp, linePoints31);
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
    for(auto itPolyline = coastlinePolylines.cbegin(); itPolyline != coastlinePolylines.cend(); ++itPolyline)
    {
        const auto& polyline = *itPolyline;

        const std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_isArea = false;
        mapObject->_points31 = polyline;
        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastlineLine_encodingRuleId);

        outVectorized.push_back(qMove(mapObject));
    }

    const bool coastlineCrossesBounds = !coastlinePolylines.isEmpty();
    if(!coastlinePolylines.isEmpty())
    {
        // Add complete water tile with holes
        const std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_points31.push_back(qMove(PointI(context._area31.left, context._area31.top)));
        mapObject->_points31.push_back(qMove(PointI(context._area31.right, context._area31.top)));
        mapObject->_points31.push_back(qMove(PointI(context._area31.right, context._area31.bottom)));
        mapObject->_points31.push_back(qMove(PointI(context._area31.left, context._area31.bottom)));
        mapObject->_points31.push_back(mapObject->_points31.first());
        convertCoastlinePolylinesToPolygons(env, context, coastlinePolylines, mapObject->_innerPolygonsPoints31, osmId);

        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
        mapObject->_id = osmId;
        mapObject->_isArea = true;

        assert(mapObject->isClosedFigure());
        assert(mapObject->isClosedFigure(true));
        outVectorized.push_back(qMove(mapObject));
    }

    if(!coastlinePolylines.isEmpty())
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Invalid polylines found during polygonization of coastlines in area [%d, %d, %d, %d]@%d",
            context._area31.top,
            context._area31.left,
            context._area31.bottom,
            context._area31.right,
            context._zoom);
    }

    if (includeBrokenCoastlines)
    {
        for(auto itPolygon = coastlinePolylines.cbegin(); itPolygon != coastlinePolylines.cend(); ++itPolygon)
        {
            const auto& polygon = *itPolygon;

            const std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
            mapObject->_isArea = false;
            mapObject->_points31 = polygon;
            mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastlineBroken_encodingRuleId);

            outVectorized.push_back(qMove(mapObject));
        }
    }

    // Draw coastlines
    for(auto itPolygon = closedPolygons.cbegin(); itPolygon != closedPolygons.cend(); ++itPolygon)
    {
        const auto& polygon = *itPolygon;

        const std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_isArea = false;
        mapObject->_points31 = polygon;
        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastlineLine_encodingRuleId);

        outVectorized.push_back(qMove(mapObject));
    }

    if (abortIfBrokenCoastlinesExist && !coastlinePolylines.isEmpty())
        return false;

    auto fullWaterObjects = 0u;
    auto fullLandObjects = 0u;
    for(auto itPolygon = closedPolygons.cbegin(); itPolygon != closedPolygons.cend(); ++itPolygon)
    {
        const auto& polygon = *itPolygon;

        // If polygon has less than 4 points, it's invalid
        if(polygon.size() < 4)
            continue;

        bool clockwise = isClockwiseCoastlinePolygon(polygon);

        const std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_points31 = qMove(polygon);
        if(clockwise)
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

    if(fullWaterObjects == 0u && !coastlineCrossesBounds)
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Isolated islands found during polygonization of coastlines in area [%d, %d, %d, %d]@%d",
            context._area31.top,
            context._area31.left,
            context._area31.bottom,
            context._area31.right,
            context._zoom);

        // Add complete water tile
        const std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_points31.push_back(qMove(PointI(context._area31.left, context._area31.top)));
        mapObject->_points31.push_back(qMove(PointI(context._area31.right, context._area31.top)));
        mapObject->_points31.push_back(qMove(PointI(context._area31.right, context._area31.bottom)));
        mapObject->_points31.push_back(qMove(PointI(context._area31.left, context._area31.bottom)));
        mapObject->_points31.push_back(mapObject->_points31.first());

        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
        mapObject->_id = osmId;
        mapObject->_isArea = true;

        assert(mapObject->isClosedFigure());
        outVectorized.push_back(qMove(mapObject));
    }

    return true;
}

bool OsmAnd::Rasterizer_P::buildCoastlinePolygonSegment(
    const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
    bool currentInside,
    const PointI& currentPoint31,
    bool prevInside,
    const PointI& previousPoint31,
    QVector< PointI >& segmentPoints )
{
    bool lineEnded = false;

    // Align area to 32: this fixes coastlines and specifically Antarctica
    auto alignedArea31 = context._area31;
    alignedArea31.top &= ~((1u << 5) - 1);
    alignedArea31.left &= ~((1u << 5) - 1);
    alignedArea31.bottom &= ~((1u << 5) - 1);
    alignedArea31.right &= ~((1u << 5) - 1);

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

// Calculates intersection between line and bbox in clockwise manner.
bool OsmAnd::Rasterizer_P::calculateIntersection( const PointI& p1, const PointI& p0, const AreaI& bbox, PointI& pX )
{
    // Well, since Victor said not to touch this thing, I will replace only output writing,
    // and will even retain old variable names.
    const auto& px = p0.x;
    const auto& py = p0.y;
    const auto& x = p1.x;
    const auto& y = p1.y;
    const auto& leftX = bbox.left;
    const auto& rightX = bbox.right;
    const auto& topY = bbox.top;
    const auto& bottomY = bbox.bottom;

    // firstly try to search if the line goes in
    if (py < topY && y >= topY) {
        int tx = (int) (px + ((double) (x - px) * (topY - py)) / (y - py));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;//b.first = tx;
            pX.y = topY;//b.second = topY;
            return true;
        }
    }
    if (py > bottomY && y <= bottomY) {
        int tx = (int) (px + ((double) (x - px) * (py - bottomY)) / (py - y));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;//b.first = tx;
            pX.y = bottomY;//b.second = bottomY;
            return true;
        }
    }
    if (px < leftX && x >= leftX) {
        int ty = (int) (py + ((double) (y - py) * (leftX - px)) / (x - px));
        if (ty >= topY && ty <= bottomY) {
            pX.x = leftX;//b.first = leftX;
            pX.y = ty;//b.second = ty;
            return true;
        }

    }
    if (px > rightX && x <= rightX) {
        int ty = (int) (py + ((double) (y - py) * (px - rightX)) / (px - x));
        if (ty >= topY && ty <= bottomY) {
            pX.x = rightX;//b.first = rightX;
            pX.y = ty;//b.second = ty;
            return true;
        }

    }

    // try to search if point goes out
    if (py > topY && y <= topY) {
        int tx = (int) (px + ((double) (x - px) * (topY - py)) / (y - py));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;//b.first = tx;
            pX.y = topY;//b.second = topY;
            return true;
        }
    }
    if (py < bottomY && y >= bottomY) {
        int tx = (int) (px + ((double) (x - px) * (py - bottomY)) / (py - y));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;//b.first = tx;
            pX.y = bottomY;//b.second = bottomY;
            return true;
        }
    }
    if (px > leftX && x <= leftX) {
        int ty = (int) (py + ((double) (y - py) * (leftX - px)) / (x - px));
        if (ty >= topY && ty <= bottomY) {
            pX.x = leftX;//b.first = leftX;
            pX.y = ty;//b.second = ty;
            return true;
        }

    }
    if (px < rightX && x >= rightX) {
        int ty = (int) (py + ((double) (y - py) * (px - rightX)) / (px - x));
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

void OsmAnd::Rasterizer_P::appendCoastlinePolygons( QList< QVector< PointI > >& closedPolygons, QList< QVector< PointI > >& coastlinePolylines, QVector< PointI >& polyline )
{
    if(polyline.isEmpty())
        return;

    if(polyline.first() == polyline.last())
    {
        closedPolygons.push_back(polyline);
        return;
    }

    bool add = true;

    for(auto itPolygon = coastlinePolylines.begin(); itPolygon != coastlinePolylines.end();)
    {
        auto& polygon = *itPolygon;

        bool remove = false;

        if(polyline.first() == polygon.last())
        {
            polygon.reserve(polygon.size() + polyline.size() - 1);
            polygon.pop_back();
            polygon += polyline;
            remove = true;
            polyline = polygon;
        }
        else if(polyline.last() == polygon.first())
        {
            polyline.reserve(polyline.size() + polygon.size() - 1);
            polyline.pop_back();
            polyline += polygon;
            remove = true;
        }

        if (remove)
        {
            itPolygon = coastlinePolylines.erase(itPolygon);
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

void OsmAnd::Rasterizer_P::convertCoastlinePolylinesToPolygons(
    const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
    QList< QVector< PointI > >& coastlinePolylines, QList< QVector< PointI > >& coastlinePolygons, uint64_t osmId )
{
    // Align area to 32: this fixes coastlines and specifically Antarctica
    auto alignedArea31 = context._area31;
    alignedArea31.top &= ~((1u << 5) - 1);
    alignedArea31.left &= ~((1u << 5) - 1);
    alignedArea31.bottom &= ~((1u << 5) - 1);
    alignedArea31.right &= ~((1u << 5) - 1);

    QList< QVector< PointI > > validPolylines;

    // Check if polylines has been cut by rasterization viewport
    QMutableListIterator< QVector< PointI > > itPolyline(coastlinePolylines);
    while(itPolyline.hasNext())
    {
        const auto& polyline = itPolyline.next();
        assert(!polyline.isEmpty());

        const auto& head = polyline.first();
        const auto& tail = polyline.last();

        // This curve has not been cut by rasterization viewport, so it's
        // impossible to fix it
        if (!alignedArea31.isOnEdge(head) || !alignedArea31.isOnEdge(tail))
            continue;

        validPolylines.push_back(polyline);
        itPolyline.remove();
    }

    std::set< QList< QVector< PointI > >::iterator > processedPolylines;
    while(processedPolylines.size() != validPolylines.size())
    {
        for(auto itPolyline = validPolylines.begin(); itPolyline != validPolylines.end(); ++itPolyline)
        {
            // If this polyline was already processed, skip it
            if(processedPolylines.find(itPolyline) != processedPolylines.end())
                continue;

            // Start from tail of the polyline and search for it's continuation in CCV order
            auto& polyline = *itPolyline;
            const auto& tail = polyline.last();
            auto tailEdge = AreaI::Edge::Invalid;
            alignedArea31.isOnEdge(tail, &tailEdge);
            auto itNearestPolyline = validPolylines.end();
            auto firstIteration = true;
            for(int idx = static_cast<int>(tailEdge) + 4; (idx >= static_cast<int>(tailEdge)) && (itNearestPolyline == validPolylines.end()); idx--, firstIteration = false)
            {
                const auto currentEdge = static_cast<AreaI::Edge>(idx % 4);

                for(auto itOtherPolyline = validPolylines.begin(); itOtherPolyline != validPolylines.end(); ++itOtherPolyline)
                {
                    // If this polyline was already processed, skip it
                    if(processedPolylines.find(itOtherPolyline) != processedPolylines.end())
                        continue;

                    // Skip polylines that are on other edges
                    const auto& otherHead = itOtherPolyline->first();
                    auto otherHeadEdge = AreaI::Edge::Invalid;
                    alignedArea31.isOnEdge(otherHead, &otherHeadEdge);
                    if(otherHeadEdge != currentEdge)
                        continue;

                    // Skip polyline that is not next in CCV order
                    if(firstIteration)
                    {
                        bool isNextByCCV = false;
                        if(currentEdge == AreaI::Edge::Top)
                            isNextByCCV = (otherHead.x <= tail.x);
                        else if(currentEdge == AreaI::Edge::Right)
                            isNextByCCV = (otherHead.y <= tail.y);
                        else if(currentEdge == AreaI::Edge::Bottom)
                            isNextByCCV = (tail.x <= otherHead.x);
                        else if(currentEdge == AreaI::Edge::Left)
                            isNextByCCV = (tail.y <= otherHead.y);
                        if(!isNextByCCV)
                            continue;
                    }

                    // If nearest was not yet set, set this
                    if(itNearestPolyline == validPolylines.cend())
                    {
                        itNearestPolyline = itOtherPolyline;
                        continue;
                    }

                    // Check if current polyline's head is closer (by CCV) that previously selected
                    const auto& previouslySelectedHead = itNearestPolyline->first();
                    bool isCloserByCCV = false;
                    if(currentEdge == AreaI::Edge::Top)
                        isCloserByCCV = (otherHead.x > previouslySelectedHead.x);
                    else if(currentEdge == AreaI::Edge::Right)
                        isCloserByCCV = (otherHead.y > previouslySelectedHead.y);
                    else if(currentEdge == AreaI::Edge::Bottom)
                        isCloserByCCV = (otherHead.x < previouslySelectedHead.x);
                    else if(currentEdge == AreaI::Edge::Left)
                        isCloserByCCV = (otherHead.y < previouslySelectedHead.y);

                    // If closer-by-CCV, then select this
                    if(isCloserByCCV)
                        itNearestPolyline = itOtherPolyline;
                }
            }
            assert(itNearestPolyline != validPolylines.cend());

            // Get edge of nearest-by-CCV head
            auto nearestHeadEdge = AreaI::Edge::Invalid;
            const auto& nearestHead = itNearestPolyline->first();
            alignedArea31.isOnEdge(nearestHead, &nearestHeadEdge);

            // Fill by edges of area, if required
            int loopShift = 0;
            if( static_cast<int>(tailEdge) - static_cast<int>(nearestHeadEdge) < 0 )
                loopShift = 4;
            else if(tailEdge == nearestHeadEdge)
            {
                bool skipAddingSides = false;
                if(tailEdge == AreaI::Edge::Top)
                    skipAddingSides = (tail.x >= nearestHead.x);
                else if(tailEdge == AreaI::Edge::Right)
                    skipAddingSides = (tail.y >= nearestHead.y);
                else if(tailEdge == AreaI::Edge::Bottom)
                    skipAddingSides = (tail.x <= nearestHead.x);
                else if(tailEdge == AreaI::Edge::Left)
                    skipAddingSides = (tail.y <= nearestHead.y);

                if(!skipAddingSides)
                    loopShift = 4;
            }
            for(int idx = static_cast<int>(tailEdge) + loopShift; idx > static_cast<int>(nearestHeadEdge); idx--)
            {
                const auto side = static_cast<AreaI::Edge>(idx % 4);
                PointI p;

                if(side == AreaI::Edge::Top)
                {
                    p.y = alignedArea31.top;
                    p.x = alignedArea31.left;
                }
                else if(side == AreaI::Edge::Right)
                {
                    p.y = alignedArea31.top;
                    p.x = alignedArea31.right;
                }
                else if(side == AreaI::Edge::Bottom)
                {
                    p.y = alignedArea31.bottom;
                    p.x = alignedArea31.right;
                }
                else if(side == AreaI::Edge::Left)
                {
                    p.y = alignedArea31.bottom;
                    p.x = alignedArea31.left;
                }

                polyline.push_back(qMove(p));
            }

            // If nearest-by-CCV is head of current polyline, cap it and add to polygons, ...
            if(itNearestPolyline == itPolyline)
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
            processedPolylines.insert(itNearestPolyline);
        }
    }
}

bool OsmAnd::Rasterizer_P::isClockwiseCoastlinePolygon( const QVector< PointI > & polygon )
{
    if(polygon.isEmpty())
        return true;

    // calculate middle Y
    int64_t middleY = 0;
    for(auto itVertex = polygon.cbegin(); itVertex != polygon.cend(); ++itVertex)
        middleY += itVertex->y;
    middleY /= polygon.size();

    double clockwiseSum = 0;

    bool firstDirectionUp = false;
    int previousX = INT_MIN;
    int firstX = INT_MIN;

    auto itPrevVertex = polygon.cbegin();
    auto itVertex = itPrevVertex + 1;
    for(; itVertex != polygon.cend(); itPrevVertex = itVertex, ++itVertex)
    {
        const auto& vertex0 = *itPrevVertex;
        const auto& vertex1 = *itVertex;

        int32_t rX;
        if(!Utilities::rayIntersectX(vertex0, vertex1, middleY, rX))
            continue;

        bool skipSameSide = (vertex1.y <= middleY) == (vertex0.y <= middleY);
        if (skipSameSide)
            continue;

        bool directionUp = vertex0.y >= middleY;
        if (firstX == INT_MIN) {
            firstDirectionUp = directionUp;
            firstX = rX;
        } else {
            bool clockwise = (!directionUp) == (previousX < rX);
            if (clockwise) {
                clockwiseSum += abs(previousX - rX);
            } else {
                clockwiseSum -= abs(previousX - rX);
            }
        }
        previousX = rX;
    }

    if (firstX != INT_MIN) {
        bool clockwise = (!firstDirectionUp) == (previousX < firstX);
        if (clockwise) {
            clockwiseSum += qAbs(previousX - firstX);
        } else {
            clockwiseSum -= qAbs(previousX - firstX);
        }
    }

    return clockwiseSum >= 0;
}

//////////////////////////////////////////////////////////////////////////
//#include <SkImageEncoder.h>
//////////////////////////////////////////////////////////////////////////

void OsmAnd::Rasterizer_P::rasterizeSymbolsWithoutPaths(
    QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
    std::function<bool(const std::shared_ptr<const Model::MapObject>& mapObject)> filter,
    const IQueryController* const controller )
{
    for(auto itSymbolsEntry = context._symbols.cbegin(); itSymbolsEntry != context._symbols.cend(); ++itSymbolsEntry)
    {
        if(controller && controller->isAborted())
            return;

        // Apply filter, if it's present
        if(filter && !filter(itSymbolsEntry->first))
            continue;

        // Create group
        const auto constructedGroup = new RasterizedSymbolsGroup(itSymbolsEntry->first);
        std::shared_ptr<const RasterizedSymbolsGroup> group(constructedGroup);

        for(auto itPrimitiveSymbol = itSymbolsEntry->second.cbegin(); itPrimitiveSymbol != itSymbolsEntry->second.cend(); ++itPrimitiveSymbol)
        {
            if(controller && controller->isAborted())
                return;

            const auto& symbol = *itPrimitiveSymbol;

            if(const auto textSymbol = std::dynamic_pointer_cast<const PrimitiveSymbol_Text>(symbol))
            {
                // Skip symbols that need to be rasterized along path
                if(textSymbol->drawOnPath)
                    continue;

                // Configure paint for text
                SkPaint textPaint = env.textPaint;

                textPaint.setTextSize(textSymbol->size);
                textPaint.setFakeBoldText(textSymbol->isBold);
                textPaint.setColor(textSymbol->color);

                // Measure text
                SkRect textBounds;
                textPaint.measureText(textSymbol->value.constData(), textSymbol->value.length()*sizeof(QChar), &textBounds);

                SkRect textBBox = textBounds;

                // Process shadow
                SkPaint textShadowPaint;
                SkRect shadowBounds;

                if(textSymbol->shadowRadius > 0)
                {
                    textShadowPaint = textPaint;

                    textShadowPaint.setStyle(SkPaint::kStroke_Style);
                    textShadowPaint.setColor(SK_ColorWHITE);
                    textShadowPaint.setStrokeWidth(textSymbol->shadowRadius);

                    textShadowPaint.measureText(textSymbol->value.constData(), textSymbol->value.length()*sizeof(QChar), &shadowBounds);
                    textBBox.join(shadowBounds);
                }

                // Create a bitmap that will be hold text
                auto bitmap = new SkBitmap();
                bitmap->setConfig(SkBitmap::kARGB_8888_Config, textBBox.width(), textBBox.height());
                bitmap->allocPixels();
                SkBitmapDevice target(*bitmap);
                SkCanvas canvas(&target);

                // Rasterize text
                if(textSymbol->shadowRadius > 0)
                    canvas.drawText(textSymbol->value.constData(), textSymbol->value.length()*sizeof(QChar), -textBBox.left(), -textBBox.top(), textShadowPaint);
                canvas.drawText(textSymbol->value.constData(), textSymbol->value.length()*sizeof(QChar), -textBBox.left(), -textBBox.top(), textPaint);

                //////////////////////////////////////////////////////////////////////////
                /*std::unique_ptr<SkImageEncoder> encoder(CreatePNGImageEncoder());
                QString path;
                path.sprintf("D:\\texts\\%p.png", bitmap);
                encoder->encodeFile(path.toLocal8Bit(), *bitmap, 100);*/
                //////////////////////////////////////////////////////////////////////////

                // Create RasterizedSymbol
                const auto rasterizedSymbol = new RasterizedSymbol(
                    group,
                    constructedGroup->mapObject,
                    symbol->location31,
                    symbol->order,
                    qMove(std::shared_ptr<const SkBitmap>(bitmap)));
                assert(static_cast<bool>(rasterizedSymbol->bitmap));
                constructedGroup->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));
            }
        }

        // Add group to output
        outSymbolsGroups.push_back(qMove(group));
    }
}

//void OsmAnd::Rasterizer_P::rasterizeText(
//    const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
//    bool fillBackground, SkCanvas& canvas, const IQueryController* const controller /*= nullptr*/ )
//{
//    if(fillBackground)
//    {
//        SkPaint bgPaint;
//        bgPaint.setColor(SK_ColorTRANSPARENT);
//        bgPaint.setStyle(SkPaint::kFill_Style);
//        canvas.drawRectCoords(context._renderViewport.top, context._renderViewport.left, context._renderViewport.right, context._renderViewport.bottom, bgPaint);
//    }
//    /*
//    SkRect r = SkRect::MakeLTRB(0, 0, rc->getWidth(), rc->getHeight());
//    r.inset(-100, -100);
//    quad_tree<TextDrawInfo*> boundsIntersect(r, 4, 0.6);
//    */
//    /*
//#if defined(ANDROID)
//    //TODO: This is never released because of always +1 of reference counter
//    if(!sDefaultTypeface)
//        sDefaultTypeface = SkTypeface::CreateFromName("Droid Serif", SkTypeface::kNormal);
//#endif
//        */
//    SkPaint::FontMetrics fontMetrics;
//    for(auto itText = context._texts.cbegin(); itText != context._texts.cend(); ++itText)
//    {
//        if(controller && controller->isAborted())
//            break;
//
//        const auto& text = *itText;
//
//        context._textPaint.setTextSize(text.size * context._densityFactor);
//        context._textPaint.setFakeBoldText(text.isBold);//TODO: use special typeface!
//        context._textPaint.setColor(text.color);
//        context._textPaint.getFontMetrics(&fontMetrics);
//        /*textDrawInfo->centerY += (-fm.fAscent);
//
//        // calculate if there is intersection
//        bool intersects = findTextIntersection(cv, rc, boundsIntersect, textDrawInfo, &paintText, &paintIcon);
//        if(intersects)
//            continue;
//
//        if (textDrawInfo->drawOnPath && textDrawInfo->path != NULL) {
//            if (textDrawInfo->textShadow > 0) {
//                paintText.setColor(0xFFFFFFFF);
//                paintText.setStyle(SkPaint::kStroke_Style);
//                paintText.setStrokeWidth(2 + textDrawInfo->textShadow);
//                rc->nativeOperations.Pause();
//                cv->drawTextOnPathHV(textDrawInfo->text.c_str(), textDrawInfo->text.length(), *textDrawInfo->path, textDrawInfo->hOffset,
//                    textDrawInfo->vOffset, paintText);
//                rc->nativeOperations.Start();
//                // reset
//                paintText.setStyle(SkPaint::kFill_Style);
//                paintText.setStrokeWidth(2);
//                paintText.setColor(textDrawInfo->textColor);
//            }
//            rc->nativeOperations.Pause();
//            cv->drawTextOnPathHV(textDrawInfo->text.c_str(), textDrawInfo->text.length(), *textDrawInfo->path, textDrawInfo->hOffset,
//                textDrawInfo->vOffset, paintText);
//            rc->nativeOperations.Start();
//        } else {
//            if (textDrawInfo->shieldRes.length() > 0) {
//                SkBitmap* ico = getCachedBitmap(rc, textDrawInfo->shieldRes);
//                if (ico != NULL) {
//                    float left = textDrawInfo->centerX - rc->getDensityValue(ico->width() / 2) - 0.5f;
//                    float top = textDrawInfo->centerY - rc->getDensityValue(ico->height() / 2)
//                        - rc->getDensityValue(4.5f);
//                    SkRect r = SkRect::MakeXYWH(left, top, rc->getDensityValue(ico->width()),
//                        rc->getDensityValue(ico->height()));
//                    PROFILE_NATIVE_OPERATION(rc, cv->drawBitmapRect(*ico, (SkIRect*) NULL, r, &paintIcon));
//                }
//            }
//            drawWrappedText(rc, cv, textDrawInfo, textSize, paintText);
//        }*/
//    }
//}

//void drawWrappedText(RenderingContext* rc, SkCanvas* cv, TextDrawInfo* text, float textSize, SkPaint& paintText) {
//    if(text->textWrap == 0) {
//        // set maximum for all text
//        text->textWrap = 40;
//    }
//
//    if(text->text.length() > text->textWrap) {
//        const char* c_str = text->text.c_str();
//
//        int end = text->text.length();
//        int line = 0;
//        int pos = 0;
//        int start = 0;
//        while(start < end) {
//            const char* p_str = c_str;
//            int lastSpace = -1;
//            int prevPos = -1;
//            int charRead = 0;
//            do {
//                int lastSpace = nextWord((uint8_t*)p_str, &charRead);
//                if (lastSpace == -1) {
//                    pos = end;
//                } else {
//                    p_str += lastSpace;
//                    if(pos != start && charRead >= text->textWrap){
//                        break;
//                    }
//                    pos += lastSpace;
//                }
//            } while(pos < end && charRead < text->textWrap);
//
//            PROFILE_NATIVE_OPERATION(rc, drawTextOnCanvas(cv, c_str, pos - start , text->centerX, text->centerY + line * (textSize + 2), paintText, text->textShadow));
//            c_str += (pos - start);
//            start = pos;
//            line++;
//        }
//    } else {
//        PROFILE_NATIVE_OPERATION(rc, drawTextOnCanvas(cv, text->text.data(), text->text.length(), text->centerX, text->centerY, paintText, text->textShadow));
//    }
//}
//
//void drawTextOnCanvas(SkCanvas* cv, const char* text, uint16_t len, float centerX, float centerY, SkPaint& paintText,
//                      float textShadow)
//{
//    if (textShadow > 0) {
//        int c = paintText.getColor();
//        paintText.setStyle(SkPaint::kStroke_Style);
//        paintText.setColor(-1); // white
//        paintText.setStrokeWidth(2 + textShadow);
//        cv->drawText(text, len, centerX, centerY, paintText);
//        
//            paintText.setStrokeWidth(2);
//        paintText.setStyle(SkPaint::kFill_Style);
//        paintText.setColor(c);
//    }
//    cv->drawText(text, len, centerX, centerY, paintText);
//}

OsmAnd::Rasterizer_P::PrimitiveSymbol::PrimitiveSymbol()
    : order(-1)
{
}
