#ifndef _OSMAND_CORE_I_ROAD_LOCATOR_H_
#define _OSMAND_CORE_I_ROAD_LOCATOR_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Data/ObfRoutingSectionReader.h>

namespace OsmAnd
{
    class IObfsCollection;
    class Road;

    struct OSMAND_CORE_API RoadInfo
    {
    public:
        RoadInfo();
        virtual ~RoadInfo();
        
        double distSquare;
        uint32_t preciseX;
        uint32_t preciseY;
    };
    
    class OSMAND_CORE_API IRoadLocator
    {
        Q_DISABLE_COPY_AND_MOVE(IRoadLocator);
    private:
    protected:
        IRoadLocator();
    public:
        virtual ~IRoadLocator();

        virtual std::shared_ptr<const Road> findNearestRoad(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel = RoutingDataLevel::Detailed,
            const ObfRoutingSectionReader::VisitorFunction filter = nullptr,
            int* const outNearestRoadPointIndex = nullptr,
            double* const outDistanceToNearestRoadPoint = nullptr) const = 0;
        virtual QVector<std::pair<std::shared_ptr<const Road>, std::shared_ptr<const RoadInfo>>> findNearestRoads(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel = RoutingDataLevel::Detailed,
            const ObfRoutingSectionReader::VisitorFunction filter = nullptr,
            QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>> * const outReferencedCacheEntries = nullptr) const = 0;
        virtual QList< std::shared_ptr<const Road> > findRoadsInArea(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel,
            const ObfRoutingSectionReader::VisitorFunction filter = nullptr) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_ROAD_LOCATOR_H_)
