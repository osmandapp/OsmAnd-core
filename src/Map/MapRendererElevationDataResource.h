#ifndef _OSMAND_CORE_MAP_RENDERER_ELEVATION_DATA_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_ELEVATION_DATA_RESOURCE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "GPUAPI.h"
#include "MapRendererBaseTiledResource.h"
#include "IMapElevationDataProvider.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;
    class ElevationDataTile;

    class MapRendererElevationDataResource : public MapRendererBaseTiledResource
    {
    private:
    protected:
        MapRendererElevationDataResource(
            MapRendererResourcesManager* owner,
            const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
            TileId tileId,
            ZoomLevel zoom);

        std::shared_ptr<IMapElevationDataProvider::Data> _sourceData;
        std::shared_ptr<const GPUAPI::ResourceInGPU> _resourceInGPU;

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
        ~MapRendererElevationDataResource() override;

        const std::shared_ptr<IMapElevationDataProvider::Data>& sourceData;
        const std::shared_ptr<const GPUAPI::ResourceInGPU>& resourceInGPU;

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_ELEVATION_DATA_RESOURCE_H_)
