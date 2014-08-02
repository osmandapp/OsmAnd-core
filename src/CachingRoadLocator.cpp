#include "CachingRoadLocator.h"
#include "CachingRoadLocator_P.h"

OsmAnd::CachingRoadLocator::CachingRoadLocator(const std::shared_ptr<const IObfsCollection>& obfsCollection_)
    : _p(new CachingRoadLocator_P(this))
    , obfsCollection(obfsCollection_)
{
}

OsmAnd::CachingRoadLocator::~CachingRoadLocator()
{
}

std::shared_ptr<const OsmAnd::Model::Road> OsmAnd::CachingRoadLocator::findNearestRoad(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel,
    int* const outNearestRoadPointIndex /*= nullptr*/,
    double* const outDistanceToNearestRoadPoint /*= nullptr*/) const
{
    return _p->findNearestRoad(position31, radiusInMeters, dataLevel, outNearestRoadPointIndex, outDistanceToNearestRoadPoint);
}

QList< std::shared_ptr<const OsmAnd::Model::Road> > OsmAnd::CachingRoadLocator::findRoadsInArea(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel) const
{
    return _p->findRoadsInArea(position31, radiusInMeters, dataLevel);
}

void OsmAnd::CachingRoadLocator::clearCache()
{
    _p->clearCache();
}

void OsmAnd::CachingRoadLocator::clearCacheConditional(const std::function<bool(const std::shared_ptr<const ObfRoutingSectionReader::DataBlock>& dataBlock)> shouldRemoveFromCacheFunctor)
{
    _p->clearCacheConditional(shouldRemoveFromCacheFunctor);
}

void OsmAnd::CachingRoadLocator::clearCacheInBBox(const AreaI bbox31, const bool checkAlsoIntersection)
{
    _p->clearCacheInBBox(bbox31, checkAlsoIntersection);
}

void OsmAnd::CachingRoadLocator::clearCacheInTiles(const QSet<TileId> tiles, const ZoomLevel zoomLevel, const bool checkAlsoIntersection)
{
    _p->clearCacheInTiles(tiles, zoomLevel, checkAlsoIntersection);
}

void OsmAnd::CachingRoadLocator::clearCacheNotInBBox(const AreaI bbox31, const bool checkAlsoIntersection)
{
    _p->clearCacheNotInBBox(bbox31, checkAlsoIntersection);
}

void OsmAnd::CachingRoadLocator::clearCacheNotInTiles(const QSet<TileId> tiles, const ZoomLevel zoomLevel, const bool checkAlsoIntersection)
{
    _p->clearCacheNotInTiles(tiles, zoomLevel, checkAlsoIntersection);
}
