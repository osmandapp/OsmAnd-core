#include "MapRendererResourcesManager.h"

#include <cassert>

#include "QtCommon.h"

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
#include "SimpleQueryController.h"
#include "QConditionalReadLocker.h"
#include "QConditionalWriteLocker.h"
#include "QConditionalMutexLocker.h"
#include "Utilities.h"
#include "SkiaUtilities.h"
#include "Logging.h"

//#define OSMAND_LOG_RESOURCE_STATE_CHANGE 1
#ifndef OSMAND_LOG_RESOURCE_STATE_CHANGE
#   define OSMAND_LOG_RESOURCE_STATE_CHANGE 0
#endif // !defined(OSMAND_LOG_RESOURCE_STATE_CHANGE)

//#define OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER 1
#ifndef OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER
#   define OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER 0
#endif // !defined(OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER)

#define FORCE_LOG_RESOURCE_STATE_CHANGE(resource, oldState, newState)                                                           \
    if (const auto tiledResource = std::dynamic_pointer_cast<const MapRendererBaseTiledResource>(resource))                     \
    {                                                                                                                           \
        LogPrintf(LogSeverityLevel::Debug,                                                                                      \
            "Tile resource %p %dx%d@%d state change '" #oldState "'->'" #newState "' at " __FILE__ ":" QT_STRINGIFY(__LINE__),  \
            resource.get(), tiledResource->tileId.x, tiledResource->tileId.y, tiledResource->zoom);                             \
    }                                                                                                                           \
    else                                                                                                                        \
    {                                                                                                                           \
        LogPrintf(LogSeverityLevel::Debug,                                                                                      \
            "Resource %p state change '" #oldState "'->'" #newState "' at " __FILE__ ":" QT_STRINGIFY(__LINE__),                \
            resource.get());                                                                                                    \
    }

#if OSMAND_LOG_RESOURCE_STATE_CHANGE
#   define LOG_RESOURCE_STATE_CHANGE(resource, oldState, newState) FORCE_LOG_RESOURCE_STATE_CHANGE(resource, oldState, newState)
#else
#   define LOG_RESOURCE_STATE_CHANGE(resource, oldState, newState)
#endif

OsmAnd::MapRendererResourcesManager::MapRendererResourcesManager(MapRenderer* const owner_)
    : _taskHostBridge(this)
    , _resourcesRequestWorkerPool(Concurrent::WorkerPool::Order::LIFO)
    , _workerThreadIsAlive(false)
    , _workerThreadId(nullptr)
    , _workerThread(new Concurrent::Thread(std::bind(&MapRendererResourcesManager::workerThreadProcedure, this)))
    , renderer(owner_)
    , processingTileStubs(_processingTileStubs)
    , unavailableTileStubs(_unavailableTileStubs)
{
    renderer->resourcesAreInUse.lock();
    resetResourceWorkerThreadsLimit();

    _requestedResourcesTasks.reserve(1024);

    _threadCountTime = std::chrono::high_resolution_clock::now();
    _threadCount = 0.0f;
    _threadCountPeriod = 0.0f;
    _basicThreadsCPULoad = 0.0f;

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

    // Wait for all tasks to complete
    _taskHostBridge.onOwnerIsBeingDestructed();
    renderer->resourcesAreInUse.unlock();
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
    ok = ok && initializeEmptyStub(
        _processingTileStubs[static_cast<int>(MapStubStyle::Empty)]);
    ok = ok && initializeTileStub(
        QLatin1String("map/stubs/unavailable_tile_light.png"),
        _unavailableTileStubs[static_cast<int>(MapStubStyle::Light)]);
    ok = ok && initializeTileStub(
        QLatin1String("map/stubs/unavailable_tile_dark.png"),
        _unavailableTileStubs[static_cast<int>(MapStubStyle::Dark)]);
    ok = ok && initializeEmptyStub(
        _unavailableTileStubs[static_cast<int>(MapStubStyle::Empty)]);

    return ok;
}

bool OsmAnd::MapRendererResourcesManager::initializeTileStub(
    const QString& resourceName,
    std::shared_ptr<const GPUAPI::ResourceInGPU>& outResource)
{
    const auto image = getCoreResourcesProvider()->getResourceAsImage(
        resourceName,
        renderer->setupOptions.displayDensityFactor);
    if (!image)
        return false;

    std::shared_ptr<const GPUAPI::ResourceInGPU> resource;
    std::shared_ptr<const IRasterMapLayerProvider::Data> tile(new IRasterMapLayerProvider::Data(
        TileId::zero(),
        InvalidZoomLevel,
        AlphaChannelPresence::Unknown,
        1.0f,
        image));
    if (!uploadTiledDataToGPU(tile, resource))
        return false;

    outResource = resource;
    return true;
}

bool OsmAnd::MapRendererResourcesManager::initializeEmptyStub(
    std::shared_ptr<const GPUAPI::ResourceInGPU>& outResource)
{
    const auto image = SkiaUtilities::getEmptyImage();

    if (!image)
        return false;

    std::shared_ptr<const GPUAPI::ResourceInGPU> resource;
    std::shared_ptr<const IRasterMapLayerProvider::Data> tile(new IRasterMapLayerProvider::Data(
        TileId::zero(),
        InvalidZoomLevel,
        AlphaChannelPresence::Present,
        1.0f,
        image));
    if (!uploadTiledDataToGPU(tile, resource))
        return false;

    outResource = resource;
    return true;
}

bool OsmAnd::MapRendererResourcesManager::releaseDefaultResources(bool gpuContextLost)
{
    // Release stubs
    for (auto& resource : _processingTileStubs)
    {
        if (gpuContextLost)
            resource->lostRefInGPU();
        resource.reset();
    }
    for (auto& resource : _unavailableTileStubs)
    {
        if (gpuContextLost)
            resource->lostRefInGPU();
        resource.reset();
    }

    return true;
}

bool OsmAnd::MapRendererResourcesManager::uploadTiledDataToGPU(
    const std::shared_ptr<const IMapTiledDataProvider::Data>& mapTile,
    std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU,
    const std::shared_ptr<MapRendererBaseResource>& resource /*= nullptr*/)
{
    QReadLocker scopedLocker(&_tileDateTimeLock);

    bool ok = renderer->gpuAPI->uploadTiledDataToGPU(
        mapTile,
        outResourceInGPU,
        renderer->setupOptions.gpuWorkerThreadEnabled,
        &(renderer->gpuContextIsLost),
        _tileDateTime,
        resource);
    return ok;
}

bool OsmAnd::MapRendererResourcesManager::uploadSymbolToGPU(
    const std::shared_ptr<const MapSymbol>& mapSymbol,
    std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU)
{
    return renderer->gpuAPI->uploadSymbolToGPU(
        mapSymbol,
        outResourceInGPU,
        renderer->setupOptions.gpuWorkerThreadEnabled,
        &(renderer->gpuContextIsLost));
}

bool OsmAnd::MapRendererResourcesManager::adjustImageToConfiguration(
    const sk_sp<const SkImage>& input,
    sk_sp<SkImage>& output,
    const AlphaChannelPresence alphaChannelPresence) const
{
    return renderer->adjustImageToConfiguration(input, output, alphaChannelPresence);
}

sk_sp<const SkImage> OsmAnd::MapRendererResourcesManager::adjustImageToConfiguration(
    const sk_sp<const SkImage>& input,
    const AlphaChannelPresence alphaChannelPresence) const
{
    return renderer->adjustImageToConfiguration(input, alphaChannelPresence);
}

void OsmAnd::MapRendererResourcesManager::releaseGpuUploadableDataFrom(const std::shared_ptr<MapSymbol>& mapSymbol)
{
    if (const auto rasterMapSymbol = std::dynamic_pointer_cast<RasterMapSymbol>(mapSymbol))
    {
        rasterMapSymbol->image.reset();
    }
    else if (const auto vectorMapSymbol = std::dynamic_pointer_cast<VectorMapSymbol>(mapSymbol))
    {
        vectorMapSymbol->releaseVerticesAndIndices();
    }
}

bool OsmAnd::MapRendererResourcesManager::updateBindingsAndTime(
    const MapRendererState& state, const MapRendererStateChanges updatedMask)
{
    assert(renderer->isInRenderThread());

    bool wasLocked = false;

    if (updatedMask.isSet(MapRendererStateChange::Elevation_DataProvider))
    {
        if (!wasLocked)
        {
            if (!_resourcesStoragesLock.tryLockForWrite())
                return false;
            wasLocked = true;
        }

        updateElevationDataProviderBindings(state);
    }

    if (updatedMask.isSet(MapRendererStateChange::MapLayers_Providers))
    {
        if (!wasLocked)
        {
            if (!_resourcesStoragesLock.tryLockForWrite())
                return false;
            wasLocked = true;
        }

        updateMapLayerProviderBindings(state);
    }

    if (updatedMask.isSet(MapRendererStateChange::Symbols_Providers))
    {
        if (!wasLocked)
        {
            if (!_resourcesStoragesLock.tryLockForWrite())
                return false;
            wasLocked = true;
        }

        updateSymbolProviderBindings(state);
    }

    if (wasLocked)
        _resourcesStoragesLock.unlock();

    auto isPeriodChanged = updatedMask.isSet(MapRendererStateChange::TimePeriod);
    auto newState = isPeriodChanged ? MapRendererResourceState::Outdated : MapRendererResourceState::PreparedRenew;
    if (isPeriodChanged || updatedMask.isSet(MapRendererStateChange::DateTime))
    {
        {
            QWriteLocker scopedLocker(&_tileDateTimeLock);

            _tileDateTime = state.dateTime;
        }

        // Mark tile resources as needed to reupload in accordance to changed tile time
        const auto mapLayerType = static_cast<int>(MapRendererResourceType::MapLayer);
        const auto& resourcesCollections = _storageByType[mapLayerType];
        const auto& bindings = _bindings[mapLayerType];

        bool atLeastOneMarked = false;
        bool needsResourcesUploadOrUnload = false;
        for (const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!bindings.collectionsToProviders.contains(resourcesCollection))
                continue;

            resourcesCollection->forEachResourceExecute(
                [&atLeastOneMarked, &needsResourcesUploadOrUnload, isPeriodChanged, newState]
                (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel)
                {
                    if (auto resource = std::dynamic_pointer_cast<MapRendererRasterMapLayerResource>(entry))
                    {
                        bool withData = resource->_sourceData && resource->_sourceData->images.size() > 1;
                        auto state = resource->getState();
                        if (!isPeriodChanged && (state == MapRendererResourceState::Uploading
                            || state == MapRendererResourceState::Renewing))
                        {
                            resource->markAsOldTime();
                        }
                        if (withData && resource->setStateIf(MapRendererResourceState::Uploaded, newState))
                        {
                            atLeastOneMarked = true;
                            LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Uploaded, newState);
                        }
                        else if (withData && isPeriodChanged
                            && resource->setStateIf(MapRendererResourceState::PreparedRenew, newState))
                            LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::PreparedRenew, newState);
                        else if (isPeriodChanged && (state == MapRendererResourceState::ProcessingRequest
                            || state == MapRendererResourceState::ProcessingUpdate
                            || state == MapRendererResourceState::ProcessingUpdateWhileRenewing
                            || state == MapRendererResourceState::Unavailable))
                        {
                            resource->markAsJunk();
                        }
                        else if (withData && isPeriodChanged
                            && (state == MapRendererResourceState::Ready
                            || state == MapRendererResourceState::Uploading
                            || state == MapRendererResourceState::Uploaded
                            || state == MapRendererResourceState::IsBeingUsed
                            || state == MapRendererResourceState::PreparedRenew
                            || state == MapRendererResourceState::Renewing))
                        {
                            resource->markAsJunk();
                        }
                    }
                });
        }
        if (isPeriodChanged)
            renderer->invalidateFrame();
        else if (atLeastOneMarked)
            requestResourcesUploadOrUnload();

    }

    return true;
}

void OsmAnd::MapRendererResourcesManager::updateElevationDataProviderBindings(const MapRendererState& state)
{
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

void OsmAnd::MapRendererResourcesManager::updateMapLayerProviderBindings(const MapRendererState& state)
{
    auto& bindings = _bindings[static_cast<int>(MapRendererResourceType::MapLayer)];
    auto& resources = _storageByType[static_cast<int>(MapRendererResourceType::MapLayer)];

    // Clean-up and unbind gone providers and their resources
    auto itBindedProvider = mutableIteratorOf(bindings.providersToCollections);
    while (itBindedProvider.hasNext())
    {
        itBindedProvider.next();

        const auto provider = std::dynamic_pointer_cast<IMapLayerProvider>(itBindedProvider.key());
        if (!provider)
            continue;

        // Skip binding if it's still active
        if (std::contains(state.mapLayersProviders, provider))
            continue;

        // Clean-up resources (deferred)
        _pendingRemovalResourcesCollections.push_back(itBindedProvider.value());

        // Reset reference to resources collection, but keep the space in array
        std::find(resources, itBindedProvider.value())->reset();

        // Remove binding
        bindings.collectionsToProviders.remove(itBindedProvider.value());
        itBindedProvider.remove();
    }

    // Create new binding and storage
    for (const auto& provider_ : constOf(state.mapLayersProviders))
    {
        const auto provider = std::static_pointer_cast<IMapDataProvider>(provider_);

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

void OsmAnd::MapRendererResourcesManager::updateSymbolProviderBindings(const MapRendererState& state)
{
    auto& bindings = _bindings[static_cast<int>(MapRendererResourceType::Symbols)];
    auto& resources = _storageByType[static_cast<int>(MapRendererResourceType::Symbols)];

    // Clean-up and unbind gone providers and their resources
    auto itBindedProvider = mutableIteratorOf(bindings.providersToCollections);
    while (itBindedProvider.hasNext())
    {
        itBindedProvider.next();

        const auto provider = itBindedProvider.key();

        // Skip binding if it's still active
        bool skip = false;
        for (const auto& tiledSymbolsSubsection : constOf(state.tiledSymbolsProviders))
        {
            if (tiledSymbolsSubsection.contains(std::dynamic_pointer_cast<IMapTiledSymbolsProvider>(provider)))
            {
                skip = true;
                break;
            }
        }
        if (skip)
            continue;
        for (const auto& keyedSymbolsSubsection : constOf(state.keyedSymbolsProviders))
        {
            if (keyedSymbolsSubsection.contains(std::dynamic_pointer_cast<IMapKeyedSymbolsProvider>(provider)))
            {
                skip = true;
                break;
            }
        }
        if (skip)
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
    for (const auto& tiledSymbolsSubsection : constOf(state.tiledSymbolsProviders))
    {
        for (const auto& provider : constOf(tiledSymbolsSubsection))
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
    }
    for (const auto& keyedSymbolsSubsection : constOf(state.keyedSymbolsProviders))
    {
        for (const auto& provider : constOf(keyedSymbolsSubsection))
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
}

void OsmAnd::MapRendererResourcesManager::updateActiveZone(
    QMap<ZoomLevel, QVector<TileId>>& activeTiles, QMap<ZoomLevel, TileId>& activeTilesTargets,
    QMap<ZoomLevel, QVector<TileId>>& visibleTiles, QSet<TileId>& extraDetailedTiles,
    int zoomLevelOffset, int visibleTilesCount)
{
    // Check if update needed
    bool update = true; //NOTE: So far this won't work, since resources won't be updated
    update = update || (_activeTiles != activeTiles) || (_activeTilesTargets != activeTilesTargets)
        || (_visibleTiles != visibleTiles)
        || (_extraDetailedTiles != extraDetailedTiles)
        || (_zoomLevelOffset != zoomLevelOffset)
        || (_visibleTilesCount != visibleTilesCount);

    if (update)
    {
        // Lock worker wakeup mutex
        QMutexLocker scopedLocker(&_workerThreadWakeupMutex);

        // Update active zone
        _zoomLevelOffset = zoomLevelOffset;
        _visibleTilesCount = visibleTilesCount;
        _visibleTiles = visibleTiles;
        _activeTiles = activeTiles;
        _activeTilesTargets = activeTilesTargets;
        _extraDetailedTiles = extraDetailedTiles;

        // Wake up the worker
        _workerThreadWakeup.wakeAll();
    }
}

void OsmAnd::MapRendererResourcesManager::setResourceWorkerThreadsLimit(const unsigned int limit)
{
    _resourcesRequestWorkerPool.setMaxThreadCount(limit);

    if (_resourcesRequestWorkerPool.maxThreadCount() > 0)
    {
        LogPrintf(LogSeverityLevel::Verbose,
            "Map renderer will use %d concurrent worker(s) to process requests",
            _resourcesRequestWorkerPool.maxThreadCount());
    }
    else
    {
        LogPrintf(LogSeverityLevel::Verbose,
            "Map renderer will use unlimited number of concurrent workers to process requests");
    }
}

void OsmAnd::MapRendererResourcesManager::resetResourceWorkerThreadsLimit()
{
#if OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER
    _resourcesRequestWorkerPool.setMaxThreadCount(1);
#else // !OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER
    _resourcesRequestWorkerPool.setMaxThreadCount(QThread::idealThreadCount());
#endif // OSMAND_SINGLE_MAP_RENDERER_RESOURCES_WORKER

    if (_resourcesRequestWorkerPool.maxThreadCount() > 0)
    {
        LogPrintf(LogSeverityLevel::Verbose,
            "Map renderer will use %d concurrent worker(s) to process requests",
            _resourcesRequestWorkerPool.maxThreadCount());
    }
    else
    {
        LogPrintf(LogSeverityLevel::Verbose,
            "Map renderer will use unlimited number of concurrent workers to process requests");
    }
}

void OsmAnd::MapRendererResourcesManager::reportActiveThread(const float delta)
{
    QMutexLocker scopedLocker(&_threadCountLock);

    const auto maxThreadCount = static_cast<float>(_resourcesRequestWorkerPool.maxThreadCount());
    if (maxThreadCount > 0.0f)
    {
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const auto timePeriod = std::chrono::duration<float>(currentTime - _threadCountTime).count();
        _basicThreadsCPULoad += timePeriod * _threadCount / maxThreadCount;
        _threadCount += delta;
        _threadCountPeriod += timePeriod;
        _threadCountTime = currentTime;
    }
}

bool OsmAnd::MapRendererResourcesManager::obtainProviderFor(
    MapRendererBaseResourcesCollection* const resourcesRef,
    std::shared_ptr<IMapDataProvider>& provider) const
{
    QConditionalReadLocker scopedLocker(&_resourcesStoragesLock, !renderer->isInRenderThread());

    return noLockObtainProviderFor(resourcesRef, provider);
}

bool OsmAnd::MapRendererResourcesManager::noLockObtainProviderFor(
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
    assert(QThread::currentThreadId() != renderer->_renderThreadId);

    const auto& binding = _bindings[static_cast<int>(collection->type)];

    return binding.collectionsToProviders.contains(collection);
}

QList< std::shared_ptr<OsmAnd::MapRendererBaseResourcesCollection> >
OsmAnd::MapRendererResourcesManager::safeGetAllResourcesCollections() const
{
    QConditionalReadLocker scopedLocker(&_resourcesStoragesLock, !renderer->isInRenderThread());

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

void OsmAnd::MapRendererResourcesManager::safeGetAllResourcesCollections(
    QList< std::shared_ptr<MapRendererBaseResourcesCollection> >& outPendingRemovalResourcesCollections,
    QList< std::shared_ptr<MapRendererBaseResourcesCollection> >& outOtherResourcesCollections) const
{
    QConditionalReadLocker scopedLocker(&_resourcesStoragesLock, !renderer->isInRenderThread());

    outPendingRemovalResourcesCollections = detachedOf(_pendingRemovalResourcesCollections);

    for (const auto& resourcesCollections : constOf(_storageByType))
    {
        for (const auto& resourcesCollection : constOf(resourcesCollections))
        {
            if (!resourcesCollection)
                continue;

            outOtherResourcesCollections.push_back(resourcesCollection);
        }
    }
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

void OsmAnd::MapRendererResourcesManager::workerThreadProcedure()
{
    // Capture worker thread ID
    _workerThreadId = QThread::currentThreadId();

    while (_workerThreadIsAlive)
    {
        // Local copy of active zone
        int zoomLevelOffset;
        int visibleTilesCount;
        QMap<ZoomLevel, QVector<TileId>> visibleTiles;
        QMap<ZoomLevel, QVector<TileId>> activeTiles;
        QMap<ZoomLevel, TileId> activeTilesTargets;
        QSet<TileId> extraDetailedTiles;

        // Wait until we're unblocked by host
        {
            QMutexLocker scopedLocker(&_workerThreadWakeupMutex);
            REPEAT_UNTIL(_workerThreadWakeup.wait(&_workerThreadWakeupMutex));

            // Copy active zone to local copy
            zoomLevelOffset = _zoomLevelOffset;
            visibleTilesCount = _visibleTilesCount;
            visibleTiles = _visibleTiles;
            activeTiles = _activeTiles;
            activeTilesTargets = _activeTilesTargets;
            extraDetailedTiles = _extraDetailedTiles;
        }
        if (!_workerThreadIsAlive)
            break;

        // Update resources
        updateResources(activeTiles, activeTilesTargets, visibleTiles, extraDetailedTiles, zoomLevelOffset);
    }

    _workerThreadId = nullptr;
}

void OsmAnd::MapRendererResourcesManager::requestOutdatedResources(
    const QList< std::shared_ptr<MapRendererBaseResourcesCollection> >& resourcesCollections)
{
    for (const auto& resourcesCollection : constOf(resourcesCollections))
    {
        if (!resourcesCollection)
            continue;

        if (resourcesCollection->type != MapRendererResourceType::MapLayer)
            continue;

        if (const auto tiledResourcesCollection =
                std::dynamic_pointer_cast<MapRendererTiledResourcesCollection>(resourcesCollection))
        {
            std::shared_ptr<IMapDataProvider> mapDataProvider;
            if (!obtainProviderFor(resourcesCollection.get(), mapDataProvider))
                continue;

            // Select all resources with "Outdated" state
            QList< std::shared_ptr<MapRendererBaseResource> > resources;
            resourcesCollection->obtainResources(&resources,
                []
                (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
                {
                    // Skip not-"Outdated" resources
                    if (entry->getState() != MapRendererResourceState::Outdated)
                        return false;

                    // Accept this resource
                    return true;
                });
            if (resources.isEmpty())
                continue;

            // Request up-to-date data for each outdated resource
            for (const auto& resource : constOf(resources))
            {
                if (resource->setStateIf(MapRendererResourceState::Outdated, MapRendererResourceState::Updating))
                {
                    requestNeededResource(resource);
                    LOG_RESOURCE_STATE_CHANGE(
                        resource, MapRendererResourceState::Outdated, MapRendererResourceState::Updating);
                }
            }
        }
    }
}

void OsmAnd::MapRendererResourcesManager::requestNeededResources(
    const QList< std::shared_ptr<MapRendererBaseResourcesCollection> >& resourcesCollections,
    const TileId centerTileId,
    const QVector<TileId>& activeTiles,
    const ZoomLevel activeZoom,
    const ZoomLevel currentZoom,
    const QVector<TileId>& visibleTiles,
    const QSet<TileId>& extraDetailedTiles,
    const int zoomLevelOffset)
{
    _requestedResourcesTasks.resize(0);
    for (const auto& resourcesCollection : constOf(resourcesCollections))
    {
        if (!resourcesCollection)
            continue;

        if (resourcesCollection->type != MapRendererResourceType::ElevationData)
        {
            requestNeededResources(resourcesCollection, centerTileId, visibleTiles, extraDetailedTiles,
                activeZoom, currentZoom, zoomLevelOffset);
        }
        else
        {
            requestNeededResources(resourcesCollection, centerTileId, activeTiles, extraDetailedTiles,
                activeZoom, currentZoom, zoomLevelOffset);
        }
    }

    _resourcesRequestWorkerPool.enqueue(
        _requestedResourcesTasks,
        [centerTileId, activeTiles, activeZoom]
        (QRunnable* const l_, QRunnable* const r_) -> bool
        {
            const auto l = static_cast<ResourceRequestTask*>(l_);
            const auto r = static_cast<ResourceRequestTask*>(r_);

            const auto lPriority = l->calculatePriority(centerTileId, activeTiles, activeZoom);
            const auto rPriority = r->calculatePriority(centerTileId, activeTiles, activeZoom);

            return lPriority < rPriority;
        });
}

void OsmAnd::MapRendererResourcesManager::requestNeededResources(
    const std::shared_ptr<MapRendererBaseResourcesCollection>& resourcesCollection,
    const TileId centerTileId,
    const QVector<TileId>& activeTiles,
    const QSet<TileId>& extraDetailedTiles,
    const ZoomLevel activeZoom,
    const ZoomLevel currentZoom,
    const int zoomLevelOffset)
{
    // Skip resource types that do not have an available data source
    std::shared_ptr<IMapDataProvider> mapDataProvider;
    if (!obtainProviderFor(resourcesCollection.get(), mapDataProvider))
        return;

    if (const auto tiledResourcesCollection =
            std::dynamic_pointer_cast<MapRendererTiledResourcesCollection>(resourcesCollection))
    {
        // Symbols only for high detail tiles are meant to be displayed
        if (tiledResourcesCollection->type != MapRendererResourceType::Symbols || activeZoom == currentZoom)
            requestNeededTiledResources(
                tiledResourcesCollection,
                centerTileId,
                activeTiles,
                extraDetailedTiles,
                activeZoom,
                currentZoom,
                zoomLevelOffset);
    }
    else if (const auto keyedResourcesCollection =
            std::dynamic_pointer_cast<MapRendererKeyedResourcesCollection>(resourcesCollection))
    {
        if (activeZoom == currentZoom)
            requestNeededKeyedResources(
                keyedResourcesCollection,
                activeZoom);
    }
}

void OsmAnd::MapRendererResourcesManager::requestNeededTiledResources(
    const std::shared_ptr<MapRendererTiledResourcesCollection>& resourcesCollection,
    const TileId centerTileId,
    const QVector<TileId>& activeTiles,
    const QSet<TileId>& extraDetailedTiles,
    const ZoomLevel activeZoom,
    const ZoomLevel currentZoom,
    const int zoomLevelOffset)
{
    const auto resourceType = resourcesCollection->type;
    const auto resourceAllocator =
        [this, resourceType]
        (const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
            const TileId tileId,
            const ZoomLevel zoom) -> MapRendererBaseTiledResource*
        {
            if (resourceType == MapRendererResourceType::MapLayer)
                return new MapRendererRasterMapLayerResource(this, collection, tileId, zoom);
            else if (resourceType == MapRendererResourceType::ElevationData)
                return new MapRendererElevationDataResource(this, collection, tileId, zoom);
            else if (resourceType == MapRendererResourceType::Symbols)
                return new MapRendererTiledSymbolsResource(this, collection, tileId, zoom);
            else
                return nullptr;
        };

    auto minZoom = MinZoomLevel;
    auto maxZoom = MaxZoomLevel;

    auto minVisibleZoom = MinZoomLevel;
    auto maxVisibleZoom = MaxZoomLevel;

    int maxMissingDataZoomShift = MapRenderer::MaxMissingDataZoomShift;
    int maxMissingDataUnderZoomShift = MapRenderer::MaxMissingDataUnderZoomShift;

    bool isMapLayer = resourceType == MapRendererResourceType::MapLayer;
    bool isElevationData = resourceType == MapRendererResourceType::ElevationData;
    bool isSymbolData = resourceType == MapRendererResourceType::Symbols;

    std::shared_ptr<IMapDataProvider> provider_;
    obtainProviderFor(static_cast<MapRendererBaseResourcesCollection*>(resourcesCollection.get()), provider_);
    const auto& tiledProvider = std::dynamic_pointer_cast<IMapTiledDataProvider>(provider_);

    if (isMapLayer || isElevationData)
    {
        if (tiledProvider)
        {
            minZoom = Utilities::clipZoomLevel(tiledProvider->getMinZoom());
            maxZoom = Utilities::clipZoomLevel(tiledProvider->getMaxZoom());

            minVisibleZoom = Utilities::clipZoomLevel(tiledProvider->getMinVisibleZoom());
            maxVisibleZoom = Utilities::clipZoomLevel(tiledProvider->getMaxVisibleZoom());

            maxMissingDataZoomShift = tiledProvider->getMaxMissingDataZoomShift();
            maxMissingDataUnderZoomShift = tiledProvider->getMaxMissingDataUnderZoomShift();
        }

        bool isCustomVisibility = minZoom != minVisibleZoom || maxZoom != maxVisibleZoom;

        if (isCustomVisibility && (activeZoom < minVisibleZoom || currentZoom > maxVisibleZoom))
            return;

        // Don't request any resources when the ones for low or high detailed tiles can't be shown
        if (!isCustomVisibility && (currentZoom - maxZoom > maxMissingDataZoomShift || minZoom - currentZoom > maxMissingDataUnderZoomShift))
            return;
    }
    bool metaTiled = tiledProvider && tiledProvider->isMetaTiled();
    // Prepare to request more detailed resource (of higher zoom level) if needed and possible
    const auto neededZoom = isElevationData || zoomLevelOffset == 0 ? activeZoom : static_cast<ZoomLevel>(std::min(
        activeZoom + std::min(zoomLevelOffset, maxMissingDataUnderZoomShift), static_cast<int>(maxZoom)));
    const auto extraZoom =
        isElevationData || zoomLevelOffset != 0 || activeZoom != currentZoom || extraDetailedTiles.empty() ? neededZoom
        : static_cast<ZoomLevel>(
            std::min(activeZoom + std::min(1, maxMissingDataUnderZoomShift), static_cast<int>(maxZoom)));
    // Request all tiles on needed zoom levels
    const int levelCount = 1 + (isSymbolData ? 0 : maxMissingDataUnderZoomShift);
    for (const auto& activeTileId : constOf(activeTiles))
    {
        const auto neededZoomForTile =
            extraZoom != neededZoom && extraDetailedTiles.contains(activeTileId) ? extraZoom : neededZoom;
        for (int absZoomShift = 0; absZoomShift < levelCount; absZoomShift++)
        {
            const int underZoom = neededZoomForTile - absZoomShift;
            if (underZoom < activeZoom || underZoom < minZoom || underZoom > maxZoom)
                break;

            auto underTiles = Utilities::getTileIdsUnderscaledByZoomShift(activeTileId, underZoom - activeZoom);
            if (metaTiled)
                addCombinedTilesToRequest(resourcesCollection, resourceAllocator, activeZoom, centerTileId, static_cast<ZoomLevel>(underZoom), underTiles);

            for (const auto& neededTileId : constOf(underTiles))
            {
                // Obtain a resource entry and if it's state is "Unknown", create a task that will
                // request resource data
                std::shared_ptr<MapRendererBaseTiledResource> resource;
                resourcesCollection->obtainOrAllocateEntry(resource,
                    neededTileId, static_cast<ZoomLevel>(underZoom), resourceAllocator);
                // Prepare to clean up scaled symbol resources when ones of needed zoom level can be used
                if (resourceType == MapRendererResourceType::Symbols
                    && resource->getState() == MapRendererResourceState::Unknown)
                    _clearOldSymbolResources = true;
                requestNeededResource(resource);
            }
        }
    }

    // Request all other zoom levels that cover unavailable tile, in case all scaled tiles are not unavailable
    if (isMapLayer || isElevationData)
    {
        const auto debugSettings = renderer->getDebugSettings();

        for (const auto& activeTileId : constOf(activeTiles))
        {
            if (activeZoom < minZoom && !isElevationData)
            {
                ZoomLevel underscaledZoom = minZoom;
                int zoomShift = underscaledZoom - activeZoom;
                if (zoomShift <= maxMissingDataUnderZoomShift)
                {
                    const auto underscaledTileIdsN = Utilities::getTileIdsUnderscaledByZoomShift(
                        activeTileId,
                        zoomShift);

                    const auto tilesCount = underscaledTileIdsN.size();
                    auto pUnderscaledTileIdN = underscaledTileIdsN.constData();
                    for (auto tileIdx = 0; tileIdx < tilesCount; tileIdx++)
                    {
                        const auto& underscaledTileId = *(pUnderscaledTileIdN++);

                        const auto underscaledTilePresent = resourcesCollection->containsResource(
                            underscaledTileId,
                            underscaledZoom);
                        if (!underscaledTilePresent)
                        {
                            std::shared_ptr<MapRendererBaseTiledResource> resource;
                            resourcesCollection->obtainOrAllocateEntry(
                                resource,
                                underscaledTileId,
                                underscaledZoom,
                                resourceAllocator);
                            requestNeededResource(resource);
                        }
                    }
                }
            }
            else if (activeZoom > maxZoom)
            {
                ZoomLevel overscaledZoom = maxZoom;
                int zoomShift = activeZoom - maxZoom;
                if (zoomShift <= maxMissingDataZoomShift)
                {
                    const auto overscaledTileId = Utilities::getTileIdOverscaledByZoomShift(
                        activeTileId,
                        zoomShift);
                    if (!resourcesCollection->containsResource(
                        overscaledTileId,
                        overscaledZoom))
                    {
                        std::shared_ptr<MapRendererBaseTiledResource> resource;
                        resourcesCollection->obtainOrAllocateEntry(
                            resource,
                            overscaledTileId,
                            overscaledZoom,
                            resourceAllocator);
                        requestNeededResource(resource);
                    }
                }
            }
        }
    }
}

void OsmAnd::MapRendererResourcesManager::requestNeededKeyedResources(
    const std::shared_ptr<MapRendererKeyedResourcesCollection>& resourcesCollection,
    const ZoomLevel activeZoom)
{
    // Get keyed provider
    std::shared_ptr<IMapDataProvider> provider_;
    if (!obtainProviderFor(static_cast<MapRendererBaseResourcesCollection*>(resourcesCollection.get()), provider_))
        return;
    const auto& provider = std::dynamic_pointer_cast<IMapKeyedDataProvider>(provider_);
    if (!provider || activeZoom < provider->getMinZoom() || activeZoom > provider->getMaxZoom())
        return;

    // Use provider's priority as value of priority for all its resources
    const auto priority = provider->getPriority();

    // Get list of keys this provider has and check that all are present
    const auto& resourceKeys = provider->getProvidedDataKeys();
    for (const auto& resourceKey : constOf(resourceKeys))
    {
        // Obtain a resource entry and if it's state is "Unknown", create a task that will
        // request resource data
        std::shared_ptr<MapRendererBaseKeyedResource> resource;
        const auto resourceType = resourcesCollection->type;
        resourcesCollection->obtainOrAllocateEntry(resource, resourceKey,
            [this, resourceType, priority]
            (const KeyedEntriesCollection<MapRendererKeyedResourcesCollection::Key,
                MapRendererBaseKeyedResource>& collection,
                MapRendererKeyedResourcesCollection::Key const key) -> MapRendererBaseKeyedResource*
            {
                if (resourceType == MapRendererResourceType::Symbols)
                    return new MapRendererKeyedSymbolsResource(this, collection, key, priority);
                else
                    return nullptr;
            });

        requestNeededResource(resource);
    }
}

void OsmAnd::MapRendererResourcesManager::requestNeededResource(
    const std::shared_ptr<MapRendererBaseResource>& resource)
{
    // Only if tile entry has "Unknown" state proceed to "Requesting" state
    if (resource->getState() != MapRendererResourceState::Updating)
    {
        if (!resource->setStateIf(MapRendererResourceState::Unknown, MapRendererResourceState::Requesting))
            return;
        LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Unknown, MapRendererResourceState::Requesting);
    }

    if (resource->supportsObtainDataAsync())
    {
        // Since resource supports natural async, there's no need to use workers pool

        if (resource->getState() == MapRendererResourceState::Requesting)
        {
            resource->setState(MapRendererResourceState::Requested);
            LOG_RESOURCE_STATE_CHANGE(resource, ?, MapRendererResourceState::Requested);
        }
        else if (resource->getState() == MapRendererResourceState::Updating)
        {
            resource->setState(MapRendererResourceState::RequestedUpdate);
            LOG_RESOURCE_STATE_CHANGE(resource, ?, MapRendererResourceState::RequestedUpdate);
        }


        if (beginResourceRequestProcessing(resource))
        {
            const std::shared_ptr<SimpleQueryController> queryController(new SimpleQueryController());

            resource->_cancelRequestCallback =
                [queryController]
                ()
                {
                    queryController->abort();
                };

            if (resource->isOld)
            {
                // Try to access remote data this time
                resource->markAsFresh();
                const MapRendererBaseResource::ObtainDataAsyncCallback callback =
                    [this, resource, queryController]
                    (const bool requestSucceeded, const bool dataAvailable)
                    {
                        endResourceRequestProcessing(resource, requestSucceeded, dataAvailable);
                        if (queryController->isAborted())
                            processResourceRequestCancellation(resource);
                    };
                // Request data accessible locally or remotely
                resource->obtainDataAsync(callback, queryController, false);
                return;
            }

            const MapRendererBaseResource::ObtainDataAsyncCallback callback =
                [this, resource, queryController]
                (const bool requestSucceeded, const bool dataAvailable)
                {
                    // Repeat the request for remote resource if proper data is not found in cache
                    const auto cachedResource = std::dynamic_pointer_cast<MapRendererRasterMapLayerResource>(resource);                    
                    if (cachedResource && requestSucceeded &&
                        (!dataAvailable || cachedResource->zoom != cachedResource->_sourceData->zoom) &&
                        (cachedResource->getState() == MapRendererResourceState::ProcessingUpdate ||
                        cachedResource->getState() == MapRendererResourceState::ProcessingUpdateWhileRenewing ||
                        cachedResource->getState() == MapRendererResourceState::ProcessingRequest) &&
                        !queryController->isAborted())
                    {
                        // Create a complete resource for received data that has different zoom level
                        const auto link_ = cachedResource->link.lock();
                        if (link_ && dataAvailable && cachedResource->zoom != cachedResource->_sourceData->zoom)
                        {
                            const auto tiledResources =
                                static_cast<MapRendererTiledResourcesCollection*>(&link_->collection);
                            if (cachedResource->_sourceData->zoom != InvalidZoomLevel)
                            {
                                // Create a resource for existing overscale (or underscale) cached tile
                                const auto zoomLevel = cachedResource->_sourceData->zoom;
                                if (!tiledResources->containsResource(cachedResource->_sourceData->tileId, zoomLevel))
                                {
                                    const auto resourceAllocator =
                                        [this, cachedResource]
                                        (const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
                                            const TileId tileId,
                                            const ZoomLevel zoom) -> MapRendererBaseTiledResource*
                                        {
                                            auto resource = new MapRendererRasterMapLayerResource(
                                                this, collection, tileId, zoom);
                                            resource->_sourceData = cachedResource->_sourceData;
                                            resource->setState(MapRendererResourceState::Ready);
                                            return resource;
                                        };
                                    std::shared_ptr<MapRendererBaseTiledResource> completeResource;
                                    tiledResources->obtainOrAllocateEntry(
                                        completeResource,
                                        cachedResource->_sourceData->tileId,
                                        zoomLevel,
                                        resourceAllocator);
                                    requestResourcesUploadOrUnload();
                                }
                            }
                            else if (!cachedResource->_sourceData->images.isEmpty())
                            {
                                cachedResource->_sourceData->zoom = cachedResource->zoom;
                                cachedResource->markAsOld();
                                endResourceRequestProcessing(resource, requestSucceeded, dataAvailable);
                                if (queryController->isAborted())
                                    processResourceRequestCancellation(resource);
                                return;
                            }
                        }
                        // Continue requesting data for the same resource (last time)
                        if (cachedResource->_sourceData)
                            cachedResource->_sourceData.reset();
                        const MapRendererBaseResource::ObtainDataAsyncCallback callback =
                            [this, resource, queryController]
                            (const bool requestSucceeded, const bool dataAvailable)
                            {
                                endResourceRequestProcessing(resource, requestSucceeded, dataAvailable);
                                if (queryController->isAborted())
                                    processResourceRequestCancellation(resource);
                            };
                        // Request data accessible locally or remotely
                        resource->obtainDataAsync(callback, queryController, false);
                    }
                    else
                    {
                        endResourceRequestProcessing(resource, requestSucceeded, dataAvailable);
                        if (queryController->isAborted())
                            processResourceRequestCancellation(resource);
                    }
                };
            // Request locally accessible data only (for the first time)
            resource->obtainDataAsync(callback, queryController, true);
        }
    }
    else
    {
        // Create async-task that will obtain needed resource data
        const auto asyncTask = new ResourceRequestTask(resource, _taskHostBridge);
        const auto weakAsyncTaskCancellator = asyncTask->obtainCancellator();

        // Register resource as requested
        resource->_cancelRequestCallback =
            [this, weakAsyncTaskCancellator, asyncTask]
            ()
            {
                if (const auto asyncTaskCancellator = weakAsyncTaskCancellator.lock())
                {
                    asyncTaskCancellator->requestCancellation();

                    // In case task was successfully dequeued, it means it will never get executed
                    const auto dequeued = _resourcesRequestWorkerPool.dequeue(asyncTask);
                    if (dequeued)
                    {
                        asyncTask->run();
                        if (asyncTask->autoDelete())
                            delete asyncTask;
                    }
                }
            };
        if (resource->getState() == MapRendererResourceState::Requesting)
        {
            resource->setState(MapRendererResourceState::Requested);
            LOG_RESOURCE_STATE_CHANGE(resource, ?, MapRendererResourceState::Requested);
        }
        else if (resource->getState() == MapRendererResourceState::Updating)
        {
            resource->setState(MapRendererResourceState::RequestedUpdate);
            LOG_RESOURCE_STATE_CHANGE(resource, ?, MapRendererResourceState::RequestedUpdate);
        }

        // Finally start the request in a proper workers pool
        _requestedResourcesTasks.push_back(asyncTask);
    }
}

void OsmAnd::MapRendererResourcesManager::addCombinedTilesToRequest(
    const std::shared_ptr<MapRendererTiledResourcesCollection>& resourcesCollection,
    const TiledResourceAllocator resourceAllocator,
    const ZoomLevel activeZoom,
    const TileId centerTileId,
    const ZoomLevel zoom,
    QVector<TileId>& tiles)
{
    if (zoom == ZoomLevel::MinZoomLevel)
        return;

    // Collect all overscaled tiles
    QSet<TileId> overscaledTilesIds;
    for (TileId underscledTile : tiles)
        overscaledTilesIds.insert(Utilities::getTileIdOverscaledByZoomShift(underscledTile, 1));

    for (TileId overscaledTileId : overscaledTilesIds)
    {
        const auto& underscaledTilesIds = Utilities::getMetaTileIds(overscaledTileId);

        // Try to find meta tile resource
        TileId metaTileId = TileId::zero();
        for (const auto tileId : underscaledTilesIds)
        {
            std::shared_ptr<MapRendererBaseTiledResource> resource;
            if (resourcesCollection->obtainEntry(resource, tileId, zoom) && resource->isMetaTiled)
            {
                metaTileId = tileId;
                break;
            }
        }

        const bool markMetaTile = metaTileId == TileId::zero();

        // Meta tile is not selected yet
        // Firstly try to find unrequested tile to mark as meta tile
        if (metaTileId == TileId::zero())
        {
            for (const auto tileId : underscaledTilesIds)
            {
                if (!tiles.contains(tileId))
                {
                    metaTileId = tileId;
                    break;
                }
            }
        }
        // If there is no unrequested tile, mark farthest tile as meta tile
        if (metaTileId == TileId::zero())
        {
            uint32_t maxDistance = 0;
            for (const auto tileId : underscaledTilesIds)
            {
                const auto& activeZoomTileId = Utilities::getTileIdOverscaledByZoomShift(tileId, zoom - activeZoom);
                const PointI offset(activeZoomTileId.x - centerTileId.x, activeZoomTileId.y - centerTileId.y);
                const auto& normalizedOffset = Utilities::shortestVector(offset, activeZoom);
                const auto distance = static_cast<uint32_t>(normalizedOffset.squareNorm());
                if (distance > maxDistance || metaTileId == TileId::zero())
                {
                    metaTileId = tileId;
                    maxDistance = distance;
                }
            }
        }

        if (markMetaTile)
        {
            std::shared_ptr<MapRendererBaseTiledResource> resource;
            resourcesCollection->obtainOrAllocateEntry(resource, metaTileId, zoom, resourceAllocator);
            resource->isMetaTiled = true;
        }

        // Add meta tile if not added yet
        if (!tiles.contains(metaTileId))
            tiles.push_back(metaTileId);
    }
}

bool OsmAnd::MapRendererResourcesManager::beginResourceRequestProcessing(
    const std::shared_ptr<MapRendererBaseResource>& resource)
{
    // Only if resource entry has "Requested" state proceed to "ProcessingRequest" state
    if (resource->setStateIf(MapRendererResourceState::Requested, MapRendererResourceState::ProcessingRequest))
    {
        LOG_RESOURCE_STATE_CHANGE(
            resource,
            MapRendererResourceState::Requested,
            MapRendererResourceState::ProcessingRequest);
    }
    else if (resource->setStateIf(MapRendererResourceState::RequestedUpdate,
        MapRendererResourceState::ProcessingUpdate))
    {
        LOG_RESOURCE_STATE_CHANGE(
            resource,
            MapRendererResourceState::RequestedUpdate,
            MapRendererResourceState::ProcessingUpdate);
    }
    else
    {
        // This actually can happen in following situation(s):
        //   - if request task was canceled before has started it's execution,
        //     since in that case a state change "Requested => JustBeforeDeath" must have happend.
        //     In this case entry will be removed in post-execute handler.
        //   - if update request task was cancelled before has started it's execution,
        //     since in that case a state change "RequestedUpdate => UnloadPending" must have happend.
        return false;
    }

    return true;
}

void OsmAnd::MapRendererResourcesManager::endResourceRequestProcessing(
    const std::shared_ptr<MapRendererBaseResource>& resource,
    const bool requestSucceeded,
    const bool dataAvailable)
{
    // If failed to obtain resource data, remove resource entry to repeat try later
    if (!requestSucceeded)
    {
        if (resource->setStateIf(MapRendererResourceState::ProcessingUpdate, MapRendererResourceState::Outdated))
        {
            LOG_RESOURCE_STATE_CHANGE(resource,
                MapRendererResourceState::ProcessingUpdate, MapRendererResourceState::Outdated);
        }
        else if (resource->setStateIf(
            MapRendererResourceState::ProcessingUpdateWhileRenewing, MapRendererResourceState::Outdated))
        {
            LOG_RESOURCE_STATE_CHANGE(resource,
                MapRendererResourceState::ProcessingUpdateWhileRenewing, MapRendererResourceState::Outdated);
        }
        else if (resource->setStateIf(
            MapRendererResourceState::UpdatingCancelledWhileBeingProcessed, MapRendererResourceState::UnloadPending))
        {
            LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::UpdatingCancelledWhileBeingProcessed,
                MapRendererResourceState::UnloadPending);
            if (!resource->leaveQuietly)
                requestResourcesUploadOrUnload();
        }
        else
        {
            // It's safe to simply remove entry, since it's not yet uploaded
            resource->removeSelfFromCollection();
        }
        if (!resource->leaveQuietly)
            renderer->invalidateFrame();
        return;
    }

    // Finalize execution of task
    if (!resource->setStateIf(MapRendererResourceState::ProcessingRequest,
        dataAvailable ? MapRendererResourceState::Ready : MapRendererResourceState::Unavailable)
        && !resource->setStateIf(MapRendererResourceState::ProcessingUpdate,
        dataAvailable ? MapRendererResourceState::PreparedRenew : MapRendererResourceState::Uploaded)
        && !resource->setStateIf(MapRendererResourceState::ProcessingUpdateWhileRenewing,
        dataAvailable ? MapRendererResourceState::PreparedRenew : MapRendererResourceState::Uploaded))
    {
        assert(resource->getState() == MapRendererResourceState::RequestCanceledWhileBeingProcessed
            || resource->getState() == MapRendererResourceState::UpdatingCancelledWhileBeingProcessed);

        // While request was processed, state may have changed to "RequestCanceledWhileBeingProcessed"
        // or "UpdatingCancelledWhileBeingProcessed"
        processResourceRequestCancellation(resource);
        return;
    }
#if OSMAND_LOG_RESOURCE_STATE_CHANGE
    else if (resource->getState() == MapRendererResourceState::MapRendererResourceState::ProcessingRequest)
    {
        if (dataAvailable)
        {
            LOG_RESOURCE_STATE_CHANGE(
                resource,
                MapRendererResourceState::ProcessingRequest,
                MapRendererResourceState::Ready);
        }
        else
        {
            LOG_RESOURCE_STATE_CHANGE(
                resource,
                MapRendererResourceState::ProcessingRequest,
                MapRendererResourceState::Unavailable);
        }
    }
    else if (resource->getState() == MapRendererResourceState::MapRendererResourceState::ProcessingUpdate)
    {
        if (dataAvailable)
        {
            LOG_RESOURCE_STATE_CHANGE(
                resource,
                MapRendererResourceState::ProcessingUpdate,
                MapRendererResourceState::PreparedRenew);
        }
        else
        {
            LOG_RESOURCE_STATE_CHANGE(
                resource,
                MapRendererResourceState::ProcessingUpdate,
                MapRendererResourceState::Uploaded);
        }
    }
    else if (resource->getState() == MapRendererResourceState::MapRendererResourceState::ProcessingUpdateWhileRenewing)
    {
        if (dataAvailable)
        {
            LOG_RESOURCE_STATE_CHANGE(
                resource,
                MapRendererResourceState::ProcessingUpdateWhileRenewing,
                MapRendererResourceState::PreparedRenew);
        }
        else
        {
            LOG_RESOURCE_STATE_CHANGE(
                resource,
                MapRendererResourceState::ProcessingUpdateWhileRenewing,
                MapRendererResourceState::Uploaded);
        }
    }
#endif // OSMAND_LOG_RESOURCE_STATE_CHANGE
    resource->_cancelRequestCallback = nullptr;

    // There is data to upload to GPU, request uploading. Or just ask to show that resource is unavailable
    if (!resource->leaveQuietly)
    {
        if (dataAvailable)
            requestResourcesUploadOrUnload();
        else
            renderer->invalidateFrame();
    }
}

void OsmAnd::MapRendererResourcesManager::processResourceRequestCancellation(
    const std::shared_ptr<MapRendererBaseResource>& resource)
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
        LOG_RESOURCE_STATE_CHANGE(resource, ?, MapRendererResourceState::JustBeforeDeath);

        resource->removeSelfFromCollection();
    }
    else if (resource->setStateIf(MapRendererResourceState::RequestedUpdate, MapRendererResourceState::UnloadPending)
        || resource->setStateIf(MapRendererResourceState::UpdatingCancelledWhileBeingProcessed,
            MapRendererResourceState::UnloadPending))
        LOG_RESOURCE_STATE_CHANGE(resource, ?, MapRendererResourceState::UnloadPending);

    // All other cases must be handled in other places, since here there is no access to GPU
}

void OsmAnd::MapRendererResourcesManager::invalidateAllResources()
{
    const auto newMask =
        ~((std::numeric_limits<uint32_t>::max() >> MapRendererResourceTypesCount) << MapRendererResourceTypesCount);
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
    assert(renderer->isInRenderThread());

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
            if (const auto keyedResourcesCollection =
                    std::dynamic_pointer_cast<MapRendererKeyedResourcesCollection::Snapshot>(collectionSnapshot))
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

bool OsmAnd::MapRendererResourcesManager::checkForUpdatesAndApply(const MapState& mapState) const
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
            if (const auto keyedResourcesCollection =
                    std::dynamic_pointer_cast<MapRendererKeyedResourcesCollection::Snapshot>(collectionSnapshot))
            {
                std::shared_ptr<IMapDataProvider> provider_;
                if (obtainProviderFor(resourcesCollection.get(), provider_))
                {
                    const auto provider = std::static_pointer_cast<IMapKeyedDataProvider>(provider_);

                    if (keyedResourcesCollection->getKeys().toSet() != provider->getProvidedDataKeys().toSet()
                        && mapState.zoomLevel >=  provider->getMinZoom()
                        && mapState.zoomLevel <= provider->getMaxZoom())
                        updatesPresent = true;
                }
            }

            // Check if any resource has applied updates
            collectionSnapshot->forEachResourceExecute(
                [&updatesApplied, &mapState]
                (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel)
                {
                    if (entry->checkForUpdatesAndApply(mapState))
                        updatesApplied = true;
                });
        }
    }

    return (updatesApplied || updatesPresent);
}

void OsmAnd::MapRendererResourcesManager::updateResources(
    QMap<ZoomLevel, QVector<TileId>>& activeTiles, QMap<ZoomLevel, TileId>& activeTilesTargets,
    QMap<ZoomLevel, QVector<TileId>>& visibleTiles, QSet<TileId>& extraDetailedTiles, int zoomLevelOffset)
{
    QList< std::shared_ptr<MapRendererBaseResourcesCollection> > pendingRemovalResourcesCollections;
    QList< std::shared_ptr<MapRendererBaseResourcesCollection> > otherResourcesCollections;
    safeGetAllResourcesCollections(pendingRemovalResourcesCollections, otherResourcesCollections);

    // Before requesting missing tiled resources, clean up cache to free some space
    auto currentZoom = MinZoomLevel;
    auto tilesBegin = activeTiles.cbegin();
    auto tilesEnd = activeTiles.cend();
    if (tilesBegin != tilesEnd)
    {
        currentZoom = (tilesEnd - 1).key();
        if (!renderer->currentDebugSettings->disableJunkResourcesCleanup)
            cleanupJunkResources(pendingRemovalResourcesCollections, otherResourcesCollections,
                activeTiles, currentZoom, visibleTiles, extraDetailedTiles, zoomLevelOffset);
    }

    // In the end of rendering processing, request tiled resources that are neither
    // present in requested list, nor in pending, nor in uploaded
    if (!renderer->currentDebugSettings->disableNeededResourcesRequests)
    {
        for (auto itTiles = tilesEnd; itTiles != tilesBegin; itTiles--)
        {
            const auto& tilesEntry = itTiles - 1;
            const auto zoomLevel = tilesEntry.key();
            const auto& targetTileId = activeTilesTargets.constFind(zoomLevel);
            const auto& visibleTileIds = visibleTiles.constFind(zoomLevel);
            if (targetTileId != activeTilesTargets.cend() && visibleTileIds != visibleTiles.cend())
            {
                requestNeededResources(otherResourcesCollections, targetTileId.value(), tilesEntry.value(),
                    zoomLevel, currentZoom, visibleTileIds.value(), extraDetailedTiles, zoomLevelOffset);
                requestOutdatedResources(otherResourcesCollections);
            }
        }
    }
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
    if (renderer->gpuContextIsLost)
        return;

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
        if (renderer->gpuContextIsLost)
            return;

        // Since state change is allowed (it's not changed to "Unloading" during query), check state here
        if (!resource->setStateIf(MapRendererResourceState::UnloadPending, MapRendererResourceState::Unloading))
            continue;

        LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::UnloadPending, MapRendererResourceState::Unloading);

        // Unload from GPU
        resource->unloadFromGPU();

        // Don't wait until GPU will execute unloading, since this resource won't be used anymore and will eventually be deleted

        assert(resource->getState() == MapRendererResourceState::Unloading);

        // Mark as unloaded
        resource->setState(MapRendererResourceState::Unloaded);
        LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Unloading, MapRendererResourceState::Unloaded);

        // Count uploaded resources
        totalUnloaded++;
    }
}

unsigned int OsmAnd::MapRendererResourcesManager::uploadResources(
    const unsigned int limit /*= 0u*/,
    bool* const outMoreThanLimitAvailable /*= nullptr*/)
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
    if (renderer->gpuContextIsLost)
        return;

    // Select all resources with "Ready" state
    QList< std::shared_ptr<MapRendererBaseResource> > resources;
    collection->obtainResources(&resources,
        [&resources, limit, totalUploaded, &moreThanLimitAvailable]
        (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
        {
            // Skip not-ready resources
            if (entry->getState() != MapRendererResourceState::Ready &&
                entry->getState() != MapRendererResourceState::PreparedRenew &&
                entry->getState() != MapRendererResourceState::ProcessingUpdate)
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
        if (renderer->gpuContextIsLost)
            return;

        // Since state change is allowed (it's not changed to "Uploading" during query), check state here
        if (resource->setStateIf(MapRendererResourceState::Ready, MapRendererResourceState::Uploading))
            LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Ready, MapRendererResourceState::Uploading);
        else if (resource->setStateIf(MapRendererResourceState::PreparedRenew, MapRendererResourceState::Renewing))
        {
            LOG_RESOURCE_STATE_CHANGE(
                resource, MapRendererResourceState::PreparedRenew, MapRendererResourceState::Renewing);
        }
        else if (resource->setStateIf(MapRendererResourceState::ProcessingUpdate,
            MapRendererResourceState::ProcessingUpdateWhileRenewing))
        {
            LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::ProcessingUpdate,
                MapRendererResourceState::ProcessingUpdateWhileRenewing);
        }
        else
            continue;

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

            if (resource->setStateIf(MapRendererResourceState::Uploading, MapRendererResourceState::Unloaded))
            {
                LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Uploading,
                    MapRendererResourceState::Unloaded);
            }
            else if (resource->setStateIf(MapRendererResourceState::Renewing, MapRendererResourceState::Uploaded))
            {
                LOG_RESOURCE_STATE_CHANGE(
                    resource, MapRendererResourceState::Renewing, MapRendererResourceState::Uploaded);
            }

            continue;
        }
        else if (resource->type == MapRendererResourceType::ElevationData)
            renderer->notifyRequestedStateWasUpdated(MapRendererStateChange::Elevation_DataProvider);

        // Mark as uploaded/outdated
        if (resource->isOld)
        {
            if (resource->setStateIf(MapRendererResourceState::Uploading, MapRendererResourceState::Outdated))
            {
                LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Uploading,
                    MapRendererResourceState::Outdated);
            }
            else if (resource->setStateIf(MapRendererResourceState::Renewing, MapRendererResourceState::Outdated))
            {
                LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Renewing,
                    MapRendererResourceState::Outdated);
            }
            else if (resource->setStateIf(MapRendererResourceState::ProcessingUpdateWhileRenewing,
                MapRendererResourceState::ProcessingUpdate))
            {
                LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::ProcessingUpdateWhileRenewing,
                    MapRendererResourceState::ProcessingUpdate);
            }
        }
        else if (resource->isOldTime)
        {
            if (resource->setStateIf(MapRendererResourceState::Uploading, MapRendererResourceState::PreparedRenew))
            {
                LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Uploading,
                    MapRendererResourceState::PreparedRenew);
                resource->unmarkAsOldTime();
            }
            else if (resource->setStateIf(MapRendererResourceState::Renewing, MapRendererResourceState::PreparedRenew))
            {
                LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Renewing,
                    MapRendererResourceState::PreparedRenew);
                resource->unmarkAsOldTime();
            }
            else if (resource->setStateIf(MapRendererResourceState::ProcessingUpdateWhileRenewing,
                MapRendererResourceState::ProcessingUpdate))
            {
                LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::ProcessingUpdateWhileRenewing,
                    MapRendererResourceState::ProcessingUpdate);
                resource->unmarkAsOldTime();
            }
        }
        else
        {
            if (resource->setStateIf(MapRendererResourceState::Uploading, MapRendererResourceState::Uploaded))
            {
                LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Uploading,
                    MapRendererResourceState::Uploaded);
            }
            else if (resource->setStateIf(MapRendererResourceState::Renewing, MapRendererResourceState::Uploaded))
            {
                LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Renewing,
                    MapRendererResourceState::Uploaded);
            }
            else if (resource->setStateIf(MapRendererResourceState::ProcessingUpdateWhileRenewing,
                MapRendererResourceState::ProcessingUpdate))
            {
                LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::ProcessingUpdateWhileRenewing,
                    MapRendererResourceState::ProcessingUpdate);
            }
        }

        // Count uploaded resources
        totalUploaded++;
    }
}

void OsmAnd::MapRendererResourcesManager::cleanupJunkResources(
    const QList< std::shared_ptr<MapRendererBaseResourcesCollection> >& pendingRemovalResourcesCollections,
    const QList< std::shared_ptr<MapRendererBaseResourcesCollection> >& resourcesCollections,
    const QMap<ZoomLevel, QVector<TileId>>& tiles, const ZoomLevel currentZoom,
    QMap<ZoomLevel, QVector<TileId>>& visibleTiles, QSet<TileId>& extraDetailedTiles, int zoomLevelOffset)
{
    const auto debugSettings = renderer->getDebugSettings();

    // This method is called from non-GPU thread, so it's impossible to unload resources from GPU here
    bool needsResourcesUploadOrUnload = false;

    // Release some resources from "pending-removal" collections
    QList< std::shared_ptr<MapRendererBaseResourcesCollection> > removedResouresCollections;
    for (const auto& pendingRemovalCollection : constOf(pendingRemovalResourcesCollections))
    {
        // Don't wait for anything. If impossible to process resource right now, just skip it
        pendingRemovalCollection->removeResources(
            [this, &needsResourcesUploadOrUnload]
            (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
            {
                return cleanupJunkResource(entry, needsResourcesUploadOrUnload);
            });

        // Remove empty collections completely
        if (pendingRemovalCollection->getResourcesCount() == 0)
            removedResouresCollections.push_back(pendingRemovalCollection);
    }
    if (!removedResouresCollections.isEmpty())
    {
        QWriteLocker scopedLocker(&_resourcesStoragesLock);

        for (const auto& pendingRemovalCollection : constOf(removedResouresCollections))
            _pendingRemovalResourcesCollections.removeOne(pendingRemovalCollection);
        removedResouresCollections.clear();
    }

    // Keep overscaled/underscaled symbol resources while ones of current zoom level can't be used yet
    bool keepOldSymbolResources = false;

    // Update symbols only if at least one overscaled/underscaled symbol resource present
    bool atLeastOneScaledPresent = false;

    // Use aggressive cache cleaning: remove all resources that are not needed
    for (const auto& resourcesCollection : constOf(resourcesCollections))
    {
        // Skip empty entries
        if (!resourcesCollection)
            continue;

        // Get data provider
        std::shared_ptr<IMapDataProvider> dataProvider;
        const auto dataSourceAvailable = obtainProviderFor(resourcesCollection.get(), dataProvider);

        // Regular checks common for all resources
        resourcesCollection->removeResources(
            [this, dataSourceAvailable, &needsResourcesUploadOrUnload]
            (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
            {
                // Resource with "Unloaded" state is junk, regardless if it's needed or not
                if (entry->setStateIf(MapRendererResourceState::Unloaded, MapRendererResourceState::JustBeforeDeath))
                {
                    LOG_RESOURCE_STATE_CHANGE(
                        entry,
                        MapRendererResourceState::Unloaded,
                        MapRendererResourceState::JustBeforeDeath);

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

        // Some checks are only valid for keyed resources
        std::shared_ptr<IMapDataProvider> provider_;
        if (dataProvider != nullptr && std::dynamic_pointer_cast<IMapRendererKeyedResourcesCollection>(resourcesCollection))
        {
            const auto keyedDataProvider = std::static_pointer_cast<IMapKeyedDataProvider>(dataProvider);
            const auto providedKeysSet = keyedDataProvider->getProvidedDataKeys().toSet();

            resourcesCollection->removeResources(
                [this, currentZoom, &keyedDataProvider, &needsResourcesUploadOrUnload, providedKeysSet]
                (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
                {
                    // If it was previously marked as junk, just leave it
                    if (entry->isJunk)
                        return false;

                    const auto keyedEntry = std::static_pointer_cast<MapRendererBaseKeyedResource>(entry);

                    // Determine if resource is junk:
                    bool isJunk = false;

                    // If this key is no longer provided by keyed provider
                    isJunk = isJunk || !providedKeysSet.contains(keyedEntry->key);
                    isJunk = isJunk || currentZoom < keyedDataProvider->getMinZoom();
                    isJunk = isJunk || currentZoom > keyedDataProvider->getMaxZoom();

                    // Skip cleaning if this resource is not junk
                    if (!isJunk)
                        return false;

                    // Mark this entry as junk until it will die
                    entry->markAsJunk();

                    return cleanupJunkResource(entry, needsResourcesUploadOrUnload);
                });
        }

        // Some checks are only valid for tiled resources
        if (const auto tiledResourcesCollection = std::dynamic_pointer_cast<IMapRendererTiledResourcesCollection>(resourcesCollection))
        {
            const auto tiledProvider = std::dynamic_pointer_cast<IMapTiledDataProvider>(dataProvider);
            const auto resourcesType = resourcesCollection->getType();
            const bool checkVisible = resourcesType != MapRendererResourceType::ElevationData;

            if (tiledProvider && tiledProvider->isMetaTiled())
            {
                assert(checkVisible);
                addCombinedTilesToAvoidCleanup(tiledResourcesCollection, visibleTiles);
            }

            auto minZoom = MinZoomLevel;
            auto maxZoom = MaxZoomLevel;

            auto minVisibleZoom = MinZoomLevel;
            auto maxVisibleZoom = MaxZoomLevel;

            int maxMissingDataZoomShift = MapRenderer::MaxMissingDataZoomShift;
            int maxMissingDataUnderZoomShift = MapRenderer::MaxMissingDataUnderZoomShift;

            if (tiledProvider && (resourcesType == MapRendererResourceType::MapLayer
                || resourcesType == MapRendererResourceType::ElevationData))
            {
                minZoom = Utilities::clipZoomLevel(tiledProvider->getMinZoom());
                maxZoom = Utilities::clipZoomLevel(tiledProvider->getMaxZoom());

                minVisibleZoom = Utilities::clipZoomLevel(tiledProvider->getMinVisibleZoom());
                maxVisibleZoom = Utilities::clipZoomLevel(tiledProvider->getMaxVisibleZoom());
            }
            if (tiledProvider)
            {
                maxMissingDataZoomShift = tiledProvider->getMaxMissingDataZoomShift();
                maxMissingDataUnderZoomShift = tiledProvider->getMaxMissingDataUnderZoomShift();
            }

            bool isCustomVisibility = minZoom != minVisibleZoom || maxZoom != maxVisibleZoom;

            resourcesCollection->removeResources(
                [this, currentZoom, tiles, visibleTiles, checkVisible,
                maxMissingDataUnderZoomShift, &needsResourcesUploadOrUnload]
                (const std::shared_ptr<MapRendererBaseResource>& entry, bool& cancel) -> bool
                {
                    // If it was previously marked as junk, just leave it
                    if (entry->isJunk)
                        return false;

                    const auto tiledEntry = std::static_pointer_cast<MapRendererBaseTiledResource>(entry);

                    // Determine if resource is junk:
                    bool isJunk = false;

                    // If this tiled entry is part of current zoom, it's treated as junk only if it's not a part
                    // of active tiles set and can't be used underscaled
                    if (tiledEntry->zoom == currentZoom)
                    {
                        bool usefulTile = false;
                        for (int zoomShift = 0; zoomShift <= maxMissingDataUnderZoomShift; zoomShift++)
                        {
                            const auto& activeTiles = checkVisible ? visibleTiles : tiles;
                            const auto tilesOfZoom =
                                activeTiles.constFind(static_cast<ZoomLevel>(tiledEntry->zoom - zoomShift));
                            if (tilesOfZoom != activeTiles.cend())
                            {
                                const auto tileId = TileId::fromXY(
                                    tiledEntry->tileId.x >> zoomShift,
                                    tiledEntry->tileId.y >> zoomShift);
                                if (tilesOfZoom->contains(tileId))
                                {
                                    usefulTile = true;
                                    break;
                                }
                            }
                            if (tiledEntry->zoom - zoomShift == MinZoomLevel)
                                break;
                        }
                        isJunk = isJunk || !usefulTile;
                    }
                    // If zoom delta is larger than provider's maxMissingDataUnderZoomShift, it means than this underscaled
                    // tile is not usable. If it's less than zero (overscaled tile), keep it.
                    const auto deltaZoom = static_cast<int>(tiledEntry->zoom) - static_cast<int>(currentZoom);
                    isJunk = isJunk || (deltaZoom > maxMissingDataUnderZoomShift);

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
                    return state == MapRendererResourceState::Uploaded;
                };
            for (const auto& tilesEntry : rangeOf(constOf(tiles)))
            {
                // Don't keep any resources if the ones for high detailed tiles aren't shown
                if (isCustomVisibility && currentZoom > maxVisibleZoom)
                    break;
                int currentZoomShift = currentZoom - maxZoom;
                if (!isCustomVisibility && currentZoomShift > maxMissingDataZoomShift)
                    break;

                const auto activeZoom = tilesEntry.key();
                const auto& visibleTilesOfZoom = visibleTiles.constFind(activeZoom);
                if (visibleTilesOfZoom == visibleTiles.cend())
                    continue;                
                const auto& activeTiles = checkVisible ? visibleTilesOfZoom.value() : tilesEntry.value();

                // Prepare to request more detailed resource (of higher zoom level) if needed and possible
                const auto neededZoom = !checkVisible || zoomLevelOffset == 0 ? activeZoom
                    : static_cast<ZoomLevel>(std::min(activeZoom
                        + std::min(zoomLevelOffset, maxMissingDataUnderZoomShift), static_cast<int>(maxZoom)));
                const auto extraZoom =
                    !checkVisible || zoomLevelOffset != 0 || activeZoom != currentZoom || extraDetailedTiles.empty()
                    ? neededZoom
                    : static_cast<ZoomLevel>(std::min(activeZoom
                        + std::min(1, maxMissingDataUnderZoomShift), static_cast<int>(maxZoom)));

                bool updateSymbolResources = _clearOldSymbolResources;
                if (resourcesType == MapRendererResourceType::Symbols)
                {
                    if (activeZoom != currentZoom)
                        continue;

                    // Don't update symbols if there are no more scaled resources already
                    if (updateSymbolResources && !keepOldSymbolResources && !atLeastOneScaledPresent)
                    {
                        const auto lowestZoom = std::min(neededZoom, extraZoom);
                        int lastZoom = std::max(lowestZoom - maxMissingDataZoomShift, static_cast<int>(minZoom));
                        for (int zoom = lowestZoom - 1; zoom >= lastZoom; zoom--)
                        {
                            if (tiledResourcesCollection->containsResources(static_cast<ZoomLevel>(zoom)))
                            {
                                atLeastOneScaledPresent = true;
                                break;
                            }
                        }
                    }
                    if (updateSymbolResources && !keepOldSymbolResources && !atLeastOneScaledPresent)
                    {
                        const auto highestZoom = std::max(neededZoom, extraZoom);
                        int lastZoom = std::min(highestZoom + maxMissingDataUnderZoomShift, static_cast<int>(maxZoom));
                        for (int zoom = highestZoom + 1; zoom <= lastZoom; zoom++)
                        {
                            if (tiledResourcesCollection->containsResources(static_cast<ZoomLevel>(zoom)))
                            {
                                atLeastOneScaledPresent = true;
                                break;
                            }
                        }
                    }

                    // Keep overscaled/underscaled resources if symbols of current zoom level ain't ready yet
                    if (updateSymbolResources)
                    {
                        const auto isLoadingResource =
                            []
                            (const std::shared_ptr<MapRendererBaseTiledResource>& entry) -> bool
                            {
                                if (entry->isJunk)
                                    return false;
                                const auto state = entry->getState();
                                return state == MapRendererResourceState::PreparedRenew
                                    || state == MapRendererResourceState::PreparingRenew
                                    || state == MapRendererResourceState::ProcessingRequest
                                    || state == MapRendererResourceState::Ready
                                    || state == MapRendererResourceState::Renewing
                                    || state == MapRendererResourceState::Requested
                                    || state == MapRendererResourceState::Requesting
                                    || state == MapRendererResourceState::Uploading;
                            };
                        bool atLeastOnePresent = false;
                        for (const auto& activeTileId : constOf(activeTiles))
                        {
                            const auto neededZoomForTile = extraZoom != neededZoom
                                && extraDetailedTiles.contains(activeTileId) ? extraZoom : neededZoom;
                            const auto neededTiles = Utilities::getTileIdsUnderscaledByZoomShift(
                                activeTileId, neededZoomForTile - activeZoom);
                            for (const auto& neededTileId : constOf(neededTiles))
                            {
                                // Don't remove overscaled/underscaled resources while some resource is loading
                                if (tiledResourcesCollection->containsResource(
                                        neededTileId,
                                        neededZoomForTile,
                                        isLoadingResource))
                                {
                                    updateSymbolResources = false;
                                    break;
                                }

                                // Don't remove overscaled/underscaled resources if no resource is there yet
                                if (tiledResourcesCollection->containsResource(
                                        neededTileId,
                                        neededZoomForTile,
                                        isUsableResource))
                                {
                                    atLeastOnePresent = true;
                                }
                            }
                            if (!updateSymbolResources)
                                break;
                        }
                        updateSymbolResources = updateSymbolResources && atLeastOnePresent;
                    }
                    if (!updateSymbolResources)
                        keepOldSymbolResources = true;
                }
                const int levelCount =
                    1 + (resourcesType == MapRendererResourceType::Symbols ? 0 : maxMissingDataUnderZoomShift);
                for (const auto& activeTileId : constOf(activeTiles))
                {
                    // If resources have complete set or match for this tile, use only that
                    const auto neededZoomForTile = extraZoom != neededZoom
                        && extraDetailedTiles.contains(activeTileId) ? extraZoom : neededZoom;
                    bool isComplete = false;
                    for (int absZoomShift = 0; absZoomShift < levelCount; absZoomShift++)
                    {
                        const int underZoom = neededZoomForTile - absZoomShift;
                        if (underZoom < activeZoom)
                            break;
                        const auto underscaledZoom = static_cast<ZoomLevel>(underZoom);
                        const auto underscaledTileIdsN = Utilities::getTileIdsUnderscaledByZoomShift(
                            activeTileId,
                            underZoom - activeZoom);
                        const auto tilesCount = underscaledTileIdsN.size();
                        auto pUnderscaledTileIdN = underscaledTileIdsN.constData();
                        bool atLeastOneAbsent = false;
                        for (auto tileIdx = 0; tileIdx < tilesCount; tileIdx++)
                        {
                            const auto& underscaledTileId = *(pUnderscaledTileIdN++);
                            neededTilesMap[underscaledZoom].insert(underscaledTileId);
                            if (!tiledResourcesCollection->containsResource(
                                    underscaledTileId,
                                    underscaledZoom,
                                    isUsableResource))
                                atLeastOneAbsent = true;
                        }
                        if (!atLeastOneAbsent)
                            isComplete = true;
                    }

                    // Keep resources for symbols of overscaled/underscaled tiles
                    if (resourcesType == MapRendererResourceType::Symbols)
                    {
                        if (updateSymbolResources)
                            continue;
                        for (int zoomShift = 1; zoomShift <= maxMissingDataUnderZoomShift; zoomShift++)
                        {
                            const auto underscaledZoom = static_cast<int>(activeZoom) + zoomShift;
                            if (underscaledZoom <= maxZoom)
                                {
                                const auto underscaledTileIdsN = Utilities::getTileIdsUnderscaledByZoomShift(
                                    activeTileId,
                                    zoomShift);
                                const auto tilesCount = underscaledTileIdsN.size();
                                auto pUnderscaledTileIdN = underscaledTileIdsN.constData();
                                for (auto tileIdx = 0; tileIdx < tilesCount; tileIdx++)
                                {
                                    const auto& underscaledTileId = *(pUnderscaledTileIdN++);
                                    neededTilesMap[static_cast<ZoomLevel>(underscaledZoom)].insert(underscaledTileId);
                                }
                            }
                        }
                        for (int zoomShift = 1; zoomShift <= maxMissingDataZoomShift; zoomShift++)
                        {
                            const auto overscaledZoom = static_cast<int>(activeZoom) - zoomShift;
                            if (overscaledZoom >= minZoom)
                            {
                                const auto overscaledTileId = Utilities::getTileIdOverscaledByZoomShift(
                                    activeTileId,
                                    zoomShift);
                                neededTilesMap[static_cast<ZoomLevel>(overscaledZoom)].insert(overscaledTileId);
                            }
                        }
                        continue;                        
                    }

                    if (isComplete)
                        continue;

                    if (isCustomVisibility && (activeZoom < minVisibleZoom || activeZoom > maxVisibleZoom))
                        continue;

                    if (resourcesType == MapRendererResourceType::MapLayer ||
                        resourcesType == MapRendererResourceType::ElevationData)
                    {
                        if (activeZoom < minZoom)
                        {
                            ZoomLevel underscaledZoom = minZoom;
                            int zoomShift = underscaledZoom - activeZoom;
                            if (zoomShift <= maxMissingDataUnderZoomShift)
                            {
                                const auto underscaledTileIdsN = Utilities::getTileIdsUnderscaledByZoomShift(
                                    activeTileId,
                                    zoomShift);

                                const auto tilesCount = underscaledTileIdsN.size();
                                auto pUnderscaledTileIdN = underscaledTileIdsN.constData();
                                for (auto tileIdx = 0; tileIdx < tilesCount; tileIdx++)
                                {
                                    const auto& underscaledTileId = *(pUnderscaledTileIdN++);
                                    neededTilesMap[underscaledZoom].insert(underscaledTileId);
                                }
                            }
                        }
                        else if (activeZoom > maxZoom)
                        {
                            ZoomLevel overscaledZoom = maxZoom;
                            int zoomShift = activeZoom - maxZoom;
                            const auto overscaledTileId = Utilities::getTileIdOverscaledByZoomShift(
                                activeTileId,
                                zoomShift);
                            neededTilesMap[overscaledZoom].insert(overscaledTileId);
                        }

                        // Exact match was not found, so now try to look for overscaled/underscaled resources.
                        // It's better to show Z-"nearest" resource available,
                        // giving preference to underscaled resource.
                        for (int absZoomShift = 1; absZoomShift <= MaxZoomLevel; absZoomShift++)
                        {
                            // Look for underscaled first. Only full match is accepted. Also, underscaled are
                            // limited to provider's maxMissingDataUnderZoomShift to avoid out-of-memory situations.
                            if (Q_LIKELY(!debugSettings->rasterLayersUnderscaleForbidden))
                            {
                                const auto underscaledZoom = static_cast<int>(activeZoom) + absZoomShift;
                                if (underscaledZoom <= static_cast<int>(MaxZoomLevel) &&
                                    absZoomShift <= maxMissingDataUnderZoomShift)
                                {
                                    const auto underscaledTileIdsN = Utilities::getTileIdsUnderscaledByZoomShift(
                                        activeTileId,
                                        absZoomShift);

                                    bool atLeastOnePresent = false;
                                    const auto tilesCount = underscaledTileIdsN.size();
                                    auto pUnderscaledTileIdN = underscaledTileIdsN.constData();
                                    for (auto tileIdx = 0; tileIdx < tilesCount; tileIdx++)
                                    {
                                        const auto& underscaledTileId = *(pUnderscaledTileIdN++);

                                        const auto underscaledTilePresent = tiledResourcesCollection->containsResource(
                                            underscaledTileId,
                                            static_cast<ZoomLevel>(underscaledZoom),
                                            isUsableResource);
                                        if (underscaledTilePresent)
                                        {
                                            neededTilesMap[static_cast<ZoomLevel>(underscaledZoom)].insert(
                                                underscaledTileId);
                                            atLeastOnePresent = true;
                                        }
                                    }
                                    if (atLeastOnePresent)
                                        break;
                                }
                            }

                            // If underscaled was not found, look for overscaled. Overscaled are not limited by
                            // provider's maxMissingDataUnderZoomShift.
                            if (Q_LIKELY(!debugSettings->rasterLayersOverscaleForbidden))
                            {
                                const auto overscaleZoom = static_cast<int>(activeZoom) - absZoomShift;
                                if (overscaleZoom >= static_cast<int>(MinZoomLevel))
                                {
                                    const auto overscaledTileId = Utilities::getTileIdOverscaledByZoomShift(
                                        activeTileId,
                                        absZoomShift);
                                    if (tiledResourcesCollection->containsResource(
                                        overscaledTileId,
                                        static_cast<ZoomLevel>(overscaleZoom),
                                        isUsableResource))
                                    {
                                        // It's needed only if present and ready
                                        neededTilesMap[static_cast<ZoomLevel>(overscaleZoom)].insert(overscaledTileId);

                                        break;
                                    }
                                }
                            }
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
                    if (citNeededTilesAtZoom != neededTilesMap.cend() &&
                        citNeededTilesAtZoom->contains(tiledEntry->tileId))
                    {
                        return false;
                    }

                    // Mark this entry as junk until it will die
                    entry->markAsJunk();

                    return cleanupJunkResource(entry, needsResourcesUploadOrUnload);
                });
        }
    }

    if (!keepOldSymbolResources && atLeastOneScaledPresent)
    {
        renderer->shouldUpdateSymbols();
        _clearOldSymbolResources = false;
    }

    if (needsResourcesUploadOrUnload)
        requestResourcesUploadOrUnload();
}

bool OsmAnd::MapRendererResourcesManager::cleanupJunkResource(
    const std::shared_ptr<MapRendererBaseResource>& resource,
    bool& needsResourcesUploadOrUnload)
{
    if (resource->setStateIf(MapRendererResourceState::Unloaded, MapRendererResourceState::JustBeforeDeath))
    {
        LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Unloaded, MapRendererResourceState::JustBeforeDeath);

        // If resource was unloaded from GPU, remove the entry.
        return true;
    }
    else if (resource->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::UnloadPending))
    {
        LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Uploaded, MapRendererResourceState::UnloadPending);

        // If resource is not needed anymore, change its state to "UnloadPending",
        // but keep the resource entry, since it must be unload from GPU in another place

        needsResourcesUploadOrUnload = true;
        return false;
    }
    else if (resource->setStateIf(MapRendererResourceState::PreparedRenew, MapRendererResourceState::UnloadPending))
    {
        LOG_RESOURCE_STATE_CHANGE(
            resource, MapRendererResourceState::PreparedRenew, MapRendererResourceState::UnloadPending);

        // If resource is not needed anymore, change its state to "UnloadPending",
        // but keep the resource entry, since it must be unload from GPU in another place

        needsResourcesUploadOrUnload = true;
        return false;
    }
    else if (resource->setStateIf(MapRendererResourceState::Outdated, MapRendererResourceState::UnloadPending))
    {
        LOG_RESOURCE_STATE_CHANGE(
            resource, MapRendererResourceState::Outdated, MapRendererResourceState::UnloadPending);

        // If resource is not needed anymore, change its state to "UnloadPending",
        // but keep the resource entry, since it must be unload from GPU in another place

        needsResourcesUploadOrUnload = true;
        return false;
    }
    else if (resource->setStateIf(MapRendererResourceState::Ready, MapRendererResourceState::JustBeforeDeath))
    {
        LOG_RESOURCE_STATE_CHANGE(
            resource,
            MapRendererResourceState::Ready,
            MapRendererResourceState::JustBeforeDeath);

        // If resource was not yet uploaded, just remove it.
        return true;
    }
    else if (resource->setStateIf(MapRendererResourceState::ProcessingRequest, MapRendererResourceState::RequestCanceledWhileBeingProcessed))
    {
        LOG_RESOURCE_STATE_CHANGE(
            resource,
            MapRendererResourceState::ProcessingRequest,
            MapRendererResourceState::RequestCanceledWhileBeingProcessed);

        // If resource request is being processed, keep the entry until processing is complete.

        // Cancel the task
        if (resource->type == MapRendererResourceType::MapLayer && resource->supportsObtainDataAsync())
        {
            assert(resource->_cancelRequestCallback != nullptr);
            resource->_cancelRequestCallback();
        }

        return false;
    }
    else if (resource->setStateIf(MapRendererResourceState::ProcessingUpdate,
        MapRendererResourceState::UpdatingCancelledWhileBeingProcessed))
    {
        LOG_RESOURCE_STATE_CHANGE(
            resource,
            MapRendererResourceState::ProcessingUpdate,
            MapRendererResourceState::UpdatingCancelledWhileBeingProcessed);

        // If resource request is being processed, keep the entry until processing is complete.

        // Cancel the task
        if (resource->type == MapRendererResourceType::MapLayer && resource->supportsObtainDataAsync())
        {
            assert(resource->_cancelRequestCallback != nullptr);
            resource->_cancelRequestCallback();
        }

        return false;
    }
    else if (resource->setStateIf(MapRendererResourceState::ProcessingUpdateWhileRenewing,
        MapRendererResourceState::UpdatingCancelledWhileBeingProcessed))
    {
        LOG_RESOURCE_STATE_CHANGE(
            resource,
            MapRendererResourceState::ProcessingUpdateWhileRenewing,
            MapRendererResourceState::UpdatingCancelledWhileBeingProcessed);

        // If resource request is being processed, keep the entry until processing is complete.

        // Cancel the task
        if (resource->type == MapRendererResourceType::MapLayer && resource->supportsObtainDataAsync())
        {
            assert(resource->_cancelRequestCallback != nullptr);
            resource->_cancelRequestCallback();
        }

        return false;
    }
    else if (resource->setStateIf(MapRendererResourceState::Requested, MapRendererResourceState::JustBeforeDeath))
    {
        LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Requested, MapRendererResourceState::JustBeforeDeath);

        // If resource was just requested, cancel its task and remove the entry.

        // Cancel the task
        assert(resource->_cancelRequestCallback != nullptr);
        resource->_cancelRequestCallback();

        return true;
    }
    else if (resource->setStateIf(MapRendererResourceState::RequestedUpdate, MapRendererResourceState::UnloadPending))
    {
        LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::RequestedUpdate,
            MapRendererResourceState::UnloadPending);

        // If resource was just requested, cancel its task and remove the entry.

        // Cancel the task
        assert(resource->_cancelRequestCallback != nullptr);
        resource->_cancelRequestCallback();

        return false;
    }
    else if (resource->setStateIf(MapRendererResourceState::Unavailable, MapRendererResourceState::JustBeforeDeath))
    {
        LOG_RESOURCE_STATE_CHANGE(resource, MapRendererResourceState::Unavailable, MapRendererResourceState::JustBeforeDeath);

        // If resource was never available, just remove the entry
        return true;
    }

    const auto state = resource->getState();
    if (state == MapRendererResourceState::JustBeforeDeath)
    {
        // If resource has JustBeforeDeath state, it means that it will be deleted from different thread in next few moments,
        // so it's safe to remove it here also (since operations with resources removal from collection are atomic, and
        // collection is blocked)
        return true;
    }

    // Resources with states
    //  - Uploading (to allow finish uploading)
    //  - PreparingRenew (to allow finish preparation)
    //  - Renewing (to allow finish uploading)
    //  - UnloadPending (to allow start unloading from GPU)
    //  - Unloading (to allow finish unloading)
    //  - RequestCanceledWhileBeingProcessed (to allow cleanup of the process)
    //  - UpdatingCancelledWhileBeingProcessed (to allow cleanup of the process)
    // should be retained, since they are being processed. So try to next time
    return false;
}

void OsmAnd::MapRendererResourcesManager::addCombinedTilesToAvoidCleanup(
    const std::shared_ptr<IMapRendererTiledResourcesCollection>& resourcesCollection,
    QMap< ZoomLevel, QVector<TileId> >& tilesByZoom)
{
    for (auto itTilesEntry = tilesByZoom.begin(); itTilesEntry != tilesByZoom.end(); itTilesEntry++)
    {
        const auto zoom = itTilesEntry.key();
        auto& tiles = itTilesEntry.value();

        if (zoom == ZoomLevel::MinZoomLevel)
            continue;

        // Collect all overscaled tiles
        QSet<TileId> overscaledTileIds;
        for (const auto tileId : tiles)
            overscaledTileIds.insert(Utilities::getTileIdOverscaledByZoomShift(tileId, 1));

        for (const auto overscaledTileId : overscaledTileIds)
        {
            const auto metaTileIds = Utilities::getMetaTileIds(overscaledTileId);
            
            for (const auto tileId : metaTileIds)
            {
                // Add meta tile if it isn't present
                if (!tiles.contains(tileId))
                {
                    std::shared_ptr<MapRendererBaseTiledResource> resource;
                    if (resourcesCollection->obtainResource(tileId, zoom, resource) && resource->isMetaTiled)
                        tiles.push_back(tileId);
                }
            }
        }
    }
}

void OsmAnd::MapRendererResourcesManager::blockingReleaseResourcesFrom(
    const std::shared_ptr<MapRendererBaseResourcesCollection>& collection, bool gpuContextLost)
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
                // Block possible attempts to access renderer in finishing async tasks
                entry->shouldLeaveQuietly();

                // When resources are released, it's a termination process, so all entries have to be removed.
                // This limits set of possible states of the entries to:
                // - Requested
                // - ProcessingRequest
                // - RequestedUpdate
                // - ProcessingUpdate
                // - ProcessingUpdateWhileRenewing
                // - Ready
                // - Unloaded
                // - Uploaded
                // - Outdated
                // - PreparedRenew
                // - Unavailable
                // All other states are considered invalid.

                if (entry->setStateIf(MapRendererResourceState::Requested, MapRendererResourceState::JustBeforeDeath))
                {
                    LOG_RESOURCE_STATE_CHANGE(
                        entry,
                        MapRendererResourceState::Requested,
                        MapRendererResourceState::JustBeforeDeath);

                    // Cancel the task
                    assert(entry->_cancelRequestCallback != nullptr);
                    entry->_cancelRequestCallback();

                    return true;
                }
                else if (entry->setStateIf(MapRendererResourceState::ProcessingRequest, MapRendererResourceState::RequestCanceledWhileBeingProcessed))
                {
                    LOG_RESOURCE_STATE_CHANGE(
                        entry,
                        MapRendererResourceState::ProcessingRequest,
                        MapRendererResourceState::RequestCanceledWhileBeingProcessed);

                    containedUnprocessableResources = true;

                    return false;
                }
                else if (entry->setStateIf(MapRendererResourceState::RequestedUpdate,
                    MapRendererResourceState::UnloadPending))
                {
                    LOG_RESOURCE_STATE_CHANGE(
                        entry,
                        MapRendererResourceState::RequestedUpdate,
                        MapRendererResourceState::UnloadPending);

                    // In case context was lost, it's impossible to unload anything, so just remove the resource
                    if (gpuContextLost)
                    {
                        entry->lostDataInGPU();
                        return true;
                    }

                    // Cancel the task
                    assert(entry->_cancelRequestCallback != nullptr);
                    entry->_cancelRequestCallback();

                    // If resource is not needed anymore, change its state to "UnloadPending",
                    // but keep the resource entry, since it must be unload from GPU in another place
                    needsResourcesUploadOrUnload = true;
                    containedUnprocessableResources = true;

                    return false;
                }
                else if (entry->setStateIf(MapRendererResourceState::ProcessingUpdate,
                    MapRendererResourceState::UpdatingCancelledWhileBeingProcessed))
                {
                    LOG_RESOURCE_STATE_CHANGE(
                        entry,
                        MapRendererResourceState::ProcessingUpdate,
                        MapRendererResourceState::UpdatingCancelledWhileBeingProcessed);

                    containedUnprocessableResources = true;

                    return false;
                }
                else if (entry->setStateIf(MapRendererResourceState::ProcessingUpdateWhileRenewing,
                    MapRendererResourceState::UpdatingCancelledWhileBeingProcessed))
                {
                    LOG_RESOURCE_STATE_CHANGE(
                        entry,
                        MapRendererResourceState::ProcessingUpdateWhileRenewing,
                        MapRendererResourceState::UpdatingCancelledWhileBeingProcessed);

                    containedUnprocessableResources = true;

                    return false;
                }
                else if (entry->setStateIf(MapRendererResourceState::Ready, MapRendererResourceState::JustBeforeDeath))
                {
                    LOG_RESOURCE_STATE_CHANGE(
                        entry,
                        MapRendererResourceState::Ready,
                        MapRendererResourceState::JustBeforeDeath);

                    return true;
                }
                else if (entry->setStateIf(MapRendererResourceState::Unloaded, MapRendererResourceState::JustBeforeDeath))
                {
                    LOG_RESOURCE_STATE_CHANGE(
                        entry,
                        MapRendererResourceState::Unloaded,
                        MapRendererResourceState::JustBeforeDeath);

                    return true;
                }
                else if (entry->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::UnloadPending))
                {
                    LOG_RESOURCE_STATE_CHANGE(
                        entry,
                        MapRendererResourceState::Uploaded,
                        MapRendererResourceState::UnloadPending);

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
                else if (entry->setStateIf(MapRendererResourceState::Outdated,
                    MapRendererResourceState::UnloadPending))
                {
                    LOG_RESOURCE_STATE_CHANGE(
                        entry,
                        MapRendererResourceState::Outdated,
                        MapRendererResourceState::UnloadPending);

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
                else if (entry->setStateIf(MapRendererResourceState::PreparedRenew,
                    MapRendererResourceState::UnloadPending))
                {
                    LOG_RESOURCE_STATE_CHANGE(
                        entry,
                        MapRendererResourceState::PreparedRenew,
                        MapRendererResourceState::UnloadPending);

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
                    LOG_RESOURCE_STATE_CHANGE(
                        entry,
                        MapRendererResourceState::Unavailable,
                        MapRendererResourceState::JustBeforeDeath);

                    return true;
                }

                const auto state = entry->getState();
                if (state == MapRendererResourceState::JustBeforeDeath)
                {
                    // If resource has JustBeforeDeath state, it means that it will be deleted from different thread
                    // in next few moments, so it's safe to remove it here also (since operations with resources removal
                    // from collection are atomic, and collection is blocked)
                    return true;
                }

                // Resources with states
                //  - Uploading (to allow finish uploading)
                //  - Renewing (to allow finish uploading)
                //  - UnloadPending (to allow start unloading from GPU)
                //  - Unloading (to allow finish unloading)
                // should be retained, since they are being processed. So try to next time
                if (state == MapRendererResourceState::Uploading ||
                    state == MapRendererResourceState::Renewing ||
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

                // In case GPU context is lost, nothing can be done with this resource, so just remove it
                if (gpuContextLost)
                {
                    entry->lostDataInGPU();
                    return true;
                }
                else if (const auto tiledEntry = std::dynamic_pointer_cast<const MapRendererBaseTiledResource>(entry))
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
                {
                    QMutexLocker scopedLocker(&gpuResourcesSyncStageExecutedOnceMutex);

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

void OsmAnd::MapRendererResourcesManager::releaseAllResources(bool gpuContextLost)
{
    QWriteLocker scopedLocker(&_resourcesStoragesLock);

    _requestedResourcesTasks.clear();
    _resourcesRequestWorkerPool.dequeueAll();
    _resourcesRequestWorkerPool.waitForDone();
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
    QConditionalReadLocker scopedLocker(&_resourcesStoragesLock, !renderer->isInRenderThread());

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
    QConditionalReadLocker scopedLocker(&_resourcesStoragesLock, !renderer->isInRenderThread());

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
    return collection->getCollectionSnapshot();
}

bool OsmAnd::MapRendererResourcesManager::eachResourceIsUploadedOrUnavailable() const
{
    QConditionalReadLocker scopedLocker(&_resourcesStoragesLock, !renderer->isInRenderThread());

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
                    if (resourceState != MapRendererResourceState::Uploaded &&
                        resourceState != MapRendererResourceState::Unavailable)
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
    QConditionalReadLocker scopedLocker(&_resourcesStoragesLock, !renderer->isInRenderThread());

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
    resourceStateMap.insert(MapRendererResourceState::RequestCanceledWhileBeingProcessed,
        QLatin1String("RequestCanceledWhileBeingProcessed"));
    resourceStateMap.insert(MapRendererResourceState::Unavailable, QLatin1String("Unavailable"));
    resourceStateMap.insert(MapRendererResourceState::Ready, QLatin1String("Ready"));
    resourceStateMap.insert(MapRendererResourceState::Uploading, QLatin1String("Uploading"));
    resourceStateMap.insert(MapRendererResourceState::Uploaded, QLatin1String("Uploaded"));
    resourceStateMap.insert(MapRendererResourceState::IsBeingUsed, QLatin1String("IsBeingUsed"));
    resourceStateMap.insert(MapRendererResourceState::UnloadPending, QLatin1String("UnloadPending"));
    resourceStateMap.insert(MapRendererResourceState::Unloading, QLatin1String("Unloading"));
    resourceStateMap.insert(MapRendererResourceState::Unloaded, QLatin1String("Unloaded"));
    resourceStateMap.insert(MapRendererResourceState::PreparingRenew, QLatin1String("PreparingRenew"));
    resourceStateMap.insert(MapRendererResourceState::PreparedRenew, QLatin1String("PreparedRenew"));
    resourceStateMap.insert(MapRendererResourceState::Renewing, QLatin1String("Renewing"));
    resourceStateMap.insert(MapRendererResourceState::Outdated, QLatin1String("Outdated"));
    resourceStateMap.insert(MapRendererResourceState::Updating, QLatin1String("Updating"));
    resourceStateMap.insert(MapRendererResourceState::RequestedUpdate, QLatin1String("RequestedUpdate"));
    resourceStateMap.insert(MapRendererResourceState::ProcessingUpdate, QLatin1String("ProcessingUpdate"));
    resourceStateMap.insert(MapRendererResourceState::ProcessingUpdateWhileRenewing,
        QLatin1String("ProcessingUpdateWhileRenewing"));
    resourceStateMap.insert(MapRendererResourceState::UpdatingCancelledWhileBeingProcessed,
        QLatin1String("UpdatingCancelledWhileBeingProcessed"));
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

float OsmAnd::MapRendererResourcesManager::getBasicThreadsCPULoad()
{
    QMutexLocker scopedLocker(&_threadCountLock);

    float result = NAN;
    if (_threadCountPeriod >= 1.0f)
    {
        result = _basicThreadsCPULoad / _threadCountPeriod;
        _threadCountTime = std::chrono::high_resolution_clock::now();
        _threadCountPeriod = 0.0f;
        _basicThreadsCPULoad = 0.0f;
    }
    return result;
}

OsmAnd::MapRendererResourcesManager::ResourceRequestTask::ResourceRequestTask(
    const std::shared_ptr<MapRendererBaseResource>& requestedResource_,
    const Concurrent::TaskHost::Bridge& bridge_)
    : HostedTask(bridge_, executeWrapper, nullptr, postExecuteWrapper)
    , manager(reinterpret_cast<MapRendererResourcesManager*>(lockedOwner))
    , requestedResource(requestedResource_)
{
    manager->_resourcesRequestTasksCounter.fetchAndAddOrdered(1);
}

OsmAnd::MapRendererResourcesManager::ResourceRequestTask::~ResourceRequestTask()
{
    manager->_resourcesRequestTasksCounter.fetchAndSubOrdered(1);
}

void OsmAnd::MapRendererResourcesManager::ResourceRequestTask::execute()
{
    if (!manager->beginResourceRequestProcessing(requestedResource))
        return;

    manager->reportActiveThread(1.0f);

    // Ask resource to obtain it's data
    bool dataAvailable = false;
    const std::shared_ptr<FunctorQueryController> obtainDataQueryController(new FunctorQueryController(
        [this]
        (const FunctorQueryController* const queryController) -> bool
        {
            return isCancellationRequested();
        }));
    const auto requestSucceeded =
        requestedResource->obtainData(dataAvailable, obtainDataQueryController) &&
        !isCancellationRequested();

    manager->endResourceRequestProcessing(requestedResource, requestSucceeded, dataAvailable);

    manager->reportActiveThread(-1.0f);
}

void OsmAnd::MapRendererResourcesManager::ResourceRequestTask::postExecute(const bool wasCancelled)
{
    if (wasCancelled)
        manager->processResourceRequestCancellation(requestedResource);
}

void OsmAnd::MapRendererResourcesManager::ResourceRequestTask::executeWrapper(Task* const task_)
{
    const auto task = static_cast<ResourceRequestTask*>(task_);
    task->execute();
}

void OsmAnd::MapRendererResourcesManager::ResourceRequestTask::postExecuteWrapper(Task* const task_, const bool wasCancelled)
{
    const auto task = static_cast<ResourceRequestTask*>(task_);
    task->postExecute(wasCancelled);
}

int64_t OsmAnd::MapRendererResourcesManager::ResourceRequestTask::calculatePriority(
    const TileId centerTileId,
    const QVector<TileId>& activeTiles,
    const ZoomLevel activeZoom) const
{
    // Priority calculation does not need to be stable

    // Special keyed resources have exactly set priority
    if (auto symbol = std::dynamic_pointer_cast<MapRendererKeyedSymbolsResource>(requestedResource))
        return symbol->getPriority();

    // Ordinary keyed resources have minimal priority always
    if (std::dynamic_pointer_cast<MapRendererBaseKeyedResource>(requestedResource))
        return std::numeric_limits<int64_t>::min();

    const auto tiledResource = std::dynamic_pointer_cast<MapRendererBaseTiledResource>(requestedResource);
    if (!tiledResource)
        return 0;

    // The closer tiled resource coordinates are from center, the higher priority it has
    auto priority = std::numeric_limits<int64_t>::max() - 1000000000;

    switch (tiledResource->type)
    {
        case MapRendererResourceType::MapLayer:
            // Do nothing, since MapLayer resources are most important
            break;
        case MapRendererResourceType::ElevationData:
            priority -= 1000000000;
            break;
        case MapRendererResourceType::Symbols:
            priority -= 2000000000;
            break;
        default:
            priority -= 3000000000;
            break;
    }

    priority -= qAbs(static_cast<int>(tiledResource->zoom) - static_cast<int>(activeZoom)) * 10000000;

    const auto dX = tiledResource->tileId.x - centerTileId.x;
    const auto dY = tiledResource->tileId.y - centerTileId.y;
    priority -= dX*dX + dY*dY;

    return priority;
}
