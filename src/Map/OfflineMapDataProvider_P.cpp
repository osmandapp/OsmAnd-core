#include "OfflineMapDataProvider_P.h"
#include "OfflineMapDataProvider.h"

#include <cassert>
#if defined(_DEBUG) || defined(DEBUG)
#   include <chrono>
#endif

#include "OfflineMapDataTile.h"
#include "OfflineMapDataTile_P.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "ObfMapSectionInfo.h"
#include "MapObject.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::OfflineMapDataProvider_P::OfflineMapDataProvider_P( OfflineMapDataProvider* owner_ )
    : owner(owner_)
    , _basemapPresenceChecked(0)
    , _isBasemapAvailable(false)
    , _link(new Link(*this))
{
}

OsmAnd::OfflineMapDataProvider_P::~OfflineMapDataProvider_P()
{
}

bool OsmAnd::OfflineMapDataProvider_P::isBasemapAvailable()
{
    if(_basemapPresenceChecked.load() != 0)
        return _isBasemapAvailable;
    else
    {
        QMutexLocker scopedLocker(&_basemapPresenceCheckMutex);

        // Repeat check, since things may have changed
        if(_basemapPresenceChecked.load() != 0)
            return _isBasemapAvailable;

        // Obtain flag
        bool isBasemapAvailable = false;
        owner->obfsCollection->obtainDataInterface()->obtainBasemapPresenceFlag(isBasemapAvailable);
        _isBasemapAvailable = isBasemapAvailable;

        _basemapPresenceChecked.store(1);

        return isBasemapAvailable;
    }
}

void OsmAnd::OfflineMapDataProvider_P::obtainTile( const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const OfflineMapDataTile>& outTile )
{
    // Check if there is a weak reference to that tile, and if that reference is still valid, use that
    std::shared_ptr<TileEntry> tileEntry;
    _tileReferences.obtainTileEntry(tileEntry, tileId, zoom, true);

    // Only if tile entry has "Unknown" state proceed to "Requesting" state
    {
        QWriteLocker scopedLock(&tileEntry->stateLock);

        assert(tileEntry->state != TileState::Released);
        if(tileEntry->state == TileState::Undefined)
        {
            // Since tile is in undefined state, it will be processed right now,
            // so just change state to 'Loading' and continue execution
            tileEntry->state = TileState::Loading;
        }
        else if(tileEntry->state == TileState::Loading)
        {
            // If tile is in 'Loading' state, wait until it will become 'Loaded'
            while(tileEntry->state != TileState::Loading)
                tileEntry->_loadedCondition.wait(&tileEntry->stateLock);
        }
        else if(tileEntry->state == TileState::Loaded)
        {
            // If tile is already 'Loaded', just verify it's reference and return that
            assert(!tileEntry->_tile.expired());
            outTile = tileEntry->_tile.lock();
            return;
        }
    }

    // Obtain OBF data interface
    const auto& dataInterface = owner->obfsCollection->obtainDataInterface();

    // Get bounding box that covers this tile
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

    // Perform read-out
    QList< std::shared_ptr<const Model::MapObject> > duplicateMapObjects;
    QList< std::shared_ptr<const Model::MapObject> > mapObjects;
    MapFoundationType tileFoundation;
#if defined(_DEBUG) || defined(DEBUG)
    float dataFilter = 0.0f;
    const auto dataRead_Begin = std::chrono::high_resolution_clock::now();
#endif
    auto& dataCache = _dataCache[zoom];
    dataInterface->obtainMapObjects(&mapObjects, &tileFoundation, tileBBox31, zoom, nullptr,
#if defined(_DEBUG) || defined(DEBUG)
        [&dataCache, &duplicateMapObjects, tileBBox31, &dataFilter](const std::shared_ptr<const ObfMapSectionInfo>& section, const uint64_t id) -> bool
#else
        [&dataCache, &duplicateMapObjects, tileBBox31](const std::shared_ptr<const ObfMapSectionInfo>& section, const uint64_t id) -> bool
#endif
        {
#if defined(_DEBUG) || defined(DEBUG)
            const auto dataFilter_Begin = std::chrono::high_resolution_clock::now();
#endif
            uint64_t internalId = id;
            if(static_cast<int64_t>(internalId) < 0)
            {
                // IDs < 0 are guaranteed to be unique only inside own section.
                const int64_t realId = -static_cast<int64_t>(internalId);
                assert((realId >> 32) == 0);
                internalId = static_cast<uint64_t>(-(static_cast<int64_t>(section->runtimeGeneratedId) << 32) - realId);
            }

            // Save reference to duplicate map object
            {
                QReadLocker scopedLocker(&dataCache._mapObjectsMutex);

                const auto itDuplicateMapObject = dataCache._mapObjects.constFind(internalId);
                if(itDuplicateMapObject != dataCache._mapObjects.cend())
                {
                    const auto& mapObjectWeakRef = *itDuplicateMapObject;

                    if(const auto& mapObject = mapObjectWeakRef.lock())
                    {
                        // Not all duplicates should be used, since some may lay outside bbox
                        if(mapObject->intersects(tileBBox31))
                            duplicateMapObjects.push_back(mapObject);

#if defined(_DEBUG) || defined(DEBUG)
                        const auto dataFilter_End = std::chrono::high_resolution_clock::now();
                        const std::chrono::duration<float> dataRead_Elapsed = dataFilter_End - dataFilter_Begin;
                        dataFilter += dataRead_Elapsed.count();
#endif

                        return false;
                    }
                }
            }

#if defined(_DEBUG) || defined(DEBUG)
            const auto dataFilter_End = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<float> dataRead_Elapsed = dataFilter_End - dataFilter_Begin;
            dataFilter += dataRead_Elapsed.count();
#endif

            return true;
        });

#if defined(_DEBUG) || defined(DEBUG)
    const auto dataRead_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> dataRead_Elapsed = dataRead_End - dataRead_Begin;

    const auto dataProcess_Begin = std::chrono::high_resolution_clock::now();
#endif

    // Append weak references to newly read map objects
    for(auto itMapObject = mapObjects.cbegin(); itMapObject != mapObjects.cend(); ++itMapObject)
    {
        const auto& mapObject = *itMapObject;

        // Modify map object id if needed
        const auto internalId = makeInternalId(mapObject);

        // Add unique map object under lock to all zoom levels, for which this map object is valid
        assert(mapObject->level);
        for(int zoomLevel = mapObject->level->minZoom; zoomLevel <= mapObject->level->maxZoom; zoomLevel++)
        {
            auto& dataCache = _dataCache[zoomLevel];
            {
                QWriteLocker scopedLocker(&dataCache._mapObjectsMutex);

                if(!dataCache._mapObjects.contains(internalId))
                    dataCache._mapObjects.insert(internalId, mapObject);
            }
        }
    }
#if defined(_DEBUG) || defined(DEBUG)
    const auto dataProcess_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> dataProcess_Elapsed = dataProcess_End - dataProcess_Begin;
    LogPrintf(LogSeverityLevel::Info,
        "%d map objects (%d unique, %d shared) in %dx%d@%d: filter %fs, read %fs, process %fs",
        mapObjects.size() + duplicateMapObjects.size(), mapObjects.size(), duplicateMapObjects.size(),
        tileId.x, tileId.y, zoom,
        dataFilter, dataRead_Elapsed.count(), dataProcess_Elapsed.count());
#endif

    // Create tile
    mapObjects << duplicateMapObjects;
    const auto newTile = new OfflineMapDataTile(tileFoundation, mapObjects);
    newTile->_d->_link = _link;
    newTile->_d->_refEntry = tileEntry;
    outTile.reset(newTile);

    // Store weak reference to new tile and mark it as 'Loaded'
    {
        QWriteLocker scopedLock(&tileEntry->stateLock);

        assert(tileEntry->state == TileState::Loading);
        tileEntry->state = TileState::Loaded;
        tileEntry->_tile = outTile;

        // Notify that tile has been loaded
        tileEntry->_loadedCondition.wakeAll();
    }
}

uint64_t OsmAnd::OfflineMapDataProvider_P::makeInternalId( const std::shared_ptr<const Model::MapObject>& mapObject )
{
    uint64_t internalId = mapObject->id;

    if(static_cast<int64_t>(internalId) < 0)
    {
        // IDs < 0 are guaranteed to be unique only inside own section
        const int64_t realId = -static_cast<int64_t>(internalId);
        assert((realId >> 32) == 0);
        assert(mapObject->section);
        internalId = static_cast<uint64_t>(-(static_cast<int64_t>(mapObject->section->runtimeGeneratedId) << 32) - realId);
    }

    return internalId;
}
