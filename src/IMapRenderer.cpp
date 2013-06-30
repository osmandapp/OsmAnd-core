#include "IMapRenderer.h"

#include <assert.h>

#include "IMapBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "OsmAndLogging.h"

OsmAnd::IMapRenderer::IMapRenderer()
    : _tileLayersCacheInvalidatedMask(0)
    , _configInvalidated(true)
    , _pendingConfigModificationMutex(QMutex::Recursive)
    , _isRenderingInitialized(false)
    , _renderThreadId(nullptr)
    , tilesCacheInvalidatedMask(_tileLayersCacheInvalidatedMask)
    , renderThreadId(_renderThreadId)
    , configuration(_pendingConfig)
    , visibleTiles(_visibleTiles)
    , isRenderingInitialized(_isRenderingInitialized)
{
    setDisplayDensityFactor(1.0f);
    setFieldOfView(45.0f);
    setDistanceToFog(1000.0f);
    setAzimuth(0.0f);
    setElevationAngle(45.0f);
    setTarget(PointI(std::numeric_limits<int32_t>::max() / 2, std::numeric_limits<int32_t>::max() / 2));
    setZoom(0);
    set16bitColorDepthLimit(false);
    setTextureAtlasesUsagePermit(true);
    setHeightmapPatchesPerSide(24);
}

OsmAnd::IMapRenderer::~IMapRenderer()
{
}

void OsmAnd::IMapRenderer::setTileProvider( const TileLayerId& layerId, const std::shared_ptr<IMapTileProvider>& tileProvider )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = (_pendingConfig.tileProviders[layerId] != tileProvider);
    if(!update)
        return;

    _pendingConfig.tileProviders[layerId] = tileProvider;

    invalidateTileLayerCache(layerId);
    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::set16bitColorDepthLimit( const bool& forced )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = (_pendingConfig.force16bitColorDepthLimit != forced);
    if(!update)
        return;

    _pendingConfig.force16bitColorDepthLimit = forced;

    for(int layerId = 0; layerId < TileLayerId::IdsCount; layerId++)
    {
        const auto& provider = _pendingConfig.tileProviders[layerId];
        if(!provider)
            continue;

        if(provider->type == IMapTileProvider::Bitmap)
            invalidateTileLayerCache(static_cast<TileLayerId>(layerId));
    }
    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setWindowSize( const PointI& windowSize )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = (_pendingConfig.windowSize != windowSize);
    if(!update)
        return;

    _pendingConfig.windowSize = windowSize;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setDisplayDensityFactor( const float& factor )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = !qFuzzyCompare(_pendingConfig.displayDensityFactor, factor);
    if(!update)
        return;

    _pendingConfig.displayDensityFactor = factor;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setViewport( const AreaI& viewport )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = (_pendingConfig.viewport != viewport);
    if(!update)
        return;

    _pendingConfig.viewport = viewport;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setFieldOfView( const float& fieldOfView )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = !qFuzzyCompare(_pendingConfig.fieldOfView, fieldOfView);
    if(!update)
        return;

    _pendingConfig.fieldOfView = fieldOfView;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setDistanceToFog( const float& fogDistance )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = !qFuzzyCompare(_pendingConfig.fogDistance, fogDistance);
    if(!update)
        return;

    _pendingConfig.fogDistance = fogDistance;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setAzimuth( const float& azimuth )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = !qFuzzyCompare(_pendingConfig.azimuth, azimuth);
    if(!update)
        return;

    _pendingConfig.azimuth = azimuth;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setElevationAngle( const float& elevationAngle )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = !qFuzzyCompare(_pendingConfig.elevationAngle, elevationAngle);
    if(!update)
        return;

    _pendingConfig.elevationAngle = elevationAngle;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setTarget( const PointI& target31 )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = (_pendingConfig.target31 != target31);
    if(!update)
        return;

    _pendingConfig.target31 = target31;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setZoom( const float& zoom )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = !qFuzzyCompare(_pendingConfig.requestedZoom, zoom);
    if(!update)
        return;

    _pendingConfig.requestedZoom = zoom;
    _pendingConfig.zoomBase = qRound(zoom);
    _pendingConfig.zoomFraction = _pendingConfig.requestedZoom - _pendingConfig.zoomBase;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setTextureAtlasesUsagePermit( const bool& allow )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = (_pendingConfig.textureAtlasesAllowed != allow);
    if(!update)
        return;

    _pendingConfig.textureAtlasesAllowed = allow;

    invalidateTileLayersCache();
    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setHeightmapPatchesPerSide( const uint32_t& patchesCount )
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    bool update = (_pendingConfig.heightmapPatchesPerSide != patchesCount);
    if(!update)
        return;

    _pendingConfig.heightmapPatchesPerSide = patchesCount;

    invalidateTileLayerCache(TileLayerId::ElevationData);
    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::initializeRendering()
{
    assert(!_isRenderingInitialized);

    assert(_renderThreadId == nullptr);
    _renderThreadId = QThread::currentThreadId();

    if(_configInvalidated)
        validateConfiguration();

    _isRenderingInitialized = true;
}

void OsmAnd::IMapRenderer::performRendering()
{
    assert(_isRenderingInitialized);
    assert(_renderThreadId == QThread::currentThreadId());

    if(_configInvalidated)
        validateConfiguration();

    // Check if we have invalidated cache layers
    {
        QMutexLocker scopeLock(&_tileLayersCacheInvalidatedMaskMutex);

        if(_tileLayersCacheInvalidatedMask)
        {
            for(int layerId = 0; layerId < TileLayerId::IdsCount; layerId++)
            {
                validateTileLayerCache(static_cast<TileLayerId>(layerId));

                _tileLayersCacheInvalidatedMask &= ~(1 << layerId);
            }
        }
        
        assert(_tileLayersCacheInvalidatedMask == 0);
    }

    // Keep cache fresh and throw away outdated tiles
    for(int layerId = 0; layerId < TileLayerId::IdsCount; layerId++)
    {
        auto& tileLayer = _tileLayers[layerId];
        
        QMutexLocker scopeLock(&tileLayer._cacheModificationMutex);

        //TODO: Use smarter clear condition
        tileLayer._cache.clearExceptInterestSet(_visibleTiles, _activeConfig.zoomBase, qMax(0, _activeConfig.zoomBase - 2), qMin(31, _activeConfig.zoomBase + 2));
    }

    // Process tiles that are in pending-to-cache queue.
    // These are tiles that have been loaded to main memory but not yet uploaded
    // to GPU memory, since this can only be done in render thread.
    for(int layerId = 0; layerId < TileLayerId::IdsCount; layerId++)
    {
        auto& tileLayer = _tileLayers[layerId];

        bool hasMore = tileLayer.uploadPending();
        if(hasMore)
        {
            // Schedule one more render pass to upload more pending
            requestRedraw();
            break;
        }
    }

    // Now we need to obtain all tiles that are still missing
    requestCacheMissTiles();
}

void OsmAnd::IMapRenderer::releaseRendering()
{
    assert(_isRenderingInitialized);
    assert(_renderThreadId == QThread::currentThreadId());

    // Purge cache of all layers
    for(int layerId = 0; layerId < TileLayerId::IdsCount; layerId++)
    {
        auto& tileLayer = _tileLayers[layerId];
        tileLayer.purgeCache();
    }

    _renderThreadId = nullptr;

    _isRenderingInitialized = false;
}

void OsmAnd::IMapRenderer::requestCacheMissTiles()
{
    for(auto itTileId = _visibleTiles.begin(); itTileId != _visibleTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        for(int layerId = 0; layerId < TileLayerId::IdsCount; layerId++)
        {
            auto& tileLayer = _tileLayers[layerId];
            if(!_activeConfig.tileProviders[layerId])
                continue;

            // Obtain tile from cache
            {
                QMutexLocker scopeLock(&tileLayer._pendingToCacheMutex);
                bool cacheHit = tileLayer._pendingToCache[_activeConfig.zoomBase].contains(tileId);
                if(cacheHit)
                    continue;
            }
            {
                tileLayer._cacheModificationMutex.lock();

                bool cacheHit = tileLayer._cache.contains(_activeConfig.zoomBase, tileId);
            
                // If tile is already in cache, or is pending to cache, do not request it
                if(cacheHit)
                    continue;

                const auto& tileProvider = _activeConfig.tileProviders[layerId];
                const auto callback = std::bind(&IMapRenderer::handleProvidedTile, this, static_cast<TileLayerId>(layerId), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

                // Try to obtain tile from provider immediately. Immediately means that data is available in-memory
                std::shared_ptr<IMapTileProvider::Tile> tile;
                bool availableImmediately = tileProvider->obtainTileImmediate(tileId, _activeConfig.zoomBase, tile);
                if(availableImmediately)
                {
                    //LogPrintf(LogSeverityLevel::Debug, "Uploading tile %dx%d@%d of layer %d to cache immediately\n", tileId.x, tileId.y, _activeConfig.zoomBase, layerId);
                    cacheTile(static_cast<TileLayerId>(layerId), tileId, _activeConfig.zoomBase, tile);
                    tileLayer._cacheModificationMutex.unlock();
                    continue;
                }
                tileLayer._cacheModificationMutex.unlock();

                // If still cache miss, order delayed
                {
                    QMutexLocker scopeLock(&tileLayer._requestedTilesMutex);
                    if(tileLayer._requestedTiles[_activeConfig.zoomBase].contains(tileId))
                        continue;
                    tileLayer._requestedTiles[_activeConfig.zoomBase].insert(tileId);

                    //LogPrintf(LogSeverityLevel::Debug, "Ordering tile %dx%d@%d of layer %d\n", tileId.x, tileId.y, _activeConfig.zoomBase, layerId);
                    tileProvider->obtainTileDeffered(tileId, _activeConfig.zoomBase, callback);
                }
            }
        }
    }
}

void OsmAnd::IMapRenderer::cacheTile( TileLayerId layerId, const TileId& tileId, uint32_t zoom, const std::shared_ptr<IMapTileProvider::Tile>& tile )
{
    auto& tileLayer = _tileLayers[layerId];

    assert(!tileLayer._cache.contains(zoom, tileId));

    if(!tile)
    {
        // Non-existent tile, simply put marker into cache
        tileLayer._cache.putTile(std::shared_ptr<TileZoomCache::Tile>(static_cast<TileZoomCache::Tile*>(
            new CachedTile(this, layerId, zoom, tileId, 0, nullptr, -1, 0))));
        return;
    }
    
    // Actually upload tile to texture
    //TODO: Uploading texture may fail. In this case we do not need to mark tile as cached
    uint64_t atlasPoolId = 0;
    void* textureRef = nullptr;
    int atlasSlotIndex = -1;
    size_t usedMemory = 0;
    uploadTileToTexture(layerId, tileId, zoom, tile, atlasPoolId, textureRef, atlasSlotIndex, usedMemory);
    
    tileLayer._cache.putTile(std::shared_ptr<TileZoomCache::Tile>(static_cast<TileZoomCache::Tile*>(
        new CachedTile(this, layerId, zoom, tileId, atlasPoolId, textureRef, atlasSlotIndex, usedMemory))));
}

void OsmAnd::IMapRenderer::handleProvidedTile( const TileLayerId& layerId, const TileId& tileId, uint32_t zoom, const std::shared_ptr<IMapTileProvider::Tile>& tile, bool success )
{
    auto& tileLayer = _tileLayers[layerId];
    
    {
        QMutexLocker scopeLock(&tileLayer._pendingToCacheMutex);

        //LogPrintf(LogSeverityLevel::Debug, "Putting tile %dx%d@%d of layer %d to cache queue\n", tileId.x, tileId.y, zoom, layerId);

        assert(!tileLayer._pendingToCache[zoom].contains(tileId));

        PendingToCacheTile pendingTile(this, layerId, zoom, tileId, tile);
        tileLayer._pendingToCacheQueue.enqueue(pendingTile);
        tileLayer._pendingToCache[zoom].insert(tileId);
    }

    {
        QMutexLocker scopeLock(&tileLayer._requestedTilesMutex);
        tileLayer._requestedTiles[zoom].remove(tileId);
    }

    requestRedraw();
}

void OsmAnd::IMapRenderer::requestRedraw()
{
    if(redrawRequestCallback)
        redrawRequestCallback();
}

void OsmAnd::IMapRenderer::invalidateTileLayerCache( const TileLayerId& layerId )
{
    _tileLayersCacheInvalidatedMask |= 1 << static_cast<int>(layerId);
}

void OsmAnd::IMapRenderer::validateTileLayerCache( const TileLayerId& layerId )
{
    auto& tileLayer = _tileLayers[layerId];

    tileLayer.purgeCache();
}

void OsmAnd::IMapRenderer::invalidateTileLayersCache()
{
    for(int layerId = 0; layerId < TileLayerId::IdsCount; layerId++)
        invalidateTileLayerCache(static_cast<TileLayerId>(layerId));
}

void OsmAnd::IMapRenderer::invalidateConfiguration()
{
    _configInvalidated = true;
    requestRedraw();
}

void OsmAnd::IMapRenderer::validateConfiguration()
{
    QMutexLocker scopeLock(&_pendingConfigModificationMutex);

    _activeConfig = _pendingConfig;
    updateConfiguration();

    _configInvalidated = false;
}

void OsmAnd::IMapRenderer::updateConfiguration()
{
}

OsmAnd::IMapRenderer::CachedTile::CachedTile( IMapRenderer* const renderer_, TileLayerId layerId_, const uint32_t& zoom_, const TileId& id_, const uint64_t& atlasPoolId_, void* textureRef_, int atlasSlotIndex_, const size_t& usedMemory_ )
    : TileZoomCache::Tile(zoom_, id_, usedMemory_)
    , renderer(renderer_)
    , layerId(layerId_)
    , atlasPoolId(atlasPoolId_)
    , textureRef(textureRef_)
    , atlasSlotIndex(atlasSlotIndex_)
{
}

OsmAnd::IMapRenderer::CachedTile::~CachedTile()
{
    assert(renderer->renderThreadId == QThread::currentThreadId());

    QMutexLocker scopeLock(&renderer->_tileLayers[layerId]._cacheModificationMutex);

    // Check if this is a slot on atlas
    if(atlasSlotIndex >= 0)
    {
        // This is a slot on atlas
        const auto& itRefCnt = renderer->_tileLayers[layerId]._textureRefCount.find(textureRef);
        auto& refCnt = *itRefCnt;
        refCnt -= 1;

        if(refCnt > 0)
        {
            // Atlas is still referenced, so free just this slot
            renderer->_tileLayers[layerId]._atlasTexturePools[atlasPoolId]._freedSlots.insert(textureRef, atlasSlotIndex);
        }
        else if(refCnt == 0)
        {
            // Oh well, last slot on atlas. Destroy atlas
            renderer->_tileLayers[layerId]._atlasTexturePools[atlasPoolId]._freedSlots.remove(textureRef);
            renderer->releaseTexture(textureRef);
            renderer->_tileLayers[layerId]._textureRefCount.remove(textureRef);
        }
    }
}

OsmAnd::IMapRenderer::Configuration::Configuration()
{
}

OsmAnd::IMapRenderer::PendingToCacheTile::PendingToCacheTile( IMapRenderer* const renderer_, TileLayerId layerId_, const uint32_t& zoom_, const TileId& tileId_, const std::shared_ptr<IMapTileProvider::Tile>& tile_ )
    : renderer(renderer_)
    , layerId(layerId_)
    , zoom(zoom_)
    , tileId(tileId_)
    , tile(tile_)
{
}

OsmAnd::IMapRenderer::AtlasTexturePool::AtlasTexturePool()
    : _textureSize(0)
    , _padding(0)
    , _lastNonFullTextureRef(nullptr)
    , _firstFreeSlotIndex(-1)
    , _tileSizeN(0.0f)
    , _tilePaddingN(0.0f)
    , _slotsPerSide(0)
    , _mipmapLevels(0)
{
}

OsmAnd::IMapRenderer::TileLayer::TileLayer()
    : _cacheModificationMutex(QMutex::Recursive)
    , _pendingToCacheMutex(QMutex::Recursive)
    , _requestedTilesMutex(QMutex::Recursive)
{
}

void OsmAnd::IMapRenderer::TileLayer::purgeCache()
{
    // Purge all already cached tiles
    {
        QMutexLocker scopeLock(&_cacheModificationMutex);
        _cache.clearAll();
    }

    // Purge all tiles that had to be cached during this frame
    {
        QMutexLocker scopeLock(&_pendingToCacheMutex);
        for(auto itLevel = _pendingToCache.begin(); itLevel != _pendingToCache.end(); ++itLevel)
        {
            auto& level = *itLevel;
            level.clear();
        }
        _pendingToCacheQueue.clear();
    }

    assert(_textureRefCount.isEmpty());
    _atlasTexturePools.clear();
}

bool OsmAnd::IMapRenderer::TileLayer::uploadPending()
{
    QMutexLocker scopeLock(&_pendingToCacheMutex);

    if(_pendingToCacheQueue.isEmpty())
        return false;

    const auto& pendingTile = _pendingToCacheQueue.dequeue();
    _pendingToCache[pendingTile.zoom].remove(pendingTile.tileId);

    //LogPrintf(LogSeverityLevel::Debug, "Going to upload tile %dx%d@%d of layer %d to cache from queue\n", pendingTile.tileId.x, pendingTile.tileId.y, pendingTile.zoom, pendingTile.layerId);
    {
        // Upload that tile to texture
        QMutexLocker scopeLock(&_cacheModificationMutex);

        //LogPrintf(LogSeverityLevel::Debug, "Uploading tile %dx%d@%d of layer %d to cache from queue\n", pendingTile.tileId.x, pendingTile.tileId.y, pendingTile.zoom, pendingTile.layerId);
        pendingTile.renderer->cacheTile(pendingTile.layerId, pendingTile.tileId, pendingTile.zoom, pendingTile.tile);
        //LogPrintf(LogSeverityLevel::Debug, "Uploaded tile %dx%d@%d of layer %d to cache from queue\n", pendingTile.tileId.x, pendingTile.tileId.y, pendingTile.zoom, pendingTile.layerId);
    }

    return !_pendingToCacheQueue.isEmpty();
}

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
#   include "OpenGL/AtlasMapRenderer_OpenGL.h"
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL OsmAnd::createAtlasMapRenderer_OpenGL()
    {
        return std::shared_ptr<OsmAnd::IMapRenderer>(new AtlasMapRenderer_OpenGL());
    }
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
