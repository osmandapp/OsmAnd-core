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
    _renderableBuildings.vertexCounts.clear();
    _renderableBuildings.indexCounts.clear();
    _renderableBuildings.ids.clear();
    _renderableBuildings.colors.clear();
    _renderableBuildings.vertexOffsets.clear();
    _renderableBuildings.indexOffsets.clear();

    const auto map3DTileData = std::dynamic_pointer_cast<Map3DObjectsTiledProvider::Data>(_sourceData);
    if (!map3DTileData || map3DTileData->buildings3D.isEmpty())
    {
        _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
        _performanceDebugInfo.totalGpuMemoryBytes = 0;
        return true;
    }

    int totalVertexCount = 0;
    int totalIndexCount = 0;
    
    for (const auto& building : map3DTileData->buildings3D)
    {
        if (building.vertices.isEmpty() || building.indices.isEmpty())
        {
            continue;
        }
        
        totalVertexCount += building.vertices.size();
        totalIndexCount += building.indices.size();
    }

    if (totalVertexCount == 0 || totalIndexCount == 0)
    {
        _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
        _performanceDebugInfo.totalGpuMemoryBytes = 0;
        return true;
    }

    QVector<BuildingVertex> combinedVertices;
    combinedVertices.reserve(totalVertexCount);
    QVector<uint16_t> combinedIndices;
    combinedIndices.reserve(totalIndexCount);

    const int buildingCount = map3DTileData->buildings3D.size();
    _renderableBuildings.vertexCounts.reserve(buildingCount);
    _renderableBuildings.indexCounts.reserve(buildingCount);
    _renderableBuildings.ids.reserve(buildingCount);
    _renderableBuildings.colors.reserve(buildingCount);
    _renderableBuildings.vertexOffsets.reserve(buildingCount);
    _renderableBuildings.indexOffsets.reserve(buildingCount);

    int currentVertexOffset = 0;
    int currentIndexOffset = 0;

    for (const auto& building : map3DTileData->buildings3D)
    {
        if (building.vertices.isEmpty() || building.indices.isEmpty())
        {
            continue;
        }

        const int vertexCount = building.vertices.size();
        const int indexCount = building.indices.size();

        _renderableBuildings.vertexOffsets.append(currentVertexOffset);
        _renderableBuildings.indexOffsets.append(currentIndexOffset);

        combinedVertices.append(building.vertices);

        for (const auto& index : building.indices)
        {
            combinedIndices.append(static_cast<uint16_t>(index + currentVertexOffset));
        }

        _renderableBuildings.vertexCounts.append(vertexCount);
        _renderableBuildings.indexCounts.append(indexCount);
        _renderableBuildings.ids.append(building.id);
        _renderableBuildings.colors.append(building.color);

        currentVertexOffset += vertexCount;
        currentIndexOffset += indexCount;
    }

    if (combinedVertices.isEmpty() || combinedIndices.isEmpty())
    {
        _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
        _performanceDebugInfo.totalGpuMemoryBytes = 0;
        return true;
    }

    const size_t vertexBufferSize = combinedVertices.size() * sizeof(BuildingVertex);
    std::shared_ptr<const GPUAPI::ArrayBufferInGPU> vertexBufferInGPU;
    const bool vertexUploadSuccess = resourcesManager->uploadVerticesToGPU(
        combinedVertices.constData(),
        vertexBufferSize,
        combinedVertices.size(),
        vertexBufferInGPU,
        false);

    if (!vertexUploadSuccess)
    {
        _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
        _performanceDebugInfo.totalGpuMemoryBytes = 0;
        return false;
    }

    const size_t indexBufferSize = combinedIndices.size() * sizeof(uint16_t);
    std::shared_ptr<const GPUAPI::ElementArrayBufferInGPU> indexBufferInGPU;
    const bool indexUploadSuccess = resourcesManager->uploadIndicesToGPU(
        combinedIndices.constData(),
        indexBufferSize,
        combinedIndices.size(),
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
    _renderableBuildings.vertexCounts.clear();
    _renderableBuildings.indexCounts.clear();
    _renderableBuildings.ids.clear();
    _renderableBuildings.colors.clear();
    _renderableBuildings.vertexOffsets.clear();
    _renderableBuildings.indexOffsets.clear();
}

void MapRenderer3DObjectsResource::lostDataInGPU()
{
    _renderableBuildings.vertexBuffer.reset();
    _renderableBuildings.indexBuffer.reset();
    _renderableBuildings.vertexCounts.clear();
    _renderableBuildings.indexCounts.clear();
    _renderableBuildings.ids.clear();
    _renderableBuildings.colors.clear();
    _renderableBuildings.vertexOffsets.clear();
    _renderableBuildings.indexOffsets.clear();
}

void MapRenderer3DObjectsResource::releaseData()
{
    _sourceData.reset();
}

