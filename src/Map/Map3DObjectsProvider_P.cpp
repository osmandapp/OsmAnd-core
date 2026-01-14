#include "Map3DObjectsProvider_P.h"

#include <mapbox/earcut.hpp>
#include <limits>
#include <Polyline2D/LineSegment.h>
#include <Polyline2D/Vec2.h>

#include "MapDataProviderHelpers.h"
#include "Utilities.h"
#include "MapRenderer.h"
#include "MapPresentationEnvironment.h"
#include "MapStyleBuiltinValueDefinitions.h"

#include "IMapTiledSymbolsProvider.h"

using namespace OsmAnd;

namespace OsmAnd
{
    inline uint qHash(const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive) Q_DECL_NOTHROW
    {
        if (!primitive || !primitive->sourceObject)
            return 0;

        const auto& obfMapObject = std::dynamic_pointer_cast<const ObfMapObject>(primitive->sourceObject);
        if (obfMapObject)
        {
            return ::qHash(static_cast<uint64_t>(obfMapObject->id));
        }

        return ::qHash(primitive->sourceObject.get());
    }

    inline bool operator==(const std::shared_ptr<const MapPrimitiviser::Primitive>& lhs,
                           const std::shared_ptr<const MapPrimitiviser::Primitive>& rhs) Q_DECL_NOTHROW
    {
        if (lhs.get() == rhs.get())
            return true;
        if (!lhs || !rhs)
            return false;
        if (!lhs->sourceObject || !rhs->sourceObject)
            return lhs->sourceObject.get() == rhs->sourceObject.get();

        const auto& lhsObj = std::dynamic_pointer_cast<const ObfMapObject>(lhs->sourceObject);
        const auto& rhsObj = std::dynamic_pointer_cast<const ObfMapObject>(rhs->sourceObject);

        if (lhsObj && rhsObj)
        {
            return lhsObj->id == rhsObj->id;
        }

        return lhs->sourceObject.get() == rhs->sourceObject.get();
    }
}

Map3DObjectsTiledProvider_P::Map3DObjectsTiledProvider_P(
    Map3DObjectsTiledProvider* const owner_,
    const std::shared_ptr<MapPrimitivesProvider>& tiledProvider,
    const std::shared_ptr<MapPresentationEnvironment>& environment)
    : _tiledProvider(tiledProvider)
    , _environment(environment)
    , _elevationProvider(nullptr)
    , owner(owner_)
{
    std::vector<std::vector<std::array<float, 2>>> polygon;

    std::vector<std::array<float, 2>> side = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 1.0f},
        {1.0f, 0.0f}
    };

    polygon.push_back(side);
    fullSideIndices = mapbox::earcut<uint16_t>(polygon);

    polygon.clear();

    std::vector<std::array<float, 2>> passageSide = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 1.0f},
        {1.0f, 0.0f},

        {0.75f, 0.0f},
        {0.75f, 0.75f},
        {0.25f, 0.75f},
        {0.25f, 0.25f},
    };

    polygon.push_back(passageSide);
    pasageSideIndices = mapbox::earcut<uint16_t>(polygon);
}

Map3DObjectsTiledProvider_P::~Map3DObjectsTiledProvider_P()
{
}

ZoomLevel Map3DObjectsTiledProvider_P::getMinZoom() const
{
    return ZoomLevel16;
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

    Buildings3D buildings3D;
    if (tileData && tileData->primitivisedObjects)
    {
        std::shared_ptr<IMapElevationDataProvider::Data> elevationData;
        if (_elevationProvider)
        {
            IMapElevationDataProvider::Request elevationRequest;
            elevationRequest.tileId = request.tileId;
            elevationRequest.zoom = request.zoom;
            elevationRequest.queryController = request.queryController;

            _elevationProvider->obtainElevationData(elevationRequest, elevationData, nullptr);
        }

        QSet<std::shared_ptr<const MapPrimitiviser::Primitive>> buildings;
        QSet<std::shared_ptr<const MapPrimitiviser::Primitive>> buildingParts;
        QHash<std::shared_ptr<const MapPrimitiviser::Primitive>, float> buildingPassages;

        for (const auto& primitive : tileData->primitivisedObjects->polygons)
        {
            collectFrompolygons(primitive, buildings, buildingParts);
        }

        for (const auto& primitive : tileData->primitivisedObjects->polylines)
        {
            collectFromPoliline(primitive, buildings, buildingParts, buildingPassages);
        }

        filterBuildings(buildings, buildingParts);

        for (const auto& buildingPart : buildingParts)
        {
            processPrimitive(buildingPart, buildings3D, elevationData, request.tileId, request.zoom, buildingPassages);
        }

        for (const auto& building : buildings)
        {
            processPrimitive(building, buildings3D, elevationData, request.tileId, request.zoom, buildingPassages);
        }
    }

    outTiledData = std::make_shared<Map3DObjectsTiledProvider::Data>(request.tileId, request.zoom, qMove(buildings3D));
    return true;
}

void Map3DObjectsTiledProvider_P::filterBuildings(QSet<std::shared_ptr<const MapPrimitiviser::Primitive>>& buildings,
                                                  QSet<std::shared_ptr<const MapPrimitiviser::Primitive>>& buildingParts) const
{
    const bool show3DbuildingParts = _environment ? _environment->getShow3DBuildingParts() : false;
    if (!show3DbuildingParts)
    {
        return;
    }

    for (auto it = buildings.begin(); it != buildings.end();)
    {
        const auto& buildingPrimitive = *it;
        const auto& building = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(buildingPrimitive->sourceObject);
        if (!building)
        {
            ++it;
            continue;
        }

        if (building->containsTag(QLatin1String("role_outline"), true) ||
            building->containsTag(QLatin1String("role_inner"), true) ||
            building->containsTag(QLatin1String("role_outer"), true))
        {
            it = buildings.erase(it);
            continue;
        }

        if (building->containsAttribute(QLatin1String("layer"), QLatin1String("-1"), true))
        {
            it = buildings.erase(it);
            continue;
        }

        bool shouldRemove = false;
        for (const auto& buildingPartPrimitive : buildingParts)
        {
            const auto& buildingPart = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(buildingPartPrimitive->sourceObject);
            if (buildingPart && building->bbox31.contains(buildingPart->bbox31))
            {
                shouldRemove = true;
                break;
            }
        }

        if (shouldRemove)
        {
            it = buildings.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Map3DObjectsTiledProvider_P::collectFromPoliline(const std::shared_ptr<const MapPrimitiviser::Primitive>& polylinePrimitive,
                                                      QSet<std::shared_ptr<const MapPrimitiviser::Primitive>>& outBuildings,
                                                      QSet<std::shared_ptr<const MapPrimitiviser::Primitive>>& outBuildingParts,
                                                      QHash<std::shared_ptr<const MapPrimitiviser::Primitive>, float>& outBuildingPassages) const
{
    const auto& sourceObject = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(polylinePrimitive->sourceObject);
    if (!sourceObject || sourceObject->points31.isEmpty())
    {
        return;
    }

    if (polylinePrimitive->type == MapPrimitiviser::PrimitiveType::Polygon)
    {
        const bool show3DbuildingParts = _environment ? _environment->getShow3DBuildingParts() : false;

        if (sourceObject->containsTag(QLatin1String("building")))
        {
            const auto primitiveIt = outBuildings.find(polylinePrimitive);

            if (primitiveIt == outBuildings.end())
            {
                outBuildings.insert(polylinePrimitive);
            }
            else
            {
                const uint32_t savedPrimitiveColor = getPolygonColor(*primitiveIt);
                if (savedPrimitiveColor == 0)
                {
                    outBuildings.erase(primitiveIt);
                    outBuildings.insert(polylinePrimitive);
                }
            }
        }
        else if (show3DbuildingParts && sourceObject->containsTag(QLatin1String("building:part")))
        {
            const auto primitiveIt = outBuildingParts.find(polylinePrimitive);

            if (primitiveIt == outBuildingParts.end())
            {
                outBuildingParts.insert(polylinePrimitive);
            }
            else
            {
                const uint32_t savedPrimitiveColor = getPolygonColor(*primitiveIt);
                if (savedPrimitiveColor == 0)
                {
                    outBuildingParts.erase(primitiveIt);
                    outBuildingParts.insert(polylinePrimitive);
                }
            }
        }
    }
    else if (polylinePrimitive->type == MapPrimitiviser::PrimitiveType::Polyline)
    {
        if (sourceObject->containsAttribute(QLatin1String("tunnel"), QLatin1String("building_passage")))
        {
            float width = 2.0;

            const QString tag = QLatin1String("width");
            QString widthValue = sourceObject->getResolvedAttribute(QStringRef(&tag));
            width = OsmAnd::Utilities::parseLength(widthValue, width);

            outBuildingPassages.insert(polylinePrimitive, width);
        }
    }
}

void Map3DObjectsTiledProvider_P::collectFrompolygons(const std::shared_ptr<const MapPrimitiviser::Primitive>& polygonPrimitive,
                                                  QSet<std::shared_ptr<const MapPrimitiviser::Primitive>>& outBuildings,
                                                  QSet<std::shared_ptr<const MapPrimitiviser::Primitive>>& outBuildingParts) const
{
    const auto& sourceObject = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(polygonPrimitive->sourceObject);
    if (!sourceObject || sourceObject->points31.isEmpty())
    {
        return;
    }

    const bool show3DbuildingParts = _environment ? _environment->getShow3DBuildingParts() : false;

    if (sourceObject->containsTag(QLatin1String("building")))
    {
        const auto primitiveIt = outBuildings.find(polygonPrimitive);

        if (primitiveIt == outBuildings.end())
        {
            outBuildings.insert(polygonPrimitive);
        }
        else
        {
            const uint32_t savedPrimitiveColor = getPolygonColor(*primitiveIt);
            if (savedPrimitiveColor == 0)
            {
                outBuildings.erase(primitiveIt);
                outBuildings.insert(polygonPrimitive);
            }
        }
    }
    else if (show3DbuildingParts && sourceObject->containsTag(QLatin1String("building:part")))
    {
        const auto primitiveIt = outBuildingParts.find(polygonPrimitive);

        if (primitiveIt == outBuildingParts.end())
        {
            outBuildingParts.insert(polygonPrimitive);
        }
        else
        {
            const uint32_t savedPrimitiveColor = getPolygonColor(*primitiveIt);
            if (savedPrimitiveColor == 0)
            {
                outBuildingParts.erase(primitiveIt);
                outBuildingParts.insert(polygonPrimitive);
            }
        }
    }
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

void Map3DObjectsTiledProvider_P::processPrimitive(const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive,
    Buildings3D& buildings3D, const std::shared_ptr<IMapElevationDataProvider::Data>& elevationData, const TileId& tileId,
    const ZoomLevel zoom, const QHash<std::shared_ptr<const MapPrimitiviser::Primitive>, float>& passagesData) const
{
    const auto& sourceObject = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(primitive->sourceObject);
    if (!sourceObject)
    {
        return;
    }

    float levelHeight = getDefaultBuildingsLevelHeight();

    float height = getDefaultBuildingsHeight();
    float minHeight = 0.0f;
    float terrainHeight = 0.0f;

    float levels = 0.0;
    float minLevels = 0.0;

    FColorARGB color = getDefaultBuildingsColor();
    const uint32_t polygonColor = getPolygonColor(primitive);
    if (polygonColor != 0)
    {
        color = FColorARGB(polygonColor);
    }

    bool heightFound = false;
    bool minHeightFound = false;
    bool levelsFound = false;
    bool minLevelsFound = false;

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
    }

    if (height == 0.0f)
    {
        return;
    }

    if (!heightFound && levelsFound)
    {
        if (levels == 0)
        {
            return;
        }

        height = levels * levelHeight;
    }

    if (!minHeightFound && minLevelsFound)
    {
        if (minLevels == 0)
        {
            return;
        }

        minHeight = minLevels * levelHeight;
    }

    if (elevationData)
    {
        float maxElevationMeters = 0.0f;
        float minElevationMeters = 0.0f;
        bool hasElevationSample = false;

        for (const auto& p : constOf(sourceObject->points31))
        {
            accumulateElevationForPoint(p, tileId, zoom, elevationData, maxElevationMeters, minElevationMeters, hasElevationSample);
        }

        if (hasElevationSample)
        {
            terrainHeight = maxElevationMeters;
        }
    }

    const glm::vec3 colorVec(color.r, color.g, color.b);

    QVector<PointI> points31 = sourceObject->points31;

    // Reverse points if needed
    double area = Utilities::computeSignedArea(points31);
    bool isClockwise = (area < 0.0);

    if (isClockwise)
    {
        std::reverse(points31.begin(), points31.end());
    }

    const int edgePointsCount = points31.size();

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
            bool innerIsClockwise = (innerArea >= 0.0);

            if (innerIsClockwise)
            {
                std::reverse(innerPoints.begin(), innerPoints.end());
            }

            innerPolygons.append(innerPoints);
        }
    }

    int currentVertexOffset = buildings3D.vertices.size();
    const int currentIndexOffset = buildings3D.indices.size();

    // Construct top side of the mesh
    std::vector<std::vector<std::array<int32_t, 2>>> topPolygon;

    std::vector<std::array<int32_t, 2>> outerRing;
    for (int i = 0; i < edgePointsCount; ++i)
    {
        const auto& p = points31[i];

        outerRing.push_back({p.x, p.y});
        buildings3D.vertices.append({glm::ivec2(p.x, p.y), height, terrainHeight, glm::vec3(0.0f, 1.0f, 0.0f), colorVec});
    }

    topPolygon.push_back(std::move(outerRing));

    for (const auto& innerPoly : innerPolygons)
    {
        std::vector<std::array<int32_t, 2>> innerRing;
        for (const auto& p : innerPoly)
        {
            innerRing.push_back({p.x, p.y});
            buildings3D.vertices.append({glm::ivec2(p.x, p.y), height, terrainHeight, glm::vec3(0.0f, 1.0f, 0.0f), colorVec});
        }

        topPolygon.push_back(std::move(innerRing));
    }

    std::vector<uint16_t> topIndices = mapbox::earcut<uint16_t>(topPolygon);
    for (uint16_t idx : topIndices)
    {
        buildings3D.indices.append(static_cast<uint16_t>(idx + currentVertexOffset));
    }

    // Construct walls of the mesh
    for (int i = 0; i < edgePointsCount; ++i)
    {
        const int next = (i + 1) % edgePointsCount;
        const auto& point31_i = points31[i];
        const auto& point31_next = points31[next];

        generateBuildingWall(buildings3D, point31_i, point31_next, minHeight, height, terrainHeight, colorVec);
    }

    // Construct inner walls of the mesh
    for (int polyIdx = 0; polyIdx < innerPolygons.size(); ++polyIdx)
    {
        const auto& innerPoly = innerPolygons[polyIdx];

        for (int i = 0; i < innerPoly.size(); ++i)
        {
            const int next = (i + 1) % innerPoly.size();
            const auto& point31_i = innerPoly[i];
            const auto& point31_next = innerPoly[next];

            generateBuildingWall(buildings3D, point31_i, point31_next, minHeight, height, terrainHeight, colorVec);
        }
    }

    const int buildingVertexCount = buildings3D.vertices.size() - currentVertexOffset;
    const int buildingIndexCount = buildings3D.indices.size() - currentIndexOffset;
    buildings3D.buildingIDs.append(sourceObject->id.id);
    buildings3D.vertexCounts.append(buildingVertexCount);
    buildings3D.indexCounts.append(buildingIndexCount);
}

void Map3DObjectsTiledProvider_P::accumulateElevationForPoint(
    const PointI& point31,
    const TileId& tileId,
    ZoomLevel zoom,
    const std::shared_ptr<IMapElevationDataProvider::Data>& elevationData,
    float& maxElevationMeters,
    float& minElevationMeters,
    bool& hasElevationSample) const
{
    const auto zoomLevelDelta = MaxZoomLevel - zoom;
    const PointI tile31(tileId.x << zoomLevelDelta, tileId.y << zoomLevelDelta);
    const auto offsetInTile = point31 - tile31;
    const auto tileSize31 = 1u << zoomLevelDelta;
    PointF offsetInTileN(
        static_cast<float>(static_cast<double>(offsetInTile.x) / tileSize31),
        static_cast<float>(static_cast<double>(offsetInTile.y) / tileSize31)
    );

    float elevationInMeters = 0.0f;
    if (elevationData->getValue(offsetInTileN, elevationInMeters))
    {
        if (!hasElevationSample)
        {
            maxElevationMeters = elevationInMeters;
            minElevationMeters = elevationInMeters;
            hasElevationSample = true;
        }
        else
        {
            if (elevationInMeters > maxElevationMeters)
            {
                maxElevationMeters = elevationInMeters;
            }
            if (elevationInMeters < minElevationMeters)
            {
                minElevationMeters = elevationInMeters;
            }
        }
    }
}

void Map3DObjectsTiledProvider_P::setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& elevationProvider)
{
    _elevationProvider = elevationProvider;
}

glm::vec3 Map3DObjectsTiledProvider_P::calculateNormalFrom2Points(PointI point31_i, PointI point31_next) const
{
    const float dx = static_cast<float>(point31_next.x - point31_i.x);
    const float dz = static_cast<float>(point31_next.y - point31_i.y);

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

    return n;
}

void Map3DObjectsTiledProvider_P::generateBuildingWall(
    Buildings3D& buildings3D,
    const PointI& point31_i,
    const PointI& point31_next,
    float minHeight,
    float height,
    float terrainHeight,
    const glm::vec3& colorVec) const
{
    const uint32_t baseVertex = buildings3D.vertices.size();

    const glm::vec3 edgeNormal = calculateNormalFrom2Points(point31_i, point31_next);
    const float minTerrainHeight = minHeight > 0.0 ? terrainHeight : 0.0;

    // Should match the side order (counter-clockwise)
    buildings3D.vertices.append({glm::ivec2(point31_i.x, point31_i.y), minHeight, minTerrainHeight, edgeNormal, colorVec});
    buildings3D.vertices.append({glm::ivec2(point31_i.x, point31_i.y), height, terrainHeight, edgeNormal, colorVec});
    buildings3D.vertices.append({glm::ivec2(point31_next.x, point31_next.y), height, terrainHeight, edgeNormal, colorVec});
    buildings3D.vertices.append({glm::ivec2(point31_next.x, point31_next.y), minHeight, minTerrainHeight, edgeNormal, colorVec});

    for (uint16_t idx : fullSideIndices)
    {
        buildings3D.indices.append(baseVertex + idx);
    }
}

uint32_t Map3DObjectsTiledProvider_P::getPolygonColor(const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive) const
{
    uint32_t colorARGB = 0;
    primitive->evaluationResult.getIntegerValue(_environment->styleBuiltinValueDefs->id_OUTPUT_COLOR, colorARGB);
    return colorARGB;
}
