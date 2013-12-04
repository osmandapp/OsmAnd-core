#include "MapRendererResources.h"

#include "MapRenderer.h"
#include "IMapProvider.h"
#include "IMapBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "IMapSymbolProvider.h"
#include "IRetainedMapTile.h"
#include "EmbeddedResources.h"
#include "Logging.h"

#include <SkBitmap.h>
#include <SkImageDecoder.h>

OsmAnd::MapRendererResources::MapRendererResources(MapRenderer* const owner_)
    : _taskHostBridge(this)
    , _invalidatedResourcesTypesMask(0)
    , _workerThreadIsAlive(false)
    , _workerThreadId(nullptr)
    , _workerThread(new Concurrent::Thread(std::bind(&MapRendererResources::workerThreadProcedure, this)))
    , renderer(owner_)
    , processingTileStub(_processingTileStub)
    , unavailableTileStub(_unavailableTileStub)
{
#if defined(DEBUG) || defined(_DEBUG)
    LogPrintf(LogSeverityLevel::Info, "Map renderer will use max %d worker thread(s) to process requests", _resourcesRequestWorkersPool.maxThreadCount());
#endif
    // Raster resources collections are special, so preallocate them
    {
        auto& resources = _storage[static_cast<int>(ResourceType::RasterMap)];
        resources.reserve(RasterMapLayersCount);
        while(resources.size() < RasterMapLayersCount)
            resources.push_back(qMove(std::shared_ptr<TiledResourcesCollection>()));
    }

    // Initialize default resources
    initializeDefaultResources();

    // Start worker thread
    _workerThreadIsAlive = true;
    _workerThread->start();
}

OsmAnd::MapRendererResources::~MapRendererResources()
{
    // Stop worker thread
    _workerThreadIsAlive = false;
    {
        QMutexLocker scopedLocker(&_workerThreadWakeupMutex);
        _workerThreadWakeup.wakeAll();
    }
    _workerThread->wait();

    // Release all resources
    for(auto itResourcesCollections = _storage.begin(); itResourcesCollections != _storage.end(); ++itResourcesCollections)
    {
        auto& resourcesCollections = *itResourcesCollections;

        for(auto itResourcesCollection = resourcesCollections.cbegin(); itResourcesCollection != resourcesCollections.cend(); ++itResourcesCollection)
            releaseResourcesFrom(*itResourcesCollection);
        resourcesCollections.clear();
    }

    // Release all bindings
    for(auto resourceType = 0; resourceType < ResourceTypesCount; resourceType++)
    {
        _bindings[resourceType].providersToCollections.clear();
        _bindings[resourceType].collectionsToProviders.clear();
    }

    // Release default resources
    releaseDefaultResources();

    // Wait for all tasks to complete
    _taskHostBridge.onOwnerIsBeingDestructed();
}

bool OsmAnd::MapRendererResources::initializeDefaultResources()
{
    // Upload stubs
    {
        const auto& data = EmbeddedResources::decompressResource(QLatin1String("map/stubs/processing_tile.png"));
        auto bitmap = new SkBitmap();
        if(!SkImageDecoder::DecodeMemory(data.data(), data.size(), bitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
        {
            delete bitmap;
            return false;
        }
        else
        {
            auto bitmapTile = new MapBitmapTile(bitmap, MapBitmapTile::AlphaChannelData::Undefined);
            renderer->renderAPI->uploadTileToGPU(std::shared_ptr< const MapTile >(bitmapTile), 1, _processingTileStub);
        }
    }
    {
        const auto& data = EmbeddedResources::decompressResource(QLatin1String("map/stubs/unavailable_tile.png"));
        auto bitmap = new SkBitmap();
        if(!SkImageDecoder::DecodeMemory(data.data(), data.size(), bitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
        {
            delete bitmap;
            return false;
        }
        else
        {
            auto bitmapTile = new MapBitmapTile(bitmap, MapBitmapTile::AlphaChannelData::Undefined);
            renderer->renderAPI->uploadTileToGPU(std::shared_ptr< const MapTile >(bitmapTile), 1, _unavailableTileStub);
        }
    }

    return true;
}

bool OsmAnd::MapRendererResources::releaseDefaultResources()
{
    // Release stubs
    if(_unavailableTileStub)
    {
        assert(_unavailableTileStub.use_count() == 1);
        _unavailableTileStub.reset();
    }
    if(_processingTileStub)
    {
        assert(_processingTileStub.use_count() == 1);
        _processingTileStub.reset();
    }

    return true;
}

void OsmAnd::MapRendererResources::updateBindings(const MapRendererState& state, const uint32_t updatedMask)
{
    if(updatedMask & (1u << static_cast<int>(MapRendererStateChange::ElevationData_Provider)))
    {
        auto& bindings = _bindings[static_cast<int>(ResourceType::ElevationData)];
        auto& resources = _storage[static_cast<int>(ResourceType::ElevationData)];

        // Clean-up and unbind gone providers and their resources
        QMutableHashIterator< std::shared_ptr<IMapProvider>, std::shared_ptr<TiledResourcesCollection> > itBindedProvider(bindings.providersToCollections);
        while(itBindedProvider.hasNext())
        {
            itBindedProvider.next();

            // Skip binding if it's still active
            if(itBindedProvider.key() == state.elevationDataProvider)
                continue;

            // Clean-up resources
            releaseResourcesFrom(itBindedProvider.value());

            // Remove binding
            bindings.collectionsToProviders.remove(itBindedProvider.value());
            itBindedProvider.remove();

            // Remove resources collection
            resources.removeOne(itBindedProvider.value());
        }

        // Create new binding and storage
        if(state.elevationDataProvider && !bindings.providersToCollections.contains(state.elevationDataProvider))
        {
            // Create new resources collection
            const std::shared_ptr< TiledResourcesCollection > newResourcesCollection(new TiledResourcesCollection(ResourceType::ElevationData));

            // Add binding
            bindings.providersToCollections.insert(state.elevationDataProvider, newResourcesCollection);
            bindings.collectionsToProviders.insert(newResourcesCollection, state.elevationDataProvider);

            // Add resources collection
            resources.push_back(qMove(newResourcesCollection));
        }
    }
    if(updatedMask & (1u << static_cast<int>(MapRendererStateChange::RasterLayers_Providers)))
    {
        auto& bindings = _bindings[static_cast<int>(ResourceType::RasterMap)];
        auto& resources = _storage[static_cast<int>(ResourceType::RasterMap)];

        // Clean-up and unbind gone providers and their resources
        QMutableHashIterator< std::shared_ptr<IMapProvider>, std::shared_ptr<TiledResourcesCollection> > itBindedProvider(bindings.providersToCollections);
        while(itBindedProvider.hasNext())
        {
            itBindedProvider.next();

            // Skip binding if it's still active
            if(std::find(state.rasterLayerProviders.cbegin(), state.rasterLayerProviders.cend(),
                std::static_pointer_cast<IMapBitmapTileProvider>(itBindedProvider.key())) != state.rasterLayerProviders.cend())
            {
                continue;
            }

            // Clean-up resources
            releaseResourcesFrom(itBindedProvider.value());

            // Reset reference to resources collection, but keep the space in array
            qFind(resources.begin(), resources.end(), itBindedProvider.value())->reset();

            // Remove binding
            bindings.collectionsToProviders.remove(itBindedProvider.value());
            itBindedProvider.remove();
        }

        // Create new binding and storage
        auto rasterLayerIdx = 0;
        for(auto itProvider = state.rasterLayerProviders.cbegin(); itProvider != state.rasterLayerProviders.cend(); ++itProvider, rasterLayerIdx++)
        {
            // Skip empty providers
            if(!*itProvider)
                continue;

            // If binding already exists, skip creation
            if(bindings.providersToCollections.contains(std::static_pointer_cast<IMapProvider>(*itProvider)))
                continue;

            // Create new resources collection
            const std::shared_ptr< TiledResourcesCollection > newResourcesCollection(new TiledResourcesCollection(ResourceType::RasterMap));

            // Add binding
            bindings.providersToCollections.insert(*itProvider, newResourcesCollection);
            bindings.collectionsToProviders.insert(newResourcesCollection, *itProvider);

            // Add resources collection
            assert(rasterLayerIdx < resources.size());
            assert(static_cast<bool>(resources[rasterLayerIdx]) == false);
            resources[rasterLayerIdx] = newResourcesCollection;
        }
    }
    if(updatedMask & (1u << static_cast<int>(MapRendererStateChange::Symbols_Providers)))
    {
        auto& bindings = _bindings[static_cast<int>(ResourceType::Symbols)];
        auto& resources = _storage[static_cast<int>(ResourceType::Symbols)];

        // Clean-up and unbind gone providers and their resources
        QMutableHashIterator< std::shared_ptr<IMapProvider>, std::shared_ptr<TiledResourcesCollection> > itBindedProvider(bindings.providersToCollections);
        while(itBindedProvider.hasNext())
        {
            itBindedProvider.next();

            // Skip binding if it's still active
            if(state.symbolProviders.contains(std::static_pointer_cast<IMapSymbolProvider>(itBindedProvider.key())))
                continue;

            // Clean-up resources
            releaseResourcesFrom(itBindedProvider.value());

            // Remove resources collection
            resources.removeOne(itBindedProvider.value());

            // Remove binding
            bindings.collectionsToProviders.remove(itBindedProvider.value());
            itBindedProvider.remove();
        }

        // Create new binding and storage
        for(auto itProvider = state.symbolProviders.cbegin(); itProvider != state.symbolProviders.cend(); ++itProvider)
        {
            // If binding already exists, skip creation
            if(bindings.providersToCollections.contains(std::static_pointer_cast<IMapProvider>(*itProvider)))
                continue;

            // Create new resources collection
            const std::shared_ptr< TiledResourcesCollection > newResourcesCollection(new TiledResourcesCollection(ResourceType::Symbols));

            // Add binding
            bindings.providersToCollections.insert(*itProvider, newResourcesCollection);
            bindings.collectionsToProviders.insert(newResourcesCollection, *itProvider);

            // Add resources collection
            resources.push_back(qMove(newResourcesCollection));
        }
    }
}

void OsmAnd::MapRendererResources::updateActiveZone(const QSet<TileId>& tiles, const ZoomLevel zoom)
{
    // Lock worker wakeup mutex
    QMutexLocker scopedLocker(&_workerThreadWakeupMutex);

    // Update active zone
    _activeTiles = tiles;
    _activeZoom = zoom;

    // Wake up the worker
    _workerThreadWakeup.wakeAll();
}

bool OsmAnd::MapRendererResources::obtainProviderFor(TiledResourcesCollection* const resourcesRef, std::shared_ptr<IMapProvider>& provider) const
{
    assert(resourcesRef != nullptr);

    const auto& bindings = _bindings[static_cast<int>(resourcesRef->type)];
    for(auto itBinding = bindings.collectionsToProviders.cbegin(); itBinding != bindings.collectionsToProviders.cend(); ++itBinding)
    {
        if(itBinding.key().get() != resourcesRef)
            continue;

        if(!static_cast<bool>(itBinding.value()))
            return false;

        provider = itBinding.value();
        return true;
    }

    return false;
}

bool OsmAnd::MapRendererResources::isDataSourceAvailableFor(const std::shared_ptr<TiledResourcesCollection>& collection) const
{
    const auto& binding = _bindings[static_cast<int>(collection->type)];

    return binding.collectionsToProviders.contains(collection);
}

void OsmAnd::MapRendererResources::notifyNewResourceAvailable()
{
    renderer->invalidateFrame();
}

void OsmAnd::MapRendererResources::workerThreadProcedure()
{
    // Capture worker thread ID
    _workerThreadId = QThread::currentThreadId();

    while(_workerThreadIsAlive)
    {
        // Local copy of active zone
        QSet<TileId> activeTiles;
        ZoomLevel activeZoom;

        // Wait until we're unblocked by host
        {
            _workerThreadWakeupMutex.lock();
            _workerThreadWakeup.wait(&_workerThreadWakeupMutex);

            // Copy active zone to local copy
            activeTiles = _activeTiles;
            activeZoom = _activeZoom;

            _workerThreadWakeupMutex.unlock();
        }
        if(!_workerThreadIsAlive)
            break;

        // Update resources
        updateResources(activeTiles, activeZoom);
    }

    _workerThreadId = nullptr;
}

void OsmAnd::MapRendererResources::requestNeededResources(const QSet<TileId>& tiles, const ZoomLevel zoom)
{
    // Request missing resources
    for(auto itTileId = tiles.cbegin(); itTileId != tiles.cend(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        for(auto itResourcesCollections = _storage.cbegin(); itResourcesCollections != _storage.cend(); ++itResourcesCollections)
        {
            const auto& resourcesCollections = *itResourcesCollections;

            for(auto itResourcesCollection = resourcesCollections.cbegin(); itResourcesCollection != resourcesCollections.cend(); ++itResourcesCollection)
            {
                const auto& resourcesCollection = *itResourcesCollection;

                // Skip empty entries
                if(!static_cast<bool>(resourcesCollection))
                    continue;

                // Skip resource types that do not have an available data source
                if(!isDataSourceAvailableFor(resourcesCollection))
                    continue;

                // Obtain a resource entry and if it's state is "Unknown", create a task that will
                // request resource data
                std::shared_ptr<BaseTiledResource> resource;
                const auto resourceType = resourcesCollection->type;
                resourcesCollection->obtainOrAllocateEntry(resource, tileId, zoom,
                    [this, resourceType](const TilesCollection<BaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom) -> BaseTiledResource*
                {
                    if(resourceType == ResourceType::ElevationData || resourceType == ResourceType::RasterMap)
                        return new MapTileResource(this, resourceType, collection, tileId, zoom);
                    else if(resourceType == ResourceType::Symbols)
                        return new SymbolsTileResource(this, collection, tileId, zoom);
                    else
                        return nullptr;
                });

                // Only if tile entry has "Unknown" state proceed to "Requesting" state
                if(!resource->setStateIf(ResourceState::Unknown, ResourceState::Requesting))
                    continue;

                // Create async-task that will obtain needed resource data
                const auto executeProc = [this](Concurrent::Task* task_, QEventLoop& eventLoop)
                {
                    const auto task = static_cast<ResourceRequestTask*>(task_);
                    const auto resource = std::static_pointer_cast<BaseTiledResource>(task->requestedResource);

                    // Only if resource entry has "Requested" state proceed to "ProcessingRequest" state
                    if(!resource->setStateIf(ResourceState::Requested, ResourceState::ProcessingRequest))
                    {
                        // This actually can happen in following situation(s):
                        //   - if request task was canceled before has started it's execution,
                        //     since in that case a state change "Requested => JustBeforeDeath" must have happend.
                        //     In this case entry will be removed in post-execute handler.
                        assert(resource->getState() == ResourceState::JustBeforeDeath);
                        return;
                    }

                    // Ask resource to obtain it's data
                    bool dataAvailable = false;
                    const auto requestSucceeded = resource->obtainData(dataAvailable);

                    // If failed to obtain resource data, remove resource entry to repeat try later
                    if(!requestSucceeded)
                    {
                        // It's safe to simply remove entry, since it's not yet uploaded
                        if(const auto link = resource->link.lock())
                            link->collection.removeEntry(resource->tileId, resource->zoom);
                        return;
                    }

                    // Finalize execution of task
                    if(!resource->setStateIf(ResourceState::ProcessingRequest, dataAvailable ? ResourceState::Ready : ResourceState::Unavailable))
                    {
                        assert(resource->getState() == ResourceState::JustBeforeDeath);

                        // While request was processed, state may have changed to "JustBeforeDeath"
                        // to indicate that this request was sort of "canceled"
                        task->requestCancellation();
                        return;
                    }
                    resource->_requestTask = nullptr;

                    // There is data to upload to GPU, request uploading. Or just ask to show that resource is unavailable
                    if(dataAvailable)
                        requestResourcesUpload();
                    else
                        notifyNewResourceAvailable();
                };
                const auto postExecuteProc = [this](Concurrent::Task* task_, bool wasCancelled)
                {
                    const auto task = static_cast<const ResourceRequestTask*>(task_);
                    const auto resource = std::static_pointer_cast<BaseTiledResource>(task->requestedResource);

                    if(wasCancelled)
                    {
                        // If request task was canceled, if could have happened:
                        //  - before it has started it's execution.
                        //    In this case, state has to be "Requested", and it won't change. So just remove it
                        //  - during it's execution.
                        //    In this case, this handler will be called after execution was finished,
                        //    and state _should_ be "Ready" or "Unavailable", but in general it can be any:
                        //    Uploading, Uploaded, Unloading, Unloaded. In case state is "Ready" or "Unavailable",
                        //    change it to "JustBeforeDeath" and delete it.

                        if(
                            resource->setStateIf(ResourceState::Requested, ResourceState::JustBeforeDeath) ||
                            resource->setStateIf(ResourceState::Ready, ResourceState::JustBeforeDeath) ||
                            resource->setStateIf(ResourceState::Unavailable, ResourceState::JustBeforeDeath))
                        {
                            if(const auto link = resource->link.lock())
                                link->collection.removeEntry(resource->tileId, resource->zoom);
                        }

                        // All other cases must be handled in other places, since here there is no access to GPU
                    }
                };
                const auto asyncTask = new ResourceRequestTask(resource, _taskHostBridge, executeProc, nullptr, postExecuteProc);

                // Register tile as requested
                resource->_requestTask = asyncTask;
                assert(resource->getState() == ResourceState::Requesting);
                resource->setState(ResourceState::Requested);

                // Finally start the request
                _resourcesRequestWorkersPool.start(asyncTask);
            }
        }
    }
}

void OsmAnd::MapRendererResources::invalidateResourcesOfType(const ResourceType type)
{
    _invalidatedResourcesTypesMask |= 1u << static_cast<int>(type);
}

void OsmAnd::MapRendererResources::validateResources()
{
    uint32_t typeIndex = 0;
    while(_invalidatedResourcesTypesMask)
    {
        if(_invalidatedResourcesTypesMask & 0x1)
            validateResourcesOfType(static_cast<ResourceType>(typeIndex));

        typeIndex++;
        _invalidatedResourcesTypesMask >>= 1;
    }
}

void OsmAnd::MapRendererResources::validateResourcesOfType(const ResourceType type)
{
    const auto& resourcesCollections = _storage[static_cast<int>(type)];
    const auto& bindings = _bindings[static_cast<int>(type)];

    // Notify owner
    renderer->onValidateResourcesOfType(type);

    for(auto itResourcesCollection = resourcesCollections.cbegin(); itResourcesCollection != resourcesCollections.cend(); ++itResourcesCollection)
    {
        if(!bindings.collectionsToProviders.contains(*itResourcesCollection))
            continue;

        releaseResourcesFrom(*itResourcesCollection);
    }
}

void OsmAnd::MapRendererResources::updateResources(const QSet<TileId>& tiles, const ZoomLevel zoom)
{
    // Before requesting missing tiled resources, clean up cache to free some space
    //cleanupJunkResources(tiles, zoom);

    // In the end of rendering processing, request tiled resources that are neither
    // present in requested list, nor in pending, nor in uploaded
    requestNeededResources(tiles, zoom);
}

unsigned int OsmAnd::MapRendererResources::uploadResources(const unsigned int limit /*= 0u*/, bool* const outMoreThanLimitAvailable /*= nullptr*/)
{
    unsigned int totalUploaded = 0u;
    bool moreThanLimitAvailable = false;
    bool atLeastOneUploadFailed = false;

    for(auto itResourcesCollections = _storage.cbegin(); itResourcesCollections != _storage.cend(); ++itResourcesCollections)
    {
        const auto& resourcesCollections = *itResourcesCollections;

        for(auto itResourcesCollection = resourcesCollections.cbegin(); itResourcesCollection != resourcesCollections.cend(); ++itResourcesCollection)
        {
            const auto& resourcesCollection = *itResourcesCollection;

            // Skip empty entries
            if(!static_cast<bool>(resourcesCollection))
                continue;

            // Select all resources with "Ready" state
            QList< std::shared_ptr<BaseTiledResource> > resources;
            resourcesCollection->obtainEntries(&resources, [&resources, limit, totalUploaded, &moreThanLimitAvailable](const std::shared_ptr<BaseTiledResource>& entry, bool& cancel) -> bool
            {
                // Skip not-ready resources
                if(entry->getState() != ResourceState::Ready)
                    return false;

                // Check if limit was reached
                if(limit > 0 && (totalUploaded + resources.size()) > limit)
                {
                    // Tell that more resources are available for upload
                    moreThanLimitAvailable = true;

                    cancel = true;
                    return false;
                }

                // Accept this resource
                return true;
            });
            if(resources.isEmpty())
                continue;

            // Upload to GPU all selected resources
            for(auto itResource = resources.cbegin(); itResource != resources.cend(); ++itResource)
            {
                const auto& resource = *itResource;

                // Since state change is allowed (it's not changed to "Uploading" during query), check state here
                if(!resource->setStateIf(ResourceState::Ready, ResourceState::Uploading))
                    continue;

                // Actually upload resource to GPU
                const auto didUpload = resource->uploadToGPU();
                if(!atLeastOneUploadFailed && !didUpload)
                    atLeastOneUploadFailed = true;
                if(!didUpload)
                {
                    LogPrintf(LogSeverityLevel::Error, "Failed to upload tiled resource for %dx%d@%d to GPU", resource->tileId.x, resource->tileId.y, resource->zoom);
                    continue;
                }

                // Mark as uploaded
                assert(resource->getState() == ResourceState::Uploading);
                resource->setState(ResourceState::Uploaded);

                // Count uploaded resources
                totalUploaded++;
            }
        }
    }

    // If any resource failed to upload, report that more ready resources are available
    if(atLeastOneUploadFailed)
        moreThanLimitAvailable = true;

    if(outMoreThanLimitAvailable)
        *outMoreThanLimitAvailable = moreThanLimitAvailable;
    return totalUploaded;
}

void OsmAnd::MapRendererResources::cleanupJunkResources(const QSet<TileId>& tiles, const ZoomLevel zoom)
{
    // Use aggressive cache cleaning: remove all tiled resources that are not needed
    for(auto itResourcesCollections = _storage.cbegin(); itResourcesCollections != _storage.cend(); ++itResourcesCollections)
    {
        const auto& resourcesCollections = *itResourcesCollections;

        for(auto itResourcesCollection = resourcesCollections.cbegin(); itResourcesCollection != resourcesCollections.cend(); ++itResourcesCollection)
        {
            const auto& resourcesCollection = *itResourcesCollection;

            // Skip empty entries
            if(!static_cast<bool>(resourcesCollection))
                continue;

            const auto dataSourceAvailable = isDataSourceAvailableFor(resourcesCollection);

            resourcesCollection->removeEntries([this, dataSourceAvailable, tiles, zoom](const std::shared_ptr<BaseTiledResource>& entry, bool& cancel) -> bool
            {
                // Skip cleaning if this tiled resource is needed
                if((tiles.contains(entry->tileId) && entry->zoom == zoom) && dataSourceAvailable)
                    return false;

                // Cleanup is only possible for entries that are in "not-in-progress" states.
                // Following situations are possible:
                //  - entry will be processed by a task, and has a "Requested" state.
                //    Cancel the task and remove entry. A post-execute handler will be called that will do nothing.
                //    If a execute handler will be called, it will see the "JustBeforeDeath" and skip it.
                //  - entry can be removed, since it has "ProcessingRequest" state, so just mark it
                //  - entry is in "Ready" state.
                //    Just remove the entry in this case.
                //  - entry is in "Uploaded" state.
                //    Unload resource from GPU, since it's the only place where this action is possible.
                // All other situations are unhandled, so don't remove entry.

                if(entry->setStateIf(ResourceState::Requested, ResourceState::JustBeforeDeath))
                {
                    // Cancel the task
                    assert(entry->_requestTask != nullptr);
                    entry->_requestTask->requestCancellation();

                    return true;
                }
                else if(entry->setStateIf(ResourceState::ProcessingRequest, ResourceState::JustBeforeDeath))
                {
                    return true;
                }
                else if(entry->setStateIf(ResourceState::Ready, ResourceState::JustBeforeDeath))
                {
                    return true;
                }
                else if(entry->setStateIf(ResourceState::Uploaded, ResourceState::Unloading))
                {
                    entry->unloadFromGPU();

                    entry->setState(ResourceState::Unloaded);
                    entry->setState(ResourceState::JustBeforeDeath);
                    return true;
                }

                //NOTE: this may happen when resources are uploaded in separate thread, or IsBeingUsed. Actually this will happen
                assert(false);
                return false;
            });
        }
    }
}

void OsmAnd::MapRendererResources::releaseResourcesFrom(const std::shared_ptr<TiledResourcesCollection>& collection)
{
    // Remove all tiles, releasing associated GPU resources
    collection->removeEntries([](const std::shared_ptr<BaseTiledResource>& entry, bool& cancel) -> bool
    {
        // When resources are released, it's a termination process, so all entries have to be removed.
        // This limits set of possible states of the entries to:
        // - Requested
        // - ProcessingRequest
        // - Ready
        // - Uploaded
        // All other states are considered invalid.

        if(entry->setStateIf(ResourceState::Requested, ResourceState::JustBeforeDeath))
        {
            // Cancel the task
            assert(entry->_requestTask != nullptr);
            entry->_requestTask->requestCancellation();

            return true;
        }
        else if(entry->setStateIf(ResourceState::ProcessingRequest, ResourceState::JustBeforeDeath))
        {
            return true;
        }
        else if(entry->setStateIf(ResourceState::Ready, ResourceState::JustBeforeDeath))
        {
            return true;
        }
        else if(entry->setStateIf(ResourceState::Uploaded, ResourceState::Unloading))
        {
            entry->unloadFromGPU();

            entry->setState(ResourceState::Unloaded);
            entry->setState(ResourceState::JustBeforeDeath);
            return true;
        }

        //NOTE: this may happen when resources are uploaded in separate thread. Actually this will happen
        const auto state = entry->getState();
        assert(false);
        return false;
    });
}

void OsmAnd::MapRendererResources::requestResourcesUpload()
{
    // This is obsolete, should be redesigned
    renderer->requestResourcesUpload();
}

std::shared_ptr<const OsmAnd::MapRendererResources::TiledResourcesCollection> OsmAnd::MapRendererResources::getCollection(const ResourceType type, const std::shared_ptr<IMapProvider>& provider) const
{
    return _bindings[static_cast<int>(type)].providersToCollections[provider];
}

OsmAnd::MapRendererResources::GenericResource::GenericResource(MapRendererResources* owner_, const ResourceType type_)
    : _requestTask(nullptr)
    , owner(owner_)
    , type(type_)
{
}

OsmAnd::MapRendererResources::GenericResource::~GenericResource()
{
}

OsmAnd::MapRendererResources::BaseTiledResource::BaseTiledResource(MapRendererResources* owner, const ResourceType type, const TilesCollection<BaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom)
    : GenericResource(owner, type)
    , TilesCollectionEntryWithState(collection, tileId, zoom)
{

}

OsmAnd::MapRendererResources::BaseTiledResource::~BaseTiledResource()
{
    const volatile auto state = getState();
    if(state == ResourceState::Uploading || state == ResourceState::Uploaded || state == ResourceState::IsBeingUsed || state == ResourceState::Unloading)
        LogPrintf(LogSeverityLevel::Error, "Tiled resource for %dx%d@%d still resides in GPU memory. This may cause GPU memory leak", tileId.x, tileId.y, zoom);
}

OsmAnd::MapRendererResources::TiledResourcesCollection::TiledResourcesCollection(const ResourceType& type_)
    : type(type_)
{
}

OsmAnd::MapRendererResources::TiledResourcesCollection::~TiledResourcesCollection()
{
    verifyNoUploadedTilesPresent();
}

void OsmAnd::MapRendererResources::TiledResourcesCollection::removeAllEntries()
{
    verifyNoUploadedTilesPresent();

    TilesCollection::removeAllEntries();
}

void OsmAnd::MapRendererResources::TiledResourcesCollection::verifyNoUploadedTilesPresent() const
{
    // Ensure that no tiles have "Uploaded" state
    bool stillUploadedTilesPresent = false;
    obtainEntries(nullptr, [&stillUploadedTilesPresent](const std::shared_ptr<BaseTiledResource>& tileEntry, bool& cancel) -> bool
    {
        const auto state = tileEntry->getState();
        if(state == ResourceState::Uploading || state == ResourceState::Uploaded || state == ResourceState::IsBeingUsed || state == ResourceState::Unloading)
        {
            stillUploadedTilesPresent = true;
            cancel = true;
            return false;
        }

        return false;
    });
    if(stillUploadedTilesPresent)
        LogPrintf(LogSeverityLevel::Error, "One or more tiled resources still reside in GPU memory. This may cause GPU memory leak");
    assert(stillUploadedTilesPresent == false);
}

OsmAnd::MapRendererResources::MapTileResource::MapTileResource(MapRendererResources* owner, const ResourceType type, const TilesCollection<BaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom)
    : BaseTiledResource(owner, type, collection, tileId, zoom)
    , sourceData(_sourceData)
    , resourceInGPU(_resourceInGPU)
{
}

OsmAnd::MapRendererResources::MapTileResource::~MapTileResource()
{
}

bool OsmAnd::MapRendererResources::MapTileResource::obtainData(bool& dataAvailable)
{
    // Get source of tile
    std::shared_ptr<IMapProvider> provider_;
    bool ok = owner->obtainProviderFor(static_cast<TiledResourcesCollection*>(&link.lock()->collection), provider_);
    if(!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapTileProvider>(provider_);

    // Obtain tile from provider
    std::shared_ptr<const MapTile> tile;
    const auto requestSucceeded = provider->obtainTile(tileId, zoom, tile);
    if(!requestSucceeded)
        return false;

    // Store data
    _sourceData = tile;
    dataAvailable = static_cast<bool>(tile);

    return true;
}

bool OsmAnd::MapRendererResources::MapTileResource::uploadToGPU()
{
    const auto& preparedSourceData = owner->renderer->prepareTileForUploadingToGPU(_sourceData);
    //TODO: This is weird, and probably should not be here. RenderAPI knows how to upload what, but on contrary - does not know the limits
    const auto tilesPerAtlasTextureLimit = owner->renderer->getTilesPerAtlasTextureLimit(type, _sourceData);
    bool ok = owner->renderer->renderAPI->uploadTileToGPU(preparedSourceData, tilesPerAtlasTextureLimit, _resourceInGPU);
    if(!ok)
        return false;

    // Release source data:
    if(const auto retainedSource = std::dynamic_pointer_cast<const IRetainedMapTile>(_sourceData))
    {
        // If map tile implements 'Retained' interface, it must be kept, but 
        std::const_pointer_cast<IRetainedMapTile>(retainedSource)->releaseNonRetainedData();
    }
    else
    {
        // or simply release entire tile
        _sourceData.reset();
    }

    return true;
}

void OsmAnd::MapRendererResources::MapTileResource::unloadFromGPU()
{
    assert(_resourceInGPU.use_count() == 1);
    _resourceInGPU.reset();
}

OsmAnd::MapRendererResources::SymbolsTileResource::SymbolsTileResource(MapRendererResources* owner, const TilesCollection<BaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom)
    : BaseTiledResource(owner, ResourceType::Symbols, collection, tileId, zoom)
    , sourceData(_sourceData)
    , resourcesInGPU(_resourcesInGPU)
{
}

OsmAnd::MapRendererResources::SymbolsTileResource::~SymbolsTileResource()
{
    // When symbols resource entry was unloaded and is destroyed, remove from unified collection it's unique symbols
    for(auto itSymbol = _sourceData.cbegin(); itSymbol != _sourceData.cend(); ++itSymbol)
    {
        const auto& symbol = *itSymbol;

        // If this is not the last reference to symbol, skip it
        if(!symbol.unique())
            continue;

        // Otherwise, remove weak reference from collection by id of WHAT??
        {
            //QMutexLocker scopedLocker(&owner->_symbolsMutex);
            //            _owner->_symbols[symbol->order].remove(symbol->id);
        }
    }
}

bool OsmAnd::MapRendererResources::SymbolsTileResource::obtainData(bool& dataAvailable)
{
    // Get source of tile
    std::shared_ptr<IMapProvider> provider_;
    bool ok = owner->obtainProviderFor(static_cast<TiledResourcesCollection*>(&link.lock()->collection), provider_);
    if(!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapSymbolProvider>(provider_);

    // 
    //NOTE: SymbolsResourceEntry represents a since tile@zoom. In a single provider. That means, that multiple symbol providers provide data for same tile@zoom
    //NOTE: part of resources collection. A symbol resource collection is bound to provider

    //TODO: a cache of symbols needs to be maintained, since same symbol may be present in several tiles, but it should be drawn once?
    //provider->obtainSymbols(tileId, zoom, _sourceData);

    // add std::weak_ptr to QMap< order_from_rules as int, QList<MapSymbol> > g_symbols?

    return false;
}

bool OsmAnd::MapRendererResources::SymbolsTileResource::uploadToGPU()
{
    return false;
}

void OsmAnd::MapRendererResources::SymbolsTileResource::unloadFromGPU()
{

}

OsmAnd::MapRendererResources::ResourceRequestTask::ResourceRequestTask(
    const std::shared_ptr<IResource>& requestedResource_,
    const Concurrent::TaskHost::Bridge& bridge, ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod /*= nullptr*/, PostExecuteSignature postExecuteMethod /*= nullptr*/)
    : HostedTask(bridge, executeMethod, preExecuteMethod, postExecuteMethod)
    , requestedResource(requestedResource_)
{
}

OsmAnd::MapRendererResources::ResourceRequestTask::~ResourceRequestTask()
{
}