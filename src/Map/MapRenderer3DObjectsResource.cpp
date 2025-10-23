#include "MapRenderer3DObjectsResource.h"

#include "MapRendererResourcesManager.h"
#include "MapRendererTiledResourcesCollection.h"
#include "IMapTiledDataProvider.h"
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
#include "Map3DObjectsProvider.h"
#include "Utilities.h"
#include "MapRenderer.h"
#include <mapbox/earcut.hpp>
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

        float height = 3.0;

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
                height = caption.toFloat();
            }
        }

        const int edgePointsCount = sourceObject->points31.size();
        const float bottomAltitude = 0.0f;
        const float topAltitude = height;

        // Create vertices for extruded mesh
        QVector<Vertex> extrudedVertices;
        QVector<uint16_t> extrudedIndices;

        // Generate bottom face vertices (for side walls only)
        QVector<Vertex> bottomVertices(edgePointsCount);
        for (int i = 0; i < edgePointsCount; ++i)
        {
            const auto& point31 = sourceObject->points31[i];
            bottomVertices[i].location31 = glm::ivec2(point31.x, point31.y);
            bottomVertices[i].height = static_cast<float>(bottomAltitude);
        }

        // Generate top face vertices
        QVector<Vertex> topVertices(edgePointsCount);
        for (int i = 0; i < edgePointsCount; ++i)
        {
            const auto& point31 = sourceObject->points31[i];
            topVertices[i].location31 = glm::ivec2(point31.x, point31.y);
            topVertices[i].height = static_cast<float>(topAltitude);
        }

        // Triangulate top face using earcut
        std::vector<std::array<int, 2>> topPolygon;
        topPolygon.reserve(edgePointsCount);
        for (int i = 0; i < edgePointsCount; ++i)
        {
            const auto& point31 = sourceObject->points31[i];
            topPolygon.push_back({ point31.x, point31.y });
        }
        std::vector< std::vector<std::array<int, 2>> > polygon;
        polygon.push_back(std::move(topPolygon));

        std::vector<uint32_t> topIndices32 = mapbox::earcut<uint32_t>(polygon);
        if (topIndices32.empty())
            continue;

        // Add top face vertices to extruded mesh
        int vertexOffset = 0;
        for (const auto& vertex : topVertices)
        {
            extrudedVertices.append(vertex);
        }

        // Add top face indices (same winding as original)
        for (uint32_t idx : topIndices32)
        {
            extrudedIndices.append(idx + vertexOffset);
        }

        // Generate side wall faces
        vertexOffset = extrudedVertices.size();
        for (int i = 0; i < edgePointsCount; ++i)
        {
            int next = (i + 1) % edgePointsCount;

            // Add vertices for this side wall quad
            extrudedVertices.append(bottomVertices[i]);     // bottom-left
            extrudedVertices.append(topVertices[i]);       // top-left
            extrudedVertices.append(topVertices[next]);     // top-right
            extrudedVertices.append(bottomVertices[next]);  // bottom-right

            // Add triangle indices for the quad (two triangles)
            int baseIdx = vertexOffset + i * 4;
            // First triangle
            extrudedIndices.append(baseIdx);
            extrudedIndices.append(baseIdx + 1);
            extrudedIndices.append(baseIdx + 2);
            // Second triangle
            extrudedIndices.append(baseIdx);
            extrudedIndices.append(baseIdx + 2);
            extrudedIndices.append(baseIdx + 3);
        }

        const size_t vertexBufferSize = static_cast<size_t>(extrudedVertices.size() * sizeof(Vertex));
        const size_t indexBufferSize = static_cast<size_t>(extrudedIndices.size() * sizeof(uint16_t));

        // Upload vertex buffer
        std::shared_ptr<const GPUAPI::ArrayBufferInGPU> vertexBufferInGPU;
        const bool vertexUploadSuccess = resourcesManager->uploadVerticesToGPU(
            extrudedVertices.constData(),
            vertexBufferSize,
            static_cast<unsigned int>(extrudedVertices.size()),
            vertexBufferInGPU,
            false);

        if (!vertexUploadSuccess)
            continue;

        // Upload index buffer using the wrapper
        std::shared_ptr<const GPUAPI::ElementArrayBufferInGPU> indexBufferInGPU;
        const bool indexUploadSuccess = resourcesManager->uploadIndicesToGPU(
            extrudedIndices.constData(),
            indexBufferSize,
            static_cast<unsigned int>(extrudedIndices.size()),
            indexBufferInGPU,
            false);

        if (!indexUploadSuccess)
            continue;

        float r = static_cast<float>(rand()) / RAND_MAX;
        float g = static_cast<float>(rand()) / RAND_MAX;
        float b = static_cast<float>(rand()) / RAND_MAX;

        TestBuildingResource building(vertexBufferInGPU, indexBufferInGPU, 
                                   static_cast<int>(extrudedVertices.size()), 
                                   static_cast<int>(extrudedIndices.size()), 
                                   glm::vec3(r, g, b));
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

