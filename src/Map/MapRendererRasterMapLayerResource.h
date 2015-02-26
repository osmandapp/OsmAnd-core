#ifndef _OSMAND_CORE_MAP_RENDERER_RASTER_MAP_LAYER_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_RASTER_MAP_LAYER_RESOURCE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "GPUAPI.h"
#include "MapRendererBaseTiledResource.h"
#include "IRasterMapLayerProvider.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    class MapRendererRasterMapLayerResource : public MapRendererBaseTiledResource
    {
    private:
    protected:
        MapRendererRasterMapLayerResource(
            MapRendererResourcesManager* owner,
            const IMapDataProvider::SourceType sourceType,
            const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
            const TileId tileId,
            const ZoomLevel zoom);

        std::shared_ptr<IRasterMapLayerProvider::Data> _sourceData;
        std::shared_ptr<const GPUAPI::ResourceInGPU> _resourceInGPU;

        virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController);
        virtual bool uploadToGPU();
        virtual void unloadFromGPU();
        virtual void lostDataInGPU();
        virtual void releaseData();
    public:
        virtual ~MapRendererRasterMapLayerResource();

        const std::shared_ptr<const GPUAPI::ResourceInGPU>& resourceInGPU;

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_RASTER_MAP_LAYER_RESOURCE_H_)
