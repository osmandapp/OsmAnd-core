#include "MapObjectsProvider_P.h"
#include "MapObjectsProvider.h"

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
        dataByZoom.mapObjectsTree = qMove(MapObjectsTree(treeRootArea, 8));

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
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapObjectsProvider::Data>& outTiledData,
    const IQueryController* const queryController)
{
    // Check if there's data for specified zoom level or there
    if (zoom > _preparedData->maxZoom || zoom < _preparedData->minZoom)
    {
        outTiledData.reset();
        return true;
    }
    const auto citDataByZoom = _preparedData->dataByZoomLevel.constFind(zoom);
    if (citDataByZoom == _preparedData->dataByZoomLevel.cend())
    {
        outTiledData.reset();
        return true;
    }

    const auto& dataByZoom = *citDataByZoom;
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

    // In case bbox31 entirely contains all data on given zoom level, return every object
    if (tileBBox31.contains(dataByZoom.bbox31))
    {
        outTiledData.reset(new MapObjectsProvider::Data(
            tileId,
            zoom,
            MapSurfaceType::Undefined,
            dataByZoom.mapObjectsList));
        return true;
    }

    // Otherwise, check if there's common area between zoom area and tile bbox
    if (!dataByZoom.bbox31.contains(tileBBox31) && !dataByZoom.bbox31.intersects(tileBBox31))
    {
        outTiledData.reset();
        return true;
    }

    // Query map objects quad-tree to select objects that are inside bbox or intersect it
    QList< std::shared_ptr<const MapObject> > mapObjectsInTileBBox;
    dataByZoom.mapObjectsTree.query(tileBBox31, mapObjectsInTileBBox);

    // Check if there are map objects for specified tile
    if (mapObjectsInTileBBox.isEmpty())
    {
        outTiledData.reset();
        return true;
    }

    outTiledData.reset(new MapObjectsProvider::Data(
        tileId,
        zoom,
        MapSurfaceType::Undefined,
        mapObjectsInTileBBox));
    return true;
}
