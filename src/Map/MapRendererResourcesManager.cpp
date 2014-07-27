#include "MapRendererResourcesManager.h"

#include <cassert>

#include "QtCommon.h"

#include "MapRenderer.h"
#include "IMapDataProvider.h"
#include "IMapKeyedDataProvider.h"
#include "IMapRasterBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "MapSymbol.h"
#include "RasterMapSymbol.h"
#include "VectorMapSymbol.h"
#include "IMapTiledSymbolsProvider.h"
#include "IMapKeyedSymbolsProvider.h"
#include "BinaryMapObject.h"
#include "EmbeddedResources.h"
#include "FunctorQueryController.h"
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

//#define OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE 1
#ifndef OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
#   define OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE 0
#endif // !defined(OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE)

#if OSMAND_LOG_RESOURCE_STATE_CHANGE
#   define LOG_RESOURCE_STATE_CHANGE(resource, oldState, newState)                                                                          \
    if (const auto tiledResource = std::dynamic_pointer_cast<const BaseTiledResource>(resource))                                            \
    {                                                                                                                                       \
        LogPrintf(LogSeverityLevel::Debug,                                                                                                  \
            "Tile resource %p %dx%d@%d state change '" #oldState "'->'" #newState "' at " __FILE__ ":" QT_STRINGIFY(__LINE__),              \
            resource.get(), tiledResource.tileId.x, tiledResource.tileId.y, tiledResource.zoom);                                            \
    }                                                                                                                                       \
    else                                                                                                                                    \
    {                                                                                                                                       \
        LogPrintf(LogSeverityLevel::Debug,                                                                                                  \
            "Resource %p state change '" #oldState "'->'" #newState "' at " __FILE__ ":" QT_STRINGIFY(__LINE__),                            \
            resource.get());                                                                                                                \
    }
#else
#   define LOG_RESOURCE_STATE_CHANGE(resource, oldState, newState)
#endif

OsmAnd::MapRendererResourcesManager::MapRendererResourcesManager(MapRenderer* const owner_)
    : _taskHostBridge(this)
    , _mapSymbolsInRegisterCount(0)
    , _invalidatedResourcesTypesMask(0)
    , _workerThreadIsAlive(false)
    , _workerThreadId(nullptr)
    , _workerThread(new Concurrent::Thread(std::bind(&MapRendererResourcesManager::workerThreadProcedure, this)))
    , renderer(owner_)
    , processingTileStub(_processingTileStub)
    , unavailableTileStub(_unavailableTileStub)
{
#if OSMAND_DEBUG
    LogPrintf(LogSeverityLevel::Info, "Map renderer will use max %d worker thread(s) to process requests", _resourcesRequestWorkersPool.maxThreadCount());
#endif
    // Raster resources collections are special, so preallocate them
    {
        auto& resources = _storageByType[static_cast<int>(MapRendererResourceType::RasterBitmapTile)];
        resources.reserve(RasterMapLayersCount);
        while(resources.size() < RasterMapLayersCount)
            resources.push_back(qMove(std::shared_ptr<MapRendererTiledResourcesCollection>()));
    }

    // Initialize default resources
    initializeDefaultResources();

    // Start worker thread
    _workerThreadIsAlive = true;
    _workerThread->start();
}

OsmAnd::MapRendererResourcesManager::~MapRendererResourcesManager()
{
    // Check all resources are released
    for(auto& resourcesCollections : _storageByType)
    {
        for(const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!resourcesCollection)
                continue;
            assert(resourcesCollection->getResourcesCount() == 0);
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

    // Release default resources
    releaseDefaultResources();

    // Wait for all tasks to complete
    _taskHostBridge.onOwnerIsBeingDestructed();
}

bool OsmAnd::MapRendererResourcesManager::initializeDefaultResources()
{
    // Upload stubs
    {
        const auto bitmap = EmbeddedResources::getBitmapResource(QLatin1String("map/stubs/processing_tile.png"));
        if (!bitmap)
            return false;
        else
        {
            std::shared_ptr<const MapTiledData> bitmapTile(new RasterBitmapTile(bitmap, AlphaChannelData::Undefined, 1.0f, TileId(), InvalidZoom));
            if (!uploadTileToGPU(bitmapTile, _processingTileStub))
                return false;
        }
    }
    {
        const auto bitmap = EmbeddedResources::getBitmapResource(QLatin1String("map/stubs/unavailable_tile.png"));
        if (!bitmap)
            return false;
        else
        {
            std::shared_ptr<const MapTiledData> bitmapTile(new RasterBitmapTile(bitmap, AlphaChannelData::Undefined, 1.0f, TileId(), InvalidZoom));
            if (!uploadTileToGPU(bitmapTile, _unavailableTileStub))
                return false;
        }
    }

    return true;
}

bool OsmAnd::MapRendererResourcesManager::releaseDefaultResources()
{
    // Release stubs
    _unavailableTileStub.reset();
    _processingTileStub.reset();

    return true;
}

bool OsmAnd::MapRendererResourcesManager::uploadTileToGPU(const std::shared_ptr<const MapTiledData>& mapTile, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU)
{
    return renderer->gpuAPI->uploadTileToGPU(mapTile, outResourceInGPU);
}

bool OsmAnd::MapRendererResourcesManager::uploadSymbolToGPU(const std::shared_ptr<const MapSymbol>& mapSymbol, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU)
{
    return renderer->gpuAPI->uploadSymbolToGPU(mapSymbol, outResourceInGPU);
}

std::shared_ptr<const SkBitmap> OsmAnd::MapRendererResourcesManager::adjustBitmapToConfiguration(const std::shared_ptr<const SkBitmap>& input, const AlphaChannelData alphaChannelData) const
{
    return renderer->adjustBitmapToConfiguration(input, alphaChannelData);
}

void OsmAnd::MapRendererResourcesManager::releaseGpuUploadableDataFrom(const std::shared_ptr<MapSymbol>& mapSymbol)
{
    if (const auto rasterMapSymbol = std::dynamic_pointer_cast<RasterMapSymbol>(mapSymbol))
    {
        rasterMapSymbol->bitmap.reset();
    }
    else if (const auto vectorMapSymbol = std::dynamic_pointer_cast<VectorMapSymbol>(mapSymbol))
    {
        vectorMapSymbol->releaseVerticesAndIndices();
    }
}

void OsmAnd::MapRendererResourcesManager::updateBindings(const MapRendererState& state, const uint32_t updatedMask)
{
    bool wasLocked = false;

    if (updatedMask & static_cast<uint32_t>(MapRendererStateChange::ElevationData_Provider))
    {
        if (!wasLocked)
        {
            _resourcesStoragesLock.lockForWrite();
            wasLocked = true;
        }

        auto& bindings = _bindings[static_cast<int>(MapRendererResourceType::ElevationDataTile)];
        auto& resources = _storageByType[static_cast<int>(MapRendererResourceType::ElevationDataTile)];

        // Clean-up and unbind gone providers and their resources
        auto itBindedProvider = mutableIteratorOf(bindings.providersToCollections);
        while(itBindedProvider.hasNext())
        {
            itBindedProvider.next();

            // Skip binding if it's still active
            if (itBindedProvider.key() == state.elevationDataProvider)
                continue;

            // Clean-up resources (deferred)
            _pendingRemovalResourcesCollections.push_back(itBindedProvider.value());

            // Remove resources collection
            resources.removeOne(itBindedProvider.value());

            // Remove binding
            bindings.collectionsToProviders.remove(itBindedProvider.value());
            itBindedProvider.remove();
        }

        // Create new binding and storage
        if (state.elevationDataProvider && !bindings.providersToCollections.contains(state.elevationDataProvider))
        {
            // Create new resources collection
            const std::shared_ptr< MapRendererTiledResourcesCollection > newResourcesCollection(new MapRendererTiledResourcesCollection(MapRendererResourceType::ElevationDataTile));

            // Add binding
            bindings.providersToCollections.insert(state.elevationDataProvider, newResourcesCollection);
            bindings.collectionsToProviders.insert(newResourcesCollection, state.elevationDataProvider);

            // Add resources collection
            resources.push_back(qMove(newResourcesCollection));
        }
    }
    if (updatedMask & static_cast<uint32_t>(MapRendererStateChange::RasterLayers_Providers))
    {
        if (!wasLocked)
        {
            _resourcesStoragesLock.lockForWrite();
            wasLocked = true;
        }

        auto& bindings = _bindings[static_cast<int>(MapRendererResourceType::RasterBitmapTile)];
        auto& resources = _storageByType[static_cast<int>(MapRendererResourceType::RasterBitmapTile)];

        // Clean-up and unbind gone providers and their resources
        auto itBindedProvider = mutableIteratorOf(bindings.providersToCollections);
        while(itBindedProvider.hasNext())
        {
            itBindedProvider.next();

            // Skip binding if it's still active
            if (std::find(state.rasterLayerProviders.cbegin(), state.rasterLayerProviders.cend(),
                std::static_pointer_cast<IMapRasterBitmapTileProvider>(itBindedProvider.key())) != state.rasterLayerProviders.cend())
            {
                continue;
            }

            // Clean-up resources (deferred)
            _pendingRemovalResourcesCollections.push_back(itBindedProvider.value());

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
            if (!provider)
                continue;

            // If binding already exists, skip creation
            if (bindings.providersToCollections.contains(std::static_pointer_cast<IMapDataProvider>(provider)))
                continue;

            // Create new resources collection
            const std::shared_ptr< MapRendererTiledResourcesCollection > newResourcesCollection(new MapRendererTiledResourcesCollection(MapRendererResourceType::RasterBitmapTile));

            // Add binding
            bindings.providersToCollections.insert(*itProvider, newResourcesCollection);
            bindings.collectionsToProviders.insert(newResourcesCollection, *itProvider);

            // Add resources collection
            assert(rasterLayerIdx < resources.size());
            assert(static_cast<bool>(resources[rasterLayerIdx]) == false);
            resources[rasterLayerIdx] = newResourcesCollection;
        }
    }
    if (updatedMask & static_cast<uint32_t>(MapRendererStateChange::Symbols_Providers))
    {
        if (!wasLocked)
        {
            _resourcesStoragesLock.lockForWrite();
            wasLocked = true;
        }

        auto& bindings = _bindings[static_cast<int>(MapRendererResourceType::Symbols)];
        auto& resources = _storageByType[static_cast<int>(MapRendererResourceType::Symbols)];

        // Clean-up and unbind gone providers and their resources
        auto itBindedProvider = mutableIteratorOf(bindings.providersToCollections);
        while(itBindedProvider.hasNext())
        {
            itBindedProvider.next();

            // Skip binding if it's still active
            if (state.symbolProviders.contains(itBindedProvider.key()))
                continue;

            // Clean-up resources (deferred)
            _pendingRemovalResourcesCollections.push_back(itBindedProvider.value());

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
            if (bindings.providersToCollections.contains(std::static_pointer_cast<IMapDataProvider>(provider)))
                continue;

            // Create new resources collection
            const std::shared_ptr< MapRendererBaseResourcesCollection > newResourcesCollection(
                (std::dynamic_pointer_cast<IMapTiledSymbolsProvider>(provider) != nullptr)
                ? static_cast<MapRendererBaseResourcesCollection*>(new MapRendererTiledSymbolsResourcesCollection())
                : static_cast<MapRendererBaseResourcesCollection*>(new MapRendererKeyedResourcesCollection(MapRendererResourceType::Symbols)));

            // Add binding
            bindings.providersToCollections.insert(provider, newResourcesCollection);
            bindings.collectionsToProviders.insert(newResourcesCollection, provider);

            // Add resources collection
            resources.push_back(qMove(newResourcesCollection));
        }
    }

    if (wasLocked)
        _resourcesStoragesLock.unlock();
}

void OsmAnd::MapRendererResourcesManager::updateActiveZone(const QSet<TileId>& tiles, const ZoomLevel zoom)
{
    // Lock worker wakeup mutex
    QMutexLocker scopedLocker(&_workerThreadWakeupMutex);

    // Update active zone
    _activeTiles = tiles;
    _activeZoom = zoom;

    // Wake up the worker
    _workerThreadWakeup.wakeAll();
}

bool OsmAnd::MapRendererResourcesManager::obtainProviderFor(MapRendererBaseResourcesCollection* const resourcesRef, std::shared_ptr<IMapDataProvider>& provider) const
{
    assert(resourcesRef != nullptr);

    const auto& bindings = _bindings[static_cast<int>(resourcesRef->type)];
    for(const auto& bindingEntry : rangeOf(constOf(bindings.collectionsToProviders)))
    {
        if (bindingEntry.key().get() != resourcesRef)
            continue;

        const auto& testProvider = bindingEntry.value();
        if (!testProvider)
            return false;

        provider = testProvider;
        return true;
    }

    return false;
}

bool OsmAnd::MapRendererResourcesManager::isDataSourceAvailableFor(const std::shared_ptr<MapRendererBaseResourcesCollection>& collection) const
{
    const auto& binding = _bindings[static_cast<int>(collection->type)];

    return binding.collectionsToProviders.contains(collection);
}

QList< std::shared_ptr<OsmAnd::MapRendererBaseResourcesCollection> > OsmAnd::MapRendererResourcesManager::safeGetAllResourcesCollections() const
{
    QReadLocker scopedLocker(&_resourcesStoragesLock);

    QList< std::shared_ptr<MapRendererBaseResourcesCollection> > result;

    // Add "pending-removal"
    result.append(_pendingRemovalResourcesCollections);

    // Add regular
    for (const auto& resourcesCollections : constOf(_storageByType))
    {
        for (const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!resourcesCollection)
                continue;

            result.push_back(resourcesCollection);
        }
    }

    return result;
}

void OsmAnd::MapRendererResourcesManager::registerMapSymbol(const std::shared_ptr<const MapSymbol>& symbol, const std::shared_ptr<MapRendererBaseResource>& resource)
{
    QMutexLocker scopedLocker(&_mapSymbolsRegistersMutex);

    auto& symbolReferencedResources = _mapSymbolsByOrderRegister[symbol->order][symbol];
    if (symbolReferencedResources.isEmpty())
        _mapSymbolsInRegisterCount++;
    symbolReferencedResources.push_back(resource);

#if OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
    LogPrintf(LogSeverityLevel::Debug,
        "Registered map symbol %p from %p (new total %d), now referenced from %d resources",
        symbol.get(),
        resource.get(),
        _mapSymbolsInRegisterCount,
        symbolReferencedResources.size());
#endif // OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
}

void OsmAnd::MapRendererResourcesManager::unregisterMapSymbol(const std::shared_ptr<const MapSymbol>& symbol, const std::shared_ptr<MapRendererBaseResource>& resource)
{
    QMutexLocker scopedLocker(&_mapSymbolsRegistersMutex);

    const auto itRegisterLayer = _mapSymbolsByOrderRegister.find(symbol->order);
    auto& registerLayer = *itRegisterLayer;

    const auto itSymbolReferencedResources = registerLayer.find(symbol);
    assert(itSymbolReferencedResources != registerLayer.cend());
    auto& symbolReferencedResources = *itSymbolReferencedResources;
    symbolReferencedResources.removeOne(resource);
#if OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
    const auto symbolReferencedResourcesSize = symbolReferencedResources.size();
#endif // OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
    if (symbolReferencedResources.isEmpty())
    {
        _mapSymbolsInRegisterCount--;
        registerLayer.erase(itSymbolReferencedResources);
    }
#if OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
    LogPrintf(LogSeverityLevel::Debug,
        "Unregistered map symbol %p from %p (new total %d), now referenced from %d resources",
        symbol.get(),
        resource.get(),
        _mapSymbolsInRegisterCount,
        symbolReferencedResourcesSize);
#endif // OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE

    // In case layer is empty, remove it entirely
    if (registerLayer.isEmpty())
        _mapSymbolsByOrderRegister.erase(itRegisterLayer);
}

void OsmAnd::MapRendererResourcesManager::notifyNewResourceAvailableForDrawing()
{
    renderer->invalidateFrame();
}

void OsmAnd::MapRendererResourcesManager::workerThreadProcedure()
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
        if (!_workerThreadIsAlive)
            break;

        // Update resources
        updateResources(activeTiles, activeZoom);
    }

    _workerThreadId = nullptr;
}

void OsmAnd::MapRendererResourcesManager::requestNeededResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom)
{
    for(const auto& resourcesCollections : constOf(_storageByType))
    {
        for(const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!resourcesCollection)
                continue;

            // Skip resource types that do not have an available data source
            if (!isDataSourceAvailableFor(resourcesCollection))
                continue;

            if (const auto resourcesCollection_ = std::dynamic_pointer_cast<MapRendererTiledResourcesCollection>(resourcesCollection))
                requestNeededTiledResources(resourcesCollection_, activeTiles, activeZoom);
            else if (const auto resourcesCollection_ = std::dynamic_pointer_cast<MapRendererKeyedResourcesCollection>(resourcesCollection))
                requestNeededKeyedResources(resourcesCollection_);
        }
    }
}

void OsmAnd::MapRendererResourcesManager::requestNeededTiledResources(const std::shared_ptr<MapRendererTiledResourcesCollection>& resourcesCollection, const QSet<TileId>& activeTiles, const ZoomLevel activeZoom)
{
    for (const auto& activeTileId : constOf(activeTiles))
    {
        // Obtain a resource entry and if it's state is "Unknown", create a task that will
        // request resource data
        std::shared_ptr<MapRendererBaseTiledResource> resource;
        const auto resourceType = resourcesCollection->type;
        resourcesCollection->obtainOrAllocateEntry(resource, activeTileId, activeZoom,
            [this, resourceType]
            (const TiledEntriesCollection<MapRendererBaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom) -> MapRendererBaseTiledResource*
            {
                if (resourceType == MapRendererResourceType::RasterBitmapTile)
                    return new MapRendererRasterBitmapTileResource(this, collection, tileId, zoom);
                else if(resourceType == MapRendererResourceType::ElevationDataTile)
                    return new MapRendererElevationDataTileResource(this, collection, tileId, zoom);
                else if (resourceType == MapRendererResourceType::Symbols)
                    return new MapRendererTiledSymbolsResource(this, collection, tileId, zoom);
                else
                    return nullptr;
            });

        requestNeededResource(resource);
    }
}

void OsmAnd::MapRendererResourcesManager::requestNeededKeyedResources(const std::shared_ptr<MapRendererKeyedResourcesCollection>& resourcesCollection)
{
    // Get keyed provider
    std::shared_ptr<IMapDataProvider> provider_;
    if (!obtainProviderFor(static_cast<MapRendererBaseResourcesCollection*>(resourcesCollection.get()), provider_))
        return;
    const auto& provider = std::dynamic_pointer_cast<IMapKeyedDataProvider>(provider_);
    if (!provider)
        return;

    // Get list of keys this provider has and check that all are present
    const auto& resourceKeys = provider->getProvidedDataKeys();
    for (const auto& resourceKey : constOf(resourceKeys))
    {
        // Obtain a resource entry and if it's state is "Unknown", create a task that will
        // request resource data
        std::shared_ptr<MapRendererBaseKeyedResource> resource;
        const auto resourceType = resourcesCollection->type;
        resourcesCollection->obtainOrAllocateEntry(resource, resourceKey,
            [this, resourceType]
            (const KeyedEntriesCollection<MapRendererKeyedResourcesCollection::Key, MapRendererBaseKeyedResource>& collection, MapRendererKeyedResourcesCollection::Key const key) -> MapRendererBaseKeyedResource*
            {
                if (resourceType == MapRendererResourceType::Symbols)
                    return new MapRendererKeyedSymbolsResource(this, collection, key);
                else
                    return nullptr;
            });

        requestNeededResource(resource);
    }
}

void OsmAnd::MapRendererResourcesManager::requestNeededResource(const std::shared_ptr<MapRendererBaseResource>& resource)
{
    // Only if tile entry has "Unknown" state proceed to "Requesting" state
    if (!resource->setStateIf(MapRendererResourceState::Unknown, MapRendererResourceState::Requesting))
        return;
    LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Unknown, MapRendererResourceState::Requesting);

    // Create async-task that will obtain needed resource data
    const auto executeProc = [this](Concurrent::Task* task_)
    {
        const auto task = static_cast<ResourceRequestTask*>(task_);
        const auto resource = std::static_pointer_cast<MapRendererBaseTiledResource>(task->requestedResource);

        // Only if resource entry has "Requested" state proceed to "ProcessingRequest" state
        if (!resource->setStateIf(MapRendererResourceState::Requested, MapRendererResourceState::ProcessingRequest))
        {
            // This actually can happen in following situation(s):
            //   - if request task was canceled before has started it's execution,
            //     since in that case a state change "Requested => JustBeforeDeath" must have happend.
            //     In this case entry will be removed in post-execute handler.
            assert(resource->getState() == MapRendererResourceState::JustBeforeDeath);
            return;
        }
        else
        {
            LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Requested, MapRendererResourceState::ProcessingRequest);
        }

        // Ask resource to obtain it's data
        bool dataAvailable = false;
        FunctorQueryController obtainDataQueryController(
            [task_]
            (const FunctorQueryController* const controller) -> bool
            {
                return task_->isCancellationRequested();
            });
        const auto requestSucceeded = resource->obtainData(dataAvailable, &obtainDataQueryController) && !task->isCancellationRequested();

        // If failed to obtain resource data, remove resource entry to repeat try later
        if (!requestSucceeded)
        {
            // It's safe to simply remove entry, since it's not yet uploaded
            if (const auto link = resource->link.lock())
                link->collection.removeEntry(resource->tileId, resource->zoom);
            return;
        }

        // Finalize execution of task
        if (!resource->setStateIf(MapRendererResourceState::ProcessingRequest, dataAvailable ? MapRendererResourceState::Ready : MapRendererResourceState::Unavailable))
        {
            assert(resource->getState() == MapRendererResourceState::RequestCanceledWhileBeingProcessed);

            // While request was processed, state may have changed to "RequestCanceledWhileBeingProcessed"
            task->requestCancellation();
            return;
        }
#if OSMAND_LOG_RESOURCE_STATE_CHANGE
        else
        {
            if (dataAvailable)
                LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::ProcessingRequest, MapRendererResourceState::Ready);
            else
                LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::ProcessingRequest, MapRendererResourceState::Unavailable);
        }
#endif // OSMAND_LOG_RESOURCE_STATE_CHANGE
        resource->_requestTask = nullptr;

        // There is data to upload to GPU, request uploading. Or just ask to show that resource is unavailable
        if (dataAvailable)
            requestResourcesUploadOrUnload();
        else
            notifyNewResourceAvailableForDrawing();
    };
    const auto postExecuteProc = [this](Concurrent::Task* task_, bool wasCancelled)
    {
        const auto task = static_cast<const ResourceRequestTask*>(task_);
        const auto resource = std::static_pointer_cast<MapRendererBaseTiledResource>(task->requestedResource);

        if (wasCancelled)
        {
            // If request task was canceled, if could have happened:
            //  - before it has started it's execution.
            //    In this case, state has to be "Requested", and it won't change. So just remove it
            //  - during it's execution.
            //    In this case, this handler will be called after execution was finished,
            //    and state _should_ be "Ready" or "Unavailable", but in general it can be any:
            //    Uploading, Uploaded, Unloading, Unloaded. In case state is "Ready" or "Unavailable",
            //    change it to "JustBeforeDeath" and delete it.
            if (
                resource->setStateIf(MapRendererResourceState::Requested, MapRendererResourceState::JustBeforeDeath) ||
                resource->setStateIf(MapRendererResourceState::Ready, MapRendererResourceState::JustBeforeDeath) ||
                resource->setStateIf(MapRendererResourceState::Unavailable, MapRendererResourceState::JustBeforeDeath) ||
                resource->setStateIf(MapRendererResourceState::RequestCanceledWhileBeingProcessed, MapRendererResourceState::JustBeforeDeath))
            {
                LOG_RESOURCE_STATE_CHANGE(resource, ? , MapRendererResourceState::JustBeforeDeath);

                task->requestedResource->removeSelfFromCollection();
            }

            // All other cases must be handled in other places, since here there is no access to GPU
        }
    };
    const auto asyncTask = new ResourceRequestTask(resource, _taskHostBridge, executeProc, nullptr, postExecuteProc);

    // Register tile as requested
    resource->_requestTask = asyncTask;
    assert(resource->getState() == MapRendererResourceState::Requesting);
    resource->setState(MapRendererResourceState::Requested);
    LOG_RESOURCE_STATE_CHANGE(resource, ? , MapRendererResourceState::Requested);

    // Finally start the request
    _resourcesRequestWorkersPool.start(asyncTask);
}

void OsmAnd::MapRendererResourcesManager::invalidateAllResources()
{
    QWriteLocker scopedLocker(&_invalidatedResourcesTypesMaskLock);

    _invalidatedResourcesTypesMask = ~((std::numeric_limits<uint32_t>::max() >> MapRendererResourceTypesCount) << MapRendererResourceTypesCount);

    renderer->invalidateFrame();
}

void OsmAnd::MapRendererResourcesManager::invalidateResourcesOfType(const MapRendererResourceType type)
{
    QWriteLocker scopedLocker(&_invalidatedResourcesTypesMaskLock);

    _invalidatedResourcesTypesMask |= 1u << static_cast<int>(type);

    renderer->invalidateFrame();
}

bool OsmAnd::MapRendererResourcesManager::validateResources()
{
    bool anyResourcesVadilated = false;

    uint32_t invalidatedResourcesTypesMask;
    {
        QReadLocker scopedLocker(&_invalidatedResourcesTypesMaskLock);

        invalidatedResourcesTypesMask = _invalidatedResourcesTypesMask;
        _invalidatedResourcesTypesMask = 0;
    }

    uint32_t typeIndex = 0;
    while(invalidatedResourcesTypesMask != 0 && typeIndex < MapRendererResourceTypesCount)
    {
        if (invalidatedResourcesTypesMask & 0x1)
        {
            if (validateResourcesOfType(static_cast<MapRendererResourceType>(typeIndex)))
                anyResourcesVadilated = true;
        }

        typeIndex++;
        invalidatedResourcesTypesMask >>= 1;
    }

    return anyResourcesVadilated;
}

bool OsmAnd::MapRendererResourcesManager::validateResourcesOfType(const MapRendererResourceType type)
{
    const auto& resourcesCollections = _storageByType[static_cast<int>(type)];
    const auto& bindings = _bindings[static_cast<int>(type)];

    // Notify owner
    renderer->onValidateResourcesOfType(type);

    bool atLeastOneMarked = false;
    for(const auto& resourcesCollection : constOf(resourcesCollections))
    {
        if (!bindings.collectionsToProviders.contains(resourcesCollection))
            continue;

        // Mark all resources as junk
        resourcesCollection->forEachResourceExecute(
            [&atLeastOneMarked]
            (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel)
            {
                entry->markAsJunk();
                atLeastOneMarked = true;
            });
    }

    return atLeastOneMarked;
}

bool OsmAnd::MapRendererResourcesManager::checkForUpdates() const
{
    bool updatesApplied = false;
    bool updatesPresent = false;

    for (const auto& resourcesCollections : constOf(_storageByType))
    {
        for (const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!resourcesCollection)
                continue;
            const auto shadowCollection = resourcesCollection->getShadowCollection();

            // Also check if keyed collection has same keys as respective provider
            if (const auto keyedResourcesCollection = std::dynamic_pointer_cast<MapRendererKeyedResourcesCollection::Shadow>(shadowCollection))
            {
                std::shared_ptr<IMapDataProvider> provider_;
                if (obtainProviderFor(resourcesCollection.get(), provider_))
                {
                    const auto provider = std::static_pointer_cast<IMapKeyedDataProvider>(provider_);

                    if (keyedResourcesCollection->getKeys().toSet() != provider->getProvidedDataKeys().toSet())
                        updatesPresent = true;
                }
            }

            // Check if any resource has applied updates
            shadowCollection->forEachResourceExecute(
                [&updatesApplied]
                (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel)
                {
                    if (entry->checkForUpdates())
                        updatesApplied = true;
                });
        }
    }

    return (updatesApplied || updatesPresent);
}

void OsmAnd::MapRendererResourcesManager::updateResources(const QSet<TileId>& tiles, const ZoomLevel zoom)
{
    // Before requesting missing tiled resources, clean up cache to free some space
    cleanupJunkResources(tiles, zoom);

    // In the end of rendering processing, request tiled resources that are neither
    // present in requested list, nor in pending, nor in uploaded
    requestNeededResources(tiles, zoom);
}

unsigned int OsmAnd::MapRendererResourcesManager::unloadResources()
{
    unsigned int totalUnloaded = 0u;

    const auto& resourcesCollections = safeGetAllResourcesCollections();
    for (const auto& resourcesCollection : constOf(resourcesCollections))
        unloadResourcesFrom(resourcesCollection, totalUnloaded);

    return totalUnloaded;
}

void OsmAnd::MapRendererResourcesManager::unloadResourcesFrom(
    const std::shared_ptr<MapRendererBaseResourcesCollection>& collection,
    unsigned int& totalUnloaded)
{
    // Select all resources with "UnloadPending" state
    QList< std::shared_ptr<MapRendererBaseResource> > resources;
    collection->obtainResources(&resources,
        []
        (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
        {
            // Skip not-"UnloadPending" resources
            if (entry->getState() != MapRendererResourceState::UnloadPending)
                return false;

            // Accept this resource
            return true;
        });
    if (resources.isEmpty())
        return;

    // Unload from GPU all selected resources
    for (const auto& resource : constOf(resources))
    {
        // Since state change is allowed (it's not changed to "Unloading" during query), check state here
        if (!resource->setStateIf(MapRendererResourceState::UnloadPending, MapRendererResourceState::Unloading))
            continue;
        LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::UnloadPending, MapRendererResourceState::Unloading);

        // Unload from GPU
        resource->unloadFromGPU();

        // Don't wait until GPU will execute unloading, since this resource won't be used anymore and will eventually be deleted

        // Mark as unloaded
        assert(resource->getState() == MapRendererResourceState::Unloading);
        resource->setState(MapRendererResourceState::Unloaded);
        LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Unloading, MapRendererResourceState::Unloaded);

        // Count uploaded resources
        totalUnloaded++;
    }
}

unsigned int OsmAnd::MapRendererResourcesManager::uploadResources(const unsigned int limit /*= 0u*/, bool* const outMoreThanLimitAvailable /*= nullptr*/)
{
    unsigned int totalUploaded = 0u;
    bool moreThanLimitAvailable = false;
    bool atLeastOneUploadFailed = false;

    const auto& resourcesCollections = safeGetAllResourcesCollections();
    for (const auto& resourcesCollection : constOf(resourcesCollections))
        uploadResourcesFrom(resourcesCollection, limit, totalUploaded, moreThanLimitAvailable, atLeastOneUploadFailed);

    // If any resource failed to upload, report that more ready resources are available
    if (atLeastOneUploadFailed)
        moreThanLimitAvailable = true;

    if (outMoreThanLimitAvailable)
        *outMoreThanLimitAvailable = moreThanLimitAvailable;
    return totalUploaded;
}

void OsmAnd::MapRendererResourcesManager::uploadResourcesFrom(
    const std::shared_ptr<MapRendererBaseResourcesCollection>& collection,
    const unsigned int limit,
    unsigned int& totalUploaded,
    bool& moreThanLimitAvailable,
    bool& atLeastOneUploadFailed)
{
    // Select all resources with "Ready" state
    QList< std::shared_ptr<MapRendererBaseResource> > resources;
    collection->obtainResources(&resources,
        [&resources, limit, totalUploaded, &moreThanLimitAvailable]
        (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
        {
            // Skip not-ready resources
            if (entry->getState() != MapRendererResourceState::Ready)
                return false;

            // Check if limit was reached
            if (limit > 0 && (totalUploaded + resources.size()) > limit)
            {
                // Tell that more resources are available for upload
                moreThanLimitAvailable = true;

                cancel = true;
                return false;
            }

            // Accept this resource
            return true;
        });
    if (resources.isEmpty())
        return;

    // Upload to GPU all selected resources
    for (const auto& resource : constOf(resources))
    {
        // Since state change is allowed (it's not changed to "Uploading" during query), check state here
        if (!resource->setStateIf(MapRendererResourceState::Ready, MapRendererResourceState::Uploading))
            continue;
        LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Ready, MapRendererResourceState::Uploading);

        // Actually upload resource to GPU
        const auto didUpload = resource->uploadToGPU();
        if (!atLeastOneUploadFailed && !didUpload)
            atLeastOneUploadFailed = true;
        if (!didUpload)
        {
            if (const auto tiledResource = std::dynamic_pointer_cast<const MapRendererBaseTiledResource>(resource))
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to upload tiled resource %p for %dx%d@%d to GPU",
                    resource.get(),
                    tiledResource->tileId.x, tiledResource->tileId.y, tiledResource->zoom);
            }
            else
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to upload resource %p to GPU",
                    resource.get());
            }
            continue;
        }

        // Before marking as uploaded, if uploading is done from GPU worker thread,
        // wait until operation completes
        if (renderer->setupOptions.gpuWorkerThreadEnabled)
            renderer->gpuAPI->waitUntilUploadIsComplete();

        // Mark as uploaded
        assert(resource->getState() == MapRendererResourceState::Uploading);
        resource->setState(MapRendererResourceState::Uploaded);
        LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Uploading, MapRendererResourceState::Uploaded);

        // Count uploaded resources
        totalUploaded++;
    }
}

void OsmAnd::MapRendererResourcesManager::cleanupJunkResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom)
{
    // This method is called from non-GPU thread, so it's impossible to unload resources from GPU here
    bool needsResourcesUploadOrUnload = false;

    // Release some resources from "pending-removal" collections
    auto itPendingRemovalCollection = mutableIteratorOf(_pendingRemovalResourcesCollections);
    while (itPendingRemovalCollection.hasNext())
    {
        const auto& pendingRemovalCollection = itPendingRemovalCollection.next();

        // Don't wait for anything. If impossible to process resource right now, just skip it
        pendingRemovalCollection->removeResources(
            [this, &needsResourcesUploadOrUnload]
            (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
            {
                return cleanupJunkResource(entry, needsResourcesUploadOrUnload);
            });

        // Remove empty collections completely
        if (pendingRemovalCollection->getResourcesCount() == 0)
            itPendingRemovalCollection.remove();
    }

    // Use aggressive cache cleaning: remove all resources that are not needed
    for(const auto& resourcesCollections : constOf(_storageByType))
    {
        for(const auto& resourcesCollection : constOf(resourcesCollections))
        {
            // Skip empty entries
            if (!resourcesCollection)
                continue;

            const auto dataSourceAvailable = isDataSourceAvailableFor(resourcesCollection);

            resourcesCollection->removeResources(
                [this, dataSourceAvailable, activeTiles, activeZoom, &needsResourcesUploadOrUnload]
                (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
                {
                    // Resource with "Unloaded" state is junk, regardless if it's needed or not
                    if (entry->setStateIf(MapRendererResourceState::Unloaded, MapRendererResourceState::JustBeforeDeath))
                    {
                        LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::Unloaded, MapRendererResourceState::JustBeforeDeath);

                        // If resource was unloaded from GPU, remove the entry.
                        return true;
                    }

                    // Determine if resource is junk:
                    bool isJunk = false;

                    // If it was previously marked as junk
                    isJunk = isJunk || entry->isJunk;

                    // If data source is gone, all resources from it are considered junk:
                    isJunk = isJunk || !dataSourceAvailable;

                    // If resource is not in set of "needed tiles", it's junk:
                    if (const auto tiledEntry = std::dynamic_pointer_cast<MapRendererBaseTiledResource>(entry))
                        isJunk = isJunk || !(activeTiles.contains(tiledEntry->tileId) && tiledEntry->zoom == activeZoom);

                    // Skip cleaning if this resource is not junk
                    if (!isJunk)
                        return false;

                    // Mark this entry as junk until it will die
                    entry->markAsJunk();

                    return cleanupJunkResource(entry, needsResourcesUploadOrUnload);
                });
        }
    }

    if (needsResourcesUploadOrUnload)
        requestResourcesUploadOrUnload();
}

bool OsmAnd::MapRendererResourcesManager::cleanupJunkResource(const std::shared_ptr<MapRendererBaseResource>& resource, bool& needsResourcesUploadOrUnload)
{
    if (resource->setStateIf(MapRendererResourceState::Unloaded, MapRendererResourceState::JustBeforeDeath))
    {
        LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::Unloaded, MapRendererResourceState::JustBeforeDeath);

        // If resource was unloaded from GPU, remove the entry.
        return true;
    }
    else if (resource->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::UnloadPending))
    {
        LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::Uploaded, MapRendererResourceState::UnloadPending);

        // If resource is not needed anymore, change its state to "UnloadPending",
        // but keep the resource entry, since it must be unload from GPU in another place

        needsResourcesUploadOrUnload = true;
        return false;
    }
    else if (resource->setStateIf(MapRendererResourceState::Ready, MapRendererResourceState::JustBeforeDeath))
    {
        LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::Ready, MapRendererResourceState::JustBeforeDeath);

        // If resource was not yet uploaded, just remove it.
        return true;
    }
    else if (resource->setStateIf(MapRendererResourceState::ProcessingRequest, MapRendererResourceState::RequestCanceledWhileBeingProcessed))
    {
        LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::ProcessingRequest, MapRendererResourceState::RequestCanceledWhileBeingProcessed);

        // If resource request is being processed, keep the entry until processing is complete.

        return false;
    }
    else if (resource->setStateIf(MapRendererResourceState::Requested, MapRendererResourceState::JustBeforeDeath))
    {
        LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::Requested, MapRendererResourceState::JustBeforeDeath);

        // If resource was just requested, cancel its task and remove the entry.

        // Cancel the task
        assert(resource->_requestTask != nullptr);
        resource->_requestTask->requestCancellation();

        return true;
    }
    else if (resource->setStateIf(MapRendererResourceState::Unavailable, MapRendererResourceState::JustBeforeDeath))
    {
        LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::Unavailable, MapRendererResourceState::JustBeforeDeath);

        // If resource was never available, just remove the entry
        return true;
    }

    const auto state = resource->getState();
    if (state == MapRendererResourceState::JustBeforeDeath)
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
}

void OsmAnd::MapRendererResourcesManager::blockingReleaseResourcesFrom(const std::shared_ptr<MapRendererBaseResourcesCollection>& collection)
{
    // This method is called from non-GPU thread, so it's impossible to unload resources from GPU here.
    // So wait here until all resources will be unloaded from GPU
    bool needsResourcesUploadOrUnload = false;

    bool containedUnprocessableResources = false;
    do
    {
        containedUnprocessableResources = false;
        collection->removeResources(
            [&needsResourcesUploadOrUnload, &containedUnprocessableResources]
            (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
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

                if (entry->setStateIf(MapRendererResourceState::Requested, MapRendererResourceState::JustBeforeDeath))
                {
                    LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::Requested, MapRendererResourceState::JustBeforeDeath);

                    // Cancel the task
                    assert(entry->_requestTask != nullptr);
                    entry->_requestTask->requestCancellation();

                    return true;
                }
                else if (entry->setStateIf(MapRendererResourceState::ProcessingRequest, MapRendererResourceState::RequestCanceledWhileBeingProcessed))
                {
                    LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::ProcessingRequest, MapRendererResourceState::RequestCanceledWhileBeingProcessed);

                    containedUnprocessableResources = true;

                    return false;
                }
                else if (entry->setStateIf(MapRendererResourceState::Ready, MapRendererResourceState::JustBeforeDeath))
                {
                    LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::Ready, MapRendererResourceState::JustBeforeDeath);

                    return true;
                }
                else if (entry->setStateIf(MapRendererResourceState::Unloaded, MapRendererResourceState::JustBeforeDeath))
                {
                    LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::Unloaded, MapRendererResourceState::JustBeforeDeath);

                    return true;
                }
                else if (entry->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::UnloadPending))
                {
                    LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::Uploaded, MapRendererResourceState::UnloadPending);

                    // If resource is not needed anymore, change its state to "UnloadPending",
                    // but keep the resource entry, since it must be unload from GPU in another place
                    needsResourcesUploadOrUnload = true;
                    containedUnprocessableResources = true;

                    return false;
                }
                else if (entry->setStateIf(MapRendererResourceState::Unavailable, MapRendererResourceState::JustBeforeDeath))
                {
                    LOG_RESOURCE_STATE_CHANGE(entry, MapRendererResourceState::Unavailable, MapRendererResourceState::JustBeforeDeath);

                    return true;
                }

                const auto state = entry->getState();
                if (state == MapRendererResourceState::JustBeforeDeath)
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
                if (state == MapRendererResourceState::Uploading ||
                    state == MapRendererResourceState::UnloadPending ||
                    state == MapRendererResourceState::Unloading)
                {
                    needsResourcesUploadOrUnload = true;
                }

                if (const auto tiledEntry = std::dynamic_pointer_cast<const MapRendererBaseTiledResource>(entry))
                {
                    LogPrintf(LogSeverityLevel::Debug,
                        "Tile resource %p %dx%d@%d has state %d which can not be processed, need to wait",
                        entry.get(),
                        tiledEntry->tileId.x,
                        tiledEntry->tileId.y,
                        tiledEntry->zoom,
                        static_cast<int>(state));
                }
                else
                {
                    LogPrintf(LogSeverityLevel::Debug,
                        "Resource %p has state %d which can not be processed, need to wait",
                        entry.get(),
                        static_cast<int>(state));
                }
                
                containedUnprocessableResources = true;
                return false;
            });

            // If there are unprocessable resources and
            if (containedUnprocessableResources && needsResourcesUploadOrUnload)
            {
                QWaitCondition gpuResourcesSyncStageExecutedOnceCondition;
                QMutex gpuResourcesSyncStageExecutedOnceMutex;

                // Dispatcher always runs after GPU resources sync stage
                renderer->getGpuThreadDispatcher().invokeAsync(
                    [&gpuResourcesSyncStageExecutedOnceCondition, &gpuResourcesSyncStageExecutedOnceMutex]
                    ()
                    {
                        QMutexLocker scopedLocker(&gpuResourcesSyncStageExecutedOnceMutex);
                        gpuResourcesSyncStageExecutedOnceCondition.wakeAll();
                    });

                requestResourcesUploadOrUnload();
                needsResourcesUploadOrUnload = false;

                // Wait up to 250ms for GPU resources sync stage to complete
                {
                    QMutexLocker scopedLocker(&gpuResourcesSyncStageExecutedOnceMutex);
                    gpuResourcesSyncStageExecutedOnceCondition.wait(&gpuResourcesSyncStageExecutedOnceMutex, 250);
                }
            }
    } while(containedUnprocessableResources);

    // Perform final request to upload or unload resources
    if (needsResourcesUploadOrUnload)
    {
        requestResourcesUploadOrUnload();
        needsResourcesUploadOrUnload = false;
    }

    // Check that collection is empty
    assert(collection->getResourcesCount() == 0);
}

void OsmAnd::MapRendererResourcesManager::requestResourcesUploadOrUnload()
{
    renderer->requestResourcesUploadOrUnload();
}

void OsmAnd::MapRendererResourcesManager::releaseAllResources()
{
    // Release all resources
    for(const auto& resourcesCollections : _storageByType)
    {
        for(const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!resourcesCollection)
                continue;
            blockingReleaseResourcesFrom(resourcesCollection);
        }
    }
    for (const auto& resourcesCollection : _pendingRemovalResourcesCollections)
        blockingReleaseResourcesFrom(resourcesCollection);
    _pendingRemovalResourcesCollections.clear();

    // Release all bindings
    for(auto resourceType = 0; resourceType < MapRendererResourceTypesCount; resourceType++)
    {
        auto& bindings = _bindings[resourceType];

        bindings.providersToCollections.clear();
        bindings.collectionsToProviders.clear();
    }
}

void OsmAnd::MapRendererResourcesManager::syncResourcesInGPU(
    const unsigned int limitUploads /*= 0u*/,
    bool* const outMoreUploadsThanLimitAvailable /*= nullptr*/,
    unsigned int* const outResourcesUploaded /*= nullptr*/,
    unsigned int* const outResourcesUnloaded /*= nullptr*/)
{
    // Unload resources
    const auto resourcesUnloaded = unloadResources();
    if (outResourcesUnloaded)
        *outResourcesUnloaded = resourcesUnloaded;

    // Upload resources
    const auto resourcesUploaded = uploadResources(limitUploads, outMoreUploadsThanLimitAvailable);
    if (outResourcesUploaded)
        *outResourcesUploaded = resourcesUploaded;
}

std::shared_ptr<const OsmAnd::MapRendererBaseResourcesCollection> OsmAnd::MapRendererResourcesManager::getCollection(
    const MapRendererResourceType type,
    const std::shared_ptr<IMapDataProvider>& provider) const
{
    return _bindings[static_cast<int>(type)].providersToCollections[provider];
}

bool OsmAnd::MapRendererResourcesManager::updateShadowCollections() const
{
    bool allSuccessful = true;

    for (const auto& binding : constOf(_bindings))
    {
        for (const auto& collection : constOf(binding.providersToCollections))
        {
            if (!collection->updateShadowCollection())
                allSuccessful = false;
        }
    }

    return allSuccessful;
}

std::shared_ptr<const OsmAnd::IMapRendererResourcesCollection> OsmAnd::MapRendererResourcesManager::getShadowCollection(
    const MapRendererResourceType type,
    const std::shared_ptr<IMapDataProvider>& ofProvider) const
{
    return getCollection(type, ofProvider)->getShadowCollection();
}

QMutex& OsmAnd::MapRendererResourcesManager::getMapSymbolsRegistersMutex() const
{
    return _mapSymbolsRegistersMutex;
}

const OsmAnd::MapRendererResourcesManager::MapSymbolsByOrderRegister& OsmAnd::MapRendererResourcesManager::getMapSymbolsByOrderRegister() const
{
    return _mapSymbolsByOrderRegister;
}

unsigned int OsmAnd::MapRendererResourcesManager::getMapSymbolsInRegisterCount() const
{
    return _mapSymbolsInRegisterCount;
}

void OsmAnd::MapRendererResourcesManager::dumpResourcesInfo() const
{
    QMap<MapRendererResourceState, QString> resourceStateMap;
    resourceStateMap.insert(MapRendererResourceState::Unknown, QLatin1String("Unknown"));
    resourceStateMap.insert(MapRendererResourceState::Requesting, QLatin1String("Requesting"));
    resourceStateMap.insert(MapRendererResourceState::Requested, QLatin1String("Requested"));
    resourceStateMap.insert(MapRendererResourceState::ProcessingRequest, QLatin1String("ProcessingRequest"));
    resourceStateMap.insert(MapRendererResourceState::RequestCanceledWhileBeingProcessed, QLatin1String("RequestCanceledWhileBeingProcessed"));
    resourceStateMap.insert(MapRendererResourceState::Unavailable, QLatin1String("Unavailable"));
    resourceStateMap.insert(MapRendererResourceState::Ready, QLatin1String("Ready"));
    resourceStateMap.insert(MapRendererResourceState::Uploading, QLatin1String("Uploading"));
    resourceStateMap.insert(MapRendererResourceState::Uploaded, QLatin1String("Uploaded"));
    resourceStateMap.insert(MapRendererResourceState::IsBeingUsed, QLatin1String("IsBeingUsed"));
    resourceStateMap.insert(MapRendererResourceState::UnloadPending, QLatin1String("UnloadPending"));
    resourceStateMap.insert(MapRendererResourceState::Unloading, QLatin1String("Unloading"));
    resourceStateMap.insert(MapRendererResourceState::Unloaded, QLatin1String("Unloaded"));
    resourceStateMap.insert(MapRendererResourceState::JustBeforeDeath, QLatin1String("JustBeforeDeath"));

    QString dump;
    dump += QLatin1String("Resources:\n");
    dump += QLatin1String("--------------------------------------------------------------------------------\n");

    dump += QLatin1String("[Tiled] Elevation data:\n");
    for(const auto& resources_ : constOf(_storageByType[static_cast<int>(MapRendererResourceType::ElevationDataTile)]))
    {
        if (!resources_)
            continue;
        const auto resources = std::dynamic_pointer_cast<const MapRendererTiledResourcesCollection>(resources_);
        if (!resources)
            continue;
        resources->obtainEntries(nullptr, [&dump, resourceStateMap](const std::shared_ptr<MapRendererBaseTiledResource>& entry, bool& cancel) -> bool
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
    for(const auto& resources_ : constOf(_storageByType[static_cast<int>(MapRendererResourceType::RasterBitmapTile)]))
    {
        if (!resources_)
            continue;
        const auto resources = std::dynamic_pointer_cast<const MapRendererTiledResourcesCollection>(resources_);
        if (!resources)
            continue;
        resources->obtainEntries(nullptr, [&dump, resourceStateMap](const std::shared_ptr<MapRendererBaseTiledResource>& entry, bool& cancel) -> bool
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
    for(const auto& resources_ : constOf(_storageByType[static_cast<int>(MapRendererResourceType::Symbols)]))
    {
        if (!resources_)
            continue;
        const auto resources = std::dynamic_pointer_cast<const MapRendererTiledResourcesCollection>(resources_);
        if (!resources)
            continue;
        resources->obtainEntries(nullptr, [&dump, resourceStateMap](const std::shared_ptr<MapRendererBaseTiledResource>& entry, bool& cancel) -> bool
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

OsmAnd::MapRendererResourcesManager::ResourceRequestTask::ResourceRequestTask(
    const std::shared_ptr<MapRendererBaseResource>& requestedResource_,
    const Concurrent::TaskHost::Bridge& bridge, ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod /*= nullptr*/, PostExecuteSignature postExecuteMethod /*= nullptr*/)
    : HostedTask(bridge, executeMethod, preExecuteMethod, postExecuteMethod)
    , requestedResource(requestedResource_)
{
}

OsmAnd::MapRendererResourcesManager::ResourceRequestTask::~ResourceRequestTask()
{
}
