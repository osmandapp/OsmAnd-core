#include "MapRendererResources.h"

#include <cassert>

#include "MapRenderer.h"
#include "IMapProvider.h"
#include "IMapBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "IMapSymbolProvider.h"
#include "IRetainableResource.h"
#include "MapObject.h"
#include "EmbeddedResources.h"
#include "LambdaQueryController.h"
#include "QKeyValueIterator.h"
#include "QCachingIterator.h"
#include "Utilities.h"
#include "Logging.h"

#include <SkBitmap.h>
#include <SkImageDecoder.h>

//#define OSMAND_LOG_RESOURCE_STATE_CHANGE 1
#ifndef OSMAND_LOG_RESOURCE_STATE_CHANGE
#   define OSMAND_LOG_RESOURCE_STATE_CHANGE 0
#endif // !defined(OSMAND_LOG_RESOURCE_STATE_CHANGE)

#if OSMAND_LOG_RESOURCE_STATE_CHANGE
#   define LOG_STATE_CHANGE(tileId, zoom, oldState, newState) \
    LogPrintf(LogSeverityLevel::Debug, "Tile %dx%d@%d state change '" #oldState "'->'" #newState "' at " __FILE__ ":" QT_STRINGIFY(__LINE__), (tileId).x, (tileId).y, (zoom))
#else
#   define LOG_STATE_CHANGE(tileId, zoom, oldState, newState)
#endif

OsmAnd::MapRendererResources::MapRendererResources(MapRenderer* const owner_)
    : _taskHostBridge(this)
    , _mapSymbolsCount(0)
    , _invalidatedResourcesTypesMask(0)
    , _workerThreadIsAlive(false)
    , _workerThreadId(nullptr)
    , _workerThread(new Concurrent::Thread(std::bind(&MapRendererResources::workerThreadProcedure, this)))
    , renderer(owner_)
    , processingTileStub(_processingTileStub)
    , unavailableTileStub(_unavailableTileStub)
{
#if OSMAND_DEBUG
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
    // Release all resources
    for(auto& resourcesCollections : _storage)
    {
        for(const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if(!resourcesCollection)
                continue;
            releaseResourcesFrom(resourcesCollection);
        }
        resourcesCollections.clear();
    }

    // Stop worker thread
    _workerThreadIsAlive = false;
    {
        QMutexLocker scopedLocker(&_workerThreadWakeupMutex);
        _workerThreadWakeup.wakeAll();
    }
    REPEAT_UNTIL(_workerThread->wait());

    // Release all bindings
    for(auto resourceType = 0; resourceType < ResourceTypesCount; resourceType++)
    {
        auto& bindings = _bindings[resourceType];

        bindings.providersToCollections.clear();
        bindings.collectionsToProviders.clear();
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
            std::shared_ptr<const MapTile> bitmapTile(new MapBitmapTile(bitmap, AlphaChannelData::Undefined));
            uploadTileToGPU(bitmapTile, _processingTileStub);
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
            std::shared_ptr<const MapTile> bitmapTile(new MapBitmapTile(bitmap, AlphaChannelData::Undefined));
            uploadTileToGPU(bitmapTile, _unavailableTileStub);
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

bool OsmAnd::MapRendererResources::uploadTileToGPU(const std::shared_ptr<const MapTile>& mapTile, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU)
{
    std::shared_ptr<const MapTile> convertedTile;
    const auto wasConverted = renderer->convertMapTile(mapTile, convertedTile);
    return renderer->gpuAPI->uploadTileToGPU(wasConverted ? convertedTile : mapTile, outResourceInGPU);
}

bool OsmAnd::MapRendererResources::uploadSymbolToGPU(const std::shared_ptr<const MapSymbol>& mapSymbol, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU)
{
    std::shared_ptr<const MapSymbol> convertedSymbol;
    const auto wasConverted = renderer->convertMapSymbol(mapSymbol, convertedSymbol);
    return renderer->gpuAPI->uploadSymbolToGPU(wasConverted ? convertedSymbol : mapSymbol, outResourceInGPU);
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

            // Remove resources collection
            resources.removeOne(itBindedProvider.value());

            // Remove binding
            bindings.collectionsToProviders.remove(itBindedProvider.value());
            itBindedProvider.remove();
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
        for(auto itProvider = iteratorOf(constOf(state.rasterLayerProviders)); itProvider; ++itProvider, rasterLayerIdx++)
        {
            const auto& provider = *itProvider;

            // Skip empty providers
            if(!provider)
                continue;

            // If binding already exists, skip creation
            if(bindings.providersToCollections.contains(std::static_pointer_cast<IMapProvider>(provider)))
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
        for(const auto& provider : constOf(state.symbolProviders))
        {
            // If binding already exists, skip creation
            if(bindings.providersToCollections.contains(std::static_pointer_cast<IMapProvider>(provider)))
                continue;

            // Create new resources collection
            const std::shared_ptr< TiledResourcesCollection > newResourcesCollection(new SymbolsResourcesCollection());

            // Add binding
            bindings.providersToCollections.insert(provider, newResourcesCollection);
            bindings.collectionsToProviders.insert(newResourcesCollection, provider);

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
    for(const auto& bindingEntry : rangeOf(constOf(bindings.collectionsToProviders)))
    {
        if(bindingEntry.key().get() != resourcesRef)
            continue;

        const auto& testProvider = bindingEntry.value();
        if(!testProvider)
            return false;

        provider = testProvider;
        return true;
    }

    return false;
}

bool OsmAnd::MapRendererResources::isDataSourceAvailableFor(const std::shared_ptr<TiledResourcesCollection>& collection) const
{
    const auto& binding = _bindings[static_cast<int>(collection->type)];

    return binding.collectionsToProviders.contains(collection);
}

void OsmAnd::MapRendererResources::addMapSymbol(const std::shared_ptr<const MapSymbol>& symbol, const std::shared_ptr<const GPUAPI::ResourceInGPU>& gpuResource)
{
    QMutexLocker scopedLocker(&_mapSymbolsByOrderMutex);

    _mapSymbolsByOrder[symbol->order].insert(symbol, gpuResource);
    _mapSymbolsCount++;
}

void OsmAnd::MapRendererResources::removeMapSymbol(const std::shared_ptr<const MapSymbol>& symbol)
{
    QMutexLocker scopedLocker(&_mapSymbolsByOrderMutex);

    const auto itSymbolsLayer = _mapSymbolsByOrder.find(symbol->order);
    const auto removedCount = itSymbolsLayer->remove(symbol);
    if(itSymbolsLayer->isEmpty())
        _mapSymbolsByOrder.erase(itSymbolsLayer);
    _mapSymbolsCount -= removedCount;
}

void OsmAnd::MapRendererResources::notifyNewResourceAvailableForDrawing()
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
            QMutexLocker scopedLocker(&_workerThreadWakeupMutex);
            REPEAT_UNTIL(_workerThreadWakeup.wait(&_workerThreadWakeupMutex));

            // Copy active zone to local copy
            activeTiles = _activeTiles;
            activeZoom = _activeZoom;
        }
        if(!_workerThreadIsAlive)
            break;

        // Update resources
        updateResources(activeTiles, activeZoom);
    }

    _workerThreadId = nullptr;
}

void OsmAnd::MapRendererResources::requestNeededResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom)
{
    // Request missing resources
    for(const auto& activeTileId : constOf(activeTiles))
    {
        for(const auto& resourcesCollections : constOf(_storage))
        {
            for(const auto& resourcesCollection : constOf(resourcesCollections))
            {
                // Skip empty entries
                if(!resourcesCollection)
                    continue;

                // Skip resource types that do not have an available data source
                if(!isDataSourceAvailableFor(resourcesCollection))
                    continue;

                // Obtain a resource entry and if it's state is "Unknown", create a task that will
                // request resource data
                std::shared_ptr<BaseTiledResource> resource;
                const auto resourceType = resourcesCollection->type;
                resourcesCollection->obtainOrAllocateEntry(resource, activeTileId, activeZoom,
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
                LOG_STATE_CHANGE(resource->tileId, resource->zoom, ResourceState::Unknown, ResourceState::Requesting);

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
                    else
                    {
                        LOG_STATE_CHANGE(resource->tileId, resource->zoom, ResourceState::Requested, ResourceState::ProcessingRequest);
                    }

                    // Ask resource to obtain it's data
                    bool dataAvailable = false;
                    LambdaQueryController obtainDataQueryController(
                        [task_]() -> bool
                        {
                            return task_->isCancellationRequested();
                        });
                    const auto requestSucceeded = resource->obtainData(dataAvailable, &obtainDataQueryController) && !task->isCancellationRequested();

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
                        assert(resource->getState() == ResourceState::RequestCanceledWhileBeingProcessed);

                        // While request was processed, state may have changed to "RequestCanceledWhileBeingProcessed"
                        task->requestCancellation();
                        return;
                    }
#if OSMAND_LOG_RESOURCE_STATE_CHANGE
                    else
                    {
                        if(dataAvailable)
                            LOG_STATE_CHANGE(resource->tileId, resource->zoom, ResourceState::ProcessingRequest, ResourceState::Ready);
                        else
                            LOG_STATE_CHANGE(resource->tileId, resource->zoom, ResourceState::ProcessingRequest, ResourceState::Unavailable);
                    }
#endif // OSMAND_LOG_RESOURCE_STATE_CHANGE
                    resource->_requestTask = nullptr;

                    // There is data to upload to GPU, request uploading. Or just ask to show that resource is unavailable
                    if(dataAvailable)
                        requestResourcesUploadOrUnload();
                    else
                        notifyNewResourceAvailableForDrawing();
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
                            resource->setStateIf(ResourceState::Unavailable, ResourceState::JustBeforeDeath) ||
                            resource->setStateIf(ResourceState::RequestCanceledWhileBeingProcessed, ResourceState::JustBeforeDeath))
                        {
                            LOG_STATE_CHANGE(resource->tileId, resource->zoom, ?, ResourceState::JustBeforeDeath);

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
                LOG_STATE_CHANGE(resource->tileId, resource->zoom, ? , ResourceState::Requested);

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

    for(const auto& resourcesCollection : constOf(resourcesCollections))
    {
        if(!bindings.collectionsToProviders.contains(resourcesCollection))
            continue;

        // Mark all resources as junk
        resourcesCollection->forAllExecute(
            [](const std::shared_ptr<BaseTiledResource>& entry, bool& cancel)
            {
                entry->markAsJunk();
            });
    }
}

void OsmAnd::MapRendererResources::updateResources(const QSet<TileId>& tiles, const ZoomLevel zoom)
{
    // Before requesting missing tiled resources, clean up cache to free some space
    cleanupJunkResources(tiles, zoom);

    // In the end of rendering processing, request tiled resources that are neither
    // present in requested list, nor in pending, nor in uploaded
    requestNeededResources(tiles, zoom);
}

unsigned int OsmAnd::MapRendererResources::unloadResources()
{
    unsigned int totalUnloaded = 0u;

    for(const auto& resourcesCollections : constOf(_storage))
    {
        for(const auto& resourcesCollection : constOf(resourcesCollections))
        {
            // Skip empty entries
            if(!resourcesCollection)
                continue;

            // Select all resources with "UnloadPending" state
            QList< std::shared_ptr<BaseTiledResource> > resources;
            resourcesCollection->obtainEntries(&resources,
                [&resources](const std::shared_ptr<BaseTiledResource>& entry, bool& cancel) -> bool
                {
                    // Skip not-"UnloadPending" resources
                    if(entry->getState() != ResourceState::UnloadPending)
                        return false;

                    // Accept this resource
                    return true;
                });
            if(resources.isEmpty())
                continue;

            // Unload from GPU all selected resources
            for(const auto& resource : constOf(resources))
            {
                // Since state change is allowed (it's not changed to "Unloading" during query), check state here
                if(!resource->setStateIf(ResourceState::UnloadPending, ResourceState::Unloading))
                    continue;
                LOG_STATE_CHANGE(resource->tileId, resource->zoom, ResourceState::UnloadPending, ResourceState::Unloading);
                
                // Unload from GPU
                resource->unloadFromGPU();

                // Don't wait until GPU will execute unloading, since this resource won't be used anymore and will eventually be deleted

                // Mark as unloaded
                assert(resource->getState() == ResourceState::Unloading);
                resource->setState(ResourceState::Unloaded);
                LOG_STATE_CHANGE(resource->tileId, resource->zoom, ResourceState::Unloading, ResourceState::Unloaded);

                // Count uploaded resources
                totalUnloaded++;
            }
        }
    }

    return totalUnloaded;
}

unsigned int OsmAnd::MapRendererResources::uploadResources(const unsigned int limit /*= 0u*/, bool* const outMoreThanLimitAvailable /*= nullptr*/)
{
    unsigned int totalUploaded = 0u;
    bool moreThanLimitAvailable = false;
    bool atLeastOneUploadFailed = false;

    for(const auto& resourcesCollections : constOf(_storage))
    {
        for(const auto& resourcesCollection : constOf(resourcesCollections))
        {
            // Skip empty entries
            if(!resourcesCollection)
                continue;

            // Select all resources with "Ready" state
            QList< std::shared_ptr<BaseTiledResource> > resources;
            resourcesCollection->obtainEntries(&resources,
                [&resources, limit, totalUploaded, &moreThanLimitAvailable](const std::shared_ptr<BaseTiledResource>& entry, bool& cancel) -> bool
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
            for(const auto& resource : constOf(resources))
            {
                // Since state change is allowed (it's not changed to "Uploading" during query), check state here
                if(!resource->setStateIf(ResourceState::Ready, ResourceState::Uploading))
                    continue;
                LOG_STATE_CHANGE(resource->tileId, resource->zoom, ResourceState::Ready, ResourceState::Uploading);

                // Actually upload resource to GPU
                const auto didUpload = resource->uploadToGPU();
                if(!atLeastOneUploadFailed && !didUpload)
                    atLeastOneUploadFailed = true;
                if(!didUpload)
                {
                    LogPrintf(LogSeverityLevel::Error, "Failed to upload tiled resource for %dx%d@%d to GPU", resource->tileId.x, resource->tileId.y, resource->zoom);
                    continue;
                }

                // Before marking as uploaded, if uploading is done from GPU worker thread,
                // wait until operation completes
                if(renderer->setupOptions.gpuWorkerThreadEnabled)
                    renderer->gpuAPI->waitUntilUploadIsComplete();

                // Mark as uploaded
                assert(resource->getState() == ResourceState::Uploading);
                resource->setState(ResourceState::Uploaded);
                LOG_STATE_CHANGE(resource->tileId, resource->zoom, ResourceState::Uploading, ResourceState::Uploaded);

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

void OsmAnd::MapRendererResources::cleanupJunkResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom)
{
    // This method is called from non-GPU thread, so it's impossible to unload resources from GPU here
    bool needsResourcesUploadOrUnload = false;

    // Use aggressive cache cleaning: remove all tiled resources that are not needed
    for(const auto& resourcesCollections : constOf(_storage))
    {
        for(const auto& resourcesCollection : constOf(resourcesCollections))
        {
            // Skip empty entries
            if(!resourcesCollection)
                continue;

            const auto dataSourceAvailable = isDataSourceAvailableFor(resourcesCollection);

            resourcesCollection->removeEntries(
                [this, dataSourceAvailable, activeTiles, activeZoom, &needsResourcesUploadOrUnload]
                (const std::shared_ptr<BaseTiledResource>& entry, bool& cancel) -> bool
                {
                    // Resource with "Unloaded" state is junk, regardless if it's needed or not
                    if(entry->setStateIf(ResourceState::Unloaded, ResourceState::JustBeforeDeath))
                    {
                        LOG_STATE_CHANGE(entry->tileId, entry->zoom, ResourceState::Unloaded, ResourceState::JustBeforeDeath);

                        // If resource was unloaded from GPU, remove the entry.
                        return true;
                    }

                    // Determine if resource is junk
                    bool isJunk = false;
                    // If it was previously marked as junk
                    isJunk = isJunk || entry->isJunk;
                    // If data source is gone, all resources from it are considered junk:
                    isJunk = isJunk || !dataSourceAvailable;
                    // If resource is not in set of "needed tiles", it's junk:
                    isJunk = isJunk || !(activeTiles.contains(entry->tileId) && entry->zoom == activeZoom);

                    // Skip cleaning if this resource is not junk
                    if(!isJunk)
                        return false;

                    // Mark this entry as junk until it will die
                    entry->markAsJunk();

                    if(entry->setStateIf(ResourceState::Uploaded, ResourceState::UnloadPending))
                    {
                        LOG_STATE_CHANGE(entry->tileId, entry->zoom, ResourceState::Uploaded, ResourceState::UnloadPending);

                        // If resource is not needed anymore, change its state to "UnloadPending",
                        // but keep the resource entry, since it must be unload from GPU in another place

                        return false;
                    }
                    else if(entry->setStateIf(ResourceState::Ready, ResourceState::JustBeforeDeath))
                    {
                        LOG_STATE_CHANGE(entry->tileId, entry->zoom, ResourceState::Ready, ResourceState::JustBeforeDeath);

                        // If resource was not yet uploaded, just remove it.
                        return true;
                    }
                    else if(entry->setStateIf(ResourceState::ProcessingRequest, ResourceState::RequestCanceledWhileBeingProcessed))
                    {
                        LOG_STATE_CHANGE(entry->tileId, entry->zoom, ResourceState::ProcessingRequest, ResourceState::RequestCanceledWhileBeingProcessed);

                        // If resource request is being processed, keep the entry until processing is complete.

                        return false;
                    }
                    else if(entry->setStateIf(ResourceState::Requested, ResourceState::JustBeforeDeath))
                    {
                        LOG_STATE_CHANGE(entry->tileId, entry->zoom, ResourceState::Requested, ResourceState::JustBeforeDeath);

                        // If resource was just requested, cancel its task and remove the entry.

                        // Cancel the task
                        assert(entry->_requestTask != nullptr);
                        entry->_requestTask->requestCancellation();

                        return true;
                    }
                    else if(entry->setStateIf(ResourceState::Unavailable, ResourceState::JustBeforeDeath))
                    {
                        LOG_STATE_CHANGE(entry->tileId, entry->zoom, ResourceState::Unavailable, ResourceState::JustBeforeDeath);

                        // If resource was never available, just remove the entry
                        return true;
                    }

                    const auto state = entry->getState();
                    if(state == ResourceState::JustBeforeDeath)
                    {
                        // If resource has JustBeforeDeath state, it means that it will be deleted from different thread in next few moments,
                        // so it's safe to remove it here also (since operations with resources removal from collection are atomic, and collection is blocked)
                        return true;
                    }
                
                    // Resources with states
                    //  - Uploading (to allow finish uploading)
                    //  - UnloadPending (to allow start unloading from GPU)
                    //  - Unloading (to allow finish unloading)
                    //  - RequestCanceledWhileBeingProcessed (to allow cleanup of the process)
                    // should be retained, since they are being processed. So try to next time
                    
                    return false;
                });
        }
    }

    if(needsResourcesUploadOrUnload)
        requestResourcesUploadOrUnload();
}

void OsmAnd::MapRendererResources::releaseResourcesFrom(const std::shared_ptr<TiledResourcesCollection>& collection)
{
    // This method is called from non-GPU thread, so it's impossible to unload resources from GPU here.
    // So wait here until all resources will be unloaded from GPU
    bool needsResourcesUploadOrUnload = false;

    bool containedUnprocessableResources = false;
    do
    {
        containedUnprocessableResources = false;
        collection->removeEntries(
            [&needsResourcesUploadOrUnload, &containedUnprocessableResources]
            (const std::shared_ptr<BaseTiledResource>& entry, bool& cancel) -> bool
            {
                // When resources are released, it's a termination process, so all entries have to be removed.
                // This limits set of possible states of the entries to:
                // - Requested
                // - ProcessingRequest
                // - Ready
                // - Unloaded
                // - Uploaded
                // - Unavailable
                // All other states are considered invalid.

                if(entry->setStateIf(ResourceState::Requested, ResourceState::JustBeforeDeath))
                {
                    LOG_STATE_CHANGE(entry->tileId, entry->zoom, ResourceState::Requested, ResourceState::JustBeforeDeath);

                    // Cancel the task
                    assert(entry->_requestTask != nullptr);
                    entry->_requestTask->requestCancellation();

                    return true;
                }
                else if(entry->setStateIf(ResourceState::ProcessingRequest, ResourceState::RequestCanceledWhileBeingProcessed))
                {
                    LOG_STATE_CHANGE(entry->tileId, entry->zoom, ResourceState::ProcessingRequest, ResourceState::RequestCanceledWhileBeingProcessed);

                    containedUnprocessableResources = true;

                    return false;
                }
                else if(entry->setStateIf(ResourceState::Ready, ResourceState::JustBeforeDeath))
                {
                    LOG_STATE_CHANGE(entry->tileId, entry->zoom, ResourceState::Ready, ResourceState::JustBeforeDeath);

                    return true;
                }
                else if(entry->setStateIf(ResourceState::Unloaded, ResourceState::JustBeforeDeath))
                {
                    LOG_STATE_CHANGE(entry->tileId, entry->zoom, ResourceState::Unloaded, ResourceState::JustBeforeDeath);

                    return true;
                }
                else if(entry->setStateIf(ResourceState::Uploaded, ResourceState::UnloadPending))
                {
                    LOG_STATE_CHANGE(entry->tileId, entry->zoom, ResourceState::Uploaded, ResourceState::UnloadPending);

                    // If resource is not needed anymore, change its state to "UnloadPending",
                    // but keep the resource entry, since it must be unload from GPU in another place
                    needsResourcesUploadOrUnload = true;
                    containedUnprocessableResources = true;

                    return false;
                }
                else if(entry->setStateIf(ResourceState::Unavailable, ResourceState::JustBeforeDeath))
                {
                    LOG_STATE_CHANGE(entry->tileId, entry->zoom, ResourceState::Unavailable, ResourceState::JustBeforeDeath);

                    return true;
                }

                const auto state = entry->getState();
                if(state == ResourceState::JustBeforeDeath)
                {
                    // If resource has JustBeforeDeath state, it means that it will be deleted from different thread in next few moments,
                    // so it's safe to remove it here also (since operations with resources removal from collection are atomic, and collection is blocked)
                    return true;
                }

                // Resources with states
                //  - Uploading (to allow finish uploading)
                //  - UnloadPending (to allow start unloading from GPU)
                //  - Unloading (to allow finish unloading)
                //  - RequestCanceledWhileBeingProcessed (to allow cleanup of the process)
                // should be retained, since they are being processed. So try to next time
                if(state == ResourceState::Uploading ||
                    state == ResourceState::UnloadPending ||
                    state == ResourceState::Unloading)
                {
                    needsResourcesUploadOrUnload = true;
                }

                LogPrintf(LogSeverityLevel::Debug, "Tile resource %dx%d@%d has state %d which can not be processed, need to wait",
                    entry->tileId.x,
                    entry->tileId.y,
                    entry->zoom,
                    static_cast<int>(state));
                containedUnprocessableResources = true;
                return false;
            });

            // If there are unprocessable resources and
            if(containedUnprocessableResources && needsResourcesUploadOrUnload)
            {
                requestResourcesUploadOrUnload();
                needsResourcesUploadOrUnload = false;
            }
    } while(containedUnprocessableResources);

    // Perform final request to upload or unload resources
    if(needsResourcesUploadOrUnload)
    {
        requestResourcesUploadOrUnload();
        needsResourcesUploadOrUnload = false;
    }

    // Check that collection is empty
    assert(collection->getEntriesCount() == 0);
}

void OsmAnd::MapRendererResources::requestResourcesUploadOrUnload()
{
    renderer->requestResourcesUploadOrUnload();
}

void OsmAnd::MapRendererResources::syncResourcesInGPU(
    const unsigned int limitUploads /*= 0u*/,
    bool* const outMoreUploadsThanLimitAvailable /*= nullptr*/,
    unsigned int* const outResourcesUploaded /*= nullptr*/,
    unsigned int* const outResourcesUnloaded /*= nullptr*/)
{
    // Unload resources
    const auto resourcesUnloaded = unloadResources();
    if(outResourcesUnloaded)
        *outResourcesUnloaded = resourcesUnloaded;

    // Upload resources
    const auto resourcesUploaded = uploadResources(limitUploads, outMoreUploadsThanLimitAvailable);
    if(outResourcesUploaded)
        *outResourcesUploaded = resourcesUploaded;
}

std::shared_ptr<const OsmAnd::MapRendererResources::TiledResourcesCollection> OsmAnd::MapRendererResources::getCollection(const ResourceType type, const std::shared_ptr<IMapProvider>& provider) const
{
    return _bindings[static_cast<int>(type)].providersToCollections[provider];
}

QMutex& OsmAnd::MapRendererResources::getSymbolsMapMutex() const
{
    return _mapSymbolsByOrderMutex;
}

const OsmAnd::MapRendererResources::MapSymbolsByOrder& OsmAnd::MapRendererResources::getMapSymbolsByOrder() const
{
    return _mapSymbolsByOrder;
}

unsigned int OsmAnd::MapRendererResources::getMapSymbolsCount() const
{
    return _mapSymbolsCount;
}

void OsmAnd::MapRendererResources::dumpResourcesInfo() const
{
    QMap<ResourceState, QString> resourceStateMap;
    resourceStateMap.insert(ResourceState::Unknown, QLatin1String("Unknown"));
    resourceStateMap.insert(ResourceState::Requesting, QLatin1String("Requesting"));
    resourceStateMap.insert(ResourceState::Requested, QLatin1String("Requested"));
    resourceStateMap.insert(ResourceState::ProcessingRequest, QLatin1String("ProcessingRequest"));
    resourceStateMap.insert(ResourceState::RequestCanceledWhileBeingProcessed, QLatin1String("RequestCanceledWhileBeingProcessed"));
    resourceStateMap.insert(ResourceState::Unavailable, QLatin1String("Unavailable"));
    resourceStateMap.insert(ResourceState::Ready, QLatin1String("Ready"));
    resourceStateMap.insert(ResourceState::Uploading, QLatin1String("Uploading"));
    resourceStateMap.insert(ResourceState::Uploaded, QLatin1String("Uploaded"));
    resourceStateMap.insert(ResourceState::IsBeingUsed, QLatin1String("IsBeingUsed"));
    resourceStateMap.insert(ResourceState::UnloadPending, QLatin1String("UnloadPending"));
    resourceStateMap.insert(ResourceState::Unloading, QLatin1String("Unloading"));
    resourceStateMap.insert(ResourceState::Unloaded, QLatin1String("Unloaded"));
    resourceStateMap.insert(ResourceState::JustBeforeDeath, QLatin1String("JustBeforeDeath"));

    QString dump;
    dump += QLatin1String("Resources:\n");
    dump += QLatin1String("--------------------------------------------------------------------------------\n");

    dump += QLatin1String("[Tiled] Elevation data:\n");
    for(const auto& resources : constOf(_storage[static_cast<int>(ResourceType::ElevationData)]))
    {
        if(!resources)
            continue;
        resources->obtainEntries(nullptr, [&dump, resourceStateMap](const std::shared_ptr<BaseTiledResource>& entry, bool& cancel) -> bool
        {
            dump += QString(QLatin1String("\t %1x%2@%3 state '%4'\n")).
                arg(entry->tileId.x).
                arg(entry->tileId.y).
                arg(static_cast<int>(entry->zoom)).
                arg(resourceStateMap[entry->getState()]);

            return false;
        });
        dump += "\t----------------------------------------------------------------------------\n";
    }

    dump += QLatin1String("[Tiled] Raster map:\n");
    for(const auto& resources : constOf(_storage[static_cast<int>(ResourceType::RasterMap)]))
    {
        if(!resources)
            continue;
        resources->obtainEntries(nullptr, [&dump, resourceStateMap](const std::shared_ptr<BaseTiledResource>& entry, bool& cancel) -> bool
        {
            dump += QString(QLatin1String("\t %1x%2@%3 state '%4'\n")).
                arg(entry->tileId.x).
                arg(entry->tileId.y).
                arg(static_cast<int>(entry->zoom)).
                arg(resourceStateMap[entry->getState()]);

            return false;
        });
        dump += QLatin1String("\t----------------------------------------------------------------------------\n");
    }

    dump += QLatin1String("[Tiled] Symbols:\n");
    for(const auto& resources : constOf(_storage[static_cast<int>(ResourceType::Symbols)]))
    {
        if(!resources)
            continue;
        resources->obtainEntries(nullptr, [&dump, resourceStateMap](const std::shared_ptr<BaseTiledResource>& entry, bool& cancel) -> bool
        {
            dump += QString(QLatin1String("\t %1x%2@%3 state '%4'\n")).
                arg(entry->tileId.x).
                arg(entry->tileId.y).
                arg(static_cast<int>(entry->zoom)).
                arg(resourceStateMap[entry->getState()]);

            return false;
        });
        dump += QLatin1String("\t----------------------------------------------------------------------------\n");
    }

    LogPrintf(LogSeverityLevel::Debug, qPrintable(dump));
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
    , _isJunk(false)
    , isJunk(_isJunk)
{
}

OsmAnd::MapRendererResources::BaseTiledResource::~BaseTiledResource()
{
    const volatile auto state = getState();
    if(state == ResourceState::Uploading || state == ResourceState::Uploaded || state == ResourceState::IsBeingUsed || state == ResourceState::Unloading)
        LogPrintf(LogSeverityLevel::Error, "Tiled resource for %dx%d@%d still resides in GPU memory. This may cause GPU memory leak", tileId.x, tileId.y, zoom);

    safeUnlink();
}

void OsmAnd::MapRendererResources::BaseTiledResource::markAsJunk()
{
    _isJunk = true;
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
    , resourceInGPU(_resourceInGPU)
{
}

OsmAnd::MapRendererResources::MapTileResource::~MapTileResource()
{
    safeUnlink();
}

bool OsmAnd::MapRendererResources::MapTileResource::obtainData(bool& dataAvailable, const IQueryController* queryController)
{
    bool ok = false;

    // Get source of tile
    std::shared_ptr<IMapProvider> provider_;
    if(const auto link_ = link.lock())
        ok = owner->obtainProviderFor(static_cast<TiledResourcesCollection*>(&link_->collection), provider_);
    if(!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapTileProvider>(provider_);

    // Obtain tile from provider
    std::shared_ptr<const MapTile> tile;
    const auto requestSucceeded = provider->obtainTile(tileId, zoom, tile, queryController);
    if(!requestSucceeded)
        return false;

    // Store data
    _sourceData = tile;
    dataAvailable = static_cast<bool>(tile);

    return true;
}

bool OsmAnd::MapRendererResources::MapTileResource::uploadToGPU()
{
    bool ok = owner->uploadTileToGPU(_sourceData, _resourceInGPU);
    if(!ok)
        return false;

    // Release source data:
    if(const auto retainedSource = std::dynamic_pointer_cast<const IRetainableResource>(_sourceData))
    {
        // If map tile implements 'Retained' interface, it must be kept, but 
        std::const_pointer_cast<IRetainableResource>(retainedSource)->releaseNonRetainedData();
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
{
}

OsmAnd::MapRendererResources::SymbolsTileResource::~SymbolsTileResource()
{
    safeUnlink();
}

bool OsmAnd::MapRendererResources::SymbolsTileResource::obtainData(bool& dataAvailable, const IQueryController* queryController)
{
    // Obtain collection link and maintain it
    const auto link_ = link.lock();
    if(!link_)
        return false;
    const auto collection = static_cast<SymbolsResourcesCollection*>(&link_->collection);

    // Get source of tile
    std::shared_ptr<IMapProvider> provider_;
    bool ok = owner->obtainProviderFor(static_cast<TiledResourcesCollection*>(&link_->collection), provider_);
    if(!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapSymbolProvider>(provider_);

    auto& sharedGroupsResources = collection->_sharedGroupsResources[zoom];

    // Obtain tile from provider
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);
    QList< std::shared_ptr<GroupResources> > referencedSharedGroupsResources;
    QList< proper::shared_future< std::shared_ptr<GroupResources> > > futureReferencedSharedGroupsResources;
    QSet< uint64_t > loadedSharedGroups;
    std::shared_ptr<const MapSymbolsTile> tile;
    const auto requestSucceeded = provider->obtainSymbols(tileId, zoom, tile,
        [this, provider, &sharedGroupsResources, &referencedSharedGroupsResources, &futureReferencedSharedGroupsResources, &loadedSharedGroups, tileBBox31](const std::shared_ptr<const Model::MapObject>& mapObject) -> bool
        {
            // All symbols that come from map object which can not be cached,
            // should be received.
            // Symbols from map object can be shared only in case this map object does not lay inside it's tile bbox completely
            if(!provider->canSymbolsBeSharedFrom(mapObject) || tileBBox31.contains(mapObject->bbox31))
                return true;

            // Check if this shared symbol is already available, or mark it as pending
            std::shared_ptr<GroupResources> sharedGroupResources;
            proper::shared_future< std::shared_ptr<GroupResources> > futureSharedGroupResources;
            if(sharedGroupsResources.obtainReferenceOrFutureReferenceOrMakePromise(mapObject->id, sharedGroupResources, futureSharedGroupResources))
            {
                if(static_cast<bool>(sharedGroupResources))
                    referencedSharedGroupsResources.push_back(qMove(sharedGroupResources));
                else
                    futureReferencedSharedGroupsResources.push_back(qMove(futureSharedGroupResources));
                return false;
            }

            // Or load this shared group
            loadedSharedGroups.insert(mapObject->id);
            return true;
        });
    if(!requestSucceeded)
        return false;

    // Store data
    _sourceData = tile;
    dataAvailable = static_cast<bool>(tile);

    // Process data
    if(!dataAvailable)
        return true;

    // Move referenced shared groups
    _referencedSharedGroupsResources = referencedSharedGroupsResources;

    // tile->symbolsGroups contains groups that derived from unique symbols, or loaded shared groups
    for(const auto& group : constOf(tile->symbolsGroups))
    {
        std::shared_ptr<GroupResources> groupResources(new GroupResources(group));

        // Check if this group is loaded as shared
        if(!loadedSharedGroups.contains(group->mapObject->id))
        {
            _uniqueGroupsResources.push_back(qMove(groupResources));
            continue;
        }

        // Otherwise insert it as shared group
        sharedGroupsResources.fulfilPromiseAndReference(group->mapObject->id, groupResources);
        _referencedSharedGroupsResources.push_back(qMove(groupResources));
    }

    // Wait for future referenced shared groups
    for(auto& futureGroup : futureReferencedSharedGroupsResources)
    {
        auto groupResources = futureGroup.get();
        
        _referencedSharedGroupsResources.push_back(qMove(groupResources));
    }

    // Release source data:
    if(const auto retainedSource = std::dynamic_pointer_cast<const IRetainableResource>(_sourceData))
    {
        // If map tile implements 'Retained' interface, it must be kept, but 
        std::const_pointer_cast<IRetainableResource>(retainedSource)->releaseNonRetainedData();
    }
    else
    {
        // or simply release entire tile
        _sourceData.reset();
    }

    return true;
}

bool OsmAnd::MapRendererResources::SymbolsTileResource::uploadToGPU()
{
    typedef std::pair< std::shared_ptr<const MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > SymbolResourceEntry;
    typedef std::pair< std::shared_ptr<const MapSymbol>, proper::shared_future< std::shared_ptr<const GPUAPI::ResourceInGPU> > > FutureSymbolResourceEntry;

    bool ok;
    bool anyUploadFailed = false;

    const auto link_ = link.lock();
    const auto collection = static_cast<SymbolsResourcesCollection*>(&link_->collection);

    // Unique
    QMultiHash< std::shared_ptr<GroupResources>, SymbolResourceEntry > uniqueUploaded;
    for(const auto& groupResources : constOf(_uniqueGroupsResources))
    {
        for(const auto& symbol : constOf(groupResources->group->symbols))
        {
            // Prepare data and upload to GPU
            assert(static_cast<bool>(symbol->bitmap));
            std::shared_ptr<const GPUAPI::ResourceInGPU> resourceInGPU;
            ok = owner->uploadSymbolToGPU(symbol, resourceInGPU);

            // If upload have failed, stop
            if(!ok)
            {
                LogPrintf(LogSeverityLevel::Error, "Failed to upload unique symbol (size %dx%d) in %dx%d@%d tile",
                    symbol->bitmap->width(), symbol->bitmap->height(),
                    tileId.x, tileId.y, zoom);

                anyUploadFailed = true;
                break;
            }

            // Mark this symbol as uploaded
            uniqueUploaded.insertMulti(groupResources, qMove(SymbolResourceEntry(symbol, resourceInGPU)));
        }
        if(anyUploadFailed)
            break;
    }

    // Shared
    QMultiHash< std::shared_ptr<GroupResources>, SymbolResourceEntry > sharedUploaded;
    for(const auto& groupResources : constOf(_referencedSharedGroupsResources))
    {
        if(groupResources->group->symbols.isEmpty())
            continue;

        // Basically, this check means "continue if shared resources where uploaded"
        if(!groupResources->resourcesInGPU.isEmpty())
            continue;

        for(const auto& symbol : constOf(groupResources->group->symbols))
        {
            // Prepare data and upload to GPU
            assert(static_cast<bool>(symbol->bitmap));
            std::shared_ptr<const GPUAPI::ResourceInGPU> resourceInGPU;
            ok = owner->uploadSymbolToGPU(symbol, resourceInGPU);

            // If upload have failed, stop
            if(!ok)
            {
                LogPrintf(LogSeverityLevel::Error, "Failed to upload unique symbol (size %dx%d) in %dx%d@%d tile",
                    symbol->bitmap->width(), symbol->bitmap->height(),
                    tileId.x, tileId.y, zoom);

                anyUploadFailed = true;
                break;
            }

            // Mark this symbol as uploaded
            sharedUploaded.insertMulti(groupResources, qMove(SymbolResourceEntry(symbol, resourceInGPU)));
        }
        if(anyUploadFailed)
            break;
    }

    // If at least one symbol failed to upload, consider entire tile as failed to upload,
    // and unload its partial GPU resources
    if(anyUploadFailed)
    {
        uniqueUploaded.clear();
        sharedUploaded.clear();

        return false;
    }

    // All resources have been uploaded to GPU successfully by this point

    // Unique
    for(const auto& entry : rangeOf(constOf(uniqueUploaded)))
    {
        const auto& groupResources = entry.key();
        auto& symbol = entry.value().first;
        auto& resource = entry.value().second;

        // Unload source data from symbol
        std::const_pointer_cast<MapSymbol>(symbol)->releaseNonRetainedData();

        // Publish symbol to global map
        owner->addMapSymbol(symbol, resource);

        // Move reference
        groupResources->resourcesInGPU.insert(qMove(symbol), qMove(resource));
    }

    // Shared
    for(const auto& entry : rangeOf(constOf(sharedUploaded)))
    {
        const auto& groupResources = entry.key();
        auto& symbol = entry.value().first;
        auto& resource = entry.value().second;

        // Unload source data from symbol
        std::const_pointer_cast<MapSymbol>(symbol)->releaseNonRetainedData();

        // Publish symbol to global map
        owner->addMapSymbol(symbol, resource);

        // Move reference
        groupResources->resourcesInGPU.insert(qMove(symbol), qMove(resource));
    }

    return true;
}

void OsmAnd::MapRendererResources::SymbolsTileResource::unloadFromGPU()
{
    const auto link_ = link.lock();
    const auto collection = static_cast<SymbolsResourcesCollection*>(&link_->collection);

    // Unique
    for(const auto& groupResources : constOf(_uniqueGroupsResources))
    {
        for(const auto& entryResourceInGPU : rangeOf(groupResources->resourcesInGPU))
        {
            const auto& symbol = entryResourceInGPU.key();
            auto& resourceInGPU = entryResourceInGPU.value();

            // Remove symbol from global map
            owner->removeMapSymbol(symbol);

            // Unload symbol from GPU
            assert(resourceInGPU.use_count() == 1);
            resourceInGPU.reset();
        }
    }
    _uniqueGroupsResources.clear();

    // Shared
    auto& sharedGroupsResources = collection->_sharedGroupsResources[zoom];
    for(auto& groupResources : _referencedSharedGroupsResources)
    {
        bool wasRemoved = false;
        auto groupResources_ = groupResources;
        sharedGroupsResources.releaseReference(groupResources->group->mapObject->id, groupResources_, true, &wasRemoved);

        // Skip removing symbols from global map in case this was not the last reference
        // to shared group resources
        if(!wasRemoved)
            continue;

        for(const auto& entryResourceInGPU : rangeOf(groupResources->resourcesInGPU))
        {
            const auto& symbol = entryResourceInGPU.key();
            auto& resourceInGPU = entryResourceInGPU.value();

            // Remove symbol from global map
            owner->removeMapSymbol(symbol);

            // Unload symbol from GPU
            assert(resourceInGPU.use_count() == 1);
            resourceInGPU.reset();
        }
    }
    _referencedSharedGroupsResources.clear();
}

bool OsmAnd::MapRendererResources::SymbolsTileResource::checkIsSafeToUnlink()
{
    return _referencedSharedGroupsResources.isEmpty();
}

void OsmAnd::MapRendererResources::SymbolsTileResource::detach()
{
    const auto link_ = link.lock();
    const auto collection = static_cast<SymbolsResourcesCollection*>(&link_->collection);

    _uniqueGroupsResources.clear();

    auto& sharedGroupsResources = collection->_sharedGroupsResources[zoom];
    for(auto& groupResources : _referencedSharedGroupsResources)
    {
        bool wasRemoved = false;
        auto groupResources_ = groupResources;
        sharedGroupsResources.releaseReference(groupResources->group->mapObject->id, groupResources_, true, &wasRemoved);

        // In case this was the last reference to shared group resources, check if any resources need to be deleted
        if(wasRemoved)
        {
            for(const auto& entryResourceInGPU : rangeOf(groupResources->resourcesInGPU))
            {
                const auto& symbol = entryResourceInGPU.key();
                auto& resourceInGPU = entryResourceInGPU.value();

                // Remove symbol from global map
                owner->removeMapSymbol(symbol);

                // Unload symbol from GPU thread (using dispatcher)
                assert(resourceInGPU.use_count() == 1);
                auto resourceInGPU_ = qMove(resourceInGPU);
                owner->renderer->getGpuThreadDispatcher().invokeAsync(
                    [resourceInGPU_]() mutable
                    {
                        assert(resourceInGPU_.use_count() == 1);
                        resourceInGPU_.reset();
                    });
#ifndef Q_COMPILER_RVALUE_REFS
                resourceInGPU.reset();
#endif //!Q_COMPILER_RVALUE_REFS
            }
        }
    }
    _referencedSharedGroupsResources.clear();
}

OsmAnd::MapRendererResources::SymbolsTileResource::GroupResources::GroupResources(const std::shared_ptr<const MapSymbolsGroup>& group_)
    : group(group_)
{
}

OsmAnd::MapRendererResources::SymbolsTileResource::GroupResources::~GroupResources()
{
}

OsmAnd::MapRendererResources::SymbolsResourcesCollection::SymbolsResourcesCollection()
: TiledResourcesCollection(ResourceType::Symbols)
{
}

OsmAnd::MapRendererResources::SymbolsResourcesCollection::~SymbolsResourcesCollection()
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
