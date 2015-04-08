#ifndef _OSMAND_CORE_MAP_RENDERER_BASE_KEYED_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_BASE_KEYED_RESOURCE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "KeyedEntriesCollection.h"
#include "MapRendererBaseResource.h"
#include "IMapKeyedDataProvider.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    class MapRendererBaseKeyedResource
        : public MapRendererBaseResource
        , public KeyedEntriesCollectionEntryWithState<
            IMapKeyedDataProvider::Key,
            MapRendererBaseKeyedResource,
            MapRendererResourceState,
            MapRendererResourceState::Unknown>
    {
        Q_DISABLE_COPY_AND_MOVE(MapRendererBaseKeyedResource);

    public:
        typedef IMapKeyedDataProvider::Key Key;
        typedef KeyedEntriesCollectionEntryWithState<
            Key,
            MapRendererBaseKeyedResource,
            MapRendererResourceState,
            MapRendererResourceState::Unknown> BaseKeyedEntriesCollectionEntryWithState;

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
