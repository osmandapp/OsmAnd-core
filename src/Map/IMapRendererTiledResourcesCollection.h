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
    public:
        typedef std::function<void(const std::shared_ptr<MapRendererBaseTiledResource>& entry, bool& cancel)> TiledResourceActionCallback;
        typedef std::function<bool(const std::shared_ptr<MapRendererBaseTiledResource>& entry, bool& cancel)> TiledResourceFilterCallback;
        typedef std::function<bool(const std::shared_ptr<MapRendererBaseTiledResource>& entry)> TiledResourceAcceptorCallback;

    private:
    protected:
        IMapRendererTiledResourcesCollection();
    public:
        virtual ~IMapRendererTiledResourcesCollection();

        virtual bool obtainResource(
            const TileId tileId,
            const ZoomLevel zoomLevel,
            std::shared_ptr<MapRendererBaseTiledResource>& outResource) const = 0;
        virtual bool containsResource(
            const TileId tileId,
            const ZoomLevel zoomLevel,
            const TiledResourceAcceptorCallback filter = nullptr) const = 0;
        virtual bool containsResources(const ZoomLevel zoomLevel) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_RENDERER_TILED_RESOURCES_COLLECTION_H_)
