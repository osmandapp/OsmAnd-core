#ifndef _OSMAND_CORE_MAP_RENDERER_ELEVATION_DATA_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_ELEVATION_DATA_RESOURCE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "GPUAPI.h"
#include "MapRendererBaseTiledResource.h"

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
            const TileId tileId,
            const ZoomLevel zoom);

        std::shared_ptr<ElevationDataTile> _sourceData;
        std::shared_ptr<const GPUAPI::ResourceInGPU> _resourceInGPU;

        virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController);
        virtual bool uploadToGPU();
        virtual void unloadFromGPU();
        virtual void releaseData();
    public:
        virtual ~MapRendererElevationDataResource();

        const std::shared_ptr<const GPUAPI::ResourceInGPU>& resourceInGPU;

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_ELEVATION_DATA_RESOURCE_H_)
