#include "MapRendererBaseKeyedResource.h"

OsmAnd::MapRendererBaseKeyedResource::MapRendererBaseKeyedResource(
    MapRendererResourcesManager* owner,
    const MapRendererResourceType type,
    const KeyedEntriesCollection<Key, MapRendererBaseKeyedResource>& collection,
    const Key key)
    : MapRendererBaseResource(owner, type)
    , KeyedEntriesCollectionEntryWithState(collection, key)
{
}

OsmAnd::MapRendererBaseKeyedResource::~MapRendererBaseKeyedResource()
{
    const volatile auto state = getState();
    if (state == MapRendererResourceState::Uploading ||
        state == MapRendererResourceState::Uploaded ||
        state == MapRendererResourceState::IsBeingUsed ||
        state == MapRendererResourceState::Unloading)
    {
        LogPrintf(LogSeverityLevel::Error, "Keyed resource for %p still resides in GPU memory. This may cause GPU memory leak", key);
    }

    safeUnlink();
}

OsmAnd::MapRendererResourceState OsmAnd::MapRendererBaseKeyedResource::getState() const
{
    return BaseKeyedEntriesCollectionEntryWithState::getState();
}

void OsmAnd::MapRendererBaseKeyedResource::setState(const MapRendererResourceState newState)
{
    return BaseKeyedEntriesCollectionEntryWithState::setState(newState);
}

bool OsmAnd::MapRendererBaseKeyedResource::setStateIf(const MapRendererResourceState testState, const MapRendererResourceState newState)
{
    return BaseKeyedEntriesCollectionEntryWithState::setStateIf(testState, newState);
}

void OsmAnd::MapRendererBaseKeyedResource::removeSelfFromCollection()
{
    if (const auto link_ = link.lock())
        link_->collection.removeEntry(key);
}
