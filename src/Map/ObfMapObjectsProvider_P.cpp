#include "ObfMapObjectsProvider_P.h"
#include <QVarLengthArray>
#include "ObfMapObjectsProvider.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSemaphore>

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
#include "MapRendererPerformanceMetrics.h"

#include <list>
#include <string>
#include <cstring>
#include <QMutex>
#include <QWaitCondition>
#if defined(__ANDROID__)
#include <sys/system_properties.h>
#endif

namespace
{
    // zoom-bench: runtime switches, Android property debug.osmand.zb (e.g. "ABC"), read once per process
    bool zbFlag(const char flag)
    {
        static const std::string value = []() -> std::string
        {
            char buffer[96] = { 0 };
#if defined(__ANDROID__)
            __system_property_get("debug.osmand.zb", buffer);
            if (!buffer[0])
                memcpy(buffer, "BCDEGLM", 8);  // zoom-bench: default variants on Android, "-" turns all off
#endif
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, ">>>> 0 ZBFLAGS 0.0: %s", buffer);
            return std::string(buffer);
        }();
        return value.find(flag) != std::string::npos;
    }

    // zoom-bench A: 4x4 metatiles of map objects
    struct ZbMetatile
    {
        QList< std::shared_ptr<const OsmAnd::BinaryMapObject> > buckets[4];
        OsmAnd::MapSurfaceType surfaceType = OsmAnd::MapSurfaceType::Undefined;
        QVector<const OsmAnd::ObfMapSectionInfo*> sections;
        bool ready = false;
    };
    struct ZbMetatileKey
    {
        const void* owner;
        int zoom;
        int mx;
        int my;
        bool operator==(const ZbMetatileKey& o) const
        {
            return owner == o.owner && zoom == o.zoom && mx == o.mx && my == o.my;
        }
    };
    struct ZbMetatileCache
    {
        QMutex mutex;
        QWaitCondition loaded;
        std::list< std::pair<ZbMetatileKey, std::shared_ptr<ZbMetatile>> > lru; // front = newest
        static constexpr size_t kMaxEntries = 16;
    };
    ZbMetatileCache& zbMetatileCache()
    {
        static ZbMetatileCache cache;
        return cache;
    }

    std::shared_ptr<ZbMetatile> zbMetatile(
        const void* owner,
        const std::shared_ptr<OsmAnd::ObfDataInterface>& dataInterface,
        const std::shared_ptr<const OsmAnd::MapPresentationEnvironment>& environment,
        const OsmAnd::ZoomLevel zoom,
        const OsmAnd::TileId tileId)
    {
        const ZbMetatileKey key{ owner, static_cast<int>(zoom), tileId.x >> 1, tileId.y >> 1 };
        auto& cache = zbMetatileCache();
        std::shared_ptr<ZbMetatile> entry;
        {
            QMutexLocker locker(&cache.mutex);
            for (;;)
            {
                auto it = cache.lru.begin();
                for (; it != cache.lru.end(); ++it)
                    if (it->first == key)
                        break;
                if (it == cache.lru.end())
                    break;
                cache.lru.splice(cache.lru.begin(), cache.lru, it);
                if (it->second->ready)
                    return it->second;
                cache.loaded.wait(&cache.mutex);
            }
            entry = std::make_shared<ZbMetatile>();
            cache.lru.emplace_front(key, entry);
            while (cache.lru.size() > ZbMetatileCache::kMaxEntries)
                cache.lru.pop_back();
        }

        const auto zoomShift = OsmAnd::ZoomLevel31 - zoom;
        const int mx0 = key.mx << 1;
        const int my0 = key.my << 1;
        const auto first = OsmAnd::Utilities::tileBoundingBox31(OsmAnd::TileId::fromXY(mx0, my0), zoom);
        const auto last = OsmAnd::Utilities::tileBoundingBox31(OsmAnd::TileId::fromXY(mx0 + 1, my0 + 1), zoom);
        const OsmAnd::AreaI bbox31(first.top(), first.left(), last.bottom(), last.right());

        QList< std::shared_ptr<const OsmAnd::BinaryMapObject> > objects;
        QList< std::shared_ptr<const OsmAnd::Road> > unusedRoads;
        auto sections = &entry->sections;
        const auto filter =
            [sections]
            (const std::shared_ptr<const OsmAnd::ObfMapSectionInfo>& section,
                const OsmAnd::ObfMapSectionReader::DataBlockId& blockId,
                const OsmAnd::ObfObjectId objectId,
                const OsmAnd::AreaI& bbox,
                const OsmAnd::ZoomLevel firstZoomLevel,
                const OsmAnd::ZoomLevel lastZoomLevel,
                const OsmAnd::ZoomLevel requestedZoom) -> bool
            {
                const auto ptr = section.get();
                if (sections->isEmpty() || sections->last() != ptr)
                {
                    if (!sections->contains(ptr))
                        sections->push_back(ptr);
                }
                return true;
            };
        dataInterface->loadMapObjects(
            &objects,
            &unusedRoads,
            &entry->surfaceType,
            environment,
            zoom,
            &bbox31,
            filter,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            true);

        for (int i = 0; i < 4; i++)
            entry->buckets[i].reserve(objects.size() / 2 + 16);
        QSet<uint> grids[4];
        for (const auto& obj : OsmAnd::constOf(objects))
        {
            const auto& b = obj->bbox31;
            int x0 = (b.left() >> zoomShift) - mx0;
            int x1 = (b.right() >> zoomShift) - mx0;
            int y0 = (b.top() >> zoomShift) - my0;
            int y1 = (b.bottom() >> zoomShift) - my0;
            x0 = qBound(0, x0, 1); x1 = qBound(0, x1, 1);
            y0 = qBound(0, y0, 1); y1 = qBound(0, y1, 1);
            // label grid per tile (as readMapObject does per request): one single-point label per 1/128 tile cell
            const bool isLabel = !obj->isArea && obj->attributeIds.size() == 1 && obj->additionalAttributeIds.isEmpty()
                && obj->points31.size() == 1 && obj->captions.size() > 0;
            for (int y = y0; y <= y1; y++)
                for (int x = x0; x <= x1; x++)
                {
                    if (isLabel && zoomShift >= 7)
                    {
                        const auto& p = obj->points31.first();
                        const uint cx = static_cast<uint>(p.x >> (zoomShift - 7)) & 127u;
                        const uint cy = static_cast<uint>(p.y >> (zoomShift - 7)) & 127u;
                        const uint code = ((cy << 25) | (cx << 18) | ((static_cast<uint>(obj->getLayerType()) + 1) << 16))
                            ^ obj->attributeIds.first();
                        auto& grid = grids[y * 2 + x];
                        if (grid.contains(code))
                            continue;
                        grid.insert(code);
                    }
                    entry->buckets[y * 2 + x].push_back(obj);
                }
        }

        {
            qint64 bytes = 0;
            for (const auto& obj : OsmAnd::constOf(objects))
                bytes += 280 + obj->points31.size() * 8
                    + (obj->attributeIds.size() + obj->additionalAttributeIds.size()) * 4 + obj->captions.size() * 64;
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, ">>>> 0 METATILE 0.0: z=%d objects=%d estMB=%.1f",
                static_cast<int>(zoom), objects.size(), bytes / 1048576.0);
        }
        {
            QMutexLocker locker(&cache.mutex);
            entry->ready = true;
            cache.loaded.wakeAll();
        }
        return entry;
    }
}

static QSemaphore readLimiter1(1);
static QSemaphore readLimiter2(2);

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

void OsmAnd::ObfMapObjectsProvider_P::acquireThreadLock()
{
    if (owner->threadsLimit == 1)
        readLimiter1.acquire();
    else if (owner->threadsLimit == 2)
        readLimiter2.acquire();
}

void OsmAnd::ObfMapObjectsProvider_P::releaseThreadLock()
{
    if (owner->threadsLimit == 1)
        readLimiter1.release();
    else if (owner->threadsLimit == 2)
        readLimiter2.release();
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
    ObfMapObjectsProvider_Metrics::Metric_obtainData* const metric)
{
    const auto& queryController = request.queryController;

    if (queryController->isAborted())
        return false;

    const auto empty =
        []
        (const std::shared_ptr<TileSharedEntry>& entry) -> bool
        {
            return !entry->dataSharedRef;
        };

    std::shared_ptr<TileSharedEntry> coastlineTileEntry;
    std::shared_ptr<ObfMapObjectsProvider::Data> coastlineTile = nullptr;
    TileId overscaledTileId;
    
    const Stopwatch allocateTimeStopwatch(OsmAnd::isPerformanceMetricsEnabled());

    if (request.zoom >= _coastlineZoom)
    {
        int zoomShift = request.zoom - _coastlineZoom;
        overscaledTileId = Utilities::getTileIdOverscaledByZoomShift(request.tileId, zoomShift);
        for (;;)
        {
            _coastlineReferences.obtainOrAllocateEntry(
                coastlineTileEntry, overscaledTileId, _coastlineZoom,
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
                QReadLocker scopedLocker(&coastlineTileEntry->loadedConditionLock);

                // If tile is in 'Loading' state, wait until it will become 'Loaded'
                while (coastlineTileEntry->getState() == TileState::Loading)
                    REPEAT_UNTIL(coastlineTileEntry->loadedCondition.wait(&coastlineTileEntry->loadedConditionLock));
            }
            
            // Try to lock coastline tile reference
            if (coastlineTileEntry->getState() == TileState::Loaded)
                coastlineTile = coastlineTileEntry->dataSharedRef;
            
            // Otherwise consider this coastline tile entry as expired
            if (!coastlineTile)
            {
                _coastlineReferences.removeEntry(overscaledTileId, _coastlineZoom, empty);
                coastlineTileEntry.reset();

                if (queryController->isAborted())
                    return false;
            }
            else
            {
                break;
            }
        }
    }

    if (queryController->isAborted())
    {
        if (request.zoom >= _coastlineZoom && !coastlineTile)
        {
            coastlineTileEntry->setState(TileState::Cancelled);
            {
                QWriteLocker scopedLocker(&coastlineTileEntry->loadedConditionLock);
                coastlineTileEntry->loadedCondition.wakeAll();
            }
            _coastlineReferences.removeEntry(overscaledTileId, _coastlineZoom, empty);
            coastlineTileEntry.reset();
        }

        return false;
    }
    
    const float allocationTime = allocateTimeStopwatch.elapsed();
    
    acquireThreadLock();

    const Stopwatch totalTimeStopwatch(metric != nullptr);
    if (OsmAnd::isPerformanceMetricsEnabled())
        OsmAnd::getPerformanceMetrics().readStart(request.tileId);

    // Get bounding box that covers this tile
    const auto zoom = request.zoom;
    const auto tileBBox31 = Utilities::tileBoundingBox31(request.tileId, zoom);

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
    QHash< UniqueBinaryMapObjectId, proper::shared_future< std::shared_ptr<const BinaryMapObject> > > futureReferencedBinaryMapObjects;
    QList< std::shared_ptr<const BinaryMapObject> > loadedBinaryMapObjects;
    QList< std::shared_ptr<const BinaryMapObject> > loadedSharedBinaryMapObjects;
    // zoom-bench v2: object ids repeat only between maps, so only the contributing map sections are tracked
    QVarLengthArray<const ObfMapSectionInfo*, 4> tileMapSections;
    QHash< UniqueBinaryMapObjectId, SmartPOD<unsigned int, 0u> > allLoadedBinaryMapObjectsCounters;
    QList< UniqueBinaryMapObjectId > loadedSharedBinaryMapObjectsCounters;
    const auto binaryMapObjectsFilteringFunctor =
        [this,
            &referencedBinaryMapObjects,
            &futureReferencedBinaryMapObjects,
            &loadedSharedBinaryMapObjectsCounters,
            &allLoadedBinaryMapObjectsCounters,
            &tileMapSections,
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
            // zoom-bench v2: objects are never shared between tiles (canNotBeShared below is always true),
            // and an id repeats only between maps: remember the map section and accept the object.
            {
                const auto sectionPtr = section.get();
                if (tileMapSections.isEmpty() || tileMapSections.last() != sectionPtr)
                {
                    if (!tileMapSections.contains(sectionPtr))
                        tileMapSections.append(sectionPtr);
                }
                return true;
            }
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


            // This map object may be shared only in case it crosses bounds of a tile
            const auto canNotBeShared = true;//requestedZoom == zoom && tileBBox31.contains(bbox);

            // If map object can not be shared, just read it
            if (canNotBeShared)
            {
                if (metric)
                    metric->elapsedTimeForObjectsFiltering += objectsFilteringStopwatch.elapsed();

                totalLoadCount++;

                return true;
            }

            // Otherwise, this map object can be shared, so it should be checked for
            // being present in shared mapObjects storage, or be reserved there
            std::shared_ptr<const BinaryMapObject> sharedMapObjectReference;
            proper::shared_future< std::shared_ptr<const BinaryMapObject> > futureSharedMapObjectReference;
            const bool withPromise = !loadedSharedBinaryMapObjectsCounters.contains(id);
            const auto existsOrWillBeInFuture = _sharedBinaryMapObjects.obtainReferenceOrFutureReferenceOrMakePromise(
                id,
                requestedZoom,
                Utilities::enumerateZoomLevels(firstZoomLevel, lastZoomLevel),
                sharedMapObjectReference,
                futureSharedMapObjectReference,
                withPromise);
            if (existsOrWillBeInFuture)
            {
                if (sharedMapObjectReference)
                {
                    // If map object is already in shared objects cache and is available, use that one
                    referencedBinaryMapObjects.push_back(qMove(sharedMapObjectReference));
                }
                else
                {
                    futureReferencedBinaryMapObjects.insert(id, qMove(futureSharedMapObjectReference));
                }

                if (metric)
                    metric->elapsedTimeForObjectsFiltering += objectsFilteringStopwatch.elapsed();

                return false;
            }

            // This map object was reserved, and is going to be shared, but needs to be loaded (and it was promised)
            if (!withPromise)
                return false;
            loadedSharedBinaryMapObjectsCounters.append(id);
            totalLoadCount++;

            return true;
        };

    // Roads:
    QSet<ObfObjectId> mapObjectIdsForRoads;
    bool mapObjectIdsForRoadsBuilt = false;
    QList< std::shared_ptr< const ObfRoutingSectionReader::DataBlock > > referencedRoadsDataBlocks;
    QList< std::shared_ptr<const Road> > referencedRoads;
    QHash< UniqueRoadId, proper::shared_future< std::shared_ptr<const Road> > > futureReferencedRoads;
    QList< std::shared_ptr<const Road> > loadedRoads;
    QList< std::shared_ptr<const Road> > loadedSharedRoads;
    QHash< ObfObjectId, QSet<ObfRoutingSectionReader::DataBlockId> > allLoadedRoadsIds;
    QHash< UniqueRoadId, SmartPOD<unsigned int, 0u> > allLoadedRoadsCounters;
    QList< UniqueRoadId > loadedSharedRoadsCounters;
    QHash< UniqueRoadId, SmartPOD<unsigned int, 0u> > loadedNonSharedRoadsCounters;
    const auto roadsFilteringFunctor =
        [this,
            &referencedRoads,
            &futureReferencedRoads,
            &loadedSharedRoadsCounters,
            &loadedNonSharedRoadsCounters,
            &allLoadedRoadsCounters,
            &loadedBinaryMapObjects,
            &referencedBinaryMapObjects,
            &mapObjectIdsForRoads,
            &mapObjectIdsForRoadsBuilt,
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
            if (!mapObjectIdsForRoadsBuilt)
            {
                mapObjectIdsForRoadsBuilt = true;
                for (const auto& obj : constOf(loadedBinaryMapObjects))
                    mapObjectIdsForRoads.insert(obj->id);
                for (const auto& obj : constOf(referencedBinaryMapObjects))
                    mapObjectIdsForRoads.insert(obj->id);
            }
            if (mapObjectIdsForRoads.contains(roadId))
                return false;

            // Check if this road was not loaded before
            unsigned int& totalLoadCount = allLoadedRoadsCounters[id];
            if (totalLoadCount > 0u)
                return false;
            totalLoadCount++;

            auto& blockIds = allLoadedRoadsIds[roadId];
            blockIds.insert(blockId);

            // This road may be shared only in case it crosses bounds of a tile
            // Sharing disabled
            const auto canNotBeShared = true;//tileBBox31.contains(bbox);

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
            const bool withPromise = !loadedSharedRoadsCounters.contains(id);
            const auto existsOrWillBeInFuture = _sharedRoads.obtainReferenceOrFutureReferenceOrMakePromise(
                id,
                sharedRoadReference,
                futureSharedRoadReference,
                withPromise);
            if (existsOrWillBeInFuture)
            {
                if (sharedRoadReference)
                {
                    // If road is already in shared roads cache and is available, use that one
                    referencedRoads.push_back(qMove(sharedRoadReference));
                }
                else
                {
                    futureReferencedRoads.insert(id, qMove(futureSharedRoadReference));
                }

                if (metric)
                    metric->elapsedTimeForObjectsFiltering += objectsFilteringStopwatch.elapsed();

                return false;
            }

            // This road was reserved, and is going to be shared, but needs to be loaded (and it was promised)
            if (!withPromise)
                return false;
            loadedSharedRoadsCounters.append(id);
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
            owner->environment,
            request.zoom,
            &tileBBox31,
            binaryMapObjectsFilteringFunctor,
            _binaryMapObjectsDataBlocksCache.get(),
            &referencedBinaryMapObjectsDataBlocks,
            request.queryController,
            loadMapObjectsMetric.get(),
            false,
            true);
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
            request.queryController,
            loadRoadsMetric.get(),
            true);
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

        if (zbFlag('A'))
        {
            // zoom-bench A: this tile's bucket of its 4x4 metatile
            const auto metatile = zbMetatile(this, dataInterface, owner->environment, request.zoom, request.tileId);
            loadedBinaryMapObjects = metatile->buckets[(request.tileId.y & 1) * 2 + (request.tileId.x & 1)];
            tileSurfaceType = metatile->surfaceType;
            for (const auto sectionPtr : constOf(metatile->sections))
                if (!tileMapSections.contains(sectionPtr))
                    tileMapSections.append(sectionPtr);
        }
        else
        dataInterface->loadMapObjects(
            &loadedBinaryMapObjects,
            &loadedRoads,
            &tileSurfaceType,
            owner->environment,
            request.zoom,
            &tileBBox31,
            binaryMapObjectsFilteringFunctor,
            _binaryMapObjectsDataBlocksCache.get(),
            &referencedBinaryMapObjectsDataBlocks,
            roadsFilteringFunctor,
            _roadsDataBlocksCache.get(),
            &referencedRoadsDataBlocks,
            request.queryController,
            loadMapObjectsMetric.get(),
            loadRoadsMetric.get(),
            true);
    }
    
    QList< std::shared_ptr<const BinaryMapObject> > loadedCoastlineMapObjects;
    if (request.zoom >= _coastlineZoom && !coastlineTile)
    {
        auto coastlineTileBBox =
            AreaI64(Utilities::tileBoundingBox31(overscaledTileId, _coastlineZoom));
        coastlineTileBBox.right()++;
        coastlineTileBBox.bottom()++;
        coastlineTileBBox = coastlineTileBBox.getEnlargedBy(coastlineTileBBox.width() / 2);
        coastlineTileBBox.right()--;
        coastlineTileBBox.bottom()--;
        AreaI coastlineTileBBox31(
            static_cast<int>(qMax(coastlineTileBBox.top(), static_cast<int64_t>(0))),
            static_cast<int>(qMax(coastlineTileBBox.left(), static_cast<int64_t>(0))),
            static_cast<int>(qMin(coastlineTileBBox.bottom(), static_cast<int64_t>(INT32_MAX))),
            static_cast<int>(qMin(coastlineTileBBox.right(), static_cast<int64_t>(INT32_MAX))));
        Ref<ObfMapSectionReader_Metrics::Metric_loadMapObjects> loadMapObjectsMetric;
        if (metric)
        {
            loadMapObjectsMetric = new ObfMapSectionReader_Metrics::Metric_loadMapObjects();
            metric->submetrics.push_back(loadMapObjectsMetric);
        }

        QSet<ObfObjectId> loadedCoastlineObjectsIds;
        const auto coastlineObjectsFilteringFunctor =
            [&loadedCoastlineObjectsIds]
            (const std::shared_ptr<const ObfMapSectionInfo>& section,
                const ObfMapSectionReader::DataBlockId& blockId,
                const ObfObjectId objectId,
                const AreaI& bbox,
                const ZoomLevel firstZoomLevel,
                const ZoomLevel lastZoomLevel,
                const ZoomLevel requestedZoom) -> bool
            {
                if (loadedCoastlineObjectsIds.contains(objectId))
                    return false;
                loadedCoastlineObjectsIds.insert(objectId);

                return true;
            };

        dataInterface->loadBinaryMapObjects(
            &loadedCoastlineMapObjects,
            &coastlineTileSurfaceType,
            owner->environment,
            _coastlineZoom,
            &coastlineTileBBox31,
            coastlineObjectsFilteringFunctor,
            nullptr,
            nullptr,
            request.queryController,
            nullptr,
            true// coastlineOnly
        );
    }

    const auto loadedSharedBinaryMapObjectsCountersSize = loadedSharedBinaryMapObjectsCounters.size();
    const auto loadedSharedRoadsCountersSize = loadedSharedRoadsCounters.size();

    // Process loaded-and-shared map objects (both binary and roads)
    for (auto& binaryMapObject : loadedBinaryMapObjects)
    {
        const UniqueBinaryMapObjectId id(binaryMapObject->blockId, binaryMapObject->id);
        // Check if this binary map object is shared (since only shared produce promises)
        if (!loadedSharedBinaryMapObjectsCounters.contains(id))
            continue;

        loadedSharedBinaryMapObjectsCounters.removeOne(id);

        // Add unique binary map object under lock to all zoom levels, for which this map object is valid
        _sharedBinaryMapObjects.fulfilPromiseAndReference(
            id,
            Utilities::enumerateZoomLevels(binaryMapObject->level->minZoom, binaryMapObject->level->maxZoom),
            binaryMapObject);
        loadedSharedBinaryMapObjects.push_back(binaryMapObject);
    }
    for (auto& loadedSharedBinaryMapObjectsCounter : loadedSharedBinaryMapObjectsCounters)
    {
        _sharedBinaryMapObjects.breakPromise(loadedSharedBinaryMapObjectsCounter);
    }
    for (auto& road : loadedRoads)
    {
        const UniqueRoadId id(road->blockId, road->id);
        // Check if this map object is shared (since only shared produce promises)
        if (!loadedSharedRoadsCounters.contains(id))
            continue;

        loadedSharedRoadsCounters.removeOne(id);

        // Add unique map object under lock to all zoom levels, for which this map object is valid
        _sharedRoads.fulfilPromiseAndReference(
            id,
            road);
        loadedSharedRoads.push_back(road);
    }
    for (auto& loadedSharedRoadsCounter : loadedSharedRoadsCounters)
    {
        _sharedRoads.breakPromise(loadedSharedRoadsCounter);
    }

    bool isCancelled = queryController->isAborted();

    int failCounter = 0;
    // Request all future map objects (both binary and roads)
    for (const auto& itFutureBinaryMapObject : rangeOf(futureReferencedBinaryMapObjects))
    {
        auto& futureBinaryMapObjectId = itFutureBinaryMapObject.key();
        auto& futureBinaryMapObject = itFutureBinaryMapObject.value();
        isCancelled = isCancelled || queryController->isAborted();
        if (isCancelled)
        {
            _sharedBinaryMapObjects.releaseFutureReference(futureBinaryMapObjectId, request.zoom);
            continue; 
        }

        auto timeout = proper::chrono::system_clock::now() + proper::chrono::seconds(3);
        if (proper::future_status::ready == futureBinaryMapObject.wait_until(timeout))
        {
            try
            {
                auto binaryMapObject = futureBinaryMapObject.get();
                referencedBinaryMapObjects.push_back(qMove(binaryMapObject));
            }
            catch(const std::exception& e)
            {
                continue;
            }
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

    for (const auto& itFutureRoad : rangeOf(futureReferencedRoads))
    {
        auto& futureRoadId = itFutureRoad.key();
        auto& futureRoad = itFutureRoad.value();
        isCancelled = isCancelled || queryController->isAborted();
        if (isCancelled)
        {
            _sharedRoads.releaseFutureReference(futureRoadId);
            continue; 
        }

        auto timeout = proper::chrono::system_clock::now() + proper::chrono::seconds(3);
        if (proper::future_status::ready == futureRoad.wait_until(timeout))
        {
            try
            {
                auto road = futureRoad.get();
                referencedRoads.push_back(qMove(road));
            }
            catch(const std::exception& e)
            {
                continue;
            }
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
    
    isCancelled = isCancelled || queryController->isAborted();
    if (isCancelled)
    {
        if (request.zoom >= _coastlineZoom && !coastlineTile)
        {
            coastlineTileEntry->setState(TileState::Cancelled);
            {
                QWriteLocker scopedLocker(&coastlineTileEntry->loadedConditionLock);
                coastlineTileEntry->loadedCondition.wakeAll();
            }
            _coastlineReferences.removeEntry(overscaledTileId, _coastlineZoom, empty);
            coastlineTileEntry.reset();
        }

        releaseThreadLock();

        return false;
    }

    if (request.zoom >= _coastlineZoom && !coastlineTile)
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
        (referencedBinaryMapObjects.size() + loadedSharedBinaryMapObjectsCountersSize) +
        (referencedRoads.size() + loadedSharedRoadsCountersSize);
    const auto allMapObjectsCount =
        (loadedBinaryMapObjects.size() + referencedBinaryMapObjects.size()) +
        (loadedRoads.size() + referencedRoads.size());
    QList< std::shared_ptr<const MapObject> > allMapObjects;
    allMapObjects.reserve(allMapObjectsCount);

    QHash< ObfObjectId, QList< std::shared_ptr<const ObfMapObject> > > duplicatedMapObjects; 
    const auto allBinaryMapObjects = loadedBinaryMapObjects + referencedBinaryMapObjects;
    const auto allRoads = loadedRoads + referencedRoads;
    // zoom-bench v2: ids can repeat only when the tile was built from 2+ maps
    QHash< ObfObjectId, uint32_t > binaryMapObjectIdCounts;
    const bool severalMaps = tileMapSections.size() > 1;
    if (severalMaps)
    {
        binaryMapObjectIdCounts.reserve(allBinaryMapObjects.size());
        for (const auto& binaryMapObject : constOf(allBinaryMapObjects))
            binaryMapObjectIdCounts[binaryMapObject->id]++;
    }
    for (const auto& binaryMapObject : constOf(allBinaryMapObjects))
    {   
        if (queryController->isAborted())
        {
            releaseThreadLock();
            return false;
        }

        const auto id = binaryMapObject->id;
        if (severalMaps && binaryMapObjectIdCounts.value(id) > 1)
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
        if (queryController->isAborted())
        {
            releaseThreadLock();
            return false;
        }

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

    bool addDuplicates = zoom <= ObfMapObjectsProvider::AddDuplicatedMapObjectsMaxZoom;

    // Date of each map section, found once: getObfSectionDate() copies the section name and compiles
    // a regular expression, and the sort below used to call it twice per comparison
    QHash<const ObfSectionInfo*, QString> sectionDates;
    for (auto& duplicates : duplicatedMapObjects.values())
    {
        if (queryController->isAborted())
        {
            releaseThreadLock();
            return false;
        }

        for (const auto& duplicate : constOf(duplicates))
        {
            const auto sectionInfo = duplicate->obfSection.get();
            if (!sectionDates.contains(sectionInfo))
                sectionDates.insert(sectionInfo, getObfSectionDate(duplicate->obfSection));
        }

        // Sort duplicated ObfMapObjects by maps date and data completeness
        std::sort(duplicates.begin(), duplicates.end(),
            [&sectionDates]
            (const std::shared_ptr<const ObfMapObject>& o1, const std::shared_ptr<const ObfMapObject>& o2)
            {
                if (std::dynamic_pointer_cast<const BinaryMapObject>(o1) && std::dynamic_pointer_cast<const Road>(o2))
                    return true;
                else if (std::dynamic_pointer_cast<const Road>(o1) && std::dynamic_pointer_cast<const BinaryMapObject>(o2))
                    return false;

                const auto& sectionDate1 = *sectionDates.constFind(o1->obfSection.get());
                const auto& sectionDate2 = *sectionDates.constFind(o2->obfSection.get());

                return sectionDate1.compare(sectionDate2) > 0;
            });

        if (addDuplicates)
        {
            // Add all duplicates with unique map and start/end points
            QSet<QString> obfSectionNames;
            QList<PointI> starts;
            QList<PointI> ends;

            for (const auto& duplicate : duplicates)
            {
                const auto& obfSectionName = formatObfSectionName(duplicate->obfSection, false);
                if (obfSectionNames.contains(obfSectionName))
                    continue;

                const auto& start = duplicate->points31.first();
                const auto& end = duplicate->points31.last();

                bool equalStartEnd = false;
                for (int i = 0; i < starts.size(); i++) {
                    if (start == starts[i] && end == ends[i])
                    {
                        equalStartEnd = true;
                        break;
                    }
                }
                if (equalStartEnd)
                    continue;

                allMapObjects.push_back(duplicate);
                obfSectionNames.insert(obfSectionName);
                starts.push_back(start);
                ends.push_back(end);
            }
        }
        else
        {
            // Add first object from most recent map
            allMapObjects.push_back(duplicates.first());
        }
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
            coastlineTileEntry,
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

    if (metric)
    {
        metric->elapsedTime += totalTimeStopwatch.elapsed();
        metric->objectsCount += allMapObjects.size();
        metric->uniqueObjectsCount += allMapObjects.size() - sharedMapObjectsCount;
        metric->sharedObjectsCount += sharedMapObjectsCount;
    }

    if (OsmAnd::isPerformanceMetricsEnabled())
        OsmAnd::getPerformanceMetrics().readFinish(request.tileId, request.zoom, allMapObjects.size(), sharedMapObjectsCount, allocationTime);

    releaseThreadLock();

    return true;
}

QString OsmAnd::ObfMapObjectsProvider_P::getObfSectionDate(const std::shared_ptr<const ObfSectionInfo>& sectionInfo)
{
    const auto& formattedSectionName = formatObfSectionName(sectionInfo, true);
    const QRegularExpression regex("(_[0-9]{2}){3}");
    const auto& match = regex.match(formattedSectionName);
    return match.captured(0);
}

QString OsmAnd::ObfMapObjectsProvider_P::formatObfSectionName(
    const std::shared_ptr<const ObfSectionInfo>& sectionInfo,
    const bool withDate)
{
    auto sectionName = sectionInfo->name.toLower();

    if (sectionName.endsWith("_2"))
        sectionName = sectionName.mid(0, sectionName.length() - 2);

    bool dateSpecified = false;
    for (const auto ch : sectionName)
    {
        if (ch >= '0' && ch <= '9')
        {
            dateSpecified = true;
            break;
        }
    }

    const QString defaultDate("_00_00_00");
    if (dateSpecified && !withDate)
        sectionName = sectionName.mid(0, sectionName.length() - defaultDate.length());
    else if (!dateSpecified && withDate)
        sectionName += defaultDate;

    return sectionName;
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
    const std::shared_ptr<TileSharedEntry>& tileSharedEntry,
    const std::shared_ptr<ObfMapSectionReader::DataBlocksCache>& binaryMapObjectsDataBlocksCache,
    const QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >& referencedBinaryMapObjectsDataBlocks_,
    const QList< std::shared_ptr<const BinaryMapObject> >& referencedBinaryMapObjects_,
    const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache>& roadsDataBlocksCache,
    const QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >& referencedRoadsDataBlocks_,
    const QList< std::shared_ptr<const Road> >& referencedRoads_)
    : zoom(zoom_)
    , weakLink(link->getWeak())
    , tileEntryWeakRef(tileSharedEntry)
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

    if (const auto link = weakLink.lock())
    {
        // Use the provider lifetime guard: the entry's link can be reset concurrently during removal.
        if (const auto tileEntry = tileEntryWeakRef.lock())
        {
            link->_coastlineReferences.removeEntry(tileEntry->tileId, tileEntry->zoom,
                [&tileEntry](const std::shared_ptr<TileSharedEntry>& entry)
                {
                    // Old metadata must not remove a replacement entry for the same tile.
                    return entry == tileEntry;
                });
        }

        // Dereference shared map objects
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
