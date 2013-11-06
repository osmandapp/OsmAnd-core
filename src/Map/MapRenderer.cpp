#include "MapRenderer.h"

#include <cassert>
#include <algorithm>

#include <QMutableHashIterator>

#include <SkBitmap.h>
#include <SkImageDecoder.h>

#include "IMapBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "IMapSymbolProvider.h"
#include "IRetainedMapTile.h"
#include "RenderAPI.h"
#include "EmbeddedResources.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::MapRenderer::MapRenderer()
    : _taskHostBridge(this)
    , _currentConfigurationInvalidatedMask(0)
    , _requestedStateUpdatedMask(0)
    , _invalidatedResourcesTypesMask(0)
    , _renderThreadId(nullptr)
    , _workerThreadId(nullptr)
    , currentConfiguration(_currentConfiguration)
    , currentState(_currentState)
    , providersAndResourcesBindings(_providersAndResourcesBindings)
    , resources(_resources)
    , renderAPI(_renderAPI)
{
    // Number of workers should be determined in runtime (exclude worker and main threads):
    const auto idealThreadCount = qMax(QThread::idealThreadCount() - 2, 1);
    _resourcesRequestWorkersPool.setMaxThreadCount(idealThreadCount);

    // Fill-up default state
    for(auto layerId = 0u; layerId < RasterMapLayersCount; layerId++)
        setRasterLayerOpacity(static_cast<RasterMapLayerId>(layerId), 1.0f);
    setElevationDataScaleFactor(1.0f, true);
    setFieldOfView(16.5f, true);
    setDistanceToFog(400.0f, true);
    setFogOriginFactor(0.36f, true);
    setFogHeightOriginFactor(0.05f, true);
    setFogDensity(1.9f, true);
    setFogColor(FColorRGB(1.0f, 0.0f, 0.0f), true);
    setSkyColor(FColorRGB(140.0f / 255.0f, 190.0f / 255.0f, 214.0f / 255.0f), true);
    setAzimuth(0.0f, true);
    setElevationAngle(45.0f, true);
    const auto centerIndex = 1u << (ZoomLevel::MaxZoomLevel - 1);
    setTarget(PointI(centerIndex, centerIndex), true);
    setZoom(0, true);
}

OsmAnd::MapRenderer::~MapRenderer()
{
    releaseRendering();

    _taskHostBridge.onOwnerIsBeingDestructed();
}

bool OsmAnd::MapRenderer::setup( const MapRendererSetupOptions& setupOptions )
{
    // We can not change setup options renderer once rendering has been initialized
    if(_isRenderingInitialized)
        return false;

    _setupOptions = setupOptions;

    return true;
}

void OsmAnd::MapRenderer::setConfiguration( const MapRendererConfiguration& configuration_, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_configurationLock);

    const bool colorDepthForcingChanged = (_requestedConfiguration.limitTextureColorDepthBy16bits != configuration_.limitTextureColorDepthBy16bits);
    const bool atlasTexturesUsageChanged = (_requestedConfiguration.altasTexturesAllowed != configuration_.altasTexturesAllowed);
    const bool elevationDataResolutionChanged = (_requestedConfiguration.heixelsPerTileSide != configuration_.heixelsPerTileSide);
    const bool texturesFilteringChanged = (_requestedConfiguration.texturesFilteringQuality != configuration_.texturesFilteringQuality);
    const bool paletteTexturesUsageChanged = (_requestedConfiguration.paletteTexturesAllowed != configuration_.paletteTexturesAllowed);

    bool invalidateRasterTextures = false;
    invalidateRasterTextures = invalidateRasterTextures || colorDepthForcingChanged;
    invalidateRasterTextures = invalidateRasterTextures || atlasTexturesUsageChanged;
    invalidateRasterTextures = invalidateRasterTextures || paletteTexturesUsageChanged;

    bool invalidateElevationData = false;
    invalidateElevationData = invalidateElevationData || elevationDataResolutionChanged;

    bool update = forcedUpdate;
    update = update || invalidateRasterTextures;
    update = update || invalidateElevationData;
    update = update || texturesFilteringChanged;
    if(!update)
        return;

    _requestedConfiguration = configuration_;
    uint32_t mask = 0;
    if(colorDepthForcingChanged)
        mask |= ConfigurationChange::ColorDepthForcing;
    if(atlasTexturesUsageChanged)
        mask |= ConfigurationChange::AtlasTexturesUsage;
    if(elevationDataResolutionChanged)
        mask |= ConfigurationChange::ElevationDataResolution;
    if(texturesFilteringChanged)
        mask |= ConfigurationChange::TexturesFilteringMode;
    if(paletteTexturesUsageChanged)
        mask |= ConfigurationChange::PaletteTexturesUsage;

    if(invalidateRasterTextures)
        invalidateResourcesOfType(ResourceType::RasterMap);
    if(invalidateElevationData)
        invalidateResourcesOfType(ResourceType::ElevationData);
    invalidateCurrentConfiguration(mask);
}

void OsmAnd::MapRenderer::invalidateCurrentConfiguration(const uint32_t changesMask)
{
    _currentConfigurationInvalidatedMask = changesMask;

    // Since our current configuration is invalid, frame is also invalidated
    invalidateFrame();
}

bool OsmAnd::MapRenderer::updateCurrentConfiguration()
{
    uint32_t bitIndex = 0;
    while(_currentConfigurationInvalidatedMask)
    {
        if(_currentConfigurationInvalidatedMask & 0x1)
            validateConfigurationChange(static_cast<ConfigurationChange>(1 << bitIndex));

        bitIndex++;
        _currentConfigurationInvalidatedMask >>= 1;
    }

    return true;
}

bool OsmAnd::MapRenderer::revalidateState()
{
    bool ok;

    // Capture new state
    MapRendererState newState;
    uint32_t updatedMask;
    {
        QMutexLocker scopedLocker(&_requestedStateMutex);

        // Check if requested state was updated at all
        if(!_requestedStateUpdatedMask)
            return true;

        // Capture new state and mask
        newState = _requestedState;
        updatedMask = _requestedStateUpdatedMask;

        // And pull down mask that requested state was updated
        _requestedStateUpdatedMask = 0;
    }

    // Save new state as current
    _currentState = newState;

    // Process updating of providers
    updateProvidersAndResourcesBindings(updatedMask);

    // Update internal state, that is derived from current state
    const auto internalState = getInternalStateRef();
    ok = updateInternalState(internalState, _currentState);

    // Postprocess internal state
    if(ok)
    {
        // Sort visible tiles by distance from target
        qSort(internalState->visibleTiles.begin(), internalState->visibleTiles.end(), [this, internalState](const TileId& l, const TileId& r) -> bool
        {
            const auto lx = l.x - internalState->targetTileId.x;
            const auto ly = l.y - internalState->targetTileId.y;

            const auto rx = r.x - internalState->targetTileId.x;
            const auto ry = r.y - internalState->targetTileId.y;

            return (lx*lx + ly*ly) > (rx*rx + ry*ry);
        });
    }

    // Frame is being invalidated anyways, since a refresh is needed due to state change (successful or not)
    invalidateFrame();

    // If there was an error, mark current state as updated again
    if(!ok)
    {
        QMutexLocker scopedLocker(&_requestedStateMutex);
        _requestedStateUpdatedMask = updatedMask;

        return false;
    }

    return true;
}

void OsmAnd::MapRenderer::notifyRequestedStateWasUpdated(const StateChange change)
{
    _requestedStateUpdatedMask |= 1u << static_cast<int>(change);

    // Since our current state is invalid, frame is also invalidated
    invalidateFrame();
}

bool OsmAnd::MapRenderer::updateInternalState(InternalState* internalState, const MapRendererState& state)
{
    const auto zoomDiff = ZoomLevel::MaxZoomLevel - state.zoomBase;

    // Get target tile id
    internalState->targetTileId.x = state.target31.x >> zoomDiff;
    internalState->targetTileId.y = state.target31.y >> zoomDiff;

    // Compute in-tile offset
    PointI targetTile31;
    targetTile31.x = internalState->targetTileId.x << zoomDiff;
    targetTile31.y = internalState->targetTileId.y << zoomDiff;

    const auto tileWidth31 = 1u << zoomDiff;
    const auto inTileOffset = state.target31 - targetTile31;
    internalState->targetInTileOffsetN.x = static_cast<float>(inTileOffset.x) / tileWidth31;
    internalState->targetInTileOffsetN.y = static_cast<float>(inTileOffset.y) / tileWidth31;

    return true;
}

void OsmAnd::MapRenderer::updateProvidersAndResourcesBindings(const uint32_t updatedMask)
{
    if(updatedMask & (1u << static_cast<int>(StateChange::ElevationData_Provider)))
    {
        auto& bindings = _providersAndResourcesBindings[static_cast<int>(ResourceType::ElevationData)];
        auto& resources = _resources[static_cast<int>(ResourceType::ElevationData)];

        // Clean-up and unbind gone providers and their resources
        QMutableHashIterator< std::shared_ptr<IMapProvider>, std::shared_ptr<TiledResources> > itBindedProvider(bindings.providersToResources);
        while(itBindedProvider.hasNext())
        {
            itBindedProvider.next();

            // Skip binding if it's still active
            if(itBindedProvider.key() == _currentState.elevationDataProvider)
                continue;
            
            // Clean-up resources
            releaseResourcesFrom(itBindedProvider.value());

            // Remove binding
            bindings.resourcesToProviders.remove(itBindedProvider.value());
            itBindedProvider.remove();

            // Remove resources collection
            resources.removeOne(itBindedProvider.value());
        }

        // Create new binding and storage
        if(_currentState.elevationDataProvider && !bindings.providersToResources.contains(std::static_pointer_cast<IMapProvider>(_currentState.elevationDataProvider)))
        {
            // Create new resources collection
            const std::shared_ptr< TiledResources > newResourcesCollection(new TiledResources(ResourceType::ElevationData));

            // Add binding
            bindings.providersToResources.insert(_currentState.elevationDataProvider, newResourcesCollection);
            bindings.resourcesToProviders.insert(newResourcesCollection, _currentState.elevationDataProvider);

            // Add resources collection
            resources.append(newResourcesCollection);
        }
    }
    if(updatedMask & (1u << static_cast<int>(StateChange::RasterLayers_Providers)))
    {
        auto& bindings = _providersAndResourcesBindings[static_cast<int>(ResourceType::RasterMap)];
        auto& resources = _resources[static_cast<int>(ResourceType::RasterMap)];

        // Clean-up and unbind gone providers and their resources
        QMutableHashIterator< std::shared_ptr<IMapProvider>, std::shared_ptr<TiledResources> > itBindedProvider(bindings.providersToResources);
        while(itBindedProvider.hasNext())
        {
            itBindedProvider.next();

            // Skip binding if it's still active
            if(std::find(
                _currentState.rasterLayerProviders.cbegin(), _currentState.rasterLayerProviders.cend(),
                std::static_pointer_cast<IMapBitmapTileProvider>(itBindedProvider.key())) != _currentState.rasterLayerProviders.cend())
            {
                continue;
            }

            // Clean-up resources
            releaseResourcesFrom(itBindedProvider.value());

            // Reset reference to resources collection, but keep the space in array
            qFind(resources.begin(), resources.end(), itBindedProvider.value())->reset();

            // Remove binding
            bindings.resourcesToProviders.remove(itBindedProvider.value());
            itBindedProvider.remove();
        }

        // Create new binding and storage
        auto rasterLayerIdx = 0;
        for(auto itProvider = _currentState.rasterLayerProviders.cbegin(); itProvider != _currentState.rasterLayerProviders.cend(); ++itProvider, rasterLayerIdx++)
        {
            // Skip empty providers
            if(!*itProvider)
                continue;

            // If binding already exists, skip creation
            if(bindings.providersToResources.contains(std::static_pointer_cast<IMapProvider>(*itProvider)))
                continue;

            // Create new resources collection
            const std::shared_ptr< TiledResources > newResourcesCollection(new TiledResources(ResourceType::RasterMap));

            // Add binding
            bindings.providersToResources.insert(*itProvider, newResourcesCollection);
            bindings.resourcesToProviders.insert(newResourcesCollection, *itProvider);

            // Add resources collection
            assert(rasterLayerIdx < resources.size());
            assert(static_cast<bool>(resources[rasterLayerIdx]) == false);
            resources[rasterLayerIdx] = newResourcesCollection;
        }
    }
    if(updatedMask & (1u << static_cast<int>(StateChange::Symbols_Providers)))
    {
        auto& bindings = _providersAndResourcesBindings[static_cast<int>(ResourceType::Symbols)];
        auto& resources = _resources[static_cast<int>(ResourceType::Symbols)];

        // Clean-up and unbind gone providers and their resources
        QMutableHashIterator< std::shared_ptr<IMapProvider>, std::shared_ptr<TiledResources> > itBindedProvider(bindings.providersToResources);
        while(itBindedProvider.hasNext())
        {
            itBindedProvider.next();

            // Skip binding if it's still active
            if(_currentState.symbolProviders.contains(std::static_pointer_cast<IMapSymbolProvider>(itBindedProvider.key())))
                continue;
            
            // Clean-up resources
            releaseResourcesFrom(itBindedProvider.value());

            // Remove resources collection
            resources.removeOne(itBindedProvider.value());

            // Remove binding
            bindings.resourcesToProviders.remove(itBindedProvider.value());
            itBindedProvider.remove();
        }

        // Create new binding and storage
        for(auto itProvider = _currentState.symbolProviders.cbegin(); itProvider != _currentState.symbolProviders.cend(); ++itProvider)
        {
            // If binding already exists, skip creation
            if(bindings.providersToResources.contains(std::static_pointer_cast<IMapProvider>(*itProvider)))
                continue;

            // Create new resources collection
            const std::shared_ptr< TiledResources > newResourcesCollection(new TiledResources(ResourceType::Symbols));

            // Add binding
            bindings.providersToResources.insert(*itProvider, newResourcesCollection);
            bindings.resourcesToProviders.insert(newResourcesCollection, *itProvider);

            // Add resources collection
            resources.append(newResourcesCollection);
        }
    }
}

void OsmAnd::MapRenderer::requestNeededResources()
{
    // Request missing resources
    for(auto itTileId = _uniqueTiles.cbegin(); itTileId != _uniqueTiles.cend(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        for(auto itResourcesCollections = _resources.cbegin(); itResourcesCollections != _resources.cend(); ++itResourcesCollections)
        {
            auto& resourcesCollections = *itResourcesCollections;

            for(auto itResourcesCollection = resourcesCollections.cbegin(); itResourcesCollection != resourcesCollections.cend(); ++itResourcesCollection)
            {
                const auto& resourcesCollection = *itResourcesCollection;

                // Skip empty entries
                if(!static_cast<bool>(resourcesCollection))
                    continue;

                const auto resourceType = resourcesCollection->type;

                // Skip resource types that do not have an available data source
                if(!isDataSourceAvailableFor(resourcesCollection))
                    continue;

                // Obtain a resource entry and if it's state is "Unknown", create a task that will
                // request resource data
                std::shared_ptr<TiledResourceEntry> entry;
                resourcesCollection->obtainOrAllocateEntry(entry, tileId, _currentState.zoomBase,
                    [this, resourceType](const TilesCollection<TiledResourceEntry>& collection, const TileId tileId, const ZoomLevel zoom) -> TiledResourceEntry*
                {
                    if(resourceType == ResourceType::ElevationData || resourceType == ResourceType::RasterMap)
                        return new MapTileResourceEntry(this, resourceType, collection, tileId, zoom);
                    else if(resourceType == ResourceType::Symbols)
                        return new SymbolsResourceEntry(this, collection, tileId, zoom);
                    else
                        return nullptr;
                });

                // Only if tile entry has "Unknown" state proceed to "Requesting" state
                if(!entry->setStateIf(ResourceState::Unknown, ResourceState::Requesting))
                    continue;

                // Create async-task that will obtain needed resource data
                const auto executeProc = [this](Concurrent::Task* task_, QEventLoop& eventLoop)
                {
                    const auto task = static_cast<TileRequestTask*>(task_);
                    const auto entry = task->requestedEntry;

                    // Only if resource entry has "Requested" state proceed to "ProcessingRequest" state
                    if(!entry->setStateIf(ResourceState::Requested, ResourceState::ProcessingRequest))
                    {
                        // This actually can happen in following situation(s):
                        //   - if request task was canceled before has started it's execution,
                        //     since in that case a state change "Requested => JustBeforeDeath" must have happend.
                        //     In this case entry will be removed in post-execute handler.
                        assert(entry->getState() == ResourceState::JustBeforeDeath);
                        return;
                    }

                    // Ask resource to obtain it's data
                    bool dataAvailable = false;
                    const auto requestSucceeded = entry->obtainData(dataAvailable);

                    // If failed to obtain resource data, remove resource entry to repeat try later
                    if(!requestSucceeded)
                    {
                        // It's safe to simply remove entry, since it's not yet uploaded
                        if(const auto link = entry->link.lock())
                            link->collection.removeEntry(entry->tileId, entry->zoom);
                        return;
                    }

                    // Finalize execution of task
                    if(!entry->setStateIf(ResourceState::ProcessingRequest, dataAvailable ? ResourceState::Ready : ResourceState::Unavailable))
                    {
                        assert(entry->getState() == ResourceState::JustBeforeDeath);

                        // While request was processed, state may have changed to "JustBeforeDeath"
                        // to indicate that this request was sort of "canceled"
                        task->requestCancellation();
                        return;
                    }
                    entry->_requestTask = nullptr;
                        
                    // There is data to upload to GPU, request uploading. Or just ask to show that resource is unavailable
                    if(dataAvailable)
                        requestResourcesUpload();
                    else
                        invalidateFrame();
                };
                const auto postExecuteProc = [this](Concurrent::Task* task_, bool wasCancelled)
                {
                    const auto task = static_cast<const TileRequestTask*>(task_);
                    const auto entry = task->requestedEntry;

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
                            entry->setStateIf(ResourceState::Requested, ResourceState::JustBeforeDeath) ||
                            entry->setStateIf(ResourceState::Ready, ResourceState::JustBeforeDeath) ||
                            entry->setStateIf(ResourceState::Unavailable, ResourceState::JustBeforeDeath))
                        {
                            if(const auto link = entry->link.lock())
                                link->collection.removeEntry(entry->tileId, entry->zoom);
                        }
                            
                        // All other cases must be handled in other places, since here there is no access to GPU
                    }
                };
                const auto asyncTask = new TileRequestTask(entry, _taskHostBridge, executeProc, nullptr, postExecuteProc);

                // Register tile as requested
                entry->_requestTask = asyncTask;
                assert(entry->getState() == ResourceState::Requesting);
                entry->setState(ResourceState::Requested);

                // Finally start the request
                _resourcesRequestWorkersPool.start(asyncTask);
            }
        }
    }
}

void OsmAnd::MapRenderer::uploadResources()
{
    const auto isOnRenderThread = (QThread::currentThreadId() == _renderThreadId);
    bool didUpload = false;
    bool triedToUpload = false;

    for(auto itResourcesCollections = _resources.cbegin(); itResourcesCollections != _resources.cend(); ++itResourcesCollections)
    {
        const auto& resourcesCollections = *itResourcesCollections;

        for(auto itResourcesCollection = resourcesCollections.cbegin(); itResourcesCollection != resourcesCollections.cend(); ++itResourcesCollection)
        {
            const auto& resourcesCollection = *itResourcesCollection;

            // Skip empty entries
            if(!static_cast<bool>(resourcesCollection))
                continue;

            // Select all entries with "Ready" state
            QList< std::shared_ptr<TiledResourceEntry> > entries;
            resourcesCollection->obtainEntries(&entries, [&entries, isOnRenderThread](const std::shared_ptr<TiledResourceEntry>& entry, bool& cancel) -> bool
            {
                // If on render thread, limit result with only 1 entry
                if(isOnRenderThread && entries.size() > 0)
                {
                    cancel = true;
                    return false;
                }

                // Only ready tiles are needed
                return (entry->getState() == ResourceState::Ready);
            });
            if(entries.isEmpty())
                continue;

            for(auto itEntry = entries.cbegin(); itEntry != entries.cend(); ++itEntry)
            {
                const auto& entry = *itEntry;

                // Since state change is allowed (it's not changed to "Uploading" during query), check state here
                if(!entry->setStateIf(ResourceState::Ready, ResourceState::Uploading))
                    continue;
  
                // Actually upload to GPU
                didUpload = entry->uploadToGPU();
                triedToUpload = true;
                if(!didUpload)
                {
                    LogPrintf(LogSeverityLevel::Error, "Failed to upload tiled resources for %dx%d@%d to GPU", entry->tileId.x, entry->tileId.y, entry->zoom);
                    continue;
                }

                // Mark as uploaded
                assert(entry->getState() == ResourceState::Uploading);
                entry->setState(ResourceState::Uploaded);

                // If we're not on render thread, and we've just uploaded a tile, invalidate frame
                if(!isOnRenderThread)
                    invalidateFrame();

                // If we're on render thread, limit to 1 tile per frame
                if(isOnRenderThread)
                    break;
            }

            // If we're on render thread, limit to 1 tile per frame
            if(isOnRenderThread && triedToUpload)
                break;
        }

        // If we're on render thread, limit to 1 tile per frame
        if(isOnRenderThread && triedToUpload)
            break;
    }

    // Schedule one more render pass to upload more pending
    // or we've just uploaded a tile and need refresh
    if(didUpload)
        invalidateFrame();
}

void OsmAnd::MapRenderer::cleanupJunkResources()
{
    // Use aggressive cache cleaning: remove all tiled resources that are not needed
    for(auto itResourcesCollections = _resources.cbegin(); itResourcesCollections != _resources.cend(); ++itResourcesCollections)
    {
        const auto& resourcesCollections = *itResourcesCollections;

        for(auto itResourcesCollection = resourcesCollections.cbegin(); itResourcesCollection != resourcesCollections.cend(); ++itResourcesCollection)
        {
            const auto& resourcesCollection = *itResourcesCollection;

            // Skip empty entries
            if(!static_cast<bool>(resourcesCollection))
                continue;

            const auto dataSourceAvailable = isDataSourceAvailableFor(resourcesCollection);

            resourcesCollection->removeEntries([this, dataSourceAvailable](const std::shared_ptr<TiledResourceEntry>& entry, bool& cancel) -> bool
            {
                // Skip cleaning if this tiled resource is needed
                if(_uniqueTiles.contains(entry->tileId) && dataSourceAvailable)
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

                //NOTE: this may happen when resources are uploaded in separate thread. Actually this will happen
                assert(false);
                return false;
            });
        }
    }
}

void OsmAnd::MapRenderer::releaseResourcesFrom( const std::shared_ptr<TiledResources>& collection )
{
    // This is allowed to be called only from render thread
    assert(QThread::currentThreadId() == _renderThreadId);

    // Remove all tiles, releasing associated GPU resources
    collection->removeEntries([](const std::shared_ptr<TiledResourceEntry>& entry, bool& cancel) -> bool
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

void OsmAnd::MapRenderer::requestResourcesUpload()
{
    if(setupOptions.backgroundWorker.enabled)
        _backgroundWorkerWakeup.wakeAll();
    else
        invalidateFrame();
}

bool OsmAnd::MapRenderer::isDataSourceAvailableFor( const std::shared_ptr<TiledResources>& collection )
{
    const auto& binding = _providersAndResourcesBindings[static_cast<int>(collection->type)];

    return binding.resourcesToProviders.contains(collection);
}

void OsmAnd::MapRenderer::invalidateFrame()
{
    _frameInvalidated = true;

    // Request frame, if such callback is defined
    if(setupOptions.frameRequestCallback)
        setupOptions.frameRequestCallback();
}

void OsmAnd::MapRenderer::backgroundWorkerProcedure()
{
    QMutex wakeupMutex;
    _workerThreadId = QThread::currentThreadId();

    // Call prologue if such exists
    if(setupOptions.backgroundWorker.prologue)
        setupOptions.backgroundWorker.prologue();

    while(_isRenderingInitialized)
    {
        // Wait until we're unblocked by host
        {
            wakeupMutex.lock();
            _backgroundWorkerWakeup.wait(&wakeupMutex);
            wakeupMutex.unlock();
        }
        if(!_isRenderingInitialized)
            break;

        // In every layer we have, upload pending tiles to GPU
        uploadResources();
    }

    // Call epilogue
    if(setupOptions.backgroundWorker.epilogue)
        setupOptions.backgroundWorker.epilogue();

    _workerThreadId = nullptr;
}

bool OsmAnd::MapRenderer::initializeRendering()
{
    bool ok;

    // Before doing any initialization, we need to allocate and initialize render API
    auto apiObject = allocateRenderAPI();
    if(!apiObject)
        return false;
    _renderAPI.reset(apiObject);

    ok = preInitializeRendering();
    if(!ok)
        return false;

    ok = doInitializeRendering();
    if(!ok)
        return false;

    ok = postInitializeRendering();
    if(!ok)
        return false;

    // Once rendering is initialized, invalidate frame
    invalidateFrame();

    return true;
}

bool OsmAnd::MapRenderer::preInitializeRendering()
{
    if(_isRenderingInitialized)
        return false;

    // Capture render thread ID, since rendering must be performed from
    // same thread where it was initialized
    _renderThreadId = QThread::currentThreadId();

    // Initialize various values
    _workerThreadId = nullptr;
    _currentConfigurationInvalidatedMask = std::numeric_limits<uint32_t>::max();
    _requestedStateUpdatedMask = std::numeric_limits<uint32_t>::max();
    _invalidatedResourcesTypesMask = 0;

    // Raster resources collections are special, so preallocate them
    {
        auto& resources = _resources[static_cast<int>(ResourceType::RasterMap)];

        assert(resources.isEmpty());
        resources.reserve(RasterMapLayersCount);
        while(resources.size() < RasterMapLayersCount)
            resources.append(std::shared_ptr<TiledResources>());
    }
    
    return true;
}

bool OsmAnd::MapRenderer::doInitializeRendering()
{
    // Create background worker if enabled
    if(setupOptions.backgroundWorker.enabled)
        _backgroundWorker.reset(new Concurrent::Thread(std::bind(&MapRenderer::backgroundWorkerProcedure, this)));

    // Upload stubs
    {
        {
            const auto& data = EmbeddedResources::decompressResource(QLatin1String("map/stubs/processing_tile.png"));
            auto bitmap = new SkBitmap();
            if(!SkImageDecoder::DecodeMemory(data.data(), data.size(), bitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            {
                delete bitmap;
            }
            else
            {
                auto bitmapTile = new MapBitmapTile(bitmap, MapBitmapTile::AlphaChannelData::Undefined);
                _renderAPI->uploadTileToGPU(std::shared_ptr< const MapTile >(bitmapTile), 1, _processingTileStub);
            }
        }
        {
            const auto& data = EmbeddedResources::decompressResource(QLatin1String("map/stubs/unavailable_tile.png"));
            auto bitmap = new SkBitmap();
            if(!SkImageDecoder::DecodeMemory(data.data(), data.size(), bitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            {
                delete bitmap;
            }
            else
            {
                auto bitmapTile = new MapBitmapTile(bitmap, MapBitmapTile::AlphaChannelData::Undefined);
                _renderAPI->uploadTileToGPU(std::shared_ptr< const MapTile >(bitmapTile), 1, _unavailableTileStub);
            }
        }
    }

    return true;
}

bool OsmAnd::MapRenderer::postInitializeRendering()
{
    _isRenderingInitialized = true;

    if(_backgroundWorker)
        _backgroundWorker->start();

    return true;
}

bool OsmAnd::MapRenderer::prepareFrame()
{
    assert(_renderThreadId == QThread::currentThreadId());

    bool ok;

    ok = prePrepareFrame();
    if(!ok)
        return false;

    ok = doPrepareFrame();
    if(!ok)
        return false;

    ok = postPrepareFrame();
    if(!ok)
        return false;

    return true;
}

bool OsmAnd::MapRenderer::prePrepareFrame()
{
    if(!_isRenderingInitialized)
        return false;

    bool ok;

    // If we have current configuration invalidated, we need to update it
    // and invalidate frame
    if(_currentConfigurationInvalidatedMask)
    {
        {
            QReadLocker scopedLocker(&_configurationLock);

            _currentConfiguration = _requestedConfiguration;
        }

        bool ok = updateCurrentConfiguration();
        if(ok)
            _currentConfigurationInvalidatedMask = 0;

        invalidateFrame();

        // If configuration is still invalidated, abort processing
        if(_currentConfigurationInvalidatedMask)
            return false;
    }

    // Deal with state
    ok = revalidateState();
    if(!ok)
        return false;

    // Get set of tiles that are unique: visible tiles may contain same tiles, but wrapped
    const auto internalState = getInternalStateRef();
    _uniqueTiles.clear();
    for(auto itTileId = internalState->visibleTiles.cbegin(); itTileId != internalState->visibleTiles.cend(); ++itTileId)
    {
        const auto& tileId = *itTileId;
        _uniqueTiles.insert(Utilities::normalizeTileId(tileId, _currentState.zoomBase));
    }

    // Validate resources
    validateResources();

    return true;
}

bool OsmAnd::MapRenderer::doPrepareFrame()
{
    return true;
}

bool OsmAnd::MapRenderer::postPrepareFrame()
{
    // Before requesting missing tiled resources, clean up cache to free some space
    cleanupJunkResources();

    // In the end of rendering processing, request tiled resources that are neither
    // present in requested list, nor in pending, nor in uploaded
    requestNeededResources();

    return true;
}

bool OsmAnd::MapRenderer::renderFrame()
{
    assert(_renderThreadId == QThread::currentThreadId());

    bool ok;

    ok = preRenderFrame();
    if(!ok)
        return false;

    ok = doRenderFrame();
    if(!ok)
        return false;

    ok = postRenderFrame();
    if(!ok)
        return false;

    return true;
}

bool OsmAnd::MapRenderer::preRenderFrame()
{
    if(!_isRenderingInitialized)
        return false;

    return true;
}

bool OsmAnd::MapRenderer::postRenderFrame()
{
    _frameInvalidated = false;

    return true;
}

bool OsmAnd::MapRenderer::processRendering()
{
    assert(_renderThreadId == QThread::currentThreadId());

    bool ok;

    ok = preProcessRendering();
    if(!ok)
        return false;

    ok = doProcessRendering();
    if(!ok)
        return false;

    ok = postProcessRendering();
    if(!ok)
        return false;

    return true;
}

bool OsmAnd::MapRenderer::preProcessRendering()
{
    return true;
}

bool OsmAnd::MapRenderer::doProcessRendering()
{
    // If background worked is was not enabled, upload tiles to GPU in render thread
    // To reduce FPS drop, upload not more than 1 tile per frame, and do that before end of the frame
    // to avoid forcing driver to upload data on current frame presentation.
    if(!setupOptions.backgroundWorker.enabled)
        uploadResources();

    return true;
}

bool OsmAnd::MapRenderer::postProcessRendering()
{
    return true;
}

bool OsmAnd::MapRenderer::releaseRendering()
{
    assert(_renderThreadId == QThread::currentThreadId());

    bool ok;

    ok = preReleaseRendering();
    if(!ok)
        return false;

    ok = doReleaseRendering();
    if(!ok)
        return false;

    ok = postReleaseRendering();
    if(!ok)
        return false;

    // After all release procedures, release render API
    ok = _renderAPI->release();
    _renderAPI.reset();
    if(!ok)
        return false;

    return true;
}

bool OsmAnd::MapRenderer::preReleaseRendering()
{
    if(!_isRenderingInitialized)
        return false;

    return true;
}

bool OsmAnd::MapRenderer::doReleaseRendering()
{
    return true;
}

bool OsmAnd::MapRenderer::postReleaseRendering()
{
    _isRenderingInitialized = false;

    // Stop worker
    if(_backgroundWorker)
    {
        _backgroundWorkerWakeup.wakeAll();
        _backgroundWorker->wait();
        _backgroundWorker.reset();
    }

    // Release all resources
    for(auto itResourcesCollections = _resources.begin(); itResourcesCollections != _resources.end(); ++itResourcesCollections)
    {
        auto& resourcesCollections = *itResourcesCollections;

        for(auto itResourcesCollection = resourcesCollections.cbegin(); itResourcesCollection != resourcesCollections.cend(); ++itResourcesCollection)
            releaseResourcesFrom(*itResourcesCollection);
        resourcesCollections.clear();
    }

    // Release all bindings
    for(auto resourceType = 0; resourceType < ResourceTypesCount; resourceType++)
    {
        _providersAndResourcesBindings[resourceType].providersToResources.clear();
        _providersAndResourcesBindings[resourceType].resourcesToProviders.clear();
    }
    
    // Release all embedded resources
    {
        assert(_unavailableTileStub.use_count() == 1);
        _unavailableTileStub.reset();
    }
    {
        assert(_processingTileStub.use_count() == 1);
        _processingTileStub.reset();
    }

    return true;
}

bool OsmAnd::MapRenderer::obtainProviderFor( TiledResources* const resourcesRef, std::shared_ptr<IMapProvider>& provider )
{
    assert(resourcesRef != nullptr);

    const auto& bindings = _providersAndResourcesBindings[static_cast<int>(resourcesRef->type)];
    for(auto itBinding = bindings.resourcesToProviders.cbegin(); itBinding != bindings.resourcesToProviders.cend(); ++itBinding)
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

void OsmAnd::MapRenderer::invalidateResourcesOfType( const ResourceType type )
{
    _invalidatedResourcesTypesMask |= 1u << static_cast<int>(type);
}

void OsmAnd::MapRenderer::validateResources()
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

void OsmAnd::MapRenderer::validateResourcesOfType( const ResourceType type )
{
    const auto& resourcesCollections = _resources[static_cast<int>(type)];
    const auto& bindings = _providersAndResourcesBindings[static_cast<int>(type)];

    for(auto itResourcesCollection = resourcesCollections.cbegin(); itResourcesCollection != resourcesCollections.cend(); ++itResourcesCollection)
    {
        if(!bindings.resourcesToProviders.contains(*itResourcesCollection))
            continue;

        releaseResourcesFrom(*itResourcesCollection);
    }
}

std::shared_ptr<const OsmAnd::MapTile> OsmAnd::MapRenderer::prepareTileForUploadingToGPU( const std::shared_ptr<const MapTile>& tile )
{
    if(tile->dataType == MapTileDataType::Bitmap)
    {
        auto bitmapTile = std::static_pointer_cast<const MapBitmapTile>(tile);

        // Check if we're going to convert
        bool doConvert = false;
        const bool force16bit = (currentConfiguration.limitTextureColorDepthBy16bits && bitmapTile->bitmap->getConfig() == SkBitmap::Config::kARGB_8888_Config);
        const bool canUsePaletteTextures = currentConfiguration.paletteTexturesAllowed && renderAPI->isSupported_8bitPaletteRGBA8;
        const bool paletteTexture = (bitmapTile->bitmap->getConfig() == SkBitmap::Config::kIndex8_Config);
        const bool unsupportedFormat =
            (canUsePaletteTextures ? !paletteTexture : paletteTexture) ||
            (bitmapTile->bitmap->getConfig() != SkBitmap::Config::kARGB_8888_Config) ||
            (bitmapTile->bitmap->getConfig() != SkBitmap::Config::kARGB_4444_Config) ||
            (bitmapTile->bitmap->getConfig() != SkBitmap::Config::kRGB_565_Config);
        doConvert = doConvert || force16bit;
        doConvert = doConvert || unsupportedFormat;

        // Pass palette texture as-is
        if(paletteTexture && canUsePaletteTextures)
            return tile;

        // Check if we need alpha
        auto convertedAlphaChannelData = bitmapTile->alphaChannelData;
        if(doConvert && (convertedAlphaChannelData == MapBitmapTile::AlphaChannelData::Undefined))
        {
            convertedAlphaChannelData = SkBitmap::ComputeIsOpaque(*bitmapTile->bitmap.get())
                ? MapBitmapTile::AlphaChannelData::NotPresent
                : MapBitmapTile::AlphaChannelData::Present;
        }

        // If we have limit of 16bits per pixel in bitmaps, convert to ARGB(4444) or RGB(565)
        if(force16bit)
        {
            auto convertedBitmap = new SkBitmap();

            bitmapTile->bitmap->deepCopyTo(convertedBitmap,
                convertedAlphaChannelData == MapBitmapTile::AlphaChannelData::Present
                ? SkBitmap::Config::kARGB_4444_Config
                : SkBitmap::Config::kRGB_565_Config);

            auto convertedTile = new MapBitmapTile(convertedBitmap, convertedAlphaChannelData);
            return std::shared_ptr<const MapTile>(convertedTile);
        }

        // If we have any other unsupported format, convert to proper 16bit or 32bit
        if(unsupportedFormat)
        {
            auto convertedBitmap = new SkBitmap();

            bitmapTile->bitmap->deepCopyTo(convertedBitmap,
                currentConfiguration.limitTextureColorDepthBy16bits
                ? (convertedAlphaChannelData == MapBitmapTile::AlphaChannelData::Present ? SkBitmap::Config::kARGB_4444_Config : SkBitmap::Config::kRGB_565_Config)
                : SkBitmap::kARGB_8888_Config);

            auto convertedTile = new MapBitmapTile(convertedBitmap, convertedAlphaChannelData);
            return std::shared_ptr<const MapTile>(convertedTile);
        }
    }

    return tile;
}

unsigned int OsmAnd::MapRenderer::getVisibleTilesCount() const
{
    QReadLocker scopedLocker(&_internalStateLock);

    return getInternalStateRef()->visibleTiles.size();
}

void OsmAnd::MapRenderer::setRasterLayerProvider( const RasterMapLayerId layerId, const std::shared_ptr<IMapBitmapTileProvider>& tileProvider, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.rasterLayerProviders[static_cast<int>(layerId)] != tileProvider);
    if(!update)
        return;

    _requestedState.rasterLayerProviders[static_cast<int>(layerId)] = tileProvider;

    notifyRequestedStateWasUpdated(StateChange::RasterLayers_Providers);
}

void OsmAnd::MapRenderer::resetRasterLayerProvider( const RasterMapLayerId layerId, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || static_cast<bool>(_requestedState.rasterLayerProviders[static_cast<int>(layerId)]);
    if(!update)
        return;

    _requestedState.rasterLayerProviders[static_cast<int>(layerId)].reset();

    notifyRequestedStateWasUpdated(StateChange::RasterLayers_Providers);
}

void OsmAnd::MapRenderer::setRasterLayerOpacity( const RasterMapLayerId layerId, const float opacity, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(0.0f, qMin(opacity, 1.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.rasterLayerOpacity[static_cast<int>(layerId)], clampedValue);
    if(!update)
        return;

    _requestedState.rasterLayerOpacity[static_cast<int>(layerId)] = clampedValue;

    notifyRequestedStateWasUpdated(StateChange::RasterLayers_Opacity);
}

void OsmAnd::MapRenderer::setElevationDataProvider( const std::shared_ptr<IMapElevationDataProvider>& tileProvider, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.elevationDataProvider != tileProvider);
    if(!update)
        return;

    _requestedState.elevationDataProvider = tileProvider;

    notifyRequestedStateWasUpdated(StateChange::ElevationData_Provider);
}

void OsmAnd::MapRenderer::resetElevationDataProvider( bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || static_cast<bool>(_requestedState.elevationDataProvider);
    if(!update)
        return;

    _requestedState.elevationDataProvider.reset();

    notifyRequestedStateWasUpdated(StateChange::ElevationData_Provider);
}

void OsmAnd::MapRenderer::setElevationDataScaleFactor( const float factor, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.elevationDataScaleFactor, factor);
    if(!update)
        return;

    _requestedState.elevationDataScaleFactor = factor;

    notifyRequestedStateWasUpdated(StateChange::ElevationData_ScaleFactor);
}

void OsmAnd::MapRenderer::addSymbolProvider( const std::shared_ptr<IMapSymbolProvider>& provider, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || !_requestedState.symbolProviders.contains(provider);
    if(!update)
        return;

    _requestedState.symbolProviders.insert(provider);

    notifyRequestedStateWasUpdated(StateChange::Symbols_Providers);
}

void OsmAnd::MapRenderer::removeSymbolProvider( const std::shared_ptr<IMapSymbolProvider>& provider, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.symbolProviders.contains(provider);
    if(!update)
        return;

    _requestedState.symbolProviders.remove(provider);

    notifyRequestedStateWasUpdated(StateChange::Symbols_Providers);
}

void OsmAnd::MapRenderer::removeAllSymbolProviders( bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.symbolProviders.isEmpty();
    if(!update)
        return;

    _requestedState.symbolProviders.clear();

    notifyRequestedStateWasUpdated(StateChange::Symbols_Providers);
}

void OsmAnd::MapRenderer::setWindowSize( const PointI& windowSize, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.windowSize != windowSize);
    if(!update)
        return;

    _requestedState.windowSize = windowSize;

    notifyRequestedStateWasUpdated(StateChange::WindowSize);
}

void OsmAnd::MapRenderer::setViewport( const AreaI& viewport, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.viewport != viewport);
    if(!update)
        return;

    _requestedState.viewport = viewport;

    notifyRequestedStateWasUpdated(StateChange::Viewport);
}

void OsmAnd::MapRenderer::setFieldOfView( const float fieldOfView, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(fieldOfView, 90.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fieldOfView, clampedValue);
    if(!update)
        return;

    _requestedState.fieldOfView = clampedValue;

    notifyRequestedStateWasUpdated(StateChange::FieldOfView);
}

void OsmAnd::MapRenderer::setDistanceToFog( const float fogDistance, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), fogDistance);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogDistance, clampedValue);
    if(!update)
        return;

    _requestedState.fogDistance = clampedValue;

    notifyRequestedStateWasUpdated(StateChange::FogParameters);
}

void OsmAnd::MapRenderer::setFogOriginFactor( const float factor, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(factor, 1.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogOriginFactor, clampedValue);
    if(!update)
        return;

    _requestedState.fogOriginFactor = clampedValue;

    notifyRequestedStateWasUpdated(StateChange::FogParameters);
}

void OsmAnd::MapRenderer::setFogHeightOriginFactor( const float factor, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(factor, 1.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogHeightOriginFactor, clampedValue);
    if(!update)
        return;

    _requestedState.fogHeightOriginFactor = clampedValue;

    notifyRequestedStateWasUpdated(StateChange::FogParameters);
}

void OsmAnd::MapRenderer::setFogDensity( const float fogDensity, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), fogDensity);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogDensity, clampedValue);
    if(!update)
        return;

    _requestedState.fogDensity = clampedValue;

    notifyRequestedStateWasUpdated(StateChange::FogParameters);
}

void OsmAnd::MapRenderer::setFogColor( const FColorRGB& color, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.fogColor != color;
    if(!update)
        return;

    _requestedState.fogColor = color;

    notifyRequestedStateWasUpdated(StateChange::FogParameters);
}

void OsmAnd::MapRenderer::setSkyColor( const FColorRGB& color, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.skyColor != color;
    if(!update)
        return;

    _requestedState.skyColor = color;

    notifyRequestedStateWasUpdated(StateChange::SkyColor);
}

void OsmAnd::MapRenderer::setAzimuth( const float azimuth, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    float normalizedAzimuth = Utilities::normalizedAngleDegrees(azimuth);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.azimuth, normalizedAzimuth);
    if(!update)
        return;

    _requestedState.azimuth = normalizedAzimuth;

    notifyRequestedStateWasUpdated(StateChange::Azimuth);
}

void OsmAnd::MapRenderer::setElevationAngle( const float elevationAngle, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(elevationAngle, 90.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.elevationAngle, clampedValue);
    if(!update)
        return;

    _requestedState.elevationAngle = clampedValue;

    notifyRequestedStateWasUpdated(StateChange::ElevationAngle);
}

void OsmAnd::MapRenderer::setTarget( const PointI& target31_, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto target31 = Utilities::normalizeCoordinates(target31_, ZoomLevel31);
    bool update = forcedUpdate || (_requestedState.target31 != target31);
    if(!update)
        return;

    _requestedState.target31 = target31;

    notifyRequestedStateWasUpdated(StateChange::Target);
}

void OsmAnd::MapRenderer::setZoom( const float zoom, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(zoom, 31.49999f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.requestedZoom, clampedValue);
    if(!update)
        return;

    _requestedState.requestedZoom = clampedValue;
    _requestedState.zoomBase = static_cast<ZoomLevel>(qRound(clampedValue));
    assert(_requestedState.zoomBase >= 0 && _requestedState.zoomBase <= 31);
    _requestedState.zoomFraction = _requestedState.requestedZoom - _requestedState.zoomBase;

    notifyRequestedStateWasUpdated(StateChange::Zoom);
}

OsmAnd::MapRenderer::TiledResources::TiledResources( const ResourceType& type_ )
    : type(type_)
{
}

OsmAnd::MapRenderer::TiledResources::~TiledResources()
{
    verifyNoUploadedTilesPresent();
}

void OsmAnd::MapRenderer::TiledResources::removeAllEntries()
{
    verifyNoUploadedTilesPresent();

    TilesCollection::removeAllEntries();
}

void OsmAnd::MapRenderer::TiledResources::verifyNoUploadedTilesPresent()
{
    // Ensure that no tiles have "Uploaded" state
    bool stillUploadedTilesPresent = false;
    obtainEntries(nullptr, [&stillUploadedTilesPresent](const std::shared_ptr<TiledResourceEntry>& tileEntry, bool& cancel) -> bool
    {
        if(tileEntry->getState() == ResourceState::Uploading || tileEntry->getState() == ResourceState::Uploaded || tileEntry->getState() == ResourceState::Unloading)
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

OsmAnd::MapRenderer::TiledResourceEntry::TiledResourceEntry( MapRenderer* owner_, const ResourceType type_, const TilesCollection<TiledResourceEntry>& collection, const TileId tileId, const ZoomLevel zoom )
    : TilesCollectionEntryWithState(collection, tileId, zoom)
    , _owner(owner_)
    , _requestTask(nullptr)
    , type(type_)
{
}

OsmAnd::MapRenderer::TiledResourceEntry::~TiledResourceEntry()
{
    const volatile auto state = getState();
    if(state == ResourceState::Uploading || state == ResourceState::Uploaded || state == ResourceState::Unloading)
        LogPrintf(LogSeverityLevel::Error, "Tiled resource for %dx%d@%d still resides in GPU memory. This may cause GPU memory leak", tileId.x, tileId.y, zoom);
}

OsmAnd::MapRenderer::InternalState::InternalState()
{
}

OsmAnd::MapRenderer::InternalState::~InternalState()
{
}

OsmAnd::MapRenderer::MapTileResourceEntry::MapTileResourceEntry( MapRenderer* owner, const ResourceType type, const TilesCollection<TiledResourceEntry>& collection, const TileId tileId, const ZoomLevel zoom )
    : TiledResourceEntry(owner, type, collection, tileId, zoom)
    , sourceData(_sourceData)
    , resourceInGPU(_resourceInGPU)
{
}

OsmAnd::MapRenderer::MapTileResourceEntry::~MapTileResourceEntry()
{

}

bool OsmAnd::MapRenderer::MapTileResourceEntry::obtainData( bool& dataAvailable )
{
    // Get source of tile
    std::shared_ptr<IMapProvider> provider_;
    bool ok = _owner->obtainProviderFor(static_cast<TiledResources*>(&link.lock()->collection), provider_);
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

bool OsmAnd::MapRenderer::MapTileResourceEntry::uploadToGPU()
{
    const auto& preparedSourceData = _owner->prepareTileForUploadingToGPU(_sourceData);
    //TODO: This is weird, and probably should not be here. RenderAPI knows how to upload what, but on contrary - does not know the limits
    const auto tilesPerAtlasTextureLimit = _owner->getTilesPerAtlasTextureLimit(type, _sourceData);
    bool ok = _owner->renderAPI->uploadTileToGPU(preparedSourceData, tilesPerAtlasTextureLimit, _resourceInGPU);
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

void OsmAnd::MapRenderer::MapTileResourceEntry::unloadFromGPU()
{
    assert(_resourceInGPU.use_count() == 1);
    _resourceInGPU.reset();
}

OsmAnd::MapRenderer::SymbolsResourceEntry::SymbolsResourceEntry( MapRenderer* owner, const TilesCollection<TiledResourceEntry>& collection, const TileId tileId, const ZoomLevel zoom )
    : TiledResourceEntry(owner, ResourceType::Symbols, collection, tileId, zoom)
    , sourceData(_sourceData)
    , resourcesInGPU(_resourcesInGPU)
{
}

OsmAnd::MapRenderer::SymbolsResourceEntry::~SymbolsResourceEntry()
{
}

bool OsmAnd::MapRenderer::SymbolsResourceEntry::obtainData( bool& dataAvailable )
{
    // Get source of tile
    std::shared_ptr<IMapProvider> provider_;
    bool ok = _owner->obtainProviderFor(static_cast<TiledResources*>(&link.lock()->collection), provider_);
    if(!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapSymbolProvider>(provider_);

    // Obtain symbols from each of symbol provider
    _sourceData.clear();
    
    //TODO: a cache of symbols needs to be maintained, since same symbol may be present in several tiles, but it should be drawn once?
    provider->obtainSymbols(tileId, zoom, _sourceData);
    
    return false;
}

bool OsmAnd::MapRenderer::SymbolsResourceEntry::uploadToGPU()
{
    return false;
}

void OsmAnd::MapRenderer::SymbolsResourceEntry::unloadFromGPU()
{

}

OsmAnd::MapRenderer::TileRequestTask::TileRequestTask(
    const std::shared_ptr<TiledResourceEntry>& requestedEntry_,
    const Concurrent::TaskHost::Bridge& bridge, ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod /*= nullptr*/, PostExecuteSignature postExecuteMethod /*= nullptr*/ )
    : HostedTask(bridge, executeMethod, preExecuteMethod, postExecuteMethod)
    , requestedEntry(requestedEntry_)
{
}

OsmAnd::MapRenderer::TileRequestTask::~TileRequestTask()
{
}
