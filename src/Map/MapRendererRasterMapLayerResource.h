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
#include "MapRasterLayerProvider.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;
    struct Metric;

    class MapRendererRasterMapLayerResource : public MapRendererBaseTiledResource
    {
    private:
    protected:
        MapRendererRasterMapLayerResource(
            MapRendererResourcesManager* owner,
            const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
            const TileId tileId,
            const ZoomLevel zoom);

        std::shared_ptr<IRasterMapLayerProvider::Data> _sourceData;
        std::shared_ptr<const GPUAPI::ResourceInGPU> _resourceInGPU;

        virtual bool supportsObtainDataAsync() const Q_DECL_OVERRIDE;
        virtual bool obtainData(
            bool& dataAvailable,
            const std::shared_ptr<const IQueryController>& queryController) Q_DECL_OVERRIDE;
        virtual void obtainDataAsync(
            const ObtainDataAsyncCallback callback,
            const std::shared_ptr<const IQueryController>& queryController,
            const bool cacheOnly = false) Q_DECL_OVERRIDE;

        virtual bool uploadToGPU() Q_DECL_OVERRIDE;
        virtual void unloadFromGPU() Q_DECL_OVERRIDE;
        virtual void lostDataInGPU() Q_DECL_OVERRIDE;
        virtual void releaseData() Q_DECL_OVERRIDE;
    public:
        virtual ~MapRendererRasterMapLayerResource();

        const std::shared_ptr<const GPUAPI::ResourceInGPU>& resourceInGPU;
        mutable QAtomicInt resourceInGPULock;
        QAtomicInt detailedZoom;

        std::shared_ptr<Metric> metric;

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_RASTER_MAP_LAYER_RESOURCE_H_)
