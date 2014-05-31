#include "MapRendererKeyedResourcesCollection.h"

OsmAnd::MapRendererKeyedResourcesCollection::MapRendererKeyedResourcesCollection(const MapRendererResourceType& type_)
    : MapRendererBaseResourcesCollection(type_)
{
}

OsmAnd::MapRendererKeyedResourcesCollection::~MapRendererKeyedResourcesCollection()
{
    verifyNoUploadedResourcesPresent();
}

void OsmAnd::MapRendererKeyedResourcesCollection::verifyNoUploadedResourcesPresent() const
{
    // Ensure that no resources have "Uploaded" state
    bool stillUploadedResourcesPresent = false;
    obtainEntries(nullptr,
        [&stillUploadedResourcesPresent]
        (const std::shared_ptr<MapRendererBaseKeyedResource>& keyedEntry, bool& cancel) -> bool
        {
            const auto state = keyedEntry->getState();
            if (state == MapRendererResourceState::Uploading ||
                state == MapRendererResourceState::Uploaded ||
                state == MapRendererResourceState::IsBeingUsed ||
                state == MapRendererResourceState::Unloading)
            {
                stillUploadedResourcesPresent = true;
                cancel = true;
                return false;
            }

            return false;
        });
    if (stillUploadedResourcesPresent)
        LogPrintf(LogSeverityLevel::Error, "One or more keyed resources still reside in GPU memory. This may cause GPU memory leak");
    assert(stillUploadedResourcesPresent == false);
}

void OsmAnd::MapRendererKeyedResourcesCollection::removeAllEntries()
{
    verifyNoUploadedResourcesPresent();

    KeyedEntriesCollection::removeAllEntries();
}

int OsmAnd::MapRendererKeyedResourcesCollection::getResourcesCount() const
{
    return getEntriesCount();
}

void OsmAnd::MapRendererKeyedResourcesCollection::forEachResourceExecute(const ResourceActionCallback action)
{
    forAllExecute(
        [action]
        (const std::shared_ptr<MapRendererBaseKeyedResource>& entry, bool& cancel)
        {
            action(entry, cancel);
        });
}

void OsmAnd::MapRendererKeyedResourcesCollection::obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter)
{
    obtainEntries(nullptr,
        [outList, filter]
        (const std::shared_ptr<MapRendererBaseKeyedResource>& entry, bool& cancel) -> bool
        {
            if (filter && !filter(entry, cancel))
                return false;

            if (outList != nullptr)
                outList->push_back(entry);
            return true;
        });
}

void OsmAnd::MapRendererKeyedResourcesCollection::removeResources(const ResourceFilterCallback filter)
{
    removeEntries(
        [filter]
        (const std::shared_ptr<MapRendererBaseKeyedResource>& entry, bool& cancel) -> bool
        {
            return filter(entry, cancel);
        });
}
