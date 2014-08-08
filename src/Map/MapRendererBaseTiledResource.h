#ifndef _OSMAND_CORE_MAP_RENDERER_BASE_TILED_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_BASE_TILED_RESOURCE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "MapRendererBaseResource.h"
#include "TiledEntriesCollection.h"
#include "Concurrent.h"
#include "IQueryController.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    class MapRendererBaseTiledResource
        : public MapRendererBaseResource
        , public TiledEntriesCollectionEntryWithState < MapRendererBaseTiledResource, MapRendererResourceState, MapRendererResourceState::Unknown >
    {
        typedef TiledEntriesCollectionEntryWithState<MapRendererBaseTiledResource, MapRendererResourceState, MapRendererResourceState::Unknown> BaseTilesCollectionEntryWithState;

    private:
    protected:
        MapRendererBaseTiledResource(
            MapRendererResourcesManager* owner,
            const MapRendererResourceType type,
            const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
            const TileId tileId,
            const ZoomLevel zoom);

        virtual void detach();

        virtual void removeSelfFromCollection();
    public:
        virtual ~MapRendererBaseTiledResource();

        virtual MapRendererResourceState getState() const;
        virtual void setState(const MapRendererResourceState newState);
        virtual bool setStateIf(const MapRendererResourceState testState, const MapRendererResourceState newState);

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_BASE_TILED_RESOURCE_H_)
