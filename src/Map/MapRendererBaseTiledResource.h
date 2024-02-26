#ifndef _OSMAND_CORE_MAP_RENDERER_BASE_TILED_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_BASE_TILED_RESOURCE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "IQueryController.h"
#include "MapRendererBaseResource.h"
#include "MapRendererResourceState.h"
#include "MapRendererResourceType.h"
#include "OsmAndCore.h"
#include "TiledEntriesCollection.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    class MapRendererBaseTiledResource
        : public MapRendererBaseResource
        , public TiledEntriesCollectionEntryWithState<MapRendererBaseTiledResource, MapRendererResourceState, MapRendererResourceState::Unknown>
    {
        typedef TiledEntriesCollectionEntryWithState<MapRendererBaseTiledResource, MapRendererResourceState, MapRendererResourceState::Unknown>
            BaseTilesCollectionEntryWithState;

    private:

    protected:
        MapRendererBaseTiledResource(
            MapRendererResourcesManager* owner,
            MapRendererResourceType type,
            const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
            TileId tileId,
            ZoomLevel zoom);

        bool isMetaTiled;

        void detach() override;

        void removeSelfFromCollection() override;

    public:
        ~MapRendererBaseTiledResource() override;

        MapRendererResourceState getState() const override;
        void setState(MapRendererResourceState newState) override;
        bool setStateIf(MapRendererResourceState testState, MapRendererResourceState newState) override;

        friend class OsmAnd::MapRendererResourcesManager;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_BASE_TILED_RESOURCE_H_)
