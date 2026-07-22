#ifndef _OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_P_H_
#define _OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QVector>
#include <QSet>

#include <OsmAndCore/Data/ObfMapObject.h>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "IMapTiledDataProvider.h"
#include "MapPrimitivesProvider.h"
#include "Map3DObjectsProvider.h"

namespace OsmAnd
{
    class Map3DObjectsTiledProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(Map3DObjectsTiledProvider_P);

    public:
        enum RoofShape : int32_t {
            Flat = 0,
            Pyramidal,
            Cone,
            Dome,
            Onion,
            Saltbox,
            Skillion,
            Sawtooth,
            Gabled,
            Gambrel,
            Round,
            Hipped,
            HalfHipped,
            Mansard
        };
        enum RoofOrientation : int32_t {
            Along = 0,
            Across
        };

        struct BuildingPrimitive
        {
            std::shared_ptr<const MapPrimitiviser::Primitive> primitive;
            mutable float height;
            float minHeight;
            float levels;
            float levelHeight;
            float roofHeight;
            float roofAngle;
            float roofDirection;
            RoofShape roofShape;
            RoofOrientation roofOrientation;
            bool heightFound;
            bool levelsFound;
            bool roofHeightFound = false;
            bool isNotPart;
            bool isEmbedded;
            mutable std::shared_ptr<const OsmAnd::ObfMapObject> parentSourceObject;
            mutable uint32_t polygonColor;
            mutable AreaI bbox31;
            mutable FColorRGB color;
            mutable bool hidden;
        };

        struct PassagePrimitive
        {
            std::shared_ptr<const MapPrimitiviser::Primitive> primitive;
            PointI startingPoint;
            PointI endingPoint;
            PointI centerPoint;
            PointI gapStartingPoint;
            PointI gapEndingPoint;
            glm::dvec2 startingSegment;
            glm::dvec2 endingSegment;
            float height;
            int halfWidth31;
            bool putStart;
            bool putEnd;
            bool withStart;
            bool withEnd;
            bool preStart;
            bool preEnd;
            bool useWide;
            BuildingVertex startBottomLeft;
            BuildingVertex startTopLeft;
            BuildingVertex startTopRight;
            BuildingVertex startBottomRight;
            BuildingVertex endBottomLeft;
            BuildingVertex endTopLeft;
            BuildingVertex endTopRight;
            BuildingVertex endBottomRight;
        };

    private:
        std::shared_ptr<MapPrimitivesProvider> _tiledProvider;
        std::shared_ptr<MapPresentationEnvironment> _environment;
        const bool _useCustomColor;
        const FColorRGB _customColor;
        QHash<TileId, FColorRGB> _objectColors;
        mutable QReadWriteLock _objectColorsLock;
        std::shared_ptr<IMapElevationDataProvider> _elevationProvider;

        void processPrimitive(const BuildingPrimitive& primitive,
            QVector<BuildingVertex>& vertices,
            QVector<uint16_t>& indices,
            const QHash<TileId, std::shared_ptr<IMapElevationDataProvider::Data>>& elevationData,
            const TileId& tileId,
            const ZoomLevel zoom,
            const QSet<PassagePrimitive>& buildingPassages,
            const QHash<TileId, FColorRGB>& objectColors,
            QList<const BuildingPrimitive*>* primaryBuildings = nullptr) const;

        void collectFromPolyline(const std::shared_ptr<const MapPrimitiviser::Primitive>& polylinePrimitive,
            const bool isHighDetail,
            QSet<BuildingPrimitive>& outBuildings,
            QSet<BuildingPrimitive>& outBuildingParts,
            QSet<PassagePrimitive>& outBuildingPassages) const;

        void collectFromPolygons(const std::shared_ptr<const MapPrimitiviser::Primitive>& polygonPrimitive,
            const bool isHighDetail,
            QSet<BuildingPrimitive>& outBuildings,
            QSet<BuildingPrimitive>& outBuildingParts) const;

        void filterBuildings(QSet<BuildingPrimitive>& buildings,
            QSet<BuildingPrimitive>& buildingParts) const;

        void insertOrUpdateBuildingPrimitive(
            const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive, const OsmAnd::ObfMapObject& sourceObject,
            bool isPart, bool isHighDetail, QSet<BuildingPrimitive>& outCollection) const;
        void insertOrUpdatePassage(const PassagePrimitive& primitive, QSet<PassagePrimitive>& outCollection) const;

        inline BuildingVertex getIntersection(
            const BuildingVertex& startVertex,
            const BuildingVertex& endVertex,
            const double range,
            const int startIndex = 0,
            const int endIndex = 0,
            QVector<PointD>* vertices = nullptr) const;

        inline void appendOneTriangle(
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
            const int offset) const;

        inline void appendTwoTriangles(
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
            const int offset) const;

        template <typename T>
        void cutMeshAlongMaxEdge(
            const T& indices,
            QVector<BuildingVertex>& vertices,
            uint16_t& index,
            QVector<uint16_t>& outIndices,
            QVector<PointD>& coords,
            const uint16_t baseOffset,
            const int offset,
            const double maxValue,
            const bool getY) const;

        void cutMeshAlongMinEdge(
            const QVector<uint16_t>& indices,
            QVector<BuildingVertex>& vertices,
            uint16_t& index,
            QVector<uint16_t>& outIndices,
            QVector<PointD>& coords,
            const int offset,
            const double minValue,
            const bool getY) const;

        void cutMeshForTile(
            const std::vector<uint16_t>& indices,
            QVector<BuildingVertex>& vertices,
            QVector<uint16_t>& outIndices,
            const uint16_t baseOffset,
            const PointD& minTile31,
            const PointD& maxTile31) const;

        void accumulateElevationForPoint(
            const PointI& point31,
            ZoomLevel zoom,
            const QHash<TileId, std::shared_ptr<IMapElevationDataProvider::Data>>& elevationData,
            float& maxElevationMeters,
            float& minElevationMeters,
            bool& hasElevationSample) const;

        glm::vec3 calculateNormalFrom2Points(PointI point31_i, PointI point31_next) const;
        uint32_t getPolygonColor(const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive) const;

        void generateBuildingWall(
            QVector<BuildingVertex>& vertices,
            QVector<uint16_t>& indices,
            const PointI& point31_i,
            const PointI& point31_next,
            const glm::vec3& prevNormal,
            const glm::vec3& nextNormal,
            bool prevCurved,
            bool nextCurved,
            bool withRoof,
            float minHeight,
            float height,
            float baseTerrainHeight,
            float terrainHeight,
            float width31,
            const PointD& minTile31,
            const PointD& maxTile31,
            const glm::vec4& colorVec,
            QList<PassagePrimitive*>& passagePrimitives) const;

    protected:
        Map3DObjectsTiledProvider_P(Map3DObjectsTiledProvider* const owner,
            const std::shared_ptr<MapPrimitivesProvider>& tiledProvider,
            const std::shared_ptr<MapPresentationEnvironment>& environment,
            const bool useCustomColor,
            const FColorRGB& customColor);

    public:
        ~Map3DObjectsTiledProvider_P();

        ImplementationInterface<Map3DObjectsTiledProvider> owner;

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

        bool getUseDefaultBuildingsColor() const;
        float getDefaultBuildingsHeight() const;
        float getDefaultBuildingsLevelHeight() const;
        FColorARGB getDefaultBuildingsColor() const;

        bool obtainTiledData(
            const IMapTiledDataProvider::Request& request,
            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
            std::shared_ptr<Metric>* const pOutMetric);

        bool supportsNaturalObtainData() const;
        bool supportsNaturalObtainDataAsync() const;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

        void setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& elevationProvider);
        void addObjectColor(const TileId& location31, const FColorRGB& color);
        bool removeObjectColor(const TileId& location31);
        bool removeAllObjectColors();

        friend class OsmAnd::Map3DObjectsTiledProvider;

    private:
        std::vector<uint16_t> fullSideIndices;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_P_H_)


