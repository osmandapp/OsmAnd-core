#include "Rasterizer.h"

#include <QtGlobal>

#include "Logging.h"
#include "Utilities.h"
#include "RasterizationStyleContext.h"
#include "RasterizationStyleEvaluator.h"
#include "ObfMapSection.h"

OsmAnd::Rasterizer::Rasterizer()
{
}

OsmAnd::Rasterizer::~Rasterizer()
{
}

bool OsmAnd::Rasterizer::rasterize(
    SkCanvas& canvas,
    const QList< std::shared_ptr<OsmAnd::Model::MapObject> >& objects,
    const RasterizationStyleContext& styleContext,
    uint32_t zoom,
    IQueryController* controller /*= nullptr*/)
{
    Context context;
    context._paint.setAntiAlias(true);

    // Fill atrributes
    /*
    TODO:attributes
    req->clearState();
	req->setIntFilter(req->props()->R_MINZOOM, rc.getZoom());
	if (req->searchRenderingAttribute("defaultColor")) {
		rc.setDefaultColor(req->getIntPropertyValue(req->props()->R_ATTR_COLOR_VALUE));
	}
	req->clearState();
	req->setIntFilter(req->props()->R_MINZOOM, rc.getZoom());
	if (req->searchRenderingAttribute("shadowRendering")) {
		rc.setShadowRenderingMode(req->getIntPropertyValue(req->props()->R_ATTR_INT_VALUE));
		rc.setShadowRenderingColor(req->getIntPropertyValue(req->props()->R_SHADOW_COLOR));
	}
	req->clearState();
	req->setIntFilter(req->props()->R_MINZOOM, rc.getZoom());
	if (req->searchRenderingAttribute("polygonMinSizeToDisplay")) {
		rc.polygonMinSizeToDisplay = req->getIntPropertyValue(req->props()->R_ATTR_INT_VALUE);
	}
	req->clearState();
	req->setIntFilter(req->props()->R_MINZOOM, rc.getZoom());
	if (req->searchRenderingAttribute("roadDensityZoomTile")) {
		rc.roadDensityZoomTile = req->getIntPropertyValue(req->props()->R_ATTR_INT_VALUE);
	}
	req->clearState();
	req->setIntFilter(req->props()->R_MINZOOM, rc.getZoom());
	if (req->searchRenderingAttribute("roadsDensityLimitPerTile")) {
		rc.roadsDensityLimitPerTile = req->getIntPropertyValue(req->props()->R_ATTR_INT_VALUE);
	}
    */

    QVector< Primitive > polygons, lines, points;
    obtainPrimitives(context, styleContext, objects, zoom, polygons, lines, points, controller);

    context._lastRenderedKey = 0;
    rasterizePrimitives(context, canvas, styleContext, zoom, polygons, Polygons, controller);
    context._lastRenderedKey = 5;
    /*if (context._shadowRenderingMode > 1)
        rasterizePrimitives(context, canvas, styleContext, zoom, lines, ShadowOnlyLines, controller);*/

    context._lastRenderedKey = 40;
    rasterizePrimitives(context, canvas, styleContext, zoom, lines, Lines, controller);
    context._lastRenderedKey = 60;

    rasterizePrimitives(context, canvas, styleContext, zoom, points, Points, controller);
    context._lastRenderedKey = 125;
    /*
    drawIconsOverCanvas(rc, canvas);

    rc->textRendering.Start();
    drawTextOverCanvas(rc, canvas);
    rc->textRendering.Pause();
    */
    return true;
}

void OsmAnd::Rasterizer::obtainPrimitives(
    Context& context,
    const RasterizationStyleContext& styleContext,
    const QList< std::shared_ptr<OsmAnd::Model::MapObject> >& objects,
    uint32_t zoom,
    QVector< Primitive >& polygons,
    QVector< Primitive >& lines,
    QVector< Primitive >& points,
    IQueryController* controller)
{
    auto mult = 1.0 / Utilities::getPowZoom(qMax(31 - (zoom + 8), 0U));
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
            auto typeId = *itType;
            auto type = mapObject->section->rules->decodingRules[typeId];
            const auto& tag = std::get<0>(type);
            const auto& value = std::get<1>(type);
            auto layer = mapObject->getSimpleLayerValue();

            RasterizationStyleEvaluator evaluator(styleContext, RasterizationStyle::RulesetType::Order, mapObject);
            evaluator.setStringValue(RasterizationStyle::builtinValueDefinitions.INPUT_TAG, tag);
            evaluator.setStringValue(RasterizationStyle::builtinValueDefinitions.INPUT_VALUE, value);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MAXZOOM, zoom);
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
                    context._shadowLevelMin = qMin(context._shadowLevelMin, static_cast<uint8_t>(zOrder));
                    context._shadowLevelMax = qMax(context._shadowLevelMax, static_cast<uint8_t>(zOrder));
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
    filterOutLinesByDensity(context, zoom, unfilteredLines, lines, controller);
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

void OsmAnd::Rasterizer::filterOutLinesByDensity( Context& context, uint32_t zoom, const QVector< Primitive >& in, QVector< Primitive >& out, IQueryController* controller )
{
    if(context._roadDensityZoomTile == 0 || context._roadsDensityLimitPerTile == 0)
    {
        out = in;
        return;
    }

    const auto dZ = zoom + context._roadDensityZoomTile;
    QMap< uint64_t, std::pair<uint32_t, double> > densityMap;
    out.reserve(in.size());
    for(int lineIdx = in.size() - 1; lineIdx >= 0; lineIdx--)
    {
        if(controller && controller->isAborted())
            return;

        bool accept = true;
        const auto& primitive = in[lineIdx];

        auto typeId = primitive.mapObject->_types[primitive.typeIndex];
        const auto& type = primitive.mapObject->section->rules->decodingRules[typeId];
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

void OsmAnd::Rasterizer::rasterizePrimitives( Context& context, SkCanvas& canvas, const RasterizationStyleContext& styleContext, uint32_t zoom, const QVector< Primitive >& primitives, PrimitivesType type, IQueryController* controller )
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
            
            rasterizePolygon(context, canvas, styleContext, zoom, primitive);
        }
        else if(type == Lines || type == ShadowOnlyLines)
        {
            //TODO:drawPolyline(mObj, req, cv, paint, rc, pair, mObj->getSimpleLayer(), objOrder == 1);
        }
        else if(type == Points)
        {
            //TODO:drawPoint(mObj, req, cv, paint, rc, pair, array[i].typeInd == 0);
        }
    }
}

bool OsmAnd::Rasterizer::updatePaint( Context& context, const RasterizationStyleEvaluator& evaluator, int idx, bool isArea )
{
    /*
    struct ValueSet
    {
        const std::shared_ptr<RasterizationStyle::ValueDefinition>& color;
        const std::shared_ptr<RasterizationStyle::ValueDefinition>& strokeWidth;
        const std::shared_ptr<RasterizationStyle::ValueDefinition>& cap;
        const std::shared_ptr<RasterizationStyle::ValueDefinition>& pathEffect;
    };
    static ValueSet sets[] =
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
        auto stroke = evaluator.getFloatValue(rStrokeW);
        if (!(stroke > 0))
            return false;

        context._paint.setColorFilter(nullptr);
        context._paint.setShader(nullptr);
        context._paint.setLooper(nullptr);
        context._paint.setStyle(SkPaint::kStroke_Style);
        context._paint.setStrokeWidth(stroke);

        const auto& cap = evaluator.getStringValue(rCap);
        const auto& pathEff = evaluator.getStringValue(rPathEff);
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
            SkPathEffect* p = getDashEffect(pathEff);
            context._paint.setPathEffect(p);
        }
        else
        {
            context._paint.setPathEffect(nullptr);
        }
    }

    auto color = evaluator.getIntegerValue(rColor);
    context._paint.setColor(color);

    if (ind == 0)
    {
        std::string shader = req->getStringPropertyValue(req->props()->R_SHADER);
        if (shader.size() > 0)
        {
            SkBitmap* bmp = getCachedBitmap(rc, shader);
            if (bmp != NULL)
                paint->setShader(new SkBitmapProcShader(*bmp, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode))->unref();
        }
    }

    // do not check shadow color here
    if (rc->getShadowRenderingMode() == 1 && ind == 0)
    {
        int shadowColor = req->getIntPropertyValue(req->props()->R_SHADOW_COLOR);
        int shadowLayer = req->getIntPropertyValue(req->props()->R_SHADOW_RADIUS);
        if (shadowColor == 0) {
            shadowColor = rc->getShadowRenderingColor();
        }
        if (shadowColor == 0)
            shadowLayer = 0;

        if (shadowLayer > 0)
            paint->setLooper(new SkBlurDrawLooper(shadowLayer, 0, 0, shadowColor))->unref();
    }
    return 1;*/
    return true;
}

void OsmAnd::Rasterizer::rasterizePolygon( Context& context, SkCanvas& canvas, const RasterizationStyleContext& styleContext, uint32_t zoom, const Primitive& primitive )
{
    if(primitive.mapObject->_coordinates.size() <=2 )
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Warning, "Map object #%llu is rendered as polygon, but has %d vertices\n", primitive.mapObject->id, primitive.mapObject->_coordinates.size());
        return;
    }
    /*
    const auto& tagValuePair = primitive.mapObject->section->rules->decodingRules[ primitive.mapObject->_types[primitive.typeIndex] ];
    
    RasterizationStyleEvaluator evaluator(styleContext, RasterizationStyle::RulesetType::Polygon, primitive.mapObject);
    evaluator.setStringValue(RasterizationStyle::builtinValueDefinitions.INPUT_TAG, std::get<0>(tagValuePair));
    evaluator.setStringValue(RasterizationStyle::builtinValueDefinitions.INPUT_VALUE, std::get<1>(tagValuePair));
    evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
    evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MAXZOOM, zoom);
    if(!evaluator.evaluate())
        return;
    if(!updatePaint(context, evaluator, 0, true))
        return;

    float xText = 0;
    float yText = 0;

    rc->visible++;
    SkPath path;
    int i = 0;
    bool containsPoint = false;
    int bounds = 0;
    std::vector< std::pair<int,int > > ps;
    for (; i < length; i++) {
        calcPoint(mObj->points.at(i), rc);
        if (i == 0) {
            path.moveTo(rc->calcX, rc->calcY);
        } else {
            path.lineTo(rc->calcX, rc->calcY);
        }
        float tx = rc->calcX;
        if (tx < 0) {
            tx = 0;
        }
        if (tx > rc->getWidth()) {
            tx = rc->getWidth();
        }
        float ty = rc->calcY;
        if (ty < 0) {
            ty = 0;
        }
        if (ty > rc->getHeight()) {
            ty = rc->getHeight();
        }
        xText += tx;
        yText += ty;
        if (!containsPoint) {
            if (rc->calcX >= 0 && rc->calcY >= 0 && rc->calcX < rc->getWidth() && rc->calcY < rc->getHeight()) {
                containsPoint = true;
            } else {
                ps.push_back(std::pair<int, int>(rc->calcX, rc->calcY));
            }
            bounds |= (rc->calcX < 0 ? 1 : 0);
            bounds |= (rc->calcX >= rc->getWidth() ? 2 : 0);
            bounds |= (rc->calcY >= rc->getHeight()  ? 4 : 0);
            bounds |= (rc->calcY <= rc->getHeight() ? 8 : 0);
        }
    }
    xText /= length;
    yText /= length;
    if(!containsPoint){
        // fast check for polygons
        if((bounds&3 != 3) || (bounds >> 2) != 3) {
            return;
        }
        if(contains(ps, 0, 0) ||
            contains(ps, rc->getWidth(), rc->getHeight()) ||
            contains(ps, 0, rc->getHeight()) ||
            contains(ps, rc->getWidth(), 0)) {
                if(contains(ps, rc->getWidth() / 2, rc->getHeight() / 2)) {
                    xText = rc->getWidth() / 2;
                    yText = rc->getHeight() / 2;
                }
        } else {
            return;
        }
    }
    std::vector<coordinates> polygonInnerCoordinates = mObj->polygonInnerCoordinates;
    if (polygonInnerCoordinates.size() > 0) {
        path.setFillType(SkPath::kEvenOdd_FillType);
        for (int j = 0; j < polygonInnerCoordinates.size(); j++) {
            coordinates cs = polygonInnerCoordinates.at(j);
            for (int i = 0; i < cs.size(); i++) {
                calcPoint(cs[i], rc);
                if (i == 0) {
                    path.moveTo(rc->calcX, rc->calcY);
                } else {
                    path.lineTo(rc->calcX, rc->calcY);
                }
            }
        }
    }

    PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
    if (updatePaint(req, paint, 1, 0, rc)) {
        PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
    }

    renderText(mObj, req, rc, pair.first, pair.second, xText, yText, NULL);
    */
}

OsmAnd::Rasterizer::Context::Context()
    : _shadowLevelMin(0)
    , _shadowLevelMax(256)
    , _roadDensityZoomTile(0)
    , _roadsDensityLimitPerTile(0)
{
}
