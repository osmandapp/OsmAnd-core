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

#ifndef _OSMAND_CORE_RASTERIZER_P_H_
#define _OSMAND_CORE_RASTERIZER_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVector>

#include <SkCanvas.h>
#include <SkPaint.h>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <MapTypes.h>
#include <MapStyleEvaluationResult.h>

namespace OsmAnd {

    class MapStyleEvaluator;
    class MapStyleEvaluator_P;
    class RasterizerEnvironment_P;
    class RasterizerContext_P;
    class RasterizedSymbol;
    namespace Model {
        class MapObject;
    } // namespace Model
    class IQueryController;
    namespace Rasterizer_Metrics {
        struct Metric_prepareContext;
    } // namespace Rasterizer_Metrics

    class Rasterizer;
    class Rasterizer_P
    {
    private:
        Rasterizer* const owner;

        const RasterizerEnvironment_P& env;
        const RasterizerContext_P& context;

        SkPaint _mapPaint;
        AreaI _destinationArea;
        PointD _31toPixelDivisor;

        static void adjustContextFromEnvironment(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const ZoomLevel zoom);

        static bool polygonizeCoastlines(
            const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
            const QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& coastlines,
            QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& outVectorized,
            bool abortIfBrokenCoastlinesExist,
            bool includeBrokenCoastlines);
        static bool buildCoastlinePolygonSegment(
            const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
            bool currentInside,
            const PointI& currentPoint31,
            bool prevInside,
            const PointI& previousPoint31,
            QVector< PointI >& segmentPoints);
        static bool calculateIntersection( const PointI& p1, const PointI& p0, const AreaI& bbox, PointI& pX );
        static void appendCoastlinePolygons( QList< QVector< PointI > >& closedPolygons, QList< QVector< PointI > >& coastlinePolylines, QVector< PointI >& polyline );
        static void convertCoastlinePolylinesToPolygons(
            const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
            QList< QVector< PointI > >& coastlinePolylines, QList< QVector< PointI > >& coastlinePolygons, uint64_t osmId);
        static bool isClockwiseCoastlinePolygon( const QVector< PointI > & polygon);

        enum {
            PolygonAreaCutoffLowerThreshold = 75,
            BasemapZoom = 11,
            DetailedLandDataZoom = 14,
        };

        enum PrimitiveType : uint32_t
        {
            Point = 1,
            Polyline = 2,
            Polygon = 3,
        };

        enum PrimitivesType
        {
            Polygons,
            Polylines,
            Polylines_ShadowOnly,
            Points,
        };

        struct PrimitiveSymbol
        {
            PrimitiveSymbol();
            virtual ~PrimitiveSymbol()
            {}

            std::shared_ptr<const Model::MapObject> mapObject;
            PointI location31;

            int order;
        };

        struct PrimitiveSymbol_Text : public PrimitiveSymbol
        {
            QString value;
            bool drawOnPath;
            int verticalOffset;
            int color;
            int size;
            int shadowRadius;
            int wrapWidth;
            bool isBold;
            int minDistance;
            QString shieldResourceName;
        };

        struct PrimitiveSymbol_Icon : public PrimitiveSymbol
        {
            QString resourceName;
        };

        struct Primitive
        {
            std::shared_ptr<const Model::MapObject> mapObject;
            double zOrder;
            uint32_t typeRuleIdIndex;
            PrimitiveType objectType;
            std::shared_ptr<MapStyleEvaluationResult> evaluationResult;
        };

        static void obtainPrimitives(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const IQueryController* const controller,
            Rasterizer_Metrics::Metric_prepareContext* const metric);
        static void filterOutLinesByDensity(
            const RasterizerEnvironment_P& env, const RasterizerContext_P& context,
            const QVector< Primitive >& in, QVector< Primitive >& out, const IQueryController* const controller);

        static void obtainPrimitivesSymbols(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const IQueryController* const controller);
        static void collectPrimitivesSymbols(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const QVector< Primitive >& primitives, const PrimitivesType type, const IQueryController* const controller );
        static void obtainPolygonSymbol(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const Primitive& primitive);
        static void obtainPolylineSymbol(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const Primitive& primitive);
        static void obtainPointSymbol(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const Primitive& primitive);
        static void obtainPrimitiveTexts(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const Primitive& primitive, const PointI& location);
        static void obtainPrimitiveIcon(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const Primitive& primitive, const PointI& location);

        STRONG_ENUM(PaintValuesSet)
        {
            Set_0,
            Set_1,
            Set_minus1,
            Set_minus2,
            Set_3,
        };
        bool updatePaint(
            const MapStyleEvaluationResult& evalResult, const PaintValuesSet valueSetSelector, const bool isArea);

        void rasterizeMapPrimitives(
            const AreaI* const destinationArea,
            SkCanvas& canvas, const QVector< Primitive >& primitives, const PrimitivesType type, const IQueryController* const controller);

        void rasterizePolygon(
            const AreaI* const destinationArea,
            SkCanvas& canvas, const Primitive& primitive);
        void rasterizePolyline(
            const AreaI* const destinationArea,
            SkCanvas& canvas, const Primitive& primitive, bool drawOnlyShadow);
        void rasterizeLineShadow(
            SkCanvas& canvas, const SkPath& path, uint32_t shadowColor, int shadowRadius);
        void rasterizeLine_OneWay(
            SkCanvas& canvas, const SkPath& path, int oneway );

        inline void calculateVertex(const PointI& point31, PointF& vertex);
        static bool contains(const QVector< PointF >& vertices, const PointF& other);
    protected:
        Rasterizer_P(Rasterizer* const owner, const RasterizerEnvironment_P& env, const RasterizerContext_P& context);
    public:
        ~Rasterizer_P();

        static void prepareContext(
            const RasterizerEnvironment_P& env, RasterizerContext_P& context,
            const AreaI& area31,
            const ZoomLevel zoom,
            const MapFoundationType foundation,
            const QList< std::shared_ptr<const Model::MapObject> >& objects,
            bool* nothingToRasterize,
            const IQueryController* const controller,
            Rasterizer_Metrics::Metric_prepareContext* const metric);

        void rasterizeMap(
            SkCanvas& canvas,
            const bool fillBackground,
            const AreaI* const destinationArea,
            const IQueryController* const controller);

        void rasterizeSymbolsWithoutPaths(
            QList< std::shared_ptr<const RasterizedSymbol> >& outSymbols,
            const IQueryController* const controller);

    friend class OsmAnd::Rasterizer;
    friend class OsmAnd::RasterizerContext_P;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_RASTERIZER_P_H_
