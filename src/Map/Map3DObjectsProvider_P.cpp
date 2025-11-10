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
    QSet<std::shared_ptr<const MapObject>> filter;

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

            if (filter.contains(sourceObject))
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

                if (pPrimitiveAttribute->tag == QLatin1String("building") ||
                    pPrimitiveAttribute->tag == QLatin1String("building:part"))
                {
                    isBuilding = true;
                    break;
                }
            }

            if (!isBuilding)
            {
                continue;
            }

            filter.insert(sourceObject);

            float levelHeight = 3.0f;

            float height = 3.0f;
            float minHeight = 0.0f;

            float levels = 0;
            float minLevels = 0;

            FColorARGB color(1.0f, 0.4f, 0.4f, 0.4f);

            bool heightFound = false;
            bool minHeightFound = false;
            bool levelsFound = false;
            bool minLevelsFound = false;
            bool colorFound = false;

            for (const auto& captionAttributeId : constOf(sourceObject->captionsOrder))
            {
                const auto& caption = constOf(sourceObject->captions)[captionAttributeId];
                const QString& captionTag = sourceObject->attributeMapping->decodeMap[captionAttributeId].tag;

                if (!heightFound && captionTag == QStringLiteral("height"))
                {
                    heightFound = true;
                    height = caption.toFloat();
                }

                if (!minHeightFound && captionTag == QStringLiteral("min_height"))
                {
                    minHeightFound = true;
                    minHeight = caption.toFloat();
                }

                if (!heightFound && !levelsFound && captionTag == QStringLiteral("building:levels"))
                {
                    levelsFound = true;
                    levels = caption.toFloat();
                }

                if (!minHeightFound && !minLevelsFound && captionTag == QStringLiteral("building:min_level"))
                {
                    minLevelsFound = true;
                    minLevels = caption.toFloat();
                }

                if (!colorFound && captionTag == QStringLiteral("building:colour"))
                {
                    colorFound = true;
                    color = OsmAnd::Utilities::parseColor(caption, color);
                }
            }

            if (!heightFound && levelsFound)
            {
                height = levels * levelHeight;
            }

            if (!minHeightFound && minLevelsFound)
            {
                minHeight = minLevels * levelHeight;
            }

            QVector<PointI> points31 = sourceObject->points31;

            double area = Utilities::computeSignedArea(points31);
            bool isClockwise = (area < 0.0);

            if (isClockwise)
            {
                std::reverse(points31.begin(), points31.end());
            }

            const int edgePointsCount = points31.size();

            const int totalVertices = edgePointsCount + edgePointsCount * 4;
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
            building.color = color;

            QVector<glm::vec3> sideNormals;
            sideNormals.resize(edgePointsCount);
            for (int i = 0; i < edgePointsCount; ++i)
            {
                const int next = (i + 1) % edgePointsCount;
                const auto& p0 = points31[i];
                const auto& p1 = points31[next];
                const float dx = static_cast<float>(p1.x - p0.x);
                const float dz = static_cast<float>(p1.y - p0.y);
                glm::vec3 n(-dz, 0.0f, dx);
                const float len = glm::length(n);
                if (len > 0.0f)
                {
                    n /= len;
                }
                else
                {
                    n = glm::vec3(0.0f, 0.0f, 1.0f);
                }

                sideNormals[i] = n;
            }

            for (int i = 0; i < edgePointsCount; ++i)
            {
                const auto& point31 = points31[i];
                building.vertices.append({glm::ivec2(point31.x, point31.y), height, glm::vec3(0.0f, 1.0f, 0.0f)});
            }

            const int wallVertexStart = edgePointsCount;
            for (int i = 0; i < edgePointsCount; ++i)
            {
                const int next = (i + 1) % edgePointsCount;
                const glm::vec3& edgeNormal = sideNormals[i];

                const auto& point31_i = points31[i];
                building.vertices.append({glm::ivec2(point31_i.x, point31_i.y), height, edgeNormal});
                building.vertices.append({glm::ivec2(point31_i.x, point31_i.y), minHeight, edgeNormal});

                const auto& point31_next = points31[next];
                building.vertices.append({glm::ivec2(point31_next.x, point31_next.y), height, edgeNormal});
                building.vertices.append({glm::ivec2(point31_next.x, point31_next.y), minHeight, edgeNormal});
            }

            std::vector<std::vector<std::array<int32_t, 2>>> polygon;
            std::vector<std::array<int32_t, 2>> ring;
            ring.reserve(edgePointsCount);

            for (int i = 0; i < edgePointsCount; ++i)
            {
                const auto& p = points31[i];
                ring.push_back({p.x, p.y});
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

            for (int i = 0; i < edgePointsCount; ++i)
            {
                const int baseIdx = wallVertexStart + 4 * i;

                building.indices.append(baseIdx + 0);
                building.indices.append(baseIdx + 1);
                building.indices.append(baseIdx + 2);

                building.indices.append(baseIdx + 1);
                building.indices.append(baseIdx + 3);
                building.indices.append(baseIdx + 2);
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


