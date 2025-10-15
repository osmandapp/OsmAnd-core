#include "Map3DObjectsProvider.h"

#include "MapDataProviderHelpers.h"
#include "Utilities.h"
#include "MapRenderer.h"

#include "IMapTiledSymbolsProvider.h"

using namespace OsmAnd;

Map3DObjectsTiledProvider::Data::Data(const TileId tileId, const ZoomLevel zoom,
    const QList<std::shared_ptr<const MapPrimitiviser::Primitive>>& polygons_)
    : IMapTiledDataProvider::Data(tileId, zoom)
    , polygons(polygons_)
{
}

Map3DObjectsTiledProvider::Data::~Data()
{
}

Map3DObjectsTiledProvider::Map3DObjectsTiledProvider(const std::shared_ptr<MapPrimitivesProvider>& tiledProvider)
    : _tiledProvider(tiledProvider)
{
}

Map3DObjectsTiledProvider::~Map3DObjectsTiledProvider()
{
}

ZoomLevel Map3DObjectsTiledProvider::getMinZoom() const
{
    return _tiledProvider ? _tiledProvider->getMinZoom() : MinZoomLevel;
}

ZoomLevel Map3DObjectsTiledProvider::getMaxZoom() const
{
    return _tiledProvider ? _tiledProvider->getMaxZoom() : MaxZoomLevel;
}

bool Map3DObjectsTiledProvider::obtainTiledData(
    const IMapTiledDataProvider::Request& request,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    if (!_tiledProvider)
        return false;

    MapPrimitivesProvider::Request tileRequest;
    tileRequest.tileId = request.tileId;
    tileRequest.zoom = request.zoom;
    tileRequest.visibleArea31 = request.visibleArea31;
    tileRequest.queryController = request.queryController;

    std::shared_ptr<MapPrimitivesProvider::Data> tilePrimitives;
    const bool tileSuccess = _tiledProvider->obtainTiledPrimitives(tileRequest, tilePrimitives);
    if (!tileSuccess)
        return false;

    QList<std::shared_ptr<const MapPrimitiviser::Primitive>> allPolygons;
    if (tilePrimitives && tilePrimitives->primitivisedObjects)
    {
        for (const auto& polygon : constOf(tilePrimitives->primitivisedObjects->polygons))
        {
            QMutexLocker scopedLocker(&primitiveFilterMutex);
            if (primitiveFilter.contains(polygon->sourceObject))
            {
                continue;
            }

            primitiveFilter.insert(polygon->sourceObject);
            allPolygons.append(polygon);
        }
    }

    if (allPolygons.isEmpty())
        return true;

    outTiledData = std::make_shared<Data>(request.tileId, request.zoom, allPolygons);
    return true;
}

bool Map3DObjectsTiledProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    // Delegate to tiled
    const auto& tiledRequest = static_cast<const IMapTiledDataProvider::Request&>(request);
    std::shared_ptr<IMapTiledDataProvider::Data> tiledData;
    const bool success = obtainTiledData(tiledRequest, tiledData, pOutMetric);
    outData = tiledData;
    return success;
}

bool Map3DObjectsTiledProvider::supportsNaturalObtainData() const
{
    return true;
}

bool Map3DObjectsTiledProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void Map3DObjectsTiledProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const ObtainDataAsyncCallback callback,
    const bool collectMetric)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

// No keyed keys and no applyMapChanges needed for tiled provider
