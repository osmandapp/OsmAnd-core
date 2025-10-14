#include "Map3DObjectsKeyedProvider.h"

#include "MapDataProviderHelpers.h"
#include "Utilities.h"
#include "MapRenderer.h"

using namespace OsmAnd;

const Map3DObjectsKeyedProvider::Key Map3DObjectsKeyedProvider::SCREEN_KEY = reinterpret_cast<const void*>(0x12345678);

Map3DObjectsKeyedProvider::Data::Data(const Key key, const QList<std::shared_ptr<const MapPrimitiviser::Primitive>>& polygons_)
    : IMapKeyedDataProvider::Data(key)
    , polygons(polygons_)
{
}

Map3DObjectsKeyedProvider::Data::~Data()
{
}

Map3DObjectsKeyedProvider::Map3DObjectsKeyedProvider(const std::shared_ptr<MapPrimitivesProvider>& tiledProvider)
    : _tiledProvider(tiledProvider)
{
}

Map3DObjectsKeyedProvider::~Map3DObjectsKeyedProvider()
{
}

ZoomLevel Map3DObjectsKeyedProvider::getMinZoom() const
{
    return _tiledProvider ? _tiledProvider->getMinZoom() : MinZoomLevel;
}

ZoomLevel Map3DObjectsKeyedProvider::getMaxZoom() const
{
    return _tiledProvider ? _tiledProvider->getMaxZoom() : MaxZoomLevel;
}

int64_t Map3DObjectsKeyedProvider::getPriority() const
{
    return std::numeric_limits<int64_t>::min();
}

void Map3DObjectsKeyedProvider::setPriority(int64_t priority)
{
    // Not used for this provider
}

bool Map3DObjectsKeyedProvider::obtainKeyedData(
    const Request& request,
    std::shared_ptr<Data>& outKeyedData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    if (!_tiledProvider)
        return false;

    const auto& mapState = request.mapState;

    const auto visibleArea31 = mapState.visibleBBox31;
    const auto tileIds = Utilities::getAllMetaTileIds(Utilities::getTileId(visibleArea31.center(), mapState.zoomLevel));

    QList<std::shared_ptr<const MapPrimitiviser::Primitive>> allPolygons;
    
    for (const auto& tileId : tileIds)
    {
        MapPrimitivesProvider::Request tileRequest;
        tileRequest.tileId = tileId;
        tileRequest.zoom = mapState.zoomLevel;
        tileRequest.visibleArea31 = visibleArea31;
        tileRequest.queryController = request.queryController;

        std::shared_ptr<MapPrimitivesProvider::Data> tilePrimitives;
        const bool tileSuccess = _tiledProvider->obtainTiledPrimitives(tileRequest, tilePrimitives);
        
        if (request.queryController && request.queryController->isAborted())
            return false;
            
        if (tileSuccess && tilePrimitives && tilePrimitives->primitivisedObjects)
        {
            for (const auto& polygon : constOf(tilePrimitives->primitivisedObjects->polygons))
            {
                allPolygons.append(polygon);
            }
        }
    }

    if (!allPolygons.isEmpty())
    {
        outKeyedData = std::make_shared<Data>(request.key, allPolygons);
    }

    return !allPolygons.isEmpty();
}

bool Map3DObjectsKeyedProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& keyedRequest = static_cast<const Request&>(request);
    
    std::shared_ptr<Data> keyedData;
    const bool success = obtainKeyedData(keyedRequest, keyedData, pOutMetric);
    
    if (success)
    {
        outData = keyedData;
    }
    
    return success;
}

bool Map3DObjectsKeyedProvider::supportsNaturalObtainData() const
{
    return true;
}

bool Map3DObjectsKeyedProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void Map3DObjectsKeyedProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const ObtainDataAsyncCallback callback,
    const bool collectMetric)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

QList<Map3DObjectsKeyedProvider::Key> Map3DObjectsKeyedProvider::getProvidedDataKeys() const
{
    QList<Key> keys;
    keys.append(SCREEN_KEY);
    return keys;
}
