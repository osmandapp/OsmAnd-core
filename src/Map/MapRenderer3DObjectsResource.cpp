#include "MapRenderer3DObjectsResource.h"

#include "MapRendererResourcesManager.h"
#include "MapRendererKeyedResourcesCollection.h"
#include "IMapKeyedDataProvider.h"
#include "Map3DObjectsKeyedProvider.h"
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
#include "Utilities.h"
#include "MapRenderer.h"
//#include "AtlasMapRenderer_OpenGL.h"

using namespace OsmAnd;

MapRenderer3DObjectsResource::MapRenderer3DObjectsResource(
    MapRendererResourcesManager* const owner,
    const KeyedEntriesCollection<MapRendererBaseKeyedResource::Key, MapRendererBaseKeyedResource>& collection,
    const MapRendererBaseKeyedResource::Key key)
    : MapRendererBaseKeyedResource(owner, MapRendererResourceType::Map3DObjects, collection, key)
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
            static_cast<MapRendererBaseResourcesCollection*>(static_cast<MapRendererKeyedResourcesCollection*>(&link_->collection)),
            provider_);
    }
    if (!ok)
        return false;
    
    const auto provider = std::dynamic_pointer_cast<IMapKeyedDataProvider>(provider_);
    if (!provider)
        return false;


    const auto& mapState = resourcesManager->renderer->getMapState();

    IMapKeyedDataProvider::Request request;
    request.key = key;
    request.mapState = mapState;
    request.queryController = queryController;

    std::shared_ptr<IMapKeyedDataProvider::Data> keyedData;
    const bool requestSucceeded = provider->obtainKeyedData(request, keyedData);
    
    if (queryController->isAborted())
        return false;
        
    dataAvailable = requestSucceeded && keyedData;
    if (dataAvailable)
    {
        const auto map3DData = std::dynamic_pointer_cast<Map3DObjectsKeyedProvider::Data>(keyedData);
        if (map3DData)
        {
            _sourceData = map3DData->polygons;
        }
    }

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
    if (_sourceData.isEmpty())
        return true;

    _testBuildings.clear();

    for (const auto& primitive : constOf(_sourceData))
    {
        if (primitive->type != MapPrimitiviser::PrimitiveType::Polygon)
        {
            continue;
        }

        const auto& sourceObject = primitive->sourceObject;
        if (!sourceObject || sourceObject->points31.isEmpty())
        {
            continue;
        }

        bool isBuilding = false;
        for (int i = 0; i < sourceObject->attributeIds.size(); ++i)
        {
            const auto pPrimitiveAttribute = sourceObject->resolveAttributeByIndex(i);
            if (!pPrimitiveAttribute)
            {
                continue;
            }

            // TODO: use styles
            //std::cout << "pPrimitiveAttribute->tag: " << pPrimitiveAttribute->tag.toStdString() << '\n';
            if (pPrimitiveAttribute->tag == QLatin1String("addr:housenumber") ||
                pPrimitiveAttribute->tag == QLatin1String("building"))
            {
                isBuilding = true;
                break;
            }
        }

        if (!isBuilding)
        {
            continue;
        }

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

    _sourceData.clear();
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
    _sourceData.clear();
}

