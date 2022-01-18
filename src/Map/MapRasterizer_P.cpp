#include "MapRasterizer_P.h"
#include "MapRasterizer.h"
#include "MapRasterizer_Metrics.h"

#include <algorithm>
#include <deque>
#include "QtCommon.h"
#include "ignore_warnings_on_external_includes.h"
#include <QReadWriteLock>
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

    const Context context(
        area31,
        primitivisedObjects,
        pDestinationArea ? *pDestinationArea : AreaI(0, 0, canvas.imageInfo().height(), canvas.imageInfo().width()));

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

        if (type == PrimitivesType::Polygons)
        {
            rasterizePolygon(
                context,
                canvas,
                primitive);
        }
        else if (type == PrimitivesType::Polylines || type == PrimitivesType::Polylines_ShadowOnly)
        {
            rasterizePolyline(
                context,
                canvas,
                primitive,
                (type == PrimitivesType::Polylines_ShadowOnly));
        }
    }
}

bool OsmAnd::MapRasterizer_P::updatePaint(
    const Context& context,
    SkPaint& paint,
    const MapStyleEvaluationResult::Packed& evalResult,
    const PaintValuesSet valueSetSelector,
    const bool isArea)
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
        paint.setStrokeWidth(stroke);

        QString cap;
        evalResult.getStringValue(valueDefId_cap, cap);
        if (cap.compare(QLatin1String("round"), Qt::CaseInsensitive) == 0)
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
    assert(primitive->sourceObject->isClosedFigure());
    assert(primitive->sourceObject->isClosedFigure(true));

    //////////////////////////////////////////////////////////////////////////
    //if ((primitive->sourceObject->id >> 1) == 9223372032559801460u)
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    SkPaint paint = _defaultPaint;
    if (!updatePaint(context, paint, primitive->evaluationResult, PaintValuesSet::Layer_1, true))
        return;

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
        ok = ok || containsHelper(outerPoints, area31.topLeft);
        ok = ok || containsHelper(outerPoints, area31.bottomRight);
        ok = ok || containsHelper(outerPoints, PointI(0, area31.bottom()));
        ok = ok || containsHelper(outerPoints, PointI(area31.right(), 0));
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

    if (updatePaint(context, paint, primitive->evaluationResult, PaintValuesSet::Layer_0, true))
        canvas.drawPath(path, paint);
    if (updatePaint(context, paint, primitive->evaluationResult, PaintValuesSet::Layer_1, true))
        canvas.drawPath(path, paint);
    if (updatePaint(context, paint, primitive->evaluationResult, PaintValuesSet::Layer_2, false))
        canvas.drawPath(path, paint);
}

bool OsmAnd::MapRasterizer_P::calculateLinePath(const Context& context, const QVector<PointI>& points31,
                                                SkPath& outPath, float offset = 0.0f) const
{
    bool rightShift = offset > 0;
    offset = abs(offset);
    bool hasShift = offset > 0;

    bool intersect = false;
    int pointIdx = 0;
    int prevCross = 0;
    const auto& area31 = context.area31;
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
    const QVector<PointI>& points31,
    const MapStyleEvaluationResult::Packed& evalResult,
    const PaintValuesSet valueSetSelector,
    const IMapStyle::ValueDefinitionId hMarginId)
{
    if (updatePaint(context, paint, evalResult, valueSetSelector, false))
    {
        float hMargin = 0.0f;
        if (evalResult.getFloatValue(hMarginId, hMargin))
        {
            SkPath newPath;
            if (calculateLinePath(context, points31, newPath, hMargin))
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
    const auto& points31 = primitive->sourceObject->points31;
    const auto& env = context.env;

    assert(points31.size() >= 2);

    SkPaint paint = _defaultPaint;
    if (!updatePaint(context, paint, primitive->evaluationResult, PaintValuesSet::Layer_1, false))
        return;

    bool ok;

    ColorARGB shadowColor;
    ok = primitive->evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_COLOR,
                                                     shadowColor.argb);
    if (!ok || shadowColor == ColorARGB::fromSkColor(SK_ColorTRANSPARENT))
        shadowColor = context.shadowColor;

    float shadowRadius;
    ok = primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_RADIUS, shadowRadius);
    if (drawOnlyShadow && (!ok || shadowRadius <= 0.0f))
        return;

    SkPath path;
    bool intersect = calculateLinePath(context, points31, path);
    if (!intersect)
        return;

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
        drawLineLayer(canvas, paint, path, context, points31, primitive->evaluationResult, PaintValuesSet::Layer_minus2,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN__2);

        drawLineLayer(canvas, paint, path, context, points31, primitive->evaluationResult, PaintValuesSet::Layer_minus1,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN__1);

        drawLineLayer(canvas, paint, path, context, points31, primitive->evaluationResult, PaintValuesSet::Layer_0,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN_0);

        drawLineLayer(canvas, paint, path, context, points31, primitive->evaluationResult, PaintValuesSet::Layer_1,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN);

        drawLineLayer(canvas, paint, path, context, points31, primitive->evaluationResult, PaintValuesSet::Layer_2,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN_2);

        drawLineLayer(canvas, paint, path, context, points31, primitive->evaluationResult, PaintValuesSet::Layer_3,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN_3);

        drawLineLayer(canvas, paint, path, context, points31, primitive->evaluationResult, PaintValuesSet::Layer_4,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN_4);

        drawLineLayer(canvas, paint, path, context, points31, primitive->evaluationResult, PaintValuesSet::Layer_5,
                      env->styleBuiltinValueDefs->id_OUTPUT_PATH_HMARGIN_5);

        rasterizePolylineIcons(context, canvas, path, primitive->evaluationResult);
    }
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

    sk_sp<const SkImage> pathIcon;
    ok = context.env->obtainMapIcon(pathIconName, pathIcon);
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

bool OsmAnd::MapRasterizer_P::containsHelper(const QVector< PointI >& points, const PointI& otherPoint)
{
    uint32_t intersections = 0;

    auto itPrevPoint = points.cbegin();
    auto itPoint = itPrevPoint + 1;
    for (const auto itEnd = points.cend(); itPoint != itEnd; itPrevPoint = itPoint, ++itPoint)
    {
        const auto& point0 = *itPrevPoint;
        const auto& point1 = *itPoint;

        if (Utilities::rayIntersect(point0, point1, otherPoint))
            intersections++;
    }

    // special handling, also count first and last, might not be closed, but
    // we want this!
    const auto& point0 = points.first();
    const auto& point1 = points.last();
    if (Utilities::rayIntersect(point0, point1, otherPoint))
        intersections++;

    return intersections % 2 == 1;
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
    if (!env->obtainShader(name, image))
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
{
    env->obtainShadowOptions(zoom, shadowMode, shadowColor);
}
