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
        typedef MapPrimitiviser::SymbolsGroupsCollection SymbolsGroupsCollection;
        typedef MapPrimitiviser::TextSymbol TextSymbol;
        typedef MapPrimitiviser::IconSymbol IconSymbol;
        typedef MapPrimitiviser::PrimitivisedObjects PrimitivisedObjects;
        typedef MapPrimitiviser::Cache Cache;

    private:
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

        static AreaI alignAreaForCoastlines(const AreaI& area31);

        static bool polygonizeCoastlines(
            const AreaI area31,
            const ZoomLevel zoom,
            const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
            const QList< std::shared_ptr<const MapObject> >& coastlines,
            QList< std::shared_ptr<const MapObject> >& outVectorized,
            bool abortIfBrokenCoastlinesExist,
            bool includeBrokenCoastlines);

        static bool buildCoastlinePolygonSegment(
            const AreaI area31,
            bool currentInside,
            const PointI& currentPoint31,
            bool prevInside,
            const PointI& previousPoint31,
            QVector< PointI >& segmentPoints);

        static bool calculateIntersection(
            const PointI& p1,
            const PointI& p0,
            const AreaI& bbox,
            PointI& pX);

        static void appendCoastlinePolygons
            (QList< QVector< PointI > >& closedPolygons,
            QList< QVector< PointI > >& coastlinePolylines,
            QVector< PointI >& polyline);

        static void convertCoastlinePolylinesToPolygons(
            const AreaI area31,
            QList< QVector< PointI > >& coastlinePolylines,
            QList< QVector< PointI > >& coastlinePolygons);

        static bool isClockwiseCoastlinePolygon(const QVector< PointI > & polygon);

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
            MapStyleEvaluationResult& evaluationResult,
            const std::shared_ptr<Cache>& cache,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void collectSymbolsFromPrimitives(
            const Context& context,
            const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
            const PrimitivesCollection& primitives,
            const PrimitivesType type,
            MapStyleEvaluationResult& evaluationResult,
            MapStyleEvaluator& textEvaluator,
            SymbolsCollection& outSymbols,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void obtainSymbolsFromPolygon(
            const Context& context,
            const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
            const std::shared_ptr<const Primitive>& primitive,
            MapStyleEvaluationResult& evaluationResult,
            MapStyleEvaluator& textEvaluator,
            SymbolsCollection& outSymbols,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void obtainSymbolsFromPolyline(
            const Context& context,
            const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
            const std::shared_ptr<const Primitive>& primitive,
            MapStyleEvaluationResult& evaluationResult,
            MapStyleEvaluator& textEvaluator,
            SymbolsCollection& outSymbols,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void obtainSymbolsFromPoint(
            const Context& context,
            const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
            const std::shared_ptr<const Primitive>& primitive,
            MapStyleEvaluationResult& evaluationResult,
            MapStyleEvaluator& textEvaluator,
            SymbolsCollection& outSymbols,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static void obtainPrimitiveTexts(
            const Context& context,
            const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
            const std::shared_ptr<const Primitive>& primitive,
            const PointI& location,
            MapStyleEvaluationResult& evaluationResult,
            MapStyleEvaluator& textEvaluator,
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
        
        static OsmAnd::MapSurfaceType determineSurfaceType(PointI center, QList< std::shared_ptr<const MapObject> > & coastlineObjects);

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
            const PointD scaleDivisor31ToPixel,
            const ZoomLevel zoom,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric);

        std::shared_ptr<PrimitivisedObjects> primitiviseWithSurface(
            const AreaI area31,
            const PointI areaSizeInPixels,
            const ZoomLevel zoom,
            const MapSurfaceType surfaceType,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitiviseWithSurface* const metric);

        std::shared_ptr<PrimitivisedObjects> primitiviseWithoutSurface(
            const PointD scaleDivisor31ToPixel,
            const ZoomLevel zoom,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitiviseWithoutSurface* const metric);

    friend class OsmAnd::MapPrimitiviser;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRIMITIVISER_P_H_)
