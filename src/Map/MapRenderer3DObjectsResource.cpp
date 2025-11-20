#include "MapRenderer3DObjectsResource.h"

#include "MapRendererResourcesManager.h"
#include "MapRendererTiledResourcesCollection.h"
#include "IMapTiledDataProvider.h"
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
#include "Map3DObjectsProvider.h"
#include "Utilities.h"
#include "MapRenderer.h"
#include "Stopwatch.h"
#include <iostream>

using namespace OsmAnd;

MapRenderer3DObjectsResource::MapRenderer3DObjectsResource(
    MapRendererResourcesManager* const owner,
    const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
    const TileId tileId,
    const ZoomLevel zoom)
    : MapRendererBaseTiledResource(owner, MapRendererResourceType::Map3DObjects, collection, tileId, zoom)
    , _performanceDebugInfo()
{
    _performanceDebugInfo.totalGpuMemoryBytes = 0;
    _performanceDebugInfo.obtainDataTimeMilliseconds = 0.0f;
    _performanceDebugInfo.uploadToGpuTimeMilliseconds = 0.0f;
}

MapRenderer3DObjectsResource::~MapRenderer3DObjectsResource()
{
}

bool MapRenderer3DObjectsResource::getProvider(std::shared_ptr<IMapDataProvider>& provider) const
{
    const auto link_ = link.lock();
    if (!link_)
    {
        return false;
    }

    const auto collection = static_cast<MapRendererTiledResourcesCollection*>(&link_->collection);
    if (!resourcesManager->obtainProviderFor(static_cast<MapRendererBaseResourcesCollection*>(collection), provider))
    {
        return false;
    }

    return true;
}

bool MapRenderer3DObjectsResource::supportsObtainDataAsync() const
{
    std::shared_ptr<IMapDataProvider> provider;
    if (!getProvider(provider))
    {
        return false;
    }

    return provider->supportsNaturalObtainDataAsync();
}

bool MapRenderer3DObjectsResource::obtainData(bool& dataAvailable, const std::shared_ptr<const IQueryController>& queryController)
{
    Stopwatch stopwatch(true);
    
    std::shared_ptr<IMapDataProvider> iProvider;
    if (!getProvider(iProvider))
    {
        return false;
    }

    const auto provider = std::static_pointer_cast<IMapTiledDataProvider>(iProvider);
    const auto mapState = resourcesManager->renderer->getMapState();
    const auto visibleArea = Utilities::roundBoundingBox31(mapState.visibleBBox31, mapState.zoomLevel);
    const auto currentTime = QDateTime::currentMSecsSinceEpoch();

    IMapTiledDataProvider::Request request;
    request.tileId = tileId;
    request.zoom = zoom;
    request.visibleArea31 = Utilities::getEnlargedVisibleArea(visibleArea);
    request.areaTime = currentTime;
    request.queryController = queryController;

    const bool requestSucceeded = provider->obtainTiledData(request, _sourceData);
    if (!requestSucceeded || !_sourceData)
    {
        return false;
    }

    const auto map3DTileData = std::dynamic_pointer_cast<Map3DObjectsTiledProvider::Data>(_sourceData);
    dataAvailable = static_cast<bool>(map3DTileData);
    
    _performanceDebugInfo.obtainDataTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
    
    return true;
}

void MapRenderer3DObjectsResource::obtainDataAsync(ObtainDataAsyncCallback callback,
    const std::shared_ptr<const IQueryController>& queryController, const bool cacheOnly)
{
    bool dataAvailable = false;
    auto okSync = obtainData(dataAvailable, queryController);
    if (callback) callback(okSync, dataAvailable);
}

bool MapRenderer3DObjectsResource::uploadToGPU()
{
    Stopwatch stopwatch(true);
    
    if (!_sourceData)
    {
        _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
        _performanceDebugInfo.totalGpuMemoryBytes = 0;
        return true;
    }

    _renderableBuildings.vertexBuffer.reset();
    _renderableBuildings.indexBuffer.reset();

    const auto map3DTileData = std::dynamic_pointer_cast<Map3DObjectsTiledProvider::Data>(_sourceData);
    if (!map3DTileData || map3DTileData->buildings3D.vertices.isEmpty() || map3DTileData->buildings3D.indices.isEmpty())
    {
        _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
        _performanceDebugInfo.totalGpuMemoryBytes = 0;
        return true;
    }

    const auto& buildings3D = map3DTileData->buildings3D;

    const size_t vertexBufferSize = buildings3D.vertices.size() * sizeof(BuildingVertex);
    std::shared_ptr<const GPUAPI::ArrayBufferInGPU> vertexBufferInGPU;
    const bool vertexUploadSuccess = resourcesManager->uploadVerticesToGPU(
        buildings3D.vertices.constData(),
        vertexBufferSize,
        buildings3D.vertices.size(),
        vertexBufferInGPU,
        false);

    if (!vertexUploadSuccess)
    {
        _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
        _performanceDebugInfo.totalGpuMemoryBytes = 0;
        return false;
    }

    const size_t indexBufferSize = buildings3D.indices.size() * sizeof(uint16_t);
    std::shared_ptr<const GPUAPI::ElementArrayBufferInGPU> indexBufferInGPU;
    const bool indexUploadSuccess = resourcesManager->uploadIndicesToGPU(
        buildings3D.indices.constData(),
        indexBufferSize,
        buildings3D.indices.size(),
        indexBufferInGPU,
        false);

    if (!indexUploadSuccess)
    {
        _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
        _performanceDebugInfo.totalGpuMemoryBytes = 0;
        return false;
    }

    _renderableBuildings.vertexBuffer = vertexBufferInGPU;
    _renderableBuildings.indexBuffer = indexBufferInGPU;
    _renderableBuildings.totalIndexCount = buildings3D.indices.size();

    const size_t totalGpuMemoryBytes = vertexBufferSize + indexBufferSize;
    _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
    _performanceDebugInfo.totalGpuMemoryBytes = totalGpuMemoryBytes;

    //_sourceData.reset();
    return true;
}

void MapRenderer3DObjectsResource::unloadFromGPU()
{
    _renderableBuildings.vertexBuffer.reset();
    _renderableBuildings.indexBuffer.reset();
    _renderableBuildings.totalIndexCount = 0;
}

void MapRenderer3DObjectsResource::lostDataInGPU()
{
    _renderableBuildings.vertexBuffer.reset();
    _renderableBuildings.indexBuffer.reset();
    _renderableBuildings.totalIndexCount = 0;
}

void MapRenderer3DObjectsResource::releaseData()
{
    _sourceData.reset();
}

