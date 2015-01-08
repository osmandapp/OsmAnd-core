#include "RoadLocator_P.h"
#include "RoadLocator.h"

#include "Road.h"
#include "IObfsCollection.h"
#include "ObfDataInterface.h"
#include "Utilities.h"

OsmAnd::RoadLocator_P::RoadLocator_P(RoadLocator* const owner_)
    : owner(owner_)
{
}


OsmAnd::RoadLocator_P::~RoadLocator_P()
{
}

std::shared_ptr<const OsmAnd::Road> OsmAnd::RoadLocator_P::findNearestRoadEx(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel,
    int* const outNearestRoadPointIndex,
    double* const outDistanceToNearestRoadPoint,
    QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* const outReferencedCacheEntries) const
{
    QList< std::shared_ptr<const Road> > roadsInBBox;

    const auto bbox31 = (AreaI)Utilities::boundingBox31FromAreaInMeters(radiusInMeters, position31);
    const auto obfDataInterface = owner->obfsCollection->obtainDataInterface(bbox31);
    obfDataInterface->loadRoads(
        dataLevel,
        &bbox31,
        &roadsInBBox,
        nullptr,
        nullptr,
        owner->cache.get(),
        outReferencedCacheEntries,
        nullptr,
        nullptr);

    return findNearestRoad(roadsInBBox, position31, radiusInMeters, outNearestRoadPointIndex, outDistanceToNearestRoadPoint);
}

QList< std::shared_ptr<const OsmAnd::Road> > OsmAnd::RoadLocator_P::findRoadsInAreaEx(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel,
    QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* const outReferencedCacheEntries) const
{
    QList< std::shared_ptr<const Road> > roadsInBBox;

    const auto bbox31 = (AreaI)Utilities::boundingBox31FromAreaInMeters(radiusInMeters, position31);
    const auto obfDataInterface = owner->obfsCollection->obtainDataInterface(bbox31);
    obfDataInterface->loadRoads(
        dataLevel,
        &bbox31,
        &roadsInBBox,
        nullptr,
        nullptr,
        owner->cache.get(),
        outReferencedCacheEntries,
        nullptr,
        nullptr);

    return findRoadsInArea(roadsInBBox, position31, radiusInMeters);
}

std::shared_ptr<const OsmAnd::Road> OsmAnd::RoadLocator_P::findNearestRoad(
    const QList< std::shared_ptr<const Road> >& collection,
    const PointI position31,
    const double radiusInMeters,
    int* const outNearestRoadPointIndex,
    double* const outDistanceToNearestRoadPoint)
{
    if (outNearestRoadPointIndex)
        *outNearestRoadPointIndex = -1;
    if (outDistanceToNearestRoadPoint)
        *outDistanceToNearestRoadPoint = -1.0;

    int nearestRoadPointIndex;
    double distanceToNearestRoadPoint;
    const auto nearestRoad = findNearestRoad(collection, position31, &nearestRoadPointIndex, &distanceToNearestRoadPoint);
    if (!nearestRoad)
        return nullptr;

    if (distanceToNearestRoadPoint > radiusInMeters)
        return nullptr;

    if (outNearestRoadPointIndex)
        *outNearestRoadPointIndex = nearestRoadPointIndex;
    if (outDistanceToNearestRoadPoint)
        *outDistanceToNearestRoadPoint = distanceToNearestRoadPoint;

    return nearestRoad;
}

std::shared_ptr<const OsmAnd::Road> OsmAnd::RoadLocator_P::findNearestRoad(
    const QList< std::shared_ptr<const Road> >& collection,
    const PointI position31,
    int* const outNearestRoadPointIndex,
    double* const outDistanceToNearestRoadPoint)
{
    if (outNearestRoadPointIndex)
        *outNearestRoadPointIndex = -1;
    if (outDistanceToNearestRoadPoint)
        *outDistanceToNearestRoadPoint = -1.0;

    std::shared_ptr<const Road> minDistanceRoad;
    int minDistancePointIdx = -1;
    double minSqDistance = std::numeric_limits<double>::max();
    uint32_t min31x, min31y;

    for (const auto& road : constOf(collection))
    {
        const auto& points31 = road->points31;
        if (points31.size() <= 1)
            continue;

        for (auto idx = 1, count = points31.size(); idx < count; idx++)
        {
            const auto& cpx31 = points31[idx].x;
            const auto& cpy31 = points31[idx].y;
            const auto& ppx31 = points31[idx - 1].x;
            const auto& ppy31 = points31[idx - 1].y;

            auto sqDistance = Utilities::squareDistance31(cpx31, cpy31, ppx31, ppy31);

            uint32_t rx31;
            uint32_t ry31;
            auto projection = Utilities::projection31(ppx31, ppy31, cpx31, cpy31, position31.x, position31.y);
            if (projection < 0)
            {
                rx31 = ppx31;
                ry31 = ppy31;
            }
            else if (projection >= sqDistance)
            {
                rx31 = cpx31;
                ry31 = cpy31;
            }
            else
            {
                const auto& factor = projection / sqDistance;
                rx31 = ppx31 + (cpx31 - ppx31) * factor;
                ry31 = ppy31 + (cpy31 - ppy31) * factor;
            }
            sqDistance = Utilities::squareDistance31(rx31, ry31, position31.x, position31.y);
            if (!road || sqDistance < minSqDistance)
            {
                minDistanceRoad = road;
                minDistancePointIdx = idx;
                minSqDistance = sqDistance;
                min31x = rx31;
                min31y = ry31;
            }
        }
    }

    if (minDistanceRoad)
    {
        if (outNearestRoadPointIndex)
            *outNearestRoadPointIndex = minDistancePointIdx;
        if (outDistanceToNearestRoadPoint)
            *outDistanceToNearestRoadPoint = qSqrt(minSqDistance);
    }

    return minDistanceRoad;
}

QList< std::shared_ptr<const OsmAnd::Road> > OsmAnd::RoadLocator_P::findRoadsInArea(
    const QList< std::shared_ptr<const Road> >& collection,
    const PointI position31, const double radiusInMeters)
{
    return QList< std::shared_ptr<const Road> >();
}
