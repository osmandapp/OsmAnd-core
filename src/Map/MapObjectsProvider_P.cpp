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
    for (const auto& mapObject : constOf(owner->mapObjects))
    {
        const auto minZoom = mapObject->getMinZoomLevel();
        const auto maxZoom = mapObject->getMaxZoomLevel();

        if (minZoom < _preparedData->minZoom)
            _preparedData->minZoom = minZoom;
        if (maxZoom > _preparedData->maxZoom)
            _preparedData->maxZoom = maxZoom;

        for (int zoomLevel = minZoom; zoomLevel <= maxZoom; zoomLevel++)
        {
            _preparedData->mapObjectsByZoomLevel[static_cast<ZoomLevel>(zoomLevel)].append(mapObject);
        }
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
    // Check if there's data for specified zoom level
    if (zoom > _preparedData->maxZoom || zoom < _preparedData->minZoom)
    {
        outTiledData.reset();
        return true;
    }
    const auto citMapObjectsInZoom = _preparedData->mapObjectsByZoomLevel.constFind(zoom);
    if (citMapObjectsInZoom == _preparedData->mapObjectsByZoomLevel.cend())
    {
        outTiledData.reset();
        return true;
    }

    const auto& mapObjectsInZoom = *citMapObjectsInZoom;
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);
    QList< std::shared_ptr<const MapObject> > mapObjects;
    for (const auto& mapObject : constOf(mapObjectsInZoom))
    {
        if (!mapObject->intersectedOrContainedBy(tileBBox31))
            continue;

        mapObjects.append(mapObject);
    }

    // Check if there are map objects for specified tile
    if (mapObjects.isEmpty())
    {
        outTiledData.reset();
        return true;
    }

    outTiledData.reset(new MapObjectsProvider::Data(tileId, zoom, MapFoundationType::Undefined, mapObjects));
    return true;
}
