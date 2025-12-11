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

    _renderableBuildings.buildingResources.clear();

    const auto map3DTileData = std::dynamic_pointer_cast<Map3DObjectsTiledProvider::Data>(_sourceData);
    if (!map3DTileData || map3DTileData->buildings3D.vertices.isEmpty() || map3DTileData->buildings3D.indices.isEmpty())
    {
        _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
        _performanceDebugInfo.totalGpuMemoryBytes = 0;
        return true;
    }

    const auto& buildings3D = map3DTileData->buildings3D;

    if (buildings3D.buildingIDs.isEmpty())
    {
        _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
        _performanceDebugInfo.totalGpuMemoryBytes = 0;
        return true;
    }

    QMutexLocker locker(&resourcesManager->_3DBuildingsDataMutex);

    QSet<uint64_t> uniqueBuildingIDs;
    QVector<BuildingVertex> uniqueVertices;
    QVector<uint16_t> uniqueIndices;
    
    int vertexOffset = 0;
    int indexOffset = 0;
    
    for (int i = 0; i < buildings3D.buildingIDs.size(); ++i)
    {
        const uint64_t buildingID = buildings3D.buildingIDs[i];
        const int vertexCount = buildings3D.vertexCounts[i];
        const int indexCount = buildings3D.indexCounts[i];

        std::shared_ptr<GPUAPI::MapRenderer3DBuildingGPUData> duplicateData;
        bool isDuplicate = false;
        for (const auto& data : constOf(resourcesManager->_shared3DBuildings))
        {
            for (const uint64_t sahredBuildingID : constOf(data->buildingIDs))
            {
                if (buildingID == sahredBuildingID)
                {
                    duplicateData = data;
                    isDuplicate = true;
                    continue;
                }
            }
        }

        if (isDuplicate)
        {
            ++duplicateData->referenceCount;
            _renderableBuildings.buildingResources.insert(duplicateData);
            vertexOffset += vertexCount;
            indexOffset += indexCount;
            continue;
        }

        uniqueBuildingIDs.insert(buildingID);
        
        uint16_t currentVertexOffset = static_cast<uint16_t>(uniqueVertices.size());
        for (int j = 0; j < vertexCount; ++j)
        {
            uniqueVertices.append(buildings3D.vertices[vertexOffset + j]);
        }
        
        for (int j = 0; j < indexCount; ++j)
        {
            uniqueIndices.append(static_cast<uint16_t>(buildings3D.indices[indexOffset + j] - vertexOffset + currentVertexOffset));
        }
        
        vertexOffset += vertexCount;
        indexOffset += indexCount;
    }
    
    std::shared_ptr<GPUAPI::MapRenderer3DBuildingGPUData> buildingData;
    
    if (!uniqueVertices.isEmpty())
    {
        buildingData = resourcesManager->findOrCreate3DBuildingGPUDataLocked(zoom, tileId, uniqueVertices, uniqueIndices, uniqueBuildingIDs);
        if (!buildingData)
        {
            _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;
            _performanceDebugInfo.totalGpuMemoryBytes = 0;
            return false;
        }
        
        buildingData->referenceCount++;
        _renderableBuildings.buildingResources.insert(buildingData);

        const size_t totalGpuMemoryBytes = uniqueVertices.size() * sizeof(BuildingVertex) + uniqueIndices.size() * sizeof(uint16_t);
        _performanceDebugInfo.totalGpuMemoryBytes = totalGpuMemoryBytes;
    }
    else
    {
        _performanceDebugInfo.totalGpuMemoryBytes = 0;
    }

    _performanceDebugInfo.uploadToGpuTimeMilliseconds = stopwatch.elapsed() * 1000.0f;

    return true;
}

void MapRenderer3DObjectsResource::unloadFromGPU()
{
    for (const auto& buildingData : constOf(_renderableBuildings.buildingResources))
    {
        resourcesManager->release3DBuildingGPUData(buildingData->zoom, buildingData->tileId);
    }
    _renderableBuildings.buildingResources.clear();
}

void MapRenderer3DObjectsResource::lostDataInGPU()
{
    for (const auto& buildingData : constOf(_renderableBuildings.buildingResources))
    {
        resourcesManager->release3DBuildingGPUData(buildingData->zoom, buildingData->tileId);
    }
    _renderableBuildings.buildingResources.clear();
}

void MapRenderer3DObjectsResource::releaseData()
{
    _sourceData.reset();
}

