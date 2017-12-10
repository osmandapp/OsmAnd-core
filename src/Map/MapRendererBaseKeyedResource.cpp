#include "MapRendererBaseKeyedResource.h"

OsmAnd::MapRendererBaseKeyedResource::MapRendererBaseKeyedResource(
    MapRendererResourcesManager* owner_,
    const MapRendererResourceType type_,
    const KeyedEntriesCollection<Key, MapRendererBaseKeyedResource>& collection_,
    const Key key_)
    : MapRendererBaseResource(owner_, type_)
    , KeyedEntriesCollectionEntryWithState(collection_, key_)
{
}

OsmAnd::MapRendererBaseKeyedResource::~MapRendererBaseKeyedResource()
{
    const volatile auto state = getState();
    if (state == MapRendererResourceState::Uploading ||
        state == MapRendererResourceState::Renewing ||
        state == MapRendererResourceState::Uploaded ||
        state == MapRendererResourceState::IsBeingUsed ||
        state == MapRendererResourceState::Unloading)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Keyed resource for %p still resides in GPU memory. This may cause GPU memory leak",
            key);
    }

    safeUnlink();
}

OsmAnd::MapRendererResourceState OsmAnd::MapRendererBaseKeyedResource::getState() const
{
    return BaseKeyedEntriesCollectionEntryWithState::getState();
}

void OsmAnd::MapRendererBaseKeyedResource::setState(const MapRendererResourceState newState)
{
    BaseKeyedEntriesCollectionEntryWithState::setState(newState);
}

bool OsmAnd::MapRendererBaseKeyedResource::setStateIf(
    const MapRendererResourceState testState,
    const MapRendererResourceState newState)
{
    return BaseKeyedEntriesCollectionEntryWithState::setStateIf(testState, newState);
}

void OsmAnd::MapRendererBaseKeyedResource::removeSelfFromCollection()
{
    if (const auto link_ = link.lock())
        link_->collection.removeEntry(key);
}

void OsmAnd::MapRendererBaseKeyedResource::detach()
{
    releaseData();
}
