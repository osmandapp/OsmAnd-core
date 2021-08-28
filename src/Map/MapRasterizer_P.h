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
#include <SkBitmapProcShader.h>
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

        bool updatePaint(
            const Context& context,
            SkPaint& paint,
            const MapStyleEvaluationResult::Packed& evalResult,
            const PaintValuesSet valueSetSelector,
            const bool isArea);

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

        inline void calculateVertex(const Context& context, const PointI& point31, PointF& vertex);
        inline float lineEquation(float x1, float y1, float x2, float y2, float x);
        inline void simplifyVertexToDirection(const Context& , const PointF& , const PointF& , PointF&);
        static bool containsHelper(const QVector< PointI >& points, const PointI& otherPoint);

        void initialize();
        
        SkPaint _defaultPaint;

        mutable QMutex _pathEffectsMutex;
        mutable QHash< QString, sk_sp<SkPathEffect> > _pathEffects;
        bool obtainPathEffect(const QString& encodedPathEffect, sk_sp<SkPathEffect> &outPathEffect) const;
        bool obtainShader(const std::shared_ptr<const MapPresentationEnvironment>& env, const QString& name, sk_sp<SkShader> &outShader);
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
