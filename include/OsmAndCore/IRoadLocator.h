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
    namespace Model
    {
        class Road;
    }

    class OSMAND_CORE_API IRoadLocator
    {
        Q_DISABLE_COPY(IRoadLocator);
    private:
    protected:
        IRoadLocator();
    public:
        virtual ~IRoadLocator();

        virtual std::shared_ptr<const Model::Road> findNearestRoad(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel,
            int* const outNearestRoadPointIndex = nullptr,
            double* const outDistanceToNearestRoadPoint = nullptr) const = 0;
        virtual QList< std::shared_ptr<const Model::Road> > findRoadsInArea(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_ROAD_LOCATOR_H_)
