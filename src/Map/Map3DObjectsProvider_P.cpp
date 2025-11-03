#include "Map3DObjectsProvider_P.h"

#include <mapbox/earcut.hpp>

#include "MapDataProviderHelpers.h"
#include "Utilities.h"
#include "MapRenderer.h"

#include "IMapTiledSymbolsProvider.h"

using namespace OsmAnd;

Map3DObjectsTiledProvider_P::Map3DObjectsTiledProvider_P(
    Map3DObjectsTiledProvider* const owner_,
    const std::shared_ptr<MapPrimitivesProvider>& tiledProvider)
    : _tiledProvider(tiledProvider)
    , owner(owner_)
{
}

Map3DObjectsTiledProvider_P::~Map3DObjectsTiledProvider_P()
{
}

ZoomLevel Map3DObjectsTiledProvider_P::getMinZoom() const
{
    return _tiledProvider ? _tiledProvider->getMinZoom() : MinZoomLevel;
}

ZoomLevel Map3DObjectsTiledProvider_P::getMaxZoom() const
{
    return _tiledProvider ? _tiledProvider->getMaxZoom() : MaxZoomLevel;
}

bool Map3DObjectsTiledProvider_P::obtainTiledData(
    const IMapTiledDataProvider::Request& request,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    Q_UNUSED(pOutMetric);

    if (!_tiledProvider)
    {
        return false;
    }

    MapPrimitivesProvider::Request tileRequest;
    tileRequest.tileId = request.tileId;
    tileRequest.zoom = request.zoom;
    tileRequest.visibleArea31 = request.visibleArea31;
    tileRequest.queryController = request.queryController;

    std::shared_ptr<MapPrimitivesProvider::Data> tileData;
    if (!_tiledProvider->obtainTiledPrimitives(tileRequest, tileData))
    {
        return false;
    }

    QVector<Building3D> buildings3D;

    if (tileData && tileData->primitivisedObjects)
    {
        for (const auto& primitive : constOf(tileData->primitivisedObjects->polygons))
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

                if (sourceObject->attributeMapping->decodeMap[captionAttributeId].tag == QStringLiteral("height"))
                {
                    height = caption.toFloat();
                }
            }

            const int edgePointsCount = sourceObject->points31.size();
            const float bottomAltitude = -1000.0f;
            const float topAltitude = height;

            const int totalVertices = edgePointsCount * 2;
            const int topTriangles = edgePointsCount - 2;
            const int sideTriangles = edgePointsCount * 2;
            const int totalIndices = (topTriangles + sideTriangles) * 3;

            const auto& bbox = sourceObject->bbox31;
            const uint64_t bboxHash =
                (static_cast<uint64_t>(qHash(bbox.top())) & 0xffffffffULL) ^
                ((static_cast<uint64_t>(qHash(bbox.left())) & 0xffffffffULL) << 1) ^
                ((static_cast<uint64_t>(qHash(bbox.bottom())) & 0xffffffffULL) << 2) ^
                ((static_cast<uint64_t>(qHash(bbox.right())) & 0xffffffffULL) << 3);

            Building3D building;
            building.vertices.reserve(totalVertices);
            building.indices.reserve(totalIndices);
            building.bboxHash = bboxHash;

            for (int i = 0; i < edgePointsCount; ++i)
            {
                const auto& point31 = sourceObject->points31[i];
                building.vertices.append({glm::ivec2(point31.x, point31.y), topAltitude});
            }

            for (int i = 0; i < edgePointsCount; ++i)
            {
                const auto& point31 = sourceObject->points31[i];
                building.vertices.append({glm::ivec2(point31.x, point31.y), bottomAltitude});
            }

            std::vector<std::vector<std::array<int32_t, 2>>> polygon;
            std::vector<std::array<int32_t, 2>> ring;
            ring.reserve(edgePointsCount);

            for (int i = 0; i < edgePointsCount; ++i)
            {
                const auto& point31 = sourceObject->points31[i];
                ring.push_back({point31.x, point31.y});
            }

            polygon.push_back(std::move(ring));

            std::vector<uint16_t> topIndices = mapbox::earcut<uint16_t>(polygon);
            if (topIndices.empty())
            {
                continue;
            }

            for (uint16_t idx : topIndices)
            {
                building.indices.append(idx);
            }

            int bottomStart = edgePointsCount;
            for (int i = 0; i < edgePointsCount; ++i)
            {
                int next = (i + 1) % edgePointsCount;

                building.indices.append(i);
                building.indices.append(bottomStart + i);
                building.indices.append(next);

                building.indices.append(bottomStart + i);
                building.indices.append(bottomStart + next);
                building.indices.append(next);
            }

            buildings3D.push_back(qMove(building));
        }
    }

    outTiledData = std::make_shared<Map3DObjectsTiledProvider::Data>(request.tileId, request.zoom, qMove(buildings3D));
    return true;
}

bool Map3DObjectsTiledProvider_P::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& tiledRequest = static_cast<const IMapTiledDataProvider::Request&>(request);
    std::shared_ptr<IMapTiledDataProvider::Data> tiledData;

    const bool success = obtainTiledData(tiledRequest, tiledData, pOutMetric);
    outData = tiledData;

    return success;
}

bool Map3DObjectsTiledProvider_P::supportsNaturalObtainData() const
{
    return true;
}

bool Map3DObjectsTiledProvider_P::supportsNaturalObtainDataAsync() const
{
    return false;
}


