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
#include <OsmAndCore/PointsAndAreas.h>
#include "Map3DObjectsProvider.h"
#include "MapRenderer3DObjects.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    class MapRenderer3DObjectsResource : public MapRendererBaseTiledResource
    {
    private:
    protected:
        MapRenderer3DObjectsResource(
            MapRendererResourcesManager* owner,
            const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
            const TileId tileId,
            const ZoomLevel zoom);

        std::shared_ptr<Map3DObjectsTiledProvider::Data> _sourceData;
        mutable QReadWriteLock _sourceDataLock;
        std::shared_ptr<const GPUAPI::MeshInGPU> _meshInGPU;

        bool supportsObtainDataAsync() const override;
        bool obtainData(bool& dataAvailable, const std::shared_ptr<const IQueryController>& queryController) override;
        void obtainDataAsync(ObtainDataAsyncCallback callback,
            const std::shared_ptr<const IQueryController>& queryController,
            const bool cacheOnly = false) override;

        bool uploadToGPU() override;
        void unloadFromGPU() override;
        void lostDataInGPU() override;
        void releaseData() override;

        bool getProvider(std::shared_ptr<IMapDataProvider>& provider) const;
    public:
        ~MapRenderer3DObjectsResource() override;

        void captureResourceInGPU(std::shared_ptr<const GPUAPI::MeshInGPU>& meshInGPU) const;
        mutable QAtomicInt resourceInGPULock;

        friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_RESOURCE_H_)
