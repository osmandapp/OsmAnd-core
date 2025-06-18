#ifndef _OSMAND_CORE_MAP_PRIMITIVISER_P_H_
#define _OSMAND_CORE_MAP_PRIMITIVISER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IQueryController.h"
#include "MapCommonTypes.h"
#include "MapPresentationEnvironment.h"
#include "MapPrimitiviser.h"
#include "commonOsmAndCore.h"

namespace OsmAnd
{
    class MapPrimitiviser;
    class MapPrimitiviser_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapPrimitiviser_P);

    public:
        typedef MapPrimitiviser::CoastlineMapObject CoastlineMapObject;
        typedef MapPrimitiviser::SurfaceMapObject SurfaceMapObject;
        typedef MapPrimitiviser::PrimitiveType PrimitiveType;
        typedef MapPrimitiviser::Primitive Primitive;
        typedef MapPrimitiviser::PrimitivesCollection PrimitivesCollection;
        typedef MapPrimitiviser::PrimitivesGroup PrimitivesGroup;
        typedef MapPrimitiviser::PrimitivesGroupsCollection PrimitivesGroupsCollection;
        typedef MapPrimitiviser::Symbol Symbol;
        typedef MapPrimitiviser::SymbolsCollection SymbolsCollection;
        typedef MapPrimitiviser::SymbolsGroup SymbolsGroup;
        typedef MapPrimitiviser::GridSymbolsGroup GridSymbolsGroup;
        typedef MapPrimitiviser::SymbolsGroupsCollection SymbolsGroupsCollection;
        typedef MapPrimitiviser::TextSymbol TextSymbol;
        typedef MapPrimitiviser::IconSymbol IconSymbol;
        typedef MapPrimitiviser::PrimitivisedObjects PrimitivisedObjects;
        typedef MapPrimitiviser::Cache Cache;

    private:
        void debugCoastline(const AreaI & area31, const QList< std::shared_ptr<const MapObject>> & coastlines) const;
        const AreaI getWidenArea(const AreaI & area31, const ZoomLevel & zoom) const;
    protected:
        MapPrimitiviser_P(MapPrimitiviser* const owner);

        enum class PrimitivesType
        {
            Polygons,
            Polylines,
            Polylines_ShadowOnly,
            Points,
        };

        struct Context Q_DECL_FINAL
        {
            Context(
                const std::shared_ptr<const MapPresentationEnvironment>& env,
                const ZoomLevel zoom);

            const std::shared_ptr<const MapPresentationEnvironment> env;
            const ZoomLevel zoom;

            double polygonAreaMinimalThreshold;
            unsigned int roadDensityZoomTile;
            unsigned int roadsDensityLimitPerTile;
            float defaultSymbolPathSpacing;
            float defaultBlockPathSpacing;

        private:
            Q_DISABLE_COPY_AND_MOVE(Context);
        };

        static bool polygonizeCoastlines(
            const AreaI area31,
            const ZoomLevel zoom,
            const QList< std::shared_ptr<const MapObject> >& coastlines,
            QList< std::shared_ptr<const MapObject> >& outVectorized);
        
        static MapDataObject convertToLegacy(const MapObject & coreObj);
        static const std::shared_ptr<MapObject> convertFromLegacy(const MapDataObject * legacyObj);

        static void obtainPrimitives(
            const Context& context,
            const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
            const QList< std::shared_ptr<const OsmAnd::MapObject> >& source,
            MapStyleEvaluationResult& evaluationResult,
            const std::shared_ptr<Cache>& cache,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static std::shared_ptr<const PrimitivesGroup> obtainPrimitivesGroup(
            const Context& context,
            const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
            const std::shared_ptr<const MapObject>& mapObject,
            MapStyleEvaluationResult& evaluationResult,
            MapStyleEvaluator& orderEvaluator,
            MapStyleEvaluator& polygonEvaluator,
            MapStyleEvaluator& polylineEvaluator,
            MapStyleEvaluator& pointEvaluator,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void sortAndFilterPrimitives(
            const Context& context,
            const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void filterOutHighwaysByDensity(
            const Context& context,
            const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void obtainPrimitivesSymbols(
            const Context& context,
            const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
            const TileId tileId,
            MapStyleEvaluationResult& evaluationResult,
            const std::shared_ptr<Cache>& cache,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void collectSymbolsFromPrimitives(
            const Context& context,
            const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
            const PrimitivesCollection& primitives,
            MapStyleEvaluationResult& evaluationResult,
            MapStyleEvaluator& textEvaluator,
            QHash<QString, int>& textOrderCache,
            SymbolsCollection& outSymbols,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void obtainSymbolsFromPolygon(
            const Context& context,
            const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
            const std::shared_ptr<const Primitive>& primitive,
            MapStyleEvaluationResult& evaluationResult,
            MapStyleEvaluator& textEvaluator,
            QHash<QString, int>& textOrderCache,
            SymbolsCollection& outSymbols,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void obtainSymbolsFromPolyline(
            const Context& context,
            const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
            const std::shared_ptr<const Primitive>& primitive,
            MapStyleEvaluationResult& evaluationResult,
            MapStyleEvaluator& textEvaluator,
            QHash<QString, int>& textOrderCache,
            SymbolsCollection& outSymbols,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void obtainSymbolsFromPoint(
            const Context& context,
            const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
            const std::shared_ptr<const Primitive>& primitive,
            MapStyleEvaluationResult& evaluationResult,
            MapStyleEvaluator& textEvaluator,
            QHash<QString, int>& textOrderCache,
            SymbolsCollection& outSymbols,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void obtainPrimitiveTexts(
            const Context& context,
            const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
            const std::shared_ptr<const Primitive>& primitive,
            const PointI& location,
            MapStyleEvaluationResult& evaluationResult,
            MapStyleEvaluator& textEvaluator,
            QHash<QString, int>& textOrderCache,
            SymbolsCollection& outSymbols,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void obtainPrimitiveIcon(
            const Context& context,
            const std::shared_ptr<const Primitive>& primitive,
            const PointI& location,
            MapStyleEvaluationResult& evaluationResult,
            SymbolsCollection& outSymbols,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);
        
        static QString prepareIconValue(
            const std::shared_ptr<const MapObject>& object,
            const QString& genTagVal);
        
        static OsmAnd::MapSurfaceType determineSurfaceType(AreaI area31, QList< std::shared_ptr<const MapObject> >& coastlines);

    public:
        ~MapPrimitiviser_P();

        ImplementationInterface<MapPrimitiviser> owner;

        std::shared_ptr<PrimitivisedObjects> primitiviseAllMapObjects(
            const ZoomLevel zoom,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric);

        std::shared_ptr<PrimitivisedObjects> primitiviseAllMapObjects(
            const ZoomLevel zoom,
            const TileId tileId,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric);

        std::shared_ptr<PrimitivisedObjects> primitiviseAllMapObjects(
            const PointD scaleDivisor31ToPixel,
            const ZoomLevel zoom,
            const TileId tileId,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric);

        std::shared_ptr<PrimitivisedObjects> primitiviseWithSurface(
            const AreaI area31,
            const PointI areaSizeInPixels,
            const ZoomLevel zoom,
            const TileId tileId,
            const AreaI visibleArea31,
            const int64_t visibleAreaTime,
            const MapSurfaceType surfaceType,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitiviseWithSurface* const metric);

        std::shared_ptr<PrimitivisedObjects> primitiviseWithoutSurface(
            const PointD scaleDivisor31ToPixel,
            const ZoomLevel zoom,
            const TileId tileId,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitiviseWithoutSurface* const metric);

    friend class OsmAnd::MapPrimitiviser;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRIMITIVISER_P_H_)
