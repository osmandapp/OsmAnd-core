#include "CachingRoadLocator_P.h"
#include "CachingRoadLocator.h"

#include "QtCommon.h"

#include "RoadLocator.h"
#include "Road.h"
#include "IObfsCollection.h"
#include "ObfDataInterface.h"
#include "Utilities.h"

OsmAnd::CachingRoadLocator_P::CachingRoadLocator_P(CachingRoadLocator* const owner_)
    : owner(owner_)
{
}

OsmAnd::CachingRoadLocator_P::~CachingRoadLocator_P()
{
}

std::shared_ptr<const OsmAnd::Road> OsmAnd::CachingRoadLocator_P::findNearestRoad(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel,
    const ObfRoutingSectionReader::VisitorFunction filter,
    int* const outNearestRoadPointIndex,
    double* const outDistanceToNearestRoadPoint) const
{
    QList< std::shared_ptr<const Road> > roadsInBBox;

    const auto bbox31 = (AreaI)Utilities::boundingBox31FromAreaInMeters(radiusInMeters, position31);
    const auto obfDataInterface = owner->obfsCollection->obtainDataInterface(
        &bbox31,
        MinZoomLevel,
        MaxZoomLevel,
        ObfDataTypesMask().set(ObfDataType::Routing));
    QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> > referencedCacheEntries;
    obfDataInterface->loadRoads(
        dataLevel,
        &bbox31,
        &roadsInBBox,
        nullptr,
        nullptr,
        &_cache,
        &referencedCacheEntries,
        nullptr,
        nullptr);

    {
        QMutexLocker scopedLocker(&_referencedDataBlocksMapMutex);

        for (auto& referencedBlock : referencedCacheEntries)
            _referencedDataBlocksMap[referencedBlock.get()].push_back(qMove(referencedBlock));
    }
    
    return RoadLocator::findNearestRoad(
        roadsInBBox,
        position31,
        radiusInMeters,
        filter,
        outNearestRoadPointIndex,
                outDistanceToNearestRoadPoint);
}

QVector<std::pair<std::shared_ptr<const OsmAnd::Road>, std::shared_ptr<const OsmAnd::RoadInfo>>> OsmAnd::CachingRoadLocator_P::findNearestRoads(
        const OsmAnd::PointI position31,
        const double radiusInMeters,
        const OsmAnd::RoutingDataLevel dataLevel,
        const OsmAnd::ObfRoutingSectionReader::VisitorFunction filter,
        QList<std::shared_ptr<const OsmAnd::ObfRoutingSectionReader::DataBlock>> * const outReferencedCacheEntries) const
{
    return {};
}

QList< std::shared_ptr<const OsmAnd::Road> > OsmAnd::CachingRoadLocator_P::findRoadsInArea(
    const PointI position31,
    const double radiusInMeters,
    const RoutingDataLevel dataLevel,
    const ObfRoutingSectionReader::VisitorFunction filter) const
{
    QList< std::shared_ptr<const Road> > roadsInBBox;

    const auto bbox31 = (AreaI)Utilities::boundingBox31FromAreaInMeters(radiusInMeters, position31);
    const auto obfDataInterface = owner->obfsCollection->obtainDataInterface(
        &bbox31,
        MinZoomLevel,
        MaxZoomLevel,
        ObfDataTypesMask().set(ObfDataType::Routing));
    QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> > referencedCacheEntries;
    obfDataInterface->loadRoads(
        dataLevel,
        &bbox31,
        &roadsInBBox,
        nullptr,
        nullptr,
        &_cache,
        &referencedCacheEntries,
        nullptr,
        nullptr);

    {
        QMutexLocker scopedLocker(&_referencedDataBlocksMapMutex);

        for (auto& referencedBlock : referencedCacheEntries)
            _referencedDataBlocksMap[referencedBlock.get()].push_back(qMove(referencedBlock));
    }

    return RoadLocator::findRoadsInArea(
        roadsInBBox,
        position31,
        radiusInMeters,
        filter);
}

void OsmAnd::CachingRoadLocator_P::clearCache()
{
    QMutexLocker scopedLocker(&_referencedDataBlocksMapMutex);

    for (auto& referencedDataBlocks : _referencedDataBlocksMap)
    {
        for (auto& reference : referencedDataBlocks)
            _cache.releaseReference(reference->id, reference);
    }
    _referencedDataBlocksMap.clear();
}

void OsmAnd::CachingRoadLocator_P::clearCacheConditional(
    const std::function<bool (const std::shared_ptr<const ObfRoutingSectionReader::DataBlock>& dataBlock)> shouldRemoveFromCacheFunctor)
{
    QMutexLocker scopedLocker(&_referencedDataBlocksMapMutex);

    auto itReferencedDataBlocks = mutableIteratorOf(_referencedDataBlocksMap);
    while (itReferencedDataBlocks.hasNext())
    {
        auto& referencedDataBlocks = itReferencedDataBlocks.next().value();

        if (!shouldRemoveFromCacheFunctor(referencedDataBlocks.first()))
            continue;

        for (auto& reference : referencedDataBlocks)
        {
            if (shouldRemoveFromCacheFunctor(reference))
                _cache.releaseReference(reference->id, reference);
        }
        itReferencedDataBlocks.remove();
    }
}

void OsmAnd::CachingRoadLocator_P::clearCacheInBBox(const AreaI bbox31, const bool checkAlsoIntersection)
{
    clearCacheConditional(
        [bbox31, checkAlsoIntersection]
        (const std::shared_ptr<const ObfRoutingSectionReader::DataBlock>& dataBlock) -> bool
        {
            if (bbox31.contains(dataBlock->area31) || dataBlock->area31.contains(bbox31))
                return true;
            if (checkAlsoIntersection && bbox31.intersects(dataBlock->area31))
                return true;
            return false;
        });
}

void OsmAnd::CachingRoadLocator_P::clearCacheInTiles(
    const QSet<TileId>& tiles,
    const ZoomLevel zoomLevel,
    const bool checkAlsoIntersection)
{
    for (const auto& tileId : constOf(tiles))
        clearCacheInBBox(Utilities::tileBoundingBox31(tileId, zoomLevel), checkAlsoIntersection);
}

void OsmAnd::CachingRoadLocator_P::clearCacheNotInBBox(const AreaI bbox31, const bool checkAlsoIntersection)
{
    clearCacheConditional(
        [bbox31, checkAlsoIntersection]
        (const std::shared_ptr<const ObfRoutingSectionReader::DataBlock>& dataBlock) -> bool
        {
            if (bbox31.contains(dataBlock->area31) || dataBlock->area31.contains(bbox31))
                return false;
            if (checkAlsoIntersection && bbox31.intersects(dataBlock->area31))
                return false;
            return true;
        });
}

void OsmAnd::CachingRoadLocator_P::clearCacheNotInTiles(
    const QSet<TileId>& tiles,
    const ZoomLevel zoomLevel,
    const bool checkAlsoIntersection)
{
    clearCacheConditional(
        [tiles, zoomLevel, checkAlsoIntersection]
        (const std::shared_ptr<const ObfRoutingSectionReader::DataBlock>& dataBlock) -> bool
        {
            bool shouldRemove = true;
            for (const auto& tileId : constOf(tiles))
            {
                const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoomLevel);

                if (tileBBox31.contains(dataBlock->area31) || dataBlock->area31.contains(tileBBox31))
                {
                    shouldRemove = false;
                    break;
                }
                if (checkAlsoIntersection && tileBBox31.intersects(dataBlock->area31))
                {
                    shouldRemove = false;
                    break;
                }
            }

            return shouldRemove;
        });
}

OsmAnd::CachingRoadLocator_P::Cache::Cache()
{
}

OsmAnd::CachingRoadLocator_P::Cache::~Cache()
{
}
