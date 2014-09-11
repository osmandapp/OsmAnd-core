#include "MapRasterizer_P.h"
#include "MapRasterizer.h"
#include "MapRasterizer_Metrics.h"

#include "QtCommon.h"
#include <QReadWriteLock>

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmapDevice.h>
#include <SkBlurDrawLooper.h>
#include <SkColorFilter.h>
#include <SkDashPathEffect.h>
#include <SkBitmapProcShader.h>
#include <SkError.h>
#include "restore_internal_warnings.h"

#include "MapPresentationEnvironment.h"
#include "MapStyleEvaluationResult.h"
#include "Primitiviser.h"
#include "BinaryMapObject.h"
#include "ObfMapSectionInfo.h"
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
    {
        QMutexLocker scopedLocker(&_pathEffectsMutex);

        for (auto& pathEffect : _pathEffects)
            pathEffect->unref();
    }
}

void OsmAnd::MapRasterizer_P::initialize()
{
    _defaultPaint.setAntiAlias(true);

    const auto intervalW = 0.5f*owner->mapPresentationEnvironment->displayDensityFactor + 0.5f; // 0.5dp + 0.5px
    {
        const float intervals_oneway[4][4] =
        {
            { 0, 12, 10 * intervalW, 152 },
            { 0, 12, 9 * intervalW, 152 + intervalW },
            { 0, 12 + 6 * intervalW, 2 * intervalW, 152 + 2 * intervalW },
            { 0, 12 + 6 * intervalW, 1 * intervalW, 152 + 3 * intervalW }
        };
        SkPathEffect* arrowDashEffect1 = SkDashPathEffect::Create(intervals_oneway[0], 4, 0);
        SkPathEffect* arrowDashEffect2 = SkDashPathEffect::Create(intervals_oneway[1], 4, 1);
        SkPathEffect* arrowDashEffect3 = SkDashPathEffect::Create(intervals_oneway[2], 4, 1);
        SkPathEffect* arrowDashEffect4 = SkDashPathEffect::Create(intervals_oneway[3], 4, 1);

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(1.0f * intervalW);
            paint.setPathEffect(arrowDashEffect1)->unref();
            _oneWayPaints.push_back(qMove(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(2.0f * intervalW);
            paint.setPathEffect(arrowDashEffect2)->unref();
            _oneWayPaints.push_back(qMove(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(3.0f * intervalW);
            paint.setPathEffect(arrowDashEffect3)->unref();
            _oneWayPaints.push_back(qMove(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(4.0f * intervalW);
            paint.setPathEffect(arrowDashEffect4)->unref();
            _oneWayPaints.push_back(qMove(paint));
        }
    }

    {
        const float intervals_reverse[4][4] =
        {
            { 0, 12, 10 * intervalW, 152 },
            { 0, 12 + 1 * intervalW, 9 * intervalW, 152 },
            { 0, 12 + 2 * intervalW, 2 * intervalW, 152 + 6 * intervalW },
            { 0, 12 + 3 * intervalW, 1 * intervalW, 152 + 6 * intervalW }
        };
        SkPathEffect* arrowDashEffect1 = SkDashPathEffect::Create(intervals_reverse[0], 4, 0);
        SkPathEffect* arrowDashEffect2 = SkDashPathEffect::Create(intervals_reverse[1], 4, 1);
        SkPathEffect* arrowDashEffect3 = SkDashPathEffect::Create(intervals_reverse[2], 4, 1);
        SkPathEffect* arrowDashEffect4 = SkDashPathEffect::Create(intervals_reverse[3], 4, 1);

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(1.0f * intervalW);
            paint.setPathEffect(arrowDashEffect1)->unref();
            _reverseOneWayPaints.push_back(qMove(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(2.0f * intervalW);
            paint.setPathEffect(arrowDashEffect2)->unref();
            _reverseOneWayPaints.push_back(qMove(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(3.0f * intervalW);
            paint.setPathEffect(arrowDashEffect3)->unref();
            _reverseOneWayPaints.push_back(qMove(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(4.0f * intervalW);
            paint.setPathEffect(arrowDashEffect4)->unref();
            _reverseOneWayPaints.push_back(qMove(paint));
        }
    }
}

void OsmAnd::MapRasterizer_P::initializeOneWayPaint(SkPaint& paint)
{
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(0xff6c70d5);
}

void OsmAnd::MapRasterizer_P::rasterize(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkCanvas& canvas,
    const bool fillBackground,
    const AreaI* const pDestinationArea,
    MapRasterizer_Metrics::Metric_rasterize* const metric,
    const IQueryController* const controller)
{
    const Stopwatch totalStopwatch(metric != nullptr);

    // Deal with background
    if (fillBackground)
    {
        if (pDestinationArea)
        {
            // If destination area is specified, fill only it with background
            SkPaint bgPaint;
            bgPaint.setColor(primitivizedArea->defaultBackgroundColor);
            bgPaint.setStyle(SkPaint::kFill_Style);
            canvas.drawRectCoords(
                pDestinationArea->top(),
                pDestinationArea->left(),
                pDestinationArea->right(),
                pDestinationArea->bottom(),
                bgPaint);
        }
        else
        {
            // Since destination area is not specified, erase whole canvas with specified color
            canvas.clear(primitivizedArea->defaultBackgroundColor);
        }
    }

    // Precalculate values
    AreaI destinationArea;
    if (pDestinationArea)
    {
        destinationArea = *pDestinationArea;
    }
    else
    {
        const auto targetSize = canvas.getDeviceSize();
        destinationArea = AreaI(0, 0, targetSize.height(), targetSize.width());
    }

    // Rasterize layers of map:
    rasterizeMapPrimitives(primitivizedArea, canvas, primitivizedArea->polygons, PrimitivesType::Polygons, controller);
    if (primitivizedArea->shadowRenderingMode > 1)
        rasterizeMapPrimitives(primitivizedArea, canvas, primitivizedArea->polylines, PrimitivesType::Polylines_ShadowOnly, controller);
    rasterizeMapPrimitives(primitivizedArea, canvas, primitivizedArea->polylines, PrimitivesType::Polylines, controller);

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();
}

void OsmAnd::MapRasterizer_P::rasterizeMapPrimitives(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkCanvas& canvas,
    const Primitiviser::PrimitivesCollection& primitives,
    PrimitivesType type,
    const IQueryController* const controller)
{
    assert(type != PrimitivesType::Points);

    for (const auto& primitive : constOf(primitives))
    {
        if (controller && controller->isAborted())
            return;

        if (type == PrimitivesType::Polygons)
        {
            rasterizePolygon(
                primitivizedArea,
                canvas,
                primitive);
        }
        else if (type == PrimitivesType::Polylines || type == PrimitivesType::Polylines_ShadowOnly)
        {
            rasterizePolyline(
                primitivizedArea,
                canvas,
                primitive,
                (type == PrimitivesType::Polylines_ShadowOnly));
        }
    }
}

bool OsmAnd::MapRasterizer_P::updatePaint(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkPaint& paint,
    const MapStyleEvaluationResult& evalResult,
    const PaintValuesSet valueSetSelector,
    const bool isArea)
{
    const auto& env = primitivizedArea->mapPresentationEnvironment;

    bool ok = true;

    int valueDefId_color = -1;
    int valueDefId_strokeWidth = -1;
    int valueDefId_cap = -1;
    int valueDefId_pathEffect = -1;
    switch (valueSetSelector)
    {
        case PaintValuesSet::Set_0:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT;
            break;
        case PaintValuesSet::Set_1:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR_2;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_2;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP_2;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT_2;
            break;
        case PaintValuesSet::Set_minus1:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR_0;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_0;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP_0;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT_0;
            break;
        case PaintValuesSet::Set_minus2:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR__1;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH__1;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP__1;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT__1;
            break;
        case PaintValuesSet::Set_3:
            valueDefId_color = env->styleBuiltinValueDefs->id_OUTPUT_COLOR_3;
            valueDefId_strokeWidth = env->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_3;
            valueDefId_cap = env->styleBuiltinValueDefs->id_OUTPUT_CAP_3;
            valueDefId_pathEffect = env->styleBuiltinValueDefs->id_OUTPUT_PATH_EFFECT_3;
            break;
    }

    if (isArea)
    {
        paint.setColorFilter(nullptr);
        paint.setShader(nullptr);
        paint.setLooper(nullptr);
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
        paint.setLooper(nullptr);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(stroke);

        QString cap;
        ok = evalResult.getStringValue(valueDefId_cap, cap);
        if (!ok || cap.isEmpty() || cap == QLatin1String("BUTT"))
            paint.setStrokeCap(SkPaint::kButt_Cap);
        else if (cap == QLatin1String("ROUND"))
            paint.setStrokeCap(SkPaint::kRound_Cap);
        else if (cap == QLatin1String("SQUARE"))
            paint.setStrokeCap(SkPaint::kSquare_Cap);
        else
            paint.setStrokeCap(SkPaint::kButt_Cap);

        QString encodedPathEffect;
        ok = evalResult.getStringValue(valueDefId_pathEffect, encodedPathEffect);
        if (!ok || encodedPathEffect.isEmpty())
        {
            paint.setPathEffect(nullptr);
        }
        else
        {
            SkPathEffect* pathEffect = nullptr;
            ok = obtainPathEffect(encodedPathEffect, pathEffect);

            if (ok && pathEffect)
                paint.setPathEffect(pathEffect);
        }
    }

    SkColor color;
    ok = evalResult.getIntegerValue(valueDefId_color, color);
    paint.setColor(ok ? color : SK_ColorWHITE);

    if (valueSetSelector == PaintValuesSet::Set_0)
    {
        QString shader;
        ok = evalResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADER, shader);
        if (ok && !shader.isEmpty())
        {
            SkBitmapProcShader* shaderObj = nullptr;
            if (obtainBitmapShader(env, shader, shaderObj) && shaderObj)
            {
                paint.setShader(static_cast<SkShader*>(shaderObj));
                shaderObj->unref();

                if (paint.getColor() == SK_ColorTRANSPARENT)
                    paint.setColor(SK_ColorWHITE);
            }
        }
    }

    // do not check shadow color here
    if (primitivizedArea->shadowRenderingMode == 1 && valueSetSelector == PaintValuesSet::Set_0)
    {
        SkColor shadowColor;
        ok = evalResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_COLOR, shadowColor);
        int shadowRadius;
        evalResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_RADIUS, shadowRadius);
        if (!ok || shadowColor == 0)
            shadowColor = primitivizedArea->shadowRenderingColor;
        if (shadowColor == 0)
            shadowRadius = 0;

        if (shadowRadius > 0)
            paint.setLooper(SkBlurDrawLooper::Create(static_cast<SkScalar>(shadowRadius), 0, 0, shadowColor))->unref();
    }

    return true;
}

void OsmAnd::MapRasterizer_P::rasterizePolygon(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkCanvas& canvas,
    const std::shared_ptr<const Primitiviser::Primitive>& primitive)
{
    const auto& points31 = primitive->sourceObject->points31;
    const auto& area31 = primitivizedArea->area31;

    assert(points31.size() > 2);
    assert(primitive->sourceObject->isClosedFigure());
    assert(primitive->sourceObject->isClosedFigure(true));

    //////////////////////////////////////////////////////////////////////////
    if ((primitive->sourceObject->id >> 1) == 25829290u)
    {
        int i = 5;
    }
    //////////////////////////////////////////////////////////////////////////

    SkPaint paint = _defaultPaint;
    if (!updatePaint(primitivizedArea, paint, primitive->evaluationResult, PaintValuesSet::Set_0, true))
        return;

    // Construct and test geometry against bbox area
    SkPath path;
    bool containsAtLeastOnePoint = false;
    int pointIdx = 0;
    PointF vertex;
    int bounds = 0;
    QVector< PointI > outerPoints;
    const auto pointsCount = points31.size();
    auto pPoint = points31.constData();
    for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        const auto& point = *pPoint;
        calculateVertex(primitivizedArea, point, vertex);

        // Hit-test
        if (!containsAtLeastOnePoint)
        {
            if (area31.contains(point))
                containsAtLeastOnePoint = true;
            else
                outerPoints.push_back(point);
            bounds |= (point.x < area31.left() ? 1 : 0);
            bounds |= (point.x > area31.right() ? 2 : 0);
            bounds |= (point.y < area31.top() ? 4 : 0);
            bounds |= (point.y > area31.bottom() ? 8 : 0);
        }

        // Plot vertex
        if (pointIdx == 0)
            path.moveTo(vertex.x, vertex.y);
        else
            path.lineTo(vertex.x, vertex.y);
    }

    if (!containsAtLeastOnePoint)
    {
        // fast check for polygons
        if ((bounds & 3) != 3 || (bounds >> 2) != 3)
            return;

        // 
        bool ok = true;
        ok = ok || containsHelper(outerPoints, area31.topLeft);
        ok = ok || containsHelper(outerPoints, area31.bottomRight);
        ok = ok || containsHelper(outerPoints, PointI(0, area31.bottom()));
        ok = ok || containsHelper(outerPoints, PointI(area31.right(), 0));
        if (!ok)
            return;
    }

    if (!primitive->sourceObject->innerPolygonsPoints31.isEmpty())
    {
        path.setFillType(SkPath::kEvenOdd_FillType);
        for (const auto& polygon : constOf(primitive->sourceObject->innerPolygonsPoints31))
        {
            pointIdx = 0;
            for (auto itVertex = cachingIteratorOf(constOf(polygon)); itVertex; ++itVertex, pointIdx++)
            {
                const auto& point = *itVertex;
                calculateVertex(primitivizedArea, point, vertex);

                if (pointIdx == 0)
                    path.moveTo(vertex.x, vertex.y);
                else
                    path.lineTo(vertex.x, vertex.y);
            }
        }
    }

    canvas.drawPath(path, paint);
    if (updatePaint(primitivizedArea, paint, primitive->evaluationResult, PaintValuesSet::Set_1, false))
        canvas.drawPath(path, paint);
}

void OsmAnd::MapRasterizer_P::rasterizePolyline(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkCanvas& canvas,
    const std::shared_ptr<const Primitiviser::Primitive>& primitive,
    bool drawOnlyShadow)
{
    const auto& points31 = primitive->sourceObject->points31;
    const auto& area31 = primitivizedArea->area31;
    const auto& env = primitivizedArea->mapPresentationEnvironment;

    assert(points31.size() >= 2);

    SkPaint paint = _defaultPaint;
    if (!updatePaint(primitivizedArea, paint, primitive->evaluationResult, PaintValuesSet::Set_0, false))
        return;

    bool ok;

    ColorARGB shadowColor;
    ok = primitive->evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_COLOR, shadowColor.argb);
    if (!ok || shadowColor == 0)
        shadowColor = primitivizedArea->shadowRenderingColor;

    int shadowRadius;
    ok = primitive->evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_RADIUS, shadowRadius);
    if (drawOnlyShadow && (!ok || shadowRadius == 0))
        return;

    const auto typeRuleId = primitive->sourceObject->typesRuleIds[primitive->typeRuleIdIndex];
    const auto& encDecRules = primitive->sourceObject->section->encodingDecodingRules;

    int oneway = 0;
    if (primitivizedArea->zoom >= ZoomLevel16 && typeRuleId == encDecRules->highway_encodingRuleId)
    {
        if (primitive->sourceObject->containsType(encDecRules->oneway_encodingRuleId, true))
            oneway = 1;
        else if (primitive->sourceObject->containsType(encDecRules->onewayReverse_encodingRuleId, true))
            oneway = -1;
    }

    // Construct and test geometry against (wider) bbox area
    SkPath path;
    int pointIdx = 0;
    bool intersect = false;
    int prevCross = 0;
    PointF vertex;
    const auto pointsCount = points31.size();
    auto pPoint = points31.constData();
    for (pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        const auto& point = *pPoint;
        calculateVertex(primitivizedArea, point, vertex);

        // Hit-test
        if (!intersect)
        {
            if (area31.contains(PointI(vertex)))
            {
                intersect = true;
            }
            else
            {
                int cross = 0;
                cross |= (point.x < area31.left() ? 1 : 0);
                cross |= (point.x > area31.right() ? 2 : 0);
                cross |= (point.y < area31.top() ? 4 : 0);
                cross |= (point.y > area31.bottom() ? 8 : 0);
                if (pointIdx > 0)
                {
                    if ((prevCross & cross) == 0)
                    {
                        intersect = true;
                    }
                }
                prevCross = cross;
            }
        }

        // Plot vertex
        if (pointIdx == 0)
            path.moveTo(vertex.x, vertex.y);
        else
            path.lineTo(vertex.x, vertex.y);
    }

    if (!intersect)
        return;

    if (pointIdx > 0)
    {
        if (drawOnlyShadow)
        {
            rasterizeLineShadow(primitivizedArea, canvas, path, paint, shadowColor, shadowRadius);
        }
        else
        {
            if (updatePaint(primitivizedArea, paint, primitive->evaluationResult, PaintValuesSet::Set_minus2, false))
                canvas.drawPath(path, paint);

            if (updatePaint(primitivizedArea, paint, primitive->evaluationResult, PaintValuesSet::Set_minus1, false))
                canvas.drawPath(path, paint);

            if (updatePaint(primitivizedArea, paint, primitive->evaluationResult, PaintValuesSet::Set_0, false))
                canvas.drawPath(path, paint);
            canvas.drawPath(path, paint);

            if (updatePaint(primitivizedArea, paint, primitive->evaluationResult, PaintValuesSet::Set_1, false))
                canvas.drawPath(path, paint);

            if (updatePaint(primitivizedArea, paint, primitive->evaluationResult, PaintValuesSet::Set_3, false))
                canvas.drawPath(path, paint);
            
            if (oneway && !drawOnlyShadow)
                rasterizeLine_OneWay(primitivizedArea, canvas, path, oneway);
        }
    }
}

void OsmAnd::MapRasterizer_P::rasterizeLineShadow(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkCanvas& canvas,
    const SkPath& path,
    SkPaint& paint,
    const ColorARGB shadowColor,
    int shadowRadius)
{
    // blurred shadows
    if (primitivizedArea->shadowRenderingMode == 2 && shadowRadius > 0)
    {
        // simply draw shadow? difference from option 3 ?
        // paint->setColor(0xffffffff);
        paint.setLooper(SkBlurDrawLooper::Create(shadowRadius, 0, 0, shadowColor))->unref();
        canvas.drawPath(path, paint);
    }

    // option shadow = 3 with solid border
    if (primitivizedArea->shadowRenderingMode == 3 && shadowRadius > 0)
    {
        paint.setLooper(nullptr);
        paint.setStrokeWidth(paint.getStrokeWidth() + shadowRadius * 2);
        //		paint->setColor(0xffbababa);
        paint.setColorFilter(SkColorFilter::CreateModeFilter(shadowColor, SkXfermode::kSrcIn_Mode))->unref();
        //		paint->setColor(shadowColor);
        canvas.drawPath(path, paint);
    }
}

void OsmAnd::MapRasterizer_P::rasterizeLine_OneWay(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkCanvas& canvas,
    const SkPath& path,
    int oneway)
{
    if (oneway > 0)
    {
        for (const auto& paint : constOf(_oneWayPaints))
            canvas.drawPath(path, paint);
    }
    else
    {
        for (const auto& paint : constOf(_reverseOneWayPaints))
            canvas.drawPath(path, paint);
    }
}

void OsmAnd::MapRasterizer_P::calculateVertex(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    const PointI& point31,
    PointF& vertex)
{
    vertex.x = static_cast<float>(point31.x - primitivizedArea->area31.left()) / primitivizedArea->scale31ToPixelDivisor.x;
    vertex.y = static_cast<float>(point31.y - primitivizedArea->area31.top()) / primitivizedArea->scale31ToPixelDivisor.y;
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

bool OsmAnd::MapRasterizer_P::obtainPathEffect(const QString& encodedPathEffect, SkPathEffect* &outPathEffect) const
{
    QMutexLocker scopedLocker(&_pathEffectsMutex);

    auto itPathEffects = _pathEffects.constFind(encodedPathEffect);
    if (itPathEffects == _pathEffects.cend())
    {
        const auto& strIntervals = encodedPathEffect.split('_', QString::SkipEmptyParts);
        const auto intervalsCount = strIntervals.size();

        const auto intervals = new SkScalar[intervalsCount];
        auto pInterval = intervals;
        for (const auto& strInterval : constOf(strIntervals))
        {
            float computedValue = 0.0f;

            if (!strInterval.contains(':'))
            {
                computedValue = strInterval.toFloat()*owner->mapPresentationEnvironment->displayDensityFactor;
            }
            else
            {
                // 'dip:px' format
                const auto& complexValue = strInterval.split(':', QString::KeepEmptyParts);

                computedValue = complexValue[0].toFloat()*owner->mapPresentationEnvironment->displayDensityFactor + complexValue[1].toFloat();
            }

            *(pInterval++) = computedValue;
        }

        // Validate
        if (intervalsCount < 2 || intervalsCount % 2 != 0)
        {
            LogPrintf(LogSeverityLevel::Warning, "Path effect (%s) with %d intervals is invalid", qPrintable(encodedPathEffect), intervalsCount);
            return false;
        }

        SkPathEffect* pathEffect = SkDashPathEffect::Create(intervals, intervalsCount, 0);
        delete[] intervals;

        itPathEffects = _pathEffects.insert(encodedPathEffect, pathEffect);
    }

    outPathEffect = *itPathEffects;
    return true;
}

bool OsmAnd::MapRasterizer_P::obtainBitmapShader(const std::shared_ptr<const MapPresentationEnvironment>& env, const QString& name, SkBitmapProcShader* &outShader)
{
    std::shared_ptr<const SkBitmap> bitmap;
    if (!env->obtainShaderBitmap(name, bitmap))
        return false;

    outShader = new SkBitmapProcShader(*bitmap, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
    return true;
}
