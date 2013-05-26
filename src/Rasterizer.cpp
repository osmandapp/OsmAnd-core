#include "Rasterizer.h"

#include <SkPaint.h>
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
    uint32_t zoom,
    const RasterizationStyleContext& styleContext,
    IQueryController* controller /*= nullptr*/)
{
    SkPaint paint;
    paint.setAntiAlias(true);

    QList< Primitive > polygons, lines, points;
    obtainPrimitives(styleContext, objects, zoom, polygons, lines, points);
    /*TODO:
    rc->lastRenderedKey = 0;

    drawObject(rc, canvas, req, paint, polygonsArray, 0);
    rc->lastRenderedKey = 5;
    if (rc->getShadowRenderingMode() > 1) {
        drawObject(rc, canvas, req, paint, linesArray, 1);
    }
    rc->lastRenderedKey = 40;
    drawObject(rc, canvas, req, paint, linesArray, 2);
    rc->lastRenderedKey = 60;

    drawObject(rc, canvas, req, paint, pointsArray, 3);
    rc->lastRenderedKey = 125;

    drawIconsOverCanvas(rc, canvas);

    rc->textRendering.Start();
    drawTextOverCanvas(rc, canvas);
    rc->textRendering.Pause();
    */

    return true;
}

void OsmAnd::Rasterizer::obtainPrimitives(
    const RasterizationStyleContext& styleContext,
    const QList< std::shared_ptr<OsmAnd::Model::MapObject> >& objects,
    uint32_t zoom,
    QList< Primitive >& polygons,
    QList< Primitive >& lines,
    QList< Primitive >& points)
{
    auto mult = 1.0 / Utilities::getPowZoom(qMax(31 - (zoom + 8), 0U));
    mult *= mult;
    uint32_t idx = 0;
    for(auto itMapObject = objects.begin(); itMapObject != objects.end(); ++itMapObject, idx++)
    {
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
            evaluator.setIntegerValue(evaluator.styleContext.style->INPUT_MINZOOM, zoom);
            evaluator.setIntegerValue(evaluator.styleContext.style->INPUT_MAXZOOM, zoom);
            evaluator.setIntegerValue(evaluator.styleContext.style->INPUT_LAYER, layer);
            evaluator.setStringValue(evaluator.styleContext.style->INPUT_TAG, tag);
            evaluator.setStringValue(evaluator.styleContext.style->INPUT_VALUE, value);
            evaluator.setBooleanValue(evaluator.styleContext.style->INPUT_AREA, mapObject->_isArea);
            evaluator.setBooleanValue(evaluator.styleContext.style->INPUT_POINT, mapObject->_coordinates.size() == 1);
            evaluator.setBooleanValue(evaluator.styleContext.style->INPUT_CYCLE, mapObject->isClosedFigure());
            if(evaluator.evaluate())
            {
                auto objectType = static_cast<PrimitiveType>(evaluator.getIntegerValue(evaluator.styleContext.style->OUTPUT_OBJECT_TYPE));
                auto zOrder = evaluator.getIntegerValue(evaluator.styleContext.style->OUTPUT_ORDER);

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
                    lines.push_back(primitive);
                }

                if (evaluator.getIntegerValue(evaluator.styleContext.style->OUTPUT_SHADOW_LEVEL) > 0)
                {
//TODO:                    rc->shadowLevelMin = std::min(rc->shadowLevelMin, zOrder);
//TODO:                    rc->shadowLevelMax = std::max(rc->shadowLevelMax, zOrder);
                }
            }
        }
    }

    qSort(polygons.begin(), polygons.end(), [](const Primitive& l, Primitive& r) -> bool
    {
        if( qFuzzyCompare(l.zOrder, r.zOrder) )
            return l.typeIndex < r.typeIndex;
        return l.zOrder > r.zOrder;
    });
    qSort(lines.begin(), lines.end(), [](const Primitive& l, Primitive& r) -> bool
    {
        if(qFuzzyCompare(l.zOrder, r.zOrder))
        {
            if(l.typeIndex == r.typeIndex)
                return l.mapObject->_coordinates.size() < r.mapObject->_coordinates.size();
            return l.typeIndex < r.typeIndex;
        }
        return l.zOrder < r.zOrder;
    });
    qSort(points.begin(), points.end(), [](const Primitive& l, Primitive& r) -> bool
    {
        if(qFuzzyCompare(l.zOrder, r.zOrder))
        {
            if(l.typeIndex == r.typeIndex)
                return l.mapObject->_coordinates.size() < r.mapObject->_coordinates.size();
            return l.typeIndex < r.typeIndex;
        }
        return l.zOrder < r.zOrder;
    });
    //TODO:filterLinesByDensity(rc, linesResArray, linesArray);
}

