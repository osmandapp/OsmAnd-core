#include "MapRenderer.h"

#include <cassert>
#include <algorithm>

#include <OsmAndCore/QtExtensions.h>
#include <QMutableHashIterator>

#include "ignore_warnings_on_external_includes.h"
#include <SkImage.h>
#include "restore_internal_warnings.h"

#include "IMapRenderer_Metrics.h"
#include "MapRendererInternalState.h"
#include "MapRendererResourcesManager.h"
#include "IMapLayerProvider.h"
#include "IMapElevationDataProvider.h"
#include "MapSymbol.h"
#include "GPUAPI.h"
#include "Utilities.h"
#include "Logging.h"
#include "Stopwatch.h"
#include "QKeyValueIterator.h"
#include <SqliteHeightmapTileProvider.h>
#include <VectorLinesCollection.h>

//#define OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE 1
#ifndef OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
#   define OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE 0
#endif // !defined(OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE)

//#define OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY 1
#ifndef OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY
#   define OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY 0
#endif // !defined(OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY)

OsmAnd::MapRenderer::MapRenderer(
    GPUAPI* const gpuAPI_,
    const std::unique_ptr<const MapRendererConfiguration>& baseConfiguration_,
    const std::unique_ptr<const MapRendererDebugSettings>& baseDebugSettings_)
    : _isRenderingInitialized(false)
    , _currentConfiguration(baseConfiguration_->createCopy())
    , _currentConfigurationAsConst(_currentConfiguration)
    , _requestedConfiguration(baseConfiguration_->createCopy())
    , _suspendSymbolsUpdateCounter(0)
    , _updateSymbols(false)
    , _gpuWorkerThreadId(nullptr)
    , _gpuWorkerThreadIsAlive(false)
    , _gpuWorkerIsSuspended(false)
    , _renderThreadId(nullptr)
    , _currentDebugSettings(baseDebugSettings_->createCopy())
    , _currentDebugSettingsAsConst(_currentDebugSettings)
    , _requestedDebugSettings(baseDebugSettings_->createCopy())
    , gpuContextIsLost(false)
    , gpuAPI(gpuAPI_)
    , setupOptions(_setupOptions)
    , currentConfiguration(_currentConfigurationAsConst)
    , currentState(_currentState)
    , publishedMapSymbolsByOrderLock(_publishedMapSymbolsByOrderLock)
    , publishedMapSymbolsByOrder(_publishedMapSymbolsByOrder)
    , currentDebugSettings(_currentDebugSettingsAsConst)
    , _jsonEnabled(false)
{
}

OsmAnd::MapRenderer::~MapRenderer()
{
    if (_isRenderingInitialized)
    {
        LogPrintf(LogSeverityLevel::Error, "MapRenderer is destroyed before releaseRendering() was called");
        assert(false);
    }
}

bool OsmAnd::MapRenderer::isRenderingInitialized() const
{
    return _isRenderingInitialized;
}

bool OsmAnd::MapRenderer::hasGpuWorkerThread() const
{
    return (_gpuWorkerThreadId != nullptr);
}

bool OsmAnd::MapRenderer::isInGpuWorkerThread() const
{
    return (_gpuWorkerThreadId == QThread::currentThreadId());
}

bool OsmAnd::MapRenderer::isInRenderThread() const
{
    return (_renderThreadId == QThread::currentThreadId());
}

OsmAnd::MapRendererSetupOptions OsmAnd::MapRenderer::getSetupOptions() const
{
    return _setupOptions;
}

bool OsmAnd::MapRenderer::setup(const MapRendererSetupOptions& setupOptions)
{
    // We can not change setup options renderer once rendering has been initialized
    if (_isRenderingInitialized)
        return false;

    _setupOptions = setupOptions;

    return true;
}

std::shared_ptr<OsmAnd::MapRendererConfiguration> OsmAnd::MapRenderer::getConfiguration() const
{
    QReadLocker scopedLocker(&_configurationLock);

    return _requestedConfiguration->createCopy();
}

void OsmAnd::MapRenderer::setConfiguration(
    const std::shared_ptr<const MapRendererConfiguration>& configuration,
    bool forcedUpdate /*= false*/)
{
    QWriteLocker scopedLocker(&_configurationLock);

    const auto mask = getConfigurationChangeMask(_requestedConfiguration, configuration);
    if (mask == 0 && !forcedUpdate)
        return;

    configuration->copyTo(*_requestedConfiguration);

    invalidateCurrentConfiguration(mask);
}

uint32_t OsmAnd::MapRenderer::getConfigurationChangeMask(
    const std::shared_ptr<const MapRendererConfiguration>& current,
    const std::shared_ptr<const MapRendererConfiguration>& updated) const
{
    const bool colorDepthForcingChanged = (current->limitTextureColorDepthBy16bits != updated->limitTextureColorDepthBy16bits);
    const bool texturesFilteringChanged = (current->texturesFilteringQuality != updated->texturesFilteringQuality);

    uint32_t mask = 0;
    if (colorDepthForcingChanged)
        mask |= enumToBit(ConfigurationChange::ColorDepthForcing);
    if (texturesFilteringChanged)
        mask |= enumToBit(ConfigurationChange::TexturesFilteringMode);

    return mask;
}

void OsmAnd::MapRenderer::invalidateCurrentConfiguration(const uint32_t changesMask)
{
    // Since our current configuration is invalid, frame is also invalidated
    _currentConfigurationInvalidatedMask.fetchAndOrOrdered(changesMask);

    if (isRenderingInitialized())
        invalidateFrame();
}

bool OsmAnd::MapRenderer::updateCurrentConfiguration(const unsigned int currentConfigurationInvalidatedMask_)
{
    uint32_t changeIndex = 0;
    unsigned int currentConfigurationInvalidatedMask = currentConfigurationInvalidatedMask_;
    while (currentConfigurationInvalidatedMask != 0)
    {
        if (currentConfigurationInvalidatedMask & 0x1)
            validateConfigurationChange(static_cast<ConfigurationChange>(changeIndex));

        changeIndex++;
        currentConfigurationInvalidatedMask >>= 1;
    }

    return true;
}

void OsmAnd::MapRenderer::notifyRequestedStateWasUpdated(const MapRendererStateChange change)
{
    const auto changeMask = (1u << static_cast<uint32_t>(change));
    const unsigned int newChangesMask = _requestedStateUpdatedMask.fetchAndOrOrdered(changeMask) | changeMask;

    // Notify all observers
    stateChangeObservable.postNotify(this, change, MapRendererStateChanges(newChangesMask));

    // Since our current state is invalid, frame is also invalidated
    invalidateFrame();
}

void OsmAnd::MapRenderer::validateConfigurationChange(const ConfigurationChange& change)
{
    bool invalidateMapLayers = false;
    invalidateMapLayers = invalidateMapLayers || (change == ConfigurationChange::ColorDepthForcing);
    if (invalidateMapLayers)
        getResources().invalidateResourcesOfType(MapRendererResourceType::MapLayer);

    bool invalidateSymbols = false;
    invalidateSymbols = invalidateSymbols || (change == ConfigurationChange::ColorDepthForcing);
    if (invalidateSymbols)
        getResources().invalidateResourcesOfType(MapRendererResourceType::Symbols);
}

bool OsmAnd::MapRenderer::updateInternalState(
    MapRendererInternalState& outInternalState,
    const MapRendererState& state,
    const MapRendererConfiguration& configuration,
    const bool skipTiles /*=false*/, const bool sortTiles /*=false*/) const
{
    return true;
}

void OsmAnd::MapRenderer::invalidateFrame()
{
    // Increment frame-invalidated counter by 1
    _frameInvalidatesCounter.fetchAndAddOrdered(1);

    // Request frame, if such callback is defined
    if (_setupOptions.frameUpdateRequestCallback)
        _setupOptions.frameUpdateRequestCallback(this);
}

void OsmAnd::MapRenderer::gpuWorkerThreadProcedure()
{
    assert(_setupOptions.gpuWorkerThreadEnabled);

    // Capture thread ID
    _gpuWorkerThreadId = QThread::currentThreadId();

    // Call prologue if such exists
    if (_setupOptions.gpuWorkerThreadPrologue)
        _setupOptions.gpuWorkerThreadPrologue(this);

    while (_gpuWorkerThreadIsAlive)
    {
        // Wait until we're unblocked by host
        {
            QMutexLocker scopedLocker(&_gpuWorkerThreadWakeupMutex);
            REPEAT_UNTIL(_gpuWorkerThreadWakeup.wait(&_gpuWorkerThreadWakeupMutex));
        }

        // If worker was requested to stop, let it be so
        if (!_gpuWorkerThreadIsAlive)
            break;

        if (!_gpuWorkerIsSuspended)
            processGpuWorker();
    }

    // Call epilogue
    if (_setupOptions.gpuWorkerThreadEpilogue)
        _setupOptions.gpuWorkerThreadEpilogue(this);

    _gpuWorkerThreadId = nullptr;
}

void OsmAnd::MapRenderer::processGpuWorker()
{
    if (isInGpuWorkerThread())
    {
        // In every layer we have, upload pending resources to GPU without limiting
        int unprocessedRequests = 0;
        do
        {
            const auto requestsToProcess = _resourcesGpuSyncRequestsCounter.fetchAndAddOrdered(0);
            unsigned int resourcesUploaded = 0u;
            unsigned int resourcesUnloaded = 0u;
            _resources->syncResourcesInGPU(0, nullptr, &resourcesUploaded, &resourcesUnloaded);
            if (resourcesUploaded > 0 || resourcesUnloaded > 0)
                invalidateFrame();
            unprocessedRequests = _resourcesGpuSyncRequestsCounter.fetchAndAddOrdered(-requestsToProcess) - requestsToProcess;
        } while (_gpuWorkerThreadIsAlive && unprocessedRequests > 0);
    }
    else if (isInRenderThread())
    {
        // To reduce FPS drop, upload not more than 1 resource per frame.
        const auto requestsToProcess = _resourcesGpuSyncRequestsCounter.fetchAndAddOrdered(0);
        bool moreUploadThanLimitAvailable = false;
        unsigned int resourcesUploaded = 0u;
        unsigned int resourcesUnloaded = 0u;
        _resources->syncResourcesInGPU(1u, &moreUploadThanLimitAvailable, &resourcesUploaded, &resourcesUnloaded);
        const auto unprocessedRequests =
            _resourcesGpuSyncRequestsCounter.fetchAndAddOrdered(-requestsToProcess) - requestsToProcess;

        // If any resource was uploaded or there is more resources to uploaded, invalidate frame
        // to use that resource
        if (resourcesUploaded > 0 || moreUploadThanLimitAvailable || resourcesUnloaded > 0 || unprocessedRequests > 0)
            invalidateFrame();
    }
    else
    {
        LogPrintf(LogSeverityLevel::Error, "MapRenderer::processGpuWorker() was called from invalid thread");
        assert(false);
    }

    // Process GPU dispatcher
    _gpuThreadDispatcher.runAll();
}

bool OsmAnd::MapRenderer::initializeRendering(bool renderTargetAvailable)
{
    bool ok;

    _resourcesGpuSyncRequestsCounter.storeRelaxed(0);

    ok = gpuAPI->initialize();
    if (!ok)
        return false;

    ok = preInitializeRendering();
    if (!ok)
        return false;

    ok = doInitializeRendering();
    if (!ok)
        return false;

    ok = postInitializeRendering();
    if (!ok)
        return false;

    // DEPTH BUFFER READING IS NOT NEEDED
    // Once rendering is initialized, attach to render target if available
    //if (renderTargetAvailable)
    //{
    //    ok = attachToRenderTarget();
    //    if (!ok)
    //        return false;
    //}

    // Once rendering is initialized, invalidate frame
    invalidateFrame();

    return true;
}

bool OsmAnd::MapRenderer::preInitializeRendering()
{
    if (_isRenderingInitialized)
        return false;

    gpuContextIsLost = false;

    // Capture render thread ID, since rendering must be performed from
    // same thread where it was initialized
    _renderThreadId = QThread::currentThreadId();

    // Initialize various values
    _gpuWorkerThreadId = nullptr;
    _currentConfigurationInvalidatedMask = std::numeric_limits<uint32_t>::max();
    _requestedStateUpdatedMask = std::numeric_limits<uint32_t>::max();

    // Create resources
    assert(!static_cast<bool>(_resources));
    _resources.reset(new MapRendererResourcesManager(this));
    if (!_resources->initializeDefaultResources())
        return false;

    return true;
}

bool OsmAnd::MapRenderer::doInitializeRendering()
{
    // Create GPU worker thread
    if (_setupOptions.gpuWorkerThreadEnabled)
    {
        const auto thread = new Concurrent::Thread(std::bind(&MapRenderer::gpuWorkerThreadProcedure, this));
        _gpuWorkerThread.reset(thread);
    }

    return true;
}

bool OsmAnd::MapRenderer::postInitializeRendering()
{
    _isRenderingInitialized = true;

    // GPU worker should not be suspended initially
    _gpuWorkerIsSuspended = false;

    // Start GPU worker (if it exists)
    if (_gpuWorkerThread)
    {
        _gpuWorkerThreadIsAlive = true;
        _gpuWorkerThread->start();
    }

    return true;
}

bool OsmAnd::MapRenderer::update(IMapRenderer_Metrics::Metric_update* const metric /*= nullptr*/)
{
    assert(_renderThreadId == QThread::currentThreadId());

    bool ok = true;

    Stopwatch totalStopwatch(metric != nullptr);

    ok = ok && preUpdate(metric);
    ok = ok && doUpdate(metric);
    ok = ok && postUpdate(metric);

    if (metric)
        metric->elapsedTime = totalStopwatch.elapsed();

    return ok;
}

bool OsmAnd::MapRenderer::preUpdate(IMapRenderer_Metrics::Metric_update* const metric)
{
    // Check for resources updates
    Stopwatch updatesStopwatch(metric != nullptr);
    const auto& mapState = getMapState();
    if (_resources->checkForUpdatesAndApply(mapState))
        invalidateFrame();
    if (metric)
        metric->elapsedTimeForUpdatesProcessing = updatesStopwatch.elapsed();

    return true;
}

bool OsmAnd::MapRenderer::doUpdate(IMapRenderer_Metrics::Metric_update* const metric)
{
    // If GPU worker thread is not enabled, upload resource to GPU from render thread.
    if (!_gpuWorkerThread && !_gpuWorkerIsSuspended)
        processGpuWorker();

    // Process render thread dispatcher
    Stopwatch renderThreadDispatcherStopwatch(metric != nullptr);
    _renderThreadDispatcher.runAll();
    if (metric)
        metric->elapsedTimeForRenderThreadDispatcher = renderThreadDispatcherStopwatch.elapsed();

    return true;
}

bool OsmAnd::MapRenderer::postUpdate(IMapRenderer_Metrics::Metric_update* const metric)
{
    return true;
}

bool OsmAnd::MapRenderer::prepareFrame(IMapRenderer_Metrics::Metric_prepareFrame* const metric /*= nullptr*/)
{
    assert(_renderThreadId == QThread::currentThreadId());

    bool ok = true;

    Stopwatch totalStopwatch(metric != nullptr);

    ok = ok && prePrepareFrame();
    ok = ok && doPrepareFrame();
    ok = ok && postPrepareFrame();

    if (metric)
        metric->elapsedTime = totalStopwatch.elapsed();

    if (ok)
        framePreparedObservable.postNotify(this);

    return ok;
}

bool OsmAnd::MapRenderer::prePrepareFrame()
{
    if (!_isRenderingInitialized)
        return false;

    bool ok = true;

    // Update debug settings if needed
    const auto currentDebugSettingsInvalidatedCounter = _currentDebugSettingsInvalidatedCounter.fetchAndAddOrdered(0);
    if (currentDebugSettingsInvalidatedCounter > 0)
    {
        updateCurrentDebugSettings();

        _currentDebugSettingsInvalidatedCounter.fetchAndAddOrdered(-currentDebugSettingsInvalidatedCounter);
    }

    // If we have current configuration invalidated, we need to update it
    // and invalidate frame
    const unsigned int currentConfigurationInvalidatedMask = _currentConfigurationInvalidatedMask.fetchAndStoreOrdered(0);
    if (currentConfigurationInvalidatedMask != 0)
    {
        // Capture configuration
        QReadLocker scopedLocker(&_configurationLock);

        _requestedConfiguration->copyTo(*_currentConfiguration);
    }

    // Update configuration
    if (currentConfigurationInvalidatedMask != 0)
    {
        ok = updateCurrentConfiguration(currentConfigurationInvalidatedMask);
        if (!ok)
        {
            _currentConfigurationInvalidatedMask.fetchAndOrOrdered(currentConfigurationInvalidatedMask);
            invalidateFrame();
            return false;
        }
    }

    // Set corrected map target in case heightmap data became available
    {
        QMutexLocker scopedLocker(&_requestedStateMutex);

        bool adjustTarget = 
        _requestedState.fixedPixel.x >= 0 && _requestedState.fixedPixel.y >= 0
            && (!_targetIsElevated || _requestedState.fixedHeight == 0.0f)
            && _requestedState.elevationDataProvider && _requestedState.fixedLocation31 == _requestedState.target31
            && isLocationHeightAvailable(_requestedState, _requestedState.target31);
        if (adjustTarget)
        {
            _targetIsElevated = true;
            setMapTarget(_requestedState);
        }
    }

    // Deal with state
    bool isNew = false;
    unsigned int requestedStateUpdatedMask;
    if ((requestedStateUpdatedMask = _requestedStateUpdatedMask.fetchAndStoreOrdered(0)) != 0)
    {
        QMutexLocker scopedLocker(&_requestedStateMutex);

        // Update internal state, that is derived from requested state and configuration
        isNew = updateInternalState(*getInternalStateRef(), _requestedState, *currentConfiguration, false, true);

        // Elevate and zoom out if camera is too close to the ground
        PointF zoomAndTilt;
        if (isNew && getExtraZoomAndTiltForRelief(_requestedState, zoomAndTilt))
        {
            const auto zoom = zoomAndTilt.x + 
                _requestedState.zoomLevel + Utilities::visualZoomToFraction(_requestedState.visualZoom);
            const auto elevationAngle = zoomAndTilt.y + _requestedState.elevationAngle;
            const auto zoomLevel = qBound(
                _requestedState.minZoomLimit,
                static_cast<ZoomLevel>(qRound(zoom)),
                _requestedState.maxZoomLimit);
            const auto zoomFraction = zoom - zoomLevel;
            if (zoomFraction >= -0.5f && zoomFraction <= 0.5f && elevationAngle <= 90.0f)
            {
                _requestedState.zoomLevel = zoomLevel;
                _requestedState.visualZoom = Utilities::zoomFractionToVisual(zoomFraction);
                _requestedState.elevationAngle = elevationAngle;
                setMapTargetOnly(_requestedState, _requestedState.fixedLocation31, 0.0f, false, true);
                _requestedState.surfaceZoomLevel = getSurfaceZoom(_requestedState, _requestedState.surfaceVisualZoom);
                const auto changeMask = (1u << static_cast<uint32_t>(MapRendererStateChange::Zoom));
                _requestedStateUpdatedMask.fetchAndOrOrdered(changeMask);
                isNew = false;
            }
        }

        // Capture new state
        _currentState = _requestedState;
    }

    if (requestedStateUpdatedMask != 0)
    {
        ok = handleStateChange(_currentState, requestedStateUpdatedMask);
        if (!ok)
        {
            _requestedStateUpdatedMask.fetchAndOrOrdered(requestedStateUpdatedMask);
            invalidateFrame();
            return false;
        }
    }

    // Update internal state, that is derived from current state and configuration
    if (requestedStateUpdatedMask != 0 || currentConfigurationInvalidatedMask != 0)
    {
        if (!isNew)
            isNew = updateInternalState(*getInternalStateRef(), _currentState, *currentConfiguration, false, true);

        // Anyways, invalidate the frame
        invalidateFrame();

        if (!isNew)
        {
            // In case updating internal state failed, restore changes as not-applied
            if (requestedStateUpdatedMask != 0)
                _requestedStateUpdatedMask.fetchAndOrOrdered(requestedStateUpdatedMask);
            if (currentConfigurationInvalidatedMask != 0)
                _currentConfigurationInvalidatedMask.fetchAndOrOrdered(currentConfigurationInvalidatedMask);

            return false;
        }

        _currentState.metersPerPixel = getPixelsToMetersScaleFactor(_currentState, getInternalState());
        _currentState.visibleBBox31 = getVisibleBBox31(getInternalState());
        _currentState.visibleBBoxShifted = getVisibleBBoxShifted(getInternalState());
    }

    return true;
}

bool OsmAnd::MapRenderer::doPrepareFrame()
{
    // Validate resources:
    // If any resources were validated, they will be updated by worker thread during postPrepareFrame
    _resources->validateResources();

    return true;
}

bool OsmAnd::MapRenderer::postPrepareFrame()
{
    // Tell resources to update snapshots of resources collections
    if (!_resources->updateCollectionsSnapshots())
        invalidateFrame();

    return true;
}

bool OsmAnd::MapRenderer::renderFrame(IMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
{
    assert(_renderThreadId == QThread::currentThreadId());

    bool ok = true;

    Stopwatch totalStopwatch(metric != nullptr);

    ok = ok && preRenderFrame(metric);
    ok = ok && doRenderFrame(metric);
    ok = ok && postRenderFrame(metric);

    if (metric)
        metric->elapsedTime = totalStopwatch.elapsed();

    return ok;
}

bool OsmAnd::MapRenderer::preRenderFrame(IMapRenderer_Metrics::Metric_renderFrame* const metric)
{
    if (!_isRenderingInitialized)
        return false;

    // Capture how many "frame-invalidates" are going to be processed
    _frameInvalidatesToBeProcessed = _frameInvalidatesCounter.fetchAndAddOrdered(0);

    return true;
}

bool OsmAnd::MapRenderer::postRenderFrame(IMapRenderer_Metrics::Metric_renderFrame* const metric)
{
    // Decrement "frame-invalidates" counter by amount of processed "frame-invalidates"
    _frameInvalidatesCounter.fetchAndAddOrdered(-_frameInvalidatesToBeProcessed);
    _frameInvalidatesToBeProcessed = 0;

    // Flush all pending GPU commands
    flushRenderCommands();
    
    if (freshSymbols())
        clearSymbolsUpdated();
    else if (!isFrameInvalidated())
    {
        // Invalidate the last frame and request updated symbols for the next one
        shouldUpdateSymbols();
        invalidateFrame();
    }

    return true;
}

bool OsmAnd::MapRenderer::releaseRendering(bool gpuContextLost)
{
    assert(_renderThreadId == QThread::currentThreadId());

    if (gpuContextLost)
        gpuContextIsLost = true;

    _setupOptions.gpuWorkerThreadEnabled = false;
    _setupOptions.gpuWorkerThreadPrologue = nullptr;
    _setupOptions.gpuWorkerThreadEpilogue = nullptr;
    _setupOptions.frameUpdateRequestCallback = nullptr;

    bool ok;

    ok = preReleaseRendering(gpuContextLost);
    if (!ok)
        return false;

    ok = doReleaseRendering(gpuContextLost);
    if (!ok)
        return false;

    ok = postReleaseRendering(gpuContextLost);
    if (!ok)
        return false;

    // After all release procedures, release render API
    ok = gpuAPI->release(gpuContextLost);
    if (!ok)
        return false;

    return true;
}

bool OsmAnd::MapRenderer::preReleaseRendering(const bool gpuContextLost)
{
    if (!_isRenderingInitialized)
        return false;

    return true;
}

bool OsmAnd::MapRenderer::doReleaseRendering(const bool gpuContextLost)
{
    return true;
}

bool OsmAnd::MapRenderer::postReleaseRendering(const bool gpuContextLost)
{
    // Wait for GPU worker to finish its job
    if (_gpuWorkerThread)
    {
        QWaitCondition gpuResourcesSyncStageExecutedOnceCondition;
        QMutex gpuResourcesSyncStageExecutedOnceMutex;       
        {
            QMutexLocker scopedLocker(&gpuResourcesSyncStageExecutedOnceMutex);

            // Dispatcher always runs after GPU resources sync stage
            getGpuThreadDispatcher().invokeAsync(
                [&gpuResourcesSyncStageExecutedOnceCondition, &gpuResourcesSyncStageExecutedOnceMutex]
                ()
                {
                    QMutexLocker scopedLocker(&gpuResourcesSyncStageExecutedOnceMutex);
                    gpuResourcesSyncStageExecutedOnceCondition.wakeAll();
                });

            // Wake up GPU worker thread
            {
                QMutexLocker scopedLocker(&_gpuWorkerThreadWakeupMutex);
                _gpuWorkerThreadWakeup.wakeAll();
            }

            // Wait up to 2s for GPU resources sync stage to complete
            gpuResourcesSyncStageExecutedOnceCondition.wait(&gpuResourcesSyncStageExecutedOnceMutex, 2000);
        }
    }

    // Release resources (to let all resources be released)
    _resources->releaseAllResources(gpuContextLost);

    // GPU worker should not be suspended afterwards
    _gpuWorkerIsSuspended = false;

    // Stop GPU worker if it exists
    if (_gpuWorkerThread)
    {
        // Deactivate worker thread
        _gpuWorkerThreadIsAlive = false;

        // Since _gpuWorkerAlive == false, wake up GPU worker thread to allow it to exit
        {
            QMutexLocker scopedLocker(&_gpuWorkerThreadWakeupMutex);
            _gpuWorkerThreadWakeup.wakeAll();
        }

        // Wait until thread will exit
        REPEAT_UNTIL(_gpuWorkerThread->wait());

        // And destroy thread object
        _gpuWorkerThread.reset();
    }

    _resources->releaseDefaultResources(gpuContextLost);
    _resources.reset();
    if (resourcesAreInUse.try_lock_for(std::chrono::seconds(2)))
        resourcesAreInUse.unlock();

    _isRenderingInitialized = false;

    return true;
}

bool OsmAnd::MapRenderer::handleStateChange(const MapRendererState& state, MapRendererStateChanges mask)
{
    bool ok = true;

    // Process updating of providers
    ok = ok && _resources->updateBindingsAndTime(state, mask);

    return ok;
}

bool OsmAnd::MapRenderer::attachToRenderTarget()
{
    if (isAttachedToRenderTarget())
        return false;

    bool ok;
    ok = gpuAPI->attachToRenderTarget();
    if (!ok)
        return false;

    invalidateFrame();

    return true;
}

bool OsmAnd::MapRenderer::isAttachedToRenderTarget()
{
    return gpuAPI->isAttachedToRenderTarget();
}

bool OsmAnd::MapRenderer::detachFromRenderTarget()
{
    return gpuAPI->detachFromRenderTarget(false);
}

bool OsmAnd::MapRenderer::isIdle() const
{
    bool isNotIdle = false;

    isNotIdle = isNotIdle || _resources->updatesPresent();
    isNotIdle = isNotIdle || (_resourcesGpuSyncRequestsCounter.loadAcquire() > 0);
    isNotIdle = isNotIdle || (_renderThreadDispatcher.queueSize() > 0);
    isNotIdle = isNotIdle || (_currentDebugSettingsInvalidatedCounter.loadAcquire() > 0);
    isNotIdle = isNotIdle || (_currentConfigurationInvalidatedMask.loadAcquire() != 0);
    isNotIdle = isNotIdle || (_requestedStateUpdatedMask.loadAcquire() != 0);
    isNotIdle = isNotIdle || (_resources->_invalidatedResourcesTypesMask.loadAcquire() != 0);
    isNotIdle = isNotIdle || (_resources->_resourcesRequestTasksCounter.loadAcquire() > 0);
    isNotIdle = isNotIdle || _resources->collectionsSnapshotsInvalidated();
    isNotIdle = isNotIdle || (_frameInvalidatesCounter.loadAcquire() > 0);
    isNotIdle = isNotIdle || !_resources->eachResourceIsUploadedOrUnavailable();

    return !isNotIdle;
}

QString OsmAnd::MapRenderer::getNotIdleReason() const
{
    QStringList notIdleReasons;
    bool shouldDumpResources = false;

    if (_resources->updatesPresent())
        notIdleReasons += QLatin1String("_resources->updatesPresent()");
    if ((_resourcesGpuSyncRequestsCounter.loadAcquire() > 0))
        notIdleReasons += QLatin1String("(_resourcesGpuSyncRequestsCounter.loadAcquire() > 0)");
    if (_renderThreadDispatcher.queueSize() > 0)
        notIdleReasons += QLatin1String("(_renderThreadDispatcher.queueSize() > 0)");
    if (_currentDebugSettingsInvalidatedCounter.loadAcquire() > 0)
        notIdleReasons += QLatin1String("(_currentDebugSettingsInvalidatedCounter.loadAcquire() > 0)");
    if (_currentConfigurationInvalidatedMask.loadAcquire() != 0)
        notIdleReasons += QLatin1String("(_currentConfigurationInvalidatedMask.loadAcquire() != 0)");
    if (_requestedStateUpdatedMask.loadAcquire() != 0)
        notIdleReasons += QLatin1String("(_requestedStateUpdatedMask.loadAcquire() != 0)");
    if (_resources->_invalidatedResourcesTypesMask.loadAcquire() != 0)
        notIdleReasons += QLatin1String("(_resources->_invalidatedResourcesTypesMask.loadAcquire() != 0)");
    if (_resources->_resourcesRequestTasksCounter.loadAcquire() > 0)
        notIdleReasons += QLatin1String("(_resources->_resourcesRequestTasksCounter.loadAcquire() > 0)");
    if (_resources->collectionsSnapshotsInvalidated())
        notIdleReasons += QLatin1String("_resources->collectionsSnapshotsInvalidated()");
    if (_frameInvalidatesCounter.loadAcquire() > 0)
        notIdleReasons += QLatin1String("(_frameInvalidatesCounter.loadAcquire() > 0)");
    if (!_resources->eachResourceIsUploadedOrUnavailable())
    {
        notIdleReasons += QLatin1String("!_resources->eachResourceIsUploadedOrUnavailable()");
        shouldDumpResources = true;
    }

    if (shouldDumpResources)
        dumpResourcesInfo();

    return notIdleReasons.join(QLatin1String("; "));
}

bool OsmAnd::MapRenderer::isGpuWorkerPaused() const
{
    return _gpuWorkerIsSuspended;
}

bool OsmAnd::MapRenderer::suspendGpuWorker()
{
    if (_gpuWorkerIsSuspended)
        return false;

    _gpuWorkerIsSuspended = true;

    return true;
}

bool OsmAnd::MapRenderer::resumeGpuWorker()
{
    if (!_gpuWorkerIsSuspended)
        return false;

    if (_gpuWorkerThread)
    {
        QMutexLocker scopedLocker(&_gpuWorkerThreadWakeupMutex);
        _gpuWorkerIsSuspended = false;
        _gpuWorkerThreadWakeup.wakeAll();
    }
    else
    {
        _gpuWorkerIsSuspended = false;
        invalidateFrame();
    }

    return true;
}

void OsmAnd::MapRenderer::reloadEverything()
{
    _resources->invalidateAllResources();
}

const OsmAnd::MapRendererResourcesManager& OsmAnd::MapRenderer::getResources() const
{
    return *_resources.get();
}

OsmAnd::MapRendererResourcesManager& OsmAnd::MapRenderer::getResources()
{
    return *_resources.get();
}

std::shared_ptr<OsmAnd::MapRendererResourcesManager>& OsmAnd::MapRenderer::getResourcesSharedPtr()
{
    return _resources;
}

void OsmAnd::MapRenderer::onValidateResourcesOfType(const MapRendererResourceType type)
{
    // Empty stub
}

void OsmAnd::MapRenderer::requestResourcesUploadOrUnload()
{
    _resourcesGpuSyncRequestsCounter.fetchAndAddOrdered(1);

    if (_gpuWorkerThread)
    {
        QMutexLocker scopedLocker(&_gpuWorkerThreadWakeupMutex);
        _gpuWorkerThreadWakeup.wakeAll();
    }
    else
        invalidateFrame();
}

bool OsmAnd::MapRenderer::adjustImageToConfiguration(
    const sk_sp<const SkImage>& input,
    sk_sp<SkImage>& output,
    const AlphaChannelPresence alphaChannelPresence /*= AlphaChannelPresence::Undefined*/) const
{
    // There's no way to convert empty input
    if (!input)
    {
        return false;
    }

    // Check if we're going to convert
    bool doConvert = false;
    const bool force16bit =
        (currentConfiguration->limitTextureColorDepthBy16bits && input->colorType() == SkColorType::kRGBA_8888_SkColorType);
    const bool unsupportedFormat =
        (input->colorType() != SkColorType::kRGBA_8888_SkColorType) &&
        (input->colorType() != SkColorType::kARGB_4444_SkColorType) &&
        (input->colorType() != SkColorType::kRGB_565_SkColorType);
    doConvert = doConvert || force16bit;
    doConvert = doConvert || unsupportedFormat;

    // Check if we need alpha
    auto convertedAlphaChannelPresence = alphaChannelPresence;
    if (doConvert && (convertedAlphaChannelPresence == AlphaChannelPresence::Unknown))
    {
        SkPixmap pixmap;
        if (input->peekPixels(&pixmap))
        {
            convertedAlphaChannelPresence = pixmap.computeIsOpaque()
                ? AlphaChannelPresence::NotPresent
                : AlphaChannelPresence::Present;
        }
    }

    // If we have limit of 16bits per pixel in bitmaps, convert to ARGB(4444) or RGB(565)
    if (force16bit)
    {
        const auto colorSpace = input->refColorSpace();
        const auto convertedImage = input->makeColorTypeAndColorSpace(
            convertedAlphaChannelPresence == AlphaChannelPresence::Present
                ? SkColorType::kARGB_4444_SkColorType
                : SkColorType::kRGB_565_SkColorType,
            colorSpace ? colorSpace : SkColorSpace::MakeSRGB()
        );
        if (!convertedImage)
        {
            return false;
        }

        output = convertedImage;
        return true;
    }

    // If we have any other unsupported format, convert to proper 16bit or 32bit
    if (unsupportedFormat)
    {
        const auto colorSpace = input->refColorSpace();
        const auto convertedImage = input->makeColorTypeAndColorSpace(
            currentConfiguration->limitTextureColorDepthBy16bits
                ? (convertedAlphaChannelPresence == AlphaChannelPresence::Present
                    ? SkColorType::kARGB_4444_SkColorType
                    : SkColorType::kRGB_565_SkColorType)
                : SkColorType::kRGBA_8888_SkColorType,
            colorSpace ? colorSpace : SkColorSpace::MakeSRGB()
        );
        if (!convertedImage)
        {
            return false;
        }

        output = convertedImage;
        return true;
    }

    return false;
}

sk_sp<const SkImage> OsmAnd::MapRenderer::adjustImageToConfiguration(
    const sk_sp<const SkImage>& input,
    const AlphaChannelPresence alphaChannelPresence /*= AlphaChannelPresence::Undefined*/) const
{
    sk_sp<SkImage> output;
    if (adjustImageToConfiguration(input, output, alphaChannelPresence))
        return output;
    return input;
}

void OsmAnd::MapRenderer::publishMapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& symbolGroup,
    const std::shared_ptr<const MapSymbol>& symbol,
    const std::shared_ptr<MapRendererBaseResource>& resource)
{
    QWriteLocker scopedLocker(&_publishedMapSymbolsByOrderLock);

    doPublishMapSymbol(symbolGroup, symbol, resource);
}

void OsmAnd::MapRenderer::batchPublishMapSymbols(const QList< PublishOrUnpublishMapSymbol >& mapSymbolsToPublish)
{
    QWriteLocker scopedLocker(&_publishedMapSymbolsByOrderLock);

    for (const auto& mapSymbolToPublish : constOf(mapSymbolsToPublish))
        doPublishMapSymbol(mapSymbolToPublish.symbolGroup, mapSymbolToPublish.symbol, mapSymbolToPublish.resource);
}

void OsmAnd::MapRenderer::doPublishMapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& symbolGroup,
    const std::shared_ptr<const MapSymbol>& symbol,
    const std::shared_ptr<MapRendererBaseResource>& resource)
{
#if OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY
    // Ensure that this symbol belongs to specified group
    assert(symbolGroup->symbols.contains(std::const_pointer_cast<MapSymbol>(symbol)));
    assert(symbol->groupPtr == symbolGroup.get());
    assert(validatePublishedMapSymbolsIntegrity());
#endif // OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY

    auto& publishedMapSymbols = _publishedMapSymbolsByOrder[symbol->order];
    auto& publishedMapSymbolsByGroup = publishedMapSymbols[symbolGroup];
    auto& publishedSymbolInfo = publishedMapSymbolsByGroup[symbol];
    auto& symbolReferencedResources = publishedSymbolInfo.referenceOrigins;
    if (symbolReferencedResources.isEmpty())
        _publishedMapSymbolsCount.fetchAndAddOrdered(1);
    assert(!symbolReferencedResources.contains(resource));
    symbolReferencedResources.insert(resource);

    _publishedMapSymbolsGroups[symbolGroup] += 1;

#if OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
    LogPrintf(LogSeverityLevel::Debug,
        "Published map symbol %p (group %p, new refs #%d) from %p (new total %d), now referenced from %d resources",
        symbol.get(),
        symbolGroup.get(),
        (unsigned int)_publishedMapSymbolsGroups[symbolGroup],
        resource.get(),
        _publishedMapSymbolsCount.load(),
        symbolReferencedResources.size());
#endif // OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE

#if OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY
    assert(validatePublishedMapSymbolsIntegrity());
#endif // OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY
}

void OsmAnd::MapRenderer::unpublishMapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& symbolGroup,
    const std::shared_ptr<const MapSymbol>& symbol,
    const std::shared_ptr<MapRendererBaseResource>& resource)
{
    QWriteLocker scopedLocker(&_publishedMapSymbolsByOrderLock);

    doUnpublishMapSymbol(symbolGroup, symbol, resource);
}

void OsmAnd::MapRenderer::batchUnpublishMapSymbols(const QList< PublishOrUnpublishMapSymbol >& mapSymbolsToUnpublish)
{
    QWriteLocker scopedLocker(&_publishedMapSymbolsByOrderLock);

    for (const auto& mapSymbolToUnpublish : constOf(mapSymbolsToUnpublish))
        doUnpublishMapSymbol(mapSymbolToUnpublish.symbolGroup, mapSymbolToUnpublish.symbol, mapSymbolToUnpublish.resource);
}

void OsmAnd::MapRenderer::doUnpublishMapSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& symbolGroup,
    const std::shared_ptr<const MapSymbol>& symbol,
    const std::shared_ptr<MapRendererBaseResource>& resource)
{
#if OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY
    // Ensure that this symbol belongs to specified group
    assert(symbolGroup->symbols.contains(std::const_pointer_cast<MapSymbol>(symbol)));
    assert(symbol->groupPtr == symbolGroup.get());
    assert(validatePublishedMapSymbolsIntegrity());
#endif // OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY

    const auto itPublishedMapSymbolsByGroup = _publishedMapSymbolsByOrder.find(symbol->order);
    if (itPublishedMapSymbolsByGroup == _publishedMapSymbolsByOrder.end())
    {
        assert(false);
        return;
    }
    auto& publishedMapSymbolsByGroup = *itPublishedMapSymbolsByGroup;

    const auto itPublishedMapSymbols = publishedMapSymbolsByGroup.find(symbolGroup);
    if (itPublishedMapSymbols == publishedMapSymbolsByGroup.end())
    {
        assert(false);
        return;
    }
    auto& publishedMapSymbols = itPublishedMapSymbols->second;

    const auto itPublishedSymbolInfo = publishedMapSymbols.find(symbol);
    if (itPublishedSymbolInfo == publishedMapSymbols.end())
    {
        assert(false);
        return;
    }
    auto& symbolReferencedResources = itPublishedSymbolInfo->referenceOrigins;

    if (!symbolReferencedResources.remove(resource))
    {
        assert(false);
        return;
    }
#if OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
    const auto symbolReferencedResourcesSize = symbolReferencedResources.size();
#endif // OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
    if (symbolReferencedResources.isEmpty())
    {
        _publishedMapSymbolsCount.fetchAndAddOrdered(-1);
        publishedMapSymbols.erase(itPublishedSymbolInfo);
    }
    if (publishedMapSymbols.isEmpty())
        publishedMapSymbolsByGroup.erase(itPublishedMapSymbols);
    if (publishedMapSymbolsByGroup.size() == 0)
        _publishedMapSymbolsByOrder.erase(itPublishedMapSymbolsByGroup);

    const auto itGroupRefsCounter = _publishedMapSymbolsGroups.find(symbolGroup);
    auto& groupRefsCounter = *itGroupRefsCounter;
    groupRefsCounter -= 1;
#if OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
    const auto newGroupRefsCounter = (unsigned int)groupRefsCounter;
#endif // OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
    if (groupRefsCounter == 0)
        _publishedMapSymbolsGroups.erase(itGroupRefsCounter);

#if OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
    LogPrintf(LogSeverityLevel::Debug,
        "Unpublished map symbol %p (group %p, new refs #%d) from %p (new total %d), now referenced from %d resources",
        symbol.get(),
        symbolGroup.get(),
        newGroupRefsCounter,
        resource.get(),
        _publishedMapSymbolsCount.load(),
        symbolReferencedResourcesSize);
#endif // OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE

#if OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY
    assert(validatePublishedMapSymbolsIntegrity());
#endif // OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY
}

bool OsmAnd::MapRenderer::validatePublishedMapSymbolsIntegrity()
{
    bool integrityValid = true;

    for (const auto& publishedMapSymbolsEntry : rangeOf(constOf(publishedMapSymbolsByOrder)))
    {
        const auto order = publishedMapSymbolsEntry.key();
        const auto& publishedMapSymbols = publishedMapSymbolsEntry.value();

        for (const auto& publishedMapSymbolsEntry : constOf(publishedMapSymbols))
        {
            const auto& mapSymbolsGroup = publishedMapSymbolsEntry.first;
            const auto& publishedMapSymbolsFromGroup = publishedMapSymbolsEntry.second;

            // Check that all published map symbols belong to specified group
            for (const auto& publishedMapSymbolEntry : rangeOf(constOf(publishedMapSymbolsFromGroup)))
            {
                const auto& publishedMapSymbol = publishedMapSymbolEntry.key();

                bool validated = true;

                if (publishedMapSymbol->groupPtr != mapSymbolsGroup.get())
                    validated = false;

                if (!mapSymbolsGroup->symbols.contains(std::const_pointer_cast<MapSymbol>(publishedMapSymbol)))
                    validated = false;

                if (!validated)
                    integrityValid = false;
            }
        }
    }

    return integrityValid;
}

unsigned int OsmAnd::MapRenderer::getSymbolsCount() const
{
    return _publishedMapSymbolsCount.loadAcquire();
}

bool OsmAnd::MapRenderer::isSymbolsUpdateSuspended(int* const pOutSuspendsCounter /*= nullptr*/) const
{
    const auto suspendsCounter = _suspendSymbolsUpdateCounter.loadAcquire();

    if (pOutSuspendsCounter != nullptr)
        *pOutSuspendsCounter = suspendsCounter;

    return (suspendsCounter > 0);
}

bool OsmAnd::MapRenderer::suspendSymbolsUpdate()
{
    const auto prevCounter = _suspendSymbolsUpdateCounter.fetchAndAddOrdered(+1);
    if (prevCounter == 0)
        invalidateFrame();

    return (prevCounter >= 0);
}

bool OsmAnd::MapRenderer::resumeSymbolsUpdate()
{
    auto prevCounter = _suspendSymbolsUpdateCounter.loadAcquire();
    while (prevCounter > 0)
    {
        if (_suspendSymbolsUpdateCounter.testAndSetOrdered(prevCounter, prevCounter - 1))
            break;

        prevCounter = _suspendSymbolsUpdateCounter.loadAcquire();
    }

    if (prevCounter == 1)
        invalidateFrame();

    return (prevCounter <= 1);
}

int OsmAnd::MapRenderer::getSymbolsUpdateInterval()
{
    return _symbolsUpdateInterval;
}

void OsmAnd::MapRenderer::setSymbolsUpdateInterval(int interval)
{
    _symbolsUpdateInterval = interval;
}

void OsmAnd::MapRenderer::shouldUpdateSymbols()
{
    _updateSymbols = true;
}

bool OsmAnd::MapRenderer::needUpdatedSymbols()
{
    return _updateSymbols;
}

void OsmAnd::MapRenderer::dontNeedUpdatedSymbols()
{
    _updateSymbols = false;
}

void OsmAnd::MapRenderer::setSymbolsUpdated()
{
    _freshSymbols = true;
}

bool OsmAnd::MapRenderer::freshSymbols()
{
    return _freshSymbols;
}

void OsmAnd::MapRenderer::clearSymbolsUpdated()
{
    _freshSymbols = false;
}

OsmAnd::MapRendererState OsmAnd::MapRenderer::getState() const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    return _requestedState;
}

OsmAnd::MapState OsmAnd::MapRenderer::getMapState() const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    return currentState.getMapState();
}

bool OsmAnd::MapRenderer::isFrameInvalidated() const
{
    return (_frameInvalidatesCounter.loadAcquire() > 0);
}

void OsmAnd::MapRenderer::forcedFrameInvalidate()
{
    invalidateFrame();
}

void OsmAnd::MapRenderer::forcedGpuProcessingCycle()
{
    requestResourcesUploadOrUnload();
}

OsmAnd::Concurrent::Dispatcher& OsmAnd::MapRenderer::getRenderThreadDispatcher()
{
    return _renderThreadDispatcher;
}

OsmAnd::Concurrent::Dispatcher& OsmAnd::MapRenderer::getGpuThreadDispatcher()
{
    return _gpuThreadDispatcher;
}

bool OsmAnd::MapRenderer::setMapLayerProvider(
    const int layerIndex,
    const std::shared_ptr<IMapLayerProvider>& provider,
    bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!provider)
        return false;

    const auto itMapLayerProvider = _requestedState.mapLayersProviders.find(layerIndex);
    const auto isSet = (itMapLayerProvider != _requestedState.mapLayersProviders.end());

    bool update = forcedUpdate || (!isSet || *itMapLayerProvider != provider);
    if (!update)
        return false;

    if (isSet)
        *itMapLayerProvider = provider;
    else
        _requestedState.mapLayersProviders[layerIndex] = provider;

    notifyRequestedStateWasUpdated(MapRendererStateChange::MapLayers_Providers);

    return true;
}

bool OsmAnd::MapRenderer::resetMapLayerProvider(const int layerIndex, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto itMapLayerProvider = _requestedState.mapLayersProviders.find(layerIndex);

    bool update = forcedUpdate || (itMapLayerProvider != _requestedState.mapLayersProviders.end());
    if (!update)
        return false;

    _requestedState.mapLayersProviders.erase(itMapLayerProvider);

    notifyRequestedStateWasUpdated(MapRendererStateChange::MapLayers_Providers);

    return true;
}

bool OsmAnd::MapRenderer::setMapLayerConfiguration(
    const int layerIndex,
    const MapLayerConfiguration& configuration,
    bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!configuration.isValid())
        return false;

    const auto itMapLayerConfiguration = _requestedState.mapLayersConfigurations.find(layerIndex);
    const auto isSet = (itMapLayerConfiguration != _requestedState.mapLayersConfigurations.end());

    bool update = forcedUpdate || (!isSet || *itMapLayerConfiguration != configuration);
    if (!update)
        return false;

    if (isSet)
        *itMapLayerConfiguration = configuration;
    else
        _requestedState.mapLayersConfigurations[layerIndex] = configuration;

    notifyRequestedStateWasUpdated(MapRendererStateChange::MapLayers_Configuration);

    return true;
}

bool OsmAnd::MapRenderer::setElevationDataProvider(
    const std::shared_ptr<IMapElevationDataProvider>& provider,
    bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!provider)
        return false;

    if (provider->getTileSize() != getElevationDataTileSize())
        return false;

    bool update = forcedUpdate || (_requestedState.elevationDataProvider != provider);
    if (!update)
        return false;

    _requestedState.elevationDataProvider = provider;

    setMapTarget(_requestedState, forcedUpdate);

    notifyRequestedStateWasUpdated(MapRendererStateChange::Elevation_DataProvider);

    return true;
}

bool OsmAnd::MapRenderer::resetElevationDataProvider(bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || static_cast<bool>(_requestedState.elevationDataProvider);
    if (!update)
        return false;

    // Check there are vector symbols that need elevation provider to be present
    bool withVolumetricSymbols = false;
    for (const auto& keyedSymbolsSubsection : constOf(_requestedState.keyedSymbolsProviders))
    {
        for (const auto& provider : constOf(keyedSymbolsSubsection))
        {
            if (const auto vectorLinesCollection = std::dynamic_pointer_cast<VectorLinesCollection>(provider))
                withVolumetricSymbols = withVolumetricSymbols || vectorLinesCollection->hasVolumetricSymbols;
        }
    }
    if (withVolumetricSymbols)
        _requestedState.elevationDataProvider.reset(new SqliteHeightmapTileProvider());
    else
        _requestedState.elevationDataProvider.reset();

    _requestedState.fixedHeight = 0.0f;

    _requestedState.aimHeight = 0.0f;

    setMapTarget(_requestedState, forcedUpdate);

    notifyRequestedStateWasUpdated(MapRendererStateChange::Elevation_DataProvider);

    return true;
}

bool OsmAnd::MapRenderer::setElevationConfiguration(
    const ElevationConfiguration& configuration,
    bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!configuration.isValid())
        return false;

    bool update = forcedUpdate || (_requestedState.elevationConfiguration != configuration);
    if (!update)
        return false;

    _requestedState.elevationConfiguration = configuration;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Elevation_Configuration);

    return true;
}

bool OsmAnd::MapRenderer::setElevationScaleFactor(const float scaleFactor, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!_requestedState.elevationConfiguration.isValid())
        return false;

    bool update = forcedUpdate || (_requestedState.elevationConfiguration.zScaleFactor != scaleFactor);
    if (!update)
        return false;

    _requestedState.elevationConfiguration.setZScaleFactor(scaleFactor);

    notifyRequestedStateWasUpdated(MapRendererStateChange::Elevation_Configuration);

    return true;
}

float OsmAnd::MapRenderer::getElevationScaleFactor()
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!_requestedState.elevationConfiguration.isValid())
        return NAN;

    return _requestedState.elevationConfiguration.zScaleFactor;
}

bool OsmAnd::MapRenderer::addSymbolsProvider(
    const int subsectionIndex,
    const std::shared_ptr<IMapTiledSymbolsProvider>& provider,
    bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!provider)
        return false;

    const auto itTiledSymbolSubsection = _requestedState.tiledSymbolsProviders.find(subsectionIndex);
    const auto isSet = (itTiledSymbolSubsection != _requestedState.tiledSymbolsProviders.end());

    bool update = forcedUpdate || (!isSet || !itTiledSymbolSubsection->contains(provider));
    if (!update)
        return false;

    if (isSet)
        itTiledSymbolSubsection->insert(provider);
    else
    {
        QSet< std::shared_ptr<IMapTiledSymbolsProvider> > providers;
        providers.insert(provider);
        _requestedState.tiledSymbolsProviders[subsectionIndex] = providers;
    }

    notifyRequestedStateWasUpdated(MapRendererStateChange::Symbols_Providers);

    return true;
}

bool OsmAnd::MapRenderer::addSymbolsProvider(
    const std::shared_ptr<IMapTiledSymbolsProvider>& provider,
    bool forcedUpdate /*= false*/)
{
    return addSymbolsProvider(0, provider, forcedUpdate);
}

bool OsmAnd::MapRenderer::addSymbolsProvider(
    const int subsectionIndex,
    const std::shared_ptr<IMapKeyedSymbolsProvider>& provider,
    bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!provider)
        return false;

    const auto itKeyedSymbolSubsection = _requestedState.keyedSymbolsProviders.find(subsectionIndex);
    const auto isSet = (itKeyedSymbolSubsection != _requestedState.keyedSymbolsProviders.end());

    bool update = forcedUpdate || (!isSet || !itKeyedSymbolSubsection->contains(provider));
    if (!update)
        return false;

    // Check elevation provider is needed
    if (!_requestedState.elevationDataProvider)
    {
        bool hasVolumetricSymbols = false;
        if (const auto vectorLinesCollection = std::dynamic_pointer_cast<VectorLinesCollection>(provider))
            hasVolumetricSymbols = vectorLinesCollection->hasVolumetricSymbols;
        if (hasVolumetricSymbols)
        {
            _requestedState.elevationDataProvider.reset(new SqliteHeightmapTileProvider());

            notifyRequestedStateWasUpdated(MapRendererStateChange::Elevation_DataProvider);
        }
    }

    if (isSet)
        itKeyedSymbolSubsection->insert(provider);
    else
    {
        QSet< std::shared_ptr<IMapKeyedSymbolsProvider> > providers;
        providers.insert(provider);
        _requestedState.keyedSymbolsProviders[subsectionIndex] = providers;
    }

    notifyRequestedStateWasUpdated(MapRendererStateChange::Symbols_Providers);

    return true;
}

bool OsmAnd::MapRenderer::addSymbolsProvider(
    const std::shared_ptr<IMapKeyedSymbolsProvider>& provider,
    bool forcedUpdate /*= false*/)
{
    return addSymbolsProvider(0, provider, forcedUpdate);
}

bool OsmAnd::MapRenderer::hasSymbolsProvider(
    const std::shared_ptr<IMapTiledSymbolsProvider>& provider)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!provider)
        return false;

    for (const auto& tiledSymbolsSubsection : constOf(_requestedState.tiledSymbolsProviders))
    {
        if (tiledSymbolsSubsection.contains(provider))
            return true;
    }

    return false;
}

bool OsmAnd::MapRenderer::hasSymbolsProvider(
    const std::shared_ptr<IMapKeyedSymbolsProvider>& provider)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!provider)
        return false;

    for (const auto& keyedSymbolsSubsection : constOf(_requestedState.keyedSymbolsProviders))
    {
        if (keyedSymbolsSubsection.contains(provider))
            return true;
    }

    return false;
}

int OsmAnd::MapRenderer::getSymbolsProviderSubsection(
    const std::shared_ptr<IMapTiledSymbolsProvider>& provider)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!provider)
        return 0;

    for (const auto& subsectionEntry : rangeOf(constOf(_requestedState.tiledSymbolsProviders)))
    {
        const auto subsectionIndex = subsectionEntry.key();
        const auto& subsection = subsectionEntry.value();
        if (subsection.contains(provider))
            return subsectionIndex;
    }

    return 0;
}

int OsmAnd::MapRenderer::getSymbolsProviderSubsection(
    const std::shared_ptr<IMapKeyedSymbolsProvider>& provider)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!provider)
        return 0;

    for (const auto& subsectionEntry : rangeOf(constOf(_requestedState.keyedSymbolsProviders)))
    {
        const auto subsectionIndex = subsectionEntry.key();
        const auto& subsection = subsectionEntry.value();
        if (subsection.contains(provider))
            return subsectionIndex;
    }

    return 0;
}

bool OsmAnd::MapRenderer::removeSymbolsProvider(
    const std::shared_ptr<IMapTiledSymbolsProvider>& provider,
    bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!provider)
        return false;

    bool update = forcedUpdate;
    for (auto& tiledSymbolsSubsection : _requestedState.tiledSymbolsProviders)
    {
        if (tiledSymbolsSubsection.contains(provider))
        {
            tiledSymbolsSubsection.remove(provider);
            update = true;
        }
    }
    if (!update)
        return false;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Symbols_Providers);

    return true;
}

bool OsmAnd::MapRenderer::removeSymbolsProvider(
    const std::shared_ptr<IMapKeyedSymbolsProvider>& provider,
    bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!provider)
        return false;

    // Check there is elevation provider without heightmap collections
    bool withoutHeightmaps = false;
    if (const auto heightmapProvider =
        std::dynamic_pointer_cast<SqliteHeightmapTileProvider>(_requestedState.elevationDataProvider))
    {
        if (!heightmapProvider->sourcesCollection && !heightmapProvider->filesCollection)
            withoutHeightmaps = true;
    }

    bool withVolumetricSymbols = false;
    bool update = forcedUpdate;
    for (auto& keyedSymbolsSubsection : _requestedState.keyedSymbolsProviders)
    {
        if (keyedSymbolsSubsection.contains(provider))
        {
            keyedSymbolsSubsection.remove(provider);
            update = true;
        }
        if (withoutHeightmaps)
        {
            // Check there are vector symbols that need elevation provider to be present
            for (const auto& provider : constOf(keyedSymbolsSubsection))
            {
                if (const auto vectorLinesCollection = std::dynamic_pointer_cast<VectorLinesCollection>(provider))
                    withVolumetricSymbols = withVolumetricSymbols || vectorLinesCollection->hasVolumetricSymbols;
            }
        }
    }
    if (!update)
        return false;

    // Remove elevation provider if it isn't needed anymore
    if (withoutHeightmaps && !withVolumetricSymbols)
    {
        _requestedState.elevationDataProvider.reset();

        notifyRequestedStateWasUpdated(MapRendererStateChange::Elevation_DataProvider);
    }

    notifyRequestedStateWasUpdated(MapRendererStateChange::Symbols_Providers);

    return true;
}

bool OsmAnd::MapRenderer::removeAllSymbolsProviders(bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto update =
        forcedUpdate ||
        !_requestedState.tiledSymbolsProviders.isEmpty() ||
        !_requestedState.keyedSymbolsProviders.isEmpty();
    if (!update)
        return false;

    // Remove elevation provider if it has no heightmap collections
    if (const auto heightmapProvider =
        std::dynamic_pointer_cast<SqliteHeightmapTileProvider>(_requestedState.elevationDataProvider))
    {
        if (!heightmapProvider->sourcesCollection && !heightmapProvider->filesCollection)
        {
            _requestedState.elevationDataProvider.reset();

            notifyRequestedStateWasUpdated(MapRendererStateChange::Elevation_DataProvider);
        }
    }

    _requestedState.tiledSymbolsProviders.clear();
    _requestedState.keyedSymbolsProviders.clear();

    notifyRequestedStateWasUpdated(MapRendererStateChange::Symbols_Providers);

    return true;
}

bool OsmAnd::MapRenderer::setSymbolSubsectionConfiguration(
    const int subsectionIndex,
    const SymbolSubsectionConfiguration& configuration,
    bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (!configuration.isValid())
        return false;

    const auto itSymbolSubsectionConfiguration = _requestedState.symbolSubsectionConfigurations.find(subsectionIndex);
    const auto isSet = (itSymbolSubsectionConfiguration != _requestedState.symbolSubsectionConfigurations.end());

    bool update = forcedUpdate || (!isSet || *itSymbolSubsectionConfiguration != configuration);
    if (!update)
        return false;

    if (isSet)
        *itSymbolSubsectionConfiguration = configuration;
    else
        _requestedState.symbolSubsectionConfigurations[subsectionIndex] = configuration;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Symbols_Configuration);

    return true;
}

bool OsmAnd::MapRenderer::setWindowSize(const PointI& windowSize, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (windowSize.x <= 0 || windowSize.y <= 0)
        return false;

    bool update = forcedUpdate || (_requestedState.windowSize != windowSize);
    if (!update)
        return false;

    _requestedState.windowSize = windowSize;

    setMapTarget(_requestedState, forcedUpdate);

    notifyRequestedStateWasUpdated(MapRendererStateChange::WindowSize);

    return true;
}

bool OsmAnd::MapRenderer::setViewport(const AreaI& viewport, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.viewport != viewport);
    if (!update)
        return false;

    _requestedState.viewport = viewport;

    setMapTarget(_requestedState, forcedUpdate);

    notifyRequestedStateWasUpdated(MapRendererStateChange::Viewport);

    return true;
}

bool OsmAnd::MapRenderer::setFieldOfView(const float fieldOfView, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (fieldOfView <= 0.0f || fieldOfView >= 90.0f)
        return false;

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fieldOfView, fieldOfView);
    if (!update)
        return false;

    _requestedState.fieldOfView = fieldOfView;

    setMapTarget(_requestedState, forcedUpdate);

    notifyRequestedStateWasUpdated(MapRendererStateChange::FieldOfView);

    return true;
}

bool OsmAnd::MapRenderer::setVisibleDistance(const float visibleDistance, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (visibleDistance < 0.0f)
        return false;

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.visibleDistance, visibleDistance);
    if (!update)
        return false;

    _requestedState.visibleDistance = visibleDistance;

    notifyRequestedStateWasUpdated(MapRendererStateChange::VisibleDistance);

    return true;
}

bool OsmAnd::MapRenderer::setDetailedDistance(const float detailedDistance, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (detailedDistance < 0.0f)
        return false;

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.detailedDistance, detailedDistance);
    if (!update)
        return false;

    _requestedState.detailedDistance = detailedDistance;

    notifyRequestedStateWasUpdated(MapRendererStateChange::DetailedDistance);

    return true;
}

bool OsmAnd::MapRenderer::setSkyColor(const FColorRGB& color, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.skyColor != color;
    if (!update)
        return false;

    _requestedState.skyColor = color;

    notifyRequestedStateWasUpdated(MapRendererStateChange::SkyColor);

    return true;
}

bool OsmAnd::MapRenderer::setAzimuth(const float azimuth, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    return setAzimuthToState(_requestedState, azimuth, forcedUpdate);
}

bool OsmAnd::MapRenderer::setElevationAngle(const float elevationAngle, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (elevationAngle <= 0.0f || elevationAngle > 90.0f)
        return false;

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.elevationAngle, elevationAngle);
    if (!update)
        return false;

    _requestedState.elevationAngle = elevationAngle;

    setMapTarget(_requestedState, forcedUpdate);

    notifyRequestedStateWasUpdated(MapRendererStateChange::ElevationAngle);

    return true;
}

bool OsmAnd::MapRenderer::setTarget(
    const PointI& target31_, bool forcedUpdate /*= false*/, bool disableUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto target31 = Utilities::normalizeCoordinates(target31_, ZoomLevel31);
    bool update = forcedUpdate || (_requestedState.target31 != target31);
    if (!update)
        return false;

    _requestedState.target31 = target31;
    _requestedState.surfaceZoomLevel = getSurfaceZoom(_requestedState, _requestedState.surfaceVisualZoom);

    if (disableUpdate)
        return true;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Target);

    return true;
}

bool OsmAnd::MapRenderer::setMapTarget(const PointI& screenPoint_, const PointI& location31_,
    bool forcedUpdate /*= false*/, bool disableUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (screenPoint_.x < 0 || screenPoint_.y < 0)
    {
        _requestedState.fixedPixel = screenPoint_;
        return false;
    }

    const auto location31 = Utilities::normalizeCoordinates(location31_, ZoomLevel31);
    bool sameHeight = _requestedState.fixedHeight != 0.0f && _requestedState.fixedLocation31 == location31 &&
        _requestedState.fixedZoomLevel == _requestedState.zoomLevel;
    auto height = sameHeight ? _requestedState.fixedHeight : getHeightOfLocation(_requestedState, location31);
    PointI target31;
    bool haveTarget = getNewTargetByScreenPoint(_requestedState, screenPoint_, location31, target31, height);
    if (!haveTarget)
        return false;

    if (target31.x < 0)
    {
        height = 0.0f;
        target31 = location31;
    }

    bool update = forcedUpdate
        || _requestedState.fixedPixel != screenPoint_
        || _requestedState.fixedLocation31 != location31
        || _requestedState.fixedHeight != height
        || _requestedState.fixedZoomLevel != _requestedState.zoomLevel
        || _requestedState.target31 != target31;
    if (!update)
        return false;

    _requestedState.fixedPixel = screenPoint_;
    _requestedState.fixedLocation31 = location31;
    _requestedState.fixedHeight = height;
    _requestedState.fixedZoomLevel = _requestedState.zoomLevel;
    _requestedState.target31 = target31;
    _requestedState.surfaceZoomLevel = getSurfaceZoom(_requestedState, _requestedState.surfaceVisualZoom);

    setSecondaryTarget(_requestedState, forcedUpdate, disableUpdate);

    if (disableUpdate)
        return true;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Target);

    return true;
}

bool OsmAnd::MapRenderer::setMapTargetLocationToState(MapRendererState& state,
    const PointI& location31_, const bool forcedUpdate /*= false*/, const bool disableUpdate /*= false*/)
{
    const auto location31 = Utilities::normalizeCoordinates(location31_, ZoomLevel31);
    const bool toTarget = state.fixedPixel.x < 0 || state.fixedPixel.y < 0;
    bool update = forcedUpdate ||
        (location31 != (toTarget ? state.target31 : state.fixedLocation31));
    if (!update)
        return false;

    if (toTarget)
    {
        state.target31 = location31;
        state.surfaceZoomLevel = state.zoomLevel;
        state.surfaceVisualZoom = state.visualZoom;

        if (!disableUpdate)
            notifyRequestedStateWasUpdated(MapRendererStateChange::Target);

        return true;
    }
    else
    {
        auto result = setMapTargetOnly(state, location31, 0.0f, forcedUpdate, disableUpdate);
        if (result)
        {
            state.surfaceZoomLevel = getSurfaceZoom(state, state.surfaceVisualZoom);
            setSecondaryTarget(state, forcedUpdate, disableUpdate);
        }
        return result;
    }
}

bool OsmAnd::MapRenderer::setMapTarget(MapRendererState& state,
    bool forcedUpdate /*= false*/, bool disableUpdate /*= false*/, bool ignoreSecondaryTarget /*= false*/)
{
    auto zoomLevel = state.surfaceZoomLevel;
    auto visualZoom = state.surfaceVisualZoom;
    if (state.fixedPixel.x >= 0 && state.fixedPixel.y >= 0)
    {
        auto height = state.fixedHeight;
        zoomLevel = state.fixedZoomLevel;
        if (height == 0.0f)
        {
            zoomLevel = state.zoomLevel;
            height = getHeightOfLocation(state, state.fixedLocation31);
        }
        const auto pointElevation = static_cast<double>(height) / static_cast<double>(1u << zoomLevel);
        zoomLevel = getFlatZoom(state, state.surfaceZoomLevel, state.surfaceVisualZoom, pointElevation, visualZoom);
    }
    bool update = forcedUpdate || state.zoomLevel != zoomLevel || state.visualZoom != visualZoom;
    if (update)
    {
        state.zoomLevel = zoomLevel;
        state.visualZoom = visualZoom;
        if (!disableUpdate)
            notifyRequestedStateWasUpdated(MapRendererStateChange::Zoom);
    }
    auto result = setMapTargetOnly(state, state.fixedLocation31, 0.0f, forcedUpdate, disableUpdate);
    if (!ignoreSecondaryTarget && (result || update))
        setSecondaryTarget(state, forcedUpdate, disableUpdate);

    return result;
}

bool OsmAnd::MapRenderer::setMapTargetOnly(MapRendererState& state,
    const PointI& location31, const float heightInMeters,
    bool forcedUpdate /*= false*/, bool disableUpdate /*= false*/)
{
    if (state.fixedPixel.x < 0 || state.fixedPixel.y < 0)
        return false;

    bool sameHeight = state.fixedHeight != 0.0f && state.fixedLocation31 == location31 &&
        state.fixedZoomLevel == state.zoomLevel;
    auto height = sameHeight ? state.fixedHeight : getHeightOfLocation(state, location31);
    if (height == 0.0f && heightInMeters != 0.0f)
        height = getWorldElevationOfLocation(state, heightInMeters, location31);
    if (height == 0.0f && state.fixedLocation31 == location31)
    {
        const float zoomDelta = state.zoomLevel - state.fixedZoomLevel;
        if (zoomDelta >= -1.0f && zoomDelta <= 1.0f)
            height = state.fixedHeight * (zoomDelta * 0.75f + (zoomDelta == 0.0f ? 1.0f : 1.25f));
        sameHeight = true;
    }
    else
        sameHeight = false;

    PointI target31;
    bool haveTarget = getNewTargetByScreenPoint(state, state.fixedPixel, location31, target31, height);
    if(!haveTarget)
        return false;

    if (target31.x < 0)
    {
        height = 0.0f;
        target31 = location31;
        sameHeight = false;
    }

    if (!sameHeight)
    {
        state.fixedHeight = height;
        state.fixedZoomLevel = state.zoomLevel;
    }

    state.fixedLocation31 = location31;

    bool update = forcedUpdate || state.target31 != target31;
    if (!update)
        return false;

    state.target31 = target31;

    if (disableUpdate)
        return true;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Target);

    return true;
}

bool OsmAnd::MapRenderer::setZoomToState(MapRendererState& state,
    const float zoom, const bool forcedUpdate /*= false*/, const bool disableUpdate /*= false*/)
{
    const auto zoomLevel = qBound(
        state.minZoomLimit,
        static_cast<ZoomLevel>(qRound(zoom)),
        state.maxZoomLimit);

    const auto zoomFraction = zoom - zoomLevel;
    if (zoomFraction < -0.5f || zoomFraction > 0.5f)
        return false;
    const auto visualZoom = Utilities::zoomFractionToVisual(zoomFraction);

    return setZoomToState(state, zoomLevel, visualZoom, forcedUpdate, disableUpdate);
}

bool OsmAnd::MapRenderer::setZoomToState(MapRendererState& state,
    const ZoomLevel zoomLevel, const float visualZoom, const bool forcedUpdate /*= false*/, const bool disableUpdate /*= false*/)
{
    if (zoomLevel < state.minZoomLimit || zoomLevel > state.maxZoomLimit)
        return false;

    bool update =
        forcedUpdate ||
        (state.surfaceZoomLevel != zoomLevel) ||
        !qFuzzyCompare(state.surfaceVisualZoom, visualZoom);
    if (!update)
        return false;

    state.surfaceZoomLevel = zoomLevel;
    state.surfaceVisualZoom = visualZoom;

    setMapTarget(state, forcedUpdate, disableUpdate);

    return true;
}

bool OsmAnd::MapRenderer::setFlatZoomToState(MapRendererState& state,
    const float zoom, const bool forcedUpdate /*= false*/, const bool disableUpdate /*= false*/)
{
    const auto zoomLevel = qBound(
        state.minZoomLimit,
        static_cast<ZoomLevel>(qRound(zoom)),
        state.maxZoomLimit);

    const auto zoomFraction = zoom - zoomLevel;
    if (zoomFraction < -0.5f || zoomFraction > 0.5f)
        return false;
    const auto visualZoom = Utilities::zoomFractionToVisual(zoomFraction);

    return setFlatZoomToState(state, zoomLevel, visualZoom, forcedUpdate, disableUpdate);
}

bool OsmAnd::MapRenderer::setFlatZoomToState(MapRendererState& state,
    const ZoomLevel zoomLevel, const float visualZoom, const bool forcedUpdate /*= false*/, const bool disableUpdate /*= false*/)
{
    if (zoomLevel < state.minZoomLimit || zoomLevel > state.maxZoomLimit)
        return false;

    bool update =
        forcedUpdate ||
        (state.zoomLevel != zoomLevel) ||
        !qFuzzyCompare(state.visualZoom, visualZoom);
    if (!update)
        return false;

    state.zoomLevel = zoomLevel;
    state.visualZoom = visualZoom;

    setMapTargetOnly(state, state.fixedLocation31, 0.0f, forcedUpdate, disableUpdate);

    state.surfaceZoomLevel = getSurfaceZoom(state, state.surfaceVisualZoom);
    if (state.surfaceVisualZoom == 1.0
        && (state.surfaceZoomLevel == state.minZoomLimit || state.surfaceZoomLevel == state.maxZoomLimit))
        setMapTarget(state, forcedUpdate, disableUpdate, true);

    setSecondaryTarget(state, forcedUpdate, disableUpdate);

    if (!disableUpdate)
        notifyRequestedStateWasUpdated(MapRendererStateChange::Zoom);

    return true;
}

bool OsmAnd::MapRenderer::setAzimuthToState(MapRendererState& state,
    const float azimuth, const bool forcedUpdate /*= false*/, const bool disableUpdate /*= false*/)
{
    const float normalizedAzimuth = Utilities::normalizedAngleDegrees(azimuth);

    bool update = forcedUpdate || !qFuzzyCompare(state.azimuth, normalizedAzimuth);
    if (!update)
        return false;

    state.azimuth = normalizedAzimuth;

    setMapTarget(state, forcedUpdate, disableUpdate);

    if (!disableUpdate)
        notifyRequestedStateWasUpdated(MapRendererStateChange::Azimuth);

    return true;
}

bool OsmAnd::MapRenderer::resetMapTarget()
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (_requestedState.fixedPixel.x < 0 || _requestedState.fixedPixel.y < 0)
        return false;

    PointI location31 = _requestedState.fixedLocation31;
    float height = 0.0f;
    bool found = getLocationFromElevatedPoint(_requestedState, _requestedState.fixedPixel, location31, &height);
    _requestedState.fixedLocation31 = location31;
    _requestedState.fixedHeight = found ? getWorldElevationOfLocation(_requestedState, height, location31) : 0.0f;
    _requestedState.fixedZoomLevel = _requestedState.zoomLevel;
    _requestedState.surfaceZoomLevel = getSurfaceZoom(_requestedState, _requestedState.surfaceVisualZoom);

    return true;
}

bool OsmAnd::MapRenderer::resetMapTargetPixelCoordinates(const PointI& screenPoint_)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (screenPoint_.x < 0 || screenPoint_.y < 0)
        return false;

    if (_requestedState.fixedPixel != screenPoint_)
    {
        PointI location31 = _requestedState.fixedLocation31;
        float height = 0.0f;
        bool found = getLocationFromElevatedPoint(_requestedState, screenPoint_, location31, &height);
        _requestedState.fixedPixel = screenPoint_;
        _requestedState.fixedLocation31 = location31;
        _requestedState.fixedHeight = found ? getWorldElevationOfLocation(_requestedState, height, location31) : 0.0f;
        _requestedState.fixedZoomLevel = _requestedState.zoomLevel;
        _requestedState.surfaceZoomLevel = getSurfaceZoom(_requestedState, _requestedState.surfaceVisualZoom);
    }

    return true;
}

bool OsmAnd::MapRenderer::setMapTargetPixelCoordinates(const PointI& screenPoint_,
    bool forcedUpdate /*= false*/, bool disableUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (screenPoint_.x < 0 || screenPoint_.y < 0)
    {
        _requestedState.fixedPixel = screenPoint_;
        return false;
    }

    PointI target31;
    bool haveTarget = getNewTargetByScreenPoint(_requestedState,
        screenPoint_, _requestedState.fixedLocation31, target31, _requestedState.fixedHeight);
    if(!haveTarget)
        return false;

    if (target31.x < 0)
    {
        _requestedState.fixedHeight = 0.0f;
        _requestedState.surfaceZoomLevel = _requestedState.zoomLevel;
        _requestedState.surfaceVisualZoom = _requestedState.visualZoom;
        target31 = _requestedState.fixedLocation31;
    }

    _requestedState.fixedPixel = screenPoint_;

    bool update = forcedUpdate || _requestedState.target31 != target31;
    if (!update)
        return false;

    _requestedState.target31 = target31;
    _requestedState.surfaceZoomLevel = getSurfaceZoom(_requestedState, _requestedState.surfaceVisualZoom);

    setSecondaryTarget(_requestedState, forcedUpdate, disableUpdate);

    if (disableUpdate)
        return true;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Target);

    return true;
}

bool OsmAnd::MapRenderer::setMapTargetLocation(const PointI& location31_,
    bool forcedUpdate /*= false*/, bool disableUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    return setMapTargetLocationToState(_requestedState, location31_, forcedUpdate, disableUpdate);
}

bool OsmAnd::MapRenderer::setSecondaryTarget(MapRendererState& state,
    bool forcedUpdate /*= false*/, bool disableUpdate /*= false*/)
{
    if (state.fixedPixel.x < 0 || state.fixedPixel.y < 0 || state.aimPixel.x < 0 || state.aimPixel.y < 0)
        return false;

    auto startAzimuth = state.azimuth;
    auto startElevationAngle = state.elevationAngle;

    auto azimuth = state.azimuth;
    auto zoomLevel = state.zoomLevel;
    auto visualZoom = state.visualZoom;
    auto elevationAngle = state.elevationAngle;

    const auto aimingActions = state.aimingActions;
    if (aimingActions.none())
        return false;
    
    bool isTiltAndRotation = false;
    bool isZoomAndRotation = false;
    PointD tiltAndRotation;
    PointD zoomAndRotation;
    if (!aimingActions[AimingAction::Zoom])
        isTiltAndRotation = getTiltAndRotationForAiming(
            state,
            state.fixedLocation31,
            state.fixedHeight,
            state.fixedPixel,
            state.aimLocation31,
            state.aimHeight,
            state.aimPixel,
            tiltAndRotation);
    else if (!aimingActions[AimingAction::Elevation])
        isZoomAndRotation = getExtraZoomAndRotationForAiming(
            state,
            state.fixedLocation31,
            getElevationOfLocationInMeters(state, state.fixedHeight, state.fixedZoomLevel, state.fixedLocation31),
            state.fixedPixel,
            state.aimLocation31,
            getElevationOfLocationInMeters(state, state.aimHeight, state.aimZoomLevel, state.aimLocation31),
            state.aimPixel,
            zoomAndRotation);
    else
    {
        isTiltAndRotation = getTiltAndRotationForAiming(
            state,
            state.fixedLocation31,
            state.fixedHeight,
            state.fixedPixel,
            state.aimLocation31,
            state.aimHeight,
            state.aimPixel,
            tiltAndRotation);
        if (isTiltAndRotation)
        {
            const auto halfTilt = (tiltAndRotation.x - state.elevationAngle) / 2.0 + state.elevationAngle;
            const auto halfRotation =
                Utilities::normalizedAngleDegrees((tiltAndRotation.y - state.azimuth) / 2.0 + state.azimuth);
            state.elevationAngle = static_cast<float>(halfTilt);
            state.azimuth = static_cast<float>(halfRotation);
            setMapTarget(state, forcedUpdate, disableUpdate, true);
            state.surfaceZoomLevel = getSurfaceZoom(state, state.surfaceVisualZoom);
        }
        isZoomAndRotation = getExtraZoomAndRotationForAiming(
            state,
            state.fixedLocation31,
            getElevationOfLocationInMeters(state, state.fixedHeight, state.fixedZoomLevel, state.fixedLocation31),
            state.fixedPixel,
            state.aimLocation31,
            getElevationOfLocationInMeters(state, state.aimHeight, state.aimZoomLevel, state.aimLocation31),
            state.aimPixel,
            zoomAndRotation);
    }

    if (isTiltAndRotation)
    {
        if (aimingActions[AimingAction::Elevation])
            elevationAngle = static_cast<float>(tiltAndRotation.x);
        if (aimingActions[AimingAction::Azimuth])
            azimuth = static_cast<float>(tiltAndRotation.y);
    }

    if (isZoomAndRotation)
    {
        float zoom = visualZoom - 1.0f;
        if (zoom < 0.0f)
            zoom *= 2.0f;
        zoom += static_cast<float>(zoomLevel) + static_cast<float>(zoomAndRotation.x);
        zoomLevel = qBound(state.minZoomLimit, static_cast<ZoomLevel>(qRound(zoom)), state.maxZoomLimit);
        const auto zoomFraction = qBound(
            zoomLevel > state.minZoomLimit ? -0.5f : 0.0f,
            zoom - zoomLevel,
            zoomLevel < state.maxZoomLimit ? 0.5f : 0.0f);
        visualZoom = 1.0f + (zoomFraction >= 0.0f ? zoomFraction : zoomFraction * 0.5f);
        if (aimingActions[AimingAction::Azimuth])
            azimuth = static_cast<float>(Utilities::normalizedAngleDegrees(state.azimuth + zoomAndRotation.y));
    }

    bool azimuthIsModified = startAzimuth != azimuth;
    bool zoomIsModified = state.zoomLevel != zoomLevel || !qFuzzyCompare(state.visualZoom, visualZoom);
    bool elevationAngleIsModified = startElevationAngle != elevationAngle;
    
    bool result = azimuthIsModified || zoomIsModified || elevationAngleIsModified;
    bool update = forcedUpdate || result;
    if (!update)
        return false;

    if (azimuthIsModified)
        state.azimuth = azimuth;
    if (zoomIsModified)
    {
        state.zoomLevel = zoomLevel;
        state.visualZoom = visualZoom;
    }
    if (elevationAngleIsModified)
        state.elevationAngle = elevationAngle;

    if (result)
    {
        result = setMapTargetOnly(state, state.fixedLocation31, 0.0f, forcedUpdate, disableUpdate);
        state.surfaceZoomLevel = getSurfaceZoom(state, state.surfaceVisualZoom);
        if (state.surfaceVisualZoom == 1.0
            && (state.surfaceZoomLevel == state.minZoomLimit || state.surfaceZoomLevel == state.maxZoomLimit))
            setMapTarget(state, forcedUpdate, disableUpdate, true);
    }

    if (disableUpdate)
        return result;

    if (azimuthIsModified)
        notifyRequestedStateWasUpdated(MapRendererStateChange::Azimuth);
    if (zoomIsModified)
        notifyRequestedStateWasUpdated(MapRendererStateChange::Zoom);
    if (elevationAngleIsModified)
        notifyRequestedStateWasUpdated(MapRendererStateChange::ElevationAngle);

    return result;
}

bool OsmAnd::MapRenderer::setSecondaryTarget(const PointI& screenPoint_, const PointI& location31_,
    bool forcedUpdate /*= false*/, bool disableUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (screenPoint_.x < 0 || screenPoint_.y < 0)
    {
        _requestedState.aimPixel = screenPoint_;
        return false;
    }

    const auto location31 = Utilities::normalizeCoordinates(location31_, ZoomLevel31);
    bool sameHeight = _requestedState.aimHeight != 0.0f && _requestedState.aimLocation31 == location31 &&
        _requestedState.aimZoomLevel == _requestedState.zoomLevel;
    auto height = sameHeight ? _requestedState.aimHeight : getHeightOfLocation(_requestedState, location31);

    bool update = forcedUpdate
        || _requestedState.aimPixel != screenPoint_
        || _requestedState.aimLocation31 != location31
        || _requestedState.aimHeight != height
        || _requestedState.aimZoomLevel != _requestedState.zoomLevel;
    if (!update)
        return false;

    _requestedState.aimPixel = screenPoint_;
    _requestedState.aimLocation31 = location31;
    _requestedState.aimHeight = height;
    _requestedState.aimZoomLevel = _requestedState.zoomLevel;

    setSecondaryTarget(_requestedState, forcedUpdate, disableUpdate);

    if (disableUpdate)
        return true;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Target);

    return true;
}

bool OsmAnd::MapRenderer::setSecondaryTargetPixelCoordinates(const PointI& screenPoint_,
    bool forcedUpdate /*= false*/, bool disableUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (screenPoint_.x < 0 || screenPoint_.y < 0)
    {
        _requestedState.aimPixel = screenPoint_;
        return false;
    }

    bool update = forcedUpdate || _requestedState.aimPixel != screenPoint_;
    if (!update)
        return false;

    _requestedState.aimPixel = screenPoint_;

    setSecondaryTarget(_requestedState, forcedUpdate, disableUpdate);

    if (disableUpdate)
        return true;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Target);

    return true;
}

bool OsmAnd::MapRenderer::setSecondaryTargetLocation(const PointI& location31_,
    bool forcedUpdate /*= false*/, bool disableUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (_requestedState.aimPixel.x < 0 || _requestedState.aimPixel.y < 0)
        return false;

    const auto location31 = Utilities::normalizeCoordinates(location31_, ZoomLevel31);

    bool update = forcedUpdate || _requestedState.aimLocation31 != location31;
    if (!update)
        return false;

    _requestedState.aimLocation31 = location31;

    setSecondaryTarget(_requestedState, forcedUpdate, disableUpdate);

    if (disableUpdate)
        return true;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Target);

    return true;
}

int OsmAnd::MapRenderer::getAimingActions()
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    return static_cast<int>(_requestedState.aimingActions.to_ulong());
}

bool OsmAnd::MapRenderer::setAimingActions(const int actionBits, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto aimingActions = AimingActions(actionBits);

    bool update = forcedUpdate || _requestedState.aimingActions != aimingActions;
    if (!update)
        return false;

    _requestedState.aimingActions = aimingActions;

    setSecondaryTarget(_requestedState, forcedUpdate);

    return true;
}

bool OsmAnd::MapRenderer::setFlatZoom(const float zoom, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    return setFlatZoomToState(_requestedState, zoom, forcedUpdate);
}

bool OsmAnd::MapRenderer::setFlatZoom(const ZoomLevel zoomLevel, const float visualZoom, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    return setFlatZoomToState(_requestedState, zoomLevel, visualZoom, forcedUpdate);
}

bool OsmAnd::MapRenderer::setFlatZoomLevel(const ZoomLevel zoomLevel, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (zoomLevel < _requestedState.minZoomLimit || zoomLevel > _requestedState.maxZoomLimit)
        return false;

    bool update =
        forcedUpdate ||
        (_requestedState.zoomLevel != zoomLevel);
    if (!update)
        return false;

    _requestedState.zoomLevel = zoomLevel;

    setMapTargetOnly(_requestedState, _requestedState.fixedLocation31, 0.0f, forcedUpdate);

    _requestedState.surfaceZoomLevel = getSurfaceZoom(_requestedState, _requestedState.surfaceVisualZoom);
    if (_requestedState.surfaceVisualZoom == 1.0
        && (_requestedState.surfaceZoomLevel == _requestedState.minZoomLimit
        || _requestedState.surfaceZoomLevel == _requestedState.maxZoomLimit))
        setMapTarget(_requestedState, forcedUpdate, false, true);

    setSecondaryTarget(_requestedState, forcedUpdate);

    notifyRequestedStateWasUpdated(MapRendererStateChange::Zoom);

    return true;
}

bool OsmAnd::MapRenderer::setFlatVisualZoom(const float visualZoom, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update =
        forcedUpdate ||
        !qFuzzyCompare(_requestedState.visualZoom, visualZoom);
    if (!update)
        return false;

    _requestedState.visualZoom = visualZoom;

    setMapTargetOnly(_requestedState, _requestedState.fixedLocation31, 0.0f, forcedUpdate);

    _requestedState.surfaceZoomLevel = getSurfaceZoom(_requestedState, _requestedState.surfaceVisualZoom);
    if (_requestedState.surfaceVisualZoom == 1.0
        && (_requestedState.surfaceZoomLevel == _requestedState.minZoomLimit
        || _requestedState.surfaceZoomLevel == _requestedState.maxZoomLimit))
        setMapTarget(_requestedState, forcedUpdate, false, true);

    setSecondaryTarget(_requestedState, forcedUpdate);

    notifyRequestedStateWasUpdated(MapRendererStateChange::Zoom);

    return true;
}

bool OsmAnd::MapRenderer::setZoom(const float zoom, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    return setZoomToState(_requestedState, zoom, forcedUpdate);
}

bool OsmAnd::MapRenderer::setZoom(const ZoomLevel zoomLevel, const float visualZoom, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    return setZoomToState(_requestedState, zoomLevel, visualZoom, forcedUpdate);
}

bool OsmAnd::MapRenderer::setZoomLevel(const ZoomLevel zoomLevel, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (zoomLevel < _requestedState.minZoomLimit || zoomLevel > _requestedState.maxZoomLimit)
        return false;

    bool update =
        forcedUpdate ||
        (_requestedState.surfaceZoomLevel != zoomLevel);
    if (!update)
        return false;

    _requestedState.surfaceZoomLevel = zoomLevel;

    setMapTarget(_requestedState, forcedUpdate);

    return true;
}

bool OsmAnd::MapRenderer::setVisualZoom(const float visualZoom, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update =
        forcedUpdate ||
        !qFuzzyCompare(_requestedState.visualZoom, visualZoom);
    if (!update)
        return false;

    _requestedState.surfaceVisualZoom = visualZoom;

    setMapTarget(_requestedState, forcedUpdate);

    return true;
}

bool OsmAnd::MapRenderer::setVisualZoomShift(const float visualZoomShift, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update =
        forcedUpdate ||
        !qFuzzyCompare(_requestedState.visualZoomShift, visualZoomShift);
    if (!update)
        return false;

    _requestedState.visualZoomShift = visualZoomShift;

    setMapTarget(_requestedState, forcedUpdate);

    notifyRequestedStateWasUpdated(MapRendererStateChange::Zoom);

    return true;
}

bool OsmAnd::MapRenderer::restoreFlatZoom(const float heightInMeters, bool forcedUpdate /*= false*/)
{
    bool result = true;

    if (_requestedState.elevationDataProvider)
    {
        QMutexLocker scopedLocker(&_requestedStateMutex);

        // Store current zoom level
        const auto zoom = _requestedState.zoomLevel; 
        
        // Set the safe zoom level
        _requestedState.zoomLevel = qMax(ZoomLevel10, _requestedState.elevationDataProvider->getMinZoom());

        // Set target pixel elevation
        setMapTargetOnly(_requestedState, _requestedState.fixedLocation31, heightInMeters, forcedUpdate);

        // Restore current zoom level
        _requestedState.zoomLevel = zoom;

        // Restore flat zoom
        result = setMapTarget(_requestedState, forcedUpdate, false, true);

        // Restore target pixel elevation
        if (result)
            setMapTargetOnly(_requestedState, _requestedState.fixedLocation31, heightInMeters, forcedUpdate);
    }

    return result;
}

bool OsmAnd::MapRenderer::setStubsStyle(const MapStubStyle style, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (style == MapStubStyle::Unspecified)
        return false;

    bool update = forcedUpdate || _requestedState.stubsStyle != style;
    if (!update)
        return false;

    _requestedState.stubsStyle = style;

    notifyRequestedStateWasUpdated(MapRendererStateChange::StubsStyle);

    return true;
}

bool OsmAnd::MapRenderer::setBackgroundColor(const FColorRGB& color, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.backgroundColor != color;
    if (!update)
        return false;

    _requestedState.backgroundColor = color;

    notifyRequestedStateWasUpdated(MapRendererStateChange::BackgroundColor);

    return true;
}

bool OsmAnd::MapRenderer::setFogColor(const FColorRGB& color, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.fogColor != color;
    if (!update)
        return false;

    _requestedState.fogColor = color;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FogColor);

    return true;
}

bool OsmAnd::MapRenderer::setMyLocationColor(const FColorARGB& color, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.myLocationColor != color;
    if (!update)
        return false;

    _requestedState.myLocationColor = color;

    notifyRequestedStateWasUpdated(MapRendererStateChange::MyLocation);

    return true;
}

bool OsmAnd::MapRenderer::setMyLocation31(const PointI& location31, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto myLocation31 = Utilities::normalizeCoordinates(location31, ZoomLevel31);
    bool update = forcedUpdate || _requestedState.myLocation31 != myLocation31;
    if (!update)
        return false;

    _requestedState.myLocation31 = myLocation31;

    notifyRequestedStateWasUpdated(MapRendererStateChange::MyLocation);

    return true;
}

bool OsmAnd::MapRenderer::setMyLocationRadiusInMeters(const float radius, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.myLocationRadiusInMeters != radius;
    if (!update)
        return false;

    _requestedState.myLocationRadiusInMeters = radius;

    notifyRequestedStateWasUpdated(MapRendererStateChange::MyLocation);

    return true;
}

bool OsmAnd::MapRenderer::setSymbolsOpacity(const float opacityFactor, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.symbolsOpacity != opacityFactor;
    if (!update)
        return false;

    _requestedState.symbolsOpacity = opacityFactor;

    notifyRequestedStateWasUpdated(MapRendererStateChange::SymbolsOpacity);

    return true;
}

float OsmAnd::MapRenderer::getSymbolsOpacity() const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    return _requestedState.symbolsOpacity;
}

bool OsmAnd::MapRenderer::setDateTime(const int64_t dateTime, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.dateTime != dateTime;
    if (!update)
        return false;

    _requestedState.dateTime = dateTime;

    notifyRequestedStateWasUpdated(MapRendererStateChange::DateTime);

    return true;
}

bool OsmAnd::MapRenderer::changeTimePeriod()
{
    notifyRequestedStateWasUpdated(MapRendererStateChange::TimePeriod);

    return true;
}

bool OsmAnd::MapRenderer::getMapTargetLocation(PointI& location31) const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (_requestedState.fixedPixel.x < 0 || _requestedState.fixedPixel.y < 0)
        location31 = _requestedState.target31;
    else
        location31 = _requestedState.fixedLocation31;
    return true;
}

bool OsmAnd::MapRenderer::getSecondaryTargetLocation(PointI& location31) const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    location31 = _requestedState.aimLocation31;

    return true;
}

float OsmAnd::MapRenderer::getMapTargetHeightInMeters() const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (_requestedState.fixedPixel.x < 0 || _requestedState.fixedPixel.y < 0)
        return 0.0f;
    else
    {
        return getElevationOfLocationInMeters(_requestedState,
            _requestedState.fixedHeight, _requestedState.fixedZoomLevel, _requestedState.fixedLocation31);
    }
}

float OsmAnd::MapRenderer::getSurfaceZoomAfterPinch(
    const PointI& firstLocation31, const float firstHeightInMeters, const PointI& firstScreenPoint,
    const PointI& secondLocation31, const float secondHeightInMeters, const PointI& secondScreenPoint)
{
    PointD zoomAndRotation;
    bool ok = getZoomAndRotationAfterPinch(
        firstLocation31, firstHeightInMeters, firstScreenPoint,
        secondLocation31, secondHeightInMeters, secondScreenPoint,
        zoomAndRotation);
    if (!ok)
        return -1.0f;

    auto state = getState();
    const float expectedFlatZoom = state.zoomLevel
        + Utilities::visualZoomToFraction(state.visualZoom)
        + static_cast<float>(zoomAndRotation.x);
    setFlatZoomToState(state, expectedFlatZoom, false, true);

    state.surfaceZoomLevel = getSurfaceZoom(state, state.surfaceVisualZoom);
    return state.surfaceZoomLevel + Utilities::visualZoomToFraction(state.surfaceVisualZoom);
}

float OsmAnd::MapRenderer::getSurfaceZoomAfterPinchWithParams(
    const PointI& fixedLocation31, float surfaceZoom, float rotation,
    const PointI& firstLocation31, const float firstHeightInMeters, const PointI& firstScreenPoint,
    const PointI& secondLocation31, const float secondHeightInMeters, const PointI& secondScreenPoint)
{
    auto state = getState();

    setMapTargetLocationToState(state, fixedLocation31, false, true);
    setZoomToState(state, surfaceZoom, false, true);
    setAzimuthToState(state, rotation, false, true);

    PointD zoomAndRotation;
    const bool ok = getExtraZoomAndRotationForAiming(state,
        firstLocation31, firstHeightInMeters, firstScreenPoint,
        secondLocation31, secondHeightInMeters, secondScreenPoint,
        zoomAndRotation);
    if (!ok)
        return -1.0f;

    const auto expectedFlatZoom = state.zoomLevel
        + Utilities::visualZoomToFraction(state.visualZoom)
        + static_cast<float>(zoomAndRotation.x);
    setFlatZoomToState(state, expectedFlatZoom, false, true);
    
    state.surfaceZoomLevel = getSurfaceZoom(state, state.surfaceVisualZoom);
    return state.surfaceZoomLevel + Utilities::visualZoomToFraction(state.surfaceVisualZoom);
}

OsmAnd::ZoomLevel OsmAnd::MapRenderer::getMinZoomLevel() const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    return _requestedState.minZoomLimit;
}

OsmAnd::ZoomLevel OsmAnd::MapRenderer::getMaxZoomLevel() const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    return _requestedState.maxZoomLimit;
}

bool OsmAnd::MapRenderer::setMinZoomLevel(const ZoomLevel zoomLevel, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (zoomLevel < MinZoomLevel || zoomLevel > MaxZoomLevel)
        return false;

    bool update = forcedUpdate || _requestedState.minZoomLimit != zoomLevel;
    if (!update)
        return false;

    _requestedState.minZoomLimit = zoomLevel;

    if (_requestedState.zoomLevel < zoomLevel)
    {
        _requestedState.zoomLevel = zoomLevel;
        notifyRequestedStateWasUpdated(MapRendererStateChange::Zoom);
    }

    if (_requestedState.surfaceZoomLevel < zoomLevel)
    {
        _requestedState.surfaceZoomLevel = zoomLevel;
        notifyRequestedStateWasUpdated(MapRendererStateChange::Zoom);
    }

    return true;
}

bool OsmAnd::MapRenderer::setMaxZoomLevel(const ZoomLevel zoomLevel, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (zoomLevel < MinZoomLevel || zoomLevel > MaxZoomLevel)
        return false;

    bool update = forcedUpdate || _requestedState.maxZoomLimit != zoomLevel;
    if (!update)
        return false;

    _requestedState.maxZoomLimit = zoomLevel;

    if (_requestedState.zoomLevel > zoomLevel)
    {
        _requestedState.zoomLevel = zoomLevel;
        notifyRequestedStateWasUpdated(MapRendererStateChange::Zoom);
    }

    if (_requestedState.surfaceZoomLevel > zoomLevel)
    {
        _requestedState.surfaceZoomLevel = zoomLevel;
        notifyRequestedStateWasUpdated(MapRendererStateChange::Zoom);
    }

    return true;
}

OsmAnd::ZoomLevel OsmAnd::MapRenderer::getMinimalZoomLevelsRangeLowerBound() const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (_requestedState.mapLayersProviders.isEmpty())
        return _requestedState.minZoomLimit;

    auto zoomLevel = _requestedState.minZoomLimit;
    for (const auto& provider : constOf(_requestedState.mapLayersProviders))
        zoomLevel = qMax(zoomLevel, provider->getMinZoom());
    return zoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::MapRenderer::getMinimalZoomLevelsRangeUpperBound() const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (_requestedState.mapLayersProviders.isEmpty())
        return _requestedState.maxZoomLimit;

    auto zoomLevel = _requestedState.maxZoomLimit;
    for (const auto& provider : constOf(_requestedState.mapLayersProviders))
        zoomLevel = qMin(zoomLevel, provider->getMaxZoom());
    return zoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::MapRenderer::getMaximalZoomLevelsRangeLowerBound() const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (_requestedState.mapLayersProviders.isEmpty())
        return _requestedState.minZoomLimit;

    auto zoomLevel = _requestedState.maxZoomLimit;
    for (const auto& provider : constOf(_requestedState.mapLayersProviders))
        zoomLevel = qMin(zoomLevel, provider->getMinZoom());
    return zoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::MapRenderer::getMaximalZoomLevelsRangeUpperBound() const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    if (_requestedState.mapLayersProviders.isEmpty())
        return _requestedState.maxZoomLimit;

    auto zoomLevel = _requestedState.minZoomLimit;
    for (const auto& provider : constOf(_requestedState.mapLayersProviders))
        zoomLevel = qMax(zoomLevel, provider->getMaxZoom());
    return zoomLevel;
}

int OsmAnd::MapRenderer::getMaxMissingDataZoomShift() const
{
    return MaxMissingDataZoomShift;
}

int OsmAnd::MapRenderer::getMaxMissingDataUnderZoomShift() const
{
    return MaxMissingDataUnderZoomShift;
}

int OsmAnd::MapRenderer::getHeixelsPerTileSide() const
{
    return HeixelsPerTileSide;
}

int OsmAnd::MapRenderer::getElevationDataTileSize() const
{
    return ElevationDataTileSize;
}

bool OsmAnd::MapRenderer::updateCurrentDebugSettings()
{
    QReadLocker scopedLocker(&_debugSettingsLock);

    _requestedDebugSettings->copyTo(*_currentDebugSettings);

    return true;
}

std::shared_ptr<OsmAnd::MapRendererDebugSettings> OsmAnd::MapRenderer::getDebugSettings() const
{
    QReadLocker scopedLocker(&_debugSettingsLock);

    return _requestedDebugSettings->createCopy();
}

void OsmAnd::MapRenderer::setDebugSettings(const std::shared_ptr<const MapRendererDebugSettings>& debugSettings)
{
    QWriteLocker scopedLocker(&_debugSettingsLock);

    debugSettings->copyTo(*_requestedDebugSettings);
    _currentDebugSettingsInvalidatedCounter.fetchAndAddOrdered(1);

    invalidateFrame();
}

void OsmAnd::MapRenderer::useJSON()
{
    _jsonEnabled = true;
}

bool OsmAnd::MapRenderer::withJSON() const
{
    return _jsonEnabled;
}

void OsmAnd::MapRenderer::setJSON(const QJsonDocument* jsonDocument)
{
    QWriteLocker scopedLocker(&_jsonDocumentLock);

    _jsonDocument.reset(jsonDocument);
}

QByteArray OsmAnd::MapRenderer::getJSON() const
{
    QReadLocker scopedLocker(&_jsonDocumentLock);

    return _jsonDocument ? _jsonDocument->toJson() : QByteArray();
}

void OsmAnd::MapRenderer::setResourceWorkerThreadsLimit(const unsigned int limit)
{
    _resources->setResourceWorkerThreadsLimit(limit);
}

void OsmAnd::MapRenderer::resetResourceWorkerThreadsLimit()
{
    _resources->resetResourceWorkerThreadsLimit();
}

unsigned int OsmAnd::MapRenderer::getActiveResourceRequestsCount() const
{
    return static_cast<unsigned int>(_resources->_resourcesRequestTasksCounter.loadAcquire());
}

void OsmAnd::MapRenderer::dumpResourcesInfo() const
{
    getResources().dumpResourcesInfo();
}

int OsmAnd::MapRenderer::getWaitTime() const
{
    return gpuAPI->waitTimeInMicroseconds.fetchAndStoreOrdered(0) / 1000;
}
