#include "MapRendererBaseTiledResource.h"

OsmAnd::MapRendererBaseTiledResource::MapRendererBaseTiledResource(
    MapRendererResourcesManager* owner,
    const MapRendererResourceType type,
    const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
    const TileId tileId,
    const ZoomLevel zoom)
    : MapRendererBaseResource(owner, type)
    , TiledEntriesCollectionEntryWithState(collection, tileId, zoom)
{
}

OsmAnd::MapRendererBaseTiledResource::~MapRendererBaseTiledResource()
{
    const volatile auto state = getState();
    if (state == MapRendererResourceState::Uploading ||
        state == MapRendererResourceState::Uploaded ||
        state == MapRendererResourceState::IsBeingUsed ||
        state == MapRendererResourceState::Unloading)
    {
        LogPrintf(LogSeverityLevel::Error, "Tiled resource for %dx%d@%d still resides in GPU memory. This may cause GPU memory leak", tileId.x, tileId.y, zoom);
    }

    safeUnlink();
}

OsmAnd::MapRendererResourceState OsmAnd::MapRendererBaseTiledResource::getState() const
{
    return BaseTilesCollectionEntryWithState::getState();
}

void OsmAnd::MapRendererBaseTiledResource::setState(const MapRendererResourceState newState)
{
    return BaseTilesCollectionEntryWithState::setState(newState);
}

bool OsmAnd::MapRendererBaseTiledResource::setStateIf(const MapRendererResourceState testState, const MapRendererResourceState newState)
{
    return BaseTilesCollectionEntryWithState::setStateIf(testState, newState);
}

void OsmAnd::MapRendererBaseTiledResource::removeSelfFromCollection()
{
    if (const auto link_ = link.lock())
        link_->collection.removeEntry(tileId, zoom);
}

void OsmAnd::MapRendererBaseTiledResource::detach()
{
    releaseData();
}
