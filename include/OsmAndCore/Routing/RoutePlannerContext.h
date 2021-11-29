#ifndef _OSMAND_CORE_ROUTE_PLANNER_CONTEXT_H_
#define _OSMAND_CORE_ROUTE_PLANNER_CONTEXT_H_

#include <limits>
#include <memory>

#include <OsmAndCore/stdlib_common.h>
#include <ctime>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/Routing/RoutingConfiguration.h>
#include <OsmAndCore/Data/Model/Road.h>
#include <OsmAndCore/Routing/RouteSegment.h>
#include <OsmAndCore/Routing/RoutingProfileContext.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfReader;
    class ObfRoutingSectionInfo;
    class ObfRoutingSubsectionInfo;
    class ObfRoutingBorderLinePoint;
    class RoutePlanner;

    struct RouteStatistics
    {
        uint32_t forwardIterations;
        uint32_t backwardIterations;
        uint32_t sizeOfDQueue;
        uint32_t sizeOfRQueue;
        uint64_t timeToLoad;
        uint64_t timeToCalculate;

        uint32_t maxLoadedTiles;
        uint32_t loadedTiles;
        uint32_t unloadedTiles;
        uint32_t distinctLoadedTiles;
        uint32_t loadedPrevUnloadedTiles;

        std::chrono::steady_clock::time_point timeToLoadBegin;
        std::chrono::steady_clock::time_point timeToCalculateBegin;

        RouteStatistics() {
            //TODO: very dangerous to zeroify non-POD types (timeToLoadBegin, timeToCalculateBegin)
            memset(this, 0, sizeof(struct RouteStatistics));
        }
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

            RouteCalculationSegment(const std::shared_ptr<const Model::Road>& road, uint32_t pointIndex);

            void dump(const QString& prefix = {}) const;
        public:
            virtual ~RouteCalculationSegment();

            const std::shared_ptr<const Model::Road> road;
            const uint32_t pointIndex;

            const std::shared_ptr<RouteCalculationSegment>& next;
            const std::shared_ptr<RouteCalculationSegment>& parent;
            const uint32_t& parentEndPointIndex;

            // 1 - only positive allowed, -1 - only negative allowed
            int _allowedDirection;

            friend class OsmAnd::RoutePlanner;
            friend class OsmAnd::RoutePlannerContext;
        };

        class OSMAND_CORE_API RouteCalculationFinalSegment : public RouteCalculationSegment
        {
        private:
        protected:
            RouteCalculationFinalSegment(const std::shared_ptr<const Model::Road>& road, uint32_t pointIndex);

            bool _reverseWaySearch;
            std::shared_ptr<RouteCalculationSegment> _opposite;
        public:
            virtual ~RouteCalculationFinalSegment();

            const bool& reverseWaySearch;
            const std::shared_ptr<RouteCalculationSegment>& opposite;

            friend class OsmAnd::RoutePlanner;
            friend class OsmAnd::RoutePlannerContext;
        };

        class OSMAND_CORE_API RoutingSubsectionContext
        {
        private:
            int _mixedLoadsCounter;
            int _access;
        protected:
            RoutingSubsectionContext(RoutePlannerContext* owner, const std::shared_ptr<ObfReader>& origin, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection);

            QMap< uint64_t, std::shared_ptr<RouteCalculationSegment> > _roadSegments;

            void markLoaded();
            void unload();
            std::shared_ptr<RouteCalculationSegment> loadRouteCalculationSegment(uint32_t x31, uint32_t y31, QMap<uint64_t, std::shared_ptr<const Model::Road> >& processed, const std::shared_ptr<RouteCalculationSegment>& original);
        public:
            virtual ~RoutingSubsectionContext();

            const std::shared_ptr<const ObfRoutingSubsectionInfo> subsection;
            RoutePlannerContext* const owner;
            const std::shared_ptr<ObfReader> origin;

            bool isLoaded() const;
            uint32_t getLoadsCounter() const;
            uint32_t getAccessCounter() const {return _access;}

            void registerRoad(const std::shared_ptr<const Model::Road>& road);
            void collectRoads(QList< std::shared_ptr<const Model::Road> >& output, QMap<uint64_t, std::shared_ptr<const Model::Road> >* duplicatesRegistry = nullptr);

            friend class OsmAnd::RoutePlanner;
            friend class OsmAnd::RoutePlannerContext;
        };

        struct OSMAND_CORE_API BorderLine
        {
            uint32_t _y31;
            QList< std::shared_ptr<const ObfRoutingBorderLinePoint> > _borderPoints;
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
            friend class OsmAnd::RoutePlannerContext;
        };
    private:
    protected:
        QList< std::shared_ptr<RoutingSubsectionContext> > _subsectionsContexts;
        QMap< const ObfRoutingSubsectionInfo*, std::shared_ptr<RoutingSubsectionContext> > _subsectionsContextsLUT;
        QMap< const ObfRoutingSectionInfo*, std::shared_ptr<ObfReader> > _sourcesLUT;

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
        int _loadedTiles;
        std::shared_ptr<RouteStatistics> _routeStatistics;

        enum {
            DefaultRoadTilesLoadingZoomLevel = 16,
        };
    public:
        RoutePlannerContext(
            const QList< std::shared_ptr<OsmAnd::ObfReader> >& sources,
            const std::shared_ptr<OsmAnd::RoutingConfiguration>& routingConfig,
            const QString& vehicle,
            bool useBasemap,
            float initialHeading = std::numeric_limits<float>::quiet_NaN(),
            QHash<QString, QString>* options = nullptr,
            size_t memoryLimit = 1000000);
        virtual ~RoutePlannerContext();

        const QList< std::shared_ptr<OsmAnd::ObfReader> > sources;
        const std::shared_ptr<OsmAnd::RoutingConfiguration> configuration;
        const std::shared_ptr<OsmAnd::RoutingProfileContext> profileContext;

        uint32_t getCurrentlyLoadedTiles();
        uint32_t getCurrentEstimatedSize();
        void unloadUnusedTiles(size_t memoryTarget);

        friend class OsmAnd::RoutePlanner;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_ROUTE_PLANNER_CONTEXT_H_)
