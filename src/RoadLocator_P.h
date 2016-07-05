#ifndef _OSMAND_CORE_ROAD_LOCATOR_P_H_
#define _OSMAND_CORE_ROAD_LOCATOR_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "ObfRoutingSectionReader.h"

#include <tuple>

namespace OsmAnd
{
    class IObfsCollection;
    class Road;
    
    class RoadLocator;
    class RoadLocator_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(RoadLocator_P);
    private:
        QList<std::shared_ptr<const Road> > roadsInRadius(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel,
            QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>> * const outReferencedCacheEntries) const;
    protected:
        RoadLocator_P(RoadLocator* const owner);
    public:
        ~RoadLocator_P();

        ImplementationInterface<RoadLocator> owner;

        std::shared_ptr<const Road> findNearestRoadEx(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel,
            const ObfRoutingSectionReader::VisitorFunction filter,
            int* const outNearestRoadPointIndex,
            double* const outDistanceToNearestRoadPoint,
            QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* const outReferencedCacheEntries) const;
        QVector<std::pair<std::shared_ptr<const Road>, double>> findNearestRoads(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel,
            const ObfRoutingSectionReader::VisitorFunction filter,
            QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* const outReferencedCacheEntries) const;
        QList<std::shared_ptr<const Road>> findRoadsInAreaEx(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel,
            const ObfRoutingSectionReader::VisitorFunction filter,
            QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* const outReferencedCacheEntries) const;

        static std::shared_ptr<const Road> findNearestRoad(
            const QList<std::shared_ptr<const Road>>& collection,
            const PointI position31,
            const double radiusInMeters,
            const ObfRoutingSectionReader::VisitorFunction filter,
            int* const outNearestRoadPointIndex,
            double* const outDistanceToNearestRoadPoint);
        static std::shared_ptr<const Road> findNearestRoad(
            const QList< std::shared_ptr<const Road> >& collection,
            const PointI position31,
            const ObfRoutingSectionReader::VisitorFunction filter,
            int* const outNearestRoadPointIndex,
            double* const outDistanceToNearestRoadPoint);
        static QVector<std::pair<std::shared_ptr<const Road>, double>> sortedRoadsByDistance(
            QList<std::shared_ptr<const Road>>& collection,
            const PointI position31,
            const double radiusInMeters,
            const ObfRoutingSectionReader::VisitorFunction filter);
        static QVector<std::pair<std::shared_ptr<const Road>, double>> sortedRoadsByDistance(
            QList<std::shared_ptr<const Road>>& collection,
            const PointI position31,
            const ObfRoutingSectionReader::VisitorFunction filter);
        static QList<std::shared_ptr<const Road>> findRoadsInArea(
            const QList<std::shared_ptr<const Road>>& collection,
            const PointI position31,
            const double radiusInMeters,
            const ObfRoutingSectionReader::VisitorFunction filter);

        friend class OsmAnd::RoadLocator;
    };
}

#endif // !defined(_OSMAND_CORE_ROAD_LOCATOR_P_H_)
