#include "MapRenderer.h"

#include <cassert>
#include <algorithm>

#include <OsmAndCore/QtExtensions.h>
#include <QMutableHashIterator>

#include "MapRendererInternalState.h"
#include "MapRendererResources.h"
#include "IMapBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "IMapSymbolProvider.h"
#include "IRetainableResource.h"
#include "GPUAPI.h"
#include "Utilities.h"
#include "Logging.h"

#include <SkBitmap.h>

OsmAnd::MapRenderer::MapRenderer(GPUAPI* const gpuAPI_)
    : _currentConfigurationInvalidatedMask(0)
    , _requestedStateUpdatedMask(0)
    , _renderThreadId(nullptr)
    , _gpuWorkerThreadId(nullptr)
    , _gpuWorkerIsAlive(false)
    , currentConfiguration(_currentConfiguration)
    , currentState(_currentState)
    , gpuAPI(gpuAPI_)
{
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
    const bool elevationDataResolutionChanged = (_requestedConfiguration.heixelsPerTileSide != configuration_.heixelsPerTileSide);
    const bool texturesFilteringChanged = (_requestedConfiguration.texturesFilteringQuality != configuration_.texturesFilteringQuality);
    const bool paletteTexturesUsageChanged = (_requestedConfiguration.paletteTexturesAllowed != configuration_.paletteTexturesAllowed);

    bool invalidateRasterTextures = false;
    invalidateRasterTextures = invalidateRasterTextures || colorDepthForcingChanged;
    invalidateRasterTextures = invalidateRasterTextures || paletteTexturesUsageChanged;

    bool invalidateElevationData = false;
    invalidateElevationData = invalidateElevationData || elevationDataResolutionChanged;

    bool invalidateSymbols = false;
    invalidateSymbols = invalidateSymbols || colorDepthForcingChanged;

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
    if(elevationDataResolutionChanged)
        mask |= ConfigurationChange::ElevationDataResolution;
    if(texturesFilteringChanged)
        mask |= ConfigurationChange::TexturesFilteringMode;
    if(paletteTexturesUsageChanged)
        mask |= ConfigurationChange::PaletteTexturesUsage;

    if(invalidateRasterTextures)
        _resources->invalidateResourcesOfType(ResourceType::RasterMap);
    if(invalidateElevationData)
        _resources->invalidateResourcesOfType(ResourceType::ElevationData);
    if(invalidateSymbols)
        _resources->invalidateResourcesOfType(ResourceType::Symbols);
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
    _resources->updateBindings(_currentState, updatedMask);

    // Update internal state, that is derived from current state
    const auto internalState = getInternalStateRef();
    ok = updateInternalState(internalState, _currentState);

    // Postprocess internal state
    if(ok)
    {
        // Sort visible tiles by distance from target
        qSort(internalState->visibleTiles.begin(), internalState->visibleTiles.end(), [internalState](const TileId& l, const TileId& r) -> bool
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

void OsmAnd::MapRenderer::notifyRequestedStateWasUpdated(const MapRendererStateChange change)
{
    _requestedStateUpdatedMask |= 1u << static_cast<int>(change);

    // Since our current state is invalid, frame is also invalidated
    invalidateFrame();
}

void OsmAnd::MapRenderer::validateConfigurationChange(const ConfigurationChange& change)
{
    // Empty stub
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

    const auto tileSize31 = 1u << zoomDiff;
    const auto inTileOffset = state.target31 - targetTile31;
    internalState->targetInTileOffsetN.x = static_cast<float>(inTileOffset.x) / tileSize31;
    internalState->targetInTileOffsetN.y = static_cast<float>(inTileOffset.y) / tileSize31;

    return true;
}

void OsmAnd::MapRenderer::invalidateFrame()
{
    // Increment frame-invalidated counter by 1
    _frameInvalidatesCounter.fetchAndAddOrdered(1);

    // Request frame, if such callback is defined
    if(setupOptions.frameUpdateRequestCallback)
        setupOptions.frameUpdateRequestCallback();
}

void OsmAnd::MapRenderer::gpuWorkerThreadProcedure()
{
    assert(setupOptions.gpuWorkerThread.enabled);

    // Capture thread ID
    _gpuWorkerThreadId = QThread::currentThreadId();

    // Call prologue if such exists
    if(setupOptions.gpuWorkerThread.prologue)
        setupOptions.gpuWorkerThread.prologue();

    while(_gpuWorkerIsAlive)
    {
        // Wait until we're unblocked by host
        {
            QMutexLocker scopedLocker(&_gpuWorkerThreadWakeupMutex);
            REPEAT_UNTIL(_gpuWorkerThreadWakeup.wait(&_gpuWorkerThreadWakeupMutex));
        }
        if(!_gpuWorkerIsAlive)
            break;

        // In every layer we have, upload pending resources to GPU without limiting
        unsigned int resourcesUploaded = 0u;
        unsigned int resourcesUnloaded = 0u;
        _resources->syncResourcesInGPU(0, nullptr, &resourcesUploaded, &resourcesUnloaded);
        if(resourcesUploaded > 0 || resourcesUnloaded > 0)
            invalidateFrame();

        // Process GPU dispatcher
        _gpuThreadDispatcher.runAll();
    }

    // Call epilogue
    if(setupOptions.gpuWorkerThread.epilogue)
        setupOptions.gpuWorkerThread.epilogue();

    _gpuWorkerThreadId = nullptr;
}

bool OsmAnd::MapRenderer::initializeRendering()
{
    bool ok;

    ok = gpuAPI->initialize();
    if(!ok)
        return false;

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
    _gpuWorkerThreadId = nullptr;
    _currentConfigurationInvalidatedMask = std::numeric_limits<uint32_t>::max();
    _requestedStateUpdatedMask = std::numeric_limits<uint32_t>::max();
    
    // Create resources
    assert(!static_cast<bool>(_resources));
    _resources.reset(new MapRendererResources(this));
    
    return true;
}

bool OsmAnd::MapRenderer::doInitializeRendering()
{
    // Create GPU worker thread
    if(setupOptions.gpuWorkerThread.enabled)
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
    if(_gpuWorkerThread)
    {
        _gpuWorkerIsAlive = true;
        _gpuWorkerThread->start();
    }

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
        // Capture configuration
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
    _resources->validateResources();

    return true;
}

bool OsmAnd::MapRenderer::doPrepareFrame()
{
    return true;
}

bool OsmAnd::MapRenderer::postPrepareFrame()
{
    // Notify resources manager about new active zone
    _resources->updateActiveZone(_uniqueTiles, _currentState.zoomBase);

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
    
    // Capture how many "frame-invalidates" are going to be processed
    _frameInvalidatesToBeProcessed = _frameInvalidatesCounter.fetchAndAddOrdered(0);

    return true;
}

bool OsmAnd::MapRenderer::postRenderFrame()
{
    // Decrement "frame-invalidates" counter by amount of processed "frame-invalidates"
    _frameInvalidatesCounter.fetchAndAddOrdered(-_frameInvalidatesToBeProcessed);

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
    // If GPU worker thread is not enabled, upload resource to GPU from render thread.
    // To reduce FPS drop, upload not more than 1 resource per frame.
    if(!_gpuWorkerThread)
    {
        bool moreUploadThanLimitAvailable = false;
        unsigned int resourcesUploaded = 0u;
        unsigned int resourcesUnloaded = 0u;
        _resources->syncResourcesInGPU(1u, &moreUploadThanLimitAvailable, &resourcesUploaded, &resourcesUnloaded);
        
        // If any resource was uploaded or there is more resources to uploaded, invalidate frame
        // to use that resource
        if(resourcesUploaded > 0 || moreUploadThanLimitAvailable || resourcesUnloaded > 0)
            invalidateFrame();

        // Process GPU thread dispatcher
        _gpuThreadDispatcher.runAll();
    }

    // Process render thread dispatcher
    _renderThreadDispatcher.runAll();

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
    ok = gpuAPI->release();
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
    // Stop GPU worker if it exists
    if(_gpuWorkerThread)
    {
        // Deactivate worker thread
        _gpuWorkerIsAlive = false;

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

    // Release resources
    _resources.reset();

    _isRenderingInitialized = false;

    return true;
}

const OsmAnd::MapRendererResources& OsmAnd::MapRenderer::getResources() const
{
    return *_resources.get();
}

void OsmAnd::MapRenderer::onValidateResourcesOfType(const MapRendererResources::ResourceType type)
{
    // Empty stub
}

void OsmAnd::MapRenderer::requestResourcesUpload()
{
    if(_gpuWorkerThread)
    {
        QMutexLocker scopedLocker(&_gpuWorkerThreadWakeupMutex);
        _gpuWorkerThreadWakeup.wakeAll();
    }
    else
        invalidateFrame();
}

bool OsmAnd::MapRenderer::convertBitmap(const std::shared_ptr<const SkBitmap>& input, std::shared_ptr<const SkBitmap>& output, const AlphaChannelData alphaChannelData /*= AlphaChannelData::Undefined*/) const
{
    // Check if we're going to convert
    bool doConvert = false;
    const bool force16bit = (currentConfiguration.limitTextureColorDepthBy16bits && input->getConfig() == SkBitmap::Config::kARGB_8888_Config);
    const bool canUsePaletteTextures = currentConfiguration.paletteTexturesAllowed && gpuAPI->isSupported_8bitPaletteRGBA8;
    const bool paletteTexture = (input->getConfig() == SkBitmap::Config::kIndex8_Config);
    const bool unsupportedFormat =
        (canUsePaletteTextures ? !paletteTexture : paletteTexture) ||
        (input->getConfig() != SkBitmap::Config::kARGB_8888_Config) ||
        (input->getConfig() != SkBitmap::Config::kARGB_4444_Config) ||
        (input->getConfig() != SkBitmap::Config::kRGB_565_Config);
    doConvert = doConvert || force16bit;
    doConvert = doConvert || unsupportedFormat;

    // Pass palette texture as-is
    if(paletteTexture && canUsePaletteTextures)
        return false;

    // Check if we need alpha
    auto convertedAlphaChannelData = alphaChannelData;
    if(doConvert && (convertedAlphaChannelData == AlphaChannelData::Undefined))
    {
        convertedAlphaChannelData = SkBitmap::ComputeIsOpaque(*input)
            ? AlphaChannelData::NotPresent
            : AlphaChannelData::Present;
    }

    // If we have limit of 16bits per pixel in bitmaps, convert to ARGB(4444) or RGB(565)
    if(force16bit)
    {
        auto convertedBitmap = new SkBitmap();

        input->deepCopyTo(convertedBitmap,
            convertedAlphaChannelData == AlphaChannelData::Present
            ? SkBitmap::Config::kARGB_4444_Config
            : SkBitmap::Config::kRGB_565_Config);

        output.reset(convertedBitmap);
        return true;
    }

    // If we have any other unsupported format, convert to proper 16bit or 32bit
    if(unsupportedFormat)
    {
        auto convertedBitmap = new SkBitmap();

        input->deepCopyTo(convertedBitmap,
            currentConfiguration.limitTextureColorDepthBy16bits
            ? (convertedAlphaChannelData == AlphaChannelData::Present ? SkBitmap::Config::kARGB_4444_Config : SkBitmap::Config::kRGB_565_Config)
            : SkBitmap::kARGB_8888_Config);

        output.reset(convertedBitmap);
        return true;
    }

    return false;
}

bool OsmAnd::MapRenderer::convertMapTile(std::shared_ptr<const MapTile>& mapTile) const
{
    return convertMapTile(mapTile, mapTile);
}

bool OsmAnd::MapRenderer::convertMapTile(const std::shared_ptr<const MapTile>& input_, std::shared_ptr<const MapTile>& output) const
{
    if(input_->dataType == MapTileDataType::Bitmap)
    {
        const auto input = std::static_pointer_cast<const MapBitmapTile>(input_);

        std::shared_ptr<const SkBitmap> convertedBitmap;
        const bool wasConverted = convertBitmap(input->bitmap, convertedBitmap, input->alphaChannelData);
        if(!wasConverted)
            return false;

        output.reset(new MapBitmapTile(convertedBitmap, input->alphaChannelData));
        return true;
    }

    return false;
}

bool OsmAnd::MapRenderer::convertMapSymbol(std::shared_ptr<const MapSymbol>& mapSymbol) const
{
    return convertMapSymbol(mapSymbol, mapSymbol);
}

bool OsmAnd::MapRenderer::convertMapSymbol(const std::shared_ptr<const MapSymbol>& input, std::shared_ptr<const MapSymbol>& output) const
{
    std::shared_ptr<const SkBitmap> convertedBitmap;
    const bool wasConverted = convertBitmap(input->bitmap, convertedBitmap, AlphaChannelData::Present);
    if(!wasConverted)
        return false;

    output.reset(input->cloneWithReplacedBitmap(convertedBitmap));
    return true;
}

bool OsmAnd::MapRenderer::isFrameInvalidated() const
{
    return (_frameInvalidatesCounter.fetchAndAddOrdered(0) > 0);
}

OsmAnd::Concurrent::Dispatcher& OsmAnd::MapRenderer::getRenderThreadDispatcher()
{
    return _renderThreadDispatcher;
}

OsmAnd::Concurrent::Dispatcher& OsmAnd::MapRenderer::getGpuThreadDispatcher()
{
    return _gpuThreadDispatcher;
}

unsigned int OsmAnd::MapRenderer::getVisibleTilesCount() const
{
    QReadLocker scopedLocker(&_internalStateLock);

    return getInternalStateRef()->visibleTiles.size();
}

unsigned int OsmAnd::MapRenderer::getSymbolsCount() const
{
    return getResources().getMapSymbolsCount();
}

void OsmAnd::MapRenderer::setRasterLayerProvider( const RasterMapLayerId layerId, const std::shared_ptr<IMapBitmapTileProvider>& tileProvider, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.rasterLayerProviders[static_cast<int>(layerId)] != tileProvider);
    if(!update)
        return;

    _requestedState.rasterLayerProviders[static_cast<int>(layerId)] = tileProvider;

    notifyRequestedStateWasUpdated(MapRendererStateChange::RasterLayers_Providers);
}

void OsmAnd::MapRenderer::resetRasterLayerProvider( const RasterMapLayerId layerId, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || static_cast<bool>(_requestedState.rasterLayerProviders[static_cast<int>(layerId)]);
    if(!update)
        return;

    _requestedState.rasterLayerProviders[static_cast<int>(layerId)].reset();

    notifyRequestedStateWasUpdated(MapRendererStateChange::RasterLayers_Providers);
}

void OsmAnd::MapRenderer::setRasterLayerOpacity( const RasterMapLayerId layerId, const float opacity, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(0.0f, qMin(opacity, 1.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.rasterLayerOpacity[static_cast<int>(layerId)], clampedValue);
    if(!update)
        return;

    _requestedState.rasterLayerOpacity[static_cast<int>(layerId)] = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::RasterLayers_Opacity);
}

void OsmAnd::MapRenderer::setElevationDataProvider( const std::shared_ptr<IMapElevationDataProvider>& tileProvider, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.elevationDataProvider != tileProvider);
    if(!update)
        return;

    _requestedState.elevationDataProvider = tileProvider;

    _resources->invalidateResourcesOfType(ResourceType::ElevationData);
    notifyRequestedStateWasUpdated(MapRendererStateChange::ElevationData_Provider);
}

void OsmAnd::MapRenderer::resetElevationDataProvider( bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || static_cast<bool>(_requestedState.elevationDataProvider);
    if(!update)
        return;

    _requestedState.elevationDataProvider.reset();

    _resources->invalidateResourcesOfType(ResourceType::ElevationData);
    notifyRequestedStateWasUpdated(MapRendererStateChange::ElevationData_Provider);
}

void OsmAnd::MapRenderer::setElevationDataScaleFactor( const float factor, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.elevationDataScaleFactor, factor);
    if(!update)
        return;

    _requestedState.elevationDataScaleFactor = factor;

    notifyRequestedStateWasUpdated(MapRendererStateChange::ElevationData_ScaleFactor);
}

void OsmAnd::MapRenderer::addSymbolProvider( const std::shared_ptr<IMapSymbolProvider>& provider, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || !_requestedState.symbolProviders.contains(provider);
    if(!update)
        return;

    _requestedState.symbolProviders.insert(provider);

    notifyRequestedStateWasUpdated(MapRendererStateChange::Symbols_Providers);
}

void OsmAnd::MapRenderer::removeSymbolProvider( const std::shared_ptr<IMapSymbolProvider>& provider, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.symbolProviders.contains(provider);
    if(!update)
        return;

    _requestedState.symbolProviders.remove(provider);

    notifyRequestedStateWasUpdated(MapRendererStateChange::Symbols_Providers);
}

void OsmAnd::MapRenderer::removeAllSymbolProviders( bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || !_requestedState.symbolProviders.isEmpty();
    if(!update)
        return;

    _requestedState.symbolProviders.clear();

    notifyRequestedStateWasUpdated(MapRendererStateChange::Symbols_Providers);
}

void OsmAnd::MapRenderer::setWindowSize( const PointI& windowSize, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.windowSize != windowSize);
    if(!update)
        return;

    _requestedState.windowSize = windowSize;

    notifyRequestedStateWasUpdated(MapRendererStateChange::WindowSize);
}

void OsmAnd::MapRenderer::setViewport( const AreaI& viewport, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || (_requestedState.viewport != viewport);
    if(!update)
        return;

    _requestedState.viewport = viewport;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Viewport);
}

void OsmAnd::MapRenderer::setFieldOfView( const float fieldOfView, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(fieldOfView, 90.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fieldOfView, clampedValue);
    if(!update)
        return;

    _requestedState.fieldOfView = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FieldOfView);
}

void OsmAnd::MapRenderer::setDistanceToFog( const float fogDistance, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), fogDistance);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogDistance, clampedValue);
    if(!update)
        return;

    _requestedState.fogDistance = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FogParameters);
}

void OsmAnd::MapRenderer::setFogOriginFactor( const float factor, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(factor, 1.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogOriginFactor, clampedValue);
    if(!update)
        return;

    _requestedState.fogOriginFactor = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FogParameters);
}

void OsmAnd::MapRenderer::setFogHeightOriginFactor( const float factor, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(factor, 1.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogHeightOriginFactor, clampedValue);
    if(!update)
        return;

    _requestedState.fogHeightOriginFactor = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FogParameters);
}

void OsmAnd::MapRenderer::setFogDensity( const float fogDensity, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), fogDensity);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogDensity, clampedValue);
    if(!update)
        return;

    _requestedState.fogDensity = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FogParameters);
}

void OsmAnd::MapRenderer::setFogColor( const FColorRGB& color, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.fogColor != color;
    if(!update)
        return;

    _requestedState.fogColor = color;

    notifyRequestedStateWasUpdated(MapRendererStateChange::FogParameters);
}

void OsmAnd::MapRenderer::setSkyColor( const FColorRGB& color, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || _requestedState.skyColor != color;
    if(!update)
        return;

    _requestedState.skyColor = color;

    notifyRequestedStateWasUpdated(MapRendererStateChange::SkyColor);
}

void OsmAnd::MapRenderer::setAzimuth( const float azimuth, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    float normalizedAzimuth = Utilities::normalizedAngleDegrees(azimuth);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.azimuth, normalizedAzimuth);
    if(!update)
        return;

    _requestedState.azimuth = normalizedAzimuth;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Azimuth);
}

void OsmAnd::MapRenderer::setElevationAngle( const float elevationAngle, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(elevationAngle, 90.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.elevationAngle, clampedValue);
    if(!update)
        return;

    _requestedState.elevationAngle = clampedValue;

    notifyRequestedStateWasUpdated(MapRendererStateChange::ElevationAngle);
}

void OsmAnd::MapRenderer::setTarget( const PointI& target31_, bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    const auto target31 = Utilities::normalizeCoordinates(target31_, ZoomLevel31);
    bool update = forcedUpdate || (_requestedState.target31 != target31);
    if(!update)
        return;

    _requestedState.target31 = target31;

    notifyRequestedStateWasUpdated(MapRendererStateChange::Target);
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

    notifyRequestedStateWasUpdated(MapRendererStateChange::Zoom);
}
