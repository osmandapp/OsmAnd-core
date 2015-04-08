#include "MapObjectsProvider_P.h"
#include "MapObjectsProvider.h"

#include "MapDataProviderHelpers.h"
#include "MapObject.h"
#include "Utilities.h"

OsmAnd::MapObjectsProvider_P::MapObjectsProvider_P(MapObjectsProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapObjectsProvider_P::~MapObjectsProvider_P()
{
}

bool OsmAnd::MapObjectsProvider_P::prepareData()
{
    if (_preparedData)
        return false;

    _preparedData = new PreparedData();
    _preparedData->minZoom = MaxZoomLevel;
    _preparedData->maxZoom = MinZoomLevel;
    _preparedData->bbox31.top() = _preparedData->bbox31.left() = std::numeric_limits<int32_t>::max();
    _preparedData->bbox31.bottom() = _preparedData->bbox31.right() = std::numeric_limits<int32_t>::min();
    for (const auto& mapObject : constOf(owner->mapObjects))
    {
        const auto minZoom = mapObject->getMinZoomLevel();
        const auto maxZoom = mapObject->getMaxZoomLevel();

        if (minZoom < _preparedData->minZoom)
            _preparedData->minZoom = minZoom;
        if (maxZoom > _preparedData->maxZoom)
            _preparedData->maxZoom = maxZoom;

        _preparedData->bbox31.enlargeToInclude(mapObject->bbox31);

        for (int zoomLevel = minZoom; zoomLevel <= maxZoom; zoomLevel++)
            _preparedData->dataByZoomLevel[static_cast<ZoomLevel>(zoomLevel)].mapObjectsList.append(mapObject);
    }
    for (int zoomLevel = _preparedData->minZoom; zoomLevel <= _preparedData->maxZoom; zoomLevel++)
    {
        auto& dataByZoom = _preparedData->dataByZoomLevel[static_cast<ZoomLevel>(zoomLevel)];

        dataByZoom.bbox31.top() = dataByZoom.bbox31.left() = std::numeric_limits<int32_t>::max();
        dataByZoom.bbox31.bottom() = dataByZoom.bbox31.right() = std::numeric_limits<int32_t>::min();

        for (const auto& mapObject : constOf(dataByZoom.mapObjectsList))
            dataByZoom.bbox31.enlargeToInclude(mapObject->bbox31);

        // QuadTree root node needs to be outer rectangle with each side being power of two
        auto treeRootArea = dataByZoom.bbox31;
        treeRootArea.top() = Utilities::getPreviousPowerOfTwo(treeRootArea.top());
        treeRootArea.left() = Utilities::getPreviousPowerOfTwo(treeRootArea.left());
        treeRootArea.bottom() = qMin(Utilities::getNextPowerOfTwo(treeRootArea.bottom()), static_cast<uint32_t>(std::numeric_limits<int32_t>::max()));
        treeRootArea.right() = qMin(Utilities::getNextPowerOfTwo(treeRootArea.right()), static_cast<uint32_t>(std::numeric_limits<int32_t>::max()));
        dataByZoom.mapObjectsTree = qMove(MapObjectsTree(treeRootArea, 4));

        for (const auto& mapObject : constOf(dataByZoom.mapObjectsList))
            dataByZoom.mapObjectsTree.insert(mapObject, mapObject->bbox31);
    }

    return true;
}

OsmAnd::ZoomLevel OsmAnd::MapObjectsProvider_P::getMinZoom() const
{
    if (!_preparedData)
        return MinZoomLevel;
    return _preparedData->minZoom;
}

OsmAnd::ZoomLevel OsmAnd::MapObjectsProvider_P::getMaxZoom() const
{
    if (!_preparedData)
        return MaxZoomLevel;
    return _preparedData->maxZoom;
}

bool OsmAnd::MapObjectsProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<MapObjectsProvider::Request>(request_);

    if (pOutMetric)
        pOutMetric->reset();

    std::shared_ptr<TileEntry> tileEntry;

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

        if (!tileEntry->dataIsPresent)
        {
            // If there was no data, return same
            outData.reset();
            return true;
        }
        else
        {
            // Otherwise, try to lock tile reference
            outData = tileEntry->dataWeakRef.lock();

            // If successfully locked, just return it
            if (outData)
                return true;

            // Otherwise consider this tile entry as expired, remove it from collection (it's safe to do that right now)
            // This will enable creation of new entry on next loop cycle
            _tileReferences.removeEntry(request.tileId, request.zoom);
            tileEntry.reset();
        }
    }

    // Check if there's data for specified zoom level or there
    if (request.zoom > _preparedData->maxZoom || request.zoom < _preparedData->minZoom)
    {
        // Store flag that there was no data and mark tile entry as 'Loaded'
        tileEntry->dataIsPresent = false;
        tileEntry->setState(TileState::Loaded);

        // Notify that tile has been loaded
        {
            QWriteLocker scopedLcoker(&tileEntry->loadedConditionLock);
            tileEntry->loadedCondition.wakeAll();
        }

        outData.reset();
        return true;
    }
    const auto citDataByZoom = _preparedData->dataByZoomLevel.constFind(request.zoom);
    if (citDataByZoom == _preparedData->dataByZoomLevel.cend())
    {
        // Store flag that there was no data and mark tile entry as 'Loaded'
        tileEntry->dataIsPresent = false;
        tileEntry->setState(TileState::Loaded);

        // Notify that tile has been loaded
        {
            QWriteLocker scopedLcoker(&tileEntry->loadedConditionLock);
            tileEntry->loadedCondition.wakeAll();
        }

        outData.reset();
        return true;
    }

    const auto& dataByZoom = *citDataByZoom;
    const auto tileBBox31 = Utilities::tileBoundingBox31(request.tileId, request.zoom);

    // In case bbox31 entirely contains all data on given zoom level, return every object
    if (tileBBox31.contains(dataByZoom.bbox31))
    {
        const std::shared_ptr<MapObjectsProvider::Data> newTiledData(new MapObjectsProvider::Data(
            request.tileId,
            request.zoom,
            MapSurfaceType::Undefined,
            dataByZoom.mapObjectsList));

        // Publish new tile
        outData = newTiledData;

        // Store weak reference to new tile and mark it as 'Loaded'
        tileEntry->dataIsPresent = true;
        tileEntry->dataWeakRef = newTiledData;
        tileEntry->setState(TileState::Loaded);

        return true;
    }

    // Otherwise, check if there's common area between zoom area and tile bbox
    if (!dataByZoom.bbox31.contains(tileBBox31) && !dataByZoom.bbox31.intersects(tileBBox31))
    {
        // Store flag that there was no data and mark tile entry as 'Loaded'
        tileEntry->dataIsPresent = false;
        tileEntry->setState(TileState::Loaded);

        // Notify that tile has been loaded
        {
            QWriteLocker scopedLcoker(&tileEntry->loadedConditionLock);
            tileEntry->loadedCondition.wakeAll();
        }

        outData.reset();
        return true;
    }

    // Query map objects quad-tree to select objects that are inside bbox or intersect it
    QList< std::shared_ptr<const MapObject> > mapObjectsInTileBBox;
    dataByZoom.mapObjectsTree.query(tileBBox31, mapObjectsInTileBBox);

    // Check if there are map objects for specified tile
    if (mapObjectsInTileBBox.isEmpty())
    {
        // Store flag that there was no data and mark tile entry as 'Loaded'
        tileEntry->dataIsPresent = false;
        tileEntry->setState(TileState::Loaded);

        // Notify that tile has been loaded
        {
            QWriteLocker scopedLcoker(&tileEntry->loadedConditionLock);
            tileEntry->loadedCondition.wakeAll();
        }

        outData.reset();
        return true;
    }

    const std::shared_ptr<MapObjectsProvider::Data> newTiledData(new MapObjectsProvider::Data(
        request.tileId,
        request.zoom,
        MapSurfaceType::Undefined,
        mapObjectsInTileBBox));

    // Publish new tile
    outData = newTiledData;

    // Store weak reference to new tile and mark it as 'Loaded'
    tileEntry->dataIsPresent = true;
    tileEntry->dataWeakRef = newTiledData;
    tileEntry->setState(TileState::Loaded);

    return true;
}

OsmAnd::MapObjectsProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<TileEntry>& tileEntry)
    : tileEntryWeakRef(tileEntry)
{
}

OsmAnd::MapObjectsProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
    // Remove tile reference from collection. All checks here does not matter,
    // since entry->tile reference is already expired
    if (const auto tileEntry = tileEntryWeakRef.lock())
    {
        if (const auto link = tileEntry->link.lock())
            link->collection.removeEntry(tileEntry->tileId, tileEntry->zoom);
    }
}
