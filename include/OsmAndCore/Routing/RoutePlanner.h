/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
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

#ifndef _OSMAND_CORE_ROUTE_PLANNER_H_
#define _OSMAND_CORE_ROUTE_PLANNER_H_

#include <limits>
#include <memory>
#include <queue>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/Routing/RoutePlannerContext.h>
#include <OsmAndCore/IQueryController.h>
//#define DEBUG_ROUTING 1
//#define TRACE_ROUTING 1
// very verbose method and needed only if there is a problem in queue store
//#define TRACE_DUMP_QUEUE 1
#define ROUTE_STATISTICS 1

#if !defined(DEBUG_ROUTING)
#   if defined(DEBUG) || defined(_DEBUG)
#       define DEBUG_ROUTING 1
#   else
#       define DEBUG_ROUTING 0
#   endif
#endif

#if !defined(TRACE_ROUTING)
#   if defined(DEBUG) || defined(_DEBUG)
#       define TRACE_ROUTING 1
#   else
#       define TRACE_ROUTING 0
#   endif
#endif

namespace OsmAnd {

    struct RouteCalculationResult {
        QList< std::shared_ptr<OsmAnd::RouteSegment> >  list;
        QString warnMessage;
        RouteCalculationResult(QString warn=""){
            warnMessage=warn;
        }
    };

    class OSMAND_CORE_API RoutePlanner
    {

    protected:
        RoutePlanner();

        typedef std::priority_queue<
            std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>,
            std::vector< std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >,
            std::function< bool(const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>&, const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>&) >
        >  RoadSegmentsPriorityQueue;

        static void loadRoads(RoutePlannerContext* context, uint32_t x31, uint32_t y31, uint32_t zoomAround, QList< std::shared_ptr<const Model::Road> >& roads);
        static void loadRoadsFromTile(RoutePlannerContext* context, uint64_t tileId, QList< std::shared_ptr<const Model::Road> >& roads);
        static uint64_t getRoutingTileId(RoutePlannerContext* context, uint32_t x31, uint32_t y31, bool dontLoad);
        static uint32_t getCurrentEstimatedSize(RoutePlannerContext* context);
        static void cacheRoad(RoutePlannerContext* context, const std::shared_ptr<Model::Road>& road);
        static void loadTileHeader(RoutePlannerContext* context, uint32_t x31, uint32_t y31, QList< std::shared_ptr<RoutePlannerContext::RoutingSubsectionContext> >& subsectionsContexts);
        static void loadSubregionContext(RoutePlannerContext::RoutingSubsectionContext* context);

        static bool findClosestRouteSegment(OsmAnd::RoutePlannerContext* context, double latitude, double longitude, std::shared_ptr<OsmAnd::RoutePlannerContext::RouteCalculationSegment>& routeSegment);

        static OsmAnd::RouteCalculationResult calculateRoute(
            OsmAnd::RoutePlannerContext::CalculationContext* context,
            const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& from,
            const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& to,
            bool leftSideNavigation,
            const IQueryController* const controller = nullptr);
        static void loadBorderPoints(OsmAnd::RoutePlannerContext::CalculationContext* context);
        static void updateDistanceForBorderPoints(OsmAnd::RoutePlannerContext::CalculationContext* context, const PointI& sPoint, bool isDistanceToStart);
        static uint64_t encodeRoutePointId(const std::shared_ptr<const Model::Road>& road, uint64_t pointIndex, bool positive);
        static uint64_t encodeRoutePointId(const std::shared_ptr<const Model::Road>& road, uint64_t pointIndex);
        static float estimateTimeDistance(
            OsmAnd::RoutePlannerContext::CalculationContext* context,
            const PointI& from,
            const PointI& to);
        static int roadPriorityComparator(
            double aDistanceFromStart, double aDistanceToEnd,
            double bDistanceFromStart, double bDistanceToEnd,
            double heuristicCoefficient);
        static void calculateRouteSegment(
            OsmAnd::RoutePlannerContext::CalculationContext* context,
            bool reverseWaySearch,
            RoadSegmentsPriorityQueue& graphSegments,
            QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& visitedSegments,
            const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& segment,
            QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& oppositeSegments,
            bool forwardDirection);
        static float calculateTurnTime(
            OsmAnd::RoutePlannerContext::CalculationContext* context,
            const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& a,
            uint32_t aEndPointIndex,
            const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& b,
            uint32_t bEndPointIndex);
        static bool checkIfInitialMovementAllowedOnSegment(
            OsmAnd::RoutePlannerContext::CalculationContext* context,
            bool reverseWaySearch,
            QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& visitedSegments,
            const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& segment,
            bool forwardDirection,
            const std::shared_ptr<const Model::Road>& road);
        static bool checkIfOppositeSegmentWasVisited(
            OsmAnd::RoutePlannerContext::CalculationContext* context,
            bool reverseWaySearch,
            RoadSegmentsPriorityQueue& graphSegments,
            const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& segment,
            QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& oppositeSegments,
            const std::shared_ptr<const Model::Road>& road,
            uint32_t segmentEnd,
            bool forwardDirection,
            uint32_t intervalId,
            float segmentDist,
            float obstaclesTime);
        static float calculateTimeWithObstacles(
            OsmAnd::RoutePlannerContext::CalculationContext* context,
            const std::shared_ptr<const Model::Road>& road,
            float distOnRoadToPass,
            float obstaclesTime);
        static bool processRestrictions(
            OsmAnd::RoutePlannerContext::CalculationContext* context,
            QList< std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& prescripted,
            const std::shared_ptr<const Model::Road>& road,
            const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& inputNext,
            bool reverseWay);
        static void processIntersections(
            OsmAnd::RoutePlannerContext::CalculationContext* context,
            RoadSegmentsPriorityQueue& graphSegments,
            QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& visitedSegments,
            float distFromStart,
            const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& segment,
            uint32_t segmentEnd,
            const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& inputNext,
            bool reverseWaySearch,
            bool addSameRoadFutureDirection);
        static bool checkPartialRecalculationPossible(
            OsmAnd::RoutePlannerContext::CalculationContext* context,
            QMap<uint64_t, std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> >& visitedOppositeSegments,
            std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& segment);
        static std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> loadRouteCalculationSegment(
            OsmAnd::RoutePlannerContext* context,
            uint32_t x31, uint32_t y31);
        static void printDebugInformation(OsmAnd::RoutePlannerContext::CalculationContext* ctx,
            int directSegmentSize, int reverseSegmentSize,
            std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>);
        static double h(OsmAnd::RoutePlannerContext::CalculationContext* context,
            const PointI& start, const PointI& end,
            const std::shared_ptr<RoutePlannerContext::RouteCalculationSegment>& next);

        enum {
            RoutePointsBitSpace = 11,
        };

        static OsmAnd::RouteCalculationResult prepareResult(OsmAnd::RoutePlannerContext::CalculationContext* context,
            std::shared_ptr<RoutePlannerContext::RouteCalculationSegment> finalSegment,
            bool leftSideNavigation);
        static void addRouteSegmentToRoute(QVector< std::shared_ptr<RouteSegment> >& route, const std::shared_ptr<RouteSegment>& segment, bool reverse);
        static bool combineTwoSegmentResult(const std::shared_ptr<RouteSegment>& toAdd, const std::shared_ptr<RouteSegment>& previous, bool reverse);
        static bool validateAllPointsConnected(const QVector< std::shared_ptr<RouteSegment> >& route);
        static void splitRoadsAndAttachRoadSegments(OsmAnd::RoutePlannerContext::CalculationContext* context, QVector< std::shared_ptr<RouteSegment> >& route);
        static void calculateTimeSpeedInRoute(OsmAnd::RoutePlannerContext::CalculationContext* context, QVector< std::shared_ptr<RouteSegment> >& route);
        static void addTurnInfoToRoute( bool leftSideNavigation, QVector< std::shared_ptr<RouteSegment> >& route );
        static void attachRouteSegments(
            OsmAnd::RoutePlannerContext::CalculationContext* context,
            QVector< std::shared_ptr<RouteSegment> >& route,
            const QVector< std::shared_ptr<RouteSegment> >::iterator& itSegment,
            uint32_t pointIdx,
            bool isIncrement);

        static void printRouteInfo(QVector< std::shared_ptr<RouteSegment> >& route);
    public:
        virtual ~RoutePlanner();
        enum {
            MinTurnAngle = 45
        };

        static bool findClosestRoadPoint(
            OsmAnd::RoutePlannerContext* context,
            double latitude, double longitude,
            std::shared_ptr<const OsmAnd::Model::Road>* closestRoad = nullptr,
            uint32_t* closestPointIndex = nullptr,
            double* sqDistanceToClosestPoint = nullptr,
            uint32_t* rx31 = nullptr, uint32_t* ry31 = nullptr);

        static RouteCalculationResult calculateRoute(
            OsmAnd::RoutePlannerContext* context,
            const QList< std::pair<double, double> >& points,
            bool leftSideNavigation,
            const OsmAnd::IQueryController* const controller = nullptr);

        friend class OsmAnd::RoutePlannerContext;
        friend class OsmAnd::RoutePlannerAnalyzer;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_ROUTE_PLANNER_H_
