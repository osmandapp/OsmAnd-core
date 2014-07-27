#include "MapRendererTiledResourcesCollection.h"

OsmAnd::MapRendererTiledResourcesCollection::MapRendererTiledResourcesCollection(const MapRendererResourceType type_)
    : MapRendererBaseResourcesCollection(type_)
    , _shadow(new Shadow(type_))
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
    _shadowCollectionInvalidatesCount.fetchAndAddOrdered(1);
}

bool OsmAnd::MapRendererTiledResourcesCollection::updateShadowCollection() const
{
    const auto invalidatesDiscarded = _shadowCollectionInvalidatesCount.fetchAndAddOrdered(0);
    if (invalidatesDiscarded == 0)
        return true;

    // Copy from original storage to temp storage
    Storage tempShadowStorage;
    {
        if (!_collectionLock.tryLockForRead())
            return false;

        for (int zoomLevel = MinZoomLevel; zoomLevel <= MaxZoomLevel; zoomLevel++)
        {
            const auto& sourceStorage = constOf(_storage)[zoomLevel];
            auto& targetStorage = tempShadowStorage[zoomLevel];

            targetStorage = detachedOf(sourceStorage);
        }

        _collectionLock.unlock();
    }
    _shadowCollectionInvalidatesCount.fetchAndAddOrdered(-invalidatesDiscarded);

    // Copy from temp storage to shadow
    {
        QWriteLocker scopedLocker(&_shadow->_lock);

        _shadow->_storage = qMove(tempShadowStorage);
    }

    return true;
}

std::shared_ptr<const OsmAnd::IMapRendererResourcesCollection> OsmAnd::MapRendererTiledResourcesCollection::getShadowCollection() const
{
    return _shadow;
}

std::shared_ptr<OsmAnd::IMapRendererResourcesCollection> OsmAnd::MapRendererTiledResourcesCollection::getShadowCollection()
{
    return _shadow;
}

OsmAnd::MapRendererTiledResourcesCollection::Shadow::Shadow(const MapRendererResourceType type_)
    : type(type_)
{
}

OsmAnd::MapRendererTiledResourcesCollection::Shadow::~Shadow()
{
}

OsmAnd::MapRendererResourceType OsmAnd::MapRendererTiledResourcesCollection::Shadow::getType() const
{
    return type;
}

int OsmAnd::MapRendererTiledResourcesCollection::Shadow::getResourcesCount() const
{
    QReadLocker scopedLocker(&_lock);

    int count = 0;
    for (const auto& storage : constOf(_storage))
        count += storage.size();

    return count;
}

void OsmAnd::MapRendererTiledResourcesCollection::Shadow::forEachResourceExecute(const ResourceActionCallback action)
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

void OsmAnd::MapRendererTiledResourcesCollection::Shadow::obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter)
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

bool OsmAnd::MapRendererTiledResourcesCollection::Shadow::obtainResource(const TileId tileId, const ZoomLevel zoomLevel, std::shared_ptr<MapRendererBaseTiledResource>& outResource) const
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
