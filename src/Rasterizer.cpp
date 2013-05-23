#include "Rasterizer.h"

#include <SkPaint.h>
#include <QtGlobal>

#include "Utilities.h"
#include "RasterizationStyleContext.h"

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
    RasterizationStyleContext& styleContext,
    IQueryController* controller /*= nullptr*/)
{
    SkPaint paint;
    paint.setAntiAlias(true);

    QList< Primitive > polygons, lines, points;
    obtainPrimitives(objects, zoom, polygons, lines, points);
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

void OsmAnd::Rasterizer::obtainPrimitives( const QList< std::shared_ptr<OsmAnd::Model::MapObject> >& objects, uint32_t zoom, QList< Primitive >& polygons, QList< Primitive >& lines, QList< Primitive >& points )
{
    auto mult = 1.0 / Utilities::getPowZoom(qMax(31 - (zoom + 8), 0U));
    uint32_t idx = 0;
    for(auto itMapObject = objects.begin(); itMapObject != objects.end(); ++itMapObject, idx++)
    {
        auto mapObject = *itMapObject;
        auto sh = idx << 8;

        for(auto itType = mapObject->_types.begin(); itType != mapObject->_types.end(); ++itType)
        {
            auto type = *itType;// pair
            /*TODO:
            int layer = mobj->getSimpleLayer();
            req->setTagValueZoomLayer(pair.first, pair.second, rc->getZoom(), layer, mobj);
            req->setIntFilter(req->props()->R_AREA, mobj->area);
            req->setIntFilter(req->props()->R_POINT, mobj->points.size() == 1);
            req->setIntFilter(req->props()->R_CYCLE, mobj->cycle());
            if (req->searchRule(RenderingRulesStorage::ORDER_RULES)) {
                int objectType = req->getIntPropertyValue(req->props()->R_OBJECT_TYPE);
                int order = req->getIntPropertyValue(req->props()->R_ORDER);
                MapDataObjectPrimitive mapObj;
                mapObj.objectType = objectType;
                mapObj.order = order;
                mapObj.typeInd = j;
                mapObj.obj = mobj;
                // polygon
                if(objectType == 3) {
                    MapDataObjectPrimitive pointObj = mapObj;
                    pointObj.objectType = 1;
                    mapObj.order = polygonArea(mobj, mult);
                    if(mapObj.order > MAX_V) { 
                        polygonsArray.push_back(mapObj);
                        pointsArray.push_back(pointObj);
                    }
                } else if(objectType == 1) {
                    pointsArray.push_back(mapObj);
                } else {
                    linesArray.push_back(mapObj);
                }
                if (req->getIntPropertyValue(req->props()->R_SHADOW_LEVEL) > 0) {
                    rc->shadowLevelMin = std::min(rc->shadowLevelMin, order);
                    rc->shadowLevelMax = std::max(rc->shadowLevelMax, order);
                    req->clearIntvalue(req->props()->R_SHADOW_LEVEL);
                }
            }*/
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

