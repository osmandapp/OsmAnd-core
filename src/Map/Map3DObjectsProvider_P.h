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
        struct Primirive3D
        {
            std::shared_ptr<const MapPrimitiviser::Primitive> primitive;
            uint32_t polygonColor = 0;
        };

    private:
        std::shared_ptr<MapPrimitivesProvider> _tiledProvider;
        std::shared_ptr<MapPresentationEnvironment> _environment;
        std::shared_ptr<IMapElevationDataProvider> _elevationProvider;

        void processPrimitive(const Primirive3D& primitive,
                              Buildings3D& buildings3D,
                              const std::shared_ptr<IMapElevationDataProvider::Data>& elevationData,
                              const TileId& tileId,
                              const ZoomLevel zoom,
                              const QHash<std::shared_ptr<const MapPrimitiviser::Primitive>, float>& passagesData) const;

        void collectFromPoliline(const std::shared_ptr<const MapPrimitiviser::Primitive>& polylinePrimitive,
                                 QSet<Primirive3D>& outBuildings,
                                 QSet<Primirive3D>& outBuildingParts,
                                 QHash<std::shared_ptr<const MapPrimitiviser::Primitive>, float>& outBuildingPassages) const;

        void collectFromPolygons(const std::shared_ptr<const MapPrimitiviser::Primitive>& polygonPrimitive,
                                QSet<Primirive3D>& outBuildings,
                                QSet<Primirive3D>& outBuildingParts) const;

        void filterBuildings(QSet<Primirive3D>& buildings,
                             QSet<Primirive3D>& buildingParts) const;

        void insertOrUpdateBuilding(const Primirive3D& primitive, QSet<Primirive3D>& outCollection) const;

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
            float minHeight,
            float height,
            float terrainHeight,
            const glm::vec3& colorVec,
            bool generateOutline,
            const PointI& passageIntersection,
            float passageHeight,
            bool hasPassage,
            QVector<BuildingVertex>& passageVertices) const;

    protected:
        Map3DObjectsTiledProvider_P(Map3DObjectsTiledProvider* const owner,
            const std::shared_ptr<MapPrimitivesProvider>& tiledProvider,
            const std::shared_ptr<MapPresentationEnvironment>& environment);

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
        std::vector<uint16_t> pasageSideIndices;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_P_H_)


