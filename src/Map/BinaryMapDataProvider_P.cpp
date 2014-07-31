#include "BinaryMapDataProvider_P.h"
#include "BinaryMapDataProvider.h"

//#define OSMAND_PERFORMANCE_METRICS 2
#if !defined(OSMAND_PERFORMANCE_METRICS)
#   define OSMAND_PERFORMANCE_METRICS 0
#endif // !defined(OSMAND_PERFORMANCE_METRICS)

#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "ObfMapSectionInfo.h"
#include "BinaryMapObject.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::BinaryMapDataProvider_P::BinaryMapDataProvider_P(BinaryMapDataProvider* owner_)
    : _dataBlocksCache(new DataBlocksCache(false))
    , _link(new Link(this))
    , owner(owner_)
{
}

OsmAnd::BinaryMapDataProvider_P::~BinaryMapDataProvider_P()
{
    _link->release();
}

bool OsmAnd::BinaryMapDataProvider_P::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    BinaryMapDataProvider_Metrics::Metric_obtainData* const metric_,
    const IQueryController* const queryController)
{
#if OSMAND_PERFORMANCE_METRICS
    BinaryMapDataProvider_Metrics::Metric_obtainData localMetric;
    const auto metric = metric_ ? metric_ : &localMetric;
#else
    const auto metric = metric_;
#endif

    std::shared_ptr<TileEntry> tileEntry;

    for (;;)
    {
        // Try to obtain previous instance of tile
        _tileReferences.obtainOrAllocateEntry(tileEntry, tileId, zoom,
            []
            (const TiledEntriesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom) -> TileEntry*
            {
                return new TileEntry(collection, tileId, zoom);
            });

        // If state is "Undefined", change it to "Loading" and proceed with loading
        if (tileEntry->setStateIf(TileState::Undefined, TileState::Loading))
            break;

        // In case tile entry is being loaded, wait until it will finish loading
        if (tileEntry->getState() == TileState::Loading)
        {
            QReadLocker scopedLcoker(&tileEntry->_loadedConditionLock);

            // If tile is in 'Loading' state, wait until it will become 'Loaded'
            while (tileEntry->getState() != TileState::Loaded)
                REPEAT_UNTIL(tileEntry->_loadedCondition.wait(&tileEntry->_loadedConditionLock));
        }

        // Try to lock tile reference
        outTiledData = tileEntry->_tile.lock();

        // If successfully locked, just return it
        if (outTiledData)
            return true;

        // Otherwise consider this tile entry as expired, remove it from collection (it's safe to do that right now)
        // This will enable creation of new entry on next loop cycle
        _tileReferences.removeEntry(tileId, zoom);
        tileEntry.reset();
    }

    const Stopwatch totalTimeStopwatch(
#if OSMAND_PERFORMANCE_METRICS
        true
#else
        metric != nullptr
#endif // OSMAND_PERFORMANCE_METRICS
        );

    // Obtain OBF data interface
    const Stopwatch obtainObfInterfaceStopwatch(metric != nullptr);
    const auto& dataInterface = owner->obfsCollection->obtainDataInterface();
    if (metric)
        metric->elapsedTimeForObtainingObfInterface += obtainObfInterfaceStopwatch.elapsed();

    // Get bounding box that covers this tile
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

    // Perform read-out
    const Stopwatch totalReadTimeStopwatch(metric != nullptr);
    QList< std::shared_ptr< const ObfMapSectionReader::DataBlock > > referencedMapDataBlocks;
    QList< std::shared_ptr<const Model::BinaryMapObject> > referencedMapObjects;
    QList< proper::shared_future< std::shared_ptr<const Model::BinaryMapObject> > > futureReferencedMapObjects;
    QList< std::shared_ptr<const Model::BinaryMapObject> > loadedMapObjects;
    QSet< uint64_t > loadedSharedMapObjects;
    MapFoundationType tileFoundation;
    dataInterface->loadMapObjects(
        &loadedMapObjects,
        &tileFoundation,
        zoom,
        &tileBBox31,
        [this, zoom, &referencedMapObjects, &futureReferencedMapObjects, &loadedSharedMapObjects, tileBBox31, metric]
        (const std::shared_ptr<const ObfMapSectionInfo>& section, const uint64_t id, const AreaI& bbox, const ZoomLevel firstZoomLevel, const ZoomLevel lastZoomLevel) -> bool
        {
            const Stopwatch objectsFilteringStopwatch(metric != nullptr);

            // This map object may be shared only in case it crosses bounds of a tile
            const auto canNotBeShared = tileBBox31.contains(bbox);

            // If map object can not be shared, just read it
            if (canNotBeShared)
            {
                if (metric)
                    metric->elapsedTimeForObjectsFiltering += objectsFilteringStopwatch.elapsed();

                return true;
            }

            // Otherwise, this map object can be shared, so it should be checked for
            // being present in shared mapObjects storage, or be reserved there
            std::shared_ptr<const Model::BinaryMapObject> sharedMapObjectReference;
            proper::shared_future< std::shared_ptr<const Model::BinaryMapObject> > futureSharedMapObjectReference;
            if (_sharedMapObjects.obtainReferenceOrFutureReferenceOrMakePromise(id, zoom, Utilities::enumerateZoomLevels(firstZoomLevel, lastZoomLevel), sharedMapObjectReference, futureSharedMapObjectReference))
            {
                if (sharedMapObjectReference)
                {
                    // If map object is already in shared objects cache and is available, use that one
                    referencedMapObjects.push_back(qMove(sharedMapObjectReference));
                }
                else
                {
                    futureReferencedMapObjects.push_back(qMove(futureSharedMapObjectReference));
                }

                if (metric)
                    metric->elapsedTimeForObjectsFiltering += objectsFilteringStopwatch.elapsed();

                return false;
            }

            // This map object was reserved, and is going to be shared, but needs to be loaded
            loadedSharedMapObjects.insert(id);
            return true;
        },
        _dataBlocksCache.get(),
        &referencedMapDataBlocks,
        nullptr,// query controller
        metric ? &metric->loadMapObjectsMetric : nullptr);

    // Process loaded-and-shared map objects
    for (auto& mapObject : loadedMapObjects)
    {
        // Check if this map object is shared
        if (!loadedSharedMapObjects.contains(mapObject->id))
            continue;

        // Add unique map object under lock to all zoom levels, for which this map object is valid
        assert(mapObject->level);
        _sharedMapObjects.fulfilPromiseAndReference(
            mapObject->id,
            Utilities::enumerateZoomLevels(mapObject->level->minZoom, mapObject->level->maxZoom),
            mapObject);
    }

    for (auto& futureMapObject : futureReferencedMapObjects)
    {
        auto mapObject = futureMapObject.get();

        referencedMapObjects.push_back(qMove(mapObject));
    }

    if (metric)
        metric->elapsedTimeForRead += totalReadTimeStopwatch.elapsed();

    // Prepare data for the tile
    const auto sharedMapObjectsCount = referencedMapObjects.size() + loadedSharedMapObjects.size();
    const auto allMapObjects = loadedMapObjects + referencedMapObjects;

    // Create tile
    const std::shared_ptr<BinaryMapDataTile> newTile(new BinaryMapDataTile(
        _dataBlocksCache,
        referencedMapDataBlocks,
        tileFoundation,
        allMapObjects,
        tileId,
        zoom));
    newTile->_p->_weakLink = _link->getWeak();
    newTile->_p->_refEntry = tileEntry;

    // Publish new tile
    outTiledData = newTile;

    // Store weak reference to new tile and mark it as 'Loaded'
    tileEntry->_tile = newTile;
    tileEntry->setState(TileState::Loaded);

    // Notify that tile has been loaded
    {
        QWriteLocker scopedLcoker(&tileEntry->_loadedConditionLock);
        tileEntry->_loadedCondition.wakeAll();
    }

    if (metric)
    {
        metric->elapsedTime += totalTimeStopwatch.elapsed();
        metric->objectsCount += allMapObjects.size();
        metric->uniqueObjectsCount += allMapObjects.size() - sharedMapObjectsCount;
        metric->sharedObjectsCount += sharedMapObjectsCount;
    }

#if OSMAND_PERFORMANCE_METRICS
#if OSMAND_PERFORMANCE_METRICS <= 1
    LogPrintf(LogSeverityLevel::Info,
        "%d map objects (%d unique, %d shared) read from %dx%d@%d in %fs",
        allMapObjects.size(),
        allMapObjects.size() - sharedMapObjectsCount,
        sharedMapObjectsCount,
        tileId.x,
        tileId.y,
        zoom,
        totalTimeStopwatch.elapsed());
#else
    LogPrintf(LogSeverityLevel::Info,
        "%d map objects (%d unique, %d shared) read from %dx%d@%d in %fs:\n%s",
        allMapObjects.size(),
        allMapObjects.size() - sharedMapObjectsCount,
        sharedMapObjectsCount,
        tileId.x,
        tileId.y,
        zoom,
        totalTimeStopwatch.elapsed(),
        qPrintable(metric ? metric->toString(QLatin1String("\t - ")) : QLatin1String("(null)")));
#endif // OSMAND_PERFORMANCE_METRICS <= 1
#endif // OSMAND_PERFORMANCE_METRICS

    return true;
}

OsmAnd::BinaryMapDataTile_P::BinaryMapDataTile_P(BinaryMapDataTile* owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapDataTile_P::~BinaryMapDataTile_P()
{
}

void OsmAnd::BinaryMapDataTile_P::cleanup()
{
    // Dereference DataBlocks from cache
    if (const auto dataBlocksCache = owner->dataBlocksCache.lock())
    {
        for (auto& block : _referencedDataBlocks)
            dataBlocksCache->releaseReference(block->id, owner->zoom, block);
        _referencedDataBlocks.clear();
    }

    // Remove tile reference from collection. All checks here does not matter,
    // since entry->tile reference is already expired (execution is already in destructor of OfflineMapDataTile!)
    if (const auto entry = _refEntry.lock())
    {
        if (const auto link = entry->link.lock())
            link->collection.removeEntry(entry->tileId, entry->zoom);
    }

    // Dereference shared map objects
    if (const auto link = _weakLink.lock())
    {
        // Get bounding box that covers this tile
        const auto tileBBox31 = Utilities::tileBoundingBox31(owner->tileId, owner->zoom);

        for (auto& mapObject : _mapObjects)
        {
            // Skip all map objects that can not be shared
            const auto canNotBeShared = tileBBox31.contains(mapObject->bbox31);
            if (canNotBeShared)
                continue;

            link->_sharedMapObjects.releaseReference(mapObject->id, owner->zoom, mapObject);
        }
    }
    _mapObjects.clear();
}

OsmAnd::BinaryMapDataProvider_P::DataBlocksCache::DataBlocksCache(const bool cacheTileInnerDataBlocks_)
    : cacheTileInnerDataBlocks(cacheTileInnerDataBlocks_)
{
}

OsmAnd::BinaryMapDataProvider_P::DataBlocksCache::~DataBlocksCache()
{
}

bool OsmAnd::BinaryMapDataProvider_P::DataBlocksCache::shouldCacheBlock(const DataBlockId id, const AreaI blockBBox31, const AreaI* const queryArea31 /*= nullptr*/) const
{
    if (!queryArea31)
        return ObfMapSectionReader::DataBlocksCache::shouldCacheBlock(id, blockBBox31, queryArea31);

    if (queryArea31->contains(blockBBox31))
        return cacheTileInnerDataBlocks;
    return true;
}
