#include "Map3DObjectsProvider_P.h"

#include <mapbox/earcut.hpp>

#include <OsmAndCore/Data/ObfMapObject.h>

#include "MapDataProviderHelpers.h"
#include "Utilities.h"
#include "MapRenderer.h"
#include "MapPresentationEnvironment.h"

#include "IMapTiledSymbolsProvider.h"

using namespace OsmAnd;

Map3DObjectsTiledProvider_P::Map3DObjectsTiledProvider_P(
    Map3DObjectsTiledProvider* const owner_,
    const std::shared_ptr<MapPrimitivesProvider>& tiledProvider,
    const std::shared_ptr<MapPresentationEnvironment>& environment)
    : _tiledProvider(tiledProvider)
    , _environment(environment)
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
            processPrimitive(primitive, buildings3D, tileData->primitivisedObjects->polygons);
        }

        for (const auto& primitive : constOf(tileData->primitivisedObjects->polylines))
        {
            processPrimitive(primitive, buildings3D, tileData->primitivisedObjects->polylines);
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

bool Map3DObjectsTiledProvider_P::getUseDefaultBuildingsColor() const
{
    return _environment ? _environment->getUseDefaultBuildingColor() : false;
}

float Map3DObjectsTiledProvider_P::getDefaultBuildingsHeight() const
{
    return _environment ? _environment->getDefault3DBuildingHeight() : 3.0f;
}

float Map3DObjectsTiledProvider_P::getDefaultBuildingsLevelHeight() const
{
    return _environment ? _environment->getDefault3DBuildingLevelHeight() : 3.0f;
}

FColorARGB Map3DObjectsTiledProvider_P::getDefaultBuildingsColor() const
{
    return _environment ? _environment->get3DBuildingsColor() : FColorARGB(1.0f, 0.4f, 0.4f, 0.4f);
}

float Map3DObjectsTiledProvider_P::getDefaultBuildingsAlpha() const
{
    if (_environment)
    {
        return _environment->get3DBuildingsColor().a;
    }
    return 1.0f;
}

void Map3DObjectsTiledProvider_P::processPrimitive(const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive, QVector<Building3D>& buildings3D,
    const MapPrimitiviser::PrimitivesCollection& PrimitivesCollection) const
{
    if (primitive->type != MapPrimitiviser::PrimitiveType::Polygon)
    {
        return;
    }

    const auto& sourceObject = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(primitive->sourceObject);
    if (!sourceObject || sourceObject->points31.isEmpty())
    {
        return;
    }

    bool isBuilding = false;
    bool isBuildingPart = false;

    isBuilding = sourceObject->containsTag(QLatin1String("building"));
    isBuildingPart = sourceObject->containsTag(QLatin1String("building:part"));

    if (!isBuilding && !isBuildingPart)
    {
        return;
    }

    bool isEdge = false;
    if (isBuilding && !isBuildingPart)
    {
        if (sourceObject->innerPolygonsPoints31.empty())
        {
            for (int i = 0; i < sourceObject->additionalAttributeIds.size(); ++i)
            {
                const auto pPrimitiveAttribute = sourceObject->resolveAttributeByIndex(i, true);
                if (!pPrimitiveAttribute)
                {
                    continue;
                }

                if (pPrimitiveAttribute->tag == QLatin1String("role_outline") ||
                    pPrimitiveAttribute->tag == QLatin1String("role_inner") ||
                    pPrimitiveAttribute->tag == QLatin1String("role_outer"))
                {
                    isEdge = true;
                    break;
                }
            }

            if (!isEdge)
            {
                for (const auto& primitiveToCheck : constOf(PrimitivesCollection))
                {
                    if (primitiveToCheck->type != MapPrimitiviser::PrimitiveType::Polygon)
                    {
                        continue;
                    }

                    if (primitiveToCheck == primitive)
                    {
                        continue;
                    }

                    if (!primitiveToCheck->sourceObject->containsTag(QLatin1String("building:part")))
                    {
                        continue;
                    }

                    if (sourceObject->bbox31.contains(primitiveToCheck->sourceObject->bbox31))
                    {
                        isEdge = true;
                        break;
                    }
                }
            }
        }
    }

    if (isEdge)
    {
        return;
    }

    float levelHeight = getDefaultBuildingsLevelHeight();

    float height = getDefaultBuildingsHeight();
    float minHeight = 0.0f;

    float levels = 0.0;
    float minLevels = 0.0;

    FColorARGB color = getDefaultBuildingsColor();

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
            height = OsmAnd::Utilities::parseLength(caption, height, &heightFound);
            continue;
        }

        if (!minHeightFound && captionTag == QStringLiteral("min_height"))
        {
            minHeight = OsmAnd::Utilities::parseLength(caption, height, &minHeightFound);
            continue;
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
            continue;
        }

        if (!getUseDefaultBuildingsColor() && !colorFound && captionTag == QStringLiteral("building:colour"))
        {
            colorFound = true;
            color = OsmAnd::Utilities::parseColor(caption, color);
            color.a = getDefaultBuildingsAlpha();
            continue;
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

    QVector<QVector<PointI>> innerPolygons;
    if (!sourceObject->innerPolygonsPoints31.isEmpty())
    {
        for (const auto& innerPolygon : constOf(sourceObject->innerPolygonsPoints31))
        {
            if (innerPolygon.isEmpty())
            {
                continue;
            }

            QVector<PointI> innerPoints = innerPolygon;
            double innerArea = Utilities::computeSignedArea(innerPoints);
            bool innerIsClockwise = (innerArea < 0.0);

            if (innerIsClockwise == isClockwise)
            {
                std::reverse(innerPoints.begin(), innerPoints.end());
            }

            innerPolygons.append(innerPoints);
        }
    }

    const int edgePointsCount = points31.size();

    int totalTopVertices = edgePointsCount;
    for (const auto& innerPoly : innerPolygons)
    {
        totalTopVertices += innerPoly.size();
    }
    
    const int totalVertices = totalTopVertices + edgePointsCount * 4;
    const int topTriangles = edgePointsCount - 2;
    const int sideTriangles = edgePointsCount * 2;
    const int totalIndices = (topTriangles + sideTriangles) * 3;

    Building3D building;
    building.vertices.reserve(totalVertices);
    building.indices.reserve(totalIndices);
    building.id = sourceObject->id.id;
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

    for (const auto& innerPoly : innerPolygons)
    {
        for (const auto& point31 : innerPoly)
        {
            building.vertices.append({glm::ivec2(point31.x, point31.y), height, glm::vec3(0.0f, 1.0f, 0.0f)});
        }
    }

    const int wallVertexStart = totalTopVertices;
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
    
    std::vector<std::array<int32_t, 2>> outerRing;
    outerRing.reserve(edgePointsCount);
    for (int i = 0; i < edgePointsCount; ++i)
    {
        const auto& p = points31[i];
        outerRing.push_back({p.x, p.y});
    }
    polygon.push_back(std::move(outerRing));

    for (const auto& innerPoly : innerPolygons)
    {
        std::vector<std::array<int32_t, 2>> innerRing;
        innerRing.reserve(innerPoly.size());
        for (const auto& p : innerPoly)
        {
            innerRing.push_back({p.x, p.y});
        }
        polygon.push_back(std::move(innerRing));
    }

    std::vector<uint16_t> topIndices = mapbox::earcut<uint16_t>(polygon);
    if (topIndices.empty())
    {
        return;
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
