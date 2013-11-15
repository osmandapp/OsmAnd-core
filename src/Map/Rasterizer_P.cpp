#include "Rasterizer_P.h"
#include "Rasterizer.h"
#include "Rasterizer_Metrics.h"

#include <cassert>
#include <cinttypes>
#include <set>
#include <chrono>

#include "RasterizerEnvironment.h"
#include "RasterizerEnvironment_P.h"
#include "RasterizerContext.h"
#include "RasterizerContext_P.h"
#include "RasterizedSymbol.h"
#include "MapStyleEvaluator.h"
#include "MapStyleEvaluatorState.h"
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

//////////////////////////////////////////////////////////////////////////
//#include <SkImageEncoder.h>
//////////////////////////////////////////////////////////////////////////

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
                context._basemapCoastlineObjects.push_back(mapObject);
            else
                context._coastlineObjects.push_back(mapObject);
        }
        else
        {
            if(mapObject->section->isBasemap)
                context._basemapMapObjects.push_back(mapObject);
            else
                context._mapObjects.push_back(mapObject);
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
    const bool detailedLandData = zoom >= DetailedLandDataZoom && !context._mapObjects.isEmpty();
    if(!context._coastlineObjects.empty())
    {
        const bool coastlinesWereAdded = polygonizeCoastlines(env, context,
            context._coastlineObjects,
            context._triangulatedCoastlineObjects,
            !context._basemapCoastlineObjects.isEmpty(),
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
            context._basemapCoastlineObjects,
            context._triangulatedCoastlineObjects,
            false,
            true);
        fillEntireArea = !coastlinesWereAdded && fillEntireArea;
    }

    // Update metric
    if(metric)
    {
        const std::chrono::duration<float> polygonizeCoastlines_elapsed = std::chrono::high_resolution_clock::now() - polygonizeCoastlines_begin;
        metric->elapsedTimeForPolygonizingCoastlines += polygonizeCoastlines_elapsed.count();
        metric->polygonizedCoastlines = context._triangulatedCoastlineObjects.size();
    }

    if(context._basemapMapObjects.isEmpty() && context._mapObjects.isEmpty() && foundation == MapFoundationType::Undefined)
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

        std::shared_ptr<Model::MapObject> bgMapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        bgMapObject->_isArea = true;
        bgMapObject->_points31.push_back(PointI(area31.left, area31.top));
        bgMapObject->_points31.push_back(PointI(area31.right, area31.top));
        bgMapObject->_points31.push_back(PointI(area31.right, area31.bottom));
        bgMapObject->_points31.push_back(PointI(area31.left, area31.bottom));
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
        context._triangulatedCoastlineObjects.push_back(bgMapObject);
    }

    // Obtain primitives
    const bool detailedDataMissing = zoom > BasemapZoom && context._mapObjects.isEmpty() && context._coastlineObjects.isEmpty();

    // Update metric
    std::chrono::high_resolution_clock::time_point combineObjects_begin;
    if(metric)
        combineObjects_begin = std::chrono::high_resolution_clock::now();

    context._combinedMapObjects << context._mapObjects;
    if(zoom <= BasemapZoom || detailedDataMissing)
        context._combinedMapObjects << context._basemapMapObjects;
    context._combinedMapObjects << context._triangulatedCoastlineObjects;

    // Update metric
    if(metric)
    {
        const std::chrono::duration<float> combineObjects_elapsed = std::chrono::high_resolution_clock::now() - combineObjects_begin;
        metric->elapsedTimeForCombiningObjects += combineObjects_elapsed.count();
    }

    if(context._combinedMapObjects.isEmpty())
    {
        // We simply have no data to render. Report, clean-up and exit
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
    obtainPrimitives(env, context, controller, metric);
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

    // After obtaining primitives, map objects are no longer needed
    context.cleanupMapObjects();

    // Update metric
    std::chrono::high_resolution_clock::time_point obtainPrimitivesSymbols_begin;
    if(metric)
        obtainPrimitivesSymbols_begin = std::chrono::high_resolution_clock::now();

    // Obtain text from primitives
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
    const IQueryController* const controller,
    Rasterizer_Metrics::Metric_prepareContext* const metric)
{
    std::shared_ptr<MapStyleEvaluatorState> sharedEvaluatorState(new MapStyleEvaluatorState());

    bool ok;
    QVector< Primitive > unfilteredLines;
    for(auto itMapObject = context._combinedMapObjects.cbegin(); itMapObject != context._combinedMapObjects.cend(); ++itMapObject)
    {
        if(controller && controller->isAborted())
            return;

        const auto& mapObject = *itMapObject;

        uint32_t typeRuleIdIndex = 0;
        for(auto itTypeRuleId = mapObject->typesRuleIds.cbegin(); itTypeRuleId != mapObject->typesRuleIds.cend(); ++itTypeRuleId, typeRuleIdIndex++)
        {
            const auto& decodedType = mapObject->section->encodingDecodingRules->decodingRules[*itTypeRuleId];
            const auto layer = mapObject->getSimpleLayerValue();

            // Update metric
            std::chrono::high_resolution_clock::time_point orderEvaluation_begin;
            if(metric)
                orderEvaluation_begin = std::chrono::high_resolution_clock::now();
            
            sharedEvaluatorState->clear();
            MapStyleEvaluator evaluator(sharedEvaluatorState, env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Order, mapObject);
            env.applyTo(evaluator);
            evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, decodedType.tag);
            evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, decodedType.value);
            evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
            evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
            evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_LAYER, layer);
            evaluator.setBooleanValue(MapStyle::builtinValueDefinitions.INPUT_AREA, mapObject->isArea);
            evaluator.setBooleanValue(MapStyle::builtinValueDefinitions.INPUT_POINT, mapObject->points31.size() == 1);
            evaluator.setBooleanValue(MapStyle::builtinValueDefinitions.INPUT_CYCLE, mapObject->isClosedFigure());
            ok = evaluator.evaluate();

            // Update metric
            if(metric)
            {
                const std::chrono::duration<float> orderEvaluation_elapsed = std::chrono::high_resolution_clock::now() - orderEvaluation_begin;
                metric->elapsedTimeForOrderEvaluation += orderEvaluation_elapsed.count();
            }

            // If evaluation failed, skip
            if(!ok)
                continue;

            int objectType;
            if(!evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_OBJECT_TYPE, objectType))
                continue;
            int zOrder;
            if(!evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_ORDER, zOrder))
                continue;

            Primitive primitive;
            primitive.mapObject = mapObject;
            primitive.objectType = static_cast<PrimitiveType>(objectType);
            primitive.zOrder = zOrder;
            primitive.typeRuleIdIndex = typeRuleIdIndex;

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

                // Evaluate style for this primitive to check if it passes
                std::shared_ptr<MapStyleEvaluatorState> evaluatorState(new MapStyleEvaluatorState());
                MapStyleEvaluator evaluator(evaluatorState, env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Polygon, primitive.mapObject);
                initializePolygonEvaluator(env, context, primitive, evaluator);
                ok = evaluator.evaluate();

                // Update metric
                if(metric)
                {
                    const std::chrono::duration<float> polygonEvaluation_elapsed = std::chrono::high_resolution_clock::now() - polygonEvaluation_begin;
                    metric->elapsedTimeForPolygonEvaluation += polygonEvaluation_elapsed.count();
                }

                // If evaluation failed, skip
                if(!ok)
                    continue;
                primitive.evaluatorState = evaluatorState;

                // Check size of polygon
                auto polygonArea31 = Utilities::polygonArea(mapObject->points31);
                if(polygonArea31 > PolygonAreaCutoffLowerThreshold)
                {
                    primitive.zOrder += 1.0 / polygonArea31;

                    // Accept this primitive
                    context._polygons.push_back(primitive);

                    // Update metric
                    if(metric)
                        metric->polygonPrimitives++;

                    // Duplicate primitive as point
                    auto pointPrimitive = primitive;
                    pointPrimitive.objectType = PrimitiveType::Point;

                    // Update metric
                    std::chrono::high_resolution_clock::time_point pointEvaluation_begin;
                    if(metric)
                        pointEvaluation_begin = std::chrono::high_resolution_clock::now();

                    // Evaluate Point rules
                    std::shared_ptr<MapStyleEvaluatorState> pointEvaluatorState(new MapStyleEvaluatorState());
                    MapStyleEvaluator evaluator(pointEvaluatorState, env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Point, primitive.mapObject);
                    initializePointEvaluator(env, context, pointPrimitive, evaluator);
                    ok = evaluator.evaluate();

                    // Update metric
                    if(metric)
                    {
                        const std::chrono::duration<float> pointEvaluation_elapsed = std::chrono::high_resolution_clock::now() - pointEvaluation_begin;
                        metric->elapsedTimeForPointEvaluation += pointEvaluation_elapsed.count();
                    }
                    
                    // Point evaluation is a bit special, it's success only indicates that point has an icon
                    if(ok)
                    {
                        pointPrimitive.evaluatorState = pointEvaluatorState;
                    }
                    else
                    {
                        pointPrimitive.evaluatorState.reset();
                    }

                    // Accept also point primitive only if typeIndex == 0 and (there is text or icon)
                    if(pointPrimitive.typeRuleIdIndex == 0 && (!pointPrimitive.mapObject->names.isEmpty() || ok))
                    {
                        context._points.push_back(pointPrimitive);

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
                        primitive.mapObject->id >> 1, static_cast<int64_t>(primitive.mapObject->id) / 2,
                        primitive.mapObject->_points31.size());
                    continue;
                }

                // Update metric
                std::chrono::high_resolution_clock::time_point polylineEvaluation_begin;
                if(metric)
                    polylineEvaluation_begin = std::chrono::high_resolution_clock::now();

                // Evaluate style for this primitive to check if it passes
                std::shared_ptr<MapStyleEvaluatorState> evaluatorState(new MapStyleEvaluatorState());
                MapStyleEvaluator evaluator(evaluatorState, env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Polyline, primitive.mapObject);
                initializePolylineEvaluator(env, context, primitive, evaluator);
                ok = evaluator.evaluate();

                // Update metric
                if(metric)
                {
                    const std::chrono::duration<float> polylineEvaluation_elapsed = std::chrono::high_resolution_clock::now() - polylineEvaluation_begin;
                    metric->elapsedTimeForPolylineEvaluation += polylineEvaluation_elapsed.count();
                }

                // If evaluation failed, skip
                if(!ok)
                    continue;
                primitive.evaluatorState = evaluatorState;

                // Accept this primitive
                unfilteredLines.push_back(primitive);

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
                        primitive.mapObject->id >> 1, static_cast<int64_t>(primitive.mapObject->id) / 2);
                    continue;
                }

                // Update metric
                std::chrono::high_resolution_clock::time_point pointEvaluation_begin;
                if(metric)
                    pointEvaluation_begin = std::chrono::high_resolution_clock::now();

                // Evaluate Point rules
                std::shared_ptr<MapStyleEvaluatorState> evaluatorState(new MapStyleEvaluatorState());
                MapStyleEvaluator evaluator(evaluatorState, env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Point, primitive.mapObject);
                initializePointEvaluator(env, context, primitive, evaluator);
                ok = evaluator.evaluate();

                // Update metric
                if(metric)
                {
                    const std::chrono::duration<float> pointEvaluation_elapsed = std::chrono::high_resolution_clock::now() - pointEvaluation_begin;
                    metric->elapsedTimeForPointEvaluation += pointEvaluation_elapsed.count();
                }

                // Point evaluation is a bit special, it's success only indicates that point has an icon
                if(ok)
                    primitive.evaluatorState = evaluatorState;

                // Skip is possible if typeIndex != 0 or (there is no text and no icon)
                if(primitive.typeRuleIdIndex != 0 || (primitive.mapObject->names.isEmpty() && !ok))
                    continue;

                context._points.push_back(primitive);

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
            if(evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_SHADOW_LEVEL, shadowLevel) && shadowLevel > 0)
            {
                context._shadowLevelMin = qMin(context._shadowLevelMin, static_cast<uint32_t>(zOrder));
                context._shadowLevelMax = qMax(context._shadowLevelMax, static_cast<uint32_t>(zOrder));
            }
        }
    }

    const auto privitivesSort = [](const Primitive& l, const Primitive& r) -> bool
    {
        if(qFuzzyCompare(l.zOrder, r.zOrder))
        {
            if(l.typeRuleIdIndex == r.typeRuleIdIndex)
                return l.mapObject->_points31.size() < r.mapObject->_points31.size();
            return l.typeRuleIdIndex < r.typeRuleIdIndex;
        }
        return l.zOrder < r.zOrder;
    };

    qSort(context._polygons.begin(), context._polygons.end(), privitivesSort);
    qSort(unfilteredLines.begin(), unfilteredLines.end(), privitivesSort);
    filterOutLinesByDensity(env, context, unfilteredLines, context._lines, controller);
    qSort(context._points.begin(), context._points.end(), privitivesSort);
}

void OsmAnd::Rasterizer_P::filterOutLinesByDensity(
    const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
    const QVector< Primitive >& in, QVector< Primitive >& out, const IQueryController* const controller )
{
    if(context._roadDensityZoomTile == 0 || context._roadsDensityLimitPerTile == 0)
    {
        out = in;
        return;
    }

    const auto dZ = context._zoom + context._roadDensityZoomTile;
    QMap< uint64_t, std::pair<uint32_t, double> > densityMap;
    out.reserve(in.size());
    for(int lineIdx = in.size() - 1; lineIdx >= 0; lineIdx--)
    {
        if(controller && controller->isAborted())
            return;

        bool accept = true;
        const auto& primitive = in[lineIdx];

        const auto typeRuleId = primitive.mapObject->_typesRuleIds[primitive.typeRuleIdIndex];
        if(typeRuleId == primitive.mapObject->section->encodingDecodingRules->highway_encodingRuleId)
        {
            accept = false;

            uint64_t prevId = 0;
            for(auto itPoint = primitive.mapObject->_points31.cbegin(); itPoint != primitive.mapObject->_points31.cend(); ++itPoint)
            {
                if(controller && controller->isAborted())
                    return;

                const auto& point = *itPoint;

                auto x = point.x >> (31 - dZ);
                auto y = point.y >> (31 - dZ);
                uint64_t id = (static_cast<uint64_t>(x) << dZ) | y;
                if(prevId != id)
                {
                    prevId = id;

                    auto& mapEntry = densityMap[id];
                    if (mapEntry.first < context._roadsDensityLimitPerTile /*&& p.second > o */)
                    {
                        accept = true;
                        mapEntry.first += 1;
                        mapEntry.second = primitive.zOrder;
                    }
                }
            }
        }

        if(accept)
            out.push_front(primitive);
    }
}

void OsmAnd::Rasterizer_P::obtainPrimitivesSymbols(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const IQueryController* const controller )
{
    // Collect symbols from all primitives
    collectPrimitivesSymbols(env, context, context._polygons, Polygons, controller);
    collectPrimitivesSymbols(env, context, context._lines, Polylines, controller);
    collectPrimitivesSymbols(env, context, context._points, Points, controller);

    // Sort symbols by order
    qSort(context._symbols.begin(), context._symbols.end(), [](const PrimitiveSymbol& l, const PrimitiveSymbol& r) -> bool
    {
        return l.order < r.order;
    });
}

void OsmAnd::Rasterizer_P::collectPrimitivesSymbols(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const QVector< Rasterizer_P::Primitive >& primitives, const PrimitivesType type, const IQueryController* const controller )
{
    assert(type != PrimitivesType::Polylines_ShadowOnly);

    for(auto itPrimitive = primitives.cbegin(); itPrimitive != primitives.cend(); ++itPrimitive)
    {
        if(controller && controller->isAborted())
            return;

        const auto& primitive = *itPrimitive;

        if(type == Polygons)
        {
            obtainPolygonSymbol(env, context, primitive);
        }
        else if(type == Polylines)
        {
            obtainPolylineSymbol(env, context, primitive);
        }
        else if(type == Points)
        {
            assert(primitive.typeRuleIdIndex == 0);

            obtainPointSymbol(env, context, primitive);
        }
    }
}

void OsmAnd::Rasterizer_P::obtainPolygonSymbol(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const Primitive& primitive )
{
    assert(primitive.mapObject->_points31.size() > 2);
    assert(primitive.mapObject->isClosedFigure());
    assert(primitive.mapObject->isClosedFigure(true));

    // Get center of polygon, since all symbols of polygon are related to it's center
    PointI64 center;
    for(auto itPoint = primitive.mapObject->_points31.cbegin(); itPoint != primitive.mapObject->_points31.cend(); ++itPoint)
    {
        const auto& point = *itPoint;

        center.x += point.x;
        center.y += point.y;
    }
    const auto verticesCount = primitive.mapObject->_points31.size();
    center.x /= verticesCount;
    center.y /= verticesCount;

    PrimitiveSymbol primitiveSymbol;
    primitiveSymbol.mapObject = primitive.mapObject;
    primitiveSymbol.location31 = Utilities::normalizeCoordinates(center, ZoomLevel31);

    // Obtain texts for this symbol
    obtainPrimitiveTexts(env, context, primitive, primitiveSymbol);

    // Publish symbol
    if(!primitiveSymbol.isEmpty())
        context._symbols.push_back(primitiveSymbol);
}

void OsmAnd::Rasterizer_P::obtainPolylineSymbol(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const Primitive& primitive )
{
    assert(primitive.mapObject->_points31.size() >= 2);

    // Symbols for polyline are always related to it's "middle" point
    const auto center = primitive.mapObject->_points31[primitive.mapObject->_points31.size() >> 1];

    PrimitiveSymbol primitiveSymbol;
    primitiveSymbol.mapObject = primitive.mapObject;
    primitiveSymbol.location31 = center;

    // Obtain texts for this symbol
    obtainPrimitiveTexts(env, context, primitive, primitiveSymbol);

    // Publish symbol
    if(!primitiveSymbol.isEmpty())
        context._symbols.push_back(primitiveSymbol);
}

void OsmAnd::Rasterizer_P::obtainPointSymbol(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const Primitive& primitive )
{
    assert(primitive.mapObject->_points31.size() > 0);

    // Depending on type of point, center is determined differently
    PointI center;
    if(primitive.mapObject->_points31.size() == 1)
    {
        // Regular point
        center = primitive.mapObject->_points31.first();
    }
    else
    {
        // Point represents center of polygon
        PointI64 center_;
        for(auto itPoint = primitive.mapObject->_points31.cbegin(); itPoint != primitive.mapObject->_points31.cend(); ++itPoint)
        {
            const auto& point = *itPoint;

            center_.x += point.x;
            center_.y += point.y;
        }

        const auto verticesCount = primitive.mapObject->_points31.size();
        center_.x /= verticesCount;
        center_.y /= verticesCount;

        center = Utilities::normalizeCoordinates(center_, ZoomLevel31);
    }

    PrimitiveSymbol primitiveSymbol;
    primitiveSymbol.mapObject = primitive.mapObject;
    primitiveSymbol.location31 = center;

    // Point can have icon associated with it
    if(primitive.evaluatorState)
    {
        MapStyleEvaluator evaluator(primitive.evaluatorState, env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Point, primitive.mapObject);
        bool ok;

        QString iconResourceName;
        ok = evaluator.getStringValue(MapStyle::builtinValueDefinitions.OUTPUT_ICON, iconResourceName);

        if(ok && !iconResourceName.isEmpty())
        {
            primitiveSymbol.icon.resourceName = iconResourceName;
            primitiveSymbol.order = 100;

            evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_ICON, primitiveSymbol.order);
        }
    }

    // Obtain texts for this symbol
    obtainPrimitiveTexts(env, context, primitive, primitiveSymbol);

    // Publish symbol
    if(!primitiveSymbol.isEmpty())
        context._symbols.push_back(primitiveSymbol);
}

void OsmAnd::Rasterizer_P::obtainPrimitiveTexts(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const Primitive& primitive, PrimitiveSymbol& primitiveSymbol )
{
    const auto typeRuleId = primitive.mapObject->_typesRuleIds[primitive.typeRuleIdIndex];
    const auto& decodedType = primitive.mapObject->section->encodingDecodingRules->decodingRules[typeRuleId];

    std::shared_ptr<MapStyleEvaluatorState> sharedEvaluatorState(new MapStyleEvaluatorState());

    bool ok;
    auto firstTextProcessed = false;
    for(auto itName = primitive.mapObject->names.cbegin(); itName != primitive.mapObject->names.cend(); ++itName)
    {
        const auto& name = itName.value();

        // Skip empty names
        if(name.isEmpty())
            continue;

        //TODO: reshape name with icu4c

        // Evaluate style to obtain text parameters
        sharedEvaluatorState->clear();
        MapStyleEvaluator evaluator(sharedEvaluatorState, env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Text, primitive.mapObject);
        env.applyTo(evaluator);
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, decodedType.tag);
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, decodedType.value);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_TEXT_LENGTH, name.length());

        QString nameTag;
        if(itName.key() != primitive.mapObject->section->encodingDecodingRules->name_encodingRuleId)
            nameTag = primitive.mapObject->section->encodingDecodingRules->decodingRules[itName.key()].tag;

        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_NAME_TAG, nameTag);
        if(!evaluator.evaluate())
            continue;

        // Skip text that doesn't have valid size
        int textSize = 0;
        ok = evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_SIZE, textSize);
        if(!ok || textSize == 0)
            continue;

        // Some text parameters are applied to symbol entirely.
        if(!firstTextProcessed)
        {
            evaluator.getBooleanValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_ON_PATH, primitiveSymbol.drawOnPath);
            evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_ORDER, primitiveSymbol.order);

            firstTextProcessed = true;
        }
#if defined(_DEBUG) || defined(DEBUG)
        else
        {
            bool drawOnPath;
            ok = evaluator.getBooleanValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_ON_PATH, drawOnPath);
            if(ok)
            {
                assert(primitiveSymbol.drawOnPath == drawOnPath);
            }

            int order;
            ok = evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_ORDER, order);
            if(ok)
            {
                assert(primitiveSymbol.order == order);
            }
        }
#endif

        PrimitiveSymbol::Text text;
        text.value = name;
        
        text.verticalOffset = 0;
        evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_DY, text.verticalOffset);

        ok = evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_COLOR, text.color);
        if(!ok || !text.color)
            text.color = SK_ColorBLACK;

        text.size = textSize;

        text.shadowRadius = 0;
        evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_HALO_RADIUS, text.shadowRadius);

        text.wrapWidth = 0;
        evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_WRAP_WIDTH, text.wrapWidth);

        text.isBold = false;
        evaluator.getBooleanValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_BOLD, text.isBold);

        text.minDistance = 0;
        evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_MIN_DISTANCE, text.minDistance);

        evaluator.getStringValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_SHIELD, text.shieldResourceName);

        primitiveSymbol.texts.push_back(text);
    }
}

void OsmAnd::Rasterizer_P::adjustContextFromEnvironment(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const ZoomLevel zoom)
{
    std::shared_ptr<MapStyleEvaluatorState> sharedEvaluatorState(new MapStyleEvaluatorState());

    context._defaultBgColor = env.defaultBgColor;
    if(env.attributeRule_defaultColor)
    {
        sharedEvaluatorState->clear();
        MapStyleEvaluator evaluator(sharedEvaluatorState, env.owner->style, env.owner->displayDensityFactor, env.attributeRule_defaultColor);

        env.applyTo(evaluator);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
        if(evaluator.evaluate())
            evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_ATTR_COLOR_VALUE, context._defaultBgColor);
    }

    context._shadowRenderingMode = env.shadowRenderingMode;
    context._shadowRenderingColor = env.shadowRenderingColor;
    if(env.attributeRule_shadowRendering)
    {
        sharedEvaluatorState->clear();
        MapStyleEvaluator evaluator(sharedEvaluatorState, env.owner->style, env.owner->displayDensityFactor, env.attributeRule_shadowRendering);

        env.applyTo(evaluator);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
        if(evaluator.evaluate())
        {
            evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE, context._shadowRenderingMode);
            evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_SHADOW_COLOR, context._shadowRenderingColor);
        }
    }

    context._polygonMinSizeToDisplay = env.polygonMinSizeToDisplay;
    if(env.attributeRule_polygonMinSizeToDisplay)
    {
        sharedEvaluatorState->clear();
        MapStyleEvaluator evaluator(sharedEvaluatorState, env.owner->style, env.owner->displayDensityFactor, env.attributeRule_polygonMinSizeToDisplay);
        env.applyTo(evaluator);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
        if(evaluator.evaluate())
        {
            int polygonMinSizeToDisplay;
            if(evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE, polygonMinSizeToDisplay))
                context._polygonMinSizeToDisplay = polygonMinSizeToDisplay;
        }
    }

    context._roadDensityZoomTile = env.roadDensityZoomTile;
    if(env.attributeRule_roadDensityZoomTile)
    {
        sharedEvaluatorState->clear();
        MapStyleEvaluator evaluator(sharedEvaluatorState, env.owner->style, env.owner->displayDensityFactor, env.attributeRule_roadDensityZoomTile);
        env.applyTo(evaluator);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
        if(evaluator.evaluate())
            evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE, context._roadDensityZoomTile);
    }

    context._roadsDensityLimitPerTile = env.roadsDensityLimitPerTile;
    if(env.attributeRule_roadsDensityLimitPerTile)
    {
        sharedEvaluatorState->clear();
        MapStyleEvaluator evaluator(sharedEvaluatorState, env.owner->style, env.owner->displayDensityFactor, env.attributeRule_roadsDensityLimitPerTile);
        env.applyTo(evaluator);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
        if(evaluator.evaluate())
            evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE, context._roadsDensityLimitPerTile);
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
        rasterizeMapPrimitives(destinationArea, canvas, context._lines, Polylines_ShadowOnly, controller);
    rasterizeMapPrimitives(destinationArea, canvas, context._lines, Polylines, controller);
}

void OsmAnd::Rasterizer_P::initializePolygonEvaluator(
    const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
    const Primitive& primitive, MapStyleEvaluator& evaluator )
{
    assert(primitive.objectType == PrimitiveType::Polygon);

    const auto typeRuleId = primitive.mapObject->_typesRuleIds[primitive.typeRuleIdIndex];
    const auto& decodedType = primitive.mapObject->section->encodingDecodingRules->decodingRules[typeRuleId];

    env.applyTo(evaluator);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, decodedType.tag);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, decodedType.value);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
}

void OsmAnd::Rasterizer_P::initializePolylineEvaluator(
    const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
    const Primitive& primitive, MapStyleEvaluator& evaluator )
{
    assert(primitive.objectType == PrimitiveType::Polyline);

    const auto typeRuleId = primitive.mapObject->_typesRuleIds[primitive.typeRuleIdIndex];
    const auto& decodedType = primitive.mapObject->section->encodingDecodingRules->decodingRules[typeRuleId];

    env.applyTo(evaluator);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, decodedType.tag);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, decodedType.value);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_LAYER, primitive.mapObject->getSimpleLayerValue());
}

void OsmAnd::Rasterizer_P::initializePointEvaluator(
    const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
    const Primitive& primitive, MapStyleEvaluator& evaluator )
{
    assert(primitive.objectType == PrimitiveType::Point);

    const auto typeRuleId = primitive.mapObject->_typesRuleIds[primitive.typeRuleIdIndex];
    const auto& decodedType = primitive.mapObject->section->encodingDecodingRules->decodingRules[typeRuleId];

    env.applyTo(evaluator);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, decodedType.tag);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, decodedType.value);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
}

void OsmAnd::Rasterizer_P::rasterizeMapPrimitives(
    const AreaI* const destinationArea,
    SkCanvas& canvas, const QVector< Primitive >& primitives, PrimitivesType type, const IQueryController* const controller )
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
            if(primitive.zOrder > polygonSizeThreshold + static_cast<int>(primitive.zOrder))
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
    const MapStyleEvaluator& evaluator, PaintValuesSet valueSetSelector, bool isArea )
{
    bool ok = true;
    struct ValueSet
    {
        const std::shared_ptr<const MapStyleValueDefinition>& color;
        const std::shared_ptr<const MapStyleValueDefinition>& strokeWidth;
        const std::shared_ptr<const MapStyleValueDefinition>& cap;
        const std::shared_ptr<const MapStyleValueDefinition>& pathEffect;
    };
    static ValueSet valueSets[] =
    {
        {//0
            MapStyle::builtinValueDefinitions.OUTPUT_COLOR,
            MapStyle::builtinValueDefinitions.OUTPUT_STROKE_WIDTH,
            MapStyle::builtinValueDefinitions.OUTPUT_CAP,
            MapStyle::builtinValueDefinitions.OUTPUT_PATH_EFFECT
        },
        {//1
            MapStyle::builtinValueDefinitions.OUTPUT_COLOR_2,
            MapStyle::builtinValueDefinitions.OUTPUT_STROKE_WIDTH_2,
            MapStyle::builtinValueDefinitions.OUTPUT_CAP_2,
            MapStyle::builtinValueDefinitions.OUTPUT_PATH_EFFECT_2
        },
        {//-1
            MapStyle::builtinValueDefinitions.OUTPUT_COLOR_0,
            MapStyle::builtinValueDefinitions.OUTPUT_STROKE_WIDTH_0,
            MapStyle::builtinValueDefinitions.OUTPUT_CAP_0,
            MapStyle::builtinValueDefinitions.OUTPUT_PATH_EFFECT_0
        },
        {//-2
            MapStyle::builtinValueDefinitions.OUTPUT_COLOR__1,
            MapStyle::builtinValueDefinitions.OUTPUT_STROKE_WIDTH__1,
            MapStyle::builtinValueDefinitions.OUTPUT_CAP__1,
            MapStyle::builtinValueDefinitions.OUTPUT_PATH_EFFECT__1
        },
        {//else
            MapStyle::builtinValueDefinitions.OUTPUT_COLOR_3,
            MapStyle::builtinValueDefinitions.OUTPUT_STROKE_WIDTH_3,
            MapStyle::builtinValueDefinitions.OUTPUT_CAP_3,
            MapStyle::builtinValueDefinitions.OUTPUT_PATH_EFFECT_3
        },
    };
    const ValueSet& valueSet = valueSets[static_cast<int>(valueSetSelector)];

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
        ok = evaluator.getFloatValue(valueSet.strokeWidth, stroke);
        if(!ok || stroke <= 0.0f)
            return false;

        _mapPaint.setColorFilter(nullptr);
        _mapPaint.setShader(nullptr);
        _mapPaint.setLooper(nullptr);
        _mapPaint.setStyle(SkPaint::kStroke_Style);
        _mapPaint.setStrokeWidth(stroke);

        QString cap;
        ok = evaluator.getStringValue(valueSet.cap, cap);
        if (!ok || cap.isEmpty() || cap == QLatin1String("BUTT"))
            _mapPaint.setStrokeCap(SkPaint::kButt_Cap);
        else if (cap == QLatin1String("ROUND"))
            _mapPaint.setStrokeCap(SkPaint::kRound_Cap);
        else if (cap == QLatin1String("SQUARE"))
            _mapPaint.setStrokeCap(SkPaint::kSquare_Cap);
        else
            _mapPaint.setStrokeCap(SkPaint::kButt_Cap);

        QString encodedPathEffect;
        ok = evaluator.getStringValue(valueSet.pathEffect, encodedPathEffect);
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
    ok = evaluator.getIntegerValue(valueSet.color, color);
    if(!ok || !color)
        return false;
    _mapPaint.setColor(color);

    if (valueSetSelector == PaintValuesSet::Set_0)
    {
        QString shader;
        ok = evaluator.getStringValue(MapStyle::builtinValueDefinitions.OUTPUT_SHADER, shader);
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
        ok = evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_SHADOW_COLOR, shadowColor);
        int shadowRadius;
        evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_SHADOW_RADIUS, shadowRadius);
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
    SkCanvas& canvas, const Primitive& primitive )
{
    assert(primitive.mapObject->_points31.size() > 2);
    assert(primitive.mapObject->isClosedFigure());
    assert(primitive.mapObject->isClosedFigure(true));

    MapStyleEvaluator evaluator(primitive.evaluatorState, env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Polygon, primitive.mapObject);
    if(!updatePaint(evaluator, Set_0, true))
        return;

    SkPath path;
    bool containsAtLeastOnePoint = false;
    int pointIdx = 0;
    PointF vertex;
    int bounds = 0;
    QVector< PointF > outsideBounds;
    for(auto itPoint = primitive.mapObject->points31.cbegin(); itPoint != primitive.mapObject->points31.cend(); ++itPoint, pointIdx++)
    {
        const auto& point = *itPoint;

        calculateVertex(point, vertex);

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
                outsideBounds.push_back(vertex);
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

    if(!primitive.mapObject->innerPolygonsPoints31.isEmpty())
    {
        path.setFillType(SkPath::kEvenOdd_FillType);
        for(auto itPolygon = primitive.mapObject->innerPolygonsPoints31.cbegin(); itPolygon != primitive.mapObject->innerPolygonsPoints31.cend(); ++itPolygon)
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
    if(updatePaint(evaluator, Set_1, false))
        canvas.drawPath(path, _mapPaint);
}

void OsmAnd::Rasterizer_P::rasterizePolyline(
    const AreaI* const destinationArea,
    SkCanvas& canvas, const Primitive& primitive, bool drawOnlyShadow )
{
    assert(primitive.mapObject->_points31.size() >= 2);

    MapStyleEvaluator evaluator(primitive.evaluatorState, env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Polyline, primitive.mapObject);
    if(!updatePaint(evaluator, Set_0, false))
        return;

    bool ok;

    int shadowColor;
    ok = evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_SHADOW_COLOR, shadowColor);
    if(!ok || shadowColor == 0)
        shadowColor = context._shadowRenderingColor;

    int shadowRadius;
    ok = evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_SHADOW_RADIUS, shadowRadius);
    if(drawOnlyShadow && (!ok || shadowRadius == 0))
        return;

    const auto typeRuleId = primitive.mapObject->_typesRuleIds[primitive.typeRuleIdIndex];

    int oneway = 0;
    if(context._zoom >= ZoomLevel16 && typeRuleId == primitive.mapObject->section->encodingDecodingRules->highway_encodingRuleId)
    {
        if(primitive.mapObject->containsType(primitive.mapObject->section->encodingDecodingRules->oneway_encodingRuleId, true))
            oneway = 1;
        else if(primitive.mapObject->containsType(primitive.mapObject->section->encodingDecodingRules->onewayReverse_encodingRuleId, true))
            oneway = -1;
    }

    SkPath path;
    int pointIdx = 0;
    const auto middleIdx = primitive.mapObject->_points31.size() >> 1;
    bool intersect = false;
    int prevCross = 0;
    PointF vertex, middleVertex;
    for(auto itPoint = primitive.mapObject->_points31.cbegin(); itPoint != primitive.mapObject->_points31.cend(); ++itPoint, pointIdx++)
    {
        const auto& point = *itPoint;

        calculateVertex(point, vertex);

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
            if (updatePaint(evaluator, Set_minus2, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            if (updatePaint(evaluator, Set_minus1, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            if (updatePaint(evaluator, Set_0, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            canvas.drawPath(path, _mapPaint);
            if (updatePaint(evaluator, Set_1, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            if (updatePaint(evaluator, Set_3, false))
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

        std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_isArea = false;
        mapObject->_points31 = polyline;
        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastlineLine_encodingRuleId);

        outVectorized.push_back(mapObject);
    }

    const bool coastlineCrossesBounds = !coastlinePolylines.isEmpty();
    if(!coastlinePolylines.isEmpty())
    {
        // Add complete water tile with holes
        std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_points31.push_back(PointI(context._area31.left, context._area31.top));
        mapObject->_points31.push_back(PointI(context._area31.right, context._area31.top));
        mapObject->_points31.push_back(PointI(context._area31.right, context._area31.bottom));
        mapObject->_points31.push_back(PointI(context._area31.left, context._area31.bottom));
        mapObject->_points31.push_back(mapObject->_points31.first());
        convertCoastlinePolylinesToPolygons(env, context, coastlinePolylines, mapObject->_innerPolygonsPoints31, osmId);

        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
        mapObject->_id = osmId;
        mapObject->_isArea = true;

        assert(mapObject->isClosedFigure());
        assert(mapObject->isClosedFigure(true));
        outVectorized.push_back(mapObject);
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

            std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
            mapObject->_isArea = false;
            mapObject->_points31 = polygon;
            mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastlineBroken_encodingRuleId);

            outVectorized.push_back(mapObject);
        }
    }

    // Draw coastlines
    for(auto itPolygon = closedPolygons.cbegin(); itPolygon != closedPolygons.cend(); ++itPolygon)
    {
        const auto& polygon = *itPolygon;

        std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_isArea = false;
        mapObject->_points31 = polygon;
        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastlineLine_encodingRuleId);

        outVectorized.push_back(mapObject);
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

        std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_points31 = polygon;
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
        outVectorized.push_back(mapObject);
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
        std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_points31.push_back(PointI(context._area31.left, context._area31.top));
        mapObject->_points31.push_back(PointI(context._area31.right, context._area31.top));
        mapObject->_points31.push_back(PointI(context._area31.right, context._area31.bottom));
        mapObject->_points31.push_back(PointI(context._area31.left, context._area31.bottom));
        mapObject->_points31.push_back(mapObject->_points31.first());

        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
        mapObject->_id = osmId;
        mapObject->_isArea = true;

        assert(mapObject->isClosedFigure());
        outVectorized.push_back(mapObject);
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

                polyline.push_back(p);
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

void OsmAnd::Rasterizer_P::rasterizeSymbolsWithoutPaths(
    QList< std::shared_ptr<const RasterizedSymbol> >& outSymbols,
    const IQueryController* const controller )
{
    for(auto itPrimitiveSymbol = context._symbols.cbegin(); itPrimitiveSymbol != context._symbols.end(); ++itPrimitiveSymbol)
    {
        const auto& primitiveSymbol = *itPrimitiveSymbol;

        // Skip symbols that need to be rasterized along path
        if(primitiveSymbol.drawOnPath)
            continue;

        // Obtain icon if there is one
        std::shared_ptr<const SkBitmap> icon;
        if(!primitiveSymbol.icon.resourceName.isEmpty())
            env.obtainIcon(primitiveSymbol.icon.resourceName, icon);

        // Rasterize each of texts in this symbol
        QList< std::shared_ptr<const SkBitmap> > rasterizedTexts;
        for(auto itText = primitiveSymbol.texts.cbegin(); itText != primitiveSymbol.texts.cend(); ++itText)
        {
            const auto& text = *itText;

            // Configure paint for text
            SkPaint textPaint = env.textPaint;

            textPaint.setTextSize(text.size);
            textPaint.setFakeBoldText(text.isBold);
            textPaint.setColor(text.color);

            // Measure text
            SkRect textBounds;
            textPaint.measureText(text.value.constData(), text.value.length()*sizeof(QChar), &textBounds);

            SkRect textBBox = textBounds;

            // Process shadow
            SkPaint textShadowPaint;
            SkRect shadowBounds;

            if(text.shadowRadius > 0)
            {
                textShadowPaint = textPaint;

                textShadowPaint.setStyle(SkPaint::kStroke_Style);
                textShadowPaint.setColor(SK_ColorWHITE);
                textShadowPaint.setStrokeWidth(text.shadowRadius);

                textShadowPaint.measureText(text.value.constData(), text.value.length()*sizeof(QChar), &shadowBounds);
                textBBox.join(shadowBounds);
            }

            // Create a bitmap that will be hold text
            auto bitmap = new SkBitmap();
            bitmap->setConfig(SkBitmap::kARGB_8888_Config, textBBox.width(), textBBox.height());
            bitmap->allocPixels();
            SkBitmapDevice target(*bitmap);
            SkCanvas canvas(&target);

            // Rasterize text
            if(text.shadowRadius > 0)
                canvas.drawText(text.value.constData(), text.value.length()*sizeof(QChar), -textBBox.left(), -textBBox.top(), textShadowPaint);
            canvas.drawText(text.value.constData(), text.value.length()*sizeof(QChar), -textBBox.left(), -textBBox.top(), textPaint);

            //////////////////////////////////////////////////////////////////////////
            /*std::unique_ptr<SkImageEncoder> encoder(CreatePNGImageEncoder());
            QString path;
            path.sprintf("D:\\texts\\%p.png", bitmap);
            encoder->encodeFile(path.toLocal8Bit(), *bitmap, 100);*/
            //////////////////////////////////////////////////////////////////////////

            rasterizedTexts.push_back(std::shared_ptr<const SkBitmap>(bitmap));
        }

        // Create container and store it
        auto rasterizedSymbol = new RasterizedSymbol(
            primitiveSymbol.mapObject,
            primitiveSymbol.location31,
            icon,
            rasterizedTexts);
        outSymbols.push_back(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol));
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
    , drawOnPath(false)
{
}
