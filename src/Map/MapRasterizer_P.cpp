#include "MapRasterizer_P.h"
#include "MapRasterizer.h"
#include "MapRasterizer_Metrics.h"

#include <algorithm>
#include <deque>
#include <cmath>
#include <limits>
#include "QtCommon.h"
#include "ignore_warnings_on_external_includes.h"
#include <QReadWriteLock>
#include <QSet>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkImage.h>
#include <SkBlurMaskFilter.h>
#include <SkColorFilter.h>
#include <SkDashPathEffect.h>
#include <SkShader.h>
#include <SkPoint.h>
#include <SkPathMeasure.h>
#include "restore_internal_warnings.h"

#include "MapPresentationEnvironment.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleBuiltinValueDefinitions.h"
#include "MapPrimitiviser.h"
#include "QKeyValueIterator.h"
#include "QCachingIterator.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"

#define RealisticRoadsMinZoom ZoomLevel17
#define RealisticRoadsMinLaneMarkingWidth 8.0f
#define RealisticRoadsMinLaneArrowWidth 14.0f
#define RealisticRoadsTaperLength 100.0f
#define RealisticRoadsLaneTaperLength 70.0f
#define RealisticRoadsGoreMaxLength 100.0f
#define RealisticRoadsGoreWidth 3.0f
#define RealisticRoadsJoinBlendLength 6.0f
// cos(40 degrees)
#define RealisticRoadsMaxJoinCos 0.766

// #define DRAW_AREA_BOUNDS 1
#ifndef DRAW_AREA_BOUNDS
#   define DRAW_AREA_BOUNDS 0
#endif // !defined(DRAW_AREA_BOUNDS)

// #define DRAW_OVERSCALED_TILE_BOUNDS 1
#ifndef DRAW_OVERSCALED_TILE_BOUNDS
#   define DRAW_OVERSCALED_TILE_BOUNDS 0
#endif // !defined(DRAW_OVERSCALED_TILE_BOUNDS)

OsmAnd::MapRasterizer_P::MapRasterizer_P(MapRasterizer* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapRasterizer_P::~MapRasterizer_P()
{
}

void OsmAnd::MapRasterizer_P::initialize()
{
    _defaultPaint.setAntiAlias(true);
}

void OsmAnd::MapRasterizer_P::rasterize(
    const AreaI area31,
    const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
    SkCanvas& canvas,
    const bool fillBackground,
    const AreaI* const pDestinationArea,
    MapRasterizer_Metrics::Metric_rasterize* const metric,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const Stopwatch totalStopwatch(metric != nullptr);

    Context context(
        area31,
        primitivisedObjects,
        pDestinationArea ? *pDestinationArea : AreaI(0, 0, canvas.imageInfo().height(), canvas.imageInfo().width()));

    // Lane layouts of all roads of the tile, so that joined roads can line up their lanes
    if (context.realisticRoads)
    {
        for (const auto& primitive : constOf(primitivisedObjects->polylines))
        {
            const auto& mapObject = primitive->sourceObject;
            if (context.roadLayouts.contains(mapObject.get()))
                continue;
            if (primitive->attributeIdIndex >= static_cast<uint32_t>(mapObject->attributeIds.size()))
                continue;
            const auto& decodeMap = mapObject->attributeMapping->decodeMap;
            const auto attributeId = mapObject->attributeIds[primitive->attributeIdIndex];
            if (attributeId >= static_cast<uint32_t>(decodeMap.size()))
                continue;
            const auto& type = decodeMap[attributeId];
            if (type.tag != QLatin1String("highway"))
                continue;
            RoadLayout layout;
            if (computeRoadLayout(mapObject, type.value, layout))
                context.roadLayouts.insert(mapObject.get(), layout);
        }
        resolveRoadTransitions(context);
    }

    // Deal with background
    if (fillBackground)
    {
        // Get default background color
        const auto defaultBackgroundColor = context.env->getDefaultBackgroundColor(context.zoom);

        if (pDestinationArea)
        {
            // If destination area is specified, fill only it with background
            SkPaint bgPaint;
            bgPaint.setColor(defaultBackgroundColor.toSkColor());
            bgPaint.setStyle(SkPaint::kFill_Style);
            canvas.drawRect(
                SkRect::MakeLTRB(
                    pDestinationArea->top(),
                    pDestinationArea->left(),
                    pDestinationArea->right(),
                    pDestinationArea->bottom()
                ),
                bgPaint);
        }
        else
        {
            // Since destination area is not specified, erase whole canvas with specified color
            canvas.clear(defaultBackgroundColor.toSkColor());
        }
    }

    AreaI destinationArea;
    if (pDestinationArea)
    {
        destinationArea = *pDestinationArea;
    }
    else
    {
        const auto targetSize = canvas.getBaseLayerSize();
        destinationArea = AreaI(0, 0, targetSize.height(), targetSize.width());
    }

    // Rasterize layers of map:
    rasterizeMapPrimitives(context, canvas, primitivisedObjects->polygons, PrimitivesType::Polygons, queryController);
    if (context.shadowMode != MapPresentationEnvironment::ShadowMode::NoShadow)
        rasterizeMapPrimitives(context, canvas, primitivisedObjects->polylines, PrimitivesType::Polylines_ShadowOnly,
                               queryController);
    rasterizeMapPrimitives(context, canvas, primitivisedObjects->polylines, PrimitivesType::Polylines, queryController);

#if DRAW_AREA_BOUNDS
    {
        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(context.env->displayDensityFactor);
        paint.setColor(SK_ColorRED);

        const auto rect = SkRect::MakeLTRB(0, 0, context.pixelArea.right(), context.pixelArea.bottom());
        canvas.drawRect(rect, paint);
    }
#endif // DRAW_AREA_BOUNDS

#if DRAW_OVERSCALED_TILE_BOUNDS
    if (primitivisedObjects->zoom != ZoomLevel::MinZoomLevel)
    {
        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(2 * context.env->displayDensityFactor);
        paint.setColor(SK_ColorBLACK);

        const auto tileId31 = TileId::fromXY(area31.left(), area31.top());
        const auto tileId = Utilities::getTileIdOverscaledByZoomShift(tileId31, ZoomLevel::ZoomLevel31 - primitivisedObjects->zoom);

        QVector<PointI> points;
        points.push_back(context.pixelArea.bottomLeft());
        points.push_back(context.pixelArea.topLeft);
        points.push_back(context.pixelArea.topRight());
        points.push_back(context.pixelArea.bottomRight);

        SkPath path;

        for (int i = 0; i < 4; i++)
        {
            bool draw = i % 2 == 0 && (tileId.x + i / 2) >> 1 << 1 == tileId.x + i / 2
                || i % 2 == 1 && (tileId.y + i / 2) >> 1 << 1 == tileId.y + i / 2;

            if (draw)
            {
                const auto& start = points[i];
                const auto& end = points[(i + 1) % 4];

                path.reset();
                path.moveTo(SkPoint::Make(start.x, start.y));
                path.lineTo(SkPoint::Make(end.x, end.y));
                canvas.drawPath(path, paint);
            }
        }
    }
#endif // DRAW_OVERSCALED_TILE_BOUNDS

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();
}

void OsmAnd::MapRasterizer_P::rasterizeMapPrimitives(
    const Context& context,
    SkCanvas& canvas,
    const MapPrimitiviser::PrimitivesCollection& primitives,
    PrimitivesType type,
    const std::shared_ptr<const IQueryController>& queryController)
{
    assert(type != PrimitivesType::Points);

    for (const auto& primitive : constOf(primitives))
    {
        if (queryController && queryController->isAborted())
            return;

        if (primitive->type == MapPrimitiviser::PrimitiveType::Polygon)
        {
            rasterizePolygon(
                context,
                canvas,
                primitive);
        }
        else if (primitive->type == MapPrimitiviser::PrimitiveType::Polyline)
        {
            if (type == PrimitivesType::Polylines && !context.pendingMarkings.isEmpty()
                && static_cast<int>(primitive->sourceObject->getLayerType()) != context.pendingMarkingsLayer)
            {
                flushLaneMarkings(context, canvas);
            }
            rasterizePolyline(
                context,
                canvas,
                primitive,
                (type == PrimitivesType::Polylines_ShadowOnly));
        }
    }
    if (type == PrimitivesType::Polylines)
        flushLaneMarkings(context, canvas);
}

bool OsmAnd::MapRasterizer_P::updatePaint(
    const Context& context,
    SkPaint& paint,
    const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive,
    const PaintValuesSet valueSetSelector,
    const bool isArea,
    const RealisticRoad* const road /*= nullptr*/)
{
    const auto& env = context.env;

    bool ok = true;

    int valueDefId_color = -1;
    int valueDefId_strokeWidth = -1;
    int valueDefId_cap = -1;
    int valueDefId_join = -1;
    int valueDefId_pathEffect = -1;
    switch (valueSetSelector)
    {
        case PaintValuesSet::Layer_minus2:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR__2;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH__2;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP__2;
            valueDefId_join = env->styleBuiltinValueDefs->id_OUTPUT_JOIN__2;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT__2;
            break;
        case PaintValuesSet::Layer_minus1:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR__1;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH__1;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP__1;
            valueDefId_join = env->styleBuiltinValueDefs->id_OUTPUT_JOIN__1;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT__1;
            break;
        case PaintValuesSet::Layer_0:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR_0;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_0;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP_0;
            valueDefId_join = env->styleBuiltinValueDefs->id_OUTPUT_JOIN_0;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT_0;
            break;
        case PaintValuesSet::Layer_1:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP;
            valueDefId_join = env->styleBuiltinValueDefs->id_OUTPUT_JOIN;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT;
            break;
        case PaintValuesSet::Layer_2:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR_2;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_2;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP_2;
            valueDefId_join = env->styleBuiltinValueDefs->id_OUTPUT_JOIN_2;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT_2;
            break;
        case PaintValuesSet::Layer_3:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR_3;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_3;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP_3;
            valueDefId_join = env->styleBuiltinValueDefs->id_OUTPUT_JOIN_3;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT_3;
            break;
        case PaintValuesSet::Layer_4:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR_4;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_4;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP_4;
            valueDefId_join = env->styleBuiltinValueDefs->id_OUTPUT_JOIN_4;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT_4;
            break;
        case PaintValuesSet::Layer_5:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR_5;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_5;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP_5;
            valueDefId_join = env->styleBuiltinValueDefs->id_OUTPUT_JOIN_5;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT_5;
            break;
        default:
            return false;
    }

    const auto& evalResult = primitive->evaluationResult;
    if (isArea)
    {
        if (!evalResult.contains(valueDefId_color) && !evalResult.contains(env->styleBuiltinValueDefs->id_OUTPUT_SHADER))
            return false;

        paint.setColorFilter(nullptr);
        paint.setShader(nullptr);
        paint.setStyle(SkPaint::kStrokeAndFill_Style);
        paint.setStrokeWidth(0);
    }
    else
    {
        float stroke;
        ok = evalResult.getFloatValue(valueDefId_strokeWidth, stroke);
        if (!ok || stroke <= 0.0f)
            return false;

        paint.setColorFilter(nullptr);
        paint.setShader(nullptr);
        paint.setStyle(SkPaint::kStroke_Style);
        auto strokeWidth = stroke * primitive->detailScaleFactor;
        if (road && road->valid)
        {
            // Thinner layers (centre lines, overlays) scale with the road, wider ones (casing,
            // bridge and tunnel outlines) keep their margin around the real carriageway
            if (strokeWidth <= road->styleWidth)
                strokeWidth *= road->width / road->styleWidth;
            else
                strokeWidth = road->width + (strokeWidth - road->styleWidth);
        }
        paint.setStrokeWidth(strokeWidth);

        QString cap;
        evalResult.getStringValue(valueDefId_cap, cap);
        if (road && road->valid && road->buttCaps)
            paint.setStrokeCap(SkPaint::kButt_Cap);
        else if (cap.compare(QLatin1String("round"), Qt::CaseInsensitive) == 0)
            paint.setStrokeCap(SkPaint::kRound_Cap);
        else if (cap.compare(QLatin1String("square"), Qt::CaseInsensitive) == 0)
            paint.setStrokeCap(SkPaint::kSquare_Cap);
        else
            paint.setStrokeCap(SkPaint::kButt_Cap);

        QString join;
        evalResult.getStringValue(valueDefId_join, join);
        if (join.compare(QLatin1String("miter"), Qt::CaseInsensitive) == 0)
            paint.setStrokeJoin(SkPaint::kMiter_Join);
        else if (join.compare(QLatin1String("bevel"), Qt::CaseInsensitive) == 0)
            paint.setStrokeJoin(SkPaint::kBevel_Join);
        else
            paint.setStrokeJoin(SkPaint::kRound_Join);

        QString encodedPathEffect;
        ok = evalResult.getStringValue(valueDefId_pathEffect, encodedPathEffect);
        if (!ok || encodedPathEffect.isEmpty())
        {
            paint.setPathEffect(nullptr);
        }
        else
        {
            sk_sp<SkPathEffect> pathEffect;
            ok = obtainPathEffect(encodedPathEffect, pathEffect);

            if (ok && pathEffect)
                paint.setPathEffect(pathEffect);
        }
    }

    SkColor color = SK_ColorTRANSPARENT;
    evalResult.getIntegerValue(valueDefId_color, color);
    paint.setColor(color);

    if (valueSetSelector == PaintValuesSet::Layer_1)
    {
        QString shader;
        ok = evalResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADER, shader);
        if (ok && !shader.isEmpty())
        {
            sk_sp<SkShader> skShader;
            if (obtainImageShader(env, shader, skShader) && skShader)
            {
                // SKIA requires non-transparent color
                if (paint.getColor() == SK_ColorTRANSPARENT)
                    paint.setColor(SK_ColorWHITE);

                paint.setShader(skShader);
            }
        }
    }

    // do not check shadow color here
    if (context.shadowMode == MapPresentationEnvironment::ShadowMode::OneStep && valueSetSelector == PaintValuesSet::Layer_1)
    {
        ColorARGB shadowColor(0x00000000);
        ok = evalResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_COLOR, shadowColor.argb);
        if (!ok || shadowColor.isTransparent())
            shadowColor = context.shadowColor;

        float shadowRadius = 0.0f;
        evalResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_RADIUS, shadowRadius);

        if (shadowRadius > 0.0f && !shadowColor.isTransparent())
        {
            // const auto looper = SkBlurDrawLooper::Make(
            //     shadowColor.toSkColor(),
            //     SkBlurMaskFilter::ConvertRadiusToSigma(shadowRadius),
            //     0, 0
            // );
            // TODO: loopers are not supported in Skia now 
        }
    }

    return true;
}

void OsmAnd::MapRasterizer_P::rasterizePolygon(
    const Context& context,
    SkCanvas& canvas,
    const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive)
{
    const auto& points31 = primitive->sourceObject->points31;
    const auto& area31 = context.area31;

    assert(points31.size() > 2);
    // assert(primitive->sourceObject->isClosedFigure());
    // assert(primitive->sourceObject->isClosedFigure(true));

    //////////////////////////////////////////////////////////////////////////
    //if ((primitive->sourceObject->id >> 1) == 9223372032559801460u)
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    const auto& evaluationResult = primitive->evaluationResult;
    if (!evaluationResult.contains(context.env->styleBuiltinValueDefs->id_OUTPUT_COLOR) &&
        !evaluationResult.contains(context.env->styleBuiltinValueDefs->id_OUTPUT_SHADER))
        return;

    SkPaint paint = _defaultPaint;

    // Construct and test geometry against bbox area
    SkPath path;
    bool containsAtLeastOnePoint = false;
    int pointIdx = 0;
    PointF vertex;
    Utilities::CHValue prevChValue;
    QVector< PointI > outerPoints;
    const auto pointsCount = points31.size();
    auto pPoint = points31.constData();
    for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        const auto& point = *pPoint;
        calculateVertex(context, point, vertex);

        // Hit-test
        if (!containsAtLeastOnePoint)
        {
            if (area31.contains(point))
                containsAtLeastOnePoint = true;
            else
                outerPoints.push_back(point);

            const auto chValue = Utilities::computeCohenSutherlandValue(point, area31);
            if (Q_LIKELY(pointIdx > 0))
            {
                // Check if line crosses area (reject only if points are on the same side)
                const auto intersectedChValue = prevChValue & chValue;
                if (static_cast<unsigned int>(intersectedChValue) != 0)
                    containsAtLeastOnePoint = true;
            }
            prevChValue = chValue;
        }

        // Plot vertex
        if (pointIdx == 0)
            path.moveTo(vertex.x, vertex.y);
        else
            path.lineTo(vertex.x, vertex.y);
    }

    //////////////////////////////////////////////////////////////////////////
    //if ((primitive->sourceObject->id >> 1) == 9223372032559801460u)
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    if (!containsAtLeastOnePoint)
    {
        // Check area is inside polygon
        bool ok = true;
        ok = ok || OsmAnd::Utilities::contains(outerPoints, area31.topLeft);
        ok = ok || OsmAnd::Utilities::contains(outerPoints, area31.bottomRight);
        ok = ok || OsmAnd::Utilities::contains(outerPoints, PointI(0, area31.bottom()));
        ok = ok || OsmAnd::Utilities::contains(outerPoints, PointI(area31.right(), 0));
        if (!ok)
            return;
    }

    //////////////////////////////////////////////////////////////////////////
    //if ((primitive->sourceObject->id >> 1) == 95692962u)
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    if (!primitive->sourceObject->innerPolygonsPoints31.isEmpty())
    {
        path.setFillType(SkPathFillType::kEvenOdd);
        for (const auto& polygon : constOf(primitive->sourceObject->innerPolygonsPoints31))
        {
            pointIdx = 0;
            for (auto itVertex = cachingIteratorOf(constOf(polygon)); itVertex; ++itVertex, pointIdx++)
            {
                const auto& point = *itVertex;
                calculateVertex(context, point, vertex);

                if (pointIdx == 0)
                    path.moveTo(vertex.x, vertex.y);
                else
                    path.lineTo(vertex.x, vertex.y);
            }
        }
    }

    if (updatePaint(context, paint, primitive, PaintValuesSet::Layer_0, true))
        canvas.drawPath(path, paint);
    if (updatePaint(context, paint, primitive, PaintValuesSet::Layer_1, true))
        canvas.drawPath(path, paint);
    if (updatePaint(context, paint, primitive, PaintValuesSet::Layer_2, false))
        canvas.drawPath(path, paint);
}

bool OsmAnd::MapRasterizer_P::calculateLinePath(
    const Context& context,
    const QVector<PointI>& points31,
    const AreaI& area31,
    SkPath& outPath,
    float offset = 0.0f) const
{
    bool rightShift = offset > 0;
    offset = abs(offset);
    bool hasShift = offset > 0;

    bool intersect = false;
    int pointIdx = 0;
    int prevCross = 0;
    const auto pointsCount = points31.size();
    auto pPoint = points31.constData();
    PointF vertex;
    PointF pVertex;
    PointF tempVertex;
    PointF correctedVertex;
    // Could be implemented/extended custom simple deque to store only last 3 point for the originalPoints
    std::deque<PointF> originalPoints;
    std::deque<PointF> shiftedPoints;

    for (pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        const auto& point = *pPoint;
        calculateVertex(context, point, vertex);

        int cross = 0;
        cross |= (point.x < area31.left() ? 1 : 0);
        cross |= (point.x > area31.right() ? 2 : 0);
        cross |= (point.y < area31.top() ? 4 : 0);
        cross |= (point.y > area31.bottom() ? 8 : 0);
        if (pointIdx > 0)
        {
            if ((prevCross & cross) == 0)
            {
                if (prevCross != 0 || !intersect)
                {
                    simplifyVertexToDirection(context, pVertex, vertex, tempVertex);
                    if (!hasShift)
                    {
                        outPath.moveTo(tempVertex.x, tempVertex.y);
                    }
                }
                simplifyVertexToDirection(context, vertex, pVertex, tempVertex);
                if (hasShift)
                {
                    auto normal = Utilities::computeNormalToLine(pVertex, vertex, rightShift);
                    auto addition = normal * offset;
                    auto p1 = pVertex + addition;
                    outPath.moveTo(p1.x, p1.y);
                    auto p2 = tempVertex + addition;
                    outPath.lineTo(p2.x, p2.y);
                }
                else
                {
                    outPath.lineTo(tempVertex.x, tempVertex.y);
                }
                intersect = true;
            }
        }
        prevCross = cross;
        pVertex = vertex;
    }

    return intersect;
}

void OsmAnd::MapRasterizer_P::drawLineLayer(
    SkCanvas& canvas,
    SkPaint& paint,
    SkPath& path,
    const Context& context,
    const AreaI& area31,
    const QVector<PointI>& points31,
    const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive,
    const PaintValuesSet valueSetSelector,
    const IMapStyle::ValueDefinitionId hMarginId,
    const RealisticRoad* const road)
{
    if (updatePaint(context, paint, primitive, valueSetSelector, false, road))
    {
        float hMargin = 0.0f;
        const bool hasHMargin = primitive->evaluationResult.getFloatValue(hMarginId, hMargin);
        if (road && road->valid && (!hasHMargin || qAbs(hMargin) < 0.01f))
        {
            // The realistic road geometry already carries the lane placement
            drawRoadBody(canvas, paint, path, *road);
        }
        else if (hasHMargin)
        {
            SkPath newPath;
            if (calculateLinePath(context, points31, area31, newPath, hMargin))
                canvas.drawPath(newPath, paint);
        }
        else
        {
            canvas.drawPath(path, paint);
        }
    }
}

void OsmAnd::MapRasterizer_P::rasterizePolyline(
    const Context& context,
    SkCanvas& canvas,
    const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive,
    bool drawOnlyShadow)
{
    const auto& env = context.env;

    const auto& evaluationResult = primitive->evaluationResult;
    SkPaint paint = _defaultPaint;
    ColorARGB shadowColor;
    float shadowRadius = 0.0f;
    RealisticRoad road;
    computeRealisticRoad(context, primitive, road);
    if (drawOnlyShadow)
    {
        const auto hasShadowRadius = evaluationResult.getFloatValue(
            env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_RADIUS,
            shadowRadius);
        if (!hasShadowRadius || shadowRadius <= 0.0f)
            return;

        if (!updatePaint(context, paint, primitive, PaintValuesSet::Layer_1, false, &road))
            return;

        const auto hasShadowColor = evaluationResult.getIntegerValue(
            env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_COLOR,
            shadowColor.argb);
        if (!hasShadowColor || shadowColor == ColorARGB::fromSkColor(SK_ColorTRANSPARENT))
            shadowColor = context.shadowColor;
    }
    else
    {
        float strokeWidth = 0.0f;
        if (!evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH, strokeWidth) ||
            strokeWidth <= 0.0f)
        {
            return;
        }
    }

    // Enlarge area to draw stroke if logical path is outside of original area
    const auto& area31 = context.area31;
    const auto& scalePixelTo31 = context.primitivisedObjects->scaleDivisor31ToPixel;
    const auto enlarge31X = static_cast<int>(context.pixelArea.width() / 4.0f * scalePixelTo31.x);
    const auto enlarge31Y = static_cast<int>(context.pixelArea.height() / 4.0f * scalePixelTo31.y);
    const auto enlargedArea31 = area31.getEnlargedBy(PointI(enlarge31X, enlarge31Y));

    // Process only visible parts of polyline if available
    const auto& mapObject = primitive->sourceObject;
    auto areaIndex = mapObject->startReadingArea();
    if (areaIndex >= 0 && !mapObject->vapItems[areaIndex]->area31.contains(enlargedArea31))
    {
        mapObject->stopReadingArea(areaIndex);
        areaIndex = -1;
    }
    const auto& points31 = areaIndex >= 0 ? mapObject->vapItems[areaIndex]->points31 : mapObject->points31;

    assert(points31.size() >= 2);

    SkPath path;
    bool intersect = calculateLinePath(context, points31, enlargedArea31, path);
    if (!intersect && !road.valid)
    {
        if (areaIndex >= 0)
            mapObject->stopReadingArea(areaIndex);
        return;
    }

    if (road.valid)
    {
        // Full geometry: a wide road is visible in a tile even when its axis is far outside
        const auto& g = road.geometry;
        QVector<float> centre(g.vertices.size());
        for (int i = 0; i < centre.size(); i++)
            centre[i] = (g.bodyLeft[i] + g.bodyRight[i]) / 2.0f;
        path = buildOffsetPath(g, centre);
    }

    if (drawOnlyShadow)
    {
        rasterizePolylineShadow(
            context,
            canvas,
            path,
            paint,
            shadowColor,
            shadowRadius);
    }
    else
    {
        drawLineLayer(canvas, paint, path, context, enlargedArea31, points31, primitive, PaintValuesSet::Layer_minus2,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN__2, &road);

        drawLineLayer(canvas, paint, path, context, enlargedArea31, points31, primitive, PaintValuesSet::Layer_minus1,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN__1, &road);

        drawLineLayer(canvas, paint, path, context, enlargedArea31, points31, primitive, PaintValuesSet::Layer_0,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN_0, &road);

        drawLineLayer(canvas, paint, path, context, enlargedArea31, points31, primitive, PaintValuesSet::Layer_1,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN, &road);

        drawLineLayer(canvas, paint, path, context, enlargedArea31, points31, primitive, PaintValuesSet::Layer_2,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN_2, &road);

        drawLineLayer(canvas, paint, path, context, enlargedArea31, points31, primitive, PaintValuesSet::Layer_3,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN_3, &road);

        drawLineLayer(canvas, paint, path, context, enlargedArea31, points31, primitive, PaintValuesSet::Layer_4,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN_4, &road);

        drawLineLayer(canvas, paint, path, context, enlargedArea31, points31, primitive, PaintValuesSet::Layer_5,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN_5, &road);

        // Markings wait until all roads of this level are drawn, so that joined roads do not cover them
        if (road.valid && road.laneWidth >= RealisticRoadsMinLaneMarkingWidth)
        {
            if (context.pendingMarkings.isEmpty())
                context.pendingMarkingsLayer = static_cast<int>(mapObject->getLayerType());
            context.pendingMarkings.push_back(primitive);
        }

        rasterizePolylineIcons(context, canvas, path, primitive->evaluationResult);
    }

    if (areaIndex >= 0)
        mapObject->stopReadingArea(areaIndex);
}

void OsmAnd::MapRasterizer_P::rasterizePolylineShadow(
    const Context& context,
    SkCanvas& canvas,
    const SkPath& path,
    SkPaint& paint,
    const ColorARGB shadowColor,
    const float shadowRadius)
{
    if (context.shadowMode == MapPresentationEnvironment::ShadowMode::BlurShadow && shadowRadius > 0.0f)
    {
        // TODO: Loopers are long-gone in skia
/*
        // simply draw shadow? difference from option 3 ?
        paint.setLooper(SkBlurDrawLooper::Create(
            shadowColor.toSkColor(),
            SkBlurMaskFilter::ConvertRadiusToSigma(shadowRadius),
            0,
            0))->unref();
*/
        canvas.drawPath(path, paint);
    }
    else if (context.shadowMode == MapPresentationEnvironment::ShadowMode::SolidShadow && shadowRadius > 0.0f)
    {
        paint.setStrokeWidth(paint.getStrokeWidth() + shadowRadius * 2);
        paint.setColorFilter(SkColorFilters::Blend(
            shadowColor.toSkColor(),
            SkBlendMode::kSrcIn
        ));
        canvas.drawPath(path, paint);
    }
}

void OsmAnd::MapRasterizer_P::rasterizePolylineIcons(
    const Context& context,
    SkCanvas& canvas,
    const SkPath& path,
    const MapStyleEvaluationResult::Packed& evalResult)
{
    bool ok;

    QString pathIconName;
    ok = evalResult.getStringValue(context.env->styleBuiltinValueDefs->id_OUTPUT_PATH_ICON, pathIconName);
    if (!ok || pathIconName.isEmpty())
        return;

    float pathIconStep = 0.0f;
    ok = evalResult.getFloatValue(context.env->styleBuiltinValueDefs->id_OUTPUT_PATH_ICON_STEP, pathIconStep);
    if (!ok || pathIconStep <= 0.0f)
        return;

    const auto legacyScale = context.env->displayDensityFactor > 1.0f
        ? 2.0f / 3.0f
        : 1.0f;
    const auto iconScale = context.env->mapScaleFactor * legacyScale;

    sk_sp<const SkImage> pathIcon;
    ok = context.env->obtainIcon(pathIconName, iconScale, pathIcon);
    if (!ok || !pathIcon)
        return;

    SkMatrix mIconTransform;
    mIconTransform.setIdentity();
    mIconTransform.setTranslate(-0.5f * pathIcon->width(), -0.5f * pathIcon->height());
    mIconTransform.postRotate(90.0f);

    SkPathMeasure pathMeasure(path, false);

    const auto length = pathMeasure.getLength();
    auto iconOffset = 0.5f * pathIconStep;
    const auto iconInstancesCount = static_cast<int>((length - iconOffset) / pathIconStep) + 1;
    if (iconInstancesCount < 1)
        return;

    SkMatrix mIconInstanceTransform;
    for (auto iconInstanceIdx = 0; iconInstanceIdx < iconInstancesCount; iconInstanceIdx++, iconOffset += pathIconStep)
    {
        SkMatrix mPinPoint;
        ok = pathMeasure.getMatrix(iconOffset, &mPinPoint);
        if (!ok)
            break;

        mIconInstanceTransform.setConcat(mPinPoint, mIconTransform);
        canvas.save();
        canvas.concat(mIconInstanceTransform);
        canvas.drawImage(pathIcon.get(), 0, 0);
        canvas.restore();
    }
}

float OsmAnd::MapRasterizer_P::lineEquation(float x1, float y1, float x2, float y2, float x) const
{
    if(x2 == x1)
        return y1;
    return (x - x1) / (x2 - x1) * (y2 - y1) + y1;
}

void OsmAnd::MapRasterizer_P::simplifyVertexToDirection(
    const Context& context,
    const PointF& vertex,
    const PointF& vertexTo,
    PointF& res) const
{
    const auto xShiftForSpacing = context.pixelArea.width() / 4;
    const auto yShiftForSpacing = context.pixelArea.height() / 4;

    if (vertex.x > context.pixelArea.right() + xShiftForSpacing )
    {
        res.x = context.pixelArea.right() + xShiftForSpacing;
        res.y = lineEquation(vertex.x, vertex.y, vertexTo.x, vertexTo.y, res.x);
    }
    else if (vertex.x < context.pixelArea.left() - xShiftForSpacing)
    {
        res.x = context.pixelArea.left() - xShiftForSpacing;
        res.y = lineEquation(vertex.x, vertex.y, vertexTo.x, vertexTo.y, res.x);
    }
    else if (vertex.y > context.pixelArea.bottom() + yShiftForSpacing)
    {
        res.y = context.pixelArea.bottom() + yShiftForSpacing;
        res.x = lineEquation(vertex.y, vertex.x, vertexTo.y, vertexTo.x, res.y);
    }
    else if (vertex.y < context.pixelArea.top() - yShiftForSpacing)
    {
        res.y = context.pixelArea.top() - yShiftForSpacing;
        res.x = lineEquation(vertex.y, vertex.x, vertexTo.y, vertexTo.x, res.y);
    }
    else
    {
        res.x = vertex.x;
        res.y = vertex.y;
    }
}

void OsmAnd::MapRasterizer_P::calculateVertex(const Context& context, const PointI& point31, PointF& vertex) const
{
    vertex.x = static_cast<float>(point31.x - context.area31.left()) / context.primitivisedObjects->scaleDivisor31ToPixel.x;
    vertex.y = static_cast<float>(point31.y - context.area31.top()) / context.primitivisedObjects->scaleDivisor31ToPixel.y;

    vertex += PointF(context.pixelArea.topLeft);
}

bool OsmAnd::MapRasterizer_P::obtainPathEffect(const QString& encodedPathEffect, sk_sp<SkPathEffect> &outPathEffect) const
{
    QMutexLocker scopedLocker(&_pathEffectsMutex);

    auto itPathEffects = _pathEffects.constFind(encodedPathEffect);
    if (itPathEffects == _pathEffects.cend())
    {
        const auto& strIntervals = encodedPathEffect.split(QLatin1Char('_'), QString::SkipEmptyParts);
        const auto intervalsCount = strIntervals.size();

        const auto intervals = new SkScalar[intervalsCount];
        auto pInterval = intervals;
        for (const auto& strInterval : constOf(strIntervals))
        {
            float computedValue = 0.0f;

            if (!strInterval.contains(QLatin1Char(':')))
            {
                computedValue = strInterval.toFloat()*owner->mapPresentationEnvironment->displayDensityFactor;
            }
            else
            {
                // "pt:px" format
                const auto& complexValue = strInterval.split(QLatin1Char(':'), QString::KeepEmptyParts);

                computedValue = complexValue[0].toFloat()*owner->mapPresentationEnvironment->displayDensityFactor + complexValue[1].toFloat();
            }

            *(pInterval++) = computedValue;
        }

        // Validate
        if (intervalsCount < 2 || intervalsCount % 2 != 0)
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Path effect (%s) with %d intervals is invalid",
                qPrintable(encodedPathEffect),
                intervalsCount);
            return false;
        }

        const auto pathEffect = SkDashPathEffect::Make(intervals, intervalsCount, 0);
        delete[] intervals;

        itPathEffects = _pathEffects.insert(encodedPathEffect, pathEffect);
    }

    outPathEffect = *itPathEffects;
    return true;
}

bool OsmAnd::MapRasterizer_P::obtainImageShader(
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const QString& name,
    sk_sp<SkShader> &outShader)
{
    sk_sp<const SkImage> image;
    if (!env->obtainIcon(name, 1.0f, image))
    {
        LogPrintf(LogSeverityLevel::Warning,
            "Failed to get '%s' shader image",
            qPrintable(name));

        return false;
    }

    outShader = image->makeShader(SkTileMode::kRepeat, SkTileMode::kRepeat, {});
    return true;
}

OsmAnd::MapRasterizer_P::Context::Context(
    const AreaI area31_,
    const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects_,
    const AreaI pixelArea_)
    : area31(area31_)
    , primitivisedObjects(primitivisedObjects_)
    , env(primitivisedObjects->mapPresentationEnvironment)
    , zoom(primitivisedObjects->zoom)
    , pixelArea(pixelArea_)
    , realisticRoads(false)
    , pixelsPerMeter(0.0f)
{
    env->obtainShadowOptions(zoom, shadowMode, shadowColor);

    if (zoom >= RealisticRoadsMinZoom)
    {
        const auto valueDefId = env->mapStyle->getValueDefinitionIdByName(QStringLiteral("realisticRoads"));
        if (valueDefId >= 0)
        {
            const auto settings = env->getSettings();
            const auto citSetting = settings.constFind(valueDefId);
            realisticRoads = citSetting != settings.cend() && citSetting->asSimple.asInt != 0;
        }
    }
    if (realisticRoads)
    {
        const auto latitude = Utilities::get31LatitudeY(area31.center().y);
        const auto metersPer31 = 40075016.686 * qCos(qDegreesToRadians(latitude)) / static_cast<double>(1u << 31);
        pixelsPerMeter = static_cast<float>(1.0 / (metersPer31 * primitivisedObjects->scaleDivisor31ToPixel.x));
    }
}

namespace
{
    QString getTextAttribute(const std::shared_ptr<const OsmAnd::MapObject>& mapObject, const QString& tag)
    {
        const QString empty;
        const auto& encodeMap = mapObject->attributeMapping->encodeMap;
        const auto citGroup = encodeMap.constFind(QStringRef(&tag));
        if (citGroup == encodeMap.cend())
            return QString();
        const auto citId = citGroup->constFind(QStringRef(&empty));
        if (citId != citGroup->cend())
        {
            const auto citCaption = mapObject->captions.constFind(*citId);
            if (citCaption != mapObject->captions.cend())
                return *citCaption;
        }
        return mapObject->getResolvedAttribute(QStringRef(&tag));
    }

    int getIntAttribute(const std::shared_ptr<const OsmAnd::MapObject>& mapObject, const QString& tag)
    {
        bool ok = false;
        const auto value = mapObject->getResolvedAttribute(QStringRef(&tag)).trimmed().toInt(&ok);
        return ok && value > 0 && value <= 12 ? value : 0;
    }

    QStringList splitLanes(const QString& value, const int lanes)
    {
        if (value.isEmpty())
            return QStringList();
        auto result = value.split(QLatin1Char('|'), QString::KeepEmptyParts);
        return result.size() == lanes ? result : QStringList();
    }

    bool forbidsChange(const QString& change, const bool toRight)
    {
        if (change == QLatin1String("no"))
            return true;
        if (toRight)
            return change == QLatin1String("not_right") || change == QLatin1String("only_left");
        return change == QLatin1String("not_left") || change == QLatin1String("only_right");
    }

    bool isMerge(const QString& turn)
    {
        return turn.contains(QLatin1String("merge_to"));
    }

    bool isBusLane(const QString& access)
    {
        return access == QLatin1String("designated") || access == QLatin1String("yes");
    }
}

bool OsmAnd::MapRasterizer_P::computeRoadLayout(
    const std::shared_ptr<const MapObject>& mapObject,
    const QString& value,
    RoadLayout& outLayout)
{
    outLayout = RoadLayout();

    // Default lane count of a two-way road and lane width in meters
    int defaultLanes;
    float laneWidth;
    float shoulders = 0.0f;
    const bool isLink = value.endsWith(QLatin1String("_link"));
    const bool isMotorway = value.startsWith(QLatin1String("motorway"));
    const bool isTrunk = value.startsWith(QLatin1String("trunk"));
    if (isMotorway || isTrunk)
    {
        defaultLanes = isLink ? 1 : 2;
        laneWidth = 3.5f;
        shoulders = 1.0f;
    }
    else if (value.startsWith(QLatin1String("primary")) || value.startsWith(QLatin1String("secondary")))
    {
        defaultLanes = isLink ? 1 : 2;
        laneWidth = 3.3f;
    }
    else if (value.startsWith(QLatin1String("tertiary")))
    {
        defaultLanes = isLink ? 1 : 2;
        laneWidth = 3.1f;
    }
    else if (value == QLatin1String("unclassified") || value == QLatin1String("residential"))
    {
        defaultLanes = 2;
        laneWidth = 2.9f;
    }
    else if (value == QLatin1String("living_street") || value == QLatin1String("road"))
    {
        defaultLanes = 1;
        laneWidth = 4.5f;
    }
    else if (value == QLatin1String("service") || value == QLatin1String("busway"))
    {
        defaultLanes = 1;
        laneWidth = 3.2f;
    }
    else
        return false;

    const bool onewayForward = mapObject->containsAttribute(QStringLiteral("oneway"), QStringLiteral("yes"), true)
        || ((isMotorway || mapObject->containsAttribute(QStringLiteral("junction"), QStringLiteral("roundabout"), true))
            && !mapObject->containsAttribute(QStringLiteral("oneway"), QStringLiteral("no"), true));
    const bool onewayBackward = mapObject->containsAttribute(QStringLiteral("oneway"), QStringLiteral("-1"), true);

    const auto lanes = getIntAttribute(mapObject, QStringLiteral("lanes"));
    int forward = 0;
    int backward = 0;
    if (onewayForward || onewayBackward)
    {
        auto count = lanes;
        if (count == 0)
        {
            count = (isMotorway || isTrunk || value.startsWith(QLatin1String("primary"))
                || value.startsWith(QLatin1String("secondary"))) && !isLink ? 2 : 1;
            if (defaultLanes == 2 && count == 1)
                laneWidth = qMax(laneWidth, 3.5f);
        }
        const auto turn = splitLanes(getTextAttribute(mapObject, QStringLiteral("turn:lanes")), count);
        const auto change = splitLanes(getTextAttribute(mapObject, QStringLiteral("change:lanes")), count);
        auto bus = splitLanes(getTextAttribute(mapObject, QStringLiteral("bus:lanes")), count);
        if (bus.isEmpty())
            bus = splitLanes(getTextAttribute(mapObject, QStringLiteral("psv:lanes")), count);
        if (onewayForward)
        {
            forward = count;
            outLayout.turnForward = turn;
            outLayout.changeForward = change;
            outLayout.busForward = bus;
        }
        else
        {
            backward = count;
            outLayout.turnBackward = turn;
            outLayout.changeBackward = change;
            outLayout.busBackward = bus;
        }
    }
    else
    {
        forward = getIntAttribute(mapObject, QStringLiteral("lanes:forward"));
        backward = getIntAttribute(mapObject, QStringLiteral("lanes:backward"));
        if (forward == 0 && backward == 0)
        {
            const auto total = lanes > 0 ? lanes : defaultLanes;
            if (total == 1)
            {
                // Narrow road without markings
                forward = 1;
            }
            else
            {
                backward = total / 2;
                forward = total - backward;
            }
        }
        else if (lanes > 0)
        {
            if (forward == 0)
                forward = qMax(0, lanes - backward);
            if (backward == 0)
                backward = qMax(0, lanes - forward);
        }
        outLayout.turnForward = splitLanes(getTextAttribute(mapObject, QStringLiteral("turn:lanes:forward")), forward);
        outLayout.turnBackward = splitLanes(getTextAttribute(mapObject, QStringLiteral("turn:lanes:backward")), backward);
        outLayout.changeForward = splitLanes(getTextAttribute(mapObject, QStringLiteral("change:lanes:forward")), forward);
        outLayout.changeBackward = splitLanes(getTextAttribute(mapObject, QStringLiteral("change:lanes:backward")), backward);
    }
    const auto totalLanes = forward + backward;
    if (totalLanes <= 0)
        return false;

    float width = totalLanes * laneWidth + shoulders;
    const QString widthTag(QStringLiteral("width"));
    auto widthValue = mapObject->getResolvedAttribute(QStringRef(&widthTag));
    widthValue.remove(QLatin1Char('m'));
    bool ok = false;
    const auto taggedWidth = widthValue.trimmed().toFloat(&ok);
    if (ok && taggedWidth >= 2.0f && taggedWidth <= 60.0f)
    {
        width = taggedWidth;
        laneWidth = (width - shoulders) / totalLanes;
    }

    // Placement of the way line on a one-way carriageway, see OSM key "placement"
    if (forward == 0 || backward == 0)
    {
        const QString placementTag(QStringLiteral("placement"));
        const auto placement = mapObject->getResolvedAttribute(QStringRef(&placementTag));
        const auto parts = placement.split(QLatin1Char(':'));
        bool okLane = false;
        const auto lane = parts.size() == 2 ? parts[1].toInt(&okLane) : 0;
        if (okLane && lane >= 1 && lane <= totalLanes)
        {
            // Position of the way line from the left edge in the driving direction
            float line = -1.0f;
            if (parts[0] == QLatin1String("left_of"))
                line = (lane - 1) * laneWidth;
            else if (parts[0] == QLatin1String("middle_of"))
                line = (lane - 0.5f) * laneWidth;
            else if (parts[0] == QLatin1String("right_of"))
                line = lane * laneWidth;
            if (line >= 0.0f)
            {
                outLayout.hasPlacement = true;
                outLayout.shift = totalLanes * laneWidth / 2.0f - line;
                if (backward > 0)
                    outLayout.shift = -outLayout.shift;
            }
        }
    }

    outLayout.valid = true;
    outLayout.buttCaps = isMotorway || isTrunk;
    outLayout.edgeLines = isMotorway || isTrunk;
    outLayout.width = width;
    outLayout.laneWidth = laneWidth;
    outLayout.lanesForward = forward;
    outLayout.lanesBackward = backward;
    outLayout.shiftStart = outLayout.shift;
    outLayout.shiftEnd = outLayout.shift;
    return true;
}

void OsmAnd::MapRasterizer_P::resolveRoadTransitions(Context& context)
{
    // Ends of one-way roads, in the driving direction
    struct End
    {
        const MapObject* object;
        RoadLayout* layout;
        bool isTravelStart;   // the driving starts at this node
        PointD direction;     // driving direction at the node, y down
        int lanes;
        bool atWayStart;      // the node is the first point of the way
        bool reversed;        // the lanes go against the way direction
    };
    QHash<quint64, QVector<End>> ends;
    // The same way may come from several data blocks: resolve one copy, then copy the result
    QHash<QString, RoadLayout*> uniqueLayouts;
    QList<QPair<RoadLayout*, RoadLayout*>> duplicates;
    for (auto it = context.roadLayouts.begin(); it != context.roadLayouts.end(); ++it)
    {
        auto& layout = it.value();
        if (!layout.valid || (layout.lanesForward > 0 && layout.lanesBackward > 0))
            continue;
        const auto& points = it.key()->points31;
        const auto count = points.size();
        if (count < 2)
            continue;
        const auto geometryKey = QString::fromLatin1("%1,%2,%3,%4,%5")
            .arg(points[0].x).arg(points[0].y).arg(points[count - 1].x).arg(points[count - 1].y).arg(count);
        const auto citUnique = uniqueLayouts.constFind(geometryKey);
        if (citUnique != uniqueLayouts.cend())
        {
            duplicates.push_back(qMakePair(*citUnique, &layout));
            continue;
        }
        uniqueLayouts.insert(geometryKey, &layout);
        const bool reversed = layout.lanesBackward > 0;
        const auto lanes = layout.lanesForward + layout.lanesBackward;
        const auto key = [](const PointI& p) { return (static_cast<quint64>(static_cast<quint32>(p.x)) << 32) | static_cast<quint32>(p.y); };
        const auto dir = [](const PointI& from, const PointI& to)
            {
                PointD d(static_cast<double>(to.x) - from.x, static_cast<double>(to.y) - from.y);
                const auto length = std::sqrt(d.x * d.x + d.y * d.y);
                return length > 0.0 ? PointD(d.x / length, d.y / length) : PointD(1.0, 0.0);
            };
        // First point of the way
        ends[key(points[0])].push_back({ it.key(), &layout, !reversed,
            reversed ? dir(points[1], points[0]) : dir(points[0], points[1]), lanes, true, reversed });
        // Last point of the way
        ends[key(points[count - 1])].push_back({ it.key(), &layout, reversed,
            reversed ? dir(points[count - 1], points[count - 2]) : dir(points[count - 2], points[count - 1]), lanes, false, reversed });
    }

    // Everything below is in the driving frame: offsets to the right of the driving direction, lanes from the left
    const auto rightOf = [](const PointD& d) { return PointD(-d.y, d.x); };
    const auto dot = [](const PointD& a, const PointD& b) { return a.x * b.x + a.y * b.y; };
    const auto awayOf = [](const End& e) { return e.isTravelStart ? e.direction : PointD(-e.direction.x, -e.direction.y); };
    const auto travelShift = [](const End& e) { return e.reversed ? -e.layout->shift : e.layout->shift; };
    const auto lanesWidth = [](const End& e) { return e.lanes * e.layout->laneWidth; };
    const auto turnsOf = [](const End& e) -> const QStringList& { return e.reversed ? e.layout->turnBackward : e.layout->turnForward; };
    const auto setShift =
        [](const End& e, const float shift)
        {
            const auto wayShift = e.reversed ? -shift : shift;
            if (e.atWayStart)
                e.layout->shiftStart = wayShift;
            else
                e.layout->shiftEnd = wayShift;
        };
    const auto setTaper =
        [](const End& e, const bool onLeft, const int lanes)
        {
            const bool wayLeft = e.reversed ? !onLeft : onLeft;
            auto& taper = e.atWayStart
                ? (wayLeft ? e.layout->taperFirstLeft : e.layout->taperFirstRight)
                : (wayLeft ? e.layout->taperLastLeft : e.layout->taperLastRight);
            taper = lanes;
        };
    const auto setNormal =
        [](const End& e, const PointD& normal)
        {
            const auto wayNormal = e.reversed ? PointD(-normal.x, -normal.y) : normal;
            if (e.atWayStart)
            {
                e.layout->hasFirstNormal = true;
                e.layout->firstNormal = wayNormal;
            }
            else
            {
                e.layout->hasLastNormal = true;
                e.layout->lastNormal = wayNormal;
            }
        };

    for (auto itNode = ends.cbegin(); itNode != ends.cend(); ++itNode)
    {
        QVector<const End*> ins;
        QVector<const End*> outs;
        for (const auto& end : constOf(itNode.value()))
            (end.isTravelStart ? outs : ins).push_back(&end);

        // Only roads that go on nearly straight take part: at junctions roads meet at an angle and keep
        // their own geometry
        if (ins.size() == 1 && outs.size() >= 2)
        {
            const auto& parent = *ins.first();
            outs.erase(std::remove_if(outs.begin(), outs.end(),
                [&](const End* e) { return dot(e->direction, parent.direction) < RealisticRoadsMaxJoinCos; }), outs.end());
        }
        else if (ins.size() >= 2 && outs.size() == 1)
        {
            const auto& parent = *outs.first();
            ins.erase(std::remove_if(ins.begin(), ins.end(),
                [&](const End* e) { return dot(e->direction, parent.direction) < RealisticRoadsMaxJoinCos; }), ins.end());
        }
        else if (ins.size() == 1 && outs.size() == 1 && dot(ins.first()->direction, outs.first()->direction) < RealisticRoadsMaxJoinCos)
            continue;

        if (ins.size() == 1 && outs.size() == 1)
        {
            // One road continues another
            const auto& a = *ins.first();
            const auto& b = *outs.first();

            // Both are cut along the bisector of the corner
            auto normal = rightOf(a.direction) + rightOf(b.direction);
            const auto normalLength = std::sqrt(dot(normal, normal));
            normal = normalLength > 0.1 ? PointD(normal.x / normalLength, normal.y / normalLength) : rightOf(b.direction);
            setNormal(a, normal);
            setNormal(b, normal);

            // Side where the edges of both carriageways stay in line, lanes are added or dropped on the other one
            const auto& wider = a.lanes >= b.lanes ? a : b;
            bool alignLeft = true;
            if (a.layout->hasPlacement && b.layout->hasPlacement)
            {
                const auto leftA = travelShift(a) - lanesWidth(a) / 2.0f;
                const auto leftB = travelShift(b) - lanesWidth(b) / 2.0f;
                alignLeft = qAbs(leftA - leftB) <= qAbs(leftA + lanesWidth(a) - leftB - lanesWidth(b));
            }
            else
            {
                const auto& turns = turnsOf(wider);
                if (!turns.isEmpty() && turns.first().contains(QLatin1String("merge_to_right"))
                    && !turns.last().contains(QLatin1String("merge_to_left")))
                {
                    alignLeft = false;
                }
            }

            // The road without placement follows the other one, or the narrower one follows the wider one;
            // with placement on both, the next road absorbs a small mismatch of the mapped lines
            const End* anchor = nullptr;
            const End* follower = nullptr;
            if (a.layout->hasPlacement && b.layout->hasPlacement)
            {
                anchor = &a;
                follower = &b;
            }
            else if (a.layout->hasPlacement)
            {
                anchor = &a;
                follower = &b;
            }
            else if (b.layout->hasPlacement)
            {
                anchor = &b;
                follower = &a;
            }
            else if (a.lanes != b.lanes)
            {
                anchor = &wider;
                follower = &wider == &a ? &b : &a;
            }
            if (anchor)
            {
                const auto anchorLeft = travelShift(*anchor) - lanesWidth(*anchor) / 2.0f;
                setShift(*follower, alignLeft
                    ? anchorLeft + lanesWidth(*follower) / 2.0f
                    : anchorLeft + lanesWidth(*anchor) - lanesWidth(*follower) / 2.0f);
            }
            if (a.lanes != b.lanes)
                setTaper(wider, !alignLeft, qAbs(a.lanes - b.lanes));

            // Turn arrows are painted once, before the road ends or changes
            if (!turnsOf(a).isEmpty() && turnsOf(a) == turnsOf(b))
                a.layout->noArrows = true;
        }
        else if ((ins.size() == 1 && outs.size() >= 2) || (ins.size() >= 2 && outs.size() == 1))
        {
            // Split or merge
            const bool isSplit = ins.size() == 1;
            const auto& parent = isSplit ? *ins.first() : *outs.first();
            auto branches = isSplit ? outs : ins;
            const auto parentRight = rightOf(parent.direction);
            std::sort(branches.begin(), branches.end(),
                [&](const End* l, const End* r)
                {
                    return dot(awayOf(*l), parentRight) < dot(awayOf(*r), parentRight);
                });

            // All ends are cut straight across the parent
            setNormal(parent, parentRight);
            int branchLanes = 0;
            for (const auto branch : constOf(branches))
            {
                setNormal(*branch, parentRight);
                branchLanes += branch->lanes;
            }

            // Lanes beyond the parent ones open (split) or end (merge) on the right edge
            const auto excess = branchLanes - parent.lanes;
            if (excess > 0)
                setTaper(*branches.last(), false, qMin(excess, branches.last()->lanes));
            else if (excess < 0)
                setTaper(parent, false, -excess);

            // Every branch takes its block of the parent lanes at the node, a mapped placement is reached further on
            const auto parentLeft = travelShift(parent) - lanesWidth(parent) / 2.0f;
            int blockStart = 0;
            for (const auto branch : constOf(branches))
            {
                setShift(*branch, parentLeft + blockStart * parent.layout->laneWidth + lanesWidth(*branch) / 2.0f);
                blockStart += branch->lanes;
            }

            // Gore areas between neighbouring branches of motorways and trunk roads
            for (int i = 1; i < branches.size(); i++)
            {
                if (!branches[i]->layout->edgeLines || !branches[i - 1]->layout->edgeLines)
                    continue;
                if (branches[i]->atWayStart)
                    branches[i]->layout->goreFirst = branches[i - 1]->object;
                else
                    branches[i]->layout->goreLast = branches[i - 1]->object;
            }
        }
    }

    for (const auto& duplicate : constOf(duplicates))
        *duplicate.second = *duplicate.first;
}

void OsmAnd::MapRasterizer_P::computeRealisticRoad(
    const Context& context,
    const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive,
    RealisticRoad& outRoad) const
{
    outRoad.valid = false;
    if (!context.realisticRoads)
        return;

    const auto citLayout = context.roadLayouts.constFind(primitive->sourceObject.get());
    if (citLayout == context.roadLayouts.cend() || !citLayout->valid)
        return;
    const auto& layout = *citLayout;

    float styleWidth = 0.0f;
    if (!primitive->evaluationResult.getFloatValue(
            context.env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH, styleWidth) || styleWidth <= 0.0f)
    {
        return;
    }
    styleWidth *= primitive->detailScaleFactor;

    // Real width only, so tiles of different zoom levels agree in a perspective view
    const auto m = context.pixelsPerMeter;
    const auto realWidth = layout.width * m;
    if (realWidth < 1.0f)
        return;
    if (!computeRoadGeometry(context, *primitive->sourceObject, layout, outRoad.geometry))
        return;

    outRoad.valid = true;
    outRoad.layout = &layout;
    outRoad.styleWidth = styleWidth;
    outRoad.width = realWidth;
    outRoad.laneWidth = layout.laneWidth * m;
    outRoad.buttCaps = layout.buttCaps;
    outRoad.polygonBody = layout.buttCaps || outRoad.geometry.tapered
        || outRoad.geometry.hasFirstNormal || outRoad.geometry.hasLastNormal;
}

bool OsmAnd::MapRasterizer_P::computeRoadGeometry(
    const Context& context,
    const MapObject& mapObject,
    const RoadLayout& layout,
    RoadGeometry& outGeometry) const
{
    auto& g = outGeometry;
    g = RoadGeometry();
    const auto m = context.pixelsPerMeter;
    QVector<PointF> vertices;
    getPixelVertices(context, mapObject.points31, vertices);
    if (vertices.size() < 2)
        return false;
    QVector<float> distances(vertices.size(), 0.0f);
    for (int i = 1; i < vertices.size(); i++)
    {
        const auto d = vertices[i] - vertices[i - 1];
        distances[i] = distances[i - 1] + std::sqrt(d.x * d.x + d.y * d.y);
    }
    const auto total = distances.last();

    // Widths and shifts change near the ends: add vertices there, so that the changes follow their curve
    const auto zone = qMax(RealisticRoadsTaperLength, RealisticRoadsLaneTaperLength) * m;
    const auto step = qMax(3.0f, 2.5f * m);
    g.vertices.reserve(vertices.size());
    g.distances.reserve(vertices.size());
    for (int i = 0; i < vertices.size(); i++)
    {
        if (i > 0)
        {
            const auto from = distances[i - 1];
            const auto length = distances[i] - from;
            const bool nearEnds = total <= 2.0f * zone || from < zone || distances[i] > total - zone;
            const auto parts = nearEnds ? qMin(1000, static_cast<int>(length / step)) : 0;
            for (int part = 1; part < parts; part++)
            {
                const auto t = static_cast<float>(part) / parts;
                g.vertices.push_back(vertices[i - 1] + (vertices[i] - vertices[i - 1]) * t);
                g.distances.push_back(from + length * t);
            }
        }
        g.vertices.push_back(vertices[i]);
        g.distances.push_back(distances[i]);
    }
    const auto count = g.vertices.size();

    const auto lanes = layout.lanesForward + layout.lanesBackward;
    const auto laneWidth = layout.laneWidth * m;
    const auto shoulder = qMax(0.0f, (layout.width - lanes * layout.laneWidth) * m / 2.0f);
    const auto shifts = interpolateShifts(g.distances, layout.shiftStart * m, layout.shiftEnd * m, RealisticRoadsTaperLength * m);

    // Lanes that open or end at the ends of the way
    g.laneFactors = QVector<QVector<float>>(lanes, QVector<float>(count, 1.0f));
    g.tapered = layout.taperFirstLeft + layout.taperFirstRight + layout.taperLastLeft + layout.taperLastRight > 0;
    const auto taperLength = qMin(RealisticRoadsLaneTaperLength * m, total * 0.5f);
    if (g.tapered && taperLength > 0.0f)
    {
        const auto smoothstep = [](const float t) { return t * t * (3.0f - 2.0f * t); };
        for (int lane = 0; lane < lanes; lane++)
        {
            const bool atFirst = lane < layout.taperFirstLeft || lane >= lanes - layout.taperFirstRight;
            const bool atLast = lane < layout.taperLastLeft || lane >= lanes - layout.taperLastRight;
            if (!atFirst && !atLast)
                continue;
            auto& factors = g.laneFactors[lane];
            for (int i = 0; i < count; i++)
            {
                if (atFirst)
                    factors[i] *= smoothstep(qMin(1.0f, g.distances[i] / taperLength));
                if (atLast)
                    factors[i] *= smoothstep(qMin(1.0f, (total - g.distances[i]) / taperLength));
            }
        }
    }

    // Narrow lanes on the left move the left edge, the others move the right one
    const auto leftTapered = qMax(layout.taperFirstLeft, layout.taperLastLeft);
    g.boundaries = QVector<QVector<float>>(lanes + 1, QVector<float>(count));
    g.bodyLeft.resize(count);
    g.bodyRight.resize(count);
    for (int i = 0; i < count; i++)
    {
        auto left = shifts[i] - lanes * laneWidth / 2.0f;
        for (int lane = 0; lane < leftTapered && lane < lanes; lane++)
            left += laneWidth * (1.0f - g.laneFactors[lane][i]);
        g.boundaries[0][i] = left;
        for (int lane = 0; lane < lanes; lane++)
            g.boundaries[lane + 1][i] = g.boundaries[lane][i] + laneWidth * g.laneFactors[lane][i];
        g.bodyLeft[i] = g.boundaries[0][i] - shoulder;
        g.bodyRight[i] = g.boundaries[lanes][i] + shoulder;
    }

    g.blendLength = RealisticRoadsJoinBlendLength * m;
    g.hasFirstNormal = layout.hasFirstNormal;
    g.hasLastNormal = layout.hasLastNormal;
    g.firstNormal = PointF(static_cast<float>(layout.firstNormal.x), static_cast<float>(layout.firstNormal.y));
    g.lastNormal = PointF(static_cast<float>(layout.lastNormal.x), static_cast<float>(layout.lastNormal.y));
    return true;
}

void OsmAnd::MapRasterizer_P::getPixelVertices(
    const Context& context,
    const QVector<PointI>& points31,
    QVector<PointF>& outVertices) const
{
    outVertices.clear();
    outVertices.reserve(points31.size());
    PointF vertex;
    for (const auto& point : constOf(points31))
    {
        calculateVertex(context, point, vertex);
        if (outVertices.isEmpty() || qAbs(outVertices.last().x - vertex.x) + qAbs(outVertices.last().y - vertex.y) > 0.01f)
            outVertices.push_back(vertex);
    }
}

QVector<float> OsmAnd::MapRasterizer_P::interpolateShifts(
    const QVector<float>& distances,
    const float start,
    const float end,
    const float taperLength)
{
    const auto count = distances.size();
    QVector<float> result(count, start);
    if (count < 2 || qAbs(end - start) < 0.01f)
        return result;
    const auto total = distances.last();
    const auto smoothstep = [](const float t) { return t * t * (3.0f - 2.0f * t); };
    for (int i = 0; i < count; i++)
    {
        if (total <= 2.0f * taperLength)
        {
            // Smooth taper between the two ends
            const auto t = total > 0.0f ? distances[i] / total : 0.0f;
            result[i] = start + (end - start) * smoothstep(t);
        }
        else
        {
            // Long road: each end tapers into the middle within its own reach, so that a tile only
            // depends on the roads joined within the neighbours area
            const auto fromStart = qMin(1.0f, distances[i] / taperLength);
            const auto fromEnd = qMin(1.0f, (total - distances[i]) / taperLength);
            result[i] = start * (1.0f - smoothstep(fromStart)) + end * (1.0f - smoothstep(fromEnd));
        }
    }
    return result;
}

QVector<OsmAnd::PointF> OsmAnd::MapRasterizer_P::offsetPoints(const RoadGeometry& geometry, const QVector<float>& offsets)
{
    // Offsets are to the right of the direction of the vertices (screen coordinates, y down)
    const auto& vertices = geometry.vertices;
    const auto count = vertices.size();
    QVector<PointF> result;
    if (count < 2 || offsets.size() != count)
        return result;
    result.resize(count);
    const auto segmentNormal =
        [&vertices](const int from) -> PointF
        {
            const auto d = vertices[from + 1] - vertices[from];
            const auto length = std::sqrt(d.x * d.x + d.y * d.y);
            if (length <= 0.0f)
                return PointF(0.0f, 0.0f);
            return PointF(-d.y / length, d.x / length);
        };
    const auto smoothstep = [](const float t) { return t * t * (3.0f - 2.0f * t); };
    const auto total = geometry.distances.isEmpty() ? 0.0f : geometry.distances.last();
    for (int i = 0; i < count; i++)
    {
        PointF normal;
        if (i == 0 || i == count - 1)
            normal = segmentNormal(i == 0 ? 0 : count - 2);
        else
        {
            const auto n1 = segmentNormal(i - 1);
            const auto n2 = segmentNormal(i);
            auto n = n1 + n2;
            const auto length = std::sqrt(n.x * n.x + n.y * n.y);
            if (length < 0.1f)
                normal = n2;
            else
            {
                // Miter join, limited on sharp corners
                n = n / length;
                const auto cosHalf = qMax(0.5f, n.x * n2.x + n.y * n2.y);
                normal = n / cosHalf;
            }
        }

        // A joined end lies on the cut line shared with the other road, and turns to the own direction
        // of the road within the blend length
        if (geometry.blendLength > 0.0f && geometry.distances.size() == count)
        {
            const auto blendAt =
                [&](const PointF& cut, const float distance)
                {
                    if (distance >= geometry.blendLength)
                        return;
                    const auto scale = std::sqrt(normal.x * normal.x + normal.y * normal.y);
                    if (scale <= 0.0f)
                        return;
                    const auto t = smoothstep(distance / geometry.blendLength);
                    auto blended = cut * (1.0f - t) + normal / scale * t;
                    const auto length = std::sqrt(blended.x * blended.x + blended.y * blended.y);
                    if (length > 0.1f)
                        normal = blended / length * (1.0f + (scale - 1.0f) * t);
                };
            if (geometry.hasFirstNormal)
                blendAt(geometry.firstNormal, geometry.distances[i]);
            if (geometry.hasLastNormal)
                blendAt(geometry.lastNormal, total - geometry.distances[i]);
        }
        result[i] = vertices[i] + normal * offsets[i];
    }
    return result;
}

SkPath OsmAnd::MapRasterizer_P::buildOffsetPath(const RoadGeometry& geometry, const QVector<float>& offsets)
{
    SkPath path;
    const auto points = offsetPoints(geometry, offsets);
    for (int i = 0; i < points.size(); i++)
    {
        if (i == 0)
            path.moveTo(points[i].x, points[i].y);
        else
            path.lineTo(points[i].x, points[i].y);
    }
    return path;
}

void OsmAnd::MapRasterizer_P::drawRoadBody(
    SkCanvas& canvas,
    SkPaint& paint,
    const SkPath& path,
    const RealisticRoad& road) const
{
    if (!road.polygonBody || paint.getPathEffect())
    {
        canvas.drawPath(path, paint);
        return;
    }

    // Polygon between the edges of this layer: casing and other wide layers keep their margin,
    // thin layers keep their share of the width
    const auto& g = road.geometry;
    const auto count = g.vertices.size();
    const auto stroke = paint.getStrokeWidth();
    QVector<float> left(count);
    QVector<float> right(count);
    for (int i = 0; i < count; i++)
    {
        const auto centre = (g.bodyLeft[i] + g.bodyRight[i]) / 2.0f;
        const auto half = (g.bodyRight[i] - g.bodyLeft[i]) / 2.0f;
        const auto layerHalf = stroke >= road.width
            ? half + (stroke - road.width) / 2.0f
            : half * stroke / road.width;
        left[i] = centre - layerHalf;
        right[i] = centre + layerHalf;
    }
    auto leftPoints = offsetPoints(g, left);
    auto rightPoints = offsetPoints(g, right);
    if (leftPoints.isEmpty() || rightPoints.isEmpty())
        return;
    // Joined ends overlap by a pixel, so that antialiasing leaves no seam between the roads
    const auto extend =
        [&](const int end, const int next, const bool joined)
        {
            if (!joined)
                return;
            auto d = g.vertices[end] - g.vertices[next];
            const auto length = std::sqrt(d.x * d.x + d.y * d.y);
            if (length <= 0.0f)
                return;
            d = d / length;
            leftPoints[end] = leftPoints[end] + d;
            rightPoints[end] = rightPoints[end] + d;
        };
    extend(0, 1, g.hasFirstNormal);
    extend(count - 1, count - 2, g.hasLastNormal);
    SkPath polygon;
    polygon.moveTo(leftPoints[0].x, leftPoints[0].y);
    for (int i = 1; i < count; i++)
        polygon.lineTo(leftPoints[i].x, leftPoints[i].y);
    for (int i = count - 1; i >= 0; i--)
        polygon.lineTo(rightPoints[i].x, rightPoints[i].y);
    polygon.close();

    const auto style = paint.getStyle();
    paint.setStyle(SkPaint::kFill_Style);
    canvas.drawPath(polygon, paint);
    if (paint.getStrokeCap() == SkPaint::kRound_Cap)
    {
        // Round ends where the road is not joined to another one
        if (!g.hasFirstNormal)
        {
            const auto c = (leftPoints[0] + rightPoints[0]) / 2.0f;
            canvas.drawCircle(c.x, c.y, (right[0] - left[0]) / 2.0f, paint);
        }
        if (!g.hasLastNormal)
        {
            const auto c = (leftPoints[count - 1] + rightPoints[count - 1]) / 2.0f;
            canvas.drawCircle(c.x, c.y, (right[count - 1] - left[count - 1]) / 2.0f, paint);
        }
    }
    paint.setStyle(style);
}

void OsmAnd::MapRasterizer_P::flushLaneMarkings(const Context& context, SkCanvas& canvas)
{
    if (context.pendingMarkings.isEmpty())
        return;
    const auto pending = context.pendingMarkings;
    context.pendingMarkings.clear();

    QVector<RealisticRoad> roads;
    QSet<const MapObject*> seen;
    for (const auto& primitive : constOf(pending))
    {
        if (seen.contains(primitive->sourceObject.get()))
            continue;
        seen.insert(primitive->sourceObject.get());
        RealisticRoad road;
        computeRealisticRoad(context, primitive, road);
        if (!road.valid)
            continue;
        roads.push_back(road);

        // Gore areas go under the markings of all roads
        const auto& layout = *road.layout;
        if (layout.goreFirst || layout.goreLast)
        {
            SkPaint paint = _defaultPaint;
            SkColor color = SK_ColorGRAY;
            if (updatePaint(context, paint, primitive, PaintValuesSet::Layer_1, false, &road))
                color = paint.getColor();
            if (layout.goreFirst)
                rasterizeGore(context, canvas, *primitive->sourceObject, road, true, color);
            if (layout.goreLast)
                rasterizeGore(context, canvas, *primitive->sourceObject, road, false, color);
        }
    }
    for (const auto& road : constOf(roads))
        rasterizeLaneMarkings(context, canvas, road);
}

void OsmAnd::MapRasterizer_P::rasterizeGore(
    const Context& context,
    SkCanvas& canvas,
    const MapObject& mapObject,
    const RealisticRoad& road,
    const bool atFirst,
    const SkColor color)
{
    // Painted area between this road and the branch on its left, from the split or merge node
    // until the lane edges are apart
    const auto& layout = *road.layout;
    const auto partnerObject = atFirst ? layout.goreFirst : layout.goreLast;
    const auto citPartner = context.roadLayouts.constFind(partnerObject);
    if (citPartner == context.roadLayouts.cend())
        return;
    const auto& partnerLayout = *citPartner;
    RoadGeometry partner;
    if (!computeRoadGeometry(context, *partnerObject, partnerLayout, partner))
        return;

    const auto& g = road.geometry;
    auto edge = offsetPoints(g, layout.lanesBackward > 0 ? g.boundaries.last() : g.boundaries.first());
    auto partnerEdge = offsetPoints(partner, partnerLayout.lanesBackward > 0 ? partner.boundaries.first() : partner.boundaries.last());
    if (edge.size() < 2 || partnerEdge.size() < 2)
        return;
    const auto& node = atFirst ? mapObject.points31.first() : mapObject.points31.last();
    if (!atFirst)
        std::reverse(edge.begin(), edge.end());
    const auto& partnerFirst = partnerObject->points31.first();
    const auto& partnerLast = partnerObject->points31.last();
    if (partnerLast.x == node.x && partnerLast.y == node.y)
        std::reverse(partnerEdge.begin(), partnerEdge.end());
    else if (partnerFirst.x != node.x || partnerFirst.y != node.y)
        return;

    const auto m = context.pixelsPerMeter;
    const auto lengthOf = [](const QVector<PointF>& line)
        {
            float length = 0.0f;
            for (int i = 1; i < line.size(); i++)
            {
                const auto d = line[i] - line[i - 1];
                length += std::sqrt(d.x * d.x + d.y * d.y);
            }
            return length;
        };
    // Walks along a line, the distance must not decrease between calls
    struct Walker
    {
        const QVector<PointF>& line;
        int segment;
        float passed;
        PointF at(const float distance)
        {
            while (segment + 1 < line.size())
            {
                const auto d = line[segment + 1] - line[segment];
                const auto length = std::sqrt(d.x * d.x + d.y * d.y);
                if (passed + length >= distance || segment + 2 == line.size())
                {
                    const auto t = length > 0.0f ? qBound(0.0f, (distance - passed) / length, 1.0f) : 0.0f;
                    return line[segment] + d * t;
                }
                passed += length;
                segment++;
            }
            return line.last();
        }
    };
    const auto maxLength = qMin(RealisticRoadsGoreMaxLength * m, qMin(lengthOf(edge), lengthOf(partnerEdge)));
    const auto step = qMax(1.0f, 1.0f * m);
    const auto endWidth = RealisticRoadsGoreWidth * m;
    Walker walker{ edge, 0, 0.0f };
    Walker partnerWalker{ partnerEdge, 0, 0.0f };
    QVector<PointF> right;
    QVector<PointF> left;
    for (float distance = 0.0f; ; distance += step)
    {
        const auto last = distance >= maxLength;
        const auto p = walker.at(qMin(distance, maxLength));
        const auto q = partnerWalker.at(qMin(distance, maxLength));
        right.push_back(p);
        left.push_back(q);
        const auto gap = p - q;
        if (last || std::sqrt(gap.x * gap.x + gap.y * gap.y) >= endWidth)
            break;
    }
    if (right.size() < 3)
        return;

    SkPath polygon;
    polygon.moveTo(left[0].x, left[0].y);
    for (int i = 1; i < left.size(); i++)
        polygon.lineTo(left[i].x, left[i].y);
    for (int i = right.size() - 1; i >= 0; i--)
        polygon.lineTo(right[i].x, right[i].y);
    polygon.close();

    SkPaint fillPaint = _defaultPaint;
    fillPaint.setStyle(SkPaint::kFill_Style);
    fillPaint.setColor(color);
    canvas.drawPath(polygon, fillPaint);

    // Diagonal hatching
    auto along = (left.last() + right.last()) / 2.0f - (left.first() + right.first()) / 2.0f;
    const auto alongLength = std::sqrt(along.x * along.x + along.y * along.y);
    if (alongLength <= 0.0f)
        return;
    along = along / alongLength;
    const auto cosA = static_cast<float>(M_SQRT1_2);
    const PointF stripe(along.x * cosA - along.y * cosA, along.x * cosA + along.y * cosA);
    const PointF across(-stripe.y, stripe.x);
    float minAcross = std::numeric_limits<float>::max();
    float maxAcross = -std::numeric_limits<float>::max();
    float minAlong = std::numeric_limits<float>::max();
    float maxAlong = -std::numeric_limits<float>::max();
    const auto origin = left.first();
    for (const auto& points : { &left, &right })
    {
        for (const auto& p : constOf(*points))
        {
            const auto d = p - origin;
            minAcross = qMin(minAcross, d.x * across.x + d.y * across.y);
            maxAcross = qMax(maxAcross, d.x * across.x + d.y * across.y);
            minAlong = qMin(minAlong, d.x * stripe.x + d.y * stripe.y);
            maxAlong = qMax(maxAlong, d.x * stripe.x + d.y * stripe.y);
        }
    }
    SkPaint hatchPaint = _defaultPaint;
    hatchPaint.setStyle(SkPaint::kStroke_Style);
    hatchPaint.setStrokeCap(SkPaint::kButt_Cap);
    hatchPaint.setStrokeWidth(qMax(1.0f, 0.4f * m));
    hatchPaint.setColor(SkColorSetARGB(0xE0, 0xFF, 0xFF, 0xFF));
    SkPath hatch;
    const auto spacing = qMax(4.0f, 3.0f * m);
    for (auto offset = minAcross; offset <= maxAcross; offset += spacing)
    {
        const auto from = origin + across * offset + stripe * minAlong;
        const auto to = origin + across * offset + stripe * maxAlong;
        hatch.moveTo(from.x, from.y);
        hatch.lineTo(to.x, to.y);
    }
    canvas.save();
    canvas.clipPath(polygon, true);
    canvas.drawPath(hatch, hatchPaint);
    canvas.restore();
}

void OsmAnd::MapRasterizer_P::drawLaneArrows(
    SkCanvas& canvas,
    const QVector<PointF>& vertices,
    const QVector<float>& offsets,
    const QString& turn,
    const float endGap,
    const float pixelsPerMeter)
{
    // Arrow painted endGap before the end of the lane, 5 m long, like on the road
    const auto count = vertices.size();
    if (count < 2 || turn.isEmpty() || turn == QLatin1String("none"))
        return;
    float total = 0.0f;
    for (int i = 1; i < count; i++)
    {
        const auto d = vertices[i] - vertices[i - 1];
        total += std::sqrt(d.x * d.x + d.y * d.y);
    }
    const auto m = pixelsPerMeter;
    const auto startDistance = total - endGap;
    if (startDistance < 3.0f * m)
        return;

    // Find the arrow base and the direction there
    float passed = 0.0f;
    PointF base = vertices[0];
    PointF dir(1.0f, 0.0f);
    float offset = offsets[0];
    for (int i = 1; i < count; i++)
    {
        const auto d = vertices[i] - vertices[i - 1];
        const auto length = std::sqrt(d.x * d.x + d.y * d.y);
        if (length <= 0.0f)
            continue;
        if (passed + length >= startDistance || i == count - 1)
        {
            const auto t = qBound(0.0f, (startDistance - passed) / length, 1.0f);
            base = vertices[i - 1] + d * t;
            dir = d / length;
            offset = offsets[i - 1] + (offsets[i] - offsets[i - 1]) * t;
            break;
        }
        passed += length;
    }
    const PointF normal(-dir.y, dir.x);
    base = base + normal * offset;
    const auto toPixel =
        [&base, &dir, &normal, m](const float x, const float y) -> SkPoint
        {
            const auto p = base + dir * (x * m) + normal * (y * m);
            return SkPoint::Make(p.x, p.y);
        };

    SkPaint shaftPaint;
    shaftPaint.setAntiAlias(true);
    shaftPaint.setStyle(SkPaint::kStroke_Style);
    shaftPaint.setStrokeWidth(qMax(1.0f, 0.3f * m));
    shaftPaint.setStrokeJoin(SkPaint::kRound_Join);
    shaftPaint.setColor(SkColorSetARGB(0xE6, 0xFF, 0xFF, 0xFF));
    SkPaint headPaint = shaftPaint;
    headPaint.setStyle(SkPaint::kFill_Style);

    const auto head =
        [&](const float tipX, const float tipY, const float angle)
        {
            // angle: 0 = straight ahead, positive turns right
            const float dx = std::cos(angle);
            const float dy = std::sin(angle);
            const float headLength = 1.4f;
            const float headHalfWidth = 0.6f;
            const float bx = tipX - dx * headLength;
            const float by = tipY - dy * headLength;
            SkPath path;
            path.moveTo(toPixel(tipX, tipY));
            path.lineTo(toPixel(bx - dy * headHalfWidth, by + dx * headHalfWidth));
            path.lineTo(toPixel(bx + dy * headHalfWidth, by - dx * headHalfWidth));
            path.close();
            canvas.drawPath(path, headPaint);
        };

    const auto branch =
        [&](const float angle, const float length)
        {
            // Shaft goes straight for 2.6 m, then bends towards the turn
            SkPath shaft;
            shaft.moveTo(toPixel(0.0f, 0.0f));
            shaft.lineTo(toPixel(2.6f, 0.0f));
            const float ex = 2.6f + std::cos(angle) * length;
            const float ey = std::sin(angle) * length;
            shaft.lineTo(toPixel(ex, ey));
            canvas.drawPath(shaft, shaftPaint);
            head(ex + std::cos(angle) * 1.4f, ey + std::sin(angle) * 1.4f, angle);
        };

    const auto deg = static_cast<float>(M_PI / 180.0);
    for (const auto& part : turn.split(QLatin1Char(';'), QString::SkipEmptyParts))
    {
        const auto t = part.trimmed();
        if (t == QLatin1String("through"))
        {
            SkPath shaft;
            shaft.moveTo(toPixel(0.0f, 0.0f));
            shaft.lineTo(toPixel(3.6f, 0.0f));
            canvas.drawPath(shaft, shaftPaint);
            head(5.0f, 0.0f, 0.0f);
        }
        else if (t == QLatin1String("left"))
            branch(-80.0f * deg, 0.6f);
        else if (t == QLatin1String("right"))
            branch(80.0f * deg, 0.6f);
        else if (t == QLatin1String("slight_left"))
            branch(-35.0f * deg, 0.8f);
        else if (t == QLatin1String("slight_right"))
            branch(35.0f * deg, 0.8f);
        else if (t == QLatin1String("sharp_left"))
            branch(-125.0f * deg, 0.5f);
        else if (t == QLatin1String("sharp_right"))
            branch(125.0f * deg, 0.5f);
        else if (t == QLatin1String("merge_to_left"))
            branch(-25.0f * deg, 1.6f);
        else if (t == QLatin1String("merge_to_right"))
            branch(25.0f * deg, 1.6f);
        else if (t == QLatin1String("reverse"))
        {
            SkPath shaft;
            shaft.moveTo(toPixel(0.0f, 0.0f));
            shaft.lineTo(toPixel(3.0f, 0.0f));
            shaft.quadTo(toPixel(4.2f, -0.6f), toPixel(3.0f, -1.2f));
            shaft.lineTo(toPixel(2.4f, -1.2f));
            canvas.drawPath(shaft, shaftPaint);
            head(1.2f, -1.2f, static_cast<float>(M_PI));
        }
    }
}

void OsmAnd::MapRasterizer_P::rasterizeLaneMarkings(
    const Context& context,
    SkCanvas& canvas,
    const RealisticRoad& road)
{
    const auto& layout = *road.layout;
    const auto& g = road.geometry;
    const auto lanesForward = layout.lanesForward;
    const auto lanesBackward = layout.lanesBackward;
    const auto totalLanes = lanesForward + lanesBackward;
    if (road.laneWidth < RealisticRoadsMinLaneMarkingWidth || g.boundaries.size() != totalLanes + 1)
        return;
    const auto count = g.vertices.size();
    const auto m = context.pixelsPerMeter;
    const auto laneWidth = road.laneWidth;

    // Turn, change and bus values by slot, slots go from the left edge in the way direction
    QStringList turns, changes, buses;
    for (int slot = 0; slot < totalLanes; slot++)
    {
        const bool isBackward = slot < lanesBackward;
        const auto index = isBackward ? lanesBackward - 1 - slot : slot - lanesBackward;
        const auto& turnList = isBackward ? layout.turnBackward : layout.turnForward;
        const auto& changeList = isBackward ? layout.changeBackward : layout.changeForward;
        const auto& busList = isBackward ? layout.busBackward : layout.busForward;
        turns.push_back(index < turnList.size() ? turnList[index] : QString());
        changes.push_back(index < changeList.size() ? changeList[index] : QString());
        buses.push_back(index < busList.size() ? busList[index] : QString());
    }
    const auto taperedAtFirst = [&](const int slot)
        {
            return slot < layout.taperFirstLeft || slot >= totalLanes - layout.taperFirstRight;
        };
    const auto taperedAtLast = [&](const int slot)
        {
            return slot < layout.taperLastLeft || slot >= totalLanes - layout.taperLastRight;
        };
    const auto laneCentre = [&](const int slot)
        {
            QVector<float> result(count);
            for (int i = 0; i < count; i++)
                result[i] = (g.boundaries[slot][i] + g.boundaries[slot + 1][i]) / 2.0f;
            return result;
        };

    // Bus lanes
    SkPaint busPaint = _defaultPaint;
    busPaint.setStyle(SkPaint::kStroke_Style);
    busPaint.setStrokeCap(SkPaint::kButt_Cap);
    busPaint.setStrokeWidth(laneWidth);
    busPaint.setColor(SkColorSetARGB(0x55, 0xD3, 0x2F, 0x2F));
    for (int slot = 0; slot < totalLanes; slot++)
    {
        if (isBusLane(buses[slot]))
            canvas.drawPath(buildOffsetPath(g, laneCentre(slot)), busPaint);
    }

    SkPaint paint = _defaultPaint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeCap(SkPaint::kButt_Cap);
    paint.setColor(SkColorSetARGB(0xE0, 0xFF, 0xFF, 0xFF));
    const auto lineWidth = qMax(1.0f, 0.15f * m);

    // Solid edge lines of motorways and trunk roads
    if (layout.edgeLines)
    {
        paint.setStrokeWidth(qMax(1.0f, 0.2f * m));
        paint.setPathEffect(nullptr);
        canvas.drawPath(buildOffsetPath(g, g.boundaries.first()), paint);
        canvas.drawPath(buildOffsetPath(g, g.boundaries.last()), paint);
    }

    if (totalLanes >= 2)
    {
        const SkScalar dashIntervals[2] = { 3.0f * m, 6.0f * m };
        const auto dashEffect = SkDashPathEffect::Make(dashIntervals, 2, 0);
        const SkScalar blockIntervals[2] = { 1.0f * m, 1.5f * m };
        const auto blockEffect = SkDashPathEffect::Make(blockIntervals, 2, 0);

        for (int slot = 0; slot + 1 < totalLanes; slot++)
        {
            const bool separatesDirections = slot + 1 == lanesBackward;
            bool solid = separatesDirections;
            bool block = false;
            if (!separatesDirections)
            {
                // In the driving direction of these lanes, which of the two is on the left
                const bool isBackward = slot < lanesBackward;
                const auto leftSlot = isBackward ? slot + 1 : slot;
                const auto rightSlot = isBackward ? slot : slot + 1;
                solid = forbidsChange(changes[leftSlot], true) || forbidsChange(changes[rightSlot], false);
                // Lanes that end or open get block markings
                block = isMerge(turns[leftSlot]) || isMerge(turns[rightSlot])
                    || taperedAtFirst(slot) || taperedAtFirst(slot + 1) || taperedAtLast(slot) || taperedAtLast(slot + 1);
            }
            paint.setStrokeWidth(block ? qMax(1.0f, 0.3f * m) : lineWidth);
            paint.setPathEffect(solid ? nullptr : (block ? blockEffect : dashEffect));
            canvas.drawPath(buildOffsetPath(g, g.boundaries[slot + 1]), paint);
        }
    }

    if (laneWidth < RealisticRoadsMinLaneArrowWidth || layout.noArrows)
        return;
    const auto total = g.distances.last();
    const auto laneTaper = qMin(RealisticRoadsLaneTaperLength * m, total * 0.5f);
    const auto arrowGap = 13.0f * m;
    RoadGeometry reversed;
    for (int slot = 0; slot < totalLanes; slot++)
    {
        if (turns[slot].isEmpty())
            continue;
        const auto offsets = laneCentre(slot);
        if (slot < lanesBackward)
        {
            // Lanes against the way direction end at its first point
            if (reversed.vertices.isEmpty())
            {
                reversed.vertices.resize(count);
                std::reverse_copy(g.vertices.cbegin(), g.vertices.cend(), reversed.vertices.begin());
            }
            QVector<float> reversedOffsets(count);
            for (int i = 0; i < count; i++)
                reversedOffsets[i] = -offsets[count - 1 - i];
            drawLaneArrows(canvas, reversed.vertices, reversedOffsets, turns[slot],
                arrowGap + (taperedAtFirst(slot) ? laneTaper : 0.0f), m);
        }
        else
        {
            drawLaneArrows(canvas, g.vertices, offsets, turns[slot],
                arrowGap + (taperedAtLast(slot) ? laneTaper : 0.0f), m);
        }
    }
}
