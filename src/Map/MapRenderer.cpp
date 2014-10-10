#include "MapRenderer.h"

#include <cassert>
#include <algorithm>

#include <OsmAndCore/QtExtensions.h>
#include <QMutableHashIterator>

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include "restore_internal_warnings.h"

#include "IMapRenderer_Metrics.h"
#include "MapRendererInternalState.h"
#include "MapRendererResourcesManager.h"
#include "IMapRasterBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "MapSymbol.h"
#include "GPUAPI.h"
#include "Utilities.h"
#include "Logging.h"
#include "Stopwatch.h"
#include "QKeyValueIterator.h"

//#define OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE 1
#ifndef OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE
#   define OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE 0
#endif // !defined(OSMAND_LOG_MAP_SYMBOLS_REGISTRATION_LIFECYCLE)

#define OSMAND_VERIFY_PUBLISHED_MAP_SYMBOLS_INTEGRITY 1
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
    , _gpuWorkerThreadId(nullptr)
    , _gpuWorkerIsAlive(false)
    , _renderThreadId(nullptr)
    , _currentDebugSettings(baseDebugSettings_->createCopy())
    , _currentDebugSettingsAsConst(_currentDebugSettings)
    , _requestedDebugSettings(baseDebugSettings_->createCopy())
    , setupOptions(_setupOptions)
    , currentConfiguration(_currentConfigurationAsConst)
    , currentState(_currentState)
    , publishedMapSymbolsByOrderLock(_publishedMapSymbolsByOrderLock)
    , publishedMapSymbolsByOrder(_publishedMapSymbolsByOrder)
    , currentDebugSettings(_currentDebugSettingsAsConst)
    , gpuAPI(gpuAPI_)
{
    // Fill-up default state
    for (auto layerId = 0u; layerId < RasterMapLayersCount; layerId++)
        setRasterLayerOpacity(static_cast<RasterMapLayerId>(layerId), 1.0f);
    setElevationDataScaleFactor(1.0f, true);
    setFieldOfView(16.5f, true);
    setDistanceToFog(400.0f, true);
    setFogOriginFactor(0.36f, true);
    setFogHeightOriginFactor(0.05f, true);
    setFogDensity(1.9f, true);
    setFogColor(FColorRGB(1.0f, 0.0f, 0.0f), true);
    setSkyColor(ColorRGB(140, 190, 214), true);
    setAzimuth(0.0f, true);
    setElevationAngle(45.0f, true);
    const auto centerIndex = 1u << (ZoomLevel::MaxZoomLevel - 1);
    setTarget(PointI(centerIndex, centerIndex), true);
    setZoom(0, true);
}

OsmAnd::MapRenderer::~MapRenderer()
{
    if (_isRenderingInitialized)
        releaseRendering();
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

void OsmAnd::MapRenderer::setConfiguration(const std::shared_ptr<const MapRendererConfiguration>& configuration, bool forcedUpdate /*= false*/)
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
    const bool elevationDataResolutionChanged = (current->heixelsPerTileSide != updated->heixelsPerTileSide);
    const bool texturesFilteringChanged = (current->texturesFilteringQuality != updated->texturesFilteringQuality);
    const bool paletteTexturesUsageChanged = (current->paletteTexturesAllowed != updated->paletteTexturesAllowed);

    uint32_t mask = 0;
    if (colorDepthForcingChanged)
        mask |= enumToBit(ConfigurationChange::ColorDepthForcing);
    if (elevationDataResolutionChanged)
        mask |= enumToBit(ConfigurationChange::ElevationDataResolution);
    if (texturesFilteringChanged)
        mask |= enumToBit(ConfigurationChange::TexturesFilteringMode);
    if (paletteTexturesUsageChanged)
        mask |= enumToBit(ConfigurationChange::PaletteTexturesUsage);

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
    bool invalidateRasterTextures = false;
    invalidateRasterTextures = invalidateRasterTextures || (change == ConfigurationChange::ColorDepthForcing);
    invalidateRasterTextures = invalidateRasterTextures || (change == ConfigurationChange::PaletteTexturesUsage);

    bool invalidateElevationData = false;
    invalidateElevationData = invalidateElevationData || (change == ConfigurationChange::ElevationDataResolution);

    bool invalidateSymbols = false;
    invalidateSymbols = invalidateSymbols || (change == ConfigurationChange::ColorDepthForcing);

    if (invalidateRasterTextures)
        getResources().invalidateResourcesOfType(MapRendererResourceType::RasterBitmapTile);
    if (invalidateElevationData)
        getResources().invalidateResourcesOfType(MapRendererResourceType::ElevationDataTile);
    if (invalidateSymbols)
        getResources().invalidateResourcesOfType(MapRendererResourceType::Symbols);
}

bool OsmAnd::MapRenderer::updateInternalState(
    MapRendererInternalState& outInternalState,
    const MapRendererState& state,
    const MapRendererConfiguration& configuration) const
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

    while (_gpuWorkerIsAlive)
    {
        // Check if worker was requested to pause and is allowed to be alive,
        // wait for wake-up
        while (_gpuWorkerIsPaused && _gpuWorkerIsAlive)
        {
            // Notify that GPU worker was put into pause
            {
                QMutexLocker scopedLocker(&_gpuWorkerThreadPausedMutex);
                _gpuWorkerThreadPaused.wakeAll();
            }

            // Wait to wake up
            QMutexLocker scopedLocker(&_gpuWorkerThreadWakeupMutex);
            REPEAT_UNTIL(_gpuWorkerThreadWakeup.wait(&_gpuWorkerThreadWakeupMutex));
        }

        // If worker was requested to stop, let it be so
        if (!_gpuWorkerIsAlive)
            break;

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
        } while (unprocessedRequests > 0);
    }
    else if (isInRenderThread())
    {
        // To reduce FPS drop, upload not more than 1 resource per frame.
        const auto requestsToProcess = _resourcesGpuSyncRequestsCounter.fetchAndAddOrdered(0);
        bool moreUploadThanLimitAvailable = false;
        unsigned int resourcesUploaded = 0u;
        unsigned int resourcesUnloaded = 0u;
        _resources->syncResourcesInGPU(1u, &moreUploadThanLimitAvailable, &resourcesUploaded, &resourcesUnloaded);
        const auto unprocessedRequests = _resourcesGpuSyncRequestsCounter.fetchAndAddOrdered(-requestsToProcess) - requestsToProcess;

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

bool OsmAnd::MapRenderer::initializeRendering()
{
    bool ok;

    _resourcesGpuSyncRequestsCounter.store(0);

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

    // Once rendering is initialized, invalidate frame
    invalidateFrame();

    return true;
}

bool OsmAnd::MapRenderer::preInitializeRendering()
{
    if (_isRenderingInitialized)
        return false;

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

    // Start GPU worker (if it exists)
    if (_gpuWorkerThread)
    {
        _gpuWorkerIsAlive = true;
        _gpuWorkerIsPaused = false;
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
    if (_resources->checkForUpdatesAndApply())
        invalidateFrame();
    if (metric)
        metric->elapsedTimeForUpdatesProcessing = updatesStopwatch.elapsed();

    return true;
}

bool OsmAnd::MapRenderer::doUpdate(IMapRenderer_Metrics::Metric_update* const metric)
{
    // If GPU worker thread is not enabled, upload resource to GPU from render thread.
    if (!_gpuWorkerThread)
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

    bool ok;

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

    // Deal with state
    unsigned int requestedStateUpdatedMask;
    if ((requestedStateUpdatedMask = _requestedStateUpdatedMask.fetchAndStoreOrdered(0)) != 0)
    {
        QMutexLocker scopedLocker(&_requestedStateMutex);

        // Capture new state
        _currentState = _requestedState;
    }

    if (requestedStateUpdatedMask != 0)
    {
        // Process updating of providers
        _resources->updateBindings(_currentState, requestedStateUpdatedMask);
    }

    // Update internal state, that is derived from current state and configuration
    if (requestedStateUpdatedMask != 0 || currentConfigurationInvalidatedMask != 0)
    {
        ok = updateInternalState(*getInternalStateRef(), _currentState, *currentConfiguration);

        // Anyways, invalidate the frame
        invalidateFrame();

        if (!ok)
        {
            // In case updating internal state failed, restore changes as not-applied
            if (requestedStateUpdatedMask != 0)
                _requestedStateUpdatedMask.fetchAndOrOrdered(requestedStateUpdatedMask);
            if (currentConfigurationInvalidatedMask != 0)
                _currentConfigurationInvalidatedMask.fetchAndOrOrdered(currentConfigurationInvalidatedMask);

            return false;
        }
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

    ok = ok && preRenderFrame();
    ok = ok && doRenderFrame();
    ok = ok && postRenderFrame();

    if (metric)
        metric->elapsedTime = totalStopwatch.elapsed();

    return ok;
}

bool OsmAnd::MapRenderer::preRenderFrame()
{
    if (!_isRenderingInitialized)
        return false;

    // Capture how many "frame-invalidates" are going to be processed
    _frameInvalidatesToBeProcessed = _frameInvalidatesCounter.fetchAndAddOrdered(0);

    return true;
}

bool OsmAnd::MapRenderer::postRenderFrame()
{
    // Decrement "frame-invalidates" counter by amount of processed "frame-invalidates"
    _frameInvalidatesCounter.fetchAndAddOrdered(-_frameInvalidatesToBeProcessed);
    _frameInvalidatesToBeProcessed = 0;

    return true;
}

bool OsmAnd::MapRenderer::releaseRendering()
{
    assert(_renderThreadId == QThread::currentThreadId());

    bool ok;

    ok = preReleaseRendering();
    if (!ok)
        return false;

    ok = doReleaseRendering();
    if (!ok)
        return false;

    ok = postReleaseRendering();
    if (!ok)
        return false;

    // After all release procedures, release render API
    ok = gpuAPI->release();
    if (!ok)
        return false;

    return true;
}

bool OsmAnd::MapRenderer::preReleaseRendering()
{
    if (!_isRenderingInitialized)
        return false;

    return true;
}

bool OsmAnd::MapRenderer::doReleaseRendering()
{
    return true;
}

bool OsmAnd::MapRenderer::postReleaseRendering()
{
    // Release resources (to let all resources be released)
    _resources->releaseAllResources();

    // Stop GPU worker if it exists
    if (_gpuWorkerThread)
    {
        // Deactivate worker thread
        _gpuWorkerIsAlive = false;
        _gpuWorkerIsPaused = false;

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

    _resources.reset();

    _isRenderingInitialized = false;

    return true;
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

bool OsmAnd::MapRenderer::pauseGpuWorkerThread()
{
    if (!_gpuWorkerThread)
        return false;

    // Notify that worker should be paused
    _gpuWorkerIsPaused = true;

    // Since _gpuWorkerIsPaused == true, wake up GPU worker thread to allow it to pause
    {
        QMutexLocker scopedLocker(&_gpuWorkerThreadWakeupMutex);
        _gpuWorkerThreadWakeup.wakeAll();
    }

    // Wait until worker thread will signalize that it was paused
    {
        QMutexLocker scopedLocker(&_gpuWorkerThreadPausedMutex);
        REPEAT_UNTIL(_gpuWorkerThreadPaused.wait(&_gpuWorkerThreadPausedMutex));
    }

    return true;
}

bool OsmAnd::MapRenderer::resumeGpuWorkerThread()
{
    if (!_gpuWorkerThread)
        return false;

    // Notify that worker should be paused
    _gpuWorkerIsPaused = false;

    // Since _gpuWorkerIsPaused == false, wake up GPU worker thread to allow it to resume
    {
        QMutexLocker scopedLocker(&_gpuWorkerThreadWakeupMutex);
        _gpuWorkerThreadWakeup.wakeAll();
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

bool OsmAnd::MapRenderer::adjustBitmapToConfiguration(
    const std::shared_ptr<const SkBitmap>& input,
    std::shared_ptr<const SkBitmap>& output,
    const AlphaChannelData alphaChannelData /*= AlphaChannelData::Undefined*/) const
{
    // Check if we're going to convert
    bool doConvert = false;
    const bool force16bit = (currentConfiguration->limitTextureColorDepthBy16bits && input->config() == SkBitmap::Config::kARGB_8888_Config);
    const bool canUsePaletteTextures = currentConfiguration->paletteTexturesAllowed && gpuAPI->isSupported_8bitPaletteRGBA8;
    const bool paletteTexture = (input->config() == SkBitmap::Config::kIndex8_Config);
    const bool unsupportedFormat =
        (canUsePaletteTextures ? !paletteTexture : paletteTexture) ||
        (input->config() != SkBitmap::Config::kARGB_8888_Config) ||
        (input->config() != SkBitmap::Config::kARGB_4444_Config) ||
        (input->config() != SkBitmap::Config::kRGB_565_Config);
    doConvert = doConvert || force16bit;
    doConvert = doConvert || unsupportedFormat;

    // Pass palette texture as-is
    if (paletteTexture && canUsePaletteTextures)
        return false;

    // Check if we need alpha
    auto convertedAlphaChannelData = alphaChannelData;
    if (doConvert && (convertedAlphaChannelData == AlphaChannelData::Undefined))
    {
        convertedAlphaChannelData = SkBitmap::ComputeIsOpaque(*input)
            ? AlphaChannelData::NotPresent
            : AlphaChannelData::Present;
    }

    // If we have limit of 16bits per pixel in bitmaps, convert to ARGB(4444) or RGB(565)
    if (force16bit)
    {
        auto convertedBitmap = new SkBitmap();

        const bool ok = input->copyTo(convertedBitmap,
            convertedAlphaChannelData == AlphaChannelData::Present
            ? SkColorType::kARGB_4444_SkColorType
            : SkColorType::kRGB_565_SkColorType);
        if (!ok)
        {
            assert(false);
            return false;
        }

        output.reset(convertedBitmap);
        return true;
    }

    // If we have any other unsupported format, convert to proper 16bit or 32bit
    if (unsupportedFormat)
    {
        auto convertedBitmap = new SkBitmap();

        const bool ok = input->copyTo(convertedBitmap,
            currentConfiguration->limitTextureColorDepthBy16bits
            ? (convertedAlphaChannelData == AlphaChannelData::Present ? SkColorType::kARGB_4444_SkColorType : SkColorType::kRGB_565_SkColorType)
            : SkColorType::kN32_SkColorType);
        if (!ok)
        {
            assert(false);
            return false;
        }

        output.reset(convertedBitmap);
        return true;
    }

    return false;
}

std::shared_ptr<const SkBitmap> OsmAnd::MapRenderer::adjustBitmapToConfiguration(
    const std::shared_ptr<const SkBitmap>& input,
    const AlphaChannelData alphaChannelData /*= AlphaChannelData::Undefined*/) const
{
    std::shared_ptr<const SkBitmap> output;
    if (adjustBitmapToConfiguration(input, output, alphaChannelData))
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
    auto& symbolReferencedResources = publishedMapSymbolsByGroup[symbol];
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

    const auto itSymbolReferencedResources = publishedMapSymbols.find(symbol);
    if (itSymbolReferencedResources == publishedMapSymbols.end())
    {
        assert(false);
        return;
    }
    auto& symbolReferencedResources = *itSymbolReferencedResources;

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
        publishedMapSymbols.erase(itSymbolReferencedResources);
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

OsmAnd::MapRendererState OsmAnd::MapRenderer::getState() const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto copy = _requestedState;
    return copy;
}

bool OsmAnd::MapRenderer::isFrameInvalidated() const
{
    return (_frameInvalidatesCounter.load() > 0);
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

void OsmAnd::MapRenderer::setRasterLayerProvider(const RasterMapLayerId layerId, const std::shared_ptr<IMapRasterBitmapTileProvider>& tileProvider, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.rasterLayerProviders[static_cast<int>(layerId)] != tileProvider);
    if (!update)
        return;

    _requestedState.rasterLayerProviders[static_cast<int>(layerId)] = tileProvider;

    notifyRequestedStateWasUpdated(MapRendererStateChange::RasterLayers_Providers);
}

void OsmAnd::MapRenderer::resetRasterLayerProvider(const RasterMapLayerId layerId, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || static_cast<bool>(_requestedState.rasterLayerProviders[static_cast<int>(layerId)]);
    if (!update)
        return;

    _requestedState.rasterLayerProviders[static_cast<int>(layerId)].reset();

    notifyRequestedStateWasUpdated(MapRendererStateChange::RasterLayers_Providers);
}

void OsmAnd::MapRenderer::setRasterLayerOpacity(const RasterMapLayerId layerId, const float opacity, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(0.0f, qMin(opacity, 1.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.rasterLayerOpacity[static_cast<int>(layerId)], clampedValue);
    if (!update)
        return;

    _requestedState.rasterLayerOpacity[static_cast<int>(layerId)] = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::RasterLayers_Opacity);
}

void OsmAnd::MapRenderer::setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& tileProvider, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.elevationDataProvider != tileProvider);
    if (!update)
        return;

    _requestedState.elevationDataProvider = tileProvider;

    _resources->invalidateResourcesOfType(MapRendererResourceType::ElevationDataTile);
    notifyRequestedStateWasUpdated(MapRendererStateChange::ElevationData_Provider);
}

void OsmAnd::MapRenderer::resetElevationDataProvider(bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || static_cast<bool>(_requestedState.elevationDataProvider);
    if (!update)
        return;

    _requestedState.elevationDataProvider.reset();

    _resources->invalidateResourcesOfType(MapRendererResourceType::ElevationDataTile);
    notifyRequestedStateWasUpdated(MapRendererStateChange::ElevationData_Provider);
}

void OsmAnd::MapRenderer::setElevationDataScaleFactor(const float factor, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.elevationDataScaleFactor, factor);
    if (!update)
        return;

    _requestedState.elevationDataScaleFactor = factor;

    notifyRequestedStateWasUpdated(MapRendererStateChange::ElevationData_ScaleFactor);
}

void OsmAnd::MapRenderer::addSymbolProvider(const std::shared_ptr<IMapDataProvider>& provider, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || !_requestedState.symbolProviders.contains(provider);
    if (!update)
        return;

    _requestedState.symbolProviders.insert(provider);

    notifyRequestedStateWasUpdated(MapRendererStateChange::Symbols_Providers);
}

void OsmAnd::MapRenderer::removeSymbolProvider(const std::shared_ptr<IMapDataProvider>& provider, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.symbolProviders.contains(provider);
    if (!update)
        return;

    _requestedState.symbolProviders.remove(provider);

    notifyRequestedStateWasUpdated(MapRendererStateChange::Symbols_Providers);
}

void OsmAnd::MapRenderer::removeAllSymbolProviders(bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || !_requestedState.symbolProviders.isEmpty();
    if (!update)
        return;

    _requestedState.symbolProviders.clear();

    notifyRequestedStateWasUpdated(MapRendererStateChange::Symbols_Providers);
}

void OsmAnd::MapRenderer::setWindowSize(const PointI& windowSize, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.windowSize != windowSize);
    if (!update)
        return;

    _requestedState.windowSize = windowSize;

    notifyRequestedStateWasUpdated(MapRendererStateChange::WindowSize);
}

void OsmAnd::MapRenderer::setViewport(const AreaI& viewport, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.viewport != viewport);
    if (!update)
        return;

    _requestedState.viewport = viewport;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Viewport);
}

void OsmAnd::MapRenderer::setFieldOfView(const float fieldOfView, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(fieldOfView, 90.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fieldOfView, clampedValue);
    if (!update)
        return;

    _requestedState.fieldOfView = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FieldOfView);
}

void OsmAnd::MapRenderer::setDistanceToFog(const float fogDistance, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), fogDistance);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogDistance, clampedValue);
    if (!update)
        return;

    _requestedState.fogDistance = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FogParameters);
}

void OsmAnd::MapRenderer::setFogOriginFactor(const float factor, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(factor, 1.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogOriginFactor, clampedValue);
    if (!update)
        return;

    _requestedState.fogOriginFactor = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FogParameters);
}

void OsmAnd::MapRenderer::setFogHeightOriginFactor(const float factor, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(factor, 1.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogHeightOriginFactor, clampedValue);
    if (!update)
        return;

    _requestedState.fogHeightOriginFactor = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FogParameters);
}

void OsmAnd::MapRenderer::setFogDensity(const float fogDensity, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), fogDensity);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogDensity, clampedValue);
    if (!update)
        return;

    _requestedState.fogDensity = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FogParameters);
}

void OsmAnd::MapRenderer::setFogColor(const FColorRGB& color, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.fogColor != color;
    if (!update)
        return;

    _requestedState.fogColor = color;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FogParameters);
}

void OsmAnd::MapRenderer::setSkyColor(const FColorRGB& color, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.skyColor != color;
    if (!update)
        return;

    _requestedState.skyColor = color;

    notifyRequestedStateWasUpdated(MapRendererStateChange::SkyColor);
}

void OsmAnd::MapRenderer::setAzimuth(const float azimuth, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    float normalizedAzimuth = Utilities::normalizedAngleDegrees(azimuth);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.azimuth, normalizedAzimuth);
    if (!update)
        return;

    _requestedState.azimuth = normalizedAzimuth;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Azimuth);
}

void OsmAnd::MapRenderer::setElevationAngle(const float elevationAngle, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(elevationAngle, 90.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.elevationAngle, clampedValue);
    if (!update)
        return;

    _requestedState.elevationAngle = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::ElevationAngle);
}

void OsmAnd::MapRenderer::setTarget(const PointI& target31_, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto target31 = Utilities::normalizeCoordinates(target31_, ZoomLevel31);
    bool update = forcedUpdate || (_requestedState.target31 != target31);
    if (!update)
        return;

    _requestedState.target31 = target31;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Target);
}

void OsmAnd::MapRenderer::setZoom(const float zoom, bool forcedUpdate /*= false*/)
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(getMinZoom(), qMin(zoom, getMaxZoom()));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.requestedZoom, clampedValue);
    if (!update)
        return;

    _requestedState.requestedZoom = clampedValue;
    _requestedState.zoomBase = static_cast<ZoomLevel>(qRound(clampedValue));
    assert(_requestedState.zoomBase >= MinZoomLevel && _requestedState.zoomBase <= MaxZoomLevel);
    _requestedState.zoomFraction = _requestedState.requestedZoom - _requestedState.zoomBase;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Zoom);
}

float OsmAnd::MapRenderer::getMinZoom() const
{
    return static_cast<float>(MinZoomLevel);
}

float OsmAnd::MapRenderer::getMaxZoom() const
{
    return static_cast<float>(MaxZoomLevel)+0.49999f;
}

float OsmAnd::MapRenderer::getRecommendedMinZoom(const ZoomRecommendationStrategy strategy) const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    float zoomLimit;
    if (strategy == IMapRenderer::ZoomRecommendationStrategy::NarrowestRange)
        zoomLimit = getMinZoom();
    else if (strategy == IMapRenderer::ZoomRecommendationStrategy::WidestRange)
        zoomLimit = getMaxZoom();

    bool atLeastOneValid = false;
    for (const auto& provider : constOf(_requestedState.rasterLayerProviders))
    {
        if (!provider)
            continue;

        atLeastOneValid = true;
        if (strategy == IMapRenderer::ZoomRecommendationStrategy::NarrowestRange)
            zoomLimit = qMax(zoomLimit, static_cast<float>(provider->getMinZoom()));
        else if (strategy == IMapRenderer::ZoomRecommendationStrategy::WidestRange)
            zoomLimit = qMin(zoomLimit, static_cast<float>(provider->getMinZoom()));
    }

    if (!atLeastOneValid)
        return getMinZoom();
    return (zoomLimit >= 0.5f) ? (zoomLimit - 0.5f) : zoomLimit;
}

float OsmAnd::MapRenderer::getRecommendedMaxZoom(const ZoomRecommendationStrategy strategy) const
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    float zoomLimit;
    if (strategy == IMapRenderer::ZoomRecommendationStrategy::NarrowestRange)
        zoomLimit = getMaxZoom();
    else if (strategy == IMapRenderer::ZoomRecommendationStrategy::WidestRange)
        zoomLimit = getMinZoom();

    bool atLeastOneValid = false;
    for (const auto& provider : constOf(_requestedState.rasterLayerProviders))
    {
        if (!provider)
            continue;

        atLeastOneValid = true;
        if (strategy == IMapRenderer::ZoomRecommendationStrategy::NarrowestRange)
            zoomLimit = qMin(zoomLimit, static_cast<float>(provider->getMaxZoom()));
        else if (strategy == IMapRenderer::ZoomRecommendationStrategy::WidestRange)
            zoomLimit = qMax(zoomLimit, static_cast<float>(provider->getMaxZoom()));
    }

    if (!atLeastOneValid)
        return getMaxZoom();
    return zoomLimit + 0.49999f;
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

void OsmAnd::MapRenderer::dumpResourcesInfo() const
{
    getResources().dumpResourcesInfo();
}
