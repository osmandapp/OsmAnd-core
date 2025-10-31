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
#include "MapRenderer3DObjects.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    class MapRenderer3DObjectsResource : public MapRendererBaseTiledResource
    {
    public:
        struct RenderableBuilding
        {
            std::shared_ptr<const GPUAPI::ArrayBufferInGPU> vertexBuffer;
            std::shared_ptr<const GPUAPI::ElementArrayBufferInGPU> indexBuffer;
            int vertexCount;
            int indexCount;
            glm::vec3 debugColor;
            uint64_t bboxHash;
        };

        ~MapRenderer3DObjectsResource() override;

        const QVector<RenderableBuilding>& getRenderableBuildings() const { return _renderableBuildings; }

        friend class OsmAnd::MapRendererResourcesManager;

    protected:
        MapRenderer3DObjectsResource(
            MapRendererResourcesManager* owner,
            const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
            const TileId tileId,
            const ZoomLevel zoom);

        std::shared_ptr<IMapTiledDataProvider::Data> _sourceData;

        QVector<RenderableBuilding> _renderableBuildings;

        bool supportsObtainDataAsync() const override;
        bool obtainData(bool& dataAvailable, const std::shared_ptr<const IQueryController>& queryController) override;
        void obtainDataAsync(ObtainDataAsyncCallback callback,
            const std::shared_ptr<const IQueryController>& queryController,
            const bool cacheOnly = false) override;

        bool uploadToGPU() override;
        void unloadFromGPU() override;
        void lostDataInGPU() override;
        void releaseData() override;

    private:
        bool getProvider(std::shared_ptr<IMapDataProvider>& provider) const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_RESOURCE_H_)
