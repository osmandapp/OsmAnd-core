#include "MapRendererResourcesManager.h"

#include <cassert>

#include "QtCommon.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include <SkImageDecoder.h>
#include "restore_internal_warnings.h"

#include "MapRenderer.h"
#include "IMapDataProvider.h"
#include "IMapLayerProvider.h"
#include "IMapElevationDataProvider.h"
#include "IRasterMapLayerProvider.h"
#include "MapSymbol.h"
#include "RasterMapSymbol.h"
#include "VectorMapSymbol.h"
#include "IMapTiledSymbolsProvider.h"
#include "IMapKeyedSymbolsProvider.h"
#include "BinaryMapObject.h"
#include "CoreResourcesEmbeddedBundle.h"
#include "FunctorQueryController.h"
#include "QKeyValueIterator.h"
#include "QCachingIterator.h"
#include "Utilities.h"
#include "Logging.h"

//#define OSMAND_LOG_RESOURCE_STATE_CHANGE 1
#ifndef OSMAND_LOG_RESOURCE_STATE_CHANGE
#   define OSMAND_LOG_RESOURCE_STATE_CHANGE 0
#endif // !defined(OSMAND_LOG_RESOURCE_STATE_CHANGE)

//#define OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER 1
#ifndef OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER
#   define OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER 0
#endif // !defined(OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER)

#if OSMAND_LOG_RESOURCE_STATE_CHANGE
#   define LOG_RESOURCE_STATE_CHANGE(resource, oldState, newState)                                                              \
    if (const auto tiledResource = std::dynamic_pointer_cast<const BaseTiledResource>(resource))                                \
    {                                                                                                                           \
        LogPrintf(LogSeverityLevel::Debug,                                                                                      \
            "Tile resource %p %dx%d@%d state change '" #oldState "'->'" #newState "' at " __FILE__ ":" QT_STRINGIFY(__LINE__),  \
            resource.get(), tiledResource.tileId.x, tiledResource.tileId.y, tiledResource.zoom);                                \
    }                                                                                                                           \
    else                                                                                                                        \
    {                                                                                                                           \
        LogPrintf(LogSeverityLevel::Debug,                                                                                      \
            "Resource %p state change '" #oldState "'->'" #newState "' at " __FILE__ ":" QT_STRINGIFY(__LINE__),                \
            resource.get());                                                                                                    \
    }
#else
#   define LOG_RESOURCE_STATE_CHANGE(resource, oldState, newState)
#endif

OsmAnd::MapRendererResourcesManager::MapRendererResourcesManager(MapRenderer* const owner_)
    : _taskHostBridge(this)
    , _workerThreadIsAlive(false)
    , _workerThreadId(nullptr)
    , _workerThread(new Concurrent::Thread(std::bind(&MapRendererResourcesManager::workerThreadProcedure, this)))
    , renderer(owner_)
    , processingTileStubs(_processingTileStubs)
    , unavailableTileStubs(_unavailableTileStubs)
{
#if OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER
    _resourcesRequestWorkersPool.setMaxThreadCount(1);
#endif //OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER
#if OSMAND_DEBUG
    LogPrintf(LogSeverityLevel::Info,
        "Map renderer will use max %d worker thread(s) to process requests",
        _resourcesRequestWorkersPool.maxThreadCount());
#endif

    // Initialize default resources
    initializeDefaultResources();

    // Start worker thread
    _workerThreadIsAlive = true;
    _workerThread->start();
}

OsmAnd::MapRendererResourcesManager::~MapRendererResourcesManager()
{
    // Check all resources are released
    for (auto& resourcesCollections : _storageByType)
    {
        for (const auto& resourcesCollection : constOf(resourcesCollections))
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
    bool ok = true;

    // Upload stubs
    ok = ok && initializeTileStub(
        QLatin1String("map/stubs/processing_tile_light.png"),
        _processingTileStubs[static_cast<int>(MapStubStyle::Light)]);
    ok = ok && initializeTileStub(
        QLatin1String("map/stubs/processing_tile_dark.png"),
        _processingTileStubs[static_cast<int>(MapStubStyle::Dark)]);
    ok = ok && initializeTileStub(
        QLatin1String("map/stubs/unavailable_tile_light.png"),
        _unavailableTileStubs[static_cast<int>(MapStubStyle::Light)]);
    ok = ok && initializeTileStub(
        QLatin1String("map/stubs/unavailable_tile_dark.png"),
        _unavailableTileStubs[static_cast<int>(MapStubStyle::Dark)]);

    return ok;
}

bool OsmAnd::MapRendererResourcesManager::initializeTileStub(
    const QString& resourceName,
    std::shared_ptr<const GPUAPI::ResourceInGPU>& outResource)
{
    const auto bitmap = getCoreResourcesProvider()->getResourceAsBitmap(
        resourceName,
        renderer->setupOptions.displayDensityFactor);
    if (!bitmap)
        return false;

    std::shared_ptr<const GPUAPI::ResourceInGPU> resource;
    std::shared_ptr<const IRasterMapLayerProvider::Data> bitmapTile(new IRasterMapLayerProvider::Data(
        TileId(),
        InvalidZoom,
        AlphaChannelPresence::Unknown,
        1.0f,
        bitmap));
    if (!uploadTiledDataToGPU(bitmapTile, resource))
        return false;
    
    outResource = resource;
    return true;
}

bool OsmAnd::MapRendererResourcesManager::releaseDefaultResources()
{
    // Release stubs
    for (auto& resource : _processingTileStubs)
        resource.reset();
    for (auto& resource : _processingTileStubs)
        resource.reset();

    return true;
}

bool OsmAnd::MapRendererResourcesManager::uploadTiledDataToGPU(
    const std::shared_ptr<const IMapTiledDataProvider::Data>& mapTile,
    std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU)
{
    return renderer->gpuAPI->uploadTiledDataToGPU(mapTile, outResourceInGPU);
}

bool OsmAnd::MapRendererResourcesManager::uploadSymbolToGPU(
    const std::shared_ptr<const MapSymbol>& mapSymbol,
    std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU)
{
    return renderer->gpuAPI->uploadSymbolToGPU(mapSymbol, outResourceInGPU);
}

std::shared_ptr<const SkBitmap> OsmAnd::MapRendererResourcesManager::adjustBitmapToConfiguration(
    const std::shared_ptr<const SkBitmap>& input,
    const AlphaChannelPresence alphaChannelPresence) const
{
    return renderer->adjustBitmapToConfiguration(input, alphaChannelPresence);
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

void OsmAnd::MapRendererResourcesManager::updateBindings(
    const MapRendererState& state,
    const MapRendererStateChanges updatedMask)
{
    bool wasLocked = false;

    if (updatedMask.isSet(MapRendererStateChange::ElevationData_Provider))
    {
        if (!wasLocked)
        {
            _resourcesStoragesLock.lockForWrite();
            wasLocked = true;
        }

        auto& bindings = _bindings[static_cast<int>(MapRendererResourceType::ElevationData)];
        auto& resources = _storageByType[static_cast<int>(MapRendererResourceType::ElevationData)];

        // Clean-up and unbind gone providers and their resources
        auto itBindedProvider = mutableIteratorOf(bindings.providersToCollections);
        while (itBindedProvider.hasNext())
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
            const std::shared_ptr< MapRendererTiledResourcesCollection > newResourcesCollection(
                new MapRendererTiledResourcesCollection(MapRendererResourceType::ElevationData));

            // Add binding
            bindings.providersToCollections.insert(state.elevationDataProvider, newResourcesCollection);
            bindings.collectionsToProviders.insert(newResourcesCollection, state.elevationDataProvider);

            // Add resources collection
            resources.push_back(qMove(newResourcesCollection));
        }
    }
    if (updatedMask.isSet(MapRendererStateChange::MapLayers_Providers))
    {
        if (!wasLocked)
        {
            _resourcesStoragesLock.lockForWrite();
            wasLocked = true;
        }

        auto& bindings = _bindings[static_cast<int>(MapRendererResourceType::MapLayer)];
        auto& resources = _storageByType[static_cast<int>(MapRendererResourceType::MapLayer)];

        // Clean-up and unbind gone providers and their resources
        auto itBindedProvider = mutableIteratorOf(bindings.providersToCollections);
        while (itBindedProvider.hasNext())
        {
            itBindedProvider.next();

            const auto provider = std::dynamic_pointer_cast<IMapLayerProvider>(itBindedProvider.key());

            // Skip binding if it's still active
            if (std::find(state.mapLayersProviders.cbegin(), state.mapLayersProviders.cend(), provider) != state.mapLayersProviders.cend())
                continue;

            // Clean-up resources (deferred)
            _pendingRemovalResourcesCollections.push_back(itBindedProvider.value());

            // Reset reference to resources collection, but keep the space in array
            qFind(resources.begin(), resources.end(), itBindedProvider.value())->reset();

            // Remove binding
            bindings.collectionsToProviders.remove(itBindedProvider.value());
            itBindedProvider.remove();
        }

        // Create new binding and storage
        for (const auto& provider_ : constOf(state.mapLayersProviders))
        {
            const auto provider = std::dynamic_pointer_cast<IMapDataProvider>(provider_);

            // If binding already exists, skip creation
            if (bindings.providersToCollections.contains(provider))
                continue;

            // Create new resources collection
            const std::shared_ptr< MapRendererTiledResourcesCollection > newResourcesCollection(
                new MapRendererTiledResourcesCollection(MapRendererResourceType::MapLayer));

            // Add binding
            bindings.providersToCollections.insert(provider, newResourcesCollection);
            bindings.collectionsToProviders.insert(newResourcesCollection, provider);

            // Add resources collection
            resources.push_back(qMove(newResourcesCollection));
        }
    }
    if (updatedMask.isSet(MapRendererStateChange::Symbols_Providers))
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
        while (itBindedProvider.hasNext())
        {
            itBindedProvider.next();

            const auto provider = itBindedProvider.key();

            // Skip binding if it's still active
            if (state.tiledSymbolsProviders.contains(std::dynamic_pointer_cast<IMapTiledSymbolsProvider>(provider)) ||
                state.keyedSymbolsProviders.contains(std::dynamic_pointer_cast<IMapKeyedSymbolsProvider>(provider)))
            {
                continue;
            }

            // Clean-up resources (deferred)
            _pendingRemovalResourcesCollections.push_back(itBindedProvider.value());

            // Remove resources collection
            resources.removeOne(itBindedProvider.value());

            // Remove binding
            bindings.collectionsToProviders.remove(itBindedProvider.value());
            itBindedProvider.remove();
        }

        // Create new binding and storage
        for (const auto& provider : constOf(state.tiledSymbolsProviders))
        {
            // If binding already exists, skip creation
            if (bindings.providersToCollections.contains(provider))
                continue;

            // Create new resources collection
            const std::shared_ptr< MapRendererBaseResourcesCollection > newResourcesCollection(
                static_cast<MapRendererBaseResourcesCollection*>(new MapRendererTiledSymbolsResourcesCollection()));

            // Add binding
            bindings.providersToCollections.insert(provider, newResourcesCollection);
            bindings.collectionsToProviders.insert(newResourcesCollection, provider);

            // Add resources collection
            resources.push_back(qMove(newResourcesCollection));
        }
        for (const auto& provider : constOf(state.keyedSymbolsProviders))
        {
            // If binding already exists, skip creation
            if (bindings.providersToCollections.contains(provider))
                continue;

            // Create new resources collection
            const std::shared_ptr< MapRendererBaseResourcesCollection > newResourcesCollection(
                static_cast<MapRendererBaseResourcesCollection*>(new MapRendererKeyedResourcesCollection(MapRendererResourceType::Symbols)));

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
    // Check if update needed
    bool update = true; //NOTE: So far this won't work, since resources won't be updated
    update = update || (_activeZoom != zoom);
    update = update || (_activeTiles != tiles);

    if (update)
    {
        // Lock worker wakeup mutex
        QMutexLocker scopedLocker(&_workerThreadWakeupMutex);

        // Update active zone
        _activeTiles = tiles;
        _activeZoom = zoom;

        // Wake up the worker
        _workerThreadWakeup.wakeAll();
    }
}

bool OsmAnd::MapRendererResourcesManager::obtainProviderFor(
    MapRendererBaseResourcesCollection* const resourcesRef,
    std::shared_ptr<IMapDataProvider>& provider) const
{
    assert(resourcesRef != nullptr);

    const auto& bindings = _bindings[static_cast<int>(resourcesRef->type)];
    for (const auto& bindingEntry : rangeOf(constOf(bindings.collectionsToProviders)))
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

bool OsmAnd::MapRendererResourcesManager::isDataSourceAvailableFor(
    const std::shared_ptr<MapRendererBaseResourcesCollection>& collection) const
{
    const auto& binding = _bindings[static_cast<int>(collection->type)];

    return binding.collectionsToProviders.contains(collection);
}

QList< std::shared_ptr<OsmAnd::MapRendererBaseResourcesCollection> >
OsmAnd::MapRendererResourcesManager::safeGetAllResourcesCollections() const
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

void OsmAnd::MapRendererResourcesManager::publishMapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& symbolGroup,
    const std::shared_ptr<const MapSymbol>& symbol,
    const std::shared_ptr<MapRendererBaseResource>& resource)
{
    renderer->publishMapSymbol(symbolGroup, symbol, resource);
}

void OsmAnd::MapRendererResourcesManager::unpublishMapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& symbolGroup,
    const std::shared_ptr<const MapSymbol>& symbol,
    const std::shared_ptr<MapRendererBaseResource>& resource)
{
    renderer->unpublishMapSymbol(symbolGroup, symbol, resource);
}

void OsmAnd::MapRendererResourcesManager::batchPublishMapSymbols(
    const QList< PublishOrUnpublishMapSymbol >& mapSymbolsToPublish)
{
    renderer->batchPublishMapSymbols(mapSymbolsToPublish);
}

void OsmAnd::MapRendererResourcesManager::batchUnpublishMapSymbols(
    const QList< PublishOrUnpublishMapSymbol >& mapSymbolsToUnublish)
{
    renderer->batchUnpublishMapSymbols(mapSymbolsToUnublish);
}

void OsmAnd::MapRendererResourcesManager::notifyNewResourceAvailableForDrawing()
{
    renderer->invalidateFrame();
}

void OsmAnd::MapRendererResourcesManager::workerThreadProcedure()
{
    // Capture worker thread ID
    _workerThreadId = QThread::currentThreadId();

    while (_workerThreadIsAlive)
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
    for (const auto& resourcesCollections : constOf(_storageByType))
    {
        for (const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!resourcesCollection)
                continue;

            // Skip resource types that do not have an available data source
            if (!isDataSourceAvailableFor(resourcesCollection))
                continue;

            if (const auto tiledResourcesCollection = std::dynamic_pointer_cast<MapRendererTiledResourcesCollection>(resourcesCollection))
                requestNeededTiledResources(tiledResourcesCollection, activeTiles, activeZoom);
            else if (const auto keyedResourcesCollection = std::dynamic_pointer_cast<MapRendererKeyedResourcesCollection>(resourcesCollection))
                requestNeededKeyedResources(keyedResourcesCollection);
        }
    }
}

void OsmAnd::MapRendererResourcesManager::requestNeededTiledResources(
    const std::shared_ptr<MapRendererTiledResourcesCollection>& resourcesCollection,
    const QSet<TileId>& activeTiles,
    const ZoomLevel activeZoom)
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
                if (resourceType == MapRendererResourceType::MapLayer)
                    return new MapRendererRasterMapLayerResource(this, collection, tileId, zoom);
                else if (resourceType == MapRendererResourceType::ElevationData)
                    return new MapRendererElevationDataResource(this, collection, tileId, zoom);
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
    const auto newMask = ~((std::numeric_limits<uint32_t>::max() >> MapRendererResourceTypesCount) << MapRendererResourceTypesCount);
    _invalidatedResourcesTypesMask.fetchAndStoreOrdered(newMask);

    renderer->invalidateFrame();
}

void OsmAnd::MapRendererResourcesManager::invalidateResourcesOfType(const MapRendererResourceType type)
{
    _invalidatedResourcesTypesMask.fetchAndOrOrdered(1u << static_cast<int>(type));

    renderer->invalidateFrame();
}

bool OsmAnd::MapRendererResourcesManager::validateResources()
{
    bool anyResourcesVadilated = false;

    unsigned int invalidatedResourcesTypesMask = _invalidatedResourcesTypesMask.fetchAndStoreOrdered(0);
    uint32_t typeIndex = 0;
    while (invalidatedResourcesTypesMask != 0 && typeIndex < MapRendererResourceTypesCount)
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
    for (const auto& resourcesCollection : constOf(resourcesCollections))
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

bool OsmAnd::MapRendererResourcesManager::updatesPresent() const
{
    bool updatesPresent = false;

    for (const auto& resourcesCollections : constOf(_storageByType))
    {
        for (const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!resourcesCollection)
                continue;
            const auto collectionSnapshot = resourcesCollection->getCollectionSnapshot();

            // Also check if keyed collection has same keys as respective provider
            if (const auto keyedResourcesCollection = std::dynamic_pointer_cast<MapRendererKeyedResourcesCollection::Snapshot>(collectionSnapshot))
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
            collectionSnapshot->forEachResourceExecute(
                [&updatesPresent]
                (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel)
                {
                    if (entry->updatesPresent())
                        updatesPresent = true;
                });
        }
    }

    return updatesPresent;
}

bool OsmAnd::MapRendererResourcesManager::checkForUpdatesAndApply() const
{
    bool updatesApplied = false;
    bool updatesPresent = false;

    for (const auto& resourcesCollections : constOf(_storageByType))
    {
        for (const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!resourcesCollection)
                continue;
            const auto collectionSnapshot = resourcesCollection->getCollectionSnapshot();

            // Also check if keyed collection has same keys as respective provider
            if (const auto keyedResourcesCollection = std::dynamic_pointer_cast<MapRendererKeyedResourcesCollection::Snapshot>(collectionSnapshot))
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
            collectionSnapshot->forEachResourceExecute(
                [&updatesApplied]
                (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel)
                {
                    if (entry->checkForUpdatesAndApply())
                        updatesApplied = true;
                });
        }
    }

    return (updatesApplied || updatesPresent);
}

void OsmAnd::MapRendererResourcesManager::updateResources(const QSet<TileId>& tiles, const ZoomLevel zoom)
{
    // Before requesting missing tiled resources, clean up cache to free some space
    if (!renderer->currentDebugSettings->disableJunkResourcesCleanup)
        cleanupJunkResources(tiles, zoom);

    // In the end of rendering processing, request tiled resources that are neither
    // present in requested list, nor in pending, nor in uploaded
    if (!renderer->currentDebugSettings->disableNeededResourcesRequests)
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
    for (const auto& resourcesCollections : constOf(_storageByType))
    {
        for (const auto& resourcesCollection : constOf(resourcesCollections))
        {
            // Skip empty entries
            if (!resourcesCollection)
                continue;

            const auto dataSourceAvailable = isDataSourceAvailableFor(resourcesCollection);

            // Regular checks common for all resources
            resourcesCollection->removeResources(
                [this, dataSourceAvailable, &needsResourcesUploadOrUnload]
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

                    // Skip cleaning if this resource is not junk
                    if (!isJunk)
                        return false;

                    // Mark this entry as junk until it will die
                    entry->markAsJunk();

                    return cleanupJunkResource(entry, needsResourcesUploadOrUnload);
                });

            // Some checks are only valid for tiled resources
            if (const auto tiledResourcesCollection = std::dynamic_pointer_cast<IMapRendererTiledResourcesCollection>(resourcesCollection))
            {
                resourcesCollection->removeResources(
                    [this, activeZoom, activeTiles, &needsResourcesUploadOrUnload]
                    (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
                    {
                        // If it was previously marked as junk, just leave it
                        if (entry->isJunk)
                            return false;

                        const auto tiledEntry = std::static_pointer_cast<MapRendererBaseTiledResource>(entry);

                        // Determine if resource is junk:
                        bool isJunk = false;
                        
                        // If this tiled entry is part of active zoom, it's treated as junk only if it's not a part
                        // of active tiles set
                        if (tiledEntry->zoom == activeZoom)
                            isJunk = isJunk || !activeTiles.contains(tiledEntry->tileId);

                        // If zoom delta from active zoom is larger than MapRenderer::MaxMissingDataZoomShift,
                        // it's impossible to use it anyways - thus it's junk.
                        const auto deltaZoom = static_cast<int>(tiledEntry->zoom) - static_cast<int>(activeZoom);
                        isJunk = isJunk || (qAbs(deltaZoom) > MapRenderer::MaxMissingDataZoomShift);

                        // Skip cleaning if this resource is not junk
                        if (!isJunk)
                            return false;

                        // Mark this entry as junk until it will die
                        entry->markAsJunk();

                        return cleanupJunkResource(entry, needsResourcesUploadOrUnload);
                    });

                // Remove all tiled resources that are not needed for "full coverage" of (activeTiles@ActiveZoom)
                QHash<ZoomLevel, QSet<TileId>> neededTilesMap;
                const auto isUsableResource = 
                    []
                    (const std::shared_ptr<MapRendererBaseTiledResource>& entry) -> bool
                    {
                        // Resources marked as junk are not usable
                        if (entry->isJunk)
                            return false;

                        // Only resources in GPU are usable
                        const auto state = entry->getState();
                        if (state != MapRendererResourceState::Uploaded && state != MapRendererResourceState::IsBeingUsed)
                            return false;

                        return true;
                    };
                for (const auto& activeTileId : constOf(activeTiles))
                {
                    // If resources have exact match for this tile, use only that
                    neededTilesMap[activeZoom].insert(activeTileId);
                    if (tiledResourcesCollection->containsResource(activeTileId, activeZoom, isUsableResource))
                        continue;

                    // Exact match was not found, so now try to look for overscaled/underscaled resources, taking into account
                    // MaxMissingDataZoomShift and active zoom. It's better to show Z-"nearest" resource available,
                    // giving preference to underscaled resource
                    for (int absZoomShift = 1; absZoomShift <= MapRenderer::MaxMissingDataZoomShift; absZoomShift++)
                    {
                        //TODO: Try to find underscaled first (that is, currentState.zoomBase + 1). Only full match is accepted

                        // If underscaled was not found, look for overscaled (surely, if such zoom level exists at all)
                        const auto overscaleZoom = static_cast<int>(activeZoom) - absZoomShift;
                        if (overscaleZoom >= static_cast<int>(MinZoomLevel))
                        {
                            const auto overscaledTileId = Utilities::getTileIdOverscaledByZoomShift(activeTileId, -absZoomShift);
                            neededTilesMap[static_cast<ZoomLevel>(overscaleZoom)].insert(overscaledTileId);
                            if (tiledResourcesCollection->containsResource(
                                    overscaledTileId,
                                    static_cast<ZoomLevel>(overscaleZoom),
                                    isUsableResource))
                            {
                                break;
                            }
                        }
                    }
                }
                resourcesCollection->removeResources(
                    [this, neededTilesMap, &needsResourcesUploadOrUnload]
                    (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
                    {
                        // If it's already marked as junk, keep & skip it
                        // (since if it has to be removed, it will be done during first-pass)
                        if (entry->isJunk)
                            return false;

                        const auto tiledEntry = std::static_pointer_cast<MapRendererBaseTiledResource>(entry);
                        
                        // Any tiled resource that is not contained in neededTilesMap is junk
                        const auto citNeededTilesAtZoom = neededTilesMap.constFind(tiledEntry->zoom);
                        if (citNeededTilesAtZoom != neededTilesMap.cend() && citNeededTilesAtZoom->contains(tiledEntry->tileId))
                            return false;

                        // Mark this entry as junk until it will die
                        entry->markAsJunk();

                        return cleanupJunkResource(entry, needsResourcesUploadOrUnload);
                    });
            }
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

void OsmAnd::MapRendererResourcesManager::blockingReleaseResourcesFrom(
    const std::shared_ptr<MapRendererBaseResourcesCollection>& collection,
    const bool gpuContextLost)
{
    // This method is called from non-GPU thread, so it's impossible to unload resources from GPU here.
    // So wait here until all resources will be unloaded from GPU
    bool needsResourcesUploadOrUnload = false;

    bool containedUnprocessableResources = false;
    do
    {
        containedUnprocessableResources = false;
        collection->removeResources(
            [&needsResourcesUploadOrUnload, &containedUnprocessableResources, gpuContextLost]
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

                    // In case context was lost, it's impossible to unload anything, so just remove the resource
                    if (gpuContextLost)
                    {
                        entry->lostDataInGPU();
                        return true;
                    }

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
                    // In case GPU context is lost, nothing can be done with this resource, so just remove it
                    if (gpuContextLost)
                    {
                        entry->lostDataInGPU();
                        return true;
                    }

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

        // If there are unprocessable resources and upload/upload is required
        if (containedUnprocessableResources && needsResourcesUploadOrUnload)
        {
            if (renderer->hasGpuWorkerThread())
            {
                // This method should not be called from GPU worker thread
                assert(!renderer->isInGpuWorkerThread());

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
            else
            {
                // If there's no GPU worker thread, this method has to be called from render thread
                assert(renderer->isInRenderThread());

                renderer->processGpuWorker();
            }
        }
    } while (containedUnprocessableResources);

    // Perform final request to upload or unload resources
    if (needsResourcesUploadOrUnload && !gpuContextLost)
    {
        if (renderer->hasGpuWorkerThread())
            requestResourcesUploadOrUnload();
        else
            renderer->processGpuWorker();
        needsResourcesUploadOrUnload = false;
    }

    // Check that collection is empty
    assert(collection->getResourcesCount() == 0);
}

void OsmAnd::MapRendererResourcesManager::requestResourcesUploadOrUnload()
{
    renderer->requestResourcesUploadOrUnload();
}

void OsmAnd::MapRendererResourcesManager::releaseAllResources(const bool gpuContextLost)
{
    // Release all resources
    for (const auto& resourcesCollections : _storageByType)
    {
        for (const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!resourcesCollection)
                continue;
            blockingReleaseResourcesFrom(resourcesCollection, gpuContextLost);
        }
    }
    for (const auto& resourcesCollection : _pendingRemovalResourcesCollections)
        blockingReleaseResourcesFrom(resourcesCollection, gpuContextLost);
    _pendingRemovalResourcesCollections.clear();

    // Release all bindings
    for (auto resourceType = 0; resourceType < MapRendererResourceTypesCount; resourceType++)
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

bool OsmAnd::MapRendererResourcesManager::updateCollectionsSnapshots() const
{
    bool allSuccessful = true;

    for (const auto& binding : constOf(_bindings))
    {
        for (const auto& collection : constOf(binding.providersToCollections))
        {
            if (!collection->updateCollectionSnapshot())
                allSuccessful = false;
        }
    }

    return allSuccessful;
}

bool OsmAnd::MapRendererResourcesManager::collectionsSnapshotsInvalidated() const
{
    bool atLeastOneInvalidated = false;

    for (const auto& binding : constOf(_bindings))
    {
        for (const auto& collection : constOf(binding.providersToCollections))
        {
            if (collection->collectionSnapshotInvalidated())
            {
                atLeastOneInvalidated = true;
                break;
            }
        }

        if (atLeastOneInvalidated)
            break;
    }

    return atLeastOneInvalidated;
}

std::shared_ptr<const OsmAnd::IMapRendererResourcesCollection> OsmAnd::MapRendererResourcesManager::getCollectionSnapshot(
    const MapRendererResourceType type,
    const std::shared_ptr<IMapDataProvider>& ofProvider) const
{
    const auto collection = getCollection(type, ofProvider);
    if (!collection)
        return nullptr;
    return getCollection(type, ofProvider)->getCollectionSnapshot();
}

bool OsmAnd::MapRendererResourcesManager::eachResourceIsUploadedOrUnavailable() const
{
    QReadLocker scopedLocker(&_resourcesStoragesLock);

    bool atLeastOneNotUploadedOrUnavailable = false;

    for (const auto& resourcesCollections : constOf(_storageByType))
    {
        for (const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!resourcesCollection)
                continue;

            resourcesCollection->obtainResources(nullptr,
                [&atLeastOneNotUploadedOrUnavailable]
                (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
                {
                    const auto resourceState = entry->getState();
                    if (resourceState != MapRendererResourceState::Uploaded && resourceState != MapRendererResourceState::Unavailable)
                    {
                        atLeastOneNotUploadedOrUnavailable = true;
                        cancel = true;
                        return false;
                    }

                    return false;
                });
        }
    }

    return !atLeastOneNotUploadedOrUnavailable;
}

bool OsmAnd::MapRendererResourcesManager::allResourcesAreUploaded() const
{
    QReadLocker scopedLocker(&_resourcesStoragesLock);

    bool atLeastOneNotUploaded = false;

    for (const auto& resourcesCollections : constOf(_storageByType))
    {
        for (const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!resourcesCollection)
                continue;

            resourcesCollection->obtainResources(nullptr,
                [&atLeastOneNotUploaded]
                (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
                {
                    if (entry->getState() != MapRendererResourceState::Uploaded)
                    {
                        atLeastOneNotUploaded = true;
                        cancel = true;
                        return false;
                    }

                    return false;
                });
        }
    }

    return !atLeastOneNotUploaded;
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
    for (const auto& resources_ : constOf(_storageByType[static_cast<int>(MapRendererResourceType::ElevationData)]))
    {
        const auto resources = std::dynamic_pointer_cast<const MapRendererTiledResourcesCollection>(resources_);
        if (!resources)
            continue;
        resources->obtainEntries(nullptr,
            [&dump, resourceStateMap]
            (const std::shared_ptr<MapRendererBaseTiledResource>& entry, bool& cancel) -> bool
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

    dump += QLatin1String("[Tiled] Map layer:\n");
    for (const auto& resources_ : constOf(_storageByType[static_cast<int>(MapRendererResourceType::MapLayer)]))
    {
        const auto resources = std::dynamic_pointer_cast<const MapRendererTiledResourcesCollection>(resources_);
        if (!resources)
            continue;
        resources->obtainEntries(nullptr,
            [&dump, resourceStateMap]
            (const std::shared_ptr<MapRendererBaseTiledResource>& entry, bool& cancel) -> bool
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
    for (const auto& resources_ : constOf(_storageByType[static_cast<int>(MapRendererResourceType::Symbols)]))
    {
        const auto resources = std::dynamic_pointer_cast<const MapRendererTiledResourcesCollection>(resources_);
        if (!resources)
            continue;
        resources->obtainEntries(nullptr,
            [&dump, resourceStateMap]
            (const std::shared_ptr<MapRendererBaseTiledResource>& entry, bool& cancel) -> bool
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
    const Concurrent::TaskHost::Bridge& bridge_,
    ExecuteSignature executeMethod_,
    PreExecuteSignature preExecuteMethod_ /*= nullptr*/,
    PostExecuteSignature postExecuteMethod_ /*= nullptr*/)
    : HostedTask(bridge_, executeMethod_, preExecuteMethod_, postExecuteMethod_)
    , manager(reinterpret_cast<const MapRendererResourcesManager*>(lockedOwner))
    , requestedResource(requestedResource_)
{
    manager->_resourcesRequestTasksCounter.fetchAndAddOrdered(1);
}

OsmAnd::MapRendererResourcesManager::ResourceRequestTask::~ResourceRequestTask()
{
    manager->_resourcesRequestTasksCounter.fetchAndSubOrdered(1);
}
