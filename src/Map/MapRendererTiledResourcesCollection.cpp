#include "MapRendererTiledResourcesCollection.h"

OsmAnd::MapRendererTiledResourcesCollection::MapRendererTiledResourcesCollection(const MapRendererResourceType type_)
    : MapRendererBaseResourcesCollection(type_)
    , _snapshot(new Snapshot(type_))
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

bool OsmAnd::MapRendererTiledResourcesCollection::obtainResource(
    const TileId tileId,
    const ZoomLevel zoomLevel,
    std::shared_ptr<MapRendererBaseTiledResource>& outResource) const
{
    return obtainEntry(outResource, tileId, zoomLevel);
}

bool OsmAnd::MapRendererTiledResourcesCollection::containsResource(
    const TileId tileId,
    const ZoomLevel zoomLevel,
    const TiledResourceAcceptorCallback filter /*= nullptr*/) const
{
    std::shared_ptr<MapRendererBaseTiledResource> resource;
    if (!obtainEntry(resource, tileId, zoomLevel))
        return false;

    return filter
        ? filter(resource)
        : true;
}

bool OsmAnd::MapRendererTiledResourcesCollection::containsResources(const ZoomLevel zoomLevel) const
{
    return hasEntries(zoomLevel);
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

void OsmAnd::MapRendererTiledResourcesCollection::onCollectionModified() const
{
    _collectionSnapshotInvalidatesCount.fetchAndAddOrdered(1);
}

bool OsmAnd::MapRendererTiledResourcesCollection::updateCollectionSnapshot() const
{
    const auto invalidatesDiscarded = _collectionSnapshotInvalidatesCount.fetchAndAddOrdered(0);
    if (invalidatesDiscarded == 0)
        return true;

    // Copy from original storage to temp storage
    Storage storageCopy;
    {
        if (!_collectionLock.tryLockForRead())
            return false;

        for (int zoomLevel = MinZoomLevel; zoomLevel <= MaxZoomLevel; zoomLevel++)
        {
            const auto& sourceStorageLevel = constOf(_storage)[zoomLevel];
            auto& targetStorageLevel = storageCopy[zoomLevel];

            targetStorageLevel = detachedOf(sourceStorageLevel);
        }

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

bool OsmAnd::MapRendererTiledResourcesCollection::collectionSnapshotInvalidated() const
{
    return (_collectionSnapshotInvalidatesCount.loadAcquire() > 0);
}

std::shared_ptr<const OsmAnd::IMapRendererResourcesCollection> OsmAnd::MapRendererTiledResourcesCollection::getCollectionSnapshot() const
{
    return _snapshot;
}

std::shared_ptr<OsmAnd::IMapRendererResourcesCollection> OsmAnd::MapRendererTiledResourcesCollection::getCollectionSnapshot()
{
    return _snapshot;
}

OsmAnd::MapRendererTiledResourcesCollection::Snapshot::Snapshot(const MapRendererResourceType type_)
    : type(type_)
{
}

OsmAnd::MapRendererTiledResourcesCollection::Snapshot::~Snapshot()
{
}

OsmAnd::MapRendererResourceType OsmAnd::MapRendererTiledResourcesCollection::Snapshot::getType() const
{
    return type;
}

int OsmAnd::MapRendererTiledResourcesCollection::Snapshot::getResourcesCount() const
{
    QReadLocker scopedLocker(&_lock);

    int count = 0;
    for (const auto& storage : constOf(_storage))
        count += storage.size();

    return count;
}

void OsmAnd::MapRendererTiledResourcesCollection::Snapshot::forEachResourceExecute(const ResourceActionCallback action)
{
    QReadLocker scopedLocker(&_lock);

    bool doCancel = false;
    for (const auto& storage : constOf(_storage))
    {
        for (const auto& entry : constOf(storage))
        {
            action(entry, doCancel);

            if (doCancel)
                return;
        }
    }
}

void OsmAnd::MapRendererTiledResourcesCollection::Snapshot::obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter)
{
    QReadLocker scopedLocker(&_lock);

    bool doCancel = false;
    for (const auto& storage : constOf(_storage))
    {
        for (const auto& entry : constOf(storage))
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
}

bool OsmAnd::MapRendererTiledResourcesCollection::Snapshot::obtainResource(
    const TileId tileId,
    const ZoomLevel zoomLevel,
    std::shared_ptr<MapRendererBaseTiledResource>& outResource) const
{
    QReadLocker scopedLocker(&_lock);

    const auto& storage = _storage[zoomLevel];
    const auto& itEntry = storage.constFind(tileId);
    if (itEntry != storage.cend())
    {
        outResource = *itEntry;

        return true;
    }

    return false;
}

bool OsmAnd::MapRendererTiledResourcesCollection::Snapshot::containsResource(
    const TileId tileId,
    const ZoomLevel zoomLevel,
    const TiledResourceAcceptorCallback filter /*= nullptr*/) const
{
    QReadLocker scopedLocker(&_lock);

    const auto& storage = _storage[zoomLevel];
    const auto& itEntry = storage.constFind(tileId);
    if (itEntry != storage.cend())
    {
        if (!filter)
            return true;
        
        return filter(*itEntry);
    }

    return false;
}

bool OsmAnd::MapRendererTiledResourcesCollection::Snapshot::containsResources(const ZoomLevel zoomLevel) const
{
    QReadLocker scopedLocker(&_lock);

    const auto& storage = _storage[zoomLevel];

    return !storage.isEmpty();
}
