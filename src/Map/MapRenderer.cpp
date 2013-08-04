#include "MapRenderer.h"

#include <assert.h>

#include <SkBitmap.h>

#include "IMapBitmapTileProvider.h"
#include "RenderAPI.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::MapRenderer::MapRenderer()
    : currentConfiguration(_currentConfiguration)
    , _currentConfigurationInvalidatedMask(0xFFFFFFFF)
    , currentState(_currentState)
    , _currentStateInvalidated(true)
    , _invalidatedLayers(0)
    , _renderThreadId(nullptr)
    , _workerThreadId(nullptr)
    , layers(_layers)
    , renderAPI(_renderAPI)
{
    // Create all layers
    for(auto layerId = 0u; layerId < MapTileLayerIdsCount; layerId++)
        _layers[layerId].reset(new MapRendererTileLayer(static_cast<MapTileLayerId>(layerId)));

    // Fill-up default state
    for(auto layerId = 0u; layerId < MapTileLayerIdsCount; layerId++)
        setTileLayerOpacity(static_cast<MapTileLayerId>(layerId), 1.0f);
    setFieldOfView(16.5f, true);
    setDistanceToFog(400.0f, true);
    setFogOriginFactor(0.36f, true);
    setFogHeightOriginFactor(0.05f, true);
    setFogDensity(1.9f, true);
    setFogColor(1.0f, 0.0f, 0.0f, true);
    setSkyColor(140.0f / 255.0f, 190.0f / 255.0f, 214.0f / 255.0f, true);
    setAzimuth(0.0f, true);
    setElevationAngle(45.0f, true);
    const auto centerIndex = 1u << (ZoomLevel::MaxZoomLevel - 1);
    setTarget(PointI(centerIndex, centerIndex), true);
    setZoom(0, true);
    setHeightScaleFactor(1.0f, true);
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
    {
        for(int layerId = MapTileLayerId::RasterMap; layerId < MapTileLayerIdsCount; layerId++)
            invalidateLayer(static_cast<MapTileLayerId>(layerId));
    }
    if(invalidateElevationData)
    {
        invalidateLayer(MapTileLayerId::ElevationData);
    }
    invalidateCurrentConfiguration(mask);
}

void OsmAnd::MapRenderer::invalidateCurrentConfiguration(const uint32_t& changesMask)
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

    return true;
}

bool OsmAnd::MapRenderer::doInitializeRendering()
{
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
    if(!_isRenderingInitialized)
        return false;

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

    // If we have current state invalidated, we need to update it
    // and invalidate frame
    if(_currentStateInvalidated)
    {
        {
            QReadLocker scopedLocker(&_stateLock);

            _currentState = _requestedState;
        }

        bool ok = updateCurrentState();
        if(ok)
            _currentStateInvalidated = false;

        invalidateFrame();

        // If state is still invalidated, abort processing
        if(_currentStateInvalidated)
            return false;
    }

    // If we have invalidated layers, purge them
    if(_invalidatedLayers)
    {
        QReadLocker scopedLocker(&_invalidatedLayersLock);

        for(int layerId = 0; layerId < MapTileLayerIdsCount; layerId++)
        {
            if((_invalidatedLayers & (1 << layerId)) == 0)
                continue;

            validateLayer(static_cast<MapTileLayerId>(layerId));

            _invalidatedLayers &= ~(1 << layerId);
        }
    }

    // Sort visible tiles by distance from target
    qSort(_visibleTiles.begin(), _visibleTiles.end(), [this](const TileId& l, const TileId& r) -> bool
    {
        const auto lx = l.x - _targetTileId.x;
        const auto ly = l.y - _targetTileId.y;

        const auto rx = r.x - _targetTileId.x;
        const auto ry = r.y - _targetTileId.y;

        return (lx*lx + ly*ly) > (rx*rx + ry*ry);
    });

    // Get set of tiles that are unique: visible tiles may contain same tiles, but wrapped
    _uniqueTiles.clear();
    for(auto itTileId = _visibleTiles.begin(); itTileId != _visibleTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;
        _uniqueTiles.insert(Utilities::normalizeTileId(tileId, _currentState.zoomBase));
    }

    //TODO: Keep cache fresh and throw away outdated tiles

    return true;
}

bool OsmAnd::MapRenderer::doProcessRendering()
{
    return true;
}

bool OsmAnd::MapRenderer::updateCurrentState()
{
    // Get target tile id
    _targetTileId.x = _currentState.target31.x >> (ZoomLevel::MaxZoomLevel - _currentState.zoomBase);
    _targetTileId.y = _currentState.target31.y >> (ZoomLevel::MaxZoomLevel - _currentState.zoomBase);

    // Compute in-tile offset
    if(_currentState.zoomBase == ZoomLevel::MaxZoomLevel)
    {
        _targetInTileOffsetN.x = 0;
        _targetInTileOffsetN.y = 0;
    }
    else
    {
        PointI targetTile31;
        targetTile31.x = _targetTileId.x << (ZoomLevel::MaxZoomLevel - _currentState.zoomBase);
        targetTile31.y = _targetTileId.y << (ZoomLevel::MaxZoomLevel - _currentState.zoomBase);

        auto tileWidth31 = (1u << (ZoomLevel::MaxZoomLevel - _currentState.zoomBase)) - 1;
        _targetInTileOffsetN.x = static_cast<float>(_currentState.target31.x - targetTile31.x) / tileWidth31;
        _targetInTileOffsetN.y = static_cast<float>(_currentState.target31.y - targetTile31.y) / tileWidth31;
    }

    return true;
}

bool OsmAnd::MapRenderer::postProcessRendering()
{
    // In the end of rendering processing, request tiles that are neither present in
    // requested list, nor in pending, nor in uploaded
    requestMissingTiles();

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

bool OsmAnd::MapRenderer::postprocessRendering()
{
    assert(_renderThreadId == QThread::currentThreadId());

    bool ok;

    ok = prePostprocessRendering();
    if(!ok)
        return false;

    ok = doPostprocessRendering();
    if(!ok)
        return false;

    ok = postPostprocessRendering();
    if(!ok)
        return false;

    return true;
}

bool OsmAnd::MapRenderer::prePostprocessRendering()
{
    return true;
}

bool OsmAnd::MapRenderer::doPostprocessRendering()
{
    // If background worked is was not enabled, upload tiles to GPU in render thread
    // To reduce FPS drop, upload not more than 1 tile per frame, and do that before end of the frame
    // to avoid forcing driver to upload data on current frame presentation.
    if(!setupOptions.backgroundWorker.enabled)
    {
        QList< std::shared_ptr<MapRendererTileLayer::TileEntry> > tileEntries;

        for(int layerId = 0; layerId < MapTileLayerIdsCount; layerId++)
        {
            // Obtain as maximum 1 
            tileEntries.clear();
            _layers[layerId]->obtainTileEntries(tileEntries, 1, MapRendererTileLayer::TileEntry::Ready);

            if(tileEntries.isEmpty())
                continue;
            const auto& tileEntry = tileEntries.first();

            {
                QWriteLocker scopedLock(&tileEntry->stateLock);
                if(tileEntry->state != MapRendererTileLayer::TileEntry::Ready)
                    continue;

                const auto& preparedSourceData = prepareTileForUploadingToGPU(static_cast<MapTileLayerId>(layerId), tileEntry->sourceData);
                const auto tilesPerAtlasTextureLimit = getTilesPerAtlasTextureLimit(static_cast<MapTileLayerId>(layerId), tileEntry->sourceData);
                bool ok = renderAPI->uploadTileToGPU(tileEntry->tileId, tileEntry->zoom, preparedSourceData, tilesPerAtlasTextureLimit, tileEntry->_resourceInGPU);
                if(!ok)
                {
                    LogPrintf(LogSeverityLevel::Error, "Failed to upload tile %dx%d@%d[%d] to GPU", tileEntry->tileId.x, tileEntry->tileId.y, tileEntry->zoom, tileEntry->layerId);
                    continue;
                }

                tileEntry->_sourceData.reset();
                tileEntry->_state = MapRendererTileLayer::TileEntry::Uploaded;
            }

            // Schedule one more render pass to upload more pending
            // or we've just uploaded a tile and need refresh
            invalidateFrame();

            break;
        }
    }

    return true;
}

bool OsmAnd::MapRenderer::postPostprocessRendering()
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

    // Remove all tiles from all layers
    for(auto itLayer = _layers.begin(); itLayer != _layers.end(); ++itLayer)
    {
        const auto& layer = *itLayer;

        // Collect all tiles that are uploaded to GPU and release them
        QList< std::shared_ptr<MapRendererTileLayer::TileEntry> > tilesInGPU;
        layer->obtainTileEntries(tilesInGPU, 0, MapRendererTileLayer::TileEntry::State::Uploaded);
        for(auto itUploadedTileEntry = tilesInGPU.begin(); itUploadedTileEntry != tilesInGPU.end(); ++itUploadedTileEntry)
        {
            const auto& tileEntry = *itUploadedTileEntry;

            QWriteLocker scopedLock(&tileEntry->stateLock);
            if(tileEntry->state != MapRendererTileLayer::TileEntry::Uploaded)
                continue;

            // This should be last reference, so assert on that
            assert(tileEntry->_resourceInGPU.use_count() == 1);
            tileEntry->_resourceInGPU.reset();

            tileEntry->_state = MapRendererTileLayer::TileEntry::Unloaded;
        }

        layer->removeAllEntries();
    }

    return true;
}

void OsmAnd::MapRenderer::invalidateCurrentState()
{
    _currentStateInvalidated = true;

    // Since our current state is invalid, frame is also invalidated
    invalidateFrame();
}

void OsmAnd::MapRenderer::invalidateFrame()
{
    _frameInvalidated = true;

    // Request frame, if such callback is defined
    if(setupOptions.frameRequestCallback)
        setupOptions.frameRequestCallback();
}

void OsmAnd::MapRenderer::requestUploadDataToGPU()
{
    if(setupOptions.backgroundWorker.enabled)
        _backgroundWorkerWakeup.wakeAll();
    else
        invalidateFrame();
}

void OsmAnd::MapRenderer::invalidateLayer( const MapTileLayerId& layerId )
{
    QWriteLocker scopedLocker(&_invalidatedLayersLock);

    _invalidatedLayers |= 1 << layerId;
}

void OsmAnd::MapRenderer::validateLayer( const MapTileLayerId& layerId )
{
    const auto& layer = layers[layerId];

    // Collect all tiles that are uploaded to GPU and release them
    QList< std::shared_ptr<MapRendererTileLayer::TileEntry> > tilesInGPU;
    layer->obtainTileEntries(tilesInGPU, 0, MapRendererTileLayer::TileEntry::State::Uploaded);
    for(auto itUploadedTileEntry = tilesInGPU.begin(); itUploadedTileEntry != tilesInGPU.end(); ++itUploadedTileEntry)
    {
        const auto& tileEntry = *itUploadedTileEntry;

        QWriteLocker scopedLock(&tileEntry->stateLock);
        if(tileEntry->state != MapRendererTileLayer::TileEntry::Uploaded)
            continue;

        // This should be last reference, so assert on that
        assert(tileEntry->_resourceInGPU.use_count() == 1);
        tileEntry->_resourceInGPU.reset();

        tileEntry->_state = MapRendererTileLayer::TileEntry::Unloaded;
    }

    layer->removeAllEntries();
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
        QList< std::shared_ptr<MapRendererTileLayer::TileEntry> > tileEntries;
        for(int layerId = 0; layerId < MapTileLayerIdsCount; layerId++)
        {
            // Obtain all tiles that are ready
            tileEntries.clear();
            _layers[layerId]->obtainTileEntries(tileEntries, 0, MapRendererTileLayer::TileEntry::Ready);

            for(auto itTileEntry = tileEntries.begin(); itTileEntry != tileEntries.end(); ++itTileEntry)
            {
                {
                    const auto& tileEntry = *itTileEntry;
                    QWriteLocker scopedLock(&tileEntry->stateLock);
                    if(tileEntry->state != MapRendererTileLayer::TileEntry::Ready)
                        continue;

                    const auto& preparedSourceData = prepareTileForUploadingToGPU(static_cast<MapTileLayerId>(layerId), tileEntry->sourceData);
                    const auto tilesPerAtlasTextureLimit = getTilesPerAtlasTextureLimit(static_cast<MapTileLayerId>(layerId), tileEntry->sourceData);
                    bool ok = renderAPI->uploadTileToGPU(tileEntry->tileId, tileEntry->zoom, preparedSourceData, tilesPerAtlasTextureLimit, tileEntry->_resourceInGPU);
                    if(!ok)
                    {
                        LogPrintf(LogSeverityLevel::Error, "Failed to upload tile %dx%d@%d[%d] to GPU", tileEntry->tileId.x, tileEntry->tileId.y, tileEntry->zoom, tileEntry->layerId);
                        continue;
                    }

                    tileEntry->_sourceData.reset();
                    tileEntry->_state = MapRendererTileLayer::TileEntry::Uploaded;
                }

                // We have additional tile to show
                invalidateFrame();
            }

            break;
        }
    }

    // Call epilogue
    if(setupOptions.backgroundWorker.epilogue)
        setupOptions.backgroundWorker.epilogue();

    _workerThreadId = nullptr;
}

void OsmAnd::MapRenderer::requestMissingTiles()
{
    uint32_t requestedProvidersMask = 0;
    bool wasImmediateDataObtained = true;

    for(auto itTileId = _uniqueTiles.begin(); itTileId != _uniqueTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        for(int layerId = 0; layerId < MapTileLayerIdsCount; layerId++)
        {
            const auto& provider = _currentState.tileProviders[layerId];

            // Skip layers that do not have tile providers
            if(!provider)
                continue;

            const auto& tileEntry = _layers[layerId]->obtainTileEntry(tileId, _currentState.zoomBase, true);
            {
                QWriteLocker scopedLock(&tileEntry->stateLock);

                if(tileEntry->state != MapRendererTileLayer::TileEntry::Unknown)
                    continue;

                const auto callback = std::bind(&MapRenderer::processRequestedTile,
                    this,
                    static_cast<MapTileLayerId>(layerId),
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    std::placeholders::_4);

                // Try to obtain tile from provider immediately. Immediately means that data is available in-memory
                std::shared_ptr<IMapTileProvider::Tile> tile;
                bool availableImmediately = provider->obtainTileImmediate(tileId, _currentState.zoomBase, tile);
                if(availableImmediately)
                {
                    tileEntry->_sourceData = tile;
                    tileEntry->_state = tile ? MapRendererTileLayer::TileEntry::Ready : MapRendererTileLayer::TileEntry::Unavailable;

                    wasImmediateDataObtained = true;
                    continue;
                }

                // If tile was not available immediately, request it
                provider->obtainTileDeffered(tileId, _currentState.zoomBase, callback);
                tileEntry->_state = MapRendererTileLayer::TileEntry::Requested;

                requestedProvidersMask |= (1 << layerId);
            }
        }
    }

    if(wasImmediateDataObtained)
        requestUploadDataToGPU();

    //TODO: sort requests in all requestedProvidersMask so that closest tiles would be downloaded first
}

void OsmAnd::MapRenderer::processRequestedTile( const MapTileLayerId& layerId, const TileId& tileId, const ZoomLevel& zoom, const std::shared_ptr<IMapTileProvider::Tile>& tile, bool success )
{
    const auto& tileEntry = _layers[layerId]->obtainTileEntry(tileId, zoom);
    {
        QWriteLocker scopedLock(&tileEntry->stateLock);

        assert(tileEntry->state == MapRendererTileLayer::TileEntry::Requested);

        tileEntry->_sourceData = tile;
        tileEntry->_state = tile ? MapRendererTileLayer::TileEntry::Ready : MapRendererTileLayer::TileEntry::Unavailable;
    }

    // We have data to upload, so request that
    requestUploadDataToGPU();
}

void OsmAnd::MapRenderer::setTileProvider( const MapTileLayerId& layerId, const std::shared_ptr<IMapTileProvider>& tileProvider, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    bool update = forcedUpdate || (_requestedState.tileProviders[layerId] != tileProvider);
    if(!update)
        return;

    _requestedState.tileProviders[layerId] = tileProvider;

    invalidateLayer(layerId);
    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setTileLayerOpacity( const MapTileLayerId& layerId, const float& opacity, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    const auto clampedValue = qMax(0.0f, qMin(opacity, 1.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.tileLayerOpacity[layerId], clampedValue);
    if(!update)
        return;

    _requestedState.tileLayerOpacity[layerId] = clampedValue;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setWindowSize( const PointI& windowSize, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    bool update = forcedUpdate || (_requestedState.windowSize != windowSize);
    if(!update)
        return;

    _requestedState.windowSize = windowSize;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setViewport( const AreaI& viewport, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    bool update = forcedUpdate || (_requestedState.viewport != viewport);
    if(!update)
        return;

    _requestedState.viewport = viewport;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setFieldOfView( const float& fieldOfView, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(fieldOfView, 90.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fieldOfView, clampedValue);
    if(!update)
        return;

    _requestedState.fieldOfView = clampedValue;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setDistanceToFog( const float& fogDistance, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), fogDistance);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogDistance, clampedValue);
    if(!update)
        return;

    _requestedState.fogDistance = clampedValue;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setFogOriginFactor( const float& factor, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(factor, 1.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogOriginFactor, clampedValue);
    if(!update)
        return;

    _requestedState.fogOriginFactor = clampedValue;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setFogHeightOriginFactor( const float& factor, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(factor, 1.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogHeightOriginFactor, clampedValue);
    if(!update)
        return;

    _requestedState.fogHeightOriginFactor = clampedValue;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setFogDensity( const float& fogDensity, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), fogDensity);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogDensity, clampedValue);
    if(!update)
        return;

    _requestedState.fogDensity = clampedValue;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setFogColor( const float& r, const float& g, const float& b, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.fogColor[0], r);
    update = update || !qFuzzyCompare(_requestedState.fogColor[1], g);
    update = update || !qFuzzyCompare(_requestedState.fogColor[2], b);
    if(!update)
        return;

    _requestedState.fogColor[0] = r;
    _requestedState.fogColor[1] = g;
    _requestedState.fogColor[2] = b;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setSkyColor( const float& r, const float& g, const float& b, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.skyColor[0], r);
    update = update || !qFuzzyCompare(_requestedState.skyColor[1], g);
    update = update || !qFuzzyCompare(_requestedState.skyColor[2], b);
    if(!update)
        return;

    _requestedState.skyColor[0] = r;
    _requestedState.skyColor[1] = g;
    _requestedState.skyColor[2] = b;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setAzimuth( const float& azimuth, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    float normalizedAzimuth = Utilities::normalizedAngleDegrees(azimuth);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.azimuth, normalizedAzimuth);
    if(!update)
        return;

    _requestedState.azimuth = normalizedAzimuth;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setElevationAngle( const float& elevationAngle, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(elevationAngle, 90.0f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.elevationAngle, clampedValue);
    if(!update)
        return;

    _requestedState.elevationAngle = clampedValue;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setTarget( const PointI& target31, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    auto wrappedTarget31 = target31;
    const auto maxTile31Index = static_cast<signed>(1u << 31);
    if(wrappedTarget31.x < 0)
        wrappedTarget31.x += maxTile31Index;
    if(wrappedTarget31.y < 0)
        wrappedTarget31.y += maxTile31Index;

    bool update = forcedUpdate || (_requestedState.target31 != wrappedTarget31);
    if(!update)
        return;

    _requestedState.target31 = wrappedTarget31;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setZoom( const float& zoom, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    const auto clampedValue = qMax(std::numeric_limits<float>::epsilon(), qMin(zoom, 31.49999f));

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.requestedZoom, clampedValue);
    if(!update)
        return;

    _requestedState.requestedZoom = clampedValue;
    _requestedState.zoomBase = static_cast<ZoomLevel>(qRound(clampedValue));
    assert(_requestedState.zoomBase >= 0 && _requestedState.zoomBase <= 31);
    _requestedState.zoomFraction = _requestedState.requestedZoom - _requestedState.zoomBase;

    invalidateCurrentState();
}

void OsmAnd::MapRenderer::setHeightScaleFactor( const float& factor, bool forcedUpdate /*= false*/ )
{
    QWriteLocker scopedLocker(&_stateLock);

    bool update = forcedUpdate || !qFuzzyCompare(_requestedState.heightScaleFactor, factor);
    if(!update)
        return;

    _requestedState.heightScaleFactor = factor;

    invalidateCurrentState();
}

std::shared_ptr<OsmAnd::IMapTileProvider::Tile> OsmAnd::MapRenderer::prepareTileForUploadingToGPU( const MapTileLayerId& layerId, const std::shared_ptr<IMapTileProvider::Tile>& tile )
{
    if(tile->type == IMapTileProvider::Type::Bitmap)
    {
        auto bitmapTile = static_cast<IMapBitmapTileProvider::Tile*>(tile.get());

        // Check if we're going to convert
        bool doConvert = false;
        const bool force16bit = (currentConfiguration.limitTextureColorDepthBy16bits && bitmapTile->bitmap->getConfig() == SkBitmap::Config::kARGB_8888_Config);
        const bool canUsePaletteTextures = currentConfiguration.paletteTexturesAllowed && renderAPI->isSupported_8bitPaletteRGBA8;
        const bool paletteTexture = (bitmapTile->bitmap->getConfig() == SkBitmap::Config::kIndex8_Config);
        const bool unsupportedFormat =
            ( canUsePaletteTextures ? !paletteTexture : paletteTexture) ||
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
        if(doConvert && (convertedAlphaChannelData == IMapBitmapTileProvider::AlphaChannelData::Undefined))
        {
            convertedAlphaChannelData = SkBitmap::ComputeIsOpaque(*bitmapTile->bitmap.get())
                ? IMapBitmapTileProvider::AlphaChannelData::NotPresent
                : IMapBitmapTileProvider::AlphaChannelData::Present;
        }

        // If we have limit of 16bits per pixel in bitmaps, convert to ARGB(4444) or RGB(565)
        if(force16bit)
        {
            auto convertedBitmap = new SkBitmap();

            bitmapTile->bitmap->deepCopyTo(convertedBitmap,
                convertedAlphaChannelData == IMapBitmapTileProvider::AlphaChannelData::Present
                    ? SkBitmap::Config::kARGB_4444_Config
                    : SkBitmap::Config::kRGB_565_Config);

            auto convertedTile = new IMapBitmapTileProvider::Tile(convertedBitmap, convertedAlphaChannelData);
            return std::shared_ptr<IMapTileProvider::Tile>(convertedTile);
        }

        // If we have any other unsupported format, convert to proper 16bit or 32bit
        if(unsupportedFormat)
        {
            auto convertedBitmap = new SkBitmap();

            bitmapTile->bitmap->deepCopyTo(convertedBitmap,
                currentConfiguration.limitTextureColorDepthBy16bits
                    ? (convertedAlphaChannelData == IMapBitmapTileProvider::AlphaChannelData::Present ? SkBitmap::Config::kARGB_4444_Config : SkBitmap::Config::kRGB_565_Config)
                    : SkBitmap::kARGB_8888_Config);

            auto convertedTile = new IMapBitmapTileProvider::Tile(convertedBitmap, convertedAlphaChannelData);
            return std::shared_ptr<IMapTileProvider::Tile>(convertedTile);
        }
    }

    return tile;
}
