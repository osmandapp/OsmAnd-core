#include "MapRenderer3DObjectsResource.h"

#include "MapRendererResourcesManager.h"
#include "MapRendererTiledResourcesCollection.h"
#include "IMapTiledDataProvider.h"
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
#include "Map3DObjectsProvider.h"
#include "Utilities.h"
#include "MapRenderer.h"
//#include "AtlasMapRenderer_OpenGL.h"
#include <iostream>

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
    bool ok = false;

    std::shared_ptr<IMapDataProvider> provider;
    if (const auto link_ = link.lock())
    {
        const auto collection = static_cast<MapRendererTiledResourcesCollection*>(&link_->collection);
        ok = resourcesManager->obtainProviderFor(
            static_cast<MapRendererBaseResourcesCollection*>(collection),
            provider);
    }
    if (!ok)
        return false;

    return provider->supportsNaturalObtainDataAsync();
}

bool MapRenderer3DObjectsResource::obtainData(bool& dataAvailable, const std::shared_ptr<const IQueryController>& queryController)
{
    bool ok = false;

    // Get source of tile
    std::shared_ptr<IMapDataProvider> provider_;
    if (const auto link_ = link.lock())
    {
        ok = resourcesManager->obtainProviderFor(
            static_cast<MapRendererBaseResourcesCollection*>(static_cast<MapRendererTiledResourcesCollection*>(&link_->collection)),
            provider_);
    }
    if (!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapTiledDataProvider>(provider_);

    // Obtain tile from provider
    std::shared_ptr<IMapTiledDataProvider::Data> tiledData;
    IMapTiledDataProvider::Request request;
    request.tileId = tileId;
    request.zoom = zoom;
    const auto mapState = resourcesManager->renderer->getMapState();
    const auto visibleArea = Utilities::roundBoundingBox31(mapState.visibleBBox31, mapState.zoomLevel);
    request.visibleArea31 = Utilities::getEnlargedVisibleArea(visibleArea);
    const auto currentTime = QDateTime::currentMSecsSinceEpoch();
    request.areaTime = currentTime;
    request.queryController = queryController;

    const bool requestSucceeded = provider->obtainTiledData(request, tiledData);
    if (!requestSucceeded)
        return false;
    dataAvailable = static_cast<bool>(tiledData);


    if (dataAvailable)
    {
        const auto map3DTileData = std::dynamic_pointer_cast<Map3DObjectsTiledProvider::Data>(tiledData);
        if (map3DTileData)
            _sourceData = map3DTileData->polygons;
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

        int height = 3;

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

        for (const auto& captionAttributeId : constOf(sourceObject->captionsOrder))
        {
            const auto& caption = constOf(sourceObject->captions)[captionAttributeId];

            // TODO: use styles
            if (sourceObject->attributeMapping->decodeMap[captionAttributeId].tag == QStringLiteral("height"))
            {
                height = caption.toInt();
            }
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

        float r = static_cast<float>(rand()) / RAND_MAX;
        float g = static_cast<float>(rand()) / RAND_MAX;
        float b = static_cast<float>(rand()) / RAND_MAX;

        TestBuildingResource building(vertexBufferInGPU, vertexCount, height, glm::vec3(r, g, b));
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

