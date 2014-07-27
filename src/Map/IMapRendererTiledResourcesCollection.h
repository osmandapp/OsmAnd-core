#ifndef _OSMAND_CORE_I_MAP_RENDERER_TILED_RESOURCES_COLLECTION_H_
#define _OSMAND_CORE_I_MAP_RENDERER_TILED_RESOURCES_COLLECTION_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"

namespace OsmAnd
{
    class MapRendererBaseTiledResource;

    class IMapRendererTiledResourcesCollection
    {
    private:
    protected:
        IMapRendererTiledResourcesCollection();
    public:
        virtual ~IMapRendererTiledResourcesCollection();

        virtual bool obtainResource(
            const TileId tileId,
            const ZoomLevel zoomLevel,
            std::shared_ptr<MapRendererBaseTiledResource>& outResource) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_RENDERER_TILED_RESOURCES_COLLECTION_H_)
