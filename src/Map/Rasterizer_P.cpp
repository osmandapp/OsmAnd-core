#include "Rasterizer_P.h"
#include "Rasterizer.h"
#include "Rasterizer_Metrics.h"

#include "QtCommon.h"
#include <QReadWriteLock>

#include "ICU.h"
#include "MapPresentationEnvironment.h"
#include "MapStyleEvaluationResult.h"
#include "RasterizedSymbolsGroup.h"
#include "RasterizedSymbol.h"
#include "RasterizedOnPathSymbol.h"
#include "RasterizedSpriteSymbol.h"
#include "Primitiviser.h"
#include "BinaryMapObject.h"
#include "ObfMapSectionInfo.h"
#include "QKeyValueIterator.h"
#include "QCachingIterator.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"

#include <SkBitmapDevice.h>
#include <SkBlurDrawLooper.h>
#include <SkColorFilter.h>
#include <SkDashPathEffect.h>
#include <SkBitmapProcShader.h>
#include <SkError.h>
#define OSMAND_DUMP_SYMBOLS 0
#if OSMAND_DUMP_SYMBOLS
#   include <SkImageEncoder.h>
#endif // OSMAND_DUMP_SYMBOLS

OsmAnd::Rasterizer_P::Rasterizer_P(Rasterizer* const owner_)
    : owner(owner_)
{
}

OsmAnd::Rasterizer_P::~Rasterizer_P()
{
}

void OsmAnd::Rasterizer_P::rasterizeMap(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkCanvas& canvas,
    const bool fillBackground,
    const AreaI* const pDestinationArea,
    const IQueryController* const controller)
{
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
                pDestinationArea->top,
                pDestinationArea->left,
                pDestinationArea->right,
                pDestinationArea->bottom,
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
}

void OsmAnd::Rasterizer_P::rasterizeMapPrimitives(
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

bool OsmAnd::Rasterizer_P::updatePaint(
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
            ok = env->obtainPathEffect(encodedPathEffect, pathEffect);

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
            if (env->obtainBitmapShader(shader, shaderObj) && shaderObj)
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
            paint.setLooper(new SkBlurDrawLooper(static_cast<SkScalar>(shadowRadius), 0, 0, shadowColor))->unref();
    }

    return true;
}

void OsmAnd::Rasterizer_P::rasterizePolygon(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkCanvas& canvas,
    const std::shared_ptr<const Primitiviser::Primitive>& primitive)
{
    const auto& points31 = primitive->sourceObject->points31;
    const auto& area31 = primitivizedArea->area31;

    assert(points31.size() > 2);
    assert(primitive->sourceObject->isClosedFigure());
    assert(primitive->sourceObject->isClosedFigure(true));

    SkPaint paint;
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
            bounds |= (point.x < area31.left ? 1 : 0);
            bounds |= (point.x > area31.right ? 2 : 0);
            bounds |= (point.y < area31.top ? 4 : 0);
            bounds |= (point.y > area31.bottom ? 8 : 0);
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
        ok = ok || containsHelper(outerPoints, PointI(0, area31.bottom));
        ok = ok || containsHelper(outerPoints, PointI(area31.right, 0));
        if (!ok)
            return;
    }

    if (!primitive->sourceObject->innerPolygonsPoints31.isEmpty())
    {
        path.setFillType(SkPath::kEvenOdd_FillType);
        for (const auto& polygon : constOf(primitive->sourceObject->innerPolygonsPoints31))
        {
            pointIdx = 0;
            for (auto itVertex = iteratorOf(constOf(polygon)); itVertex; ++itVertex, pointIdx++)
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

void OsmAnd::Rasterizer_P::rasterizePolyline(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkCanvas& canvas,
    const std::shared_ptr<const Primitiviser::Primitive>& primitive,
    bool drawOnlyShadow)
{
    const auto& points31 = primitive->sourceObject->points31;
    const auto& area31 = primitivizedArea->area31;
    const auto& env = primitivizedArea->mapPresentationEnvironment;

    assert(points31.size() >= 2);

    SkPaint paint;
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
                cross |= (point.x < area31.left ? 1 : 0);
                cross |= (point.x > area31.right ? 2 : 0);
                cross |= (point.y < area31.top ? 4 : 0);
                cross |= (point.y > area31.bottom ? 8 : 0);
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

void OsmAnd::Rasterizer_P::rasterizeLineShadow(
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
        paint.setLooper(new SkBlurDrawLooper(shadowRadius, 0, 0, shadowColor))->unref();
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

void OsmAnd::Rasterizer_P::rasterizeLine_OneWay(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkCanvas& canvas,
    const SkPath& path,
    int oneway)
{
    if (oneway > 0)
    {
        for (const auto& paint : constOf(primitivizedArea->mapPresentationEnvironment->oneWayPaints))
            canvas.drawPath(path, paint);
    }
    else
    {
        for (const auto& paint : constOf(primitivizedArea->mapPresentationEnvironment->reverseOneWayPaints))
            canvas.drawPath(path, paint);
    }
}

void OsmAnd::Rasterizer_P::calculateVertex(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    const PointI& point31,
    PointF& vertex)
{
    vertex.x = static_cast<float>(point31.x - primitivizedArea->area31.left) / primitivizedArea->scale31ToPixelDivisor.x;
    vertex.y = static_cast<float>(point31.y - primitivizedArea->area31.top) / primitivizedArea->scale31ToPixelDivisor.y;
}

bool OsmAnd::Rasterizer_P::containsHelper(const QVector< PointI >& points, const PointI& otherPoint)
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

void OsmAnd::Rasterizer_P::rasterizeSymbolsWithoutPaths(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
    std::function<bool (const std::shared_ptr<const Model::BinaryMapObject>& mapObject)> filter,
    const IQueryController* const controller)
{
    const auto& env = primitivizedArea->mapPresentationEnvironment;

    for (const auto& symbolsEntry : rangeOf(constOf(primitivizedArea->symbolsBySourceObjects)))
    {
        if (controller && controller->isAborted())
            return;

        // Apply filter, if it's present
        if (filter && !filter(symbolsEntry.key()))
            continue;

        // Create group
        const auto constructedGroup = new RasterizedSymbolsGroup(symbolsEntry.key());
        std::shared_ptr<const RasterizedSymbolsGroup> group(constructedGroup);

        // Total offset allows several texts to stack into column
        PointI totalOffset;

        for (const auto& symbol : constOf(symbolsEntry.value()))
        {
            if (controller && controller->isAborted())
                return;

            if (const auto& textSymbol = std::dynamic_pointer_cast<const Primitiviser::TextSymbol>(symbol))
            {
                const auto text = ICU::convertToVisualOrder(textSymbol->value);
                const auto lineRefs =
                    (textSymbol->wrapWidth > 0 && !textSymbol->drawOnPath && textSymbol->shieldResourceName.isEmpty())
                    ? ICU::getTextWrappingRefs(text, textSymbol->wrapWidth)
                    : (QVector<QStringRef>() << QStringRef(&text));
                const auto& linesCount = lineRefs.size();

                // Obtain shield for text if such exists
                std::shared_ptr<const SkBitmap> textShieldBitmap;
                if (!textSymbol->shieldResourceName.isEmpty())
                    env->obtainTextShield(textSymbol->shieldResourceName, textShieldBitmap);

                // Get base text settings from environment
                SkPaint textPaint = env->textPaint;

                // Configure paint for text
                env->configurePaintForText(textPaint, text, textSymbol->isBold, false);
                textPaint.setTextSize(textSymbol->size);
                textPaint.setColor(textSymbol->color);

                // Get line spacing
                SkPaint::FontMetrics fontMetrics;
                auto fullLineHeight = textPaint.getFontMetrics(&fontMetrics);
                auto lineSpacing = fontMetrics.fLeading;
                auto fontMaxTop = -fontMetrics.fTop;
                auto fontMaxBottom = fontMetrics.fBottom;

                // Measure text
                QVector<SkRect> linesBounds(linesCount);
                auto itLineBounds = linesBounds.begin();
                for (const auto& lineRef : constOf(lineRefs))
                    textPaint.measureText(lineRef.constData(), lineRef.length()*sizeof(QChar), &*(itLineBounds++));

                // Measure glyphs
                QVector<SkScalar> glyphsWidth;
                if (textSymbol->drawOnPath)
                {
                    const auto& lineRef = lineRefs.first();

                    const auto glyphsCount = textPaint.countText(lineRef.constData(), lineRef.length()*sizeof(QChar));
                    glyphsWidth.resize(glyphsCount);
                    textPaint.getTextWidths(lineRef.constData(), lineRef.length()*sizeof(QChar), glyphsWidth.data());
                }

                // Process shadow
                SkPaint textShadowPaint;
                if (textSymbol->shadowRadius > 0)
                {
                    // Configure paint for text shadow
                    textShadowPaint = textPaint;
                    textShadowPaint.setStyle(SkPaint::kStroke_Style);
                    textShadowPaint.setColor(textSymbol->shadowColor);
                    textShadowPaint.setStrokeWidth(textSymbol->shadowRadius + 2 /*px*/);
                    //NOTE: ^^^ This is same as specifying 'x:2' in style, but due to backward compatibility with Android, leave as-is

                    // Get line spacing
                    SkPaint::FontMetrics shadowFontMetrics;
                    const auto fullShadowLineHeight = textShadowPaint.getFontMetrics(&shadowFontMetrics);
                    fullLineHeight = qMax(fullLineHeight, fullShadowLineHeight);
                    lineSpacing = qMax(lineSpacing, shadowFontMetrics.fLeading);
                    fontMaxTop = qMax(fontMaxTop, -shadowFontMetrics.fTop);
                    fontMaxBottom = qMax(fontMaxBottom, shadowFontMetrics.fBottom);

                    // Measure text shadow bounds
                    auto itLineBounds = linesBounds.begin();
                    for (const auto& lineRef : constOf(lineRefs))
                    {
                        SkRect lineShadowBounds;
                        textShadowPaint.measureText(lineRef.constData(), lineRef.length()*sizeof(QChar), &lineShadowBounds);

                        // Combine shadow bounds with text bounds
                        (itLineBounds++)->join(lineShadowBounds);
                    }

                    // Re-measure glyphs, since shadow is larger than text itself
                    if (textSymbol->drawOnPath)
                    {
                        const auto& lineRef = lineRefs.first();

                        textShadowPaint.getTextWidths(lineRef.constData(), lineRef.length()*sizeof(QChar), glyphsWidth.data());
                    }
                }

                // Calculate extra top and bottom space of symbol
                auto symbolExtraTopSpace = qMax(0.0f, fontMaxTop - (-linesBounds.first().fTop));
                auto symbolExtraBottomSpace = qMax(0.0f, fontMaxBottom - linesBounds.last().fBottom);

                // Shift first glyph width
                if (!glyphsWidth.isEmpty())
                    glyphsWidth[0] += -linesBounds.first().left();

                // Normalize line bounds (move origin top bottom-left corner of bitmap)
                QVector<SkRect> linesNormalizedBounds(linesCount);
                auto itNormalizedLineBounds = linesNormalizedBounds.begin();
                for (auto& lineBounds : linesBounds)
                {
                    auto& normalizedLineBounds = *(itNormalizedLineBounds++);

                    normalizedLineBounds = lineBounds;
                    normalizedLineBounds.offset(-2.0f*lineBounds.left(), -2.0f*lineBounds.top());
                }

                // Calculate text area and move bounds vertically
                auto textArea = linesNormalizedBounds.first();
                auto linesHeightSum = textArea.height();
                auto citPrevLineBounds = linesBounds.cbegin();
                auto citLineBounds = citPrevLineBounds + 1;
                for (auto itNormalizedLineBounds = linesNormalizedBounds.begin() + 1, itEnd = linesNormalizedBounds.end(); itNormalizedLineBounds != itEnd; ++itNormalizedLineBounds, citPrevLineBounds = citLineBounds, ++citLineBounds)
                {
                    auto& lineNormalizedBounds = *itNormalizedLineBounds;
                    const auto& prevLineBounds = *citPrevLineBounds;
                    const auto& lineBounds = *citLineBounds;

                    // Include gap between previous line and it's font-end
                    const auto extraPrevGapHeight = qMax(0.0f, fontMaxBottom - prevLineBounds.fBottom);
                    textArea.fBottom += extraPrevGapHeight;
                    linesHeightSum += extraPrevGapHeight;

                    // Include line spacing
                    textArea.fBottom += lineSpacing;
                    linesHeightSum += lineSpacing;

                    // Include gap between current line and it's font-start
                    const auto extraGapHeight = qMax(0.0f, fontMaxTop - (-lineBounds.fTop));
                    textArea.fBottom += extraGapHeight;
                    linesHeightSum += extraGapHeight;

                    // Move current line baseline
                    lineNormalizedBounds.offset(0.0f, linesHeightSum);

                    // Include height of current line
                    const auto& lineHeight = lineNormalizedBounds.height();
                    textArea.fBottom += lineHeight;
                    linesHeightSum += lineHeight;

                    // This will expand left-right bounds to get proper area width
                    textArea.fLeft = qMin(textArea.fLeft, lineNormalizedBounds.fLeft);
                    textArea.fRight = qMax(textArea.fRight, lineNormalizedBounds.fRight);
                }

                // Calculate bitmap size
                auto bitmapWidth = qCeil(textArea.width());
                auto bitmapHeight = qCeil(textArea.height());
                if (textShieldBitmap)
                {
                    // Clear extra spacing
                    symbolExtraTopSpace = 0.0f;
                    symbolExtraBottomSpace = 0.0f;

                    // Enlarge bitmap if shield is larger than text
                    bitmapWidth = qMax(bitmapWidth, textShieldBitmap->width());
                    bitmapHeight = qMax(bitmapHeight, textShieldBitmap->height());

                    // Shift text area to proper position in a larger
                    auto& firstLineNormalizedBounds = linesNormalizedBounds.first();
                    firstLineNormalizedBounds.offset(
                        (bitmapWidth - qCeil(firstLineNormalizedBounds.width())) / 2.0f,
                        (bitmapHeight - qCeil(firstLineNormalizedBounds.height())) / 2.0f);
                }

                // Check if bitmap size was successfully calculated
                if (bitmapWidth <= 0 || bitmapHeight <= 0)
                {
                    LogPrintf(LogSeverityLevel::Error, "Failed to rasterize symbol with text '%s'", qPrintable(text));
                    continue;
                }

                // Create a bitmap that will be hold entire symbol
                const auto pBitmap = new SkBitmap();
                pBitmap->setConfig(SkBitmap::kARGB_8888_Config, bitmapWidth, bitmapHeight);
                pBitmap->allocPixels();
                pBitmap->eraseColor(SK_ColorTRANSPARENT);
                SkBitmapDevice target(*pBitmap);
                SkCanvas canvas(&target);

                // If there is shield for this text, rasterize it also
                if (textShieldBitmap)
                {
                    canvas.drawBitmap(*textShieldBitmap,
                        (bitmapWidth - textShieldBitmap->width()) / 2.0f,
                        (bitmapHeight - textShieldBitmap->height()) / 2.0f,
                        nullptr);
                }

                // Rasterize text shadow first (if enabled)
                if (textSymbol->shadowRadius > 0)
                {
                    auto itLineShadowNormalizedBounds = linesNormalizedBounds.cbegin();
                    for (const auto& lineRef : constOf(lineRefs))
                    {
                        const auto& lineShadowNormalizedBounds = *(itLineShadowNormalizedBounds++);
                        canvas.drawText(
                            lineRef.constData(), lineRef.length()*sizeof(QChar),
                            lineShadowNormalizedBounds.left(), lineShadowNormalizedBounds.top(),
                            textShadowPaint);
                    }
                }

                // Rasterize text itself
                auto citLineNormalizedBounds = linesNormalizedBounds.cbegin();
                for (const auto& lineRef : constOf(lineRefs))
                {
                    const auto& lineNormalizedBounds = *(citLineNormalizedBounds++);
                    canvas.drawText(
                        lineRef.constData(), lineRef.length()*sizeof(QChar),
                        lineNormalizedBounds.left(), lineNormalizedBounds.top(),
                        textPaint);
                }

#if OSMAND_DUMP_SYMBOLS
                {
                    QDir::current().mkpath("text_symbols");
                    std::unique_ptr<SkImageEncoder> encoder(CreatePNGImageEncoder());
                    QString filename;
                    filename.sprintf("%s\\text_symbols\\%p.png", qPrintable(QDir::currentPath()), pBitmap);
                    encoder->encodeFile(qPrintable(filename), *pBitmap, 100);
                }
#endif // OSMAND_DUMP_SYMBOLS

                if (textSymbol->drawOnPath)
                {
                    // Publish new rasterized symbol
                    const auto rasterizedSymbol = new RasterizedOnPathSymbol(
                        group,
                        constructedGroup->mapObject,
                        qMove(std::shared_ptr<const SkBitmap>(pBitmap)),
                        textSymbol->order,
                        text,
                        textSymbol->languageId,
                        textSymbol->minDistance,
                        glyphsWidth);
                    assert(static_cast<bool>(rasterizedSymbol->bitmap));
                    constructedGroup->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));
                }
                else
                {
                    // Calculate local offset
                    PointI localOffset;
                    localOffset.y = ((symbolExtraTopSpace + pBitmap->height() + symbolExtraBottomSpace) / 2) + textSymbol->verticalOffset;

                    // Increment total offset
                    totalOffset += localOffset;

                    // Publish new rasterized symbol
                    const auto rasterizedSymbol = new RasterizedSpriteSymbol(
                        group,
                        constructedGroup->mapObject,
                        qMove(std::shared_ptr<const SkBitmap>(pBitmap)),
                        textSymbol->order,
                        text,
                        textSymbol->languageId,
                        textSymbol->minDistance,
                        textSymbol->location31,
                        (constructedGroup->symbols.isEmpty() ? PointI() : totalOffset));
                    assert(static_cast<bool>(rasterizedSymbol->bitmap));
                    constructedGroup->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));
                }
            }
            else if (const auto& iconSymbol = std::dynamic_pointer_cast<const Primitiviser::IconSymbol>(symbol))
            {
                std::shared_ptr<const SkBitmap> bitmap;
                if (!env->obtainMapIcon(iconSymbol->resourceName, bitmap) || !bitmap)
                    continue;

#if OSMAND_DUMP_SYMBOLS
                {
                    QDir::current().mkpath("icon_symbols");
                    std::unique_ptr<SkImageEncoder> encoder(CreatePNGImageEncoder());
                    QString filename;
                    filename.sprintf("%s\\text_symbols\\%p.png", qPrintable(QDir::currentPath()), bitmap.get());
                    encoder->encodeFile(qPrintable(filename), *bitmap, 100);
                }
#endif // OSMAND_DUMP_SYMBOLS

                // Calculate local offset
                PointI localOffset;
                localOffset.y = (bitmap->height() / 2);

                // Increment total offset
                totalOffset += localOffset;

                // Publish new rasterized symbol
                const auto rasterizedSymbol = new RasterizedSpriteSymbol(
                    group,
                    constructedGroup->mapObject,
                    qMove(bitmap),
                    iconSymbol->order,
                    iconSymbol->resourceName,
                    LanguageId::Invariant,
                    PointI(),
                    iconSymbol->location31,
                    (constructedGroup->symbols.isEmpty() ? PointI() : totalOffset));
                assert(static_cast<bool>(rasterizedSymbol->bitmap));
                constructedGroup->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));
            }
        }

        // Add group to output
        outSymbolsGroups.push_back(qMove(group));
    }
}
