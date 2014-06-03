#ifndef _OSMAND_CORE_MAP_RENDERER_TILED_SYMBOLS_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_TILED_SYMBOLS_RESOURCE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "MapRendererBaseTiledResource.h"
#include "GPUAPI.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;
    class MapRendererTiledSymbolsResourcesCollection;
    class MapSymbol;
    class MapSymbolsGroup;
    class TiledMapSymbolsData;

    class MapRendererTiledSymbolsResource : public MapRendererBaseTiledResource
    {
    private:
    protected:
        MapRendererTiledSymbolsResource(MapRendererResourcesManager* owner, const TiledEntriesCollection<MapRendererBaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom);

        class GroupResources
        {
            Q_DISABLE_COPY(GroupResources);
        private:
        protected:
        public:
            GroupResources(const std::shared_ptr<MapSymbolsGroup>& group);
            ~GroupResources();

            const std::shared_ptr<MapSymbolsGroup> group;
            QHash< std::shared_ptr<MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > resourcesInGPU;
        };

        std::shared_ptr<TiledMapSymbolsData> _sourceData;
        QList< std::shared_ptr<GroupResources> > _uniqueGroupsResources;
        QList< std::shared_ptr<GroupResources> > _referencedSharedGroupsResources;
        QHash< std::shared_ptr<const MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > _resourcesInGPU;

        virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController);
        virtual bool uploadToGPU();
        virtual void unloadFromGPU();
        virtual void releaseData();
    public:
        virtual ~MapRendererTiledSymbolsResource();

        std::shared_ptr<const GPUAPI::ResourceInGPU> getGpuResourceFor(const std::shared_ptr<const MapSymbol>& mapSymbol);

    friend class OsmAnd::MapRendererResourcesManager;
    friend class OsmAnd::MapRendererTiledSymbolsResourcesCollection;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_TILED_SYMBOLS_RESOURCE_H_)
