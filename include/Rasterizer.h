/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __RASTERIZER_H_
#define __RASTERIZER_H_

#include <stdint.h>
#include <memory>

#include <SkCanvas.h>
#include <SkPaint.h>
#include <QList>

#include <OsmAndCore.h>
#include <MapObject.h>
#include <IQueryController.h>
#include <RasterizationStyleEvaluator.h>

namespace OsmAnd {

    class RasterizerContext;

    class OSMAND_CORE_API Rasterizer
    {
    private:
        Rasterizer();

        enum {
            PolygonAreaCutoffLowerThreshold = 75,
            ZoomOnlyForBasemaps = 7,
            BasemapZoom = 11,
            DetailedLandDataZoom = 14,
        };
    protected:
        enum PrimitiveType : uint32_t
        {
            Point = 1,
            Line = 2,
            Polygon = 3,
        };

        struct TextPrimitive
        {
            QString content;
            bool drawOnPath;
            std::shared_ptr<SkPath> path;
            PointF center;
            int vOffset;
            int color;
            int size;
            int shadowRadius;
            int wrapWidth;
            bool isBold;
            int minDistance;
            QString shieldResource;
            int order;
        };

        struct Primitive
        {
            std::shared_ptr<OsmAnd::Model::MapObject> mapObject;
            double zOrder;
            uint32_t typeIndex;
            PrimitiveType objectType;
        };

        static void obtainPrimitives(RasterizerContext& context, IQueryController* controller);
        static void filterOutLinesByDensity(RasterizerContext& context, const QVector< Primitive >& in, QVector< Primitive >& out, IQueryController* controller);
        static bool polygonizeCoastlines(
            RasterizerContext& context,
            const QList< std::shared_ptr<OsmAnd::Model::MapObject> >& coastlines,
            QList< std::shared_ptr<OsmAnd::Model::MapObject> >& outVectorized,
            bool abortIfBrokenCoastlinesExist,
            bool includeBrokenCoastlines
            );
        static bool buildCoastlinePolygonSegment(
            RasterizerContext& context,
            bool currentInside,
            const PointI& currentPoint31,
            bool prevInside,
            const PointI& previousPoint31,
            QVector< PointI >& segmentPoints );
        static bool calculateIntersection( const PointI& p1, const PointI& p0, const AreaI& bbox, PointI& pX );
        static void appendCoastlinePolygons( QList< QVector< PointI > >& closedPolygons, QList< QVector< PointI > >& brokenPolygons, QVector< PointI >& polyline );
        static void mergeBrokenPolygons( RasterizerContext& context, QList< QVector< PointI > >& brokenPolygons, QList< QVector< PointI > >& closedPolygons, uint64_t osmId );
        static bool isClockwiseCoastlinePolygon( const QVector< PointI > & polygon );

        enum PrimitivesType
        {
            Polygons,
            Lines,
            ShadowOnlyLines,
            Points,
        };
        static void rasterizeMapPrimitives(RasterizerContext& context, SkCanvas& canvas, const QVector< Primitive >& primitives, PrimitivesType type, IQueryController* controller);
        enum PaintValuesSet : int
        {
            Set_0 = 0,
            Set_1 = 1,
            Set_minus1 = 2,
            Set_minus2 = 3,
            Set_3 = 4,
        };
        static bool updatePaint(RasterizerContext& context, const RasterizationStyleEvaluator& evaluator, PaintValuesSet valueSetSelector, bool isArea);
        static void rasterizePolygon(RasterizerContext& context, SkCanvas& canvas, const Primitive& primitive);
        static void rasterizeLine(RasterizerContext& context, SkCanvas& canvas, const Primitive& primitive, bool drawOnlyShadow);
        static void rasterizeLineShadow(RasterizerContext& context, SkCanvas& canvas, const SkPath& path, uint32_t shadowColor, int shadowRadius);
        static void rasterizeLine_OneWay( RasterizerContext& context, SkCanvas& canvas, const SkPath& path, int oneway );

        static void obtainPrimitivesTexts(RasterizerContext& context, IQueryController* controller);
        static void collectPrimitivesTexts( RasterizerContext& context, const QVector< Rasterizer::Primitive >& primitives, PrimitivesType type, IQueryController* controller );
        static void collectPolygonText(RasterizerContext& context, const Primitive& primitive);
        static void collectLineText(RasterizerContext& context, const Primitive& primitive);
        static void collectPointText(RasterizerContext& context, const Primitive& primitive);
        static void preparePrimitiveText(RasterizerContext& context, const Primitive& primitive, const PointF& point, SkPath* path);

        static void calculateVertex(RasterizerContext& context, const PointI& point, PointF& vertex);
        static bool contains(const QVector< PointF >& vertices, const PointF& other);
    public:
        virtual ~Rasterizer();

        static void update(
            RasterizerContext& context,
            const AreaI& area31,
            uint32_t zoom,
            uint32_t tileSidePixelLength,
            const QList< std::shared_ptr<OsmAnd::Model::MapObject> >* objects = nullptr,
            const PointF& tlOriginOffset = PointF(),
            bool* nothingToRender = nullptr,
            IQueryController* controller = nullptr
            );
        static bool rasterizeMap(
            RasterizerContext& context,
            bool fillBackground,
            SkCanvas& canvas,
            IQueryController* controller = nullptr);
        static void rasterizeText(
            RasterizerContext& context,
            bool fillBackground,
            SkCanvas& canvas,
            IQueryController* controller = nullptr);

        friend class OsmAnd::RasterizerContext;
    };

} // namespace OsmAnd

#endif // __RASTERIZER_H_
