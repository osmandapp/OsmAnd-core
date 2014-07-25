#ifndef _OSMAND_CORE_RASTERIZER_P_H_
#define _OSMAND_CORE_RASTERIZER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QList>
#include <QVector>

#include <SkCanvas.h>
#include <SkPaint.h>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapTypes.h"
#include "Primitiviser.h"

namespace OsmAnd
{
    class RasterizedSymbolsGroup;
    class RasterizedSymbol;
    namespace Model
    {
        class BinaryMapObject;
    }
    class IQueryController;

    class Rasterizer;
    class Rasterizer_P Q_DECL_FINAL
    {
    private:
    protected:
        Rasterizer_P(Rasterizer* const owner);

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
        };

        bool updatePaint(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            SkPaint& paint,
            const MapStyleEvaluationResult& evalResult,
            const PaintValuesSet valueSetSelector,
            const bool isArea);

        void rasterizeMapPrimitives(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            SkCanvas& canvas,
            const Primitiviser::PrimitivesCollection& primitives,
            const PrimitivesType type,
            const IQueryController* const controller);

        void rasterizePolygon(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            SkCanvas& canvas,
            const std::shared_ptr<const Primitiviser::Primitive>& primitive);

        void rasterizePolyline(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            SkCanvas& canvas,
            const std::shared_ptr<const Primitiviser::Primitive>& primitive,
            bool drawOnlyShadow);

        void rasterizeLineShadow(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            SkCanvas& canvas,
            const SkPath& path,
            SkPaint& paint,
            const ColorARGB shadowColor,
            int shadowRadius);

        void rasterizeLine_OneWay(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            SkCanvas& canvas,
            const SkPath& path,
            int oneway);

        inline void calculateVertex(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            const PointI& point31,
            PointF& vertex);

        static bool containsHelper(
            const QVector< PointI >& points,
            const PointI& otherPoint);
    public:
        ~Rasterizer_P();

        ImplementationInterface<Rasterizer> owner;

        void rasterizeMap(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            SkCanvas& canvas,
            const bool fillBackground,
            const AreaI* const destinationArea,
            const IQueryController* const controller);

        void rasterizeSymbolsWithoutPaths(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
            std::function<bool (const std::shared_ptr<const Model::BinaryMapObject>& mapObject)> filter,
            const IQueryController* const controller);

    friend class OsmAnd::Rasterizer;
    };
}

#endif // !defined(_OSMAND_CORE_RASTERIZER_P_H_)
