#ifndef _OSMAND_CORE_MAP_RENDERER_TILED_SYMBOLS_RESOURCES_COLLECTION_H_
#define _OSMAND_CORE_MAP_RENDERER_TILED_SYMBOLS_RESOURCES_COLLECTION_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "SharedResourcesContainer.h"
#include "MapRendererTiledResourcesCollection.h"
#include "MapRendererTiledSymbolsResource.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    class MapRendererTiledSymbolsResourcesCollection : public MapRendererTiledResourcesCollection
    {
    private:
    protected:
        MapRendererTiledSymbolsResourcesCollection();

        std::array< SharedResourcesContainer<uint64_t, MapRendererTiledSymbolsResource::GroupResources>, ZoomLevelsCount > _sharedGroupsResources;
    public:
        virtual ~MapRendererTiledSymbolsResourcesCollection();

    friend class OsmAnd::MapRendererResourcesManager;
    friend class OsmAnd::MapRendererTiledSymbolsResource;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_TILED_SYMBOLS_RESOURCES_COLLECTION_H_)
