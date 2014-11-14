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
    class MapRasterizer_P Q_DECL_FINAL
    {
    private:
    protected:
        MapRasterizer_P(MapRasterizer* const owner);

        struct Context
        {
            Context(
                const AreaI area31,
                const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects);

            const AreaI area31;
            const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects> primitivisedObjects;
            const std::shared_ptr<const MapPresentationEnvironment> env;
            const ZoomLevel zoom;

            MapPresentationEnvironment::ShadowMode shadowMode;
            ColorARGB shadowColor;
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
            Set_0,
            Set_1,
            Set_minus1,
            Set_minus2,
            Set_3,
            Set_4,
        };

        bool updatePaint(
            const Context& context,
            SkPaint& paint,
            const MapStyleEvaluationResult& evalResult,
            const PaintValuesSet valueSetSelector,
            const bool isArea);

        void rasterizeMapPrimitives(
            const Context& context,
            SkCanvas& canvas,
            const MapPrimitiviser::PrimitivesCollection& primitives,
            const PrimitivesType type,
            const IQueryController* const controller);

        void rasterizePolygon(
            const Context& context,
            SkCanvas& canvas,
            const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive);

        void rasterizePolyline(
            const Context& context,
            SkCanvas& canvas,
            const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive,
            bool drawOnlyShadow);

        void rasterizeLineShadow(
            const Context& context,
            SkCanvas& canvas,
            const SkPath& path,
            SkPaint& paint,
            const ColorARGB shadowColor,
            int shadowRadius);

        void rasterizeLine_OneWay(
            SkCanvas& canvas,
            const SkPath& path,
            int oneway);

        inline void calculateVertex(
            const Context& context,
            const PointI& point31,
            PointF& vertex);

        static bool containsHelper(
            const QVector< PointI >& points,
            const PointI& otherPoint);

        void initialize();
        
        SkPaint _defaultPaint;

        static void initializeOneWayPaint(SkPaint& paint);
        QList<SkPaint> _oneWayPaints;
        QList<SkPaint> _reverseOneWayPaints;

        mutable QMutex _pathEffectsMutex;
        mutable QHash< QString, SkPathEffect* > _pathEffects;
        bool obtainPathEffect(const QString& encodedPathEffect, SkPathEffect* &outPathEffect) const;
        bool obtainBitmapShader(const std::shared_ptr<const MapPresentationEnvironment>& env, const QString& name, SkBitmapProcShader* &outShader);
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
            const IQueryController* const controller);

    friend class OsmAnd::MapRasterizer;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RASTERIZER_P_H_)
