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

#ifndef __ROUTE_PLANNER_CONTEXT_H_
#define __ROUTE_PLANNER_CONTEXT_H_

#include <limits>
#include <memory>

#include <QString>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QList>

#include <OsmAndCore.h>
#include <Common.h>
#include <RoutingConfiguration.h>
#include <Road.h>
#include <RouteSegment.h>
#include <RoutingProfileContext.h>
#include <CommonTypes.h>
#include <ctime>
#include <chrono>


namespace OsmAnd {
    class RoutePlanner;
    class RoutePlannerAnalyzer;

    struct RouteStatistics
    {
        RouteStatistics() {
            memset(this, sizeof(RouteStatistics), 0);
        }

        uint32_t forwardIterations;
        uint32_t backwardIterations;
        uint32_t sizeOfDQueue;
        uint32_t sizeOfRQueue;
        uint64_t timeToLoad;
        uint64_t timeToCalculate;

        std::chrono::steady_clock::time_point timeToLoadBegin;
        std::chrono::steady_clock::time_point timeToCalculateBegin;

        uint32_t loadedTiles;
        uint32_t unloadedTiles;
        uint32_t distinctLoadedTiles;
        uint32_t loadedPrevUnloadedTiles;

    };

    class OSMAND_CORE_API RoutePlannerContext
    {
    public:
        class OSMAND_CORE_API RouteCalculationSegment
        {
        private:
        protected:
            std::shared_ptr<RouteCalculationSegment> _next;
            std::shared_ptr<RouteCalculationSegment> _parent;
            uint32_t _parentEndPointIndex;

            float _distanceFromStart;
            float _distanceToEnd;

            int _assignedDirection;

            RouteCalculationSegment(const std::shared_ptr<Model::Road>& road, uint32_t pointIndex);

            void dump(const QString& prefix = QString()) const;
        public:
            virtual ~RouteCalculationSegment();

            const std::shared_ptr<Model::Road> road;
            const uint32_t pointIndex;

            const std::shared_ptr<RouteCalculationSegment>& next;
            const std::shared_ptr<RouteCalculationSegment>& parent;
            const uint32_t& parentEndPointIndex;

            // 1 - only positive allowed, -1 - only negative allowed
            int _allowedDirection;

            friend class OsmAnd::RoutePlanner;
            friend class OsmAnd::RoutePlannerAnalyzer;
            friend class OsmAnd::RoutePlannerContext;
        };

        class OSMAND_CORE_API RouteCalculationFinalSegment : public RouteCalculationSegment
        {
        private:
        protected:
            RouteCalculationFinalSegment(const std::shared_ptr<Model::Road>& road, uint32_t pointIndex);

            bool _reverseWaySearch;
            std::shared_ptr<RouteCalculationSegment> _opposite;
        public:
            virtual ~RouteCalculationFinalSegment();

            const bool& reverseWaySearch;
            const std::shared_ptr<RouteCalculationSegment>& opposite;

            friend class OsmAnd::RoutePlanner;
            friend class OsmAnd::RoutePlannerAnalyzer;
            friend class OsmAnd::RoutePlannerContext;
        };

        class OSMAND_CORE_API RoutingSubsectionContext
        {
        private:
            uint32_t _mixedLoadsCounter;
        protected:
            RoutingSubsectionContext(RoutePlannerContext* owner, const std::shared_ptr<ObfReader>& origin, const std::shared_ptr<ObfRoutingSection::Subsection>& subsection);

            QMap< uint64_t, std::shared_ptr<RouteCalculationSegment> > _roadSegments;

            void markLoaded();
            void unload();
            std::shared_ptr<RouteCalculationSegment> loadRouteCalculationSegment(uint32_t x31, uint32_t y31, QMap<uint64_t, std::shared_ptr<Model::Road> >& processed, const std::shared_ptr<RouteCalculationSegment>& original) const;
        public:
            virtual ~RoutingSubsectionContext();

            const std::shared_ptr<ObfRoutingSection::Subsection> subsection;
            RoutePlannerContext* const owner;
            const std::shared_ptr<ObfReader> origin;

            bool isLoaded() const;
            uint32_t getLoadsCounter() const;

            void registerRoad(const std::shared_ptr<Model::Road>& road);
            void collectRoads(QList< std::shared_ptr<Model::Road> >& output, QMap<uint64_t, std::shared_ptr<Model::Road> >* duplicatesRegistry = nullptr);

            friend class OsmAnd::RoutePlanner;
            friend class OsmAnd::RoutePlannerAnalyzer;
            friend class OsmAnd::RoutePlannerContext;
        };

        struct OSMAND_CORE_API BorderLine
        {
            uint32_t _y31;
            QList< std::shared_ptr<OsmAnd::ObfRoutingSection::BorderLinePoint> > _borderPoints;
        };

        class OSMAND_CORE_API CalculationContext
        {
        private:
        protected:
            PointI _startPoint;
            PointI _targetPoint;

            uint64_t _entranceRoadId;
            int _entranceRoadDirection;
            
            QList< std::shared_ptr<BorderLine> > _borderLines;
            QVector< uint32_t > _borderLinesY31;
            
            CalculationContext(RoutePlannerContext* owner);
        public:
            virtual ~CalculationContext();

            RoutePlannerContext* const owner;

            friend class OsmAnd::RoutePlanner;
            friend class OsmAnd::RoutePlannerAnalyzer;
            friend class OsmAnd::RoutePlannerContext;
        };
    private:
    protected:
        QList< std::shared_ptr<RoutingSubsectionContext> > _subsectionsContexts;
        QMap< ObfRoutingSection::Subsection*, std::shared_ptr<RoutingSubsectionContext> > _subsectionsContextsLUT;
        QMap< ObfRoutingSection*, std::shared_ptr<ObfReader> > _sourcesLUT;

        QList< std::shared_ptr<RouteSegment> > _previouslyCalculatedRoute;

        QMap< uint64_t, QList< std::shared_ptr<RoutingSubsectionContext> > > _indexedSubsectionsContexts;
        QMap< uint64_t, QList< std::shared_ptr<Model::Road> > > _cachedRoadsInTiles;

        float _initialHeading;
        bool _useBasemap;
        size_t _memoryUsageLimit;
        uint32_t _roadTilesLoadingZoomLevel;
        int _planRoadDirection;
        float _heuristicCoefficient;
        float _partialRecalculationDistanceLimit;
        std::shared_ptr<RouteStatistics> _routeStatistics;

        enum {
            DefaultRoadTilesLoadingZoomLevel = 16,
        };
    public:
        RoutePlannerContext(
            const QList< std::shared_ptr<ObfReader> >& sources,
            const std::shared_ptr<RoutingConfiguration>& routingConfig,
            const QString& vehicle,
            bool useBasemap,
            float initialHeading = std::numeric_limits<float>::quiet_NaN(),
            QHash<QString, QString>* options = nullptr,
            size_t memoryLimit = std::numeric_limits<size_t>::max());
        virtual ~RoutePlannerContext();

        const QList< std::shared_ptr<ObfReader> > sources;
        const std::shared_ptr<RoutingConfiguration> configuration;
        const std::unique_ptr<RoutingProfileContext> profileContext;

        friend class OsmAnd::RoutePlanner;
        friend class OsmAnd::RoutePlannerAnalyzer;
    };

} // namespace OsmAnd

#endif // __ROUTE_PLANNER_CONTEXT_H_
