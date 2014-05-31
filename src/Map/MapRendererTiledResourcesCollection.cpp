#include "MapRendererTiledResourcesCollection.h"

OsmAnd::MapRendererTiledResourcesCollection::MapRendererTiledResourcesCollection(const MapRendererResourceType& type_)
    : MapRendererBaseResourcesCollection(type_)
{
}

OsmAnd::MapRendererTiledResourcesCollection::~MapRendererTiledResourcesCollection()
{
    verifyNoUploadedResourcesPresent();
}

void OsmAnd::MapRendererTiledResourcesCollection::removeAllEntries()
{
    verifyNoUploadedResourcesPresent();

    TiledEntriesCollection::removeAllEntries();
}

void OsmAnd::MapRendererTiledResourcesCollection::verifyNoUploadedResourcesPresent() const
{
    // Ensure that no tiles have "Uploaded" state
    bool stillUploadedTilesPresent = false;
    obtainEntries(nullptr,
        [&stillUploadedTilesPresent]
        (const std::shared_ptr<MapRendererBaseTiledResource>& tileEntry, bool& cancel) -> bool
        {
            const auto state = tileEntry->getState();
            if (state == MapRendererResourceState::Uploading ||
                state == MapRendererResourceState::Uploaded ||
                state == MapRendererResourceState::IsBeingUsed ||
                state == MapRendererResourceState::Unloading)
            {
                stillUploadedTilesPresent = true;
                cancel = true;
                return false;
            }

            return false;
        });
    if (stillUploadedTilesPresent)
        LogPrintf(LogSeverityLevel::Error, "One or more tiled resources still reside in GPU memory. This may cause GPU memory leak");
    assert(stillUploadedTilesPresent == false);
}

int OsmAnd::MapRendererTiledResourcesCollection::getResourcesCount() const
{
    return getEntriesCount();
}

void OsmAnd::MapRendererTiledResourcesCollection::forEachResourceExecute(const ResourceActionCallback action)
{
    forAllExecute(
        [action]
        (const std::shared_ptr<MapRendererBaseTiledResource>& entry, bool& cancel)
        {
            action(entry, cancel);
        });
}

void OsmAnd::MapRendererTiledResourcesCollection::obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter)
{
    obtainEntries(nullptr,
        [outList, filter]
        (const std::shared_ptr<MapRendererBaseTiledResource>& entry, bool& cancel) -> bool
        {
            if (filter && !filter(entry, cancel))
                return false;

            if (outList != nullptr)
                outList->push_back(entry);
            return true;
        });
}

void OsmAnd::MapRendererTiledResourcesCollection::removeResources(const ResourceFilterCallback filter)
{
    removeEntries(
        [filter]
        (const std::shared_ptr<MapRendererBaseTiledResource>& entry, bool& cancel) -> bool
        {
            return filter(entry, cancel);
        });
}
