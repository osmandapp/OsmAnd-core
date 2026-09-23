#ifndef _OSMAND_CORE_MAP_RASTERIZER_P_H_
#define _OSMAND_CORE_MAP_RASTERIZER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QList>
#include <QVector>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkShader.h>
#include <SkPathEffect.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapCommonTypes.h"
#include "MapPrimitiviser.h"
#include "MapPresentationEnvironment.h"
#include "MapRasterizer_Metrics.h"

namespace OsmAnd
{
    class BinaryMapObject;
    class IQueryController;

    class MapRasterizer;
    class MapRasterizer_P /*Q_DECL_FINAL*/
    {
    private:
    protected:
        MapRasterizer_P(MapRasterizer* const owner);

        // Lane layout of a road in meters, shared by all primitives of a tile
        struct RoadLayout
        {
            bool valid = false;
            bool buttCaps = false;
            // Solid lines along the outer edges of the lanes (motorways and trunk roads)
            bool edgeLines = false;
            float width = 0.0f;
            float laneWidth = 0.0f;
            int lanesForward = 0;
            int lanesBackward = 0;
            // Explicit placement: constant shift of the carriageway centre to the right of the way line
            bool hasPlacement = false;
            float shift = 0.0f;
            // Resolved shift at the first and the last point of the way
            float shiftStart = 0.0f;
            float shiftEnd = 0.0f;
            // Lanes that narrow down to nothing at the first or the last point of the way,
            // counted from the left or the right edge in the way direction
            int taperFirstLeft = 0;
            int taperFirstRight = 0;
            int taperLastLeft = 0;
            int taperLastRight = 0;
            // Cut of the carriageway at the first and the last point (unit normal to the right of the way
            // direction), so that joined roads meet without gaps or overlaps
            bool hasFirstNormal = false;
            bool hasLastNormal = false;
            PointD firstNormal;
            PointD lastNormal;
            // The next road continues the same lanes, so the turn arrows are painted there
            bool noArrows = false;
            // Branch on the left in the driving direction at a split or a merge: the gore area between
            // the two branches is painted by this road
            const MapObject* goreFirst = nullptr;
            const MapObject* goreLast = nullptr;
            QStringList turnForward;
            QStringList turnBackward;
            QStringList changeForward;
            QStringList changeBackward;
            QStringList busForward;
            QStringList busBackward;
        };

        // Road geometry in pixels, in the way direction; offsets are to the right of the way line
        struct RoadGeometry
        {
            QVector<PointF> vertices;
            QVector<float> distances;
            // Lane boundaries from the left edge to the right edge, lanes + 1 lines
            QVector<QVector<float>> boundaries;
            // Width of each lane relatively to the full lane width, per vertex
            QVector<QVector<float>> laneFactors;
            QVector<float> bodyLeft;
            QVector<float> bodyRight;
            bool hasFirstNormal = false;
            bool hasLastNormal = false;
            PointF firstNormal;
            PointF lastNormal;
            float blendLength = 0.0f;
            bool tapered = false;
        };

        struct Context
        {
            Context(
                const AreaI area31,
                const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
                const AreaI pixelArea);

            const AreaI area31;
            const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects> primitivisedObjects;
            const std::shared_ptr<const MapPresentationEnvironment> env;
            const ZoomLevel zoom;
            const AreaI pixelArea;

            MapPresentationEnvironment::ShadowMode shadowMode;
            ColorARGB shadowColor;

            bool realisticRoads;
            float pixelsPerMeter;
            QHash<const MapObject*, RoadLayout> roadLayouts;
            // Realistic roads whose markings wait until all roads of the same level are drawn
            mutable QVector<std::shared_ptr<const MapPrimitiviser::Primitive>> pendingMarkings;
            mutable int pendingMarkingsLayer = 0;

        private:
            Q_DISABLE_COPY_AND_MOVE(Context);
        };

        enum class PrimitivesType
        {
            Polygons,
            Polylines,
            Polylines_ShadowOnly,
            Points,
        };

        enum class PaintValuesSet
        {
            Layer_minus2,
            Layer_minus1,
            Layer_0,
            Layer_1,
            Layer_2,
            Layer_3,
            Layer_4,
            Layer_5,
        };

        struct RealisticRoad
        {
            bool valid = false;
            const RoadLayout* layout = nullptr;
            float styleWidth = 0.0f;
            // Sizes in pixels
            float width = 0.0f;
            float laneWidth = 0.0f;
            bool buttCaps = false;
            // Draw the carriageway as a polygon: its width changes or its ends are cut to fit the neighbours
            bool polygonBody = false;
            RoadGeometry geometry;
        };

        static bool computeRoadLayout(
            const std::shared_ptr<const MapObject>& mapObject,
            const QString& highwayType,
            RoadLayout& outLayout);
        static void resolveRoadTransitions(Context& context);
        bool updatePaint(
            const Context& context,
            SkPaint& paint,
            const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive,
            const PaintValuesSet valueSetSelector,
            const bool isArea,
            const RealisticRoad* const road = nullptr);

        void computeRealisticRoad(
            const Context& context,
            const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive,
            RealisticRoad& outRoad) const;

        bool computeRoadGeometry(
            const Context& context,
            const MapObject& mapObject,
            const RoadLayout& layout,
            RoadGeometry& outGeometry) const;
        static QVector<PointF> offsetPoints(
            const RoadGeometry& geometry,
            const QVector<float>& offsets);
        static SkPath buildOffsetPath(const RoadGeometry& geometry, const QVector<float>& offsets);
        void drawRoadBody(
            SkCanvas& canvas,
            SkPaint& paint,
            const SkPath& path,
            const RealisticRoad& road) const;
        void flushLaneMarkings(const Context& context, SkCanvas& canvas);
        void rasterizeGore(
            const Context& context,
            SkCanvas& canvas,
            const MapObject& mapObject,
            const RealisticRoad& road,
            const bool atFirst,
            const SkColor color);
        void rasterizeLaneMarkings(
            const Context& context,
            SkCanvas& canvas,
            const RealisticRoad& road);
        void getPixelVertices(const Context& context, const QVector<PointI>& points31, QVector<PointF>& outVertices) const;
        static QVector<float> interpolateShifts(
            const QVector<float>& distances,
            const float start,
            const float end,
            const float taperLength);
        static void drawLaneArrows(
            SkCanvas& canvas,
            const QVector<PointF>& vertices,
            const QVector<float>& offsets,
            const QString& turn,
            const float endGap,
            const float pixelsPerMeter);

        void rasterizeMapPrimitives(
            const Context& context,
            SkCanvas& canvas,
            const MapPrimitiviser::PrimitivesCollection& primitives,
            const PrimitivesType type,
            const std::shared_ptr<const IQueryController>& queryController);

        void rasterizePolygon(
            const Context& context,
            SkCanvas& canvas,
            const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive);

        void rasterizePolyline(
            const Context& context,
            SkCanvas& canvas,
            const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive,
            bool drawOnlyShadow);

        void rasterizePolylineShadow(
            const Context& context,
            SkCanvas& canvas,
            const SkPath& path,
            SkPaint& paint,
            const ColorARGB shadowColor,
            const float shadowRadius);

        void rasterizePolylineIcons(
            const Context& context,
            SkCanvas& canvas,
            const SkPath& path,
            const MapStyleEvaluationResult::Packed& evalResult);

        void drawLineLayer(
            SkCanvas& canvas,
            SkPaint& paint,
            SkPath& path,
            const Context& context,
            const AreaI& area31,
            const QVector<PointI>& points31,
            const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive,
            const PaintValuesSet valueSetSelector,
            const IMapStyle::ValueDefinitionId hMarginId,
            const RealisticRoad* const road);
        bool calculateLinePath(
            const Context& context,
            const QVector<PointI>& points31,
            const AreaI& area31,
            SkPath& outPath,
            float offset) const;
        inline void calculateVertex(const Context& context, const PointI& point31, PointF& vertex) const;
        inline float lineEquation(float x1, float y1, float x2, float y2, float x) const;
        inline void simplifyVertexToDirection(const Context& , const PointF& , const PointF& , PointF&) const;

        void initialize();

        SkPaint _defaultPaint;

        mutable QMutex _pathEffectsMutex;
        mutable QHash< QString, sk_sp<SkPathEffect> > _pathEffects;
        bool obtainPathEffect(const QString& encodedPathEffect, sk_sp<SkPathEffect> &outPathEffect) const;
        bool obtainImageShader(
            const std::shared_ptr<const MapPresentationEnvironment>& env,
            const QString& name, sk_sp<SkShader> &outShader);
    public:
        ~MapRasterizer_P();

        ImplementationInterface<MapRasterizer> owner;

        void rasterize(
            const AreaI area31,
            const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
            SkCanvas& canvas,
            const bool fillBackground,
            const AreaI* const destinationArea,
            MapRasterizer_Metrics::Metric_rasterize* const metric,
            const std::shared_ptr<const IQueryController>& queryController);

    friend class OsmAnd::MapRasterizer;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RASTERIZER_P_H_)
