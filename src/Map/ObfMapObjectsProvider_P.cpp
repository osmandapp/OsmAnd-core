#include "ObfMapObjectsProvider_P.h"
#include "ObfMapObjectsProvider.h"

//#define OSMAND_PERFORMANCE_METRICS 2
#if !defined(OSMAND_PERFORMANCE_METRICS)
#   define OSMAND_PERFORMANCE_METRICS 0
#endif // !defined(OSMAND_PERFORMANCE_METRICS)

#include "MapDataProviderHelpers.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "ObfMapSectionInfo.h"
#include "ObfMapSectionReader_Metrics.h"
#include "BinaryMapObject.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionReader_Metrics.h"
#include "Road.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"

#define ENLARGE_QUERY_BBOX_MIN_ZOOM 15
#define ENLARGE_QUERY_BBOX_METERS 100
#define COMPARE_OBF_OBJECTS_MAX_ZOOM 12

OsmAnd::ObfMapObjectsProvider_P::ObfMapObjectsProvider_P(ObfMapObjectsProvider* owner_)
    : _binaryMapObjectsDataBlocksCache(new BinaryMapObjectsDataBlocksCache(false))
    , _roadsDataBlocksCache(new RoadsDataBlocksCache(false))
    , _link(new Link(this))
    , owner(owner_)
{
}

OsmAnd::ObfMapObjectsProvider_P::~ObfMapObjectsProvider_P()
{
    _link->release();
}

bool OsmAnd::ObfMapObjectsProvider_P::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    if (pOutMetric)
    {
        if (!pOutMetric->get() || !dynamic_cast<ObfMapObjectsProvider_Metrics::Metric_obtainData*>(pOutMetric->get()))
            pOutMetric->reset(new ObfMapObjectsProvider_Metrics::Metric_obtainData());
        else
            pOutMetric->get()->reset();
    }

    std::shared_ptr<ObfMapObjectsProvider::Data> data;
    const auto result = obtainTiledObfMapObjects(
        MapDataProviderHelpers::castRequest<ObfMapObjectsProvider::Request>(request),
        data,
        pOutMetric ? static_cast<ObfMapObjectsProvider_Metrics::Metric_obtainData*>(pOutMetric->get()) : nullptr);
    outData = data;

    return result;
}

bool OsmAnd::ObfMapObjectsProvider_P::obtainTiledObfMapObjects(
    const ObfMapObjectsProvider::Request& request,
    std::shared_ptr<ObfMapObjectsProvider::Data>& outMapObjects,
    ObfMapObjectsProvider_Metrics::Metric_obtainData* const metric_)
{
#if OSMAND_PERFORMANCE_METRICS
    ObfMapObjectsProvider_Metrics::Metric_obtainData localMetric;
    const auto metric = metric_ ? metric_ : &localMetric;
#else
    const auto metric = metric_;
#endif

    std::shared_ptr<TileEntry> tileEntry;
    std::shared_ptr<TileSharedEntry> coastlineTileEntry;
    std::shared_ptr<ObfMapObjectsProvider::Data> coastlineTile = nullptr;
    TileId overscaledTileId;
    
    if (request.zoom > _coastlineZoom)
    {
        for (;;)
        {
            int zoomShift = request.zoom - _coastlineZoom;
            overscaledTileId = Utilities::getTileIdOverscaledByZoomShift(request.tileId, zoomShift);
            _coastlineReferences.obtainOrAllocateEntry(coastlineTileEntry, overscaledTileId, _coastlineZoom,
                []
                (const TiledEntriesCollection<TileSharedEntry>& collection, const TileId tileId, const ZoomLevel zoom) -> TileSharedEntry*
                {
                    return new TileSharedEntry(collection, tileId, zoom);
                });
            
            // If state is "Undefined", change it to "Loading" and proceed with loading
            if (coastlineTileEntry->setStateIf(TileState::Undefined, TileState::Loading))
                break;
            
            // In case tile entry is being loaded, wait until it will finish loading
            if (coastlineTileEntry->getState() == TileState::Loading)
            {
                QReadLocker scopedLcoker(&coastlineTileEntry->loadedConditionLock);

                // If tile is in 'Loading' state, wait until it will become 'Loaded'
                while (coastlineTileEntry->getState() != TileState::Loaded)
                    REPEAT_UNTIL(coastlineTileEntry->loadedCondition.wait(&coastlineTileEntry->loadedConditionLock));
            }
            
            // Try to lock coastline tile reference
            coastlineTile = coastlineTileEntry->dataSharedRef;
            
            // Otherwise consider this coastline tile entry as expired
            if (!coastlineTile)
            {
                _coastlineReferences.removeEntry(overscaledTileId, _coastlineZoom);
                coastlineTileEntry.reset();
            }
            else
            {
                break;
            }
        }
    }

    for (;;)
    {
        // Try to obtain previous instance of tile
        _tileReferences.obtainOrAllocateEntry(tileEntry, request.tileId, request.zoom,
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
            QReadLocker scopedLcoker(&tileEntry->loadedConditionLock);

            // If tile is in 'Loading' state, wait until it will become 'Loaded'
            while (tileEntry->getState() != TileState::Loaded)
                REPEAT_UNTIL(tileEntry->loadedCondition.wait(&tileEntry->loadedConditionLock));
        }

        // Try to lock tile reference
        outMapObjects = tileEntry->dataWeakRef.lock();

        // If successfully locked, just return it
        if (outMapObjects)
            return true;

        // Otherwise consider this tile entry as expired, remove it from collection (it's safe to do that right now)
        // This will enable creation of new entry on next loop cycle
        _tileReferences.removeEntry(request.tileId, request.zoom);
        tileEntry.reset();
    }

    const Stopwatch totalTimeStopwatch(
#if OSMAND_PERFORMANCE_METRICS
        true
#else
        metric != nullptr
#endif // OSMAND_PERFORMANCE_METRICS
        );

    // Get bounding box that covers this tile
    const auto zoom = request.zoom;
    const auto originalTileBBox31 = Utilities::tileBoundingBox31(request.tileId, zoom);
    AreaI tileBBox31;
    if (zoom >= ENLARGE_QUERY_BBOX_MIN_ZOOM)
    {
        // Fix showing deleted objects in live updates https://github.com/osmandapp/OsmAnd/issues/14920#issuecomment-1538488529
        const auto enlargeDeltaX = Utilities::metersToX31(ENLARGE_QUERY_BBOX_METERS);
        const auto enlargeDeltaY = Utilities::metersToY31(ENLARGE_QUERY_BBOX_METERS);
        tileBBox31 = originalTileBBox31.getEnlargedBy(PointI(enlargeDeltaX, enlargeDeltaY));
    }
    else
    {
        tileBBox31 = originalTileBBox31;
    }

    // Obtain OBF data interface
    const Stopwatch obtainObfInterfaceStopwatch(metric != nullptr);
    const auto& dataInterface = owner->obfsCollection->obtainDataInterface(
        &tileBBox31,
        request.zoom,
        request.zoom,
        ObfDataTypesMask().set(ObfDataType::Map).set(ObfDataType::Routing));
    if (metric)
        metric->elapsedTimeForObtainingObfInterface += obtainObfInterfaceStopwatch.elapsed();

    // Perform read-out
    const Stopwatch totalReadTimeStopwatch(metric != nullptr);

    // General:
    auto tileSurfaceType = MapSurfaceType::Undefined;
    auto coastlineTileSurfaceType = MapSurfaceType::Undefined;

    // BinaryMapObjects:
    QList< std::shared_ptr< const ObfMapSectionReader::DataBlock > > referencedBinaryMapObjectsDataBlocks;
    QList< std::shared_ptr<const BinaryMapObject> > referencedBinaryMapObjects;
    QList< proper::shared_future< std::shared_ptr<const BinaryMapObject> > > futureReferencedBinaryMapObjects;
    QList< std::shared_ptr<const BinaryMapObject> > loadedBinaryMapObjects;
    QList< std::shared_ptr<const BinaryMapObject> > loadedSharedBinaryMapObjects;
    QHash< ObfObjectId, QSet<ObfMapSectionReader::DataBlockId> > allLoadedBinaryMapObjectIds;
    QHash< UniqueBinaryMapObjectId, SmartPOD<unsigned int, 0u> > allLoadedBinaryMapObjectsCounters;
    QHash< UniqueBinaryMapObjectId, SmartPOD<unsigned int, 0u> > loadedSharedBinaryMapObjectsCounters;
    QHash< UniqueBinaryMapObjectId, SmartPOD<unsigned int, 0u> > loadedNonSharedBinaryMapObjectsCounters;
    const auto binaryMapObjectsFilteringFunctor =
        [this,
            &referencedBinaryMapObjects,
            &futureReferencedBinaryMapObjects,
            &loadedSharedBinaryMapObjectsCounters,
            &loadedNonSharedBinaryMapObjectsCounters,
            &allLoadedBinaryMapObjectsCounters,
            &allLoadedBinaryMapObjectIds,
            tileBBox31,
            zoom,
            metric]
        (const std::shared_ptr<const ObfMapSectionInfo>& section,
            const ObfMapSectionReader::DataBlockId& blockId,
            const ObfObjectId objectId,
            const AreaI& bbox,
            const ZoomLevel firstZoomLevel,
            const ZoomLevel lastZoomLevel,
            const ZoomLevel requestedZoom) -> bool
        {
            const Stopwatch objectsFilteringStopwatch(metric != nullptr);

            // Combine object's block id and object id itself,
            // because there can be two object with same ObfObjectId, similar content
            // but different block
            const UniqueBinaryMapObjectId id(blockId, objectId);            

            // Check if this object was not loaded before
            unsigned int& totalLoadCount = allLoadedBinaryMapObjectsCounters[id];
            if (totalLoadCount > 0u)
                return false;
            totalLoadCount++;

            auto& blockIds = allLoadedBinaryMapObjectIds[objectId];
            blockIds.insert(blockId);

            // This map object may be shared only in case it crosses bounds of a tile
            const auto canNotBeShared = requestedZoom == zoom && tileBBox31.contains(bbox);

            // If map object can not be shared, just read it
            if (canNotBeShared)
            {
                if (metric)
                    metric->elapsedTimeForObjectsFiltering += objectsFilteringStopwatch.elapsed();

                unsigned int& loadCount = loadedNonSharedBinaryMapObjectsCounters[id];
                if (loadCount > 0u)
                    return false;
                loadCount++;
                totalLoadCount++;

                return true;
            }

            // Otherwise, this map object can be shared, so it should be checked for
            // being present in shared mapObjects storage, or be reserved there
            std::shared_ptr<const BinaryMapObject> sharedMapObjectReference;
            proper::shared_future< std::shared_ptr<const BinaryMapObject> > futureSharedMapObjectReference;
            const auto existsOrWillBeInFuture = _sharedBinaryMapObjects.obtainReferenceOrFutureReferenceOrMakePromise(
                id,
                requestedZoom,
                Utilities::enumerateZoomLevels(firstZoomLevel, lastZoomLevel),
                sharedMapObjectReference,
                futureSharedMapObjectReference);
            if (existsOrWillBeInFuture)
            {
                if (sharedMapObjectReference)
                {
                    // If map object is already in shared objects cache and is available, use that one
                    referencedBinaryMapObjects.push_back(qMove(sharedMapObjectReference));
                }
                else
                {
                    futureReferencedBinaryMapObjects.push_back(qMove(futureSharedMapObjectReference));
                }

                if (metric)
                    metric->elapsedTimeForObjectsFiltering += objectsFilteringStopwatch.elapsed();

                return false;
            }

            // This map object was reserved, and is going to be shared, but needs to be loaded (and it was promised)
            unsigned int& loadCount = loadedSharedBinaryMapObjectsCounters[id];
            if (loadCount > 0u)
                return false;
            loadCount += 1;
            totalLoadCount++;

            return true;
        };

    // Roads:
    QList< std::shared_ptr< const ObfRoutingSectionReader::DataBlock > > referencedRoadsDataBlocks;
    QList< std::shared_ptr<const Road> > referencedRoads;
    QList< proper::shared_future< std::shared_ptr<const Road> > > futureReferencedRoads;
    QList< std::shared_ptr<const Road> > loadedRoads;
    QList< std::shared_ptr<const Road> > loadedSharedRoads;
    QHash< ObfObjectId, QSet<ObfRoutingSectionReader::DataBlockId> > allLoadedRoadsIds;
    QHash< UniqueRoadId, SmartPOD<unsigned int, 0u> > allLoadedRoadsCounters;
    QHash< UniqueRoadId, SmartPOD<unsigned int, 0u> > loadedSharedRoadsCounters;
    QHash< UniqueRoadId, SmartPOD<unsigned int, 0u> > loadedNonSharedRoadsCounters;
    const auto roadsFilteringFunctor =
        [this,
            &referencedRoads,
            &futureReferencedRoads,
            &loadedSharedRoadsCounters,
            &loadedNonSharedRoadsCounters,
            &allLoadedRoadsCounters,
            &allLoadedBinaryMapObjectIds,
            &allLoadedRoadsIds,
            tileBBox31,
            metric]
        (const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            const ObfRoutingSectionReader::DataBlockId& blockId,
            const ObfObjectId roadId,
            const AreaI& bbox) -> bool
        {
            const Stopwatch objectsFilteringStopwatch(metric != nullptr);

            // Combine road's block id and road id itself,
            // because there can be two roads with same ObfObjectId, similar content
            // but different block
            const UniqueRoadId id(blockId, roadId);

            // Ensure that binary map object with same ID was not yet loaded
            if (allLoadedBinaryMapObjectIds.constFind(roadId) != allLoadedBinaryMapObjectIds.cend())
                return false;

            // Check if this road was not loaded before
            unsigned int& totalLoadCount = allLoadedRoadsCounters[id];
            if (totalLoadCount > 0u)
                return false;
            totalLoadCount++;

            auto& blockIds = allLoadedRoadsIds[roadId];
            blockIds.insert(blockId);

            // This road may be shared only in case it crosses bounds of a tile
            const auto canNotBeShared = tileBBox31.contains(bbox);

            // If road can not be shared, just read it
            if (canNotBeShared)
            {
                if (metric)
                    metric->elapsedTimeForObjectsFiltering += objectsFilteringStopwatch.elapsed();

                unsigned int& loadCount = loadedNonSharedRoadsCounters[id];
                if (loadCount > 0u)
                    return false;
                loadCount++;
                totalLoadCount++;

                return true;
            }

            // Otherwise, this road can be shared, so it should be checked for
            // being present in shared roads storage, or be reserved there
            std::shared_ptr<const Road> sharedRoadReference;
            proper::shared_future< std::shared_ptr<const Road> > futureSharedRoadReference;
            const auto existsOrWillBeInFuture = _sharedRoads.obtainReferenceOrFutureReferenceOrMakePromise(
                id,
                sharedRoadReference,
                futureSharedRoadReference);
            if (existsOrWillBeInFuture)
            {
                if (sharedRoadReference)
                {
                    // If road is already in shared roads cache and is available, use that one
                    referencedRoads.push_back(qMove(sharedRoadReference));
                }
                else
                {
                    futureReferencedRoads.push_back(qMove(futureSharedRoadReference));
                }

                if (metric)
                    metric->elapsedTimeForObjectsFiltering += objectsFilteringStopwatch.elapsed();

                return false;
            }

            // This road was reserved, and is going to be shared, but needs to be loaded (and it was promised)
            unsigned int& loadCount = loadedSharedRoadsCounters[id];
            if (loadCount > 0u)
                return false;
            loadCount += 1;
            totalLoadCount++;

            return true;
        };

    if (owner->mode == ObfMapObjectsProvider::Mode::OnlyBinaryMapObjects)
    {
        Ref<ObfMapSectionReader_Metrics::Metric_loadMapObjects> loadMapObjectsMetric;
        if (metric)
        {
            loadMapObjectsMetric = new ObfMapSectionReader_Metrics::Metric_loadMapObjects();
            metric->submetrics.push_back(loadMapObjectsMetric);
        }

        dataInterface->loadBinaryMapObjects(
            &loadedBinaryMapObjects,
            &tileSurfaceType,
            request.zoom,
            &tileBBox31,
            binaryMapObjectsFilteringFunctor,
            _binaryMapObjectsDataBlocksCache.get(),
            &referencedBinaryMapObjectsDataBlocks,
            nullptr,// query queryController
            loadMapObjectsMetric.get());
    }
    else if (owner->mode == ObfMapObjectsProvider::Mode::OnlyRoads)
    {
        Ref<ObfRoutingSectionReader_Metrics::Metric_loadRoads> loadRoadsMetric;
        if (metric)
        {
            loadRoadsMetric = new ObfRoutingSectionReader_Metrics::Metric_loadRoads();
            metric->submetrics.push_back(loadRoadsMetric);
        }

        dataInterface->loadRoads(
            RoutingDataLevel::Detailed,
            &tileBBox31,
            &loadedSharedRoads,
            roadsFilteringFunctor,
            nullptr,// visitor
            _roadsDataBlocksCache.get(),
            &referencedRoadsDataBlocks,
            nullptr,// query queryController
            loadRoadsMetric.get());
    }
    else if (owner->mode == ObfMapObjectsProvider::Mode::BinaryMapObjectsAndRoads)
    {
        Ref<ObfMapSectionReader_Metrics::Metric_loadMapObjects> loadMapObjectsMetric;
        if (metric)
        {
            loadMapObjectsMetric = new ObfMapSectionReader_Metrics::Metric_loadMapObjects();
            metric->submetrics.push_back(loadMapObjectsMetric);
        }
        Ref<ObfRoutingSectionReader_Metrics::Metric_loadRoads> loadRoadsMetric;
        if (metric)
        {
            loadRoadsMetric = new ObfRoutingSectionReader_Metrics::Metric_loadRoads();
            metric->submetrics.push_back(loadRoadsMetric);
        }

        dataInterface->loadMapObjects(
            &loadedBinaryMapObjects,
            &loadedRoads,
            &tileSurfaceType,
            request.zoom,
            &tileBBox31,
            binaryMapObjectsFilteringFunctor,
            _binaryMapObjectsDataBlocksCache.get(),
            &referencedBinaryMapObjectsDataBlocks,
            roadsFilteringFunctor,
            _roadsDataBlocksCache.get(),
            &referencedRoadsDataBlocks,
            nullptr,// query queryController
            loadMapObjectsMetric.get(),
            loadRoadsMetric.get());
    }
    
    QList< std::shared_ptr<const BinaryMapObject> > loadedCoastlineMapObjects;
    if (request.zoom > _coastlineZoom && !coastlineTile)
    {
        const auto coastlineTileBBox31 = Utilities::tileBoundingBox31(overscaledTileId, _coastlineZoom);
        Ref<ObfMapSectionReader_Metrics::Metric_loadMapObjects> loadMapObjectsMetric;
        if (metric)
        {
            loadMapObjectsMetric = new ObfMapSectionReader_Metrics::Metric_loadMapObjects();
            metric->submetrics.push_back(loadMapObjectsMetric);
        }
        dataInterface->loadBinaryMapObjects(
            &loadedCoastlineMapObjects,
            &coastlineTileSurfaceType,
            _coastlineZoom,
            &coastlineTileBBox31,
            nullptr,
            nullptr,
            nullptr,
            nullptr,// query queryController
            nullptr,
            true// coastlineOnly
        );
    }

    // Process loaded-and-shared map objects (both binary and roads)
    for (auto& binaryMapObject : loadedBinaryMapObjects)
    {
        const UniqueBinaryMapObjectId id(binaryMapObject->blockId, binaryMapObject->id);
        // Check if this binary map object is shared (since only shared produce promises)
        if (!loadedSharedBinaryMapObjectsCounters.contains(id))
            continue;

        // Add unique binary map object under lock to all zoom levels, for which this map object is valid
        _sharedBinaryMapObjects.fulfilPromiseAndReference(
            id,
            Utilities::enumerateZoomLevels(binaryMapObject->level->minZoom, binaryMapObject->level->maxZoom),
            binaryMapObject);
        loadedSharedBinaryMapObjects.push_back(binaryMapObject);
    }
    for (auto& road : loadedRoads)
    {
        const UniqueRoadId id(road->blockId, road->id);
        // Check if this map object is shared (since only shared produce promises)
        if (!loadedSharedRoadsCounters.contains(id))
            continue;

        // Add unique map object under lock to all zoom levels, for which this map object is valid
        _sharedRoads.fulfilPromiseAndReference(
            id,
            road);
        loadedSharedRoads.push_back(road);
    }

    int failCounter = 0;
    // Request all future map objects (both binary and roads)
    for (auto& futureBinaryMapObject : futureReferencedBinaryMapObjects)
    {
        auto timeout = proper::chrono::system_clock::now() + proper::chrono::seconds(3);
        if (proper::future_status::ready == futureBinaryMapObject.wait_until(timeout))
        {
            auto binaryMapObject = futureBinaryMapObject.get();
            referencedBinaryMapObjects.push_back(qMove(binaryMapObject));
        }
        else
        {
            failCounter++;
            LogPrintf(LogSeverityLevel::Error, "Get futureBinaryMapObject timeout exceed = %d", failCounter);
        }
        if (failCounter >= 3)
        {
            LogPrintf(LogSeverityLevel::Error, "Get futureBinaryMapObjects timeout exceed");
            break;
        }
    }

    for (auto& futureRoad : futureReferencedRoads)
    {
        auto timeout = proper::chrono::system_clock::now() + proper::chrono::seconds(3);
        if (proper::future_status::ready == futureRoad.wait_until(timeout))
        {
            auto road = futureRoad.get();
            referencedRoads.push_back(qMove(road));
        }
        else
        {
            failCounter++;
            LogPrintf(LogSeverityLevel::Error, "Get futureRoad timeout exceed = %d", failCounter);
        }
        if (failCounter >= 3)
        {
            LogPrintf(LogSeverityLevel::Error, "Get futureRoads timeout exceed");
            break;
        }
    }

    if (metric)
        metric->elapsedTimeForRead += totalReadTimeStopwatch.elapsed();
    
    if (request.zoom > _coastlineZoom && !coastlineTile)
    {
        QList< std::shared_ptr<const MapObject> > coastlineMapObjects;
        coastlineMapObjects.reserve(loadedCoastlineMapObjects.size());
        for (const auto& coastline : constOf(loadedCoastlineMapObjects))
            coastlineMapObjects.push_back(coastline);
        const std::shared_ptr<IMapObjectsProvider::Data> newCoastlineTile(new IMapObjectsProvider::Data(
            overscaledTileId,
            _coastlineZoom,
            coastlineTileSurfaceType,
            coastlineMapObjects,
            nullptr));
        coastlineTile = newCoastlineTile;
        coastlineTileEntry->dataSharedRef = coastlineTile;
        coastlineTileEntry->setState(TileState::Loaded);
       
        // Notify that tile has been loaded
        {
            QWriteLocker scopedLcoker(&coastlineTileEntry->loadedConditionLock);
            coastlineTileEntry->loadedCondition.wakeAll();
        }
    }
    //int costlineTileSize = coastlineTile ? coastlineTile->mapObjects.size() : 0;

    // Prepare data for the tile
    const auto sharedMapObjectsCount =
        (referencedBinaryMapObjects.size() + loadedSharedBinaryMapObjectsCounters.size()) +
        (referencedRoads.size() + loadedSharedRoadsCounters.size());
    const auto allMapObjectsCount =
        (loadedBinaryMapObjects.size() + referencedBinaryMapObjects.size()) +
        (loadedRoads.size() + referencedRoads.size());
    QList< std::shared_ptr<const MapObject> > allMapObjects;
    allMapObjects.reserve(allMapObjectsCount);

    QHash< ObfObjectId, QList< std::shared_ptr<const MapObject> > > duplicatedMapObjects; 
    const auto allBinaryMapObjects = loadedBinaryMapObjects + referencedBinaryMapObjects;
    const auto allRoads = loadedRoads + referencedRoads;
    for (const auto& binaryMapObject : constOf(allBinaryMapObjects))
    {   
        const auto id = binaryMapObject->id;
        if (allLoadedBinaryMapObjectIds[id].size() > 1)
        {
            // Save objects with duplicated ids for separate processing
            auto& duplicates = duplicatedMapObjects[id];
            duplicates.push_back(binaryMapObject);
        }
        else
        {
            allMapObjects.push_back(binaryMapObject);
        }
    }
    for (const auto& road : constOf(allRoads))
    {   
        const auto id = road->id;
        if (allLoadedRoadsIds[id].size() > 1)
        {
            // Save roads with duplicated ids for separate processing
            auto& duplicates = duplicatedMapObjects[id];
            duplicates.push_back(road);
        }
        else
        {
            allMapObjects.push_back(road);
        }
    }

    // Process duplicated map objects. If zoom > 12, choose first loaded object.
    // Otherwise, choose object with largest number of points31
    for (auto& duplicates : duplicatedMapObjects.values())
    {
        auto mostPreferredObject = duplicates.at(0);
        if (zoom <= COMPARE_OBF_OBJECTS_MAX_ZOOM)
            for (auto& duplicate : duplicates)
                if (duplicate->points31.size() > mostPreferredObject->points31.size())
                    mostPreferredObject = duplicate;
        allMapObjects.push_back(mostPreferredObject);
    }

    // Create tile
    const std::shared_ptr<IMapObjectsProvider::Data> newTile(new IMapObjectsProvider::Data(
        request.tileId,
        request.zoom,
        tileSurfaceType,
        allMapObjects,
        new RetainableCacheMetadata(
            request.zoom,
            _link,
            tileEntry,
            _binaryMapObjectsDataBlocksCache,
            referencedBinaryMapObjectsDataBlocks,
            referencedBinaryMapObjects + loadedSharedBinaryMapObjects,
            _roadsDataBlocksCache,
            referencedRoadsDataBlocks,
            referencedRoads + loadedSharedRoads)));

    // Publish new tile
    outMapObjects = newTile;
    
    if (coastlineTile)
    {
        for (const std::shared_ptr<const MapObject> & coastline : constOf(coastlineTile->mapObjects))
        {
            auto zoomMin = coastline->getMinZoomLevel();
            auto zoomMax = coastline->getMaxZoomLevel();
            outMapObjects->mapObjects.push_back(coastline);
        }
    }

    // Store weak reference to new tile and mark it as 'Loaded'
    tileEntry->dataWeakRef = newTile;
    tileEntry->setState(TileState::Loaded);

    // Notify that tile has been loaded
    {
        QWriteLocker scopedLcoker(&tileEntry->loadedConditionLock);
        tileEntry->loadedCondition.wakeAll();
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

OsmAnd::ObfMapObjectsProvider_P::BinaryMapObjectsDataBlocksCache::BinaryMapObjectsDataBlocksCache(
    const bool cacheTileInnerDataBlocks_)
    : cacheTileInnerDataBlocks(cacheTileInnerDataBlocks_)
{
}

OsmAnd::ObfMapObjectsProvider_P::BinaryMapObjectsDataBlocksCache::~BinaryMapObjectsDataBlocksCache()
{
}

bool OsmAnd::ObfMapObjectsProvider_P::BinaryMapObjectsDataBlocksCache::shouldCacheBlock(
    const DataBlockId id,
    const AreaI blockBBox31,
    const AreaI* const queryArea31 /*= nullptr*/) const
{
    if (!queryArea31)
        return ObfMapSectionReader::DataBlocksCache::shouldCacheBlock(id, blockBBox31, queryArea31);

    if (queryArea31->contains(blockBBox31))
        return cacheTileInnerDataBlocks;
    return true;
}

OsmAnd::ObfMapObjectsProvider_P::RoadsDataBlocksCache::RoadsDataBlocksCache(
    const bool cacheTileInnerDataBlocks_)
    : cacheTileInnerDataBlocks(cacheTileInnerDataBlocks_)
{
}

OsmAnd::ObfMapObjectsProvider_P::RoadsDataBlocksCache::~RoadsDataBlocksCache()
{
}

bool OsmAnd::ObfMapObjectsProvider_P::RoadsDataBlocksCache::shouldCacheBlock(
    const DataBlockId id,
    const RoutingDataLevel dataLevel,
    const AreaI blockBBox31,
    const AreaI* const queryArea31 /*= nullptr*/) const
{
    if (!queryArea31)
        return ObfRoutingSectionReader::DataBlocksCache::shouldCacheBlock(id, dataLevel, blockBBox31, queryArea31);

    if (queryArea31->contains(blockBBox31))
        return cacheTileInnerDataBlocks;
    return true;
}

OsmAnd::ObfMapObjectsProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const ZoomLevel zoom_,
    const std::shared_ptr<Link>& link,
    const std::shared_ptr<TileEntry>& tileEntry,
    const std::shared_ptr<ObfMapSectionReader::DataBlocksCache>& binaryMapObjectsDataBlocksCache,
    const QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >& referencedBinaryMapObjectsDataBlocks_,
    const QList< std::shared_ptr<const BinaryMapObject> >& referencedBinaryMapObjects_,
    const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache>& roadsDataBlocksCache,
    const QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >& referencedRoadsDataBlocks_,
    const QList< std::shared_ptr<const Road> >& referencedRoads_)
    : zoom(zoom_)
    , weakLink(link->getWeak())
    , tileEntryWeakRef(tileEntry)
    , binaryMapObjectsDataBlocksCacheWeakRef(binaryMapObjectsDataBlocksCache)
    , referencedBinaryMapObjectsDataBlocks(referencedBinaryMapObjectsDataBlocks_)
    , referencedBinaryMapObjects(referencedBinaryMapObjects_)
    , roadsDataBlocksCacheWeakRef(roadsDataBlocksCache)
    , referencedRoadsDataBlocks(referencedRoadsDataBlocks_)
    , referencedRoads(referencedRoads_)
{
}

OsmAnd::ObfMapObjectsProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
    // Dereference DataBlocks from cache
    if (const auto dataBlocksCache = binaryMapObjectsDataBlocksCacheWeakRef.lock())
    {
        for (auto& referencedDataBlock : referencedBinaryMapObjectsDataBlocks)
            dataBlocksCache->releaseReference(referencedDataBlock->id, zoom, referencedDataBlock);
        referencedBinaryMapObjectsDataBlocks.clear();
    }
    if (const auto dataBlocksCache = roadsDataBlocksCacheWeakRef.lock())
    {
        for (auto& referencedDataBlock : referencedRoadsDataBlocks)
            dataBlocksCache->releaseReference(referencedDataBlock->id, referencedDataBlock);
        referencedBinaryMapObjectsDataBlocks.clear();
    }

    // Remove tile reference from collection. All checks here does not matter,
    // since entry->tile reference is already expired (execution is already in destructor of OfflineMapDataTile!)
    if (const auto tileEntry = tileEntryWeakRef.lock())
    {
        if (const auto link = tileEntry->link.lock())
            link->collection.removeEntry(tileEntry->tileId, tileEntry->zoom);
    }

    // Dereference shared map objects
    if (const auto link = weakLink.lock())
    {
        for (auto& referencedBinaryMapObject : referencedBinaryMapObjects)
        {
            const UniqueBinaryMapObjectId id(referencedBinaryMapObject->blockId, referencedBinaryMapObject->id);
            link->_sharedBinaryMapObjects.releaseReference(id, zoom, referencedBinaryMapObject);
        }

        for (auto& referencedRoad : referencedRoads)
        {
            const UniqueRoadId id(referencedRoad->blockId, referencedRoad->id);
            link->_sharedRoads.releaseReference(id, referencedRoad);
        }
    }
    referencedBinaryMapObjects.clear();
    referencedRoads.clear();
}
