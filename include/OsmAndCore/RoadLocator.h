#ifndef _OSMAND_CORE_ROAD_LOCATOR_H_
#define _OSMAND_CORE_ROAD_LOCATOR_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IRoadLocator.h>
#include <OsmAndCore/Data/ObfRoutingSectionReader.h>

namespace OsmAnd
{
    class IObfsCollection;
    class Road;

    class RoadLocator_P;
    class OSMAND_CORE_API RoadLocator : public IRoadLocator
    {
        Q_DISABLE_COPY_AND_MOVE(RoadLocator);
    private:
        PrivateImplementation<RoadLocator_P> _p;
    protected:
    public:
        RoadLocator(
            const std::shared_ptr<const IObfsCollection>& obfsCollection,
            const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache>& cache = nullptr);
        virtual ~RoadLocator();

        const std::shared_ptr<const IObfsCollection> obfsCollection;
        const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache> cache;

        virtual std::shared_ptr<const Road> findNearestRoad(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel = RoutingDataLevel::Detailed,
            const ObfRoutingSectionReader::VisitorFunction filter = nullptr,
            int* const outNearestRoadPointIndex = nullptr,
            double* const outDistanceToNearestRoadPoint = nullptr) const;
        virtual QVector<std::pair<std::shared_ptr<const Road>, double>> findNearestRoads(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel = RoutingDataLevel::Detailed,
            const ObfRoutingSectionReader::VisitorFunction filter = nullptr,
            QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>> * const outReferencedCacheEntries = nullptr) const;
        virtual QList<std::shared_ptr<const Road>> findRoadsInArea(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel = RoutingDataLevel::Detailed,
            const ObfRoutingSectionReader::VisitorFunction filter = nullptr) const;

        std::shared_ptr<const Road> findNearestRoadEx(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel = RoutingDataLevel::Detailed,
            const ObfRoutingSectionReader::VisitorFunction filter = nullptr,
            int* const outNearestRoadPointIndex = nullptr,
            double* const outDistanceToNearestRoadPoint = nullptr,
            QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>>* const outReferencedCacheEntries = nullptr) const;
        QList<std::shared_ptr<const Road>> findRoadsInAreaEx(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel,
            const ObfRoutingSectionReader::VisitorFunction filter = nullptr,
            QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>>* const outReferencedCacheEntries = nullptr) const;

        static std::shared_ptr<const Road> findNearestRoad(
            const QList<std::shared_ptr<const Road>>& collection,
            const PointI position31,
            const double radiusInMeters,
            const ObfRoutingSectionReader::VisitorFunction filter = nullptr,
            int* const outNearestRoadPointIndex = nullptr,
            double* const outDistanceToNearestRoadPoint = nullptr);
        static std::shared_ptr<const Road> findNearestRoad(
            const QList<std::shared_ptr<const Road>>& collection,
            const PointI position31,
            const ObfRoutingSectionReader::VisitorFunction filter = nullptr,
            int* const outNearestRoadPointIndex = nullptr,
            double* const outDistanceToNearestRoadPoint = nullptr);
        static QList<std::shared_ptr<const Road>> findRoadsInArea(
            const QList< std::shared_ptr<const Road> >& collection,
            const PointI position31,
            const double radiusInMeters,
            const ObfRoutingSectionReader::VisitorFunction filter = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_ROAD_LOCATOR_H_)
