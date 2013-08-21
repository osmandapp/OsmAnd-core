#include "Rasterizer.h"

#include <assert.h>
#include <set>

#include <QtGlobal>

#include "ObfMapSectionInfo.h"
#include "MapStyleEvaluator.h"
#include "RasterizerContext.h"
#include "Logging.h"
#include "Utilities.h"

#include <SkBlurDrawLooper.h>
#include <SkColorFilter.h>
#include <SkDashPathEffect.h>

OsmAnd::Rasterizer::Rasterizer()
{
}

OsmAnd::Rasterizer::~Rasterizer()
{
}

void OsmAnd::Rasterizer::update(
    RasterizerContext& context,
    const AreaI& area31,
    uint32_t zoom,
    uint32_t tileSidePixelLength,
    float densityFactor,
    const QList< std::shared_ptr<const OsmAnd::Model::MapObject> >* objects /*= nullptr*/,
    const PointF& tlOriginOffset /*= PointF() */,
    bool* noDataAvailable /*= nullptr*/,
    IQueryController* controller /*= nullptr*/)
{
    bool updateObjectsCollections = context._wasAborted;
    bool repolygonizeCoastlines = context._wasAborted;
    bool reobtainPrimitives = context._wasAborted;
    updateObjectsCollections = updateObjectsCollections || (objects != nullptr);
    updateObjectsCollections = updateObjectsCollections || (context._zoom != zoom);
    repolygonizeCoastlines = repolygonizeCoastlines || updateObjectsCollections;
    repolygonizeCoastlines = repolygonizeCoastlines || (context._area31 != area31);
    reobtainPrimitives = reobtainPrimitives || updateObjectsCollections;
    reobtainPrimitives = reobtainPrimitives || repolygonizeCoastlines;
    reobtainPrimitives = reobtainPrimitives || (context._densityFactor != densityFactor);

    context.update(area31, zoom, tlOriginOffset, tileSidePixelLength, densityFactor);

    context._wasAborted = false;

    // Split input map objects to object, coastline, basemapObjects and basemapCoastline
    if(updateObjectsCollections)
    {
        context._hasBasemap = false;
        context._hasLand = false;
        context._hasWater = false;
        context._mapObjects.clear();
        context._coastlineObjects.clear();
        context._basemapMapObjects.clear();
        context._basemapCoastlineObjects.clear();
        for(auto itMapObject = objects->begin(); itMapObject != objects->end(); ++itMapObject)
        {
            if(controller && controller->isAborted())
                break;

            const auto& mapObject = *itMapObject;

            context._hasLand = context._hasLand || mapObject->foundation == MapFoundationType::FullLand;
            context._hasWater = context._hasWater || mapObject->foundation == MapFoundationType::FullWater;
            context._hasBasemap = context._hasBasemap || mapObject->section->isBasemap;
            if(zoom < ZoomOnlyForBasemaps && !mapObject->section->isBasemap)
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
    }
    if(controller && controller->isAborted())
    {
        context._wasAborted = true;
        goto abort_cleanup;
    }

    if(repolygonizeCoastlines)
    {
        context._triangulatedCoastlineObjects.clear();

        bool addBasemapCoastlines = true;
        
        // determine if there are enough objects like land/lake..
        const bool detailedLandData = zoom >= DetailedLandDataZoom && !context._mapObjects.isEmpty();
        if(!context._coastlineObjects.empty())
        {
            const bool coastlinesWereAdded = polygonizeCoastlines(context,
                context._coastlineObjects,
                context._triangulatedCoastlineObjects,
                !context._basemapCoastlineObjects.isEmpty(),
                true);
            addBasemapCoastlines = (!coastlinesWereAdded && !detailedLandData) || zoom <= BasemapZoom;
        }
        else
        {
            addBasemapCoastlines = !detailedLandData;
        }
        if (addBasemapCoastlines)
        {
            const bool coastlinesWereAdded = polygonizeCoastlines(context,
                context._basemapCoastlineObjects,
                context._triangulatedCoastlineObjects,
                false,
                true);
            addBasemapCoastlines = !coastlinesWereAdded;
        }

        if (addBasemapCoastlines)
        {
            std::shared_ptr<Model::MapObject> bgMapObject(new Model::MapObject(nullptr));
            bgMapObject->_points31.push_back(PointI(area31.left, area31.top));
            bgMapObject->_points31.push_back(PointI(area31.right, area31.top));
            bgMapObject->_points31.push_back(PointI(area31.right, area31.bottom));
            bgMapObject->_points31.push_back(PointI(area31.left, area31.bottom));
            bgMapObject->_points31.push_back(bgMapObject->_points31.first());
            if (context._hasWater && !context._hasLand)
                bgMapObject->_types.push_back(TagValue(QString::fromLatin1("natural"), QString::fromLatin1("coastline")));
            else
                bgMapObject->_types.push_back(TagValue(QString::fromLatin1("natural"), QString::fromLatin1("land")));

            context._triangulatedCoastlineObjects.push_back(bgMapObject);
        }
    }

    if(reobtainPrimitives)
    {
        const bool emptyData = zoom > BasemapZoom && context._mapObjects.isEmpty() && context._coastlineObjects.isEmpty();
        const bool basemapMissing = zoom <= BasemapZoom && context._basemapCoastlineObjects.isEmpty() && !context._hasBasemap;
        if (emptyData || basemapMissing)
        {
            if(noDataAvailable)
                *noDataAvailable = true;

            // We simply have no data to render. Report and exit
            return;
        }

        if(noDataAvailable)
            *noDataAvailable = false;

        context._combinedMapObjects.clear();
        context._polygons.clear();
        context._lines.clear();
        context._points.clear();

        context._combinedMapObjects << context._mapObjects;
        if(zoom <= BasemapZoom || emptyData)
            context._combinedMapObjects << context._basemapMapObjects;
        context._combinedMapObjects << context._triangulatedCoastlineObjects;

        obtainPrimitives(context, controller);

        context._texts.clear();
        obtainPrimitivesTexts(context, controller);
    }

    if(controller && controller->isAborted())
    {
        context._wasAborted = true;
        goto abort_cleanup;
    }

    return;

abort_cleanup:
    context._combinedMapObjects.clear();
    context._polygons.clear();
    context._lines.clear();
    context._points.clear();
    context._triangulatedCoastlineObjects.clear();
    context._mapObjects.clear();
    context._coastlineObjects.clear();
    context._basemapMapObjects.clear();
    context._basemapCoastlineObjects.clear();
    context._texts.clear();
}

bool OsmAnd::Rasterizer::rasterizeMap(
    RasterizerContext& context,
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

    rasterizeMapPrimitives(context, canvas, context._polygons, Polygons, controller);

    if (context._shadowRenderingMode > 1)
        rasterizeMapPrimitives(context, canvas, context._lines, ShadowOnlyLines, controller);

    rasterizeMapPrimitives(context, canvas, context._lines, Lines, controller);

    return true;
}

void OsmAnd::Rasterizer::obtainPrimitives(RasterizerContext& context, IQueryController* controller)
{
    auto area31toPixelDivisor = context._precomputed31toPixelDivisor * context._precomputed31toPixelDivisor;
    
    QVector< Primitive > unfilteredLines;
    for(auto itMapObject = context._combinedMapObjects.begin(); itMapObject != context._combinedMapObjects.end(); ++itMapObject)
    {
        if(controller && controller->isAborted())
            return;

        auto mapObject = *itMapObject;
        
        uint32_t typeIdx = 0;
        for(auto itType = mapObject->_types.begin(); itType != mapObject->_types.end(); ++itType, typeIdx++)
        {
            const auto& type = *itType;
            auto layer = mapObject->getSimpleLayerValue();

            MapStyleEvaluator evaluator(context.style, MapStyle::RulesetType::Order, mapObject);
            context.applyTo(evaluator);
            evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, type.tag);
            evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, type.value);
            evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
            evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
            evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_LAYER, layer);
            evaluator.setBooleanValue(MapStyle::builtinValueDefinitions.INPUT_AREA, mapObject->_isArea);
            evaluator.setBooleanValue(MapStyle::builtinValueDefinitions.INPUT_POINT, mapObject->_points31.size() == 1);
            evaluator.setBooleanValue(MapStyle::builtinValueDefinitions.INPUT_CYCLE, mapObject->isClosedFigure());
            if(evaluator.evaluate())
            {
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
                    if(mapObject->_points31.size() <=2)
                    {
                        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu primitives are processed as polygon, but only %d vertices present", mapObject->id >> 1, mapObject->_points31.size());
                        continue;
                    }
                    if(!mapObject->isClosedFigure())
                    {
                        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu primitives are processed as polygon, but are not closed", mapObject->id >> 1);
                        continue;
                    }
                    if(!mapObject->isClosedFigure(true))
                    {
                        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu primitives are processed as polygon, but are not closed (inner)", mapObject->id >> 1);
                        continue;
                    }

                    Primitive pointPrimitive = primitive;
                    pointPrimitive.objectType = PrimitiveType::Point;
                    auto polygonArea31 = Utilities::polygonArea(mapObject->_points31);
                    primitive.zOrder = polygonArea31 / area31toPixelDivisor;
                    if(primitive.zOrder > PolygonAreaCutoffLowerThreshold * context._densityFactor)
                    {
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
    }

    qSort(context._polygons.begin(), context._polygons.end(), [](const Primitive& l, const Primitive& r) -> bool
    {
        if( qFuzzyCompare(l.zOrder, r.zOrder) )
            return l.typeIndex < r.typeIndex;
        return l.zOrder > r.zOrder;
    });
    qSort(unfilteredLines.begin(), unfilteredLines.end(), [](const Primitive& l, const Primitive& r) -> bool
    {
        if(qFuzzyCompare(l.zOrder, r.zOrder))
        {
            if(l.typeIndex == r.typeIndex)
                return l.mapObject->_points31.size() < r.mapObject->_points31.size();
            return l.typeIndex < r.typeIndex;
        }
        return l.zOrder < r.zOrder;
    });
    filterOutLinesByDensity(context, unfilteredLines, context._lines, controller);
    qSort(context._points.begin(), context._points.end(), [](const Primitive& l, const Primitive& r) -> bool
    {
        if(qFuzzyCompare(l.zOrder, r.zOrder))
        {
            if(l.typeIndex == r.typeIndex)
                return l.mapObject->_points31.size() < r.mapObject->_points31.size();
            return l.typeIndex < r.typeIndex;
        }
        return l.zOrder < r.zOrder;
    });
}

void OsmAnd::Rasterizer::filterOutLinesByDensity( RasterizerContext& context, const QVector< Primitive >& in, QVector< Primitive >& out, IQueryController* controller )
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
        if(type.tag == QString::fromLatin1("highway"))
        {
            accept = false;

            uint64_t prevId = 0;
            for(auto itPoint = primitive.mapObject->_points31.begin(); itPoint != primitive.mapObject->_points31.end(); ++itPoint)
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

void OsmAnd::Rasterizer::rasterizeMapPrimitives( RasterizerContext& context, SkCanvas& canvas, const QVector< Primitive >& primitives, PrimitivesType type, IQueryController* controller )
{
    assert(type != PrimitivesType::Points);

    for(auto itPrimitive = primitives.begin(); itPrimitive != primitives.end(); ++itPrimitive)
    {
        if(controller && controller->isAborted())
            return;

        const auto& primitive = *itPrimitive;

        if(type == Polygons)
        {
            if (primitive.zOrder < context._polygonMinSizeToDisplay * context._densityFactor)
                return;

            rasterizePolygon(context, canvas, primitive);
        }
        else if(type == Lines || type == ShadowOnlyLines)
        {
            rasterizeLine(context, canvas, primitive, type == ShadowOnlyLines);
        }
    }
}

bool OsmAnd::Rasterizer::updatePaint( RasterizerContext& context, const MapStyleEvaluator& evaluator, PaintValuesSet valueSetSelector, bool isArea )
{
    bool ok = true;
    struct ValueSet
    {
        const std::shared_ptr<MapStyle::ValueDefinition>& color;
        const std::shared_ptr<MapStyle::ValueDefinition>& strokeWidth;
        const std::shared_ptr<MapStyle::ValueDefinition>& cap;
        const std::shared_ptr<MapStyle::ValueDefinition>& pathEffect;
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
        context._mapPaint.setStrokeWidth(stroke * context._densityFactor);

        QString cap;
        ok = evaluator.getStringValue(valueSet.cap, cap);
        if (!ok || cap.isEmpty() || cap == "BUTT")
            context._mapPaint.setStrokeCap(SkPaint::kButt_Cap);
        else if (cap == "ROUND")
            context._mapPaint.setStrokeCap(SkPaint::kRound_Cap);
        else if (cap == "SQUARE")
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
            auto effect = context.obtainPathEffect(pathEff);
            context._mapPaint.setPathEffect(effect);
        }
    }

    int color;
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
            OsmAnd::LogPrintf(LogSeverityLevel::Warning, "RASTERIZER NEEDS '%s' FILL TEXTURE!!!!!", qPrintable(shader));
            //assert(false);
            //int i = 5;
            /*TODO:SkBitmap* bmp = getCachedBitmap(rc, shader);
            if (bmp != NULL)
            paint->setShader(new SkBitmapProcShader(*bmp, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode))->unref();*/
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
            context._mapPaint.setLooper(new SkBlurDrawLooper(shadowRadius * context._densityFactor, 0, 0, shadowColor))->unref();
    }

    return true;
}

void OsmAnd::Rasterizer::rasterizePolygon( RasterizerContext& context, SkCanvas& canvas, const Primitive& primitive )
{
    if(primitive.mapObject->_points31.size() <=2)
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rasterized as polygon, but has %d vertices", primitive.mapObject->id >> 1, primitive.mapObject->_points31.size());
        return;
    }
    if(!primitive.mapObject->isClosedFigure())
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rasterized as polygon, but is not closed", primitive.mapObject->id >> 1);
        return;
    }
    if(!primitive.mapObject->isClosedFigure(true))
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rasterized as polygon, but is not closed (inner)", primitive.mapObject->id >> 1);
        return;
    }

    const auto& type = primitive.mapObject->_types[primitive.typeIndex];

    MapStyleEvaluator evaluator(context.style, MapStyle::RulesetType::Polygon, primitive.mapObject);
    context.applyTo(evaluator);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, type.tag);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, type.value);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
    if(!evaluator.evaluate())
        return;
    if(!updatePaint(context, evaluator, Set_0, true))
        return;

    SkPath path;
    bool containsPoint = false;
    int pointIdx = 0;
    PointF vertex;
    int bounds = 0;
    QVector< PointF > outsideBounds;
    for(auto itPoint = primitive.mapObject->_points31.begin(); itPoint != primitive.mapObject->_points31.end(); ++itPoint, pointIdx++)
    {
        const auto& point = *itPoint;

        calculateVertex(context, point, vertex);

        if(pointIdx == 0)
        {
            path.moveTo(vertex.x, vertex.y);
        }
        else
        {
            path.lineTo(vertex.x, vertex.y);
        }

        if (!containsPoint)
        {
            if(context._renderViewport.contains(vertex))
            {
                containsPoint = true;
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

    if(!containsPoint)
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

    if(!primitive.mapObject->_innerPolygonsPoints31.isEmpty())
    {
        path.setFillType(SkPath::kEvenOdd_FillType);
        for(auto itPolygon = primitive.mapObject->_innerPolygonsPoints31.begin(); itPolygon != primitive.mapObject->_innerPolygonsPoints31.end(); ++itPolygon)
        {
            const auto& polygon = *itPolygon;

            pointIdx = 0;
            for(auto itVertex = polygon.begin(); itVertex != polygon.end(); ++itVertex, pointIdx++)
            {
                const auto& point = *itVertex;
                calculateVertex(context, point, vertex);

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
    if(updatePaint(context, evaluator, Set_1, false))
        canvas.drawPath(path, context._mapPaint);
}

void OsmAnd::Rasterizer::rasterizeLine( RasterizerContext& context, SkCanvas& canvas, const Primitive& primitive, bool drawOnlyShadow )
{
    if(primitive.mapObject->_points31.size() < 2 )
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rasterized as line, but has %d vertices", primitive.mapObject->id >> 1, primitive.mapObject->_points31.size());
        return;
    }

    bool ok;
    const auto& type = primitive.mapObject->_types[primitive.typeIndex];

    MapStyleEvaluator evaluator(context.style, MapStyle::RulesetType::Line, primitive.mapObject);
    context.applyTo(evaluator);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, type.tag);
    evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, type.value);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
    evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_LAYER, primitive.mapObject->getSimpleLayerValue());
    if(!evaluator.evaluate())
        return;
    if(!updatePaint(context, evaluator, Set_0, false))
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
    if (context._zoom >= 16 && type.tag == "highway")
    {
        if (primitive.mapObject->containsType("oneway", "yes", true))
            oneway = 1;
        else if (primitive.mapObject->containsType("oneway", "-1", true))
            oneway = -1;
    }

    SkPath path;
    int pointIdx = 0;
    const auto middleIdx = primitive.mapObject->_points31.size() >> 1;
    bool intersect = false;
    int prevCross = 0;
    PointF vertex, middleVertex;
    for(auto itPoint = primitive.mapObject->_points31.begin(); itPoint != primitive.mapObject->_points31.end(); ++itPoint, pointIdx++)
    {
        const auto& point = *itPoint;

        calculateVertex(context, point, vertex);

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
            rasterizeLineShadow(context, canvas, path, shadowColor, shadowRadius);
        }
        else
        {
            if (updatePaint(context, evaluator, Set_minus2, false))
            {
                canvas.drawPath(path, context._mapPaint);
            }
            if (updatePaint(context, evaluator, Set_minus1, false))
            {
                canvas.drawPath(path, context._mapPaint);
            }
            if (updatePaint(context, evaluator, Set_0, false))
            {
                canvas.drawPath(path, context._mapPaint);
            }
            canvas.drawPath(path, context._mapPaint);
            if (updatePaint(context, evaluator, Set_1, false))
            {
                canvas.drawPath(path, context._mapPaint);
            }
            if (updatePaint(context, evaluator, Set_3, false))
            {
                canvas.drawPath(path, context._mapPaint);
            }
            if (oneway && !drawOnlyShadow)
            {
                rasterizeLine_OneWay(context, canvas, path, oneway);
            }
        }
    }
}

void OsmAnd::Rasterizer::rasterizeLineShadow( RasterizerContext& context, SkCanvas& canvas, const SkPath& path, uint32_t shadowColor, int shadowRadius )
{
    // blurred shadows
    if (context._shadowRenderingMode == 2 && shadowRadius > 0)
    {
        // simply draw shadow? difference from option 3 ?
        // paint->setColor(0xffffffff);
        context._mapPaint.setLooper(new SkBlurDrawLooper(shadowRadius * context._densityFactor, 0, 0, shadowColor))->unref();
        canvas.drawPath(path, context._mapPaint);
    }

    // option shadow = 3 with solid border
    if (context._shadowRenderingMode == 3 && shadowRadius > 0)
    {
        context._mapPaint.setLooper(nullptr);
        context._mapPaint.setStrokeWidth((context._mapPaint.getStrokeWidth() + shadowRadius * 2) * context._densityFactor);
        //		paint->setColor(0xffbababa);
        context._mapPaint.setColorFilter(SkColorFilter::CreateModeFilter(shadowColor, SkXfermode::kSrcIn_Mode))->unref();
        //		paint->setColor(shadowColor);
        canvas.drawPath(path, context._mapPaint);
    }
}

void OsmAnd::Rasterizer::rasterizeLine_OneWay( RasterizerContext& context, SkCanvas& canvas, const SkPath& path, int oneway )
{
    if (oneway > 0)
    {
        for(auto itPaint = context._oneWayPaints.begin(); itPaint != context._oneWayPaints.end(); ++itPaint)
        {
            canvas.drawPath(path, *itPaint);
        }
    }
    else
    {
        for(auto itPaint = context._reverseOneWayPaints.begin(); itPaint != context._reverseOneWayPaints.end(); ++itPaint)
        {
            canvas.drawPath(path, *itPaint);
        }
    }
}

void OsmAnd::Rasterizer::calculateVertex( RasterizerContext& context, const PointI& point31, PointF& vertex )
{
    vertex.x = static_cast<float>(point31.x - context._area31.left) / context._precomputed31toPixelDivisor + context._renderViewport.left;
    vertex.y = static_cast<float>(point31.y - context._area31.top) / context._precomputed31toPixelDivisor + context._renderViewport.top;
}

bool OsmAnd::Rasterizer::contains( const QVector< PointF >& vertices, const PointF& other )
{
    uint32_t intersections = 0;

    auto itPrevVertex = vertices.begin();
    auto itVertex = itPrevVertex + 1;
    for(; itVertex != vertices.end(); itPrevVertex = itVertex, ++itVertex)
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

bool OsmAnd::Rasterizer::polygonizeCoastlines(
    RasterizerContext& context,
    const QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& coastlines,
    QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& outVectorized,
    bool abortIfBrokenCoastlinesExist,
    bool includeBrokenCoastlines )
{
    QList< QVector< PointI > > closedPolygons;
    QList< QVector< PointI > > brokenPolygons; // Broken == not closed in this case

    uint64_t osmId = 0;
    QVector< PointI > linePoints31;
    for(auto itCoastline = coastlines.begin(); itCoastline != coastlines.end(); ++itCoastline)
    {
        const auto& coastline = *itCoastline;

        if(coastline->_points31.size() < 2)
        {
            OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is polygonized as coastline, but has %d vertices", coastline->id >> 1, coastline->_points31.size());
            continue;
        }

        osmId = coastline->id >> 1;
        linePoints31.clear();
        auto itPoint = coastline->_points31.begin();
        auto pp = *itPoint;
        auto cp = pp;
        auto prevInside = context._area31.contains(cp);
        if(prevInside)
            linePoints31.push_back(cp);
        for(++itPoint; itPoint != coastline->_points31.end(); ++itPoint)
        {
            cp = *itPoint;

            const auto inside = context._area31.contains(cp);
            const auto lineEnded = buildCoastlinePolygonSegment(context, inside, cp, prevInside, pp, linePoints31);
            if (lineEnded)
            {
                appendCoastlinePolygons(closedPolygons, brokenPolygons, linePoints31);

                // Create new line if it goes outside
                linePoints31.clear();
            }

            pp = cp;
            prevInside = inside;
        }

        appendCoastlinePolygons(closedPolygons, brokenPolygons, linePoints31);
    }

    if (closedPolygons.isEmpty() && brokenPolygons.isEmpty())
        return false;

    if (!brokenPolygons.isEmpty())
        mergeBrokenPolygons(context, brokenPolygons, closedPolygons, osmId);

    if(!brokenPolygons.isEmpty())
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Broken polygons found during polygonization of coastlines in area [%d, %d, %d, %d] @ zoom %d",
            context._area31.top,
            context._area31.left,
            context._area31.bottom,
            context._area31.right,
            context._zoom);
    }

    if (includeBrokenCoastlines)
    {
        for(auto itPolygon = brokenPolygons.begin(); itPolygon != brokenPolygons.end(); ++itPolygon)
        {
            const auto& polygon = *itPolygon;

            std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(nullptr));
            mapObject->_points31 = polygon;
            mapObject->_types.push_back(TagValue("natural", "coastline_broken"));

            outVectorized.push_back(mapObject);
        }

        for(auto itPolygon = closedPolygons.begin(); itPolygon != closedPolygons.end(); ++itPolygon)
        {
            const auto& polygon = *itPolygon;

            std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(nullptr));
            mapObject->_points31 = polygon;
            mapObject->_types.push_back(TagValue("natural", "coastline_line"));

            outVectorized.push_back(mapObject);
        }

    }

    if (abortIfBrokenCoastlinesExist && !brokenPolygons.isEmpty())
        return false;

    bool clockwiseFound = false;
    for(auto itPolygon = closedPolygons.begin(); itPolygon != closedPolygons.end(); ++itPolygon)
    {
        const auto& polygon = *itPolygon;

        bool clockwise = isClockwiseCoastlinePolygon(polygon);
        clockwiseFound = clockwiseFound || clockwise;

        std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(nullptr));
        mapObject->_points31 = polygon;
        mapObject->_types.push_back(TagValue("natural", clockwise ? "coastline" : "land"));
        mapObject->_id = osmId;
        mapObject->_isArea = true;

        outVectorized.push_back(mapObject);
    }

    if (!clockwiseFound && brokenPolygons.isEmpty())
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Isolated islands found during polygonization of coastlines in area [%d, %d, %d, %d] @ zoom %d",
            context._area31.top,
            context._area31.left,
            context._area31.bottom,
            context._area31.right,
            context._zoom);

        // add complete water tile
        std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(nullptr));
        mapObject->_points31.push_back(PointI(context._area31.left, context._area31.top));
        mapObject->_points31.push_back(PointI(context._area31.right, context._area31.top));
        mapObject->_points31.push_back(PointI(context._area31.right, context._area31.bottom));
        mapObject->_points31.push_back(PointI(context._area31.left, context._area31.bottom));
        mapObject->_points31.push_back(PointI(context._area31.left, context._area31.top));

        mapObject->_types.push_back(TagValue("natural", "coastline"));
        mapObject->_id = osmId;
        mapObject->_isArea = true;

        outVectorized.push_back(mapObject);
    }

    return true;
}

bool OsmAnd::Rasterizer::buildCoastlinePolygonSegment(
    RasterizerContext& context,
    bool currentInside,
    const PointI& currentPoint31,
    bool prevInside,
    const PointI& previousPoint31,
    QVector< PointI >& segmentPoints )
{
    bool lineEnded = false;

    auto point = currentPoint31;
    if (prevInside)
    {
        if (!currentInside)
        {
            bool hasIntersection = calculateIntersection(currentPoint31, previousPoint31, context._area31, point);
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
        bool hasIntersection = calculateIntersection(currentPoint31, previousPoint31, context._area31, point);
        if (currentInside)
        {
            assert(hasIntersection);
            segmentPoints.push_back(point);
            segmentPoints.push_back(currentPoint31);
        }
        else if (hasIntersection)
        {
            segmentPoints.push_back(point);
            calculateIntersection(currentPoint31, point, context._area31, point);
            segmentPoints.push_back(point);
            lineEnded = true;
        }
    }

    return lineEnded;
}

// Calculates intersection between line and bbox in clockwise manner.
bool OsmAnd::Rasterizer::calculateIntersection( const PointI& p1, const PointI& p0, const AreaI& bbox, PointI& pX )
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

void OsmAnd::Rasterizer::appendCoastlinePolygons( QList< QVector< PointI > >& closedPolygons, QList< QVector< PointI > >& brokenPolygons, QVector< PointI >& polyline )
{
    if(polyline.isEmpty())
        return;

    if(polyline.first() == polyline.last())
    {
        closedPolygons.push_back(polyline);
        return;
    }
    
    bool add = true;

    for(auto itPolygon = brokenPolygons.begin(); itPolygon != brokenPolygons.end();)
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
            itPolygon = brokenPolygons.erase(itPolygon);
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
        brokenPolygons.push_back(polyline);
    }
}

void OsmAnd::Rasterizer::mergeBrokenPolygons( RasterizerContext& context, QList< QVector< PointI > >& brokenPolygons_, QList< QVector< PointI > >& closedPolygons, uint64_t osmId )
{
    std::set< QList< QVector< PointI > >::iterator > nonvisistedPolygons;
    QList< QVector< PointI > > brokenPolygons(brokenPolygons_);
    brokenPolygons_.clear();
    int idx;

    idx = 0;
    for(auto itPolygon = brokenPolygons.begin(); itPolygon != brokenPolygons.end(); ++itPolygon, idx++)
    {
        const auto& polygon = *itPolygon;
        assert(!polygon.isEmpty());

        const auto& bp = polygon.first();
        const auto& ep = polygon.last();

        const bool st = bp.y == context._area31.top || bp.x == context._area31.right || bp.y == context._area31.bottom || bp.x == context._area31.left;
        const bool end = ep.y == context._area31.top || ep.x == context._area31.right || ep.y == context._area31.bottom || ep.x == context._area31.left;

        // something goes wrong
        // These exceptions are used to check logic about processing multipolygons
        // However this situation could happen because of broken multipolygons (so it should data causes app error)
        // that's why these exceptions could be replaced with return; statement.
        if (!end || !st)
        {
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Error processing multipolygon");
            brokenPolygons_.push_back(polygon);
        }
        else
        {
            nonvisistedPolygons.insert(itPolygon);
        }
    }
    
    idx = 0;
    for(auto itPolygon0 = brokenPolygons.begin(); itPolygon0 != brokenPolygons.end(); ++itPolygon0, idx++)
    {
        if (nonvisistedPolygons.find(itPolygon0) == nonvisistedPolygons.end())
            continue;

        auto& polygon0 = *itPolygon0;
        assert(!polygon0.isEmpty());

        auto ep = polygon0.last();
        
        // 31 - (zoom + 8)
        const int EVAL_DELTA = 6 << (23 - context._zoom);
        const int UNDEFINED_MIN_DIFF = -1 - EVAL_DELTA;
        while (true) {
            int st = 0; // st already checked to be one of the four
            if (ep.y == context._area31.top) {
                st = 0;
            } else if (ep.x == context._area31.right) {
                st = 1;
            } else if (ep.y == context._area31.bottom) {
                st = 2;
            } else if (ep.x == context._area31.left) {
                st = 3;
            }
            auto itNextPolygon = brokenPolygons.end();
            // BEGIN go clockwise around rectangle
            for (int h = st; h < st + 4; h++) {

                // BEGIN find closest nonvisited start (including current)
                int mindiff = UNDEFINED_MIN_DIFF;
                for(auto itPolygon1 = brokenPolygons.begin(); itPolygon1 != brokenPolygons.end(); ++itPolygon1)
                {
                    if(nonvisistedPolygons.find(itPolygon1) == nonvisistedPolygons.end())
                        continue;

                    const auto& polygon1 = *itPolygon1;
                    assert(!polygon1.isEmpty());
                    const auto& bp = polygon1.first();
                    if (h % 4 == 0) {
                        // top
                        if (bp.y == context._area31.top && bp.x >= Utilities::sumWithSaturation(ep.x, -EVAL_DELTA)) {
                            if (mindiff == UNDEFINED_MIN_DIFF || (bp.x - ep.x) <= mindiff) {
                                mindiff = (bp.x - ep.x);
                                itNextPolygon = itPolygon1;
                            }
                        }
                    } else if (h % 4 == 1) {
                        // right
                        if (bp.x == context._area31.right && bp.y >= Utilities::sumWithSaturation(ep.y, -EVAL_DELTA)) {
                            if (mindiff == UNDEFINED_MIN_DIFF || (bp.y - ep.y) <= mindiff) {
                                mindiff = (bp.y - ep.y);
                                itNextPolygon = itPolygon1;
                            }
                        }
                    } else if (h % 4 == 2) {
                        // bottom
                        if (bp.y == context._area31.bottom && bp.x <= Utilities::sumWithSaturation(ep.x, EVAL_DELTA)) {
                            if (mindiff == UNDEFINED_MIN_DIFF || (ep.x - bp.x) <= mindiff) {
                                mindiff = (ep.x - bp.x);
                                itNextPolygon = itPolygon1;
                            }
                        }
                    } else if (h % 4 == 3) {
                        // left
                        if (bp.x == context._area31.left && bp.y <= Utilities::sumWithSaturation(ep.y, EVAL_DELTA)) {
                            if (mindiff == UNDEFINED_MIN_DIFF || (ep.y - bp.y) <= mindiff) {
                                mindiff = (ep.y - bp.y);
                                itNextPolygon = itPolygon1;
                            }
                        }
                    }
                } // END find closest start (including current)

                // we found start point
                if (mindiff != UNDEFINED_MIN_DIFF) {
                    break;
                } else {
                    if (h % 4 == 0) {
                        // top
                        ep.y = context._area31.top;
                        ep.x = context._area31.right;
                    } else if (h % 4 == 1) {
                        // right
                        ep.y = context._area31.bottom;
                        ep.x = context._area31.right;
                    } else if (h % 4 == 2) {
                        // bottom
                        ep.y = context._area31.bottom;
                        ep.x = context._area31.left;
                    } else if (h % 4 == 3) {
                        ep.y = context._area31.top;
                        ep.x = context._area31.left;
                    }

                    polygon0.push_back(ep);
                }

            } // END go clockwise around rectangle

            assert(itNextPolygon != brokenPolygons.end());
            if(itNextPolygon == itPolygon0)
            {
                polygon0.push_back(polygon0.first());
                nonvisistedPolygons.erase(itPolygon0);
                break;
            } else {
                const auto& otherPolygon = *itNextPolygon;
                polygon0 << otherPolygon;
                nonvisistedPolygons.erase(itNextPolygon);

                // get last point and start again going clockwise
                ep = polygon0.last();
            }
        }

        closedPolygons.push_back(polygon0);
    }
}

bool OsmAnd::Rasterizer::isClockwiseCoastlinePolygon( const QVector< PointI > & polygon )
{
    if(polygon.isEmpty())
        return true;

    // calculate middle Y
    int64_t middleY = 0;
    for(auto itVertex = polygon.begin(); itVertex != polygon.end(); ++itVertex)
        middleY += itVertex->y;
    middleY /= polygon.size();

    double clockwiseSum = 0;

    bool firstDirectionUp = false;
    int previousX = INT_MIN;
    int firstX = INT_MIN;

    auto itPrevVertex = polygon.begin();
    auto itVertex = itPrevVertex + 1;
    for(; itVertex != polygon.end(); itPrevVertex = itVertex, ++itVertex)
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

void OsmAnd::Rasterizer::obtainPrimitivesTexts( RasterizerContext& context, IQueryController* controller )
{
    collectPrimitivesTexts(context, context._polygons, Polygons, controller);
    collectPrimitivesTexts(context, context._lines, Lines, controller);
    collectPrimitivesTexts(context, context._points, Points, controller);

    qSort(context._texts.begin(), context._texts.end(), [](const TextPrimitive& l, const TextPrimitive& r) -> bool
    {
        return l.order < r.order;
    });
}

void OsmAnd::Rasterizer::rasterizeText( RasterizerContext& context, bool fillBackground, SkCanvas& canvas, IQueryController* controller /*= nullptr*/ )
{
    if(fillBackground)
    {
        SkPaint bgPaint;
        bgPaint.setColor(SK_ColorTRANSPARENT);
        bgPaint.setStyle(SkPaint::kFill_Style);
        canvas.drawRectCoords(context._renderViewport.top, context._renderViewport.left, context._renderViewport.right, context._renderViewport.bottom, bgPaint);
    }
    /*
    SkRect r = SkRect::MakeLTRB(0, 0, rc->getWidth(), rc->getHeight());
    r.inset(-100, -100);
    quad_tree<TextDrawInfo*> boundsIntersect(r, 4, 0.6);
    */
    /*
#if defined(ANDROID)
    //TODO: This is never released because of always +1 of reference counter
    if(!sDefaultTypeface)
        sDefaultTypeface = SkTypeface::CreateFromName("Droid Serif", SkTypeface::kNormal);
#endif
        */
    SkPaint::FontMetrics fontMetrics;
    for(auto itText = context._texts.begin(); itText != context._texts.end(); ++itText)
    {
        if(controller && controller->isAborted())
            break;

        const auto& text = *itText;

        context._textPaint.setTextSize(text.size * context._densityFactor);
        context._textPaint.setFakeBoldText(text.isBold);//TODO: use special typeface!
        context._textPaint.setColor(text.color);
        context._textPaint.getFontMetrics(&fontMetrics);
        /*textDrawInfo->centerY += (-fm.fAscent);

        // calculate if there is intersection
        bool intersects = findTextIntersection(cv, rc, boundsIntersect, textDrawInfo, &paintText, &paintIcon);
        if(intersects)
            continue;

        if (textDrawInfo->drawOnPath && textDrawInfo->path != NULL) {
            if (textDrawInfo->textShadow > 0) {
                paintText.setColor(0xFFFFFFFF);
                paintText.setStyle(SkPaint::kStroke_Style);
                paintText.setStrokeWidth(2 + textDrawInfo->textShadow);
                rc->nativeOperations.Pause();
                cv->drawTextOnPathHV(textDrawInfo->text.c_str(), textDrawInfo->text.length(), *textDrawInfo->path, textDrawInfo->hOffset,
                    textDrawInfo->vOffset, paintText);
                rc->nativeOperations.Start();
                // reset
                paintText.setStyle(SkPaint::kFill_Style);
                paintText.setStrokeWidth(2);
                paintText.setColor(textDrawInfo->textColor);
            }
            rc->nativeOperations.Pause();
            cv->drawTextOnPathHV(textDrawInfo->text.c_str(), textDrawInfo->text.length(), *textDrawInfo->path, textDrawInfo->hOffset,
                textDrawInfo->vOffset, paintText);
            rc->nativeOperations.Start();
        } else {
            if (textDrawInfo->shieldRes.length() > 0) {
                SkBitmap* ico = getCachedBitmap(rc, textDrawInfo->shieldRes);
                if (ico != NULL) {
                    float left = textDrawInfo->centerX - rc->getDensityValue(ico->width() / 2) - 0.5f;
                    float top = textDrawInfo->centerY - rc->getDensityValue(ico->height() / 2)
                        - rc->getDensityValue(4.5f);
                    SkRect r = SkRect::MakeXYWH(left, top, rc->getDensityValue(ico->width()),
                        rc->getDensityValue(ico->height()));
                    PROFILE_NATIVE_OPERATION(rc, cv->drawBitmapRect(*ico, (SkIRect*) NULL, r, &paintIcon));
                }
            }
            drawWrappedText(rc, cv, textDrawInfo, textSize, paintText);
        }*/
    }
}

void OsmAnd::Rasterizer::collectPrimitivesTexts( RasterizerContext& context, const QVector< Rasterizer::Primitive >& primitives, PrimitivesType type, IQueryController* controller )
{
    assert(type != PrimitivesType::ShadowOnlyLines);

    for(auto itPrimitive = primitives.begin(); itPrimitive != primitives.end(); ++itPrimitive)
    {
        if(controller && controller->isAborted())
            return;

        const auto& primitive = *itPrimitive;

        // Skip primitives without names
        if(primitive.mapObject->names.isEmpty())
            continue;
        bool hasNonEmptyNames = false;
        for(auto itName = primitive.mapObject->names.begin(); itName != primitive.mapObject->names.end(); ++itName)
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
            if (primitive.zOrder < context._polygonMinSizeToDisplay * context._densityFactor)
                return;

            collectPolygonText(context, primitive);
        }
        else if(type == Lines)
        {
            collectLineText(context, primitive);
        }
        else if(type == Points)
        {
            if(primitive.typeIndex != 0)
                continue;

            collectPointText(context, primitive);
        }
    }
}

void OsmAnd::Rasterizer::collectPolygonText( RasterizerContext& context, const Primitive& primitive )
{
    if(primitive.mapObject->_points31.size() <=2)
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rasterized (text) as polygon, but has %d vertices", primitive.mapObject->id >> 1, primitive.mapObject->_points31.size());
        return;
    }
    if(!primitive.mapObject->isClosedFigure())
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rasterized (text) as polygon, but is not closed", primitive.mapObject->id >> 1);
        return;
    }
    if(!primitive.mapObject->isClosedFigure(true))
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rasterized (text) as polygon, but is not closed (inner)", primitive.mapObject->id >> 1);
        return;
    }

    {
        const auto& type = primitive.mapObject->_types[primitive.typeIndex];
        MapStyleEvaluator evaluator(context.style, MapStyle::RulesetType::Polygon, primitive.mapObject);
        context.applyTo(evaluator);
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, type.tag);
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, type.value);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
        if(!evaluator.evaluate())
            return;
        if(!updatePaint(context, evaluator, Set_0, true))
            return;
    }

    PointF textPoint;

    bool containsPoint = false;
    PointF vertex;
    int bounds = 0;
    QVector< PointF > outsideBounds;
    for(auto itPoint = primitive.mapObject->_points31.begin(); itPoint != primitive.mapObject->_points31.end(); ++itPoint)
    {
        const auto& point = *itPoint;

        calculateVertex(context, point, vertex);

        textPoint.x += qMin( qMax(vertex.x, context._renderViewport.left), context._renderViewport.right);
        textPoint.y += qMin( qMax(vertex.y, context._renderViewport.bottom), context._renderViewport.top);
        
        if (!containsPoint)
        {
            if(context._renderViewport.contains(vertex))
            {
                containsPoint = true;
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

    if(!containsPoint)
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

    preparePrimitiveText(context, primitive, textPoint, nullptr);
}

void OsmAnd::Rasterizer::collectLineText( RasterizerContext& context, const Primitive& primitive )
{
    if(primitive.mapObject->_points31.size() < 2 )
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rasterized (text) as line, but has %d vertices", primitive.mapObject->id >> 1, primitive.mapObject->_points31.size());
        return;
    }

    {
        const auto& type = primitive.mapObject->_types[primitive.typeIndex];
        MapStyleEvaluator evaluator(context.style, MapStyle::RulesetType::Line, primitive.mapObject);
        context.applyTo(evaluator);
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_TAG, type.tag);
        evaluator.setStringValue(MapStyle::builtinValueDefinitions.INPUT_VALUE, type.value);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
        evaluator.setIntegerValue(MapStyle::builtinValueDefinitions.INPUT_LAYER, primitive.mapObject->getSimpleLayerValue());
        if(!evaluator.evaluate())
            return;
        if(!updatePaint(context, evaluator, Set_0, false))
            return;
    }

    SkPath path;
    int pointIdx = 0;
    const auto middleIdx = primitive.mapObject->_points31.size() >> 1;
    bool intersect = false;
    int prevCross = 0;
    PointF vertex, middleVertex;
    for(auto itPoint = primitive.mapObject->_points31.begin(); itPoint != primitive.mapObject->_points31.end(); ++itPoint, pointIdx++)
    {
        const auto& point = *itPoint;

        calculateVertex(context, point, vertex);

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
        preparePrimitiveText(context, primitive, middleVertex, &path);
    }
}

void OsmAnd::Rasterizer::collectPointText( RasterizerContext& context, const Primitive& primitive )
{
    if(primitive.mapObject->_points31.size() < 1 )
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rasterized (text) as point, but has %d vertices", primitive.mapObject->id >> 1, primitive.mapObject->_points31.size());
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
        for(auto itPoint = primitive.mapObject->_points31.begin(); itPoint != primitive.mapObject->_points31.end(); ++itPoint)
        {
            const auto& point = *itPoint;

            calculateVertex(context, point, vertex);

            textPoint += vertex;
        }

        textPoint.x /= verticesCount;
        textPoint.y /= verticesCount;
    }
    
    preparePrimitiveText(context, primitive, textPoint, nullptr);
}

void OsmAnd::Rasterizer::preparePrimitiveText( RasterizerContext& context, const Primitive& primitive, const PointF& point, SkPath* path )
{
    const auto& type = primitive.mapObject->_types[primitive.typeIndex];

    for(auto itName = primitive.mapObject->names.begin(); itName != primitive.mapObject->names.end(); ++itName)
    {
        const auto& name = itName.value();

        //TODO:name =rc->getTranslatedString(name);
        //TODO:name =rc->getReshapedString(name);

        const auto& type = primitive.mapObject->_types[primitive.typeIndex];
        MapStyleEvaluator evaluator(context.style, MapStyle::RulesetType::Text, primitive.mapObject);
        context.applyTo(evaluator);
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
