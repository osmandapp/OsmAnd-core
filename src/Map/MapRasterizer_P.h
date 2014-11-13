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
#include "Primitiviser.h"
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
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivisedArea,
            SkPaint& paint,
            const MapStyleEvaluationResult& evalResult,
            const PaintValuesSet valueSetSelector,
            const bool isArea);

        void rasterizeMapPrimitives(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivisedArea,
            SkCanvas& canvas,
            const Primitiviser::PrimitivesCollection& primitives,
            const PrimitivesType type,
            const IQueryController* const controller);

        void rasterizePolygon(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivisedArea,
            SkCanvas& canvas,
            const std::shared_ptr<const Primitiviser::Primitive>& primitive);

        void rasterizePolyline(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivisedArea,
            SkCanvas& canvas,
            const std::shared_ptr<const Primitiviser::Primitive>& primitive,
            bool drawOnlyShadow);

        void rasterizeLineShadow(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivisedArea,
            SkCanvas& canvas,
            const SkPath& path,
            SkPaint& paint,
            const ColorARGB shadowColor,
            int shadowRadius);

        void rasterizeLine_OneWay(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivisedArea,
            SkCanvas& canvas,
            const SkPath& path,
            int oneway);

        inline void calculateVertex(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivisedArea,
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
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivisedArea,
            SkCanvas& canvas,
            const bool fillBackground,
            const AreaI* const destinationArea,
            MapRasterizer_Metrics::Metric_rasterize* const metric,
            const IQueryController* const controller);

    friend class OsmAnd::MapRasterizer;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RASTERIZER_P_H_)
