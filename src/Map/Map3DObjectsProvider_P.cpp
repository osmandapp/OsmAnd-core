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

// One meter possible inaccuracy of passage coordinates
#define MAX_WALL_TOUCH_PASSAGE_INACCURACY 50

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
            return false;

        const auto& lhsObj = std::dynamic_pointer_cast<const ObfMapObject>(lhs->sourceObject);
        const auto& rhsObj = std::dynamic_pointer_cast<const ObfMapObject>(rhs->sourceObject);

        if (lhsObj && rhsObj)
        {
            return lhsObj->id == rhsObj->id;
        }

        return lhs->sourceObject.get() == rhs->sourceObject.get();
    }

    inline uint qHash(const OsmAnd::Map3DObjectsTiledProvider_P::BuildingPrimitive& primitive) Q_DECL_NOTHROW
    {
        return qHash(primitive.primitive);
    }

    inline bool operator==(const OsmAnd::Map3DObjectsTiledProvider_P::BuildingPrimitive& lhs,
                           const OsmAnd::Map3DObjectsTiledProvider_P::BuildingPrimitive& rhs) Q_DECL_NOTHROW
    {
        return lhs.primitive == rhs.primitive;
    }

    inline uint qHash(const OsmAnd::Map3DObjectsTiledProvider_P::PassagePrimitive& primitive) Q_DECL_NOTHROW
    {
        return qHash(primitive.primitive);
    }

    inline bool operator==(const OsmAnd::Map3DObjectsTiledProvider_P::PassagePrimitive& lhs,
                           const OsmAnd::Map3DObjectsTiledProvider_P::PassagePrimitive& rhs) Q_DECL_NOTHROW
    {
        return lhs.primitive == rhs.primitive;
    }

}

Map3DObjectsTiledProvider_P::Map3DObjectsTiledProvider_P(
    Map3DObjectsTiledProvider* const owner_,
    const std::shared_ptr<MapPrimitivesProvider>& tiledProvider,
    const std::shared_ptr<MapPresentationEnvironment>& environment,
    const bool useCustomColor,
    const FColorRGB& customColor)
    : _tiledProvider(tiledProvider)
    , _environment(environment)
    , _useCustomColor(useCustomColor)
    , _customColor(customColor)
    , _elevationProvider(nullptr)
    , owner(owner_)
{
    std::vector<std::vector<std::array<float, 2>>> polygon;

    std::vector<std::array<float, 2>> side = {
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {0.0f, 0.0f}
    };

    polygon.push_back(side);
    fullSideIndices = mapbox::earcut<uint16_t>(polygon);
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

    if (request.zoom < 16)
    {
        outTiledData = std::make_shared<Map3DObjectsTiledProvider::Data>(request.tileId, request.zoom, Buildings3D());
        return true;
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

        QSet<BuildingPrimitive> buildings;
        QSet<BuildingPrimitive> buildingParts;
        QSet<PassagePrimitive> buildingPassages;

        for (const auto& primitive : tileData->primitivisedObjects->polygons)
        {
            collectFromPolygons(primitive, buildings, buildingParts);
        }

        for (const auto& primitive : tileData->primitivisedObjects->polylines)
        {
            collectFromPolyline(primitive, buildings, buildingParts, buildingPassages);
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

void Map3DObjectsTiledProvider_P::filterBuildings(QSet<BuildingPrimitive>& buildings,
                                                  QSet<BuildingPrimitive>& buildingParts) const
{
    const bool show3DbuildingParts = _environment ? _environment->getShow3DBuildingParts() : false;
    if (!show3DbuildingParts)
    {
        return;
    }

    for (auto it = buildings.begin(); it != buildings.end();)
    {
        const auto& buildingPrimitive = *it;
        const auto& building = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(buildingPrimitive.primitive->sourceObject);
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
        QVector<QPair<BuildingPrimitive, BuildingPrimitive>> itemsToModify;
        for (auto it = buildingParts.begin(); it != buildingParts.end(); ++it)
        {
            const auto& buildingPartPrimitive = *it;
            const auto& buildingPart = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(buildingPartPrimitive.primitive->sourceObject);
            if (buildingPart && building->bbox31.contains(buildingPart->bbox31))
            {
                if (buildingPartPrimitive.polygonColor == 0)
                {
                    BuildingPrimitive modifiedBuildingPart = buildingPartPrimitive;
                    modifiedBuildingPart.polygonColor = buildingPrimitive.polygonColor;
                    itemsToModify.append(qMakePair(buildingPartPrimitive, modifiedBuildingPart));
                }

                shouldRemove = true;
            }
        }

        for (const auto& pair : itemsToModify)
        {
            buildingParts.remove(pair.first);
            buildingParts.insert(pair.second);
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

void Map3DObjectsTiledProvider_P::insertOrUpdateBuilding(
    const BuildingPrimitive& primitive, QSet<BuildingPrimitive>& outCollection) const
{
    auto primitiveIt = outCollection.find(primitive);

    if (primitiveIt == outCollection.end())
    {
        outCollection.insert(primitive);
    }
    else
    {
        if (primitiveIt->polygonColor == 0)
        {
            outCollection.remove(*primitiveIt);
            outCollection.insert(primitive);
        }
    }
}

void Map3DObjectsTiledProvider_P::insertOrUpdatePassage(
    const PassagePrimitive& primitive, QSet<PassagePrimitive>& outCollection) const
{
    auto primitiveIt = outCollection.find(primitive);

    if (primitiveIt == outCollection.end())
    {
        outCollection.insert(primitive);
    }
}

void Map3DObjectsTiledProvider_P::collectFromPolyline(
    const std::shared_ptr<const MapPrimitiviser::Primitive>& polylinePrimitive,
    QSet<BuildingPrimitive>& outBuildings,
    QSet<BuildingPrimitive>& outBuildingParts,
    QSet<PassagePrimitive>& outBuildingPassages) const
{
    const auto& sourceObject = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(polylinePrimitive->sourceObject);
    if (!sourceObject || sourceObject->points31.isEmpty())
    {
        return;
    }

    if (polylinePrimitive->type == MapPrimitiviser::PrimitiveType::Polygon)
    {
        BuildingPrimitive buildingPrimitive;
        buildingPrimitive.primitive = polylinePrimitive;
        buildingPrimitive.polygonColor = getPolygonColor(buildingPrimitive.primitive);

        const bool show3DbuildingParts = _environment ? _environment->getShow3DBuildingParts() : false;

        if (sourceObject->containsTag(QLatin1String("building")))
        {
            insertOrUpdateBuilding(buildingPrimitive, outBuildings);
        }
        else if (show3DbuildingParts && sourceObject->containsTag(QLatin1String("building:part")))
        {
            insertOrUpdateBuilding(buildingPrimitive, outBuildingParts);
        }
    }
    else if (polylinePrimitive->type == MapPrimitiviser::PrimitiveType::Polyline)
    {
        if (sourceObject->points31.size() > 1
            && sourceObject->containsAttribute(QLatin1String("tunnel"), QLatin1String("building_passage")))
        {
            PassagePrimitive passagePrimitive;
            passagePrimitive.primitive = polylinePrimitive;
            passagePrimitive.startingPoint = sourceObject->points31.first();
            passagePrimitive.endingPoint = sourceObject->points31.last();
            passagePrimitive.centerPoint = passagePrimitive.startingPoint / 2 + passagePrimitive.endingPoint / 2;
            const QString widthTag = QLatin1String("width");
            const QString heightTag = QLatin1String("height");
            passagePrimitive.height =
                Utilities::parseLength(sourceObject->getResolvedAttribute(QStringRef(&heightTag)), 2.5);
            const auto width = Utilities::parseLength(sourceObject->getResolvedAttribute(QStringRef(&widthTag)), 2.0);
            passagePrimitive.halfWidth31 =
                qRound(width / (2.0f * Utilities::getMetersPer31Coordinate(passagePrimitive.centerPoint)));
            passagePrimitive.putStart = false;
            passagePrimitive.withStart = false;
            passagePrimitive.putEnd = false;
            passagePrimitive.withEnd = false;
            insertOrUpdatePassage(passagePrimitive, outBuildingPassages);
        }
    }
}

void Map3DObjectsTiledProvider_P::collectFromPolygons(
    const std::shared_ptr<const MapPrimitiviser::Primitive>& polygonPrimitive,
    QSet<BuildingPrimitive>& outBuildings,
    QSet<BuildingPrimitive>& outBuildingParts) const
{
    BuildingPrimitive buildingPrimitive;
    buildingPrimitive.primitive = polygonPrimitive;
    buildingPrimitive.polygonColor = getPolygonColor(buildingPrimitive.primitive);

    const auto& sourceObject = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(polygonPrimitive->sourceObject);
    if (!sourceObject || sourceObject->points31.isEmpty())
    {
        return;
    }

    const bool show3DbuildingParts = _environment ? _environment->getShow3DBuildingParts() : false;

    if (sourceObject->containsTag(QLatin1String("building")))
    {
        insertOrUpdateBuilding(buildingPrimitive, outBuildings);
    }
    else if (show3DbuildingParts && sourceObject->containsTag(QLatin1String("building:part")))
    {
        insertOrUpdateBuilding(buildingPrimitive, outBuildingParts);
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

void Map3DObjectsTiledProvider_P::processPrimitive(const BuildingPrimitive& primitive,
    Buildings3D& buildings3D, const std::shared_ptr<IMapElevationDataProvider::Data>& elevationData, const TileId& tileId,
    const ZoomLevel zoom, QSet<PassagePrimitive>& buildingPassages) const
{
    const auto& sourceObject = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(primitive.primitive->sourceObject);
    if (!sourceObject)
    {
        return;
    }

    const bool show3DbuildingOutline = false;//_environment ? _environment->getShow3DBuildingOutline() : false;

    float levelHeight = getDefaultBuildingsLevelHeight();

    float height = getDefaultBuildingsHeight();
    float minHeight = 0.0f;
    float terrainHeight = 0.0f;
    float baseTerrainHeight = 0.0f;

    float levels = 0.0;
    float minLevels = 0.0;

    auto color = _useCustomColor ? _customColor : FColorRGB(getUseDefaultBuildingsColor()
        || primitive.polygonColor == 0 ? getDefaultBuildingsColor() : FColorARGB(primitive.polygonColor));

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
            baseTerrainHeight = minElevationMeters;
        }
    }

    QList<PassagePrimitive*> passages;
    if (minHeight > 0.0f)
        baseTerrainHeight = terrainHeight;
    else
    {
        for (auto& passagePrimitive : buildingPassages)
        {
            if (sourceObject->bbox31.contains(passagePrimitive.centerPoint))
                passages.append(const_cast<PassagePrimitive*>(&passagePrimitive));
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
    const int currentOutlineVertexOffset = buildings3D.outlineVertices.size();
    const int currentIndexOffset = buildings3D.indices.size();

    // Construct top side of the mesh
    std::vector<std::vector<std::array<int32_t, 2>>> topPolygon;

    std::vector<std::array<int32_t, 2>> outerRing;
    for (int i = 0; i < edgePointsCount; ++i)
    {
        const int next = (i + 1) % edgePointsCount;
        const auto& p = points31[i];
        BuildingVertex v{glm::ivec2(p.x, p.y), height, terrainHeight, glm::vec3(0.0f, 1.0f, 0.0f), colorVec};

        outerRing.push_back({p.x, p.y});
        buildings3D.vertices.append(v);
        if (show3DbuildingOutline)
        {
            buildings3D.outlineVertices.append(v);
            buildings3D.outlineIndices.append(static_cast<uint16_t>(currentOutlineVertexOffset + i));
            buildings3D.outlineIndices.append(static_cast<uint16_t>(currentOutlineVertexOffset + next));
        }
    }

    topPolygon.push_back(std::move(outerRing));

    for (const auto& innerPoly : innerPolygons)
    {
        std::vector<std::array<int32_t, 2>> innerRing;

        int innerOutlineStart = 0;
        if (show3DbuildingOutline)
        {
            innerOutlineStart = buildings3D.outlineVertices.size();
        }

        for (const auto& p : innerPoly)
        {
            BuildingVertex v{glm::ivec2(p.x, p.y), height, terrainHeight, glm::vec3(0.0f, 1.0f, 0.0f), colorVec};

            innerRing.push_back({p.x, p.y});
            buildings3D.vertices.append(v);
            if (show3DbuildingOutline)
            {
                buildings3D.outlineVertices.append(v);
            }
        }

        if (show3DbuildingOutline)
        {
            const int innerOutlineCount = buildings3D.outlineVertices.size() - innerOutlineStart;
            for (int i = 0; i < innerOutlineCount; ++i)
            {
                const int next = (i + 1) % innerOutlineCount;
                buildings3D.outlineIndices.append(static_cast<uint16_t>(innerOutlineStart + i));
                buildings3D.outlineIndices.append(static_cast<uint16_t>(innerOutlineStart + next));
            }
        }

        topPolygon.push_back(std::move(innerRing));
    }

    const auto zoomLevelDelta = MaxZoomLevel - zoom;
    const PointI minTile31(tileId.x << zoomLevelDelta, tileId.y << zoomLevelDelta);
    const PointI maxTile31((tileId.x + 1) << zoomLevelDelta, (tileId.y + 1) << zoomLevelDelta);

    cutMeshForTile(
        mapbox::earcut<uint16_t>(topPolygon),
        buildings3D.vertices,
        buildings3D.indices,
        currentVertexOffset,
        minTile31,
        maxTile31);

    // Construct walls of the mesh
    for (int i = 0; i < edgePointsCount; ++i)
    {
        const int next = (i + 1) % edgePointsCount;
        const auto& point31_i = points31[i];
        const auto& point31_next = points31[next];
        const auto edgeNormal = calculateNormalFrom2Points(point31_i, point31_next);

        QList<PassagePrimitive*> passagePrimitives;
        for (auto passage : passages)
        {
            const auto side = glm::dvec2(point31_next.x - point31_i.x, point31_next.y - point31_i.y);
            const auto sideLengthSqr = side.x * side.x + side.y * side.y;
            const auto halfN = static_cast<double>(passage->halfWidth31) / qSqrt(sideLengthSqr);
            if (!passage->putStart && !passage->withStart)
            {
                const auto distN = glm::dot(glm::dvec2(
                    passage->startingPoint.x - point31_i.x,
                    passage->startingPoint.y - point31_i.y), side) / sideLengthSqr;
                if (distN > 0.0 && distN < 1.0 && distN > halfN && (1.0 - distN) > halfN)
                {
                    auto proj = side * distN;
                    auto proj31 = PointI(qRound(proj.x), qRound(proj.y));
                    const auto point31 = point31_i + proj31;
                    if ((point31 - passage->startingPoint).norm() < MAX_WALL_TOUCH_PASSAGE_INACCURACY)
                    {
                        proj = side * halfN;
                        proj31 = PointI(qRound(proj.x), qRound(proj.y));
                        const auto pointLeft = point31 - proj31;
                        passage->startBottomLeft = BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                            minHeight, baseTerrainHeight, edgeNormal, colorVec};
                        passage->startTopLeft = BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                            passage->height, terrainHeight, edgeNormal, colorVec};
                        const auto pointRight = point31 + proj31;
                        passage->startBottomRight = BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                            minHeight, baseTerrainHeight, edgeNormal, colorVec};
                        passage->startTopRight = BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                            passage->height, terrainHeight, edgeNormal, colorVec};
                        passage->putStart = true;
                        passage->withStart = true;
                        passagePrimitives.append(passage);
                        break;
                    }
                }
            }
            if (!passage->putEnd && !passage->withEnd)
            {
                const auto distN = glm::dot(glm::dvec2(
                    passage->endingPoint.x - point31_i.x,
                    passage->endingPoint.y - point31_i.y), side) / sideLengthSqr;
                if (distN > 0.0 && distN < 1.0 && distN > halfN && (1.0 - distN) > halfN)
                {
                    auto proj = side * distN;
                    auto proj31 = PointI(qRound(proj.x), qRound(proj.y));
                    const auto point31 = point31_i + proj31;
                    if ((point31 - passage->endingPoint).norm() < MAX_WALL_TOUCH_PASSAGE_INACCURACY)
                    {
                        proj = side * halfN;
                        proj31 = PointI(qRound(proj.x), qRound(proj.y));
                        const auto pointLeft = point31 - proj31;
                        passage->endBottomLeft = BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                            minHeight, baseTerrainHeight, edgeNormal, colorVec};
                        passage->endTopLeft = BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                            passage->height, terrainHeight, edgeNormal, colorVec};
                        const auto pointRight = point31 + proj31;
                        passage->endBottomRight = BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                            minHeight, baseTerrainHeight, edgeNormal, colorVec};
                        passage->endTopRight = BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                            passage->height, terrainHeight, edgeNormal, colorVec};
                        passage->putEnd = true;
                        passage->withEnd = true;
                        passagePrimitives.append(passage);
                        break;
                    }
                }
            }
        }

        generateBuildingWall(
            buildings3D,
            point31_i,
            point31_next,
            edgeNormal,
            minHeight,
            height,
            baseTerrainHeight,
            terrainHeight,
            minTile31,
            maxTile31,
            colorVec,
            show3DbuildingOutline,
            passagePrimitives);
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
            const auto edgeNormal = calculateNormalFrom2Points(point31_i, point31_next);

            QList<PassagePrimitive*> passagePrimitives;
            for (auto passage : passages)
            {
                const auto side = glm::dvec2(point31_next.x - point31_i.x, point31_next.y - point31_i.y);
                const auto sideLengthSqr = side.x * side.x + side.y * side.y;
                const auto halfN = static_cast<double>(passage->halfWidth31) / qSqrt(sideLengthSqr);
                if (!passage->putStart && !passage->withStart)
                {
                    const auto distN = glm::dot(glm::dvec2(
                        passage->startingPoint.x - point31_i.x,
                        passage->startingPoint.y - point31_i.y), side) / sideLengthSqr;
                    if (distN > 0.0 && distN < 1.0 && distN > halfN && (1.0 - distN) > halfN)
                    {
                        auto proj = side * distN;
                        auto proj31 = PointI(qRound(proj.x), qRound(proj.y));
                        const auto point31 = point31_i + proj31;
                        if ((point31 - passage->startingPoint).norm() < MAX_WALL_TOUCH_PASSAGE_INACCURACY)
                        {
                            proj = side * halfN;
                            proj31 = PointI(qRound(proj.x), qRound(proj.y));
                            const auto pointLeft = point31 - proj31;
                            passage->startBottomLeft = BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                                minHeight, baseTerrainHeight, edgeNormal, colorVec};
                            passage->startTopLeft = BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                                passage->height, terrainHeight, edgeNormal, colorVec};
                            const auto pointRight = point31 + proj31;
                            passage->startBottomRight = BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                                minHeight, baseTerrainHeight, edgeNormal, colorVec};
                            passage->startTopRight = BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                                passage->height, terrainHeight, edgeNormal, colorVec};
                            passage->putStart = true;
                            passage->withStart = true;
                            passagePrimitives.append(passage);
                            break;
                        }
                    }
                }
                if (!passage->putEnd && !passage->withEnd)
                {
                    const auto distN = glm::dot(glm::dvec2(
                        passage->endingPoint.x - point31_i.x,
                        passage->endingPoint.y - point31_i.y), side) / sideLengthSqr;
                    if (distN > 0.0 && distN < 1.0 && distN > halfN && (1.0 - distN) > halfN)
                    {
                        auto proj = side * distN;
                        auto proj31 = PointI(qRound(proj.x), qRound(proj.y));
                        const auto point31 = point31_i + proj31;
                        if ((point31 - passage->endingPoint).norm() < MAX_WALL_TOUCH_PASSAGE_INACCURACY)
                        {
                            proj = side * halfN;
                            proj31 = PointI(qRound(proj.x), qRound(proj.y));
                            const auto pointLeft = point31 - proj31;
                            passage->endBottomLeft = BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                                minHeight, baseTerrainHeight, edgeNormal, colorVec};
                            passage->endTopLeft = BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                                passage->height, terrainHeight, edgeNormal, colorVec};
                            const auto pointRight = point31 + proj31;
                            passage->endBottomRight = BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                                minHeight, baseTerrainHeight, edgeNormal, colorVec};
                            passage->endTopRight = BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                                passage->height, terrainHeight, edgeNormal, colorVec};
                            passage->putEnd = true;
                            passage->withEnd = true;
                            passagePrimitives.append(passage);
                            break;
                        }
                    }
                }
            }

            generateBuildingWall(
                buildings3D,
                point31_i,
                point31_next,
                edgeNormal,
                minHeight,
                height,
                baseTerrainHeight,
                terrainHeight,
                minTile31,
                maxTile31,
                colorVec,
                show3DbuildingOutline,
                passagePrimitives);
        }
    }

    for (auto passage : passages)
    {
        if (!passage->withStart || !passage->withEnd || passage->putStart || passage->putEnd)
            continue;

        auto& points31 = passage->primitive->sourceObject->points31;
        int segCount = points31.size() - 1;
        if (segCount == 1)
        {
            uint32_t baseVertexOffset = buildings3D.vertices.size();

            buildings3D.vertices.append(passage->endBottomRight);
            buildings3D.vertices.append(passage->endTopRight);
            buildings3D.vertices.append(passage->startTopLeft);
            buildings3D.vertices.append(passage->startBottomLeft);

            cutMeshForTile(
                fullSideIndices,
                buildings3D.vertices,
                buildings3D.indices,
                baseVertexOffset,
                minTile31,
                maxTile31);

            baseVertexOffset = buildings3D.vertices.size();

            buildings3D.vertices.append(passage->startBottomRight);
            buildings3D.vertices.append(passage->startTopRight);
            buildings3D.vertices.append(passage->endTopLeft);
            buildings3D.vertices.append(passage->endBottomLeft);

            cutMeshForTile(
                fullSideIndices,
                buildings3D.vertices,
                buildings3D.indices,
                baseVertexOffset,
                minTile31,
                maxTile31);

            baseVertexOffset = buildings3D.vertices.size();

            buildings3D.vertices.append(passage->endTopRight);
            buildings3D.vertices.append(passage->endTopLeft);
            buildings3D.vertices.append(passage->startTopRight);
            buildings3D.vertices.append(passage->startTopLeft);

            cutMeshForTile(
                fullSideIndices,
                buildings3D.vertices,
                buildings3D.indices,
                baseVertexOffset,
                minTile31,
                maxTile31);
        }
        else
        {
            auto startBottomRight = passage->startBottomRight;
            auto startTopRight = passage->startTopRight;
            auto startTopLeft = passage->startTopLeft;
            auto startBottomLeft = passage->startBottomLeft;
            BuildingVertex endBottomRight, endTopRight, endTopLeft, endBottomLeft;
            auto prev = PointD(points31[1] - points31[0]).normalized();
            for (int i = 0; i < segCount; i++)
            {
                if (i + 1 == segCount)
                {
                    endBottomRight = passage->endBottomLeft;
                    endTopRight = passage->endTopLeft;
                    endTopLeft = passage->endTopRight;
                    endBottomLeft = passage->endBottomRight;
                }
                else
                {
                    const auto endRange = static_cast<double>(i + 1) / static_cast<double>(segCount);
                    endBottomRight = getIntersection(passage->startBottomRight, passage->endBottomLeft, endRange);
                    endTopRight = getIntersection(passage->startTopRight, passage->endTopLeft, endRange);
                    endTopLeft = getIntersection(passage->startTopLeft, passage->endTopRight, endRange);
                    endBottomLeft = getIntersection(passage->startBottomLeft, passage->endBottomRight, endRange);
                    const auto next = PointD(points31[i + 2] - points31[i + 1]).normalized();
                    const auto mN = (PointD(prev.y, -prev.x) + PointD(next.y, -next.x)).normalized();
                    const auto mid = mN * static_cast<double>(passage->halfWidth31)
                        / qMax(glm::dot(glm::dvec2(mN.x, mN.y), glm::dvec2(prev.y, -prev.x)), 0.1);
                    const auto v = PointI(qRound(mid.x) , qRound(mid.y));
                    const auto leftPoint = points31[i + 1] + v;
                    const auto rightPoint = points31[i + 1] - v;
                    endBottomLeft.location31 = glm::ivec2(leftPoint.x, leftPoint.y);
                    endTopLeft.location31 = endBottomLeft.location31;
                    endBottomRight.location31 = glm::ivec2(rightPoint.x, rightPoint.y);
                    endTopRight.location31 = endBottomRight.location31;
                    prev = next;
                }


                uint32_t baseVertexOffset = buildings3D.vertices.size();

                buildings3D.vertices.append(endBottomLeft);
                buildings3D.vertices.append(endTopLeft);
                buildings3D.vertices.append(startTopLeft);
                buildings3D.vertices.append(startBottomLeft);

                cutMeshForTile(
                    fullSideIndices,
                    buildings3D.vertices,
                    buildings3D.indices,
                    baseVertexOffset,
                    minTile31,
                    maxTile31);

                baseVertexOffset = buildings3D.vertices.size();

                buildings3D.vertices.append(startBottomRight);
                buildings3D.vertices.append(startTopRight);
                buildings3D.vertices.append(endTopRight);
                buildings3D.vertices.append(endBottomRight);

                cutMeshForTile(
                    fullSideIndices,
                    buildings3D.vertices,
                    buildings3D.indices,
                    baseVertexOffset,
                    minTile31,
                    maxTile31);

                baseVertexOffset = buildings3D.vertices.size();

                buildings3D.vertices.append(endTopLeft);
                buildings3D.vertices.append(endTopRight);
                buildings3D.vertices.append(startTopRight);
                buildings3D.vertices.append(startTopLeft);

                cutMeshForTile(
                    fullSideIndices,
                    buildings3D.vertices,
                    buildings3D.indices,
                    baseVertexOffset,
                    minTile31,
                    maxTile31);

                startBottomRight = endBottomRight;
                startTopRight = endTopRight;
                startTopLeft = endTopLeft;
                startBottomLeft = endBottomLeft;
            }
        }
    }

    const int buildingVertexCount = buildings3D.vertices.size() - currentVertexOffset;
    const int buildingIndexCount = buildings3D.indices.size() - currentIndexOffset;
    const int outlineVertexCount = show3DbuildingOutline ? (buildings3D.outlineVertices.size() - currentOutlineVertexOffset) : 0;

    buildings3D.buildingIDs.append(sourceObject->id.id);
    buildings3D.vertexCounts.append(buildingVertexCount);
    buildings3D.indexCounts.append(buildingIndexCount);
    buildings3D.outlineVertexCounts.append(outlineVertexCount);
}

inline OsmAnd::BuildingVertex Map3DObjectsTiledProvider_P::getIntersection(
    const BuildingVertex& startVertex, const BuildingVertex& endVertex, double range) const
{
    BuildingVertex vertex = startVertex;
    vertex.location31.x += qRound(static_cast<double>(endVertex.location31.x - startVertex.location31.x) * range);
    vertex.location31.y += qRound(static_cast<double>(endVertex.location31.y - startVertex.location31.y) * range);
    if (startVertex.height != 0.0f && endVertex.height != 0.0f)
    {
        vertex.height = static_cast<float>(static_cast<double>(vertex.height)
            + static_cast<double>(endVertex.height - startVertex.height) * range);
        vertex.terrainHeight = static_cast<float>(static_cast<double>(vertex.terrainHeight)
            + static_cast<double>(endVertex.terrainHeight - startVertex.terrainHeight) * range);
    }
    else
    {
        vertex.height = 0.0f;
        vertex.terrainHeight = startVertex.height == 0.0f ? startVertex.terrainHeight : endVertex.terrainHeight;
    }
    const auto factor = static_cast<float>(range);
    vertex.normal = glm::normalize(startVertex.normal + (endVertex.normal - startVertex.normal) * factor);
    vertex.color += (endVertex.color - startVertex.color) * factor;
    return vertex;
}

inline void Map3DObjectsTiledProvider_P::appendOneTriangle(
    QVector<BuildingVertex>& vertices,
    QVector<uint16_t>& outIndices,
    uint16_t& index,
    const int max,
    const int ad,
    const int bd,
    const int cd,
    const uint16_t ai,
    const uint16_t bi,
    const uint16_t ci) const
{
    if (vertices[ai].height != 0.0f)
    {
        vertices.append(
            getIntersection(vertices[bi], vertices[ai], static_cast<double>(max - bd) / static_cast<double>(ad - bd)));
        vertices.append(
            getIntersection(vertices[ci], vertices[ai], static_cast<double>(max - cd) / static_cast<double>(ad - cd)));
        outIndices.append(ai);
        outIndices.append(index++);
        outIndices.append(index++);
    }
}

inline void Map3DObjectsTiledProvider_P::appendTwoTriangles(
    QVector<BuildingVertex>& vertices,
    QVector<uint16_t>& outIndices,
    uint16_t& index,
    const int max,
    const int ad,
    const int bd,
    const int cd,
    const uint16_t ai,
    const uint16_t bi,
    const uint16_t ci) const
{
    int count = 0;
    if (vertices[ai].height != 0.0f || vertices[bi].height != 0.0f)
    {
        vertices.append(
            getIntersection(vertices[bi], vertices[ai], static_cast<double>(max - bd) / static_cast<double>(ad - bd)));
        count++;
    }
    if (vertices[ai].height != 0.0f || vertices[ci].height != 0.0f)
    {
        vertices.append(
            getIntersection(vertices[ci], vertices[ai], static_cast<double>(max - cd) / static_cast<double>(ad - cd)));
        count++;
    }
    if (count > 0)
    {
        outIndices.append(bi);
        outIndices.append(ci);
        outIndices.append(index);
        if (count > 1)
        {
            outIndices.append(index++);
            outIndices.append(ci);
            outIndices.append(index);
        }
        index++;
    }
}

void Map3DObjectsTiledProvider_P::cutMeshForTile(
    const std::vector<uint16_t>& indices,
    QVector<BuildingVertex>& vertices,
    QVector<uint16_t>& outIndices,
    const uint16_t offset,
    const PointI& minTile31,
    const PointI& maxTile31) const
{
    QVector<uint16_t> tmpIndices;
    auto index = static_cast<uint16_t>(vertices.size());
    auto count = indices.size();
    for (int i = 0; i < count; i += 3)
    {
        const auto ai = indices[i] + offset;
        const auto bi = indices[i + 1] + offset;
        const auto ci = indices[i + 2] + offset;
        const auto& a = vertices[ai].location31;
        const auto& b = vertices[bi].location31;
        const auto& c = vertices[ci].location31;
        if (a.x > maxTile31.x && b.x > maxTile31.x && c.x > maxTile31.x)
            continue;
        if (a.x <= maxTile31.x && b.x <= maxTile31.x && c.x <= maxTile31.x)
        {
            tmpIndices.append(ai);
            tmpIndices.append(bi);
            tmpIndices.append(ci);
        }
        else if (b.x <= maxTile31.x && c.x <= maxTile31.x)
            appendTwoTriangles(vertices, tmpIndices, index, maxTile31.x, a.x, b.x, c.x, ai, bi, ci);
        else if (a.x <= maxTile31.x && c.x <= maxTile31.x)
            appendTwoTriangles(vertices, tmpIndices, index, maxTile31.x, b.x, c.x, a.x, bi, ci, ai);
        else if (a.x <= maxTile31.x && b.x <= maxTile31.x)
            appendTwoTriangles(vertices, tmpIndices, index, maxTile31.x, c.x, a.x, b.x, ci, ai, bi);
        else if (a.x <= maxTile31.x)
            appendOneTriangle(vertices, tmpIndices, index, maxTile31.x, a.x, b.x, c.x, ai, bi, ci);
        else if (b.x <= maxTile31.x)
            appendOneTriangle(vertices, tmpIndices, index, maxTile31.x, b.x, c.x, a.x, bi, ci, ai);
        else
            appendOneTriangle(vertices, tmpIndices, index, maxTile31.x, c.x, a.x, b.x, ci, ai, bi);
    }
    QVector<uint16_t> midIndices;
    count = tmpIndices.size();
    for (int i = 0; i < count; i += 3)
    {
        const auto ai = tmpIndices[i];
        const auto bi = tmpIndices[i + 1];
        const auto ci = tmpIndices[i + 2];
        const auto& a = vertices[ai].location31;
        const auto& b = vertices[bi].location31;
        const auto& c = vertices[ci].location31;
        if (a.x < minTile31.x && b.x < minTile31.x && c.x < minTile31.x)
            continue;
        if (a.x >= minTile31.x && b.x >= minTile31.x && c.x >= minTile31.x)
        {
            midIndices.append(ai);
            midIndices.append(bi);
            midIndices.append(ci);
        }
        else if (b.x >= minTile31.x && c.x >= minTile31.x)
            appendTwoTriangles(vertices, midIndices, index, minTile31.x, a.x, b.x, c.x, ai, bi, ci);
        else if (a.x >= minTile31.x && c.x >= minTile31.x)
            appendTwoTriangles(vertices, midIndices, index, minTile31.x, b.x, c.x, a.x, bi, ci, ai);
        else if (a.x >= minTile31.x && b.x >= minTile31.x)
            appendTwoTriangles(vertices, midIndices, index, minTile31.x, c.x, a.x, b.x, ci, ai, bi);
        else if (a.x >= minTile31.x)
            appendOneTriangle(vertices, midIndices, index, minTile31.x, a.x, b.x, c.x, ai, bi, ci);
        else if (b.x >= minTile31.x)
            appendOneTriangle(vertices, midIndices, index, minTile31.x, b.x, c.x, a.x, bi, ci, ai);
        else
            appendOneTriangle(vertices, midIndices, index, minTile31.x, c.x, a.x, b.x, ci, ai, bi);
    }
    tmpIndices.clear();
    count = midIndices.size();
    for (int i = 0; i < count; i += 3)
    {
        const auto ai = midIndices[i];
        const auto bi = midIndices[i + 1];
        const auto ci = midIndices[i + 2];
        const auto& a = vertices[ai].location31;
        const auto& b = vertices[bi].location31;
        const auto& c = vertices[ci].location31;
        if (a.y > maxTile31.y && b.y > maxTile31.y && c.y > maxTile31.y)
            continue;
        if (a.y <= maxTile31.y && b.y <= maxTile31.y && c.y <= maxTile31.y)
        {
            tmpIndices.append(ai);
            tmpIndices.append(bi);
            tmpIndices.append(ci);
        }
        else if (b.y <= maxTile31.y && c.y <= maxTile31.y)
            appendTwoTriangles(vertices, tmpIndices, index, maxTile31.y, a.y, b.y, c.y, ai, bi, ci);
        else if (a.y <= maxTile31.y && c.y <= maxTile31.y)
            appendTwoTriangles(vertices, tmpIndices, index, maxTile31.y, b.y, c.y, a.y, bi, ci, ai);
        else if (a.y <= maxTile31.y && b.y <= maxTile31.y)
            appendTwoTriangles(vertices, tmpIndices, index, maxTile31.y, c.y, a.y, b.y, ci, ai, bi);
        else if (a.y <= maxTile31.y)
            appendOneTriangle(vertices, tmpIndices, index, maxTile31.y, a.y, b.y, c.y, ai, bi, ci);
        else if (b.y <= maxTile31.y)
            appendOneTriangle(vertices, tmpIndices, index, maxTile31.y, b.y, c.y, a.y, bi, ci, ai);
        else
            appendOneTriangle(vertices, tmpIndices, index, maxTile31.y, c.y, a.y, b.y, ci, ai, bi);
    }
    count = tmpIndices.size();
    for (int i = 0; i < count; i += 3)
    {
        const auto ai = tmpIndices[i];
        const auto bi = tmpIndices[i + 1];
        const auto ci = tmpIndices[i + 2];
        const auto& a = vertices[ai].location31;
        const auto& b = vertices[bi].location31;
        const auto& c = vertices[ci].location31;
        if (a.y < minTile31.y && b.y < minTile31.y && c.y < minTile31.y)
            continue;
        if (a.y >= minTile31.y && b.y >= minTile31.y && c.y >= minTile31.y)
        {
            outIndices.append(ai);
            outIndices.append(bi);
            outIndices.append(ci);
        }
        else if (b.y >= minTile31.y && c.y >= minTile31.y)
            appendTwoTriangles(vertices, outIndices, index, minTile31.y, a.y, b.y, c.y, ai, bi, ci);
        else if (a.y >= minTile31.y && c.y >= minTile31.y)
            appendTwoTriangles(vertices, outIndices, index, minTile31.y, b.y, c.y, a.y, bi, ci, ai);
        else if (a.y >= minTile31.y && b.y >= minTile31.y)
            appendTwoTriangles(vertices, outIndices, index, minTile31.y, c.y, a.y, b.y, ci, ai, bi);
        else if (a.y >= minTile31.y)
            appendOneTriangle(vertices, outIndices, index, minTile31.y, a.y, b.y, c.y, ai, bi, ci);
        else if (b.y >= minTile31.y)
            appendOneTriangle(vertices, outIndices, index, minTile31.y, b.y, c.y, a.y, bi, ci, ai);
        else
            appendOneTriangle(vertices, outIndices, index, minTile31.y, c.y, a.y, b.y, ci, ai, bi);
    }
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
    const glm::vec3& edgeNormal,
    float minHeight,
    float height,
    float baseTerrainHeight,
    float terrainHeight,
    const PointI& minTile31,
    const PointI& maxTile31,
    const glm::vec3& colorVec,
    bool generateOutline,
    QList<PassagePrimitive*>& passagePrimitives) const
{
    const uint32_t baseVertexOffset = buildings3D.vertices.size();

    // Should match the side order (counter-clockwise)
    buildings3D.vertices.append({glm::ivec2(point31_next.x, point31_next.y), minHeight, baseTerrainHeight, edgeNormal, colorVec});
    buildings3D.vertices.append({glm::ivec2(point31_next.x, point31_next.y), height, terrainHeight, edgeNormal, colorVec});
    buildings3D.vertices.append({glm::ivec2(point31_i.x, point31_i.y), height, terrainHeight, edgeNormal, colorVec});
    buildings3D.vertices.append({glm::ivec2(point31_i.x, point31_i.y), minHeight, baseTerrainHeight, edgeNormal, colorVec});

    if (!passagePrimitives.isEmpty())
    {
        std::vector<std::vector<std::array<int32_t, 2>>> polygons;
        std::vector<std::array<int32_t, 2>> polygon;

        polygon.push_back({200, 0});
        polygon.push_back({200, 100});
        polygon.push_back({0, 100});
        polygon.push_back({0, 0});

        int x = 10;
        for (auto passagePrimitive : passagePrimitives)
        {
            if (passagePrimitive->putStart)
            {
                passagePrimitive->putStart = false;

                // Should match the side order (counter-clockwise)
                buildings3D.vertices.append(passagePrimitive->startBottomLeft);
                buildings3D.vertices.append(passagePrimitive->startTopLeft);
                buildings3D.vertices.append(passagePrimitive->startTopRight);
                buildings3D.vertices.append(passagePrimitive->startBottomRight);
            }
            else if (passagePrimitive->putEnd)
            {
                passagePrimitive->putEnd = false;

                // Should match the side order (counter-clockwise)
                buildings3D.vertices.append(passagePrimitive->endBottomLeft);
                buildings3D.vertices.append(passagePrimitive->endTopLeft);
                buildings3D.vertices.append(passagePrimitive->endTopRight);
                buildings3D.vertices.append(passagePrimitive->endBottomRight);
            }

            polygon.push_back({x, 0});
            polygon.push_back({x, 90});
            x += 10;
            polygon.push_back({x, 90});
            polygon.push_back({x, 0});
            x += 10;
        }

        polygons.push_back(std::move(polygon));

        cutMeshForTile(
            mapbox::earcut<uint16_t>(polygons),
            buildings3D.vertices,
            buildings3D.indices,
            baseVertexOffset,
            minTile31,
            maxTile31);
    }
    else
    {
        cutMeshForTile(
            fullSideIndices,
            buildings3D.vertices,
            buildings3D.indices,
            baseVertexOffset,
            minTile31,
            maxTile31);
    }

    if (generateOutline)
    {
        const uint16_t outlineBase = static_cast<uint16_t>(buildings3D.outlineVertices.size());

        buildings3D.outlineVertices.append({glm::ivec2(point31_i.x, point31_i.y), minHeight, baseTerrainHeight, edgeNormal, colorVec});
        buildings3D.outlineVertices.append({glm::ivec2(point31_i.x,      point31_i.y),      height,    terrainHeight,    edgeNormal, colorVec});
        buildings3D.outlineVertices.append({glm::ivec2(point31_next.x,   point31_next.y),   height,    terrainHeight,    edgeNormal, colorVec});
        buildings3D.outlineVertices.append({glm::ivec2(point31_next.x,   point31_next.y),   minHeight, baseTerrainHeight, edgeNormal, colorVec});

        buildings3D.outlineIndices.append(outlineBase + 0);
        buildings3D.outlineIndices.append(outlineBase + 1);
        buildings3D.outlineIndices.append(outlineBase + 2);
        buildings3D.outlineIndices.append(outlineBase + 3);
        buildings3D.outlineIndices.append(outlineBase + 0);
        buildings3D.outlineIndices.append(outlineBase + 3);
        buildings3D.outlineIndices.append(outlineBase + 1);
        buildings3D.outlineIndices.append(outlineBase + 2);
    }
}

uint32_t Map3DObjectsTiledProvider_P::getPolygonColor(const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive) const
{
    uint32_t colorARGB = 0;
    primitive->evaluationResult.getIntegerValue(_environment->styleBuiltinValueDefs->id_OUTPUT_COLOR, colorARGB);
    return colorARGB;
}
