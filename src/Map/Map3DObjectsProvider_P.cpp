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

// Coordinate precision when filter out buildings by parts inside
#define MAX_COORDINATE_DELTA 30 // ~60cm

// Minimum allowed distance between corners
#define MIN_ALLOWED_SQR_DISTANCE 100.0 // ~20cm

// Maximum allowed inaccuracy of passage coordinates
#define PASSAGE_MAX_WALL_GAP 50.0 // ~1m

// Minimum cosine of the angle between adjacent walls that must be drawn seamlessly
#define MIN_COSINE_CURVED_WALL 0.76 // ~40deg

#define PASSAGE_DEFAULT_WIDTH 6.0f
#define PASSAGE_DEFAULT_HEIGHT 6.0f

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

    const bool show3DbuildingParts = _environment ? _environment->getShow3DBuildingParts() : false;

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
            collectFromPolygons(primitive, show3DbuildingParts, buildings, buildingParts);
        }

        for (const auto& primitive : tileData->primitivisedObjects->polylines)
        {
            collectFromPolyline(primitive, show3DbuildingParts, buildings, buildingParts, buildingPassages);
        }

        if (show3DbuildingParts && !buildings.isEmpty())
            filterBuildings(buildings, buildingParts);

        QHash<TileId, std::shared_ptr<IMapElevationDataProvider::Data>> elevationDataMap;
        if (_elevationProvider)
        {
            AreaI commonBbox31;
            commonBbox31.top() = INT32_MAX;
            commonBbox31.left() = INT32_MAX;
            for (const auto& buildingPart : buildingParts)
            {
                if (buildingPart.hidden)
                    continue;
                if (const auto& sourceObject =
                    std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(buildingPart.primitive->sourceObject))
                {
                    commonBbox31.enlargeToInclude(sourceObject->bbox31);
                }
            }
            for (const auto& building : buildings)
            {
                if (building.hidden)
                    continue;
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
            if (buildingPart.hidden)
                continue;
            processPrimitive(buildingPart, vertices, indices, elevationDataMap, request.tileId, request.zoom,
                buildingPassages, colors, &primaryBuildings);
        }

        for (const auto& building : buildings)
        {
            if (building.hidden)
                continue;
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

    outTiledData = std::make_shared<Map3DObjectsTiledProvider::Data>(
        request.tileId, request.zoom, vertices, indices, parts, show3DbuildingParts);
    return true;
}

bool Map3DObjectsTiledProvider_P::isVisibleBuildingPart(const std::shared_ptr<const OsmAnd::ObfMapObject>& part) const
{
    bool result = part->containsCaptionTag(QStringLiteral("height"))
        || part->containsTag(QStringLiteral("building:material"), true);

    return result;
}

void Map3DObjectsTiledProvider_P::filterBuildings(
    QSet<BuildingPrimitive>& buildings,
    QSet<BuildingPrimitive>& buildingParts) const
{
    const int64_t maxDelta = MAX_COORDINATE_DELTA;
    const int64_t maxInt = INT32_MAX;
    for (auto& building : buildings)
    {
        if (const auto& buildingSource =
            std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(building.primitive->sourceObject))
        {
            bool isHidden = false;
            bool isOutline = buildingSource->containsTag(QStringLiteral("role_outline"), true);
            const AreaI buildingArea(
                building.bbox31.top() - MAX_COORDINATE_DELTA,
                building.bbox31.left() - MAX_COORDINATE_DELTA,
                static_cast<int>(qMin(maxDelta + building.bbox31.bottom(), maxInt)),
                static_cast<int>(qMin(maxDelta + building.bbox31.right(), maxInt)));
            for (auto& buildingPart : buildingParts)
            {
                const auto& buildingPartSource =
                    std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(buildingPart.primitive->sourceObject);
                if (buildingArea.contains(buildingPartSource->bbox31) && Utilities::includes(
                    buildingSource->points31, buildingPartSource->points31, MAX_COORDINATE_DELTA))
                {
                    buildingPart.bbox31.enlargeToInclude(buildingSource->bbox31);
                    buildingPart.parentSourceObject = buildingSource;
                    if (building.polygonColor != 0)
                        buildingPart.polygonColor = building.polygonColor;
                    if (isOutline)
                        buildingPart.hidden = false;
                    else if (!buildingPart.hidden)
                        isHidden = true;
                }
            }
            if ((isOutline || isHidden)
                && (!buildingSource->containsTag(QStringLiteral("building:part"))
                || buildingSource->containsAttribute(QStringLiteral("building:part"), QStringLiteral("no"))
                || !buildingSource->innerPolygonsPoints31.isEmpty()
                || buildingSource->containsTag(QStringLiteral("role_outer"), true)
                || buildingSource->containsTag(QStringLiteral("role_inner"), true)))
                building.hidden = true;
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
    const bool show3DbuildingParts,
    QSet<BuildingPrimitive>& outBuildings,
    QSet<BuildingPrimitive>& outBuildingParts,
    QSet<PassagePrimitive>& outBuildingPassages) const
{
    const auto& sourceObject = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(polylinePrimitive->sourceObject);
    if (!sourceObject || sourceObject->points31.size() < 2)
        return;

    if (polylinePrimitive->type == MapPrimitiviser::PrimitiveType::Polygon && sourceObject->points31.size() > 2)
    {
        BuildingPrimitive buildingPrimitive;
        buildingPrimitive.primitive = polylinePrimitive;
        buildingPrimitive.polygonColor = getPolygonColor(buildingPrimitive.primitive);
        buildingPrimitive.bbox31 = sourceObject->bbox31;
        buildingPrimitive.hidden = false;

        if (sourceObject->containsTag(QStringLiteral("building")))
        {
            const auto layer = QStringLiteral("layer");
            const auto layerValue = sourceObject->getResolvedAttribute(QStringRef(&layer));
            if (!layerValue.startsWith(QLatin1Char('-')))
                insertOrUpdateBuilding(buildingPrimitive, outBuildings);
        }
        else if (show3DbuildingParts && sourceObject->containsTag(QStringLiteral("building:part")))
        {
            buildingPrimitive.hidden = !isVisibleBuildingPart(sourceObject);
            insertOrUpdateBuilding(buildingPrimitive, outBuildingParts);
        }
    }
    else if (polylinePrimitive->type == MapPrimitiviser::PrimitiveType::Polyline
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
        const auto width = QStringLiteral("width");
        const auto height = QStringLiteral("height");
        passagePrimitive.height = Utilities::parseLength(sourceObject->getResolvedAttribute(QStringRef(&height)),
            PASSAGE_DEFAULT_HEIGHT);
        const auto widthValue = Utilities::parseLength(sourceObject->getResolvedAttribute(QStringRef(&width)),
            PASSAGE_DEFAULT_WIDTH);
        passagePrimitive.halfWidth31 =
            qRound(widthValue / (2.0f * Utilities::getMetersPer31Coordinate(passagePrimitive.centerPoint)));
        passagePrimitive.putStart = false;
        passagePrimitive.putEnd = false;
        passagePrimitive.withStart = false;
        passagePrimitive.withEnd = false;
        passagePrimitive.preStart = false;
        passagePrimitive.preEnd = false;
        passagePrimitive.useWide = true;
        insertOrUpdatePassage(passagePrimitive, outBuildingPassages);
    }
}

void Map3DObjectsTiledProvider_P::collectFromPolygons(
    const std::shared_ptr<const MapPrimitiviser::Primitive>& polygonPrimitive,
    const bool show3DbuildingParts,
    QSet<BuildingPrimitive>& outBuildings,
    QSet<BuildingPrimitive>& outBuildingParts) const
{
    const auto& sourceObject = std::dynamic_pointer_cast<const OsmAnd::ObfMapObject>(polygonPrimitive->sourceObject);
    if (!sourceObject || sourceObject->points31.size() < 3)
        return;

    BuildingPrimitive buildingPrimitive;
    buildingPrimitive.primitive = polygonPrimitive;
    buildingPrimitive.polygonColor = getPolygonColor(buildingPrimitive.primitive);
    buildingPrimitive.bbox31 = sourceObject->bbox31;
    buildingPrimitive.hidden = false;

    if (sourceObject->containsTag(QStringLiteral("building")))
    {
        const auto layer = QStringLiteral("layer");
        const auto layerValue = sourceObject->getResolvedAttribute(QStringRef(&layer));
        if (!layerValue.startsWith(QLatin1Char('-')))
            insertOrUpdateBuilding(buildingPrimitive, outBuildings);
    }
    else if (show3DbuildingParts && sourceObject->containsTag(QStringLiteral("building:part")))
    {
        buildingPrimitive.hidden = !isVisibleBuildingPart(sourceObject);
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
        const auto& captionTag = sourceObject->attributeMapping->decodeMap[captionAttributeId].tag;

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

    auto color = _useCustomColor ? _customColor : FColorRGB(getUseDefaultBuildingsColor()
        || primitive.polygonColor == 0 ? getDefaultBuildingsColor() : FColorARGB(primitive.polygonColor));

    // Set an individual color to a particular (selected) building
    if (primaryBuildings)
    {
        const auto& source = primitive.parentSourceObject ? primitive.parentSourceObject : sourceObject;
        // Keep selected buildings separately to draw them first
        auto end = objectColors.end();
        for (auto it = objectColors.begin(); it != end; it++)
        {
            auto tileId = it.key();
            const PointI location31(tileId.x, tileId.y);
            if (primitive.bbox31.contains(location31))
            {
                if (Utilities::includes(source->points31, location31))
                {
                    bool inside = true;
                    if (!source->innerPolygonsPoints31.isEmpty())
                    {
                        for (const auto& innerPolygon : constOf(source->innerPolygonsPoints31))
                        {
                            if (innerPolygon.isEmpty())
                                continue;

                            if (Utilities::includes(innerPolygon, location31))
                            {
                                inside = false;
                                break;
                            }
                        }
                    }
                    if (inside)
                    {
                        primitive.color = it.value();
                        primaryBuildings->append(&primitive);
                        return;
                    }
                }
            }
        }
    }
    else
        color = primitive.color;

    glm::vec4 colorVec(color.r, color.g, color.b, 0.0f);

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
        BuildingVertex v{glm::ivec2(p.x, p.y),
            glm::vec4(0.0f), glm::vec2(height, terrainHeight), glm::vec3(0.0f, 1.0f, 0.0f), colorVec};

        outerRing.push_back({p.x, p.y});
        vertices.append(v);
    }

    topPolygon.push_back(std::move(outerRing));

    for (const auto& innerPoly : innerPolygons)
    {
        std::vector<std::array<int32_t, 2>> innerRing;

        for (const auto& p : innerPoly)
        {
            BuildingVertex v{glm::ivec2(p.x, p.y),
                glm::vec4(0.0f), glm::vec2(height, terrainHeight), glm::vec3(0.0f, 1.0f, 0.0f), colorVec};

            innerRing.push_back({p.x, p.y});
            vertices.append(v);
        }

        topPolygon.push_back(std::move(innerRing));
    }

    const auto zoomLevelDelta = MaxZoomLevel - zoom;
    const PointD minTile31(tileId.x << zoomLevelDelta, tileId.y << zoomLevelDelta);
    const PointD maxTile31((tileId.x + 1) << zoomLevelDelta, (tileId.y + 1) << zoomLevelDelta);

    cutMeshForTile(
        mapbox::earcut<uint16_t>(topPolygon),
        vertices,
        indices,
        currentVertexOffset,
        minTile31,
        maxTile31);

    // Construct walls of the mesh
    const bool withPassages = !passages.isEmpty();
    const auto stageCount = withPassages ? 2 : 1;
    for (int stage = 0; stage < stageCount; stage++)
    {
        for (int polyIdx = -1; polyIdx < innerPolygons.size(); polyIdx++)
        {
            const auto& points = polyIdx < 0 ? points31 : innerPolygons[polyIdx];

            int count = polyIdx < 0 ? edgePointsCount : points.size();
            if (count < 3)
                continue;

            glm::vec3 prevNormal, nextNormal;
            bool prevCurved = false;
            bool nextCurved = false;
            for (int i = 0; i < count + 1; i++)
            {
                const auto& point31_i = points[i % count];
                auto point31_next = points[(i + 1) % count];
                auto point31_future = points[(i + 2) % count];
                auto nextSide = glm::dvec2(point31_future.x - point31_next.x, point31_future.y - point31_next.y);
                auto nextSideLengthSqr = nextSide.x * nextSide.x + nextSide.y * nextSide.y;
                if (i > 0 && nextSideLengthSqr < MIN_ALLOWED_SQR_DISTANCE)
                {
                    i++;
                    point31_next = points[(i + 1) % count];
                    point31_future = points[(i + 2) % count];
                    nextSide = glm::dvec2(point31_future.x - point31_next.x, point31_future.y - point31_next.y);
                    nextSideLengthSqr = nextSide.x * nextSide.x + nextSide.y * nextSide.y;
                }
                const auto nextSideN = nextSide / qSqrt(nextSideLengthSqr);
                const auto side = glm::dvec2(point31_next.x - point31_i.x, point31_next.y - point31_i.y);
                const auto sideLengthSqr = side.x * side.x + side.y * side.y;
                if (qFuzzyIsNull(sideLengthSqr))
                    continue;
                const auto sideLength = qSqrt(sideLengthSqr);
                const auto sideN = side / sideLength;
                prevCurved = nextCurved;
                if (prevCurved)
                    prevNormal = nextNormal;
                nextNormal = calculateNormalFrom2Points(point31_i, point31_next);
                if (!prevCurved)
                    prevNormal = nextNormal;
                nextCurved = glm::dot(sideN, nextSideN) >= MIN_COSINE_CURVED_WALL;
                if (nextCurved)
                {
                    const auto avgLine = glm::normalize(sideN + nextSideN);
                    nextNormal = glm::vec3(-avgLine.y, 0.0f, avgLine.x);
                }

                if (i == 0)
                    continue;

                const auto width31 = static_cast<float>(sideLength);

                QList<PassagePrimitive*> passagePrimitives;
                if (withPassages)
                {
                    const auto hSize = prevCurved || nextCurved ? -width31 * 2.0f : width31;
                    const auto vSize = height - minHeight;

                    for (auto passage : passages)
                    {
                        const auto minTop = passage->height / PASSAGE_DEFAULT_HEIGHT;
                        if (height <= minTop)
                            continue;
                        if (!passage->preStart || stage > 0)
                        {
                            const auto halfN = static_cast<double>(passage->halfWidth31) / sideLength;
                            const auto minGapN = halfN / PASSAGE_DEFAULT_WIDTH;
                            const auto distN = glm::dot(glm::dvec2(
                                passage->startingPoint.x - point31_i.x,
                                passage->startingPoint.y - point31_i.y), side) / sideLengthSqr;
                            if (distN > 0.0 && distN < 1.0 && distN > minGapN && 1.0 - distN > minGapN)
                            {
                                const auto inaccuracy = glm::dot(glm::dvec2(sideN.y, -sideN.x), glm::dvec2(
                                    passage->startingSegment.x, passage->startingSegment.y)) * PASSAGE_MAX_WALL_GAP;
                                auto proj = side * distN;
                                auto proj31 = PointI(qRound(proj.x), qRound(proj.y));
                                const auto point31 = point31_i + proj31;
                                const auto dist = (point31 - passage->startingPoint).norm();
                                if (dist < inaccuracy && (stage < 1 || (passage->preStart && !passage->withStart)))
                                {
                                    if (stage < 1 || !passage->useWide)
                                    {
                                        const bool useWide = distN > halfN && 1.0 - distN > halfN && passage->useWide;
                                        passage->useWide = useWide;
                                        if (!useWide)
                                            passage->height = qMin(passage->height, PASSAGE_DEFAULT_HEIGHT / 2.0f);
                                        const auto passageHeight = qMin(passage->height,
                                            height - passage->height < minTop ? height - minTop : passage->height);
                                        passage->height = passageHeight;
                                        const auto vOffset = passageHeight / height;
                                        const auto backDistance = useWide ? halfN : minGapN;
                                        proj = side * backDistance;
                                        proj31 = PointI(qRound(proj.x), qRound(proj.y));
                                        const auto pointLeft = point31 - proj31;
                                        const auto leftN =
                                            prevCurved ? 0.5f : static_cast<float>(distN - backDistance);
                                        const auto leftNormal = prevCurved ? glm::normalize(
                                            prevNormal + (nextNormal - prevNormal) * leftN) : prevNormal;
                                        passage->startBottomLeft =
                                            BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                                            glm::vec4(leftN, 0.0f, hSize, vSize),
                                            glm::vec2(minHeight, baseTerrainHeight), leftNormal, colorVec};
                                        passage->startTopLeft =
                                            BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                                            glm::vec4(leftN, vOffset, hSize, vSize),
                                            glm::vec2(passageHeight, terrainHeight), leftNormal, colorVec};
                                        const auto forthDistance = useWide ? halfN : minGapN;
                                        proj = side * forthDistance;
                                        proj31 = PointI(qRound(proj.x), qRound(proj.y));
                                        const auto pointRight = point31 + proj31;
                                        const auto rightN =
                                            nextCurved ? 0.5f : static_cast<float>(distN + forthDistance);
                                        const auto rightNormal = nextCurved ? glm::normalize(
                                            prevNormal + (nextNormal - prevNormal) * rightN) : nextNormal;
                                        passage->startBottomRight =
                                            BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                                            glm::vec4(rightN, 0.0f, hSize, vSize),
                                            glm::vec2(minHeight, baseTerrainHeight), rightNormal, colorVec};
                                        passage->startTopRight =
                                            BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                                            glm::vec4(rightN, vOffset, hSize, vSize),
                                            glm::vec2(passageHeight, terrainHeight), rightNormal, colorVec};
                                        passage->gapStartingPoint.x = qRound((distN - backDistance) * sideLength);
                                        passage->gapStartingPoint.y = qRound((distN + forthDistance) * sideLength);
                                        passage->preStart = true;
                                    }
                                    if (stage > 0)
                                    {
                                        passage->putStart = true;
                                        passage->withStart = true;
                                        passagePrimitives.append(passage);
                                    }
                                    continue;
                                }
                            }
                        }
                        if (!passage->preEnd || stage > 0)
                        {
                            const auto halfN = static_cast<double>(passage->halfWidth31) / sideLength;
                            const auto minGapN = halfN / PASSAGE_DEFAULT_WIDTH;
                            const auto distN = glm::dot(glm::dvec2(
                                passage->endingPoint.x - point31_i.x,
                                passage->endingPoint.y - point31_i.y), side) / sideLengthSqr;
                            if (distN > 0.0 && distN < 1.0 && distN > minGapN && 1.0 - distN > minGapN)
                            {
                                const auto inaccuracy = glm::dot(glm::dvec2(-sideN.y, sideN.x), glm::dvec2(
                                    passage->endingSegment.x, passage->endingSegment.y)) * PASSAGE_MAX_WALL_GAP;
                                auto proj = side * distN;
                                auto proj31 = PointI(qRound(proj.x), qRound(proj.y));
                                const auto point31 = point31_i + proj31;
                                const auto dist = (point31 - passage->endingPoint).norm();
                                if (dist < inaccuracy && (stage < 1 || (passage->preEnd && !passage->withEnd)))
                                {
                                    if (stage < 1 || !passage->useWide)
                                    {
                                        const bool useWide = distN > halfN && 1.0 - distN > halfN && passage->useWide;
                                        passage->useWide = useWide;
                                        if (!useWide)
                                            passage->height = qMin(passage->height, PASSAGE_DEFAULT_HEIGHT / 2.0f);
                                        const auto passageHeight = qMin(passage->height,
                                            height - passage->height < minTop ? height - minTop : passage->height);
                                        passage->height = passageHeight;
                                        const auto vOffset = passageHeight / height;
                                        const auto backDistance = useWide ? halfN : minGapN;
                                        proj = side * backDistance;
                                        proj31 = PointI(qRound(proj.x), qRound(proj.y));
                                        const auto pointLeft = point31 - proj31;
                                        const auto leftN =
                                            prevCurved ? 0.5f : static_cast<float>(distN - backDistance);
                                        const auto leftNormal = prevCurved ? glm::normalize(
                                            prevNormal + (nextNormal - prevNormal) * leftN) : prevNormal;
                                        passage->endBottomLeft =
                                            BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                                            glm::vec4(leftN, 0.0f, hSize, vSize),
                                            glm::vec2(minHeight, baseTerrainHeight), leftNormal, colorVec};
                                        passage->endTopLeft =
                                            BuildingVertex{glm::ivec2(pointLeft.x, pointLeft.y),
                                            glm::vec4(leftN, vOffset, hSize, vSize),
                                            glm::vec2(passageHeight, terrainHeight), leftNormal, colorVec};
                                        const auto forthDistance = useWide ? halfN : minGapN;
                                        proj = side * forthDistance;
                                        proj31 = PointI(qRound(proj.x), qRound(proj.y));
                                        const auto pointRight = point31 + proj31;
                                        const auto rightN =
                                            nextCurved ? 0.5f : static_cast<float>(distN + forthDistance);
                                        const auto rightNormal = nextCurved ? glm::normalize(
                                            prevNormal + (nextNormal - prevNormal) * rightN) : nextNormal;
                                        passage->endBottomRight =
                                            BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                                            glm::vec4(rightN, 0.0f, hSize, vSize),
                                            glm::vec2(minHeight, baseTerrainHeight), rightNormal, colorVec};
                                        passage->endTopRight =
                                            BuildingVertex{glm::ivec2(pointRight.x, pointRight.y),
                                            glm::vec4(rightN, vOffset, hSize, vSize),
                                            glm::vec2(passageHeight, terrainHeight), rightNormal, colorVec};
                                        passage->gapEndingPoint.x = qRound((distN - backDistance) * sideLength);
                                        passage->gapEndingPoint.y = qRound((distN + forthDistance) * sideLength);
                                        passage->preEnd = true;
                                    }
                                    if (stage > 0)
                                    {
                                        passage->putEnd = true;
                                        passage->withEnd = true;
                                        passagePrimitives.append(passage);
                                    }
                                }
                            }
                        }
                    }
                    if (stage < 1)
                        continue;
                }

                generateBuildingWall(
                    vertices,
                    indices,
                    point31_i,
                    point31_next,
                    prevNormal,
                    nextNormal,
                    prevCurved,
                    nextCurved,
                    minHeight,
                    height,
                    baseTerrainHeight,
                    terrainHeight,
                    width31,
                    minTile31,
                    maxTile31,
                    colorVec,
                    passagePrimitives);
            }
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
    const BuildingVertex& startVertex,
    const BuildingVertex& endVertex,
    const double range,
    const int startIndex /* = 0 */,
    const int endIndex /* = 0 */,
    QVector<PointD>* coords /* = nullptr */) const
{
    BuildingVertex vertex = startVertex;
    if (coords)
    {
        auto startPoint = coords->value(startIndex, PointD(startVertex.location31));
        auto endPoint = coords->value(endIndex, PointD(endVertex.location31));
        auto location31(startPoint + (endPoint - startPoint) * range);
        coords->append(location31);
        vertex.location31.x = qRound(location31.x);
        vertex.location31.y = qRound(location31.y);
    }
    else
    {
        vertex.location31.x += qRound(static_cast<double>(endVertex.location31.x - startVertex.location31.x) * range);
        vertex.location31.y += qRound(static_cast<double>(endVertex.location31.y - startVertex.location31.y) * range);
    }
    vertex.sizes = glm::dvec4(vertex.sizes) + glm::dvec4(endVertex.sizes - startVertex.sizes) * range;
    if (startVertex.heights.x != 0.0f && endVertex.heights.x != 0.0f)
        vertex.heights = glm::dvec2(vertex.heights) + glm::dvec2(endVertex.heights - startVertex.heights) * range;
    else
    {
        vertex.sizes.y = 0.0f;
        vertex.heights.x = 0.0f;
        vertex.heights.y = startVertex.heights.x == 0.0f ? startVertex.heights.y : endVertex.heights.y;
    }
    const auto factor = static_cast<float>(range);
    vertex.normal = glm::normalize(startVertex.normal + (endVertex.normal - startVertex.normal) * factor);
    vertex.color += (endVertex.color - startVertex.color) * factor;
    return vertex;
}

inline void Map3DObjectsTiledProvider_P::appendOneTriangle(
    QVector<BuildingVertex>& vertices,
    QVector<uint16_t>& outIndices,
    QVector<PointD>& coords,
    uint16_t& index,
    const double max,
    const double ad,
    const double bd,
    const double cd,
    const int ai,
    const int bi,
    const int ci,
    const int offset) const
{
    if (vertices[ai].heights.x != 0.0f)
    {
        outIndices.append(ai);
        vertices.append(getIntersection(
            vertices[bi], vertices[ai], (max - bd) / (ad - bd), bi - offset, ai - offset, &coords));
        outIndices.append(index++);
        vertices.append(getIntersection(
            vertices[ci], vertices[ai], (max - cd) / (ad - cd), ci - offset, ai - offset, &coords));
        outIndices.append(index++);
    }
}

inline void Map3DObjectsTiledProvider_P::appendTwoTriangles(
    QVector<BuildingVertex>& vertices,
    QVector<uint16_t>& outIndices,
    QVector<PointD>& coords,
    uint16_t& index,
    const double max,
    const double ad,
    const double bd,
    const double cd,
    const int ai,
    const int bi,
    const int ci,
    const int offset) const
{
    int count = 0;
    if (vertices[ai].heights.x != 0.0f || vertices[bi].heights.x != 0.0f)
    {
        vertices.append(getIntersection(
            vertices[bi], vertices[ai], (max - bd) / (ad - bd), bi - offset, ai - offset, &coords));
        count++;
    }
    if (vertices[ai].heights.x != 0.0f || vertices[ci].heights.x != 0.0f)
    {
        vertices.append(getIntersection(
            vertices[ci], vertices[ai], (max - cd) / (ad - cd), ci - offset, ai - offset, &coords));
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

template <typename T>
void Map3DObjectsTiledProvider_P::cutMeshAlongMaxEdge(
    const T& indices,
    QVector<BuildingVertex>& vertices,
    uint16_t& index,
    QVector<uint16_t>& outIndices,
    QVector<PointD>& coords,
    const uint16_t baseOffset,
    const int offset,
    const double maxValue,
    const bool getY) const
{
    auto count = indices.size();
    for (int i = 0; i < count; i += 3)
    {
        const int ai = indices[i] + baseOffset;
        const int bi = indices[i + 1] + baseOffset;
        const int ci = indices[i + 2] + baseOffset;
        PointD point = coords.value(ai - offset, PointD(vertices[ai].location31));
        const auto a = getY ? point.y : point.x;
        point = coords.value(bi - offset, PointD(vertices[bi].location31));
        const auto b = getY ? point.y : point.x;
        point = coords.value(ci - offset, PointD(vertices[ci].location31));
        const auto c = getY ? point.y : point.x;
        if (a > maxValue && b > maxValue && c > maxValue)
            continue;
        if (a <= maxValue && b <= maxValue && c <= maxValue)
        {
            outIndices.append(static_cast<uint16_t>(ai));
            outIndices.append(static_cast<uint16_t>(bi));
            outIndices.append(static_cast<uint16_t>(ci));
        }
        else if (b <= maxValue && c <= maxValue)
            appendTwoTriangles(vertices, outIndices, coords, index, maxValue, a, b, c, ai, bi, ci, offset);
        else if (a <= maxValue && c <= maxValue)
            appendTwoTriangles(vertices, outIndices, coords, index, maxValue, b, c, a, bi, ci, ai, offset);
        else if (a <= maxValue && b <= maxValue)
            appendTwoTriangles(vertices, outIndices, coords, index, maxValue, c, a, b, ci, ai, bi, offset);
        else if (a <= maxValue)
            appendOneTriangle(vertices, outIndices, coords, index, maxValue, a, b, c, ai, bi, ci, offset);
        else if (b <= maxValue)
            appendOneTriangle(vertices, outIndices, coords, index, maxValue, b, c, a, bi, ci, ai, offset);
        else
            appendOneTriangle(vertices, outIndices, coords, index, maxValue, c, a, b, ci, ai, bi, offset);
    }
}

template void Map3DObjectsTiledProvider_P::cutMeshAlongMaxEdge<std::vector<uint16_t>>(
    const std::vector<uint16_t>&,
    QVector<BuildingVertex>&,
    uint16_t&,
    QVector<uint16_t>&,
    QVector<PointD>&,
    const uint16_t,
    const int,
    const double,
    const bool) const;

template void Map3DObjectsTiledProvider_P::cutMeshAlongMaxEdge<QVector<uint16_t>>(
    const QVector<uint16_t>&,
    QVector<BuildingVertex>&,
    uint16_t&,
    QVector<uint16_t>&,
    QVector<PointD>&,
    const uint16_t,
    const int,
    const double,
    const bool) const;

void Map3DObjectsTiledProvider_P::cutMeshAlongMinEdge(
    const QVector<uint16_t>& indices,
    QVector<BuildingVertex>& vertices,
    uint16_t& index,
    QVector<uint16_t>& outIndices,
    QVector<PointD>& coords,
    const int offset,
    const double minValue,
    const bool getY) const
{
    auto count = indices.size();
    for (int i = 0; i < count; i += 3)
    {
        const int ai = indices[i];
        const int bi = indices[i + 1];
        const int ci = indices[i + 2];
        PointD point = coords.value(ai - offset, PointD(vertices[ai].location31));
        const auto a = getY ? point.y : point.x;
        point = coords.value(bi - offset, PointD(vertices[bi].location31));
        const auto b = getY ? point.y : point.x;
        point = coords.value(ci - offset, PointD(vertices[ci].location31));
        const auto c = getY ? point.y : point.x;
        if (a < minValue && b < minValue && c < minValue)
            continue;
        if (a >= minValue && b >= minValue && c >= minValue)
        {
            outIndices.append(static_cast<uint16_t>(ai));
            outIndices.append(static_cast<uint16_t>(bi));
            outIndices.append(static_cast<uint16_t>(ci));
        }
        else if (b >= minValue && c >= minValue)
            appendTwoTriangles(vertices, outIndices, coords, index, minValue, a, b, c, ai, bi, ci, offset);
        else if (a >= minValue && c >= minValue)
            appendTwoTriangles(vertices, outIndices, coords, index, minValue, b, c, a, bi, ci, ai, offset);
        else if (a >= minValue && b >= minValue)
            appendTwoTriangles(vertices, outIndices, coords, index, minValue, c, a, b, ci, ai, bi, offset);
        else if (a >= minValue)
            appendOneTriangle(vertices, outIndices, coords, index, minValue, a, b, c, ai, bi, ci, offset);
        else if (b >= minValue)
            appendOneTriangle(vertices, outIndices, coords, index, minValue, b, c, a, bi, ci, ai, offset);
        else
            appendOneTriangle(vertices, outIndices, coords, index, minValue, c, a, b, ci, ai, bi, offset);
    }
}

void Map3DObjectsTiledProvider_P::cutMeshForTile(
    const std::vector<uint16_t>& indices,
    QVector<BuildingVertex>& vertices,
    QVector<uint16_t>& outIndices,
    const uint16_t baseOffset,
    const PointD& minTile31,
    const PointD& maxTile31) const
{
    auto index = static_cast<uint16_t>(vertices.size());
    QVector<uint16_t> tmpIndices, midIndices;
    QVector<PointD> midCoords;
    int offset = index;
    cutMeshAlongMaxEdge(indices, vertices, index, midIndices, midCoords, baseOffset, offset, maxTile31.x, false);
    tmpIndices.clear();
    cutMeshAlongMinEdge(midIndices, vertices, index, tmpIndices, midCoords, offset, minTile31.x, false);
    midIndices.clear();
    cutMeshAlongMaxEdge(tmpIndices, vertices, index, midIndices, midCoords, 0, offset, maxTile31.y, true);
    cutMeshAlongMinEdge(midIndices, vertices, index, outIndices, midCoords, offset, minTile31.y, true);
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

bool Map3DObjectsTiledProvider_P::removeObjectColor(const TileId& location31)
{
    QWriteLocker scopedLocker(&_objectColorsLock);

    return _objectColors.remove(location31) > 0;
}

bool Map3DObjectsTiledProvider_P::removeAllObjectColors()
{
    QWriteLocker scopedLocker(&_objectColorsLock);

    if (_objectColors.isEmpty())
        return false;

    _objectColors.clear();

    return true;
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
    const glm::vec3& prevNormal,
    const glm::vec3& nextNormal,
    bool prevCurved,
    bool nextCurved,
    float minHeight,
    float height,
    float baseTerrainHeight,
    float terrainHeight,
    float width31,
    const PointD& minTile31,
    const PointD& maxTile31,
    const glm::vec4& colorVec,
    QList<PassagePrimitive*>& passagePrimitives) const
{
    const uint32_t baseVertexOffset = vertices.size();

    // Should match the side order (counter-clockwise)
    const auto hSize = prevCurved || nextCurved ? -width31 * 2.0f : width31;
    const auto vSize = height - minHeight;
    vertices.append({glm::ivec2(point31_next.x, point31_next.y),
        glm::vec4(nextCurved ? 0.5f : 1.0f, 0.0f, hSize, vSize),
        glm::vec2(minHeight, baseTerrainHeight), nextNormal, colorVec});
    vertices.append({glm::ivec2(point31_next.x, point31_next.y),
        glm::vec4(nextCurved ? 0.5f : 1.0f, 1.0f, hSize, vSize),
        glm::vec2(height, terrainHeight), nextNormal, colorVec});
    vertices.append({glm::ivec2(point31_i.x, point31_i.y),
        glm::vec4(prevCurved ? 0.5f : 0.0f, 1.0f, hSize, vSize),
        glm::vec2(height, terrainHeight), prevNormal, colorVec});
    vertices.append({glm::ivec2(point31_i.x, point31_i.y),
        glm::vec4(prevCurved ? 0.5f : 0.0f, 0.0f, hSize, vSize),
        glm::vec2(minHeight, baseTerrainHeight), prevNormal, colorVec});

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
                const auto center = (passagePrimitive->gapStartingPoint.x + passagePrimitive->gapStartingPoint.y) / 2;
                passages.insert(center, passagePrimitive);
            }
            else if (passagePrimitive->putEnd)
            {
                const auto center = (passagePrimitive->gapEndingPoint.x + passagePrimitive->gapEndingPoint.y) / 2;
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

                if (passage->gapStartingPoint.x > prevDistance)
                {
                    prevDistance = passage->gapStartingPoint.y;

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

                if (passage->gapEndingPoint.x > prevDistance)
                {
                    prevDistance = passage->gapEndingPoint.y;

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
