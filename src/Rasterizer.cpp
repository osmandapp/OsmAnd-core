#include "Rasterizer.h"

#include <QtGlobal>

#include "Logging.h"
#include "Utilities.h"
#include "RasterizationStyleEvaluator.h"
#include "ObfMapSection.h"
#include "RasterizerContext.h"

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
    const AreaD& area,
    uint32_t zoom,
    uint32_t tileSidePixelLength,
    const QList< std::shared_ptr<OsmAnd::Model::MapObject> >* objects /*= nullptr*/,
    const PointF& tlOriginOffset /*= PointF() */,
    IQueryController* controller /*= nullptr*/)
{
    bool doUpdate = false;
    doUpdate = doUpdate || context.update(area, zoom, tlOriginOffset, tileSidePixelLength);
    doUpdate = doUpdate || (objects != nullptr);

    if(!doUpdate)
        return;

    // Split input map objects to object, coastline, basemapObjects and basemapCoastline
    context._hasBaseMap = false;
    context._hasLand = false;
    context._hasWater = false;
    context._mapObjects.clear();
    context._coastline.clear();
    context._basemapMapObjects.clear();
    context._basemapCoastline.clear();
    for(auto itMapObject = objects->begin(); itMapObject != objects->end(); ++itMapObject)
    {
        if(controller && controller->isAborted())
            return;

        const auto& mapObject = *itMapObject;

        context._hasLand = context._hasLand || mapObject->foundation == Model::MapObject::FullLand;
        context._hasWater = context._hasWater || mapObject->foundation == Model::MapObject::FullWater;
        context._hasBaseMap = context._hasBaseMap || mapObject->section->isBaseMap;
        if(zoom < ZoomOnlyForBasemaps && !mapObject->section->isBaseMap)
            continue;

        if(mapObject->containsType("natural", "coastline"))
        {
            if (mapObject->section->isBaseMap)
                context._basemapCoastline.push_back(mapObject);
            else
                context._coastline.push_back(mapObject);
        }
        else
        {
            if (mapObject->section->isBaseMap)
                context._basemapMapObjects.push_back(mapObject);
            else
                context._mapObjects.push_back(mapObject);
        }
    }

/*
    if (renderRouteDataFile >= 0 && q->zoom >= zoomOnlyForBasemaps) {
        IDS_SET ids;
        map<std::string, BinaryMapFile*>::iterator i = openFiles.begin();
        for (; i != openFiles.end() && !q->publisher->isCancelled(); i++) {
            BinaryMapFile* file = i->second;
            if (q->req != NULL) {
                q->req->clearState();
            }
            q->publisher->result.clear();
            readRouteDataAsMapObjects(q, file, tempResult, skipDuplicates, ids);
        }
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Route objects %d", tempResult.size());
    }
*/
    if(controller && controller->isAborted())
    {
        context._mapObjects.clear();
        context._coastline.clear();
        context._basemapMapObjects.clear();
        context._basemapCoastline.clear();
        return;
    }
    /*
    bool addBasemapCoastlines = true;
    bool emptyData = q->zoom > BASEMAP_ZOOM && tempResult.empty() && coastLines.empty();
    // determine if there are enough objects like land/lake..
    bool basemapMissing = q->zoom <= BASEMAP_ZOOM && basemapCoastLines.empty() && !basemapExists;
    bool detailedLandData = q->zoom >= 14 && tempResult.size() > 0 && renderRouteDataFile < 0;
    if (!coastLines.empty()) {
        bool coastlinesWereAdded = processCoastlines(coastLines, q->left, q->right, q->bottom, q->top, q->zoom,
            basemapCoastLines.empty(), true, tempResult);
        addBasemapCoastlines = (!coastlinesWereAdded && !detailedLandData) || q->zoom <= BASEMAP_ZOOM;
    } else {
        addBasemapCoastlines = !detailedLandData;
    }
    if (addBasemapCoastlines) {
        addBasemapCoastlines = false;
        bool coastlinesWereAdded = processCoastlines(basemapCoastLines, q->left, q->right, q->bottom, q->top, q->zoom,
            true, true, tempResult);
        addBasemapCoastlines = !coastlinesWereAdded;
    }
    // processCoastlines always create new objects
    deleteObjects(basemapCoastLines);
    deleteObjects(coastLines);
    if (addBasemapCoastlines)
    {
        std::shared_ptr<Model::MapObject> bgMapObject(new Model::MapObject(nullptr));
        bgMapObject->_coordinates.push_back(int_pair(q->left, q->top));
        bgMapObject->_coordinates.push_back(int_pair(q->right, q->top));
        bgMapObject->_coordinates.push_back(int_pair(q->right, q->bottom));
        bgMapObject->_coordinates.push_back(int_pair(q->left, q->bottom));
        bgMapObject->_coordinates.push_back(bgMapObject->_coordinates.first());
        if (context._hasWater && !context._hasLand)
            bgMapObject->_types.push_back(tag_value("natural", "coastline"));
        else
            bgMapObject->_types.push_back(tag_value("natural", "land"));
        tempResult.push_back(o);
    }
    if (emptyData || basemapMissing) {
        // message
        // avoid overflow int errors
        MapDataObject* o = new MapDataObject();
        o->points.push_back(int_pair(q->left + (q->right - q->left) / 2, q->top + (q->bottom - q->top) / 2));
        o->types.push_back(tag_value("natural", "coastline"));
        o->objectNames["name"] = msgNothingFound;
        tempResult.push_back(o);
    }
    if (q->zoom <= BASEMAP_ZOOM || emptyData || (renderRouteDataFile >= 0 && q->zoom < 15)) {
        tempResult.insert(tempResult.end(), basemapResult.begin(), basemapResult.end());
    } else {
        deleteObjects(basemapResult);
    }
    q->publisher->result.clear();
    q->publisher->publish(tempResult);
    */
    obtainPrimitives(context, *objects, context._polygons, context._lines, context._points, controller);
}

bool OsmAnd::Rasterizer::rasterizeMap(
    RasterizerContext& context,
    bool fillBackground,
    SkCanvas& canvas,
    IQueryController* controller /*= nullptr*/)
{
    context._paint.setColor(context._defaultColor);
    if(fillBackground)
        canvas.drawRectCoords(context._renderViewport.top, context._renderViewport.left, context._renderViewport.right, context._renderViewport.bottom, context._paint);

    rasterizeMapPrimitives(context, canvas, context._polygons, Polygons, controller);

    if (context._shadowRenderingMode > 1)
        rasterizeMapPrimitives(context, canvas, context._lines, ShadowOnlyLines, controller);

    rasterizeMapPrimitives(context, canvas, context._lines, Lines, controller);

    return true;
}

void OsmAnd::Rasterizer::obtainPrimitives(
    RasterizerContext& context,
    const QList< std::shared_ptr<OsmAnd::Model::MapObject> >& objects,
    QVector< Primitive >& polygons,
    QVector< Primitive >& lines,
    QVector< Primitive >& points,
    IQueryController* controller)
{
    auto mult = 1.0 / Utilities::getPowZoom(qMax(31 - (context._zoom + 8), 0U));
    mult *= mult;
    uint32_t idx = 0;

    QVector< Primitive > unfilteredLines;
    for(auto itMapObject = objects.begin(); itMapObject != objects.end(); ++itMapObject, idx++)
    {
        if(controller && controller->isAborted())
            return;

        auto mapObject = *itMapObject;
        auto sh = idx << 8;

        uint32_t typeIdx = 0;
        for(auto itType = mapObject->_types.begin(); itType != mapObject->_types.end(); ++itType, typeIdx++)
        {
            const auto& type = *itType;
            const auto& tag = std::get<0>(type);
            const auto& value = std::get<1>(type);
            auto layer = mapObject->getSimpleLayerValue();

            RasterizationStyleEvaluator evaluator(context.style, RasterizationStyle::RulesetType::Order, mapObject);
            context.applyContext(evaluator);
            evaluator.setStringValue(RasterizationStyle::builtinValueDefinitions.INPUT_TAG, tag);
            evaluator.setStringValue(RasterizationStyle::builtinValueDefinitions.INPUT_VALUE, value);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_LAYER, layer);
            evaluator.setBooleanValue(RasterizationStyle::builtinValueDefinitions.INPUT_AREA, mapObject->_isArea);
            evaluator.setBooleanValue(RasterizationStyle::builtinValueDefinitions.INPUT_POINT, mapObject->_coordinates.size() == 1);
            evaluator.setBooleanValue(RasterizationStyle::builtinValueDefinitions.INPUT_CYCLE, mapObject->isClosedFigure());
            if(evaluator.evaluate())
            {
                auto objectType = static_cast<PrimitiveType>(evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_OBJECT_TYPE));
                auto zOrder = evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_ORDER);

                Primitive primitive;
                primitive.mapObject = mapObject;
                primitive.objectType = objectType;
                primitive.zOrder = zOrder;
                primitive.typeIndex = typeIdx;

                if(objectType == PrimitiveType::Polygon)
                {
                    Primitive pointPrimitive = primitive;
                    pointPrimitive.objectType = PrimitiveType::Point;
                    primitive.zOrder = Utilities::polygonArea(mapObject->_coordinates) * mult;
                    if(primitive.zOrder > MaxV)
                    { 
                        polygons.push_back(primitive);
                        points.push_back(pointPrimitive);
                    }
                }
                else if(objectType == PrimitiveType::Point)
                {
                    points.push_back(primitive);
                }
                else //if(objectType == RasterizationStyle::ObjectType::Line)
                {
                    unfilteredLines.push_back(primitive);
                }

                if (evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_SHADOW_LEVEL) > 0)
                {
                    context._shadowLevelMin = qMin(context._shadowLevelMin, static_cast<uint32_t>(zOrder));
                    context._shadowLevelMax = qMax(context._shadowLevelMax, static_cast<uint32_t>(zOrder));
                }
            }
        }
    }

    qSort(polygons.begin(), polygons.end(), [](const Primitive& l, const Primitive& r) -> bool
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
                return l.mapObject->_coordinates.size() < r.mapObject->_coordinates.size();
            return l.typeIndex < r.typeIndex;
        }
        return l.zOrder < r.zOrder;
    });
    filterOutLinesByDensity(context, unfilteredLines, lines, controller);
    qSort(points.begin(), points.end(), [](const Primitive& l, const Primitive& r) -> bool
    {
        if(qFuzzyCompare(l.zOrder, r.zOrder))
        {
            if(l.typeIndex == r.typeIndex)
                return l.mapObject->_coordinates.size() < r.mapObject->_coordinates.size();
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
        if (std::get<0>(type) == "highway")
        {
            accept = false;

            uint64_t prevId = 0;
            for(auto itPoint = primitive.mapObject->_coordinates.begin(); itPoint != primitive.mapObject->_coordinates.end(); ++itPoint)
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
    for(auto itPrimitive = primitives.begin(); itPrimitive != primitives.end(); ++itPrimitive)
    {
        if(controller && controller->isAborted())
            return;

        const auto& primitive = *itPrimitive;

        if(type == Polygons)
        {
            if (primitive.zOrder < context._polygonMinSizeToDisplay)
                return;

            rasterizePolygon(context, canvas, primitive);
        }
        else if(type == Lines || type == ShadowOnlyLines)
        {
            rasterizeLine(context, canvas, primitive, type == ShadowOnlyLines);
        }
    }
}

bool OsmAnd::Rasterizer::updatePaint( RasterizerContext& context, const RasterizationStyleEvaluator& evaluator, PaintValuesSet valueSetSelector, bool isArea )
{
    struct ValueSet
    {
        const std::shared_ptr<RasterizationStyle::ValueDefinition>& color;
        const std::shared_ptr<RasterizationStyle::ValueDefinition>& strokeWidth;
        const std::shared_ptr<RasterizationStyle::ValueDefinition>& cap;
        const std::shared_ptr<RasterizationStyle::ValueDefinition>& pathEffect;
    };
    static ValueSet valueSets[] =
    {
        {//0
            RasterizationStyle::builtinValueDefinitions.OUTPUT_COLOR,
                RasterizationStyle::builtinValueDefinitions.OUTPUT_STROKE_WIDTH,
                RasterizationStyle::builtinValueDefinitions.OUTPUT_CAP,
                RasterizationStyle::builtinValueDefinitions.OUTPUT_PATH_EFFECT
        },
        {//1
            RasterizationStyle::builtinValueDefinitions.OUTPUT_COLOR_2,
            RasterizationStyle::builtinValueDefinitions.OUTPUT_STROKE_WIDTH_2,
            RasterizationStyle::builtinValueDefinitions.OUTPUT_CAP_2,
            RasterizationStyle::builtinValueDefinitions.OUTPUT_PATH_EFFECT_2
        },
        {//-1
            RasterizationStyle::builtinValueDefinitions.OUTPUT_COLOR_0,
                RasterizationStyle::builtinValueDefinitions.OUTPUT_STROKE_WIDTH_0,
                RasterizationStyle::builtinValueDefinitions.OUTPUT_CAP_0,
                RasterizationStyle::builtinValueDefinitions.OUTPUT_PATH_EFFECT_0
        },
        {//-2
            RasterizationStyle::builtinValueDefinitions.OUTPUT_COLOR__1,
                RasterizationStyle::builtinValueDefinitions.OUTPUT_STROKE_WIDTH__1,
                RasterizationStyle::builtinValueDefinitions.OUTPUT_CAP__1,
                RasterizationStyle::builtinValueDefinitions.OUTPUT_PATH_EFFECT__1
        },
        {//else
            RasterizationStyle::builtinValueDefinitions.OUTPUT_COLOR_3,
            RasterizationStyle::builtinValueDefinitions.OUTPUT_STROKE_WIDTH_3,
            RasterizationStyle::builtinValueDefinitions.OUTPUT_CAP_3,
            RasterizationStyle::builtinValueDefinitions.OUTPUT_PATH_EFFECT_3
        },
    };
    const ValueSet& valueSet = valueSets[static_cast<int>(valueSetSelector)];

    if(isArea)
    {
        context._paint.setColorFilter(nullptr);
        context._paint.setShader(nullptr);
        context._paint.setLooper(nullptr);
        context._paint.setStyle(SkPaint::kStrokeAndFill_Style);
        context._paint.setStrokeWidth(0);
    }
    else
    {
        auto stroke = evaluator.getFloatValue(valueSet.strokeWidth);
        if (!(stroke > 0))
            return false;

        context._paint.setColorFilter(nullptr);
        context._paint.setShader(nullptr);
        context._paint.setLooper(nullptr);
        context._paint.setStyle(SkPaint::kStroke_Style);
        context._paint.setStrokeWidth(stroke);

        const auto& cap = evaluator.getStringValue(valueSet.cap);
        const auto& pathEff = evaluator.getStringValue(valueSet.pathEffect);
        if (cap.isEmpty() || cap == "BUTT")
            context._paint.setStrokeCap(SkPaint::kButt_Cap);
        else if (cap == "ROUND")
            context._paint.setStrokeCap(SkPaint::kRound_Cap);
        else if (cap == "SQUARE")
            context._paint.setStrokeCap(SkPaint::kSquare_Cap);
        else
            context._paint.setStrokeCap(SkPaint::kButt_Cap);

        if(!pathEff.isEmpty())
        {
            auto effect = context.obtainPathEffect(pathEff);
            context._paint.setPathEffect(effect);
        }
        else
        {
            context._paint.setPathEffect(nullptr);
        }
    }

    auto color = evaluator.getIntegerValue(valueSet.color);
    assert(color != 0);
    context._paint.setColor(color);

    if (valueSetSelector == PaintValuesSet::Set_0)
    {
        const auto& shader = evaluator.getStringValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_SHADER);
        if(!shader.isEmpty())
        {
            int i = 5;
            /*TODO:SkBitmap* bmp = getCachedBitmap(rc, shader);
            if (bmp != NULL)
            paint->setShader(new SkBitmapProcShader(*bmp, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode))->unref();*/
        }
    }

    // do not check shadow color here
    if (context._shadowRenderingMode == 1 && valueSetSelector == PaintValuesSet::Set_0)
    {
        auto shadowColor = evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_SHADOW_COLOR);
        auto shadowLayer = evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_SHADOW_RADIUS);
        if(shadowColor == 0)
            shadowColor = context._shadowRenderingColor;
        if(shadowColor == 0)
            shadowLayer = 0;

        if(shadowLayer > 0)
            context._paint.setLooper(new SkBlurDrawLooper(shadowLayer, 0, 0, shadowColor))->unref();
    }

    return true;
}

void OsmAnd::Rasterizer::rasterizePolygon( RasterizerContext& context, SkCanvas& canvas, const Primitive& primitive )
{
    if(primitive.mapObject->_coordinates.size() <=2)
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rendered as polygon, but has %d vertices\n", primitive.mapObject->id, primitive.mapObject->_coordinates.size());
        return;
    }
    if(!primitive.mapObject->isClosedFigure())
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rendered as polygon, but is not closed\n", primitive.mapObject->id);
        return;
    }
    if(!primitive.mapObject->isClosedFigure(true))
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rendered as polygon, but is not closed (inner)\n", primitive.mapObject->id);
        return;
    }

    const auto& tagValuePair = primitive.mapObject->_types[primitive.typeIndex];

    RasterizationStyleEvaluator evaluator(context.style, RasterizationStyle::RulesetType::Polygon, primitive.mapObject);
    context.applyContext(evaluator);
    evaluator.setStringValue(RasterizationStyle::builtinValueDefinitions.INPUT_TAG, std::get<0>(tagValuePair));
    evaluator.setStringValue(RasterizationStyle::builtinValueDefinitions.INPUT_VALUE, std::get<1>(tagValuePair));
    evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
    evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
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
    for(auto itPoint = primitive.mapObject->_coordinates.begin(); itPoint != primitive.mapObject->_coordinates.end(); ++itPoint, pointIdx++)
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

    if(!primitive.mapObject->_polygonInnerCoordinates.isEmpty())
    {
        path.setFillType(SkPath::kEvenOdd_FillType);
        for(auto itInnerPoints = primitive.mapObject->_polygonInnerCoordinates.begin(); itInnerPoints != primitive.mapObject->_polygonInnerCoordinates.end(); ++itInnerPoints)
        {
            const auto& innerPoints = *itInnerPoints;

            pointIdx = 0;
            for(auto itInnerPoint = innerPoints.begin(); itInnerPoint != innerPoints.end(); ++itInnerPoint, pointIdx++)
            {
                const auto& point = *itInnerPoint;
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

    canvas.drawPath(path, context._paint);
    if(updatePaint(context, evaluator, Set_1, false))
        canvas.drawPath(path, context._paint);
}

void OsmAnd::Rasterizer::rasterizeLine( RasterizerContext& context, SkCanvas& canvas, const Primitive& primitive, bool drawOnlyShadow )
{
    if(primitive.mapObject->_coordinates.size() < 2 )
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rendered as line, but has %d vertices\n", primitive.mapObject->id, primitive.mapObject->_coordinates.size());
        return;
    }

    const auto& tagValuePair = primitive.mapObject->_types[primitive.typeIndex];

    RasterizationStyleEvaluator evaluator(context.style, RasterizationStyle::RulesetType::Line, primitive.mapObject);
    context.applyContext(evaluator);
    evaluator.setStringValue(RasterizationStyle::builtinValueDefinitions.INPUT_TAG, std::get<0>(tagValuePair));
    evaluator.setStringValue(RasterizationStyle::builtinValueDefinitions.INPUT_VALUE, std::get<1>(tagValuePair));
    evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, context._zoom);
    evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MAXZOOM, context._zoom);
    evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_LAYER, primitive.mapObject->getSimpleLayerValue());
    if(!evaluator.evaluate())
        return;
    if(!updatePaint(context, evaluator, Set_0, false))
        return;

    auto shadowColor = evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_SHADOW_COLOR);
    auto shadowRadius = evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_SHADOW_RADIUS);
    if(drawOnlyShadow && shadowRadius == 0)
        return;

    if(shadowColor == 0)
        shadowColor = context._shadowRenderingColor;

    int oneway = 0;
    if (context._zoom >= 16 && std::get<0>(tagValuePair) == "highway")
    {
        if (primitive.mapObject->containsType("oneway", "yes", true))
            oneway = 1;
        else if (primitive.mapObject->containsType("oneway", "-1", true))
            oneway = -1;
    }

    SkPath path;
    int pointIdx = 0;
    const auto middleIdx = primitive.mapObject->_coordinates.size() >> 1;
    bool intersect = false;
    int prevCross = 0;
    PointF vertex, middleVertex;
    for(auto itPoint = primitive.mapObject->_coordinates.begin(); itPoint != primitive.mapObject->_coordinates.end(); ++itPoint, pointIdx++)
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
                canvas.drawPath(path, context._paint);
            }
            if (updatePaint(context, evaluator, Set_minus1, false))
            {
                canvas.drawPath(path, context._paint);
            }
            if (updatePaint(context, evaluator, Set_0, false))
            {
                canvas.drawPath(path, context._paint);
            }
            canvas.drawPath(path, context._paint);
            if (updatePaint(context, evaluator, Set_1, false))
            {
                canvas.drawPath(path, context._paint);
            }
            if (updatePaint(context, evaluator, Set_3, false))
            {
                canvas.drawPath(path, context._paint);
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
        context._paint.setLooper(new SkBlurDrawLooper(shadowRadius, 0, 0, shadowColor))->unref();
        canvas.drawPath(path, context._paint);
    }

    // option shadow = 3 with solid border
    if (context._shadowRenderingMode == 3 && shadowRadius > 0)
    {
        context._paint.setLooper(nullptr);
        context._paint.setStrokeWidth(context._paint.getStrokeWidth() + shadowRadius * 2);
        //		paint->setColor(0xffbababa);
        context._paint.setColorFilter(SkColorFilter::CreateModeFilter(shadowColor, SkXfermode::kSrcIn_Mode))->unref();
        //		paint->setColor(shadowColor);
        canvas.drawPath(path, context._paint);
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

void OsmAnd::Rasterizer::calculateVertex( RasterizerContext& context, const PointI& point, PointF& vertex )
{
    vertex.x = ((point.x / context._tileDivisor) - context._areaTileD.left) * context._tileSidePixelLength + context._renderViewport.left;
    vertex.y = ((point.y / context._tileDivisor) - context._areaTileD.top) * context._tileSidePixelLength + context._renderViewport.top;
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
