#ifndef _OSMAND_CORE_MAP_PRIMITIVISER_P_H_
#define _OSMAND_CORE_MAP_PRIMITIVISER_P_H_

#include "stdlib_common.h"
#include <deque>
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
        struct Coastline
        {
            QVector<PointI> points;
            int64_t winding;
            int proximity;
        };
        template<bool calculateClosestWinding>
        inline static void clipCoastlineForTile(int index, int size,
            const QList<std::shared_ptr<const MapObject>>& coastlines,
            PointI& center, const PointI& topLeft, const PointI& bottomRight, QVector<Coastline>* result,
            QVector<PointI>* finishPoints, const int64_t maxSqDistance, int64_t& minSqDistance, double& distance)
        {
            const std::deque<int>& sequence = {index};
            clipCoastlinesForTile<calculateClosestWinding>(
                sequence, size > 0 ? size : coastlines[index]->points31.size(), coastlines, center,
                topLeft, bottomRight, result, finishPoints, maxSqDistance, minSqDistance, distance);
        };
        template<bool calculateClosestWinding>
        inline static void clipCoastlinesForTile(const std::deque<int>& sequence, int size,
            const QList<std::shared_ptr<const MapObject>>& coastlines,
            PointI& center, const PointI& topLeft, const PointI& bottomRight, QVector<Coastline>* result,
            QVector<PointI>* finishPoints, const int64_t maxSqDistance, int64_t& minSqDistance, double& distance)
        {
            QVector<PointI> segment;
            segment.reserve(size);
            PointI prevPoint;
            PointI sm(INT32_MIN, INT32_MIN);
            int prevCode = 0;
            bool next = false;
            int64_t signedArea = 0;
            PointI filteredPoints[4];
            int filteredPointCount = 0;
            int headPointsToSkip = 0;
            int pointsToProcess = size - 1;
            const auto& firstCoastline = coastlines[sequence.front()]->points31;
            const auto& lastCoastline = coastlines[sequence.back()]->points31;
            const bool isCycle = firstCoastline.front() == lastCoastline.back();
            if (isCycle)
            {
                if (size < 4)
                    return;
                const auto lastSize = lastCoastline.size();
                if (lastSize > 2)
                    filteredPoints[0] = lastCoastline[lastSize - 3];
                else
                {
                    const auto& prevCoastline = coastlines[sequence[sequence.size() - 2]]->points31;
                    filteredPoints[0] = prevCoastline[prevCoastline.size() - 2];
                }
                filteredPoints[1] = lastCoastline[lastSize - 2];
                filteredPointCount = 2;
                headPointsToSkip = 2;
            }

            bool oneIsProcessed = false;
            bool skipFirst = false;
            bool hasInputPoint = false;
            PointI previousInputPoint;
            const auto count = sequence.size();
            bool isExtra = false;
            for (int idx = 0; idx <= count; idx++)
            {
                if (idx == count)
                {
                    isExtra = filteredPointCount > 1 && (!isCycle || oneIsProcessed || filteredPointCount > 2);
                    if (!isExtra)
                        break;
                    skipFirst = false;
                    hasInputPoint = false;
                }
                const auto& coastline = isExtra
                    ? QVector<PointI>(filteredPoints, filteredPoints + filteredPointCount)
                    : coastlines[sequence[idx]]->points31;
                for (const auto& point : coastline)
                {
                    if (Q_UNLIKELY(skipFirst))
                    {
                        skipFirst = false;
                        continue;
                    }
                    if (Q_LIKELY(hasInputPoint))
                    {
                        if (point == previousInputPoint)
                        {
                            pointsToProcess--;
                            continue;
                        }
                        bool shouldProcess = false;
                        PointI startPoint, endPoint;
                        if (isExtra)
                        {
                            startPoint = previousInputPoint;
                            endPoint = point;
                            shouldProcess = true;
                        }
                        else if (pointsToProcess-- > 0)
                        {
                            // Remove short self-intersecting loops before clipping or measuring winding.
                            const auto interPoint = filteredPointCount > 2 ? Utilities::getIntersectionPoint(
                                filteredPoints[filteredPointCount - 3], filteredPoints[filteredPointCount - 2],
                                filteredPoints[filteredPointCount - 1], point) : PointI(-1, -1);
                            if (interPoint.x >= 0)
                            {
                                const auto oldest = filteredPoints[filteredPointCount - 3];
                                if (isCycle && headPointsToSkip > 0)
                                {
                                    if (filteredPointCount > 3)
                                    {
                                        pointsToProcess--;
                                        headPointsToSkip--;
                                    }
                                    else
                                    {
                                        pointsToProcess -= headPointsToSkip;
                                        headPointsToSkip = 0;
                                    }
                                }
                                filteredPointCount -= 2;
                                if (point != oldest)
                                    filteredPoints[filteredPointCount++] = point;
                            }
                            else if (filteredPointCount > 3)
                            {
                                if (headPointsToSkip > 0)
                                    headPointsToSkip--;
                                else
                                {
                                    oneIsProcessed = true;
                                    startPoint = filteredPoints[0];
                                    endPoint = filteredPoints[1];
                                    shouldProcess = true;
                                }
                                filteredPoints[0] = filteredPoints[1];
                                filteredPoints[1] = filteredPoints[2];
                                filteredPoints[2] = filteredPoints[3];
                                filteredPoints[3] = point;
                            }
                            else
                                filteredPoints[filteredPointCount++] = point;
                        }
                        if (shouldProcess)
                        {
                            const auto nextCode = Utilities::computeOutCode(endPoint, topLeft, bottomRight);
                            auto p0 = startPoint;
                            auto p1 = endPoint;
                            int code0 = next && startPoint == prevPoint
                                ? prevCode : Utilities::computeOutCode(startPoint, topLeft, bottomRight);
                            int code1 = nextCode;
                            bool accept = false;
                            while (true)
                            {
                                if ((code0 | code1) == 0)
                                {
                                    accept = true;
                                    break;
                                }
                                else if ((code0 & code1) > 0)
                                    break;
                                else if (code0 > 0)
                                {
                                    p0 = Utilities::getIntersection(code0, p0, p1, topLeft, bottomRight);
                                    code0 = Utilities::computeOutCode(p0, topLeft, bottomRight);
                                }
                                else
                                {
                                    p1 = Utilities::getIntersection(code1, p0, p1, topLeft, bottomRight);
                                    code1 = Utilities::computeOutCode(p1, topLeft, bottomRight);
                                }
                            }
                            const bool isEmpty = segment.empty();
                            if (accept && p0 != p1)
                            {
                                if (isEmpty)
                                {
                                    sm.x = INT32_MIN;
                                    sm.y = INT32_MIN;
                                }
                                if (isEmpty || segment.back() != p0)
                                {
                                    if (!isEmpty)
                                    {
                                        const auto borderCode = Utilities::computeBorderCode(
                                            segment.front(), topLeft, bottomRight);
                                        const auto lastBorderCode = Utilities::computeBorderCode(
                                            segment.back(), topLeft, bottomRight);
                                        finishPoints[lastBorderCode].push_back(segment.back());
                                        result[borderCode].push_back({qMove(segment), signedArea, INT32_MAX});
                                        signedArea = 0;
                                        segment.reserve(size);
                                    }
                                    if (p0 != sm)
                                    {
                                        segment.push_back(p0);
                                        sm = p0;
                                    }
                                }
                                if (p1 != sm)
                                {
                                    signedArea += Utilities::intCrossProduct2D(p0, p1);
                                    segment.push_back(p1);
                                    sm = p1;
                                }
                            }
                            else if (!isEmpty)
                            {
                                if (segment.size() > 1)
                                {
                                    const auto borderCode = Utilities::computeBorderCode(
                                        segment.front(), topLeft, bottomRight);
                                    const auto lastBorderCode = Utilities::computeBorderCode(
                                        segment.back(), topLeft, bottomRight);
                                    finishPoints[lastBorderCode].push_back(segment.back());
                                    result[borderCode].push_back({qMove(segment), signedArea, INT32_MAX});
                                }
                                signedArea = 0;
                                segment.reserve(size);
                            }
                            prevPoint = endPoint;
                            prevCode = nextCode;
                            next = true;
                            if (calculateClosestWinding)
                            {
                                Utilities::findClosestWinding(center, startPoint, endPoint,
                                    maxSqDistance, minSqDistance, distance);
                            }
                        }
                    }
                    else
                    {
                        hasInputPoint = true;
                        if (!isExtra)
                            filteredPoints[filteredPointCount++] = point;
                    }
                    previousInputPoint = point;
                }
                skipFirst = true;
            }
            if (segment.size() > 1)
            {
                const auto borderCode = Utilities::computeBorderCode(segment.front(), topLeft, bottomRight);
                const auto lastBorderCode = Utilities::computeBorderCode(segment.back(), topLeft, bottomRight);
                finishPoints[lastBorderCode].push_back(segment.back());
                result[borderCode].push_back({qMove(segment), signedArea, INT32_MAX});
            }
        };
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

        static bool getCoastlines(
            const AreaI area31,
            const AreaI64 coastlineArea31,
            const QList< std::shared_ptr<const MapObject> >& coastlines,
            QList< std::shared_ptr<const MapObject> >& outVectorized,
            MapSurfaceType& surfaceType,
            bool* brokenCoastlineFault = nullptr);

        static bool polygonizeCoastlines(
            const AreaI area31,
            const ZoomLevel zoom,
            const QList< std::shared_ptr<const MapObject> >& coastlines,
            QList< std::shared_ptr<const MapObject> >& outVectorized);
        
        static MapDataObject convertToLegacy(const MapObject & coreObj);
        static const std::shared_ptr<MapObject> convertFromLegacy(const MapDataObject * legacyObj);

        static void obtainPrimitives(
            const Context& context,
            const ZoomLevel detailedZoom,
            const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
            const QList< std::shared_ptr<const OsmAnd::MapObject> >& source,
            MapStyleEvaluationResult& evaluationResult,
            const std::shared_ptr<Cache>& cache,
            const std::shared_ptr<const IQueryController>& queryController,
            MapPrimitiviser_Metrics::Metric_primitivise* const metric);

        static std::shared_ptr<const PrimitivesGroup> obtainPrimitivesGroup(
            const Context& context,
            const float detailScaleFactor,
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
            const TileId tileId,
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
            const TileId tileId,
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
            const ZoomLevel detailedZoom,
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
