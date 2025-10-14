#include "MapRenderer3DObjectsResource.h"

#include "MapRendererResourcesManager.h"
#include "MapRendererTiledResourcesCollection.h"
#include "IMapTiledDataProvider.h"
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
//#include "AtlasMapRenderer_OpenGL.h"

using namespace OsmAnd;

MapRenderer3DObjectsResource::MapRenderer3DObjectsResource(
    MapRendererResourcesManager* const owner,
    const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
    const TileId tileId,
    const ZoomLevel zoom)
    : MapRendererBaseTiledResource(owner, MapRendererResourceType::Map3DObjects, collection, tileId, zoom)
{
}

MapRenderer3DObjectsResource::~MapRenderer3DObjectsResource()
{
}

bool MapRenderer3DObjectsResource::supportsObtainDataAsync() const
{
    return false;
}

bool MapRenderer3DObjectsResource::obtainData(bool& dataAvailable, const std::shared_ptr<const IQueryController>& queryController)
{
    bool ok = false;

    std::shared_ptr<IMapDataProvider> provider_;
    if (const auto link_ = link.lock())
    {
        ok = resourcesManager->obtainProviderFor(
            static_cast<MapRendererBaseResourcesCollection*>(static_cast<MapRendererTiledResourcesCollection*>(&link_->collection)),
            provider_);
    }
    if (!ok)
        return false;
    
    const auto provider = std::dynamic_pointer_cast<MapPrimitivesProvider>(provider_);
    if (!provider)
        return false;

    MapPrimitivesProvider::Request request;
    request.tileId = tileId;
    request.zoom = zoom;
    request.queryController = queryController;

    std::shared_ptr<MapPrimitivesProvider::Data> primitives;
    dataAvailable = provider->obtainTiledPrimitives(request, primitives);
    if (dataAvailable)
        _sourceData = qMove(primitives);

    return true;
}

void MapRenderer3DObjectsResource::obtainDataAsync(ObtainDataAsyncCallback callback,
    const std::shared_ptr<const IQueryController>& queryController,
    const bool cacheOnly)
{
    bool dataAvailable = false;
    auto okSync = obtainData(dataAvailable, queryController);
    if (callback) callback(okSync, dataAvailable);
}

bool MapRenderer3DObjectsResource::uploadToGPU()
{
    auto sourceData = _sourceData;
    if (!sourceData || !sourceData->primitivisedObjects)
        return true;

    _testBuildings.clear();

    for (const auto& primitive : constOf(sourceData->primitivisedObjects->polygons))
    {
        if (primitive->type != MapPrimitiviser::PrimitiveType::Polygon)
            continue;

        const auto& sourceObject = primitive->sourceObject;
        if (!sourceObject || sourceObject->points31.isEmpty())
            continue;

        const int vertexCount = sourceObject->points31.size();
        const size_t vertexBufferSize = static_cast<size_t>(vertexCount * sizeof(Vertex));

        QVector<Vertex> vertexData(vertexCount);
        for (int i = 0; i < vertexCount; ++i)
        {
            const auto& point31 = sourceObject->points31[i];
            vertexData[i].position = glm::vec3(point31.x, 10.0f, point31.y);
        }

        std::shared_ptr<const GPUAPI::ArrayBufferInGPU> vertexBufferInGPU;
        const bool uploadSuccess = resourcesManager->uploadVerticesToGPU(
            vertexData.constData(),
            vertexBufferSize,
            static_cast<unsigned int>(vertexCount),
            vertexBufferInGPU,
            false);

        if (!uploadSuccess)
            continue;

        TestBuildingResource building(vertexBufferInGPU, vertexCount);
        building.debugPoints31 = sourceObject->points31;
        _testBuildings.append(building);
    }

    _sourceData.reset();
    return true;
}

void MapRenderer3DObjectsResource::unloadFromGPU()
{
    _testBuildings.clear();
}

void MapRenderer3DObjectsResource::lostDataInGPU()
{
    _testBuildings.clear();
}

void MapRenderer3DObjectsResource::releaseData()
{
    _sourceData.reset();
}

