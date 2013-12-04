#include "MapRenderer.h"

#include <cassert>
#include <algorithm>

#include <OsmAndCore/QtExtensions.h>
#include <QMutableHashIterator>

#include "MapRendererResources.h"
#include "IMapBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "IMapSymbolProvider.h"
#include "IRetainedMapTile.h"
#include "RenderAPI.h"
#include "Utilities.h"
#include "Logging.h"

#include <SkBitmap.h>

OsmAnd::MapRenderer::MapRenderer()
    : _currentConfigurationInvalidatedMask(0)
    , _requestedStateUpdatedMask(0)
    , _renderThreadId(nullptr)
    , _workerThreadId(nullptr)
    , currentConfiguration(_currentConfiguration)
    , currentState(_currentState)
    , renderAPI(_renderAPI)
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
    if(atlasTexturesUsageChanged)
        mask |= ConfigurationChange::AtlasTexturesUsage;
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

        // In every layer we have, upload pending resources to GPU without limiting
        const auto resourcesUploaded = _resources->uploadResources(0, nullptr);
        if(resourcesUploaded > 0)
            invalidateFrame();
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
    
    // Create resources
    assert(!static_cast<bool>(_resources));
    _resources.reset(new MapRendererResources(this));
    
    return true;
}

bool OsmAnd::MapRenderer::doInitializeRendering()
{
    // Create background worker if enabled
    if(setupOptions.backgroundWorker.enabled)
        _backgroundWorker.reset(new Concurrent::Thread(std::bind(&MapRenderer::backgroundWorkerProcedure, this)));

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
    // Before requesting missing tiled resources, clean up cache to free some space
    _resources->cleanupJunkResources(_uniqueTiles, _currentState.zoomBase);

    // In the end of rendering processing, request tiled resources that are neither
    // present in requested list, nor in pending, nor in uploaded
    _resources->requestNeededResources(_uniqueTiles, _currentState.zoomBase);

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
    {
        bool moreThanLimitAvailable = false;
        const auto resourcesUploaded = _resources->uploadResources(1, &moreThanLimitAvailable);

        // If any resource was uploaded or there is more resources to uploaded, invalidate frame
        if(resourcesUploaded > 0 || moreThanLimitAvailable)
            invalidateFrame();
    }

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

    // Release resources
    _resources.reset();

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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void OsmAnd::MapRenderer::requestResourcesUpload()
{
    if(setupOptions.backgroundWorker.enabled)
        _backgroundWorkerWakeup.wakeAll();
    else
        invalidateFrame();
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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

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

    notifyRequestedStateWasUpdated(MapRendererStateChange::ElevationData_Provider);
}

void OsmAnd::MapRenderer::resetElevationDataProvider( bool forcedUpdate /*= false*/ )
{
    QMutexLocker scopedLocker(&_requestedStateMutex);

    bool update = forcedUpdate || static_cast<bool>(_requestedState.elevationDataProvider);
    if(!update)
        return;

    _requestedState.elevationDataProvider.reset();

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

    bool update = forcedUpdate || _requestedState.symbolProviders.isEmpty();
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
