#include "Rasterizer.h"

#include <QtGlobal>

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
        /*
        MapDataObject* mObj = array[i].obj;
        tag_value pair = mObj->types.at(array[i].typeInd);

        if(type == Polygons)
        {
            if (primitive.zOrder < context._polygonMinSizeToDisplay)
                return;
            
            drawPolygon(mObj, req, cv, paint, rc, pair);
        }
        else if(type == Lines || type == ShadowOnlyLines)
        {
            drawPolyline(mObj, req, cv, paint, rc, pair, mObj->getSimpleLayer(), objOrder == 1);
        }
        else if(type == Points)
        {
            drawPoint(mObj, req, cv, paint, rc, pair, array[i].typeInd == 0);
        }*/
    }
}

OsmAnd::Rasterizer::Context::Context()
    : _shadowLevelMin(0)
    , _shadowLevelMax(256)
    , _roadDensityZoomTile(0)
    , _roadsDensityLimitPerTile(0)
{
}
