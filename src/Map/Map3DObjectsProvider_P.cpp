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

// ~ Maximum allowed inaccuracy of passage coordinates
#define PASSAGE_MAX_WALL_INACCURACY 50.0

#define PASSAGE_DEFAULT_WIDTH 2.0
#define PASSAGE_DEFAULT_HEIGHT 2.5

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
        {1.0f, -1.0f},
        {0.0f, -1.0f},
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
        return false;

    MapPrimitivesProvider::Request tileRequest;
    tileRequest.tileId = request.tileId;
    tileRequest.zoom = request.zoom;
    tileRequest.visibleArea31 = request.visibleArea31;
    tileRequest.areaTime = request.areaTime;
    tileRequest.queryController = request.queryController;

    std::shared_ptr<MapPrimitivesProvider::Data> tileData;
    if (!_tiledProvider->obtainTiledPrimitives(tileRequest, tileData))
        return false;

    QVector<BuildingVertex> vertices;
    QVector<uint16_t> indices;
    QVector<int32_t> parts;
    if (tileData && tileData->primitivisedObjects)
    {
        QSet<BuildingPrimitive> buildings;
        QSet<BuildingPrimitive> buildingParts;
        QSet<PassagePrimitive> buildingPassages;
        QList<const BuildingPrimitive*> primaryBuildings;

        for (const auto& primitive : tileData->primitivisedObjects->polygons)
        {
            collectFromPolygons(primitive, buildings, buildingParts);
        }

        for (const auto& primitive : tileData->primitivisedObjects->polylines)
        {
            collectFromPolyline(primitive, buildings, buildingParts, buildingPassages);
        }

        filterBuildings(buildings, buildingParts);

        QHash<TileId, std::shared_ptr<IMapElevationDataProvider::Data>> elevationDataMap;
        if (_elevationProvider)
        {
            AreaI commonBbox31;
            commonBbox31.top() = INT32_MAX;
            commonBbox31.left() = INT32_MAX;
            for (const auto& buildingPart : buildingParts)
            {
                if (const auto& sourceObject =
                    std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(buildingPart.primitive->sourceObject))
                {
                    commonBbox31.enlargeToInclude(sourceObject->bbox31);
                }
            }
            for (const auto& building : buildings)
            {
                if (const auto& sourceObject =
                    std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(building.primitive->sourceObject))
                {
                    commonBbox31.enlargeToInclude(sourceObject->bbox31);
                }
            }
            int zoomShift = MaxZoomLevel - request.zoom;
            const auto top = commonBbox31.top() >> zoomShift;
            const auto left = commonBbox31.left() >> zoomShift;
            const auto bottom = (commonBbox31.bottom() >> zoomShift) + 1;
            const auto right = (commonBbox31.right() >> zoomShift) + 1;
            for (int col = top; col < bottom; col++)
            {
                for (int row = left; row < right; row++)
                {
                    std::shared_ptr<IMapElevationDataProvider::Data> elevationData;
                    IMapElevationDataProvider::Request elevationRequest;
                    elevationRequest.tileId = TileId::fromXY(row, col);
                    elevationRequest.zoom = request.zoom;
                    elevationRequest.queryController = request.queryController;
                    _elevationProvider->obtainElevationData(elevationRequest, elevationData, nullptr);
                    elevationDataMap.insert(elevationRequest.tileId, elevationData);
                }
            }
        }

        QHash<TileId, FColorRGB> colors;
        {
            QReadLocker scopedLocker(&_objectColorsLock);

            colors = _objectColors;
            colors.detach();
        }

        for (const auto& buildingPart : buildingParts)
        {
            processPrimitive(buildingPart, vertices, indices, elevationDataMap, request.tileId, request.zoom,
                buildingPassages, colors, &primaryBuildings);
        }

        for (const auto& building : buildings)
        {
            processPrimitive(building, vertices, indices, elevationDataMap, request.tileId, request.zoom,
                buildingPassages, colors, &primaryBuildings);
        }

        if (!primaryBuildings.isEmpty())
            parts.append(indices.size());

        for (const auto& primaryBuilding : primaryBuildings)
        {
            processPrimitive(*primaryBuilding, vertices, indices, elevationDataMap, request.tileId, request.zoom,
                buildingPassages, colors);
        }
    }

    outTiledData =
        std::make_shared<Map3DObjectsTiledProvider::Data>(request.tileId, request.zoom, vertices, indices, parts);
    return true;
}

void Map3DObjectsTiledProvider_P::filterBuildings(QSet<BuildingPrimitive>& buildings,
                                                  QSet<BuildingPrimitive>& buildingParts) const
{
    const bool show3DbuildingParts = _environment ? _environment->getShow3DBuildingParts() : false;
    if (!show3DbuildingParts)
        return;

    for (auto it = buildings.begin(); it != buildings.end();)
    {
        const auto& buildingPrimitive = *it;
        const auto& building =
            std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(buildingPrimitive.primitive->sourceObject);
        if (!building)
        {
            it = buildings.erase(it);
            continue;
        }

        if (building->containsTag(QStringLiteral("role_outline"), true) ||
            building->containsTag(QStringLiteral("role_inner"), true) ||
            building->containsTag(QStringLiteral("role_outer"), true))
        {
            it = buildings.erase(it);
            continue;
        }

        if (building->containsAttribute(QStringLiteral("layer"), QStringLiteral("-1"), true))
        {
            it = buildings.erase(it);
            continue;
        }

        bool shouldRemove = false;
        QVector<QPair<BuildingPrimitive, BuildingPrimitive>> itemsToModify;
        for (auto& buildingPart : buildingParts)
        {
            const auto& part =
                std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(buildingPart.primitive->sourceObject);
            if (part && building->bbox31.contains(part->bbox31))
            {
                buildingPart.bbox31 = building->bbox31;
                if (buildingPart.polygonColor == 0)
                {
                    BuildingPrimitive modifiedBuildingPart = buildingPart;
                    modifiedBuildingPart.polygonColor = buildingPrimitive.polygonColor;
                    itemsToModify.append(qMakePair(buildingPart, modifiedBuildingPart));
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
            it = buildings.erase(it);
        else
        {
            buildingPrimitive.bbox31 = building->bbox31;
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
        return;

    if (polylinePrimitive->type == MapPrimitiviser::PrimitiveType::Polygon)
    {
        BuildingPrimitive buildingPrimitive;
        buildingPrimitive.primitive = polylinePrimitive;
        buildingPrimitive.polygonColor = getPolygonColor(buildingPrimitive.primitive);

        const bool show3DbuildingParts = _environment ? _environment->getShow3DBuildingParts() : false;

        if (sourceObject->containsTag(QStringLiteral("building")))
        {
            insertOrUpdateBuilding(buildingPrimitive, outBuildings);
        }
        else if (show3DbuildingParts && sourceObject->containsTag(QStringLiteral("building:part")))
        {
            insertOrUpdateBuilding(buildingPrimitive, outBuildingParts);
        }
    }
    else if (polylinePrimitive->type == MapPrimitiviser::PrimitiveType::Polyline)
    {
        if (sourceObject->points31.size() > 1
            && (sourceObject->containsAttribute(QStringLiteral("tunnel"), QStringLiteral("building_passage"))
            || sourceObject->containsAttribute(QStringLiteral("tunnel"), QStringLiteral("building_passage"), true)))
        {
            PassagePrimitive passagePrimitive;
            passagePrimitive.primitive = polylinePrimitive;
            passagePrimitive.startingPoint = sourceObject->points31.first();
            passagePrimitive.endingPoint = sourceObject->points31.last();
            passagePrimitive.centerPoint = passagePrimitive.startingPoint / 2 + passagePrimitive.endingPoint / 2;
            const auto startingSegment = sourceObject->points31[1] - passagePrimitive.startingPoint;
            const auto endingSegment =
                passagePrimitive.endingPoint - sourceObject->points31[sourceObject->points31.size() - 2];
            passagePrimitive.endingSegment = glm::normalize(glm::dvec2(endingSegment.x, endingSegment.y));
            passagePrimitive.startingSegment = glm::normalize(glm::dvec2(startingSegment.x, startingSegment.y));
            const QString width = QStringLiteral("width");
            const QString height = QStringLiteral("height");
            passagePrimitive.height = Utilities::parseLength(sourceObject->getResolvedAttribute(QStringRef(&height)),
                PASSAGE_DEFAULT_HEIGHT);
            const auto widthValue = Utilities::parseLength(sourceObject->getResolvedAttribute(QStringRef(&width)),
                PASSAGE_DEFAULT_WIDTH);
            passagePrimitive.halfWidth31 =
                qRound(widthValue / (2.0f * Utilities::getMetersPer31Coordinate(passagePrimitive.centerPoint)));
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

    if (sourceObject->containsTag(QStringLiteral("building")))
    {
        insertOrUpdateBuilding(buildingPrimitive, outBuildings);
    }
    else if (show3DbuildingParts && sourceObject->containsTag(QStringLiteral("building:part")))
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

void Map3DObjectsTiledProvider_P::processPrimitive(
    const BuildingPrimitive& primitive,
    QVector<BuildingVertex>& vertices,
    QVector<uint16_t>& indices,
    const QHash<TileId, std::shared_ptr<IMapElevationDataProvider::Data>>& elevationDataMap,
    const TileId& tileId,
    const ZoomLevel zoom,
    const QSet<PassagePrimitive>& buildingPassages,
    const QHash<TileId, FColorRGB>& objectColors,
    QList<const BuildingPrimitive*>* primaryBuildings /* = nullptr */) const
{
    const auto& sourceObject =
        std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(primitive.primitive->sourceObject);
    if (!sourceObject)
        return;

    auto color = _useCustomColor ? _customColor : FColorRGB(getUseDefaultBuildingsColor()
        || primitive.polygonColor == 0 ? getDefaultBuildingsColor() : FColorARGB(primitive.polygonColor));

    // Set an individual color to a particular (selected) building
    if (primaryBuildings)
    {
        // Keep selected buildings separately to draw them first
        auto it = objectColors.begin();
        auto end = objectColors.end();
        for (; it != end; it++)
        {
            auto tileId = it.key();
            if (primitive.bbox31.contains(PointI(tileId.x, tileId.y)))
            {
                primitive.color = it.value();
                primaryBuildings->append(&primitive);
                return;
            }
        }
    }
    else
        color = primitive.color;

    glm::vec4 colorVec(color.r, color.g, color.b, 0.0f);

    float levelHeight = getDefaultBuildingsLevelHeight();

    float height = getDefaultBuildingsHeight();
    float minHeight = 0.0f;
    float terrainHeight = 0.0f;
    float baseTerrainHeight = 0.0f;

    float levels = 0.0;
    float minLevels = 0.0;

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

    if (!elevationDataMap.isEmpty())
    {
        float maxElevationMeters = 0.0f;
        float minElevationMeters = 0.0f;
        bool hasElevationSample = false;

        for (const auto& point : constOf(sourceObject->points31))
        {
            accumulateElevationForPoint(
                point, zoom, elevationDataMap, maxElevationMeters, minElevationMeters, hasElevationSample);
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

    // Calculate pseudo-random seed value to produce slightly different colors for equally-lit walls of building
    const auto centerPoint = AreaI64(sourceObject->bbox31).center();
    int b = (centerPoint.x ^ centerPoint.y) & 255;
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    colorVec.a = static_cast<float>(b) / 255.0f;

    QVector<PointI> points31 = sourceObject->points31;

    // Reverse points if needed
    double area = Utilities::computeSignedArea(points31);
    if (area >= 0.0)
        std::reverse(points31.begin(), points31.end());

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
            if (innerArea < 0.0)
                std::reverse(innerPoints.begin(), innerPoints.end());

            innerPolygons.append(innerPoints);
        }
    }

    int currentVertexOffset = vertices.size();
    const int currentIndexOffset = indices.size();

    // Construct top side of the mesh
    std::vector<std::vector<std::array<int32_t, 2>>> topPolygon;

    std::vector<std::array<int32_t, 2>> outerRing;
    for (int i = 0; i < edgePointsCount; ++i)
    {
        const int next = (i + 1) % edgePointsCount;
        const auto& p = points31[i];
        BuildingVertex v{glm::ivec2(p.x, p.y), height, terrainHeight, glm::vec3(0.0f, 1.0f, 0.0f), colorVec};

        outerRing.push_back({p.x, p.y});
        vertices.append(v);
    }

    topPolygon.push_back(std::move(outerRing));

    for (const auto& innerPoly : innerPolygons)
    {
        std::vector<std::array<int32_t, 2>> innerRing;

        for (const auto& p : innerPoly)
        {
            BuildingVertex v{glm::ivec2(p.x, p.y), height, terrainHeight, glm::vec3(0.0f, 1.0f, 0.0f), colorVec};

            innerRing.push_back({p.x, p.y});
            vertices.append(v);
        }

        topPolygon.push_back(std::move(innerRing));
    }

    const auto zoomLevelDelta = MaxZoomLevel - zoom;
    const PointI minTile31(tileId.x << zoomLevelDelta, tileId.y << zoomLevelDelta);
    const PointI maxTile31((tileId.x + 1) << zoomLevelDelta, (tileId.y + 1) << zoomLevelDelta);

    cutMeshForTile(
        mapbox::earcut<uint16_t>(topPolygon),
        vertices,
        indices,
        currentVertexOffset,
        minTile31,
        maxTile31);

    // Construct walls of the mesh
    for (int polyIdx = -1; polyIdx < innerPolygons.size(); polyIdx++)
    {
        const auto& points = polyIdx < 0 ? points31 : innerPolygons[polyIdx];

        int count = polyIdx < 0 ? edgePointsCount : points.size();
        for (int i = 0; i < count; ++i)
        {
            const int next = (i + 1) % count;
            const auto& point31_i = points[i];
            const auto& point31_next = points[next];
            const auto centerPoint = (PointI64(point31_i) + PointI64(point31_next)) / 2;
            const auto edgeNormal = calculateNormalFrom2Points(point31_i, point31_next);

            QList<PassagePrimitive*> passagePrimitives;
            if (!passages.isEmpty())
            {
                const auto side = glm::dvec2(point31_next.x - point31_i.x, point31_next.y - point31_i.y);
                const auto sideLengthSqr = side.x * side.x + side.y * side.y;
                const auto sideLength = qSqrt(sideLengthSqr);
                if (qFuzzyIsNull(sideLengthSqr))
                    continue;
                const auto sideN = side / sideLength;

                for (auto passage : passages)
                {
                    const auto halfN = static_cast<double>(passage->halfWidth31) / sideLength;
                    const auto minGapN = halfN / 2.0;
                    if (!passage->putStart && !passage->withStart)
                    {
                        const auto distN = glm::dot(glm::dvec2(
                            passage->startingPoint.x - point31_i.x,
                            passage->startingPoint.y - point31_i.y), side) / sideLengthSqr;
                        if (distN > 0.0 && distN < 1.0 && distN > minGapN && 1.0 - distN > minGapN)
                        {
                            const auto inaccuracy = glm::dot(glm::dvec2(sideN.y, -sideN.x), glm::dvec2(
                                passage->startingSegment.x, passage->startingSegment.y)) * PASSAGE_MAX_WALL_INACCURACY;
                            auto proj = side * distN;
                            auto proj31 = PointI(qRound(proj.x), qRound(proj.y));
                            const auto point31 = point31_i + proj31;
                            const auto dist = (point31 - passage->startingPoint).norm();
                            if (dist < inaccuracy)
                            {
                                const auto backDistance = distN > halfN ? halfN : minGapN;
                                proj = side * backDistance;
                                proj31 = PointI(qRound(proj.x), qRound(proj.y));
                                const auto pointLeft = point31 - proj31;
                                passage->startBottomLeft = BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                                    minHeight, baseTerrainHeight, edgeNormal, colorVec};
                                passage->startTopLeft = BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                                    passage->height, terrainHeight, edgeNormal, colorVec};
                                const auto forthDistance = 1.0 - distN > halfN ? halfN : minGapN;
                                proj = side * forthDistance;
                                proj31 = PointI(qRound(proj.x), qRound(proj.y));
                                const auto pointRight = point31 + proj31;
                                passage->startBottomRight = BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                                    minHeight, baseTerrainHeight, edgeNormal, colorVec};
                                passage->startTopRight = BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                                    passage->height, terrainHeight, edgeNormal, colorVec};
                                passage->startingPoint.x = qRound((distN - backDistance) * sideLength);
                                passage->startingPoint.y = qRound((distN + forthDistance) * sideLength);
                                passage->putStart = true;
                                passage->withStart = true;
                                passagePrimitives.append(passage);
                                continue;
                            }
                        }
                    }
                    if (!passage->putEnd && !passage->withEnd)
                    {
                        const auto distN = glm::dot(glm::dvec2(
                            passage->endingPoint.x - point31_i.x,
                            passage->endingPoint.y - point31_i.y), side) / sideLengthSqr;
                        if (distN > 0.0 && distN < 1.0 && distN > minGapN && 1.0 - distN > minGapN)
                        {
                            const auto inaccuracy = glm::dot(glm::dvec2(-sideN.y, sideN.x), glm::dvec2(
                                passage->endingSegment.x, passage->endingSegment.y)) * PASSAGE_MAX_WALL_INACCURACY;
                            auto proj = side * distN;
                            auto proj31 = PointI(qRound(proj.x), qRound(proj.y));
                            const auto point31 = point31_i + proj31;
                            const auto dist = (point31 - passage->endingPoint).norm();
                            if (dist < inaccuracy)
                            {
                                const auto backDistance = distN > halfN ? halfN : minGapN;
                                proj = side * backDistance;
                                proj31 = PointI(qRound(proj.x), qRound(proj.y));
                                const auto pointLeft = point31 - proj31;
                                passage->endBottomLeft = BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                                    minHeight, baseTerrainHeight, edgeNormal, colorVec};
                                passage->endTopLeft = BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                                    passage->height, terrainHeight, edgeNormal, colorVec};
                                const auto forthDistance = 1.0 - distN > halfN ? halfN : minGapN;
                                proj = side * forthDistance;
                                proj31 = PointI(qRound(proj.x), qRound(proj.y));
                                const auto pointRight = point31 + proj31;
                                passage->endBottomRight = BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                                    minHeight, baseTerrainHeight, edgeNormal, colorVec};
                                passage->endTopRight = BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                                    passage->height, terrainHeight, edgeNormal, colorVec};
                                passage->endingPoint.x = qRound((distN - backDistance) * sideLength);
                                passage->endingPoint.y = qRound((distN + forthDistance) * sideLength);
                                passage->putEnd = true;
                                passage->withEnd = true;
                                passagePrimitives.append(passage);
                            }
                        }
                    }
                }
            }

            generateBuildingWall(
                vertices,
                indices,
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
            uint32_t baseVertexOffset = vertices.size();

            auto n = calculateNormalFrom2Points(
                PointI(passage->startTopLeft.location31.x, passage->startTopLeft.location31.y),
                PointI(passage->endTopRight.location31.x, passage->endTopRight.location31.y));
            vertices.append(passage->endBottomRight);
            vertices.last().normal = n;
            vertices.append(passage->endTopRight);
            vertices.last().normal = n;
            vertices.append(passage->startTopLeft);
            vertices.last().normal = n;
            vertices.append(passage->startBottomLeft);
            vertices.last().normal = n;

            cutMeshForTile(
                fullSideIndices,
                vertices,
                indices,
                baseVertexOffset,
                minTile31,
                maxTile31);

            baseVertexOffset = vertices.size();

            n = calculateNormalFrom2Points(
                PointI(passage->endTopLeft.location31.x, passage->endTopLeft.location31.y),
                PointI(passage->startTopRight.location31.x, passage->startTopRight.location31.y));
            vertices.append(passage->startBottomRight);
            vertices.last().normal = n;
            vertices.append(passage->startTopRight);
            vertices.last().normal = n;
            vertices.append(passage->endTopLeft);
            vertices.last().normal = n;
            vertices.append(passage->endBottomLeft);
            vertices.last().normal = n;

            cutMeshForTile(
                fullSideIndices,
                vertices,
                indices,
                baseVertexOffset,
                minTile31,
                maxTile31);

            baseVertexOffset = vertices.size();

            n = glm::vec3(0.0f, -1.0f, 0.0f);
            vertices.append(passage->endTopRight);
            vertices.last().normal = n;
            vertices.append(passage->endTopLeft);
            vertices.last().normal = n;
            vertices.append(passage->startTopRight);
            vertices.last().normal = n;
            vertices.append(passage->startTopLeft);
            vertices.last().normal = n;

            cutMeshForTile(
                fullSideIndices,
                vertices,
                indices,
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


                uint32_t baseVertexOffset = vertices.size();

                auto n = calculateNormalFrom2Points(
                    PointI(startTopLeft.location31.x, startTopLeft.location31.y),
                    PointI(endTopLeft.location31.x, endTopLeft.location31.y));
                vertices.append(endBottomLeft);
                vertices.last().normal = n;
                vertices.append(endTopLeft);
                vertices.last().normal = n;
                vertices.append(startTopLeft);
                vertices.last().normal = n;
                vertices.append(startBottomLeft);
                vertices.last().normal = n;

                cutMeshForTile(
                    fullSideIndices,
                    vertices,
                    indices,
                    baseVertexOffset,
                    minTile31,
                    maxTile31);

                baseVertexOffset = vertices.size();

                n = calculateNormalFrom2Points(
                    PointI(endTopRight.location31.x, endTopRight.location31.y),
                    PointI(startTopRight.location31.x, startTopRight.location31.y));
                vertices.append(startBottomRight);
                vertices.last().normal = n;
                vertices.append(startTopRight);
                vertices.last().normal = n;
                vertices.append(endTopRight);
                vertices.last().normal = n;
                vertices.append(endBottomRight);
                vertices.last().normal = n;

                cutMeshForTile(
                    fullSideIndices,
                    vertices,
                    indices,
                    baseVertexOffset,
                    minTile31,
                    maxTile31);

                baseVertexOffset = vertices.size();

                n = glm::vec3(0.0f, -1.0f, 0.0f);
                vertices.append(endTopLeft);
                vertices.last().normal = n;
                vertices.append(endTopRight);
                vertices.last().normal = n;
                vertices.append(startTopRight);
                vertices.last().normal = n;
                vertices.append(startTopLeft);
                vertices.last().normal = n;

                cutMeshForTile(
                    fullSideIndices,
                    vertices,
                    indices,
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
    ZoomLevel zoom,
    const QHash<TileId, std::shared_ptr<IMapElevationDataProvider::Data>>& elevationDataMap,
    float& maxElevationMeters,
    float& minElevationMeters,
    bool& hasElevationSample) const
{
    const auto zoomLevelDelta = MaxZoomLevel - zoom;
    const auto tileId = TileId::fromXY(point31.x >> zoomLevelDelta, point31.y >> zoomLevelDelta);
    const auto& elevationData = elevationDataMap.value(tileId);
    if (!elevationData)
        return;
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

void Map3DObjectsTiledProvider_P::setElevationDataProvider(
    const std::shared_ptr<IMapElevationDataProvider>& elevationProvider)
{
    _elevationProvider = elevationProvider;
}

void Map3DObjectsTiledProvider_P::addObjectColor(const TileId& location31, const FColorRGB& color)
{
    QWriteLocker scopedLocker(&_objectColorsLock);

    _objectColors.insert(location31, color);
}

void Map3DObjectsTiledProvider_P::removeObjectColor(const TileId& location31)
{
    QWriteLocker scopedLocker(&_objectColorsLock);

    _objectColors.remove(location31);
}

void Map3DObjectsTiledProvider_P::removeAllObjectColors()
{
    QWriteLocker scopedLocker(&_objectColorsLock);

    _objectColors.clear();
}

glm::vec3 Map3DObjectsTiledProvider_P::calculateNormalFrom2Points(PointI point31_i, PointI point31_next) const
{
    const float dx = static_cast<float>(point31_next.x - point31_i.x);
    const float dz = static_cast<float>(point31_next.y - point31_i.y);

    glm::vec3 n(-dz, 0.0f, dx);
    const float len = glm::length(n);
    if (len > 0.0f)
        n /= len;
    else
        n = glm::vec3(0.0f, 0.0f, 1.0f);

    return n;
}

void Map3DObjectsTiledProvider_P::generateBuildingWall(
    QVector<BuildingVertex>& vertices,
    QVector<uint16_t>& indices,
    const PointI& point31_i,
    const PointI& point31_next,
    const glm::vec3& edgeNormal,
    float minHeight,
    float height,
    float baseTerrainHeight,
    float terrainHeight,
    const PointI& minTile31,
    const PointI& maxTile31,
    const glm::vec4& colorVec,
    QList<PassagePrimitive*>& passagePrimitives) const
{
    const uint32_t baseVertexOffset = vertices.size();

    // Should match the side order (counter-clockwise)
    vertices.append({glm::ivec2(point31_next.x, point31_next.y), minHeight, baseTerrainHeight, edgeNormal, colorVec});
    vertices.append({glm::ivec2(point31_next.x, point31_next.y), height, terrainHeight, edgeNormal, colorVec});
    vertices.append({glm::ivec2(point31_i.x, point31_i.y), height, terrainHeight, edgeNormal, colorVec});
    vertices.append({glm::ivec2(point31_i.x, point31_i.y), minHeight, baseTerrainHeight, edgeNormal, colorVec});

    if (!passagePrimitives.isEmpty())
    {
        std::vector<std::vector<std::array<int32_t, 2>>> polygons;
        std::vector<std::array<int32_t, 2>> polygon;

        polygon.push_back({200, 0});
        polygon.push_back({200, -100});
        polygon.push_back({0, -100});
        polygon.push_back({0, 0});

        QMap<int, PassagePrimitive*> passages;
        for (auto passagePrimitive : passagePrimitives)
        {
            if (passagePrimitive->putStart)
            {
                const auto center = (passagePrimitive->startingPoint.x + passagePrimitive->startingPoint.y) / 2;
                passages.insert(center, passagePrimitive);
            }
            else if (passagePrimitive->putEnd)
            {
                const auto center = (passagePrimitive->endingPoint.x + passagePrimitive->endingPoint.y) / 2;
                passages.insert(center, passagePrimitive);
            }
        }
        int x = 10;
        int prevDistance = 0;
        for (auto passage : passages)
        {
            bool withGate = false;
            if (passage->putStart)
            {
                passage->putStart = false;

                if (passage->startingPoint.x > prevDistance)
                {
                    prevDistance = passage->startingPoint.y;

                    // Should match the side order (counter-clockwise)
                    vertices.append(passage->startBottomLeft);
                    vertices.append(passage->startTopLeft);
                    vertices.append(passage->startTopRight);
                    vertices.append(passage->startBottomRight);
                    withGate = true;
                }
            }
            else if (passage->putEnd)
            {
                passage->putEnd = false;

                if (passage->endingPoint.x > prevDistance)
                {
                    prevDistance = passage->endingPoint.y;

                    // Should match the side order (counter-clockwise)
                    vertices.append(passage->endBottomLeft);
                    vertices.append(passage->endTopLeft);
                    vertices.append(passage->endTopRight);
                    vertices.append(passage->endBottomRight);
                    withGate = true;
                }
            }

            if (withGate)
            {
                polygon.push_back({x, 0});
                polygon.push_back({x, -90});
                x += 10;
                polygon.push_back({x, -90});
                polygon.push_back({x, 0});
                x += 10;
            }
        }

        polygons.push_back(std::move(polygon));

        cutMeshForTile(
            mapbox::earcut<uint16_t>(polygons),
            vertices,
            indices,
            baseVertexOffset,
            minTile31,
            maxTile31);
    }
    else
    {
        cutMeshForTile(
            fullSideIndices,
            vertices,
            indices,
            baseVertexOffset,
            minTile31,
            maxTile31);
    }
}

uint32_t Map3DObjectsTiledProvider_P::getPolygonColor(const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive) const
{
    uint32_t colorARGB = 0;
    primitive->evaluationResult.getIntegerValue(_environment->styleBuiltinValueDefs->id_OUTPUT_COLOR, colorARGB);
    return colorARGB;
}
