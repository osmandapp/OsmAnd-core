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
        struct BuildingPrimitive
        {
            std::shared_ptr<const MapPrimitiviser::Primitive> primitive;
            uint32_t polygonColor = 0;
        };

        struct PassagePrimitive
        {
            std::shared_ptr<const MapPrimitiviser::Primitive> primitive;
            PointI startingPoint;
            PointI endingPoint;
            PointI centerPoint;
            float height;
            int halfWidth31;
            bool putStart;
            bool putEnd;
            bool withStart;
            bool withEnd;
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
        std::shared_ptr<IMapElevationDataProvider> _elevationProvider;

        void processPrimitive(const BuildingPrimitive& primitive,
                              Buildings3D& buildings3D,
                              const std::shared_ptr<IMapElevationDataProvider::Data>& elevationData,
                              const TileId& tileId,
                              const ZoomLevel zoom,
                              QSet<PassagePrimitive>& buildingPassages) const;

        void collectFromPolyline(const std::shared_ptr<const MapPrimitiviser::Primitive>& polylinePrimitive,
                                 QSet<BuildingPrimitive>& outBuildings,
                                 QSet<BuildingPrimitive>& outBuildingParts,
                                 QSet<PassagePrimitive>& outBuildingPassages) const;

        void collectFromPolygons(const std::shared_ptr<const MapPrimitiviser::Primitive>& polygonPrimitive,
                                QSet<BuildingPrimitive>& outBuildings,
                                QSet<BuildingPrimitive>& outBuildingParts) const;

        void filterBuildings(QSet<BuildingPrimitive>& buildings,
                             QSet<BuildingPrimitive>& buildingParts) const;

        void insertOrUpdateBuilding(const BuildingPrimitive& primitive, QSet<BuildingPrimitive>& outCollection) const;
        void insertOrUpdatePassage(const PassagePrimitive& primitive, QSet<PassagePrimitive>& outCollection) const;

        inline BuildingVertex getIntersection(
            const BuildingVertex& startVertex,
            const BuildingVertex& endVertex,
            double range) const;

        inline void appendOneTriangle(
            QVector<BuildingVertex>& vertices,
            QVector<uint16_t>& outIndices,
            uint16_t& index,
            const int max,
            const int ad,
            const int bd,
            const int cd,
            const uint16_t ai,
            const uint16_t bi,
            const uint16_t ci) const;

        inline void appendTwoTriangles(
            QVector<BuildingVertex>& vertices,
            QVector<uint16_t>& outIndices,
            uint16_t& index,
            const int max,
            const int ad,
            const int bd,
            const int cd,
            const uint16_t ai,
            const uint16_t bi,
            const uint16_t ci) const;

        void cutMeshForTile(
            const std::vector<uint16_t>& indices,
            QVector<BuildingVertex>& vertices,
            QVector<uint16_t>& outIndices,
            const uint16_t offset,
            const PointI& minTile31,
            const PointI& maxTile31) const;

        void accumulateElevationForPoint(
            const PointI& point31,
            const TileId& tileId,
            ZoomLevel zoom,
            const std::shared_ptr<IMapElevationDataProvider::Data>& elevationData,
            float& maxElevationMeters,
            float& minElevationMeters,
            bool& hasElevationSample) const;

        glm::vec3 calculateNormalFrom2Points(PointI point31_i, PointI point31_next) const;
        uint32_t getPolygonColor(const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive) const;

        void generateBuildingWall(
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

        friend class OsmAnd::Map3DObjectsTiledProvider;

    private:
        std::vector<uint16_t> fullSideIndices;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_P_H_)


