#include "MapRendererKeyedResourcesCollection.h"

OsmAnd::MapRendererKeyedResourcesCollection::MapRendererKeyedResourcesCollection(const MapRendererResourceType type_)
    : MapRendererBaseResourcesCollection(type_)
    , _snapshot(new Snapshot(type_))
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
                state == MapRendererResourceState::Unloading ||
                state == MapRendererResourceState::UnloadingForRenew)
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

void OsmAnd::MapRendererKeyedResourcesCollection::onCollectionModified() const
{
    _collectionSnapshotInvalidatesCount.fetchAndAddOrdered(1);
}

bool OsmAnd::MapRendererKeyedResourcesCollection::obtainResource(const Key key, std::shared_ptr<MapRendererBaseKeyedResource>& outResource) const
{
    return obtainEntry(outResource, key);
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

QList<OsmAnd::MapRendererKeyedResourcesCollection::Key> OsmAnd::MapRendererKeyedResourcesCollection::getKeys() const
{
    return KeyedEntriesCollection::getKeys();
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

bool OsmAnd::MapRendererKeyedResourcesCollection::updateCollectionSnapshot() const
{
    const auto invalidatesDiscarded = _collectionSnapshotInvalidatesCount.fetchAndAddOrdered(0);
    if (invalidatesDiscarded == 0)
        return true;

    // Copy from original storage to temp storage
    Storage storageCopy;
    {
        if (!_collectionLock.tryLockForRead())
            return false;

        storageCopy = detachedOf(_storage);
        
        _collectionLock.unlock();
    }
    _collectionSnapshotInvalidatesCount.fetchAndAddOrdered(-invalidatesDiscarded);

    // Copy from temp storage to snapshot
    {
        QWriteLocker scopedLocker(&_snapshot->_lock);

        _snapshot->_storage = qMove(storageCopy);
    }

    return true;
}

std::shared_ptr<const OsmAnd::IMapRendererResourcesCollection> OsmAnd::MapRendererKeyedResourcesCollection::getCollectionSnapshot() const
{
    return _snapshot;
}

std::shared_ptr<OsmAnd::IMapRendererResourcesCollection> OsmAnd::MapRendererKeyedResourcesCollection::getCollectionSnapshot()
{
    return _snapshot;
}

bool OsmAnd::MapRendererKeyedResourcesCollection::collectionSnapshotInvalidated() const
{
    return (_collectionSnapshotInvalidatesCount.loadAcquire() > 0);
}

OsmAnd::MapRendererKeyedResourcesCollection::Snapshot::Snapshot(const MapRendererResourceType type_)
    : type(type_)
{
}

OsmAnd::MapRendererKeyedResourcesCollection::Snapshot::~Snapshot()
{
}

OsmAnd::MapRendererResourceType OsmAnd::MapRendererKeyedResourcesCollection::Snapshot::getType() const
{
    return type;
}

int OsmAnd::MapRendererKeyedResourcesCollection::Snapshot::getResourcesCount() const
{
    QReadLocker scopedLocker(&_lock);

    return _storage.size();
}

void OsmAnd::MapRendererKeyedResourcesCollection::Snapshot::forEachResourceExecute(const ResourceActionCallback action)
{
    QReadLocker scopedLocker(&_lock);

    bool doCancel = false;
    for (const auto& entry : constOf(_storage))
    {
        action(entry, doCancel);

        if (doCancel)
            return;
    }
}

void OsmAnd::MapRendererKeyedResourcesCollection::Snapshot::obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter)
{
    QReadLocker scopedLocker(&_lock);

    bool doCancel = false;
    for (const auto& entry : constOf(_storage))
    {
        if (!filter || (filter && filter(entry, doCancel)))
        {
            if (outList)
                outList->push_back(entry);
        }

        if (doCancel)
            return;
    }
}

QList<OsmAnd::MapRendererKeyedResourcesCollection::Key> OsmAnd::MapRendererKeyedResourcesCollection::Snapshot::getKeys() const
{
    QReadLocker scopedLocker(&_lock);

    return _storage.keys();
}

bool OsmAnd::MapRendererKeyedResourcesCollection::Snapshot::obtainResource(const Key key, std::shared_ptr<MapRendererBaseKeyedResource>& outResource) const
{
    QReadLocker scopedLocker(&_lock);

    const auto& itEntry = _storage.constFind(key);
    if (itEntry != _storage.cend())
    {
        outResource = *itEntry;

        return true;
    }

    return false;
}
