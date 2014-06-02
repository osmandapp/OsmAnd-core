#ifndef _OSMAND_CORE_MAP_RENDERER_BASE_KEYED_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_BASE_KEYED_RESOURCE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "KeyedEntriesCollection.h"
#include "MapRendererBaseResource.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    // Base class for all keyed resources
    class MapRendererBaseKeyedResource
        : public MapRendererBaseResource
        , public KeyedEntriesCollectionEntryWithState < const void*, MapRendererBaseKeyedResource, MapRendererResourceState, MapRendererResourceState::Unknown >
    {
    public:
        typedef const void* Key;
        typedef KeyedEntriesCollectionEntryWithState<Key, MapRendererBaseKeyedResource, MapRendererResourceState, MapRendererResourceState::Unknown> BaseKeyedEntriesCollectionEntryWithState;

    private:
    protected:
        MapRendererBaseKeyedResource(
            MapRendererResourcesManager* owner,
            const MapRendererResourceType type,
            const KeyedEntriesCollection<Key, MapRendererBaseKeyedResource>& collection,
            const Key key);

        virtual void detach();

        virtual void removeSelfFromCollection();
    public:
        virtual ~MapRendererBaseKeyedResource();

        virtual MapRendererResourceState getState() const;
        virtual void setState(const MapRendererResourceState newState);
        virtual bool setStateIf(const MapRendererResourceState testState, const MapRendererResourceState newState);

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_BASE_KEYED_RESOURCE_H_)
