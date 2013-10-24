#include "Rasterizer_P.h"
#include "Rasterizer.h"

#include <cassert>
#include <tr1/cinttypes>
#include <set>

#include "RasterizerEnvironment.h"
#include "RasterizerEnvironment_P.h"
#include "RasterizerContext.h"
#include "RasterizerContext_P.h"
#include "MapStyleEvaluator.h"
#include "MapTypes.h"
#include "MapObject.h"
#include "ObfMapSectionInfo.h"
#include "IQueryController.h"
#include "Utilities.h"
#include "Logging.h"

#include <SkError.h>
#include <SkBlurDrawLooper.h>
#include <SkColorFilter.h>
#include <SkDashPathEffect.h>
#include <SkBitmapProcShader.h>

OsmAnd::Rasterizer_P::Rasterizer_P()
{
}

OsmAnd::Rasterizer_P::~Rasterizer_P()
{
}

void OsmAnd::Rasterizer_P::prepareContext(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const AreaI& area31, const ZoomLevel zoom, const uint32_t tileSize,
    const MapFoundationType& foundation,
    const QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& objects,
    const PointF& tlOriginOffset, bool* nothingToRasterize,
    IQueryController* controller)
{
    context.clear();

    context._tileDivisor = Utilities::getPowZoom(31 - zoom);
    adjustContextFromEnvironment(env, context, zoom);
    
    context._precomputed31toPixelDivisor = context._tileDivisor / tileSize;
    
    const auto pixelWidth = static_cast<float>(area31.width()) / context._precomputed31toPixelDivisor;
    const auto pixelHeight = static_cast<float>(area31.height()) / context._precomputed31toPixelDivisor;
    context._renderViewport.topLeft = tlOriginOffset;
    context._renderViewport.right = tlOriginOffset.x + pixelWidth;
    context._renderViewport.bottom = pixelHeight - tlOriginOffset.x;

    context._zoom = zoom;
    context._area31 = area31;
    context._tileSize = tileSize;

    // Split input map objects to object, coastline, basemapObjects and basemapCoastline
    for(auto itMapObject = objects.cbegin(); itMapObject != objects.cend(); ++itMapObject)
    {
        if(controller && controller->isAborted())
            break;

        const auto& mapObject = *itMapObject;

        if(zoom < BasemapZoom && !mapObject->section->isBasemap)
            continue;

        if(mapObject->containsType(QString::fromLatin1("natural"), QString::fromLatin1("coastline")))
        {
            if (mapObject->section->isBasemap)
                context._basemapCoastlineObjects.push_back(mapObject);
            else
                context._coastlineObjects.push_back(mapObject);
        }
        else
        {
            if (mapObject->section->isBasemap)
                context._basemapMapObjects.push_back(mapObject);
            else
                context._mapObjects.push_back(mapObject);
        }
    }

    // Cleanup if aborted
    if(controller && controller->isAborted())
    {
        context.clear();
        return;
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

        std::shared_ptr<Model::MapObject> bgMapObject(new Model::MapObject(nullptr, nullptr));
        bgMapObject->_isArea = true;
        bgMapObject->_points31.push_back(PointI(area31.left, area31.top));
        bgMapObject->_points31.push_back(PointI(area31.right, area31.top));
        bgMapObject->_points31.push_back(PointI(area31.right, area31.bottom));
        bgMapObject->_points31.push_back(PointI(area31.left, area31.bottom));
        bgMapObject->_points31.push_back(bgMapObject->_points31.first());
        if(foundation == MapFoundationType::FullWater)
            bgMapObject->_types.push_back(TagValue(QString::fromLatin1("natural"), QString::fromLatin1("coastline")));
        else if(foundation == MapFoundationType::FullLand || foundation == MapFoundationType::Mixed)
            bgMapObject->_types.push_back(TagValue(QString::fromLatin1("natural"), QString::fromLatin1("land")));
        else
        {
            bgMapObject->_isArea = false;
            bgMapObject->_types.push_back(TagValue(QString::fromLatin1("natural"), QString::fromLatin1("coastline_broken")));
        }
        bgMapObject->_extraTypes.push_back(TagValue(QString::fromLatin1("layer"), QString::fromLatin1("-5")));

        assert(bgMapObject->isClosedFigure());
        context._triangulatedCoastlineObjects.push_back(bgMapObject);
    }
    
    // Obtain primitives
    const bool detailedDataMissing = zoom > BasemapZoom && context._mapObjects.isEmpty() && context._coastlineObjects.isEmpty();

    context._combinedMapObjects << context._mapObjects;
    if(zoom <= BasemapZoom || detailedDataMissing)
        context._combinedMapObjects << context._basemapMapObjects;
    context._combinedMapObjects << context._triangulatedCoastlineObjects;

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

    obtainPrimitives(env, context, controller);
    if(controller && controller->isAborted())
    {
        context.clear();
        return;
    }

    /*context._texts.clear();
    obtainPrimitivesTexts(env, context, controller);
    if(controller && controller->isAborted())
    {
        context.clear();
        return;
    }*/
}

void OsmAnd::Rasterizer_P::adjustContextFromEnvironment(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const ZoomLevel zoom)
{
    context._mapPaint = env.mapPaint;

    context._defaultBgColor = env.defaultBgColor;
    if(env.attributeRule_defaultColor)
    {
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor, env.attributeRule_defaultColor);

        env.applyTo(evaluator);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
        if(evaluator.evaluate())
            evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_ATTR_COLOR_VALUE, context._defaultBgColor);
    }

    context._shadowRenderingMode = env.shadowRenderingMode;
    context._shadowRenderingColor = env.shadowRenderingColor;
    if(env.attributeRule_shadowRendering)
    {
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor, env.attributeRule_shadowRendering);


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
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor, env.attributeRule_polygonMinSizeToDisplay);
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
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor, env.attributeRule_roadDensityZoomTile);
        env.applyTo(evaluator);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
        if(evaluator.evaluate())
            evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE, context._roadDensityZoomTile);
    }

    context._roadsDensityLimitPerTile = env.roadsDensityLimitPerTile;
    if(env.attributeRule_roadsDensityLimitPerTile)
    {
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor, env.attributeRule_roadsDensityLimitPerTile);
        env.applyTo(evaluator);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
        if(evaluator.evaluate())
            evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE, context._roadsDensityLimitPerTile);
    }

    context._shadowLevelMin = env.shadowLevelMin;
    context._shadowLevelMax = env.shadowLevelMax;
}

bool OsmAnd::Rasterizer_P::rasterizeMap(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    bool fillBackground,
    SkCanvas& canvas,
    IQueryController* controller /*= nullptr*/)
{
    if(fillBackground)
    {
        SkPaint bgPaint;
        bgPaint.setColor(context._defaultBgColor);
        bgPaint.setStyle(SkPaint::kFill_Style);
        canvas.drawRectCoords(context._renderViewport.top, context._renderViewport.left, context._renderViewport.right, context._renderViewport.bottom, bgPaint);
    }

    rasterizeMapPrimitives(env, context, canvas, context._polygons, Polygons, controller);

    if (context._shadowRenderingMode > 1)
        rasterizeMapPrimitives(env, context, canvas, context._lines, ShadowOnlyLines, controller);

    rasterizeMapPrimitives(env, context, canvas, context._lines, Lines, controller);

    //////////////////////////////////////////////////////////////////////////
    SkPaint textPaint;
    textPaint.setColor(0xFFFF0000);
    textPaint.setAntiAlias(true);
    textPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    textPaint.setTextSize(24);
    {
        const auto data = L"Tokyo [\u6771\u4eac]";
        canvas.drawText(data, wcslen(data) * sizeof(wchar_t), 10, 20, textPaint);
    }
    {
        const auto data = L"al-Qahira [\u0627\u0644\u0642\u0627\u0647\u0631\u0629]";
        canvas.drawText(data, wcslen(data) * sizeof(wchar_t), 10, 40, textPaint);
    }
    {
        const auto data = L"Beijing [\u5317\u4eac]";
        canvas.drawText(data, wcslen(data) * sizeof(wchar_t), 10, 60, textPaint);
    }
    {
        const auto data = L"Kanada [\u0c95\u0ca8\u0ccd\u0ca8\u0ca1]";
        canvas.drawText(data, wcslen(data) * sizeof(wchar_t), 10, 80, textPaint);
    }
    {
        const auto data = L"Jerusalem [\u05d9\u05b0\u05e8\u05d5\u05bc\u05e9\u05b8\u05c1\u05dc\u05b7\u05d9\u05b4\u05dd]";
        canvas.drawText(data, wcslen(data) * sizeof(wchar_t), 10, 100, textPaint);
    }
    auto lr = SkGetLastError();
    //////////////////////////////////////////////////////////////////////////

    return true;
}

void OsmAnd::Rasterizer_P::obtainPrimitives(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    IQueryController* controller)
{
    const auto area31toPixelDivisor = context._precomputed31toPixelDivisor * context._precomputed31toPixelDivisor;
    
    QVector< Primitive > unfilteredLines;
    for(auto itMapObject = context._combinedMapObjects.cbegin(); itMapObject != context._combinedMapObjects.cend(); ++itMapObject)
    {
        if(controller && controller->isAborted())
            return;

        auto mapObject = *itMapObject;
        
        uint32_t typeIdx = 0;
        for(auto itType = mapObject->types.cbegin(); itType != mapObject->types.cend(); ++itType, typeIdx++)
        {
            const auto& type = *itType;
            auto layer = mapObject->getSimpleLayerValue();

            MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Order, mapObject);
            env.applyTo(evaluator);
            evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, type.tag);
            evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, type.value);
            evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
            evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
            evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_LAYER, layer);
            evaluator.setBooleanValue(MapStyle::builtinValueDefinitions.INPUT_AREA, mapObject->isArea);
            evaluator.setBooleanValue(MapStyle::builtinValueDefinitions.INPUT_POINT, mapObject->points31.size() == 1);
            evaluator.setBooleanValue(MapStyle::builtinValueDefinitions.INPUT_CYCLE, mapObject->isClosedFigure());
            if(!evaluator.evaluate())
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
            primitive.typeIndex = typeIdx;

            if(objectType == PrimitiveType::Polygon)
            {
                if(mapObject->points31.size() <=2)
                {
                    OsmAnd::LogPrintf(LogSeverityLevel::Warning,
                        "Map object #%" PRIu64 " (%" PRIi64 ") primitives are processed as polygon, but only %d vertices present",
                        mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2,
                        mapObject->points31.size());
                    continue;
                }
                if(!mapObject->isClosedFigure())
                {
                    OsmAnd::LogPrintf(LogSeverityLevel::Warning,
                        "Map object #%" PRIu64 " (%" PRIi64 ") primitives are processed as polygon, but are not closed",
                        mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2);
                    continue;
                }
                if(!mapObject->isClosedFigure(true))
                {
                    OsmAnd::LogPrintf(LogSeverityLevel::Warning,
                        "Map object #%" PRIu64 " (%" PRIi64 ") primitives are processed as polygon, but are not closed (inner)",
                        mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2);
                    continue;
                }

                Primitive pointPrimitive = primitive;
                pointPrimitive.objectType = PrimitiveType::Point;
                auto polygonArea31 = Utilities::polygonArea(mapObject->points31);
                if(polygonArea31 > PolygonAreaCutoffLowerThreshold)
                {
                    primitive.zOrder += 1.0 / (polygonArea31/area31toPixelDivisor);
                    context._polygons.push_back(primitive);
                    context._points.push_back(pointPrimitive);
                }
            }
            else if(objectType == PrimitiveType::Point)
            {
                context._points.push_back(primitive);
            }
            else if(objectType == PrimitiveType::Line)
            {
                unfilteredLines.push_back(primitive);
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
            if(l.typeIndex == r.typeIndex)
                return l.mapObject->_points31.size() < r.mapObject->_points31.size();
            return l.typeIndex < r.typeIndex;
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
    const QVector< Primitive >& in, QVector< Primitive >& out, IQueryController* controller )
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

        const auto& type = primitive.mapObject->_types[primitive.typeIndex];
        if(type.tag == QLatin1String("highway"))
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

void OsmAnd::Rasterizer_P::rasterizeMapPrimitives(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    SkCanvas& canvas, const QVector< Primitive >& primitives, PrimitivesType type, IQueryController* controller )
{
    assert(type != PrimitivesType::Points);

    const auto polygonSizeThreshold = 1.0 / (context._polygonMinSizeToDisplay * env.owner->displayDensityFactor*env.owner->displayDensityFactor);
    for(auto itPrimitive = primitives.cbegin(); itPrimitive != primitives.cend(); ++itPrimitive)
    {
        if(controller && controller->isAborted())
            return;

        const auto& primitive = *itPrimitive;

        if(type == Polygons)
        {
            if (primitive.zOrder > polygonSizeThreshold + static_cast<int>(primitive.zOrder))
                continue;

            rasterizePolygon(env, context, canvas, primitive);
        }
        else if(type == Lines || type == ShadowOnlyLines)
        {
            rasterizeLine(env, context, canvas, primitive, type == ShadowOnlyLines);
        }
    }
}

bool OsmAnd::Rasterizer_P::updatePaint(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
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
        context._mapPaint.setColorFilter(nullptr);
        context._mapPaint.setShader(nullptr);
        context._mapPaint.setLooper(nullptr);
        context._mapPaint.setStyle(SkPaint::kStrokeAndFill_Style);
        context._mapPaint.setStrokeWidth(0);
    }
    else
    {
        float stroke;
        ok = evaluator.getFloatValue(valueSet.strokeWidth, stroke);
        if(!ok || stroke <= 0.0f)
            return false;

        context._mapPaint.setColorFilter(nullptr);
        context._mapPaint.setShader(nullptr);
        context._mapPaint.setLooper(nullptr);
        context._mapPaint.setStyle(SkPaint::kStroke_Style);
        context._mapPaint.setStrokeWidth(stroke);

        QString cap;
        ok = evaluator.getStringValue(valueSet.cap, cap);
        if (!ok || cap.isEmpty() || cap == QLatin1String("BUTT"))
            context._mapPaint.setStrokeCap(SkPaint::kButt_Cap);
        else if (cap == QLatin1String("ROUND"))
            context._mapPaint.setStrokeCap(SkPaint::kRound_Cap);
        else if (cap == QLatin1String("SQUARE"))
            context._mapPaint.setStrokeCap(SkPaint::kSquare_Cap);
        else
            context._mapPaint.setStrokeCap(SkPaint::kButt_Cap);

        QString pathEff;
        ok = evaluator.getStringValue(valueSet.pathEffect, pathEff);
        if(!ok || pathEff.isEmpty())
        {
            context._mapPaint.setPathEffect(nullptr);
        }
        else
        {
            auto effect = obtainPathEffect(env, context, pathEff);
            context._mapPaint.setPathEffect(effect);
        }
    }

    SkColor color;
    ok = evaluator.getIntegerValue(valueSet.color, color);
    if(!ok || !color)
        return false;
    context._mapPaint.setColor(color);

    if (valueSetSelector == PaintValuesSet::Set_0)
    {
        QString shader;
        ok = evaluator.getStringValue(MapStyle::builtinValueDefinitions.OUTPUT_SHADER, shader);
        if(ok && !shader.isEmpty())
        {
            SkBitmapProcShader* shaderObj = nullptr;
            if(env.obtainBitmapShader(shader, shaderObj) && shaderObj)
            {
                context._mapPaint.setShader(static_cast<SkShader*>(shaderObj));
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
            context._mapPaint.setLooper(new SkBlurDrawLooper(static_cast<SkScalar>(shadowRadius), 0, 0, shadowColor))->unref();
    }

    return true;
}

SkPathEffect* OsmAnd::Rasterizer_P::obtainPathEffect( const RasterizerEnvironment_P& env, RasterizerContext_P& context, const QString& pathEffect )
{
    auto itEffect = context._pathEffects.constFind(pathEffect);
    if(itEffect != context._pathEffects.cend())
        return *itEffect;

    const auto& strIntervals = pathEffect.split('_', QString::SkipEmptyParts);

    SkScalar* intervals = new SkScalar[strIntervals.size()];
    uint32_t idx = 0;
    for(auto itInterval = strIntervals.cbegin(); itInterval != strIntervals.cend(); ++itInterval, idx++)
        intervals[idx] = itInterval->toFloat();

    SkPathEffect* pPathEffect = new SkDashPathEffect(intervals, strIntervals.size(), 0);
    delete[] intervals;
    context._pathEffects.insert(pathEffect, pPathEffect);
    return pPathEffect;
}

void OsmAnd::Rasterizer_P::rasterizePolygon(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    SkCanvas& canvas, const Primitive& primitive )
{
    if(primitive.mapObject->_points31.size() <=2)
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning,
            "Map object #%" PRIu64 " (%" PRIi64 ") is rasterized as polygon, but has %d vertices",
            primitive.mapObject->id >> 1, static_cast<int64_t>(primitive.mapObject->id) / 2,
            primitive.mapObject->_points31.size());
        return;
    }
    if(!primitive.mapObject->isClosedFigure())
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning,
            "Map object #%" PRIu64 " (%" PRIi64 ") is rasterized as polygon, but is not closed",
            primitive.mapObject->id >> 1, static_cast<int64_t>(primitive.mapObject->id) / 2);
        return;
    }
    if(!primitive.mapObject->isClosedFigure(true))
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning,
            "Map object #%" PRIu64 " (%" PRIi64 ") is rasterized as polygon, but is not closed (inner)",
            primitive.mapObject->id >> 1, static_cast<int64_t>(primitive.mapObject->id) / 2);
        return;
    }

    const auto& type = primitive.mapObject->_types[primitive.typeIndex];

    MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Polygon, primitive.mapObject);
    env.applyTo(evaluator);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, type.tag);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, type.value);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
    if(!evaluator.evaluate())
        return;
    if(!updatePaint(env, context, evaluator, Set_0, true))
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

        calculateVertex(env, context, point, vertex);

        if(pointIdx == 0)
        {
            path.moveTo(vertex.x, vertex.y);
        }
        else
        {
            path.lineTo(vertex.x, vertex.y);
        }

        if(!containsAtLeastOnePoint)
        {
            if(context._renderViewport.contains(vertex))
            {
                containsAtLeastOnePoint = true;
            }
            else
            {
                outsideBounds.push_back(vertex);
            }
            bounds |= (vertex.x < context._renderViewport.left ? 1 : 0);
            bounds |= (vertex.x > context._renderViewport.right ? 2 : 0);
            bounds |= (vertex.y < context._renderViewport.top ? 4 : 0);
            bounds |= (vertex.y > context._renderViewport.bottom ? 8 : 0);
        }
    }

    if(!containsAtLeastOnePoint)
    {
        // fast check for polygons
        if((bounds & 3) != 3 || (bounds >> 2) != 3)
            return;

        bool ok = true;
        ok = ok || contains(outsideBounds, context._renderViewport.topLeft);
        ok = ok || contains(outsideBounds, context._renderViewport.bottomRight);
        ok = ok || contains(outsideBounds, PointF(0, context._renderViewport.bottom));
        ok = ok || contains(outsideBounds, PointF(context._renderViewport.right, 0));
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
                calculateVertex(env, context, point, vertex);

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

    canvas.drawPath(path, context._mapPaint);
    if(updatePaint(env, context, evaluator, Set_1, false))
        canvas.drawPath(path, context._mapPaint);
}

void OsmAnd::Rasterizer_P::rasterizeLine(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    SkCanvas& canvas, const Primitive& primitive, bool drawOnlyShadow )
{
    if(primitive.mapObject->_points31.size() < 2 )
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning,
            "Map object #%" PRIu64 " (%" PRIi64 ") is rasterized as line, but has %d vertices",
            primitive.mapObject->id >> 1, static_cast<int64_t>(primitive.mapObject->id) / 2,
            primitive.mapObject->_points31.size());
        return;
    }

    bool ok;
    const auto& type = primitive.mapObject->_types[primitive.typeIndex];

    MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Line, primitive.mapObject);
    env.applyTo(evaluator);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, type.tag);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, type.value);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_LAYER, primitive.mapObject->getSimpleLayerValue());
    if(!evaluator.evaluate())
        return;
    if(!updatePaint(env, context, evaluator, Set_0, false))
        return;

    int shadowColor;
    ok = evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_SHADOW_COLOR, shadowColor);
    if(!ok || shadowColor == 0)
        shadowColor = context._shadowRenderingColor;

    int shadowRadius;
    ok = evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_SHADOW_RADIUS, shadowRadius);
    if(drawOnlyShadow && (!ok || shadowRadius == 0))
        return;
    
    int oneway = 0;
    if (context._zoom >= 16 && type.tag == QLatin1String("highway"))
    {
        if (primitive.mapObject->containsType(QLatin1String("oneway"), QLatin1String("yes"), true))
            oneway = 1;
        else if (primitive.mapObject->containsType(QLatin1String("oneway"), QLatin1String("-1"), true))
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

        calculateVertex(env, context, point, vertex);

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

        if (!intersect)
        {
            if(context._renderViewport.contains(vertex))
            {
                intersect = true;
            }
            else
            {
                int cross = 0;
                cross |= (vertex.x < context._renderViewport.left ? 1 : 0);
                cross |= (vertex.x > context._renderViewport.right ? 2 : 0);
                cross |= (vertex.y < context._renderViewport.top ? 4 : 0);
                cross |= (vertex.y > context._renderViewport.bottom ? 8 : 0);
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

    if (!intersect)
        return;

    if (pointIdx > 0)
    {
        if (drawOnlyShadow)
        {
            rasterizeLineShadow(env, context, canvas, path, shadowColor, shadowRadius);
        }
        else
        {
            if (updatePaint(env, context, evaluator, Set_minus2, false))
            {
                canvas.drawPath(path, context._mapPaint);
            }
            if (updatePaint(env, context, evaluator, Set_minus1, false))
            {
                canvas.drawPath(path, context._mapPaint);
            }
            if (updatePaint(env, context, evaluator, Set_0, false))
            {
                canvas.drawPath(path, context._mapPaint);
            }
            canvas.drawPath(path, context._mapPaint);
            if (updatePaint(env, context, evaluator, Set_1, false))
            {
                canvas.drawPath(path, context._mapPaint);
            }
            if (updatePaint(env, context, evaluator, Set_3, false))
            {
                canvas.drawPath(path, context._mapPaint);
            }
            if (oneway && !drawOnlyShadow)
            {
                rasterizeLine_OneWay(env, context, canvas, path, oneway);
            }
        }
    }
}

void OsmAnd::Rasterizer_P::rasterizeLineShadow(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    SkCanvas& canvas, const SkPath& path, uint32_t shadowColor, int shadowRadius )
{
    // blurred shadows
    if (context._shadowRenderingMode == 2 && shadowRadius > 0)
    {
        // simply draw shadow? difference from option 3 ?
        // paint->setColor(0xffffffff);
        context._mapPaint.setLooper(new SkBlurDrawLooper(shadowRadius, 0, 0, shadowColor))->unref();
        canvas.drawPath(path, context._mapPaint);
    }

    // option shadow = 3 with solid border
    if (context._shadowRenderingMode == 3 && shadowRadius > 0)
    {
        context._mapPaint.setLooper(nullptr);
        context._mapPaint.setStrokeWidth(context._mapPaint.getStrokeWidth() + shadowRadius*2);
        //		paint->setColor(0xffbababa);
        context._mapPaint.setColorFilter(SkColorFilter::CreateModeFilter(shadowColor, SkXfermode::kSrcIn_Mode))->unref();
        //		paint->setColor(shadowColor);
        canvas.drawPath(path, context._mapPaint);
    }
}

void OsmAnd::Rasterizer_P::rasterizeLine_OneWay(
    const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
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

void OsmAnd::Rasterizer_P::calculateVertex(
    const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
    const PointI& point31, PointF& vertex )
{
    vertex.x = static_cast<float>(point31.x - context._area31.left) / context._precomputed31toPixelDivisor + context._renderViewport.left;
    vertex.y = static_cast<float>(point31.y - context._area31.top) / context._precomputed31toPixelDivisor + context._renderViewport.top;
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

        std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(nullptr, nullptr));
        mapObject->_isArea = false;
        mapObject->_points31 = polyline;
        mapObject->_types.push_back(TagValue(QString::fromLatin1("natural"), QString::fromLatin1("coastline_line")));

        outVectorized.push_back(mapObject);
    }

    const bool coastlineCrossesBounds = !coastlinePolylines.isEmpty();
    if(!coastlinePolylines.isEmpty())
    {
        // Add complete water tile with holes
        std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(nullptr, nullptr));
        mapObject->_points31.push_back(PointI(context._area31.left, context._area31.top));
        mapObject->_points31.push_back(PointI(context._area31.right, context._area31.top));
        mapObject->_points31.push_back(PointI(context._area31.right, context._area31.bottom));
        mapObject->_points31.push_back(PointI(context._area31.left, context._area31.bottom));
        mapObject->_points31.push_back(mapObject->_points31.first());
        convertCoastlinePolylinesToPolygons(env, context, coastlinePolylines, mapObject->_innerPolygonsPoints31, osmId);

        mapObject->_types.push_back(TagValue(QString::fromLatin1("natural"), QString::fromLatin1("coastline")));
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

            std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(nullptr, nullptr));
            mapObject->_isArea = false;
            mapObject->_points31 = polygon;
            mapObject->_types.push_back(TagValue(QString::fromLatin1("natural"), QString::fromLatin1("coastline_broken")));

            outVectorized.push_back(mapObject);
        }
    }

    // Draw coastlines
    for(auto itPolygon = closedPolygons.cbegin(); itPolygon != closedPolygons.cend(); ++itPolygon)
    {
        const auto& polygon = *itPolygon;

        std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(nullptr, nullptr));
        mapObject->_isArea = false;
        mapObject->_points31 = polygon;
        mapObject->_types.push_back(TagValue(QString::fromLatin1("natural"), QString::fromLatin1("coastline_line")));

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

        std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(nullptr, nullptr));
        mapObject->_points31 = polygon;
        if(clockwise)
        {
            mapObject->_types.push_back(TagValue(QString::fromLatin1("natural"), QString::fromLatin1("coastline")));
            fullWaterObjects++;
        }
        else
        {
            mapObject->_types.push_back(TagValue(QString::fromLatin1("natural"), QString::fromLatin1("land")));
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
        std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(nullptr, nullptr));
        mapObject->_points31.push_back(PointI(context._area31.left, context._area31.top));
        mapObject->_points31.push_back(PointI(context._area31.right, context._area31.top));
        mapObject->_points31.push_back(PointI(context._area31.right, context._area31.bottom));
        mapObject->_points31.push_back(PointI(context._area31.left, context._area31.bottom));
        mapObject->_points31.push_back(mapObject->_points31.first());

        mapObject->_types.push_back(TagValue(QString::fromLatin1("natural"), QString::fromLatin1("coastline")));
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

void OsmAnd::Rasterizer_P::obtainPrimitivesTexts(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    IQueryController* controller )
{
    collectPrimitivesTexts(env, context, context._polygons, Polygons, controller);
    collectPrimitivesTexts(env, context, context._lines, Lines, controller);
    collectPrimitivesTexts(env, context, context._points, Points, controller);

    qSort(context._texts.begin(), context._texts.end(), [](const TextPrimitive& l, const TextPrimitive& r) -> bool
    {
        return l.order < r.order;
    });
}

//void OsmAnd::Rasterizer_P::rasterizeText(
//    const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
//    bool fillBackground, SkCanvas& canvas, IQueryController* controller /*= nullptr*/ )
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

void OsmAnd::Rasterizer_P::collectPrimitivesTexts(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const QVector< Rasterizer_P::Primitive >& primitives, PrimitivesType type, IQueryController* controller )
{
    assert(type != PrimitivesType::ShadowOnlyLines);

    for(auto itPrimitive = primitives.cbegin(); itPrimitive != primitives.cend(); ++itPrimitive)
    {
        if(controller && controller->isAborted())
            return;

        const auto& primitive = *itPrimitive;

        // Skip primitives without names
        if(primitive.mapObject->names.isEmpty())
            continue;
        bool hasNonEmptyNames = false;
        for(auto itName = primitive.mapObject->names.cbegin(); itName != primitive.mapObject->names.cend(); ++itName)
        {
            const auto& name = itName.value();

            if(!name.isEmpty())
            {
                hasNonEmptyNames = true;
                break;
            }
        }
        if(!hasNonEmptyNames)
            continue;

        if(type == Polygons)
        {
            if (primitive.zOrder < context._polygonMinSizeToDisplay * env.owner->displayDensityFactor*env.owner->displayDensityFactor)
                return;

            collectPolygonText(env, context, primitive);
        }
        else if(type == Lines)
        {
            collectLineText(env, context, primitive);
        }
        else if(type == Points)
        {
            if(primitive.typeIndex != 0)
                continue;

            collectPointText(env, context, primitive);
        }
    }
}

void OsmAnd::Rasterizer_P::collectPolygonText(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const Primitive& primitive )
{
    if(primitive.mapObject->_points31.size() <=2)
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning,
            "Map object #%" PRIu64 " (%" PRIi64 ") is rasterized (text) as polygon, but has %d vertices",
            primitive.mapObject->id >> 1, static_cast<int64_t>(primitive.mapObject->id) / 2,
            primitive.mapObject->_points31.size());
        return;
    }
    if(!primitive.mapObject->isClosedFigure())
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning,
            "Map object #%" PRIu64 " (%" PRIi64 ") is rasterized (text) as polygon, but is not closed",
            primitive.mapObject->id >> 1, static_cast<int64_t>(primitive.mapObject->id) / 2);
        return;
    }
    if(!primitive.mapObject->isClosedFigure(true))
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning,
            "Map object #%" PRIu64 " (%" PRIi64 ") is rasterized (text) as polygon, but is not closed (inner)",
            primitive.mapObject->id >> 1, static_cast<int64_t>(primitive.mapObject->id) / 2);
        return;
    }

    {
        const auto& type = primitive.mapObject->_types[primitive.typeIndex];
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Polygon, primitive.mapObject);
        env.applyTo(evaluator);
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, type.tag);
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, type.value);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
        if(!evaluator.evaluate())
            return;
        if(!updatePaint(env, context, evaluator, Set_0, true))
            return;
    }

    PointF textPoint;

    bool containsAtLeastOnePoint = false;
    PointF vertex;
    int bounds = 0;
    QVector< PointF > outsideBounds;
    for(auto itPoint = primitive.mapObject->_points31.cbegin(); itPoint != primitive.mapObject->_points31.cend(); ++itPoint)
    {
        const auto& point = *itPoint;

        calculateVertex(env, context, point, vertex);

        textPoint.x += qMin( qMax(vertex.x, context._renderViewport.left), context._renderViewport.right);
        textPoint.y += qMin( qMax(vertex.y, context._renderViewport.bottom), context._renderViewport.top);
        
        if(!containsAtLeastOnePoint)
        {
            if(context._renderViewport.contains(vertex))
            {
                containsAtLeastOnePoint = true;
            }
            else
            {
                outsideBounds.push_back(vertex);
            }
            bounds |= (vertex.x < context._renderViewport.left ? 1 : 0);
            bounds |= (vertex.x > context._renderViewport.right ? 2 : 0);
            bounds |= (vertex.y < context._renderViewport.top ? 4 : 0);
            bounds |= (vertex.y > context._renderViewport.bottom ? 8 : 0);
        }
    }

    auto verticesCount = primitive.mapObject->_points31.size();
    textPoint.x /= verticesCount;
    textPoint.y /= verticesCount;

    if(!containsAtLeastOnePoint)
    {
        // fast check for polygons
        if((bounds & 3) != 3 || (bounds >> 2) != 3)
            return;

        bool ok = true;
        ok = ok || contains(outsideBounds, context._renderViewport.topLeft);
        ok = ok || contains(outsideBounds, context._renderViewport.bottomRight);
        ok = ok || contains(outsideBounds, PointF(0, context._renderViewport.bottom));
        ok = ok || contains(outsideBounds, PointF(context._renderViewport.right, 0));
        if(!ok)
            return;
        else
        {
            const auto& viewportCenter = context._renderViewport.center();
            if(contains(outsideBounds, viewportCenter))
                textPoint = viewportCenter;
        }
    }

    preparePrimitiveText(env, context, primitive, textPoint, nullptr);
}

void OsmAnd::Rasterizer_P::collectLineText(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const Primitive& primitive )
{
    if(primitive.mapObject->_points31.size() < 2 )
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning,
            "Map object #%" PRIu64 " (%" PRIi64 ") is rasterized (text) as line, but has %d vertices",
            primitive.mapObject->id >> 1, static_cast<int64_t>(primitive.mapObject->id) / 2,
            primitive.mapObject->_points31.size());
        return;
    }

    {
        const auto& type = primitive.mapObject->_types[primitive.typeIndex];
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Line, primitive.mapObject);
        env.applyTo(evaluator);
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, type.tag);
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, type.value);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_LAYER, primitive.mapObject->getSimpleLayerValue());
        if(!evaluator.evaluate())
            return;
        if(!updatePaint(env, context, evaluator, Set_0, false))
            return;
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

        calculateVertex(env, context, point, vertex);

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

        if (!intersect)
        {
            if(context._renderViewport.contains(vertex))
            {
                intersect = true;
            }
            else
            {
                int cross = 0;
                cross |= (vertex.x < context._renderViewport.left ? 1 : 0);
                cross |= (vertex.x > context._renderViewport.right ? 2 : 0);
                cross |= (vertex.y < context._renderViewport.top ? 4 : 0);
                cross |= (vertex.y > context._renderViewport.bottom ? 8 : 0);
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

    if (!intersect)
        return;

    if (pointIdx > 0)
    {
        preparePrimitiveText(env, context, primitive, middleVertex, &path);
    }
}

void OsmAnd::Rasterizer_P::collectPointText(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const Primitive& primitive )
{
    if(primitive.mapObject->_points31.size() < 1 )
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning,
            "Map object #%" PRIu64 " (%" PRIi64 ") is rasterized (text) as point, but has %d vertices",
            primitive.mapObject->id >> 1, static_cast<int64_t>(primitive.mapObject->id) / 2,
            primitive.mapObject->_points31.size());
        return;
    }

    PointF textPoint;
    if(primitive.mapObject->_points31.size() == 1)
    {
        const auto& p = primitive.mapObject->_points31.first();
        textPoint.x = p.x;
        textPoint.y = p.y;
    }
    else
    {
        auto verticesCount = primitive.mapObject->_points31.size();

        PointF vertex;
        for(auto itPoint = primitive.mapObject->_points31.cbegin(); itPoint != primitive.mapObject->_points31.cend(); ++itPoint)
        {
            const auto& point = *itPoint;

            calculateVertex(env, context, point, vertex);

            textPoint += vertex;
        }

        textPoint.x /= verticesCount;
        textPoint.y /= verticesCount;
    }
    
    preparePrimitiveText(env, context, primitive, textPoint, nullptr);
}

void OsmAnd::Rasterizer_P::preparePrimitiveText(
    const RasterizerEnvironment_P& env, RasterizerContext_P& context,
    const Primitive& primitive, const PointF& point, SkPath* path )
{
    const auto& type = primitive.mapObject->_types[primitive.typeIndex];

    for(auto itName = primitive.mapObject->names.cbegin(); itName != primitive.mapObject->names.cend(); ++itName)
    {
        const auto& name = itName.value();

        //TODO:name =rc->getTranslatedString(name);
        //TODO:name =rc->getReshapedString(name);

        const auto& type = primitive.mapObject->_types[primitive.typeIndex];
        MapStyleEvaluator evaluator(env.owner->style, env.owner->displayDensityFactor, MapStyleRulesetType::Text, primitive.mapObject);
        env.applyTo(evaluator);
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, type.tag);
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, type.value);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_TEXT_LENGTH, name.length());
        auto nameTag = itName.key();
        if(nameTag == "name")
            nameTag.clear();
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_NAME_TAG, nameTag);
        if(!evaluator.evaluate())
            continue;

        bool ok;
        int textSize;
        ok = evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_SIZE, textSize);
        if(!ok || textSize == 0)
            continue;

        TextPrimitive text;
        text.content = name;
        text.drawOnPath = false;
        if(path)
        {
            ok = evaluator.getBooleanValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_ON_PATH, text.drawOnPath);
            if(ok && text.drawOnPath)
                text.path.reset(new SkPath(*path));
        }
        text.center = point;
        text.vOffset = 0;
        evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_DY, text.vOffset);
        ok = evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_COLOR, text.color);
        if(!ok || !text.color)
            text.color = SK_ColorBLACK;
        ok = evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_SIZE, text.size);
        if(!ok)
            continue;
        text.shadowRadius = 0;
        evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_HALO_RADIUS, text.shadowRadius);
        text.wrapWidth = 0;
        evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_WRAP_WIDTH, text.wrapWidth);
        text.isBold = false;
        evaluator.getBooleanValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_BOLD, text.isBold);
        text.minDistance = 0;
        evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_MIN_DISTANCE, text.minDistance);
        evaluator.getStringValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_SHIELD, text.shieldResource);
        text.order = 100;
        evaluator.getIntegerValue(MapStyle::builtinValueDefinitions.OUTPUT_TEXT_ORDER, text.order);

        context._texts.push_back(text);
    }
}
