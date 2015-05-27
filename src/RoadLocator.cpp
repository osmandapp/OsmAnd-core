#include "RoadLocator.h"
#include "RoadLocator_P.h"

OsmAnd::RoadLocator::RoadLocator(
    const std::shared_ptr<const IObfsCollection>& obfsCollection_,
    const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache>& cache_ /*= nullptr*/)
    : _p(new RoadLocator_P(this))
    , obfsCollection(obfsCollection_)
    , cache(cache_)
{
}

OsmAnd::RoadLocator::~RoadLocator()
{
}

std::shared_ptr<const OsmAnd::Road> OsmAnd::RoadLocator::findNearestRoad(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel,
    const ObfRoutingSectionReader::VisitorFunction filter /*= nullptr*/,
    int* const outNearestRoadPointIndex /*= nullptr*/,
    double* const outDistanceToNearestRoadPoint /*= nullptr*/) const
{
    return findNearestRoadEx(
        position31,
        radiusInMeters,
        dataLevel,
        filter,
        outNearestRoadPointIndex,
        outDistanceToNearestRoadPoint);
}

QList< std::shared_ptr<const OsmAnd::Road> > OsmAnd::RoadLocator::findRoadsInArea(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel,
    const ObfRoutingSectionReader::VisitorFunction filter /*= nullptr*/) const
{
    return findRoadsInAreaEx(
        position31,
        radiusInMeters,
        dataLevel,
        filter);
}

std::shared_ptr<const OsmAnd::Road> OsmAnd::RoadLocator::findNearestRoadEx(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel,
    const ObfRoutingSectionReader::VisitorFunction filter /*= nullptr*/,
    int* const outNearestRoadPointIndex /*= nullptr*/,
    double* const outDistanceToNearestRoadPoint /*= nullptr*/,
    QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* const outReferencedCacheEntries /*= nullptr*/) const
{
    return _p->findNearestRoadEx(
        position31,
        radiusInMeters,
        dataLevel,
        filter,
        outNearestRoadPointIndex,
        outDistanceToNearestRoadPoint,
        outReferencedCacheEntries);
}

QList< std::shared_ptr<const OsmAnd::Road> > OsmAnd::RoadLocator::findRoadsInAreaEx(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel,
    const ObfRoutingSectionReader::VisitorFunction filter /*= nullptr*/,
    QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* const outReferencedCacheEntries /*= nullptr*/) const
{
    return _p->findRoadsInAreaEx(
        position31,
        radiusInMeters,
        dataLevel,
        filter,
        outReferencedCacheEntries);
}

std::shared_ptr<const OsmAnd::Road> OsmAnd::RoadLocator::findNearestRoad(
    const QList< std::shared_ptr<const Road> >& collection,
    const PointI position31,
    const double radiusInMeters,
    const ObfRoutingSectionReader::VisitorFunction filter /*= nullptr*/,
    int* const outNearestRoadPointIndex /*= nullptr*/,
    double* const outDistanceToNearestRoadPoint /*= nullptr*/)
{
    return RoadLocator_P::findNearestRoad(
        collection,
        position31,
        radiusInMeters,
        filter,
        outNearestRoadPointIndex,
        outDistanceToNearestRoadPoint);
}

std::shared_ptr<const OsmAnd::Road> OsmAnd::RoadLocator::findNearestRoad(
    const QList< std::shared_ptr<const Road> >& collection,
    const PointI position31,
    const ObfRoutingSectionReader::VisitorFunction filter /*= nullptr*/,
    int* const outNearestRoadPointIndex /*= nullptr*/,
    double* const outDistanceToNearestRoadPoint /*= nullptr*/)
{
    return RoadLocator_P::findNearestRoad(
        collection,
        position31,
        filter,
        outNearestRoadPointIndex,
        outDistanceToNearestRoadPoint);
}

QList< std::shared_ptr<const OsmAnd::Road> > OsmAnd::RoadLocator::findRoadsInArea(
    const QList< std::shared_ptr<const Road> >& collection,
    const PointI position31,
    const double radiusInMeters,
    const ObfRoutingSectionReader::VisitorFunction filter /*= nullptr*/)
{
    return RoadLocator_P::findRoadsInArea(
        collection,
        position31,
        radiusInMeters,
        filter);
}
