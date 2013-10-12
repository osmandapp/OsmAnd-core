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

#ifndef __RASTERIZER_P_H_
#define __RASTERIZER_P_H_

#include <cstdint>
#include <memory>

#include <QList>

#include <SkCanvas.h>
#include <SkPaint.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

namespace OsmAnd {

    class MapStyleEvaluator;
    class RasterizerEnvironment_P;
    class RasterizerContext_P;
    namespace Model {
        class MapObject;
    } // namespace Model
    class IQueryController;

    class Rasterizer;
    class Rasterizer_P
    {
    private:
        Rasterizer_P();
        ~Rasterizer_P();
    protected:
        enum {
            PolygonAreaCutoffLowerThreshold = 75,
            BasemapZoom = 11,
            DetailedLandDataZoom = 14,
        };

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
            std::shared_ptr<const Model::MapObject> mapObject;
            double zOrder;
            uint32_t typeIndex;
            PrimitiveType objectType;
        };

        static void obtainPrimitives(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            IQueryController* controller);
        static void filterOutLinesByDensity(
            const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
            const QVector< Primitive >& in, QVector< Primitive >& out, IQueryController* controller);
        static bool polygonizeCoastlines(
            const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
            const QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& coastlines,
            QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& outVectorized,
            bool abortIfBrokenCoastlinesExist,
            bool includeBrokenCoastlines
            );
        static bool buildCoastlinePolygonSegment(
            const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
            bool currentInside,
            const PointI& currentPoint31,
            bool prevInside,
            const PointI& previousPoint31,
            QVector< PointI >& segmentPoints );
        static bool calculateIntersection( const PointI& p1, const PointI& p0, const AreaI& bbox, PointI& pX );
        static void appendCoastlinePolygons( QList< QVector< PointI > >& closedPolygons, QList< QVector< PointI > >& coastlinePolylines, QVector< PointI >& polyline );
        static void convertCoastlinePolylinesToPolygons(
            const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
            QList< QVector< PointI > >& coastlinePolylines, QList< QVector< PointI > >& coastlinePolygons, uint64_t osmId );
        static bool isClockwiseCoastlinePolygon( const QVector< PointI > & polygon );

        enum PrimitivesType
        {
            Polygons,
            Lines,
            ShadowOnlyLines,
            Points,
        };
        static void rasterizeMapPrimitives(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            SkCanvas& canvas, const QVector< Primitive >& primitives, PrimitivesType type, IQueryController* controller);
        enum PaintValuesSet : int
        {
            Set_0 = 0,
            Set_1 = 1,
            Set_minus1 = 2,
            Set_minus2 = 3,
            Set_3 = 4,
        };
        static bool updatePaint(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const MapStyleEvaluator& evaluator, PaintValuesSet valueSetSelector, bool isArea);
        static void rasterizePolygon(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            SkCanvas& canvas, const Primitive& primitive);
        static void rasterizeLine(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            SkCanvas& canvas, const Primitive& primitive, bool drawOnlyShadow);
        static void rasterizeLineShadow(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            SkCanvas& canvas, const SkPath& path, uint32_t shadowColor, int shadowRadius);
        static void rasterizeLine_OneWay(
            const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
            SkCanvas& canvas, const SkPath& path, int oneway );
        static SkPathEffect* obtainPathEffect(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const QString& pathEffect);

        static void obtainPrimitivesTexts(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            IQueryController* controller);
        static void collectPrimitivesTexts(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const QVector< Primitive >& primitives, PrimitivesType type, IQueryController* controller );
        static void collectPolygonText(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const Primitive& primitive);
        static void collectLineText(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const Primitive& primitive);
        static void collectPointText(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const Primitive& primitive);
        static void preparePrimitiveText(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const Primitive& primitive, const PointF& point, SkPath* path);

        static void calculateVertex(
            const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
            const PointI& point, PointF& vertex);
        static bool contains(const QVector< PointF >& vertices, const PointF& other);

        static void adjustContextFromEnvironment(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const ZoomLevel zoom);

        static void prepareContext(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const AreaI& area31,
            const ZoomLevel zoom,
            const uint32_t tileSize,
            const MapFoundationType& foundation,
            const QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& objects,
            const PointF& tlOriginOffset,
            bool* nothingToRasterize,
            IQueryController* controller);
        static bool rasterizeMap(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            bool fillBackground,
            SkCanvas& canvas,
            IQueryController* controller);
        /*static void rasterizeText(
            RasterizerEnvironment& context,
            bool fillBackground,
            SkCanvas& canvas,
            IQueryController* controller = nullptr);*/

    friend class OsmAnd::Rasterizer;
    friend class OsmAnd::RasterizerContext_P;
    };

} // namespace OsmAnd

#endif // __RASTERIZER_P_H_
