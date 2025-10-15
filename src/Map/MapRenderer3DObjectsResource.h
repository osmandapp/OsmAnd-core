#ifndef _OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_RESOURCE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "GPUAPI.h"
#include "MapRendererBaseTiledResource.h"
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>

namespace OsmAnd
{
    class MapRendererResourcesManager;

    class MapRenderer3DObjectsResource : public MapRendererBaseTiledResource
    {
    public:
        struct Vertex
        {
            glm::vec3 position;
        };

        struct TestBuildingResource
        {
            std::shared_ptr<const GPUAPI::ArrayBufferInGPU> vertexBuffer;
            int vertexCount;

            QVector<PointI> debugPoints31;

            TestBuildingResource() : vertexCount(0) {}
            TestBuildingResource(const std::shared_ptr<const GPUAPI::ArrayBufferInGPU>& vb, int count)
                : vertexBuffer(vb), vertexCount(count) {}
        };
        
    private:
    protected:
        MapRenderer3DObjectsResource(
            MapRendererResourcesManager* owner,
            const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
            const TileId tileId,
            const ZoomLevel zoom);

        QList<std::shared_ptr<const MapPrimitiviser::Primitive>> _sourceData;

        QVector<TestBuildingResource> _testBuildings;

        bool supportsObtainDataAsync() const override;
        bool obtainData(bool& dataAvailable, const std::shared_ptr<const IQueryController>& queryController) override;
        void obtainDataAsync(ObtainDataAsyncCallback callback,
            const std::shared_ptr<const IQueryController>& queryController,
            const bool cacheOnly = false) override;

        bool uploadToGPU() override;
        void unloadFromGPU() override;
        void lostDataInGPU() override;
        void releaseData() override;
    public:
        ~MapRenderer3DObjectsResource() override;

        const QVector<TestBuildingResource>& getTestBuildings() const { return _testBuildings; }

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_RESOURCE_H_)


