#include "RoadLocator_P.h"
#include "RoadLocator.h"

#include "Road.h"
#include "IObfsCollection.h"
#include "ObfDataInterface.h"
#include "Utilities.h"

static const double GPS_POSSIBLE_ERROR = 7;

OsmAnd::RoadLocator_P::RoadLocator_P(RoadLocator* const owner_)
    : owner(owner_)
{
}


OsmAnd::RoadLocator_P::~RoadLocator_P()
{
}

QList<std::shared_ptr<const OsmAnd::Road>> OsmAnd::RoadLocator_P::roadsInRadius(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel,
    QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>>* const outReferencedCacheEntries) const
{
    QList<std::shared_ptr<const Road>> result;

    const auto bbox31 = (AreaI)Utilities::boundingBox31FromAreaInMeters(radiusInMeters, position31);
    const auto obfDataInterface = owner->obfsCollection->obtainDataInterface(
        &bbox31,
        MinZoomLevel,
        MaxZoomLevel,
        ObfDataTypesMask().set(ObfDataType::Routing));
    obfDataInterface->loadRoads(
        dataLevel,
        &bbox31,
        &result,
        nullptr,
        nullptr,
        owner->cache.get(),
        outReferencedCacheEntries,
        nullptr,
        nullptr);

    return result;
}


std::shared_ptr<const OsmAnd::Road> OsmAnd::RoadLocator_P::findNearestRoadEx(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel,
    const ObfRoutingSectionReader::VisitorFunction filter,
    int* const outNearestRoadPointIndex,
    double* const outDistanceToNearestRoadPoint,
    QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>>* const outReferencedCacheEntries) const
{
    auto roadsInBBox = roadsInRadius(position31, radiusInMeters, dataLevel, outReferencedCacheEntries);
    return findNearestRoad(
        roadsInBBox,
        position31,
        radiusInMeters,
        filter,
        outNearestRoadPointIndex,
        outDistanceToNearestRoadPoint);
}

QVector<std::pair<std::shared_ptr<const OsmAnd::Road>, std::shared_ptr<const OsmAnd::RoadInfo>>> OsmAnd::RoadLocator_P::findNearestRoads(
        const OsmAnd::PointI position31,
        const double radiusInMeters,
        const OsmAnd::RoutingDataLevel dataLevel,
        const OsmAnd::ObfRoutingSectionReader::VisitorFunction filter,
        QList<std::shared_ptr<const OsmAnd::ObfRoutingSectionReader::DataBlock>>* const outReferencedCacheEntries) const
{
    auto roadsInBBox = roadsInRadius(position31, radiusInMeters, dataLevel, outReferencedCacheEntries);
    return sortedRoadsByDistance(
                roadsInBBox,
                position31,
                radiusInMeters,
                filter);
}

QList<std::shared_ptr<const OsmAnd::Road>> OsmAnd::RoadLocator_P::findRoadsInAreaEx(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel,
    const ObfRoutingSectionReader::VisitorFunction filter,
    QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>>* const outReferencedCacheEntries) const
{
    auto roadsInBBox = roadsInRadius(position31, radiusInMeters, dataLevel, outReferencedCacheEntries);
    return findRoadsInArea(
        roadsInBBox,
        position31,
        radiusInMeters,
        filter);
}

std::shared_ptr<const OsmAnd::Road> OsmAnd::RoadLocator_P::findNearestRoad(
    const QList<std::shared_ptr<const Road>>& collection,
    const PointI position31,
    const double radiusInMeters,
    const ObfRoutingSectionReader::VisitorFunction filter,
    int* const outNearestRoadPointIndex,
    double* const outDistanceToNearestRoadPoint)
{
    if (outNearestRoadPointIndex)
        *outNearestRoadPointIndex = -1;
    if (outDistanceToNearestRoadPoint)
        *outDistanceToNearestRoadPoint = -1.0;

    int nearestRoadPointIndex;
    double distanceToNearestRoadPoint;
    const auto nearestRoad = findNearestRoad(
        collection,
        position31,
        filter,
        &nearestRoadPointIndex,
        &distanceToNearestRoadPoint);
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
    const QList<std::shared_ptr<const Road>>& collection,
    const PointI position31,
    const ObfRoutingSectionReader::VisitorFunction filter,
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
    QList<ObfObjectId> processedIds;

    for (const auto& road : constOf(collection))
    {
        if (processedIds.contains(road->id))
            continue;
        
        processedIds.push_back(road->id);
        
        if (road->isDeleted())
            continue;
        
        const auto& points31 = road->points31;
        if (points31.size() <= 1)
            continue;

        if (filter && !filter(road))
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

            if (!minDistanceRoad || sqDistance < minSqDistance)
            {
                minDistanceRoad = road;
                minDistancePointIdx = idx;
                minSqDistance = sqDistance;
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

QVector<std::pair<std::shared_ptr<const OsmAnd::Road>, std::shared_ptr<const OsmAnd::RoadInfo>>> OsmAnd::RoadLocator_P::sortedRoadsByDistance(
    QList<std::shared_ptr<const Road>>& collection,
    const PointI position31,
    const double radiusInMeters,
    const ObfRoutingSectionReader::VisitorFunction filter)
{
    auto result = sortedRoadsByDistance(collection, position31, filter);
    result.erase(std::remove_if(result.begin(), result.end(),
                                [radiusInMeters]
                                (std::pair<std::shared_ptr<const Road>, std::shared_ptr<const OsmAnd::RoadInfo>> roadAndDistance) {
                                    return roadAndDistance.second->distSquare > std::pow(radiusInMeters, 2);
    }), result.end());
    return result;
}

QVector<std::pair<std::shared_ptr<const OsmAnd::Road>, std::shared_ptr<const OsmAnd::RoadInfo>>> OsmAnd::RoadLocator_P::sortedRoadsByDistance(
    QList<std::shared_ptr<const Road>>& collection,
    const PointI position31,
    const ObfRoutingSectionReader::VisitorFunction filter)
{
    QVector<std::pair<std::shared_ptr<const Road>, std::shared_ptr<const RoadInfo>>> result{};
    auto evRoadInfo = [position31](std::shared_ptr<const Road> road, std::shared_ptr<RoadInfo> roadInfo) {
        const auto& points31 = road->points31;
        double minSqDistance = std::numeric_limits<double>::max();
        for (auto idx = 1, count = points31.size(); idx < count; idx++)
        {
            const auto& cpx31 = points31[idx].x;
            const auto& cpy31 = points31[idx].y;
            const auto& ppx31 = points31[idx - 1].x;
            const auto& ppy31 = points31[idx - 1].y;

            auto mDist = Utilities::measuredDist31(cpx31, cpy31, ppx31, ppy31);
            uint32_t rx31;
            uint32_t ry31;
            auto projection = Utilities::projection31(ppx31, ppy31, cpx31, cpy31, position31.x, position31.y);
            if (projection < 0)
            {
                rx31 = ppx31;
                ry31 = ppy31;
            }
            else if (projection >= (mDist * mDist))
            {
                rx31 = cpx31;
                ry31 = cpy31;
            }
            else
            {
                const auto& factor = projection / (mDist * mDist);
                rx31 = ppx31 + (cpx31 - ppx31) * factor;
                ry31 = ppy31 + (cpy31 - ppy31) * factor;
            }
            double currentsDistSquare = Utilities::squareDistance31(rx31, ry31, position31.x, position31.y);

            if (currentsDistSquare < minSqDistance)
            {
                minSqDistance = currentsDistSquare;
                roadInfo->preciseX = rx31;
                roadInfo->preciseY = ry31;
                roadInfo->distSquare = minSqDistance;
            }
        }
        
        float prio = 1.f;
        roadInfo->distSquare = (roadInfo->distSquare + GPS_POSSIBLE_ERROR * GPS_POSSIBLE_ERROR) / (prio * prio);
    };
    
    for (auto road : collection)
    {
        if (road->points31.size() > 1 && (!filter || filter(road)))
        {
            auto roadInfo = std::make_shared<RoadInfo>();
            evRoadInfo(road, roadInfo);
            result.append(std::make_pair(road, roadInfo));
        }
    }
    
    std::sort(result.begin(), result.end(),
              []
              (std::pair<std::shared_ptr<const Road>, std::shared_ptr<const RoadInfo>> a,
               std::pair<std::shared_ptr<const Road>, std::shared_ptr<const RoadInfo>> b) {
                  
        return a.second->distSquare < b.second->distSquare;
    });
    
    return result;
}

QList<std::shared_ptr<const OsmAnd::Road>> OsmAnd::RoadLocator_P::findRoadsInArea(
    const QList<std::shared_ptr<const Road>>& collection,
    const PointI position31,
    const double radiusInMeters,
    const ObfRoutingSectionReader::VisitorFunction filter)
{
    QList<std::shared_ptr<const Road>> filteredRoads;

    for (const auto& road : constOf(collection))
    {
        const auto& points31 = road->points31;
        if (points31.size() <= 1)
            continue;

        if (filter && !filter(road))
            continue;

        filteredRoads.push_back(road);
    }

    return filteredRoads;
}
