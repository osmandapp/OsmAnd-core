#include "IMapRenderer.h"

#include <assert.h>

#include "IMapTileProvider.h"

OsmAnd::IMapRenderer::IMapRenderer()
    : _viewIsDirty(true)
    , _tilesCacheInvalidated(false)
    , _elevationDataCacheInvalidated(false)
    , _pendingToActiveConfigMutex(QMutex::Recursive)
    , _configInvalidated(true)
    , _tilesCacheMutex(QMutex::Recursive)
    , _tilesPendingToCacheMutex(QMutex::Recursive)
    , _isRenderingInitialized(false)
    , _renderThreadId(nullptr)
    , viewIsDirty(_viewIsDirty)
    , tilesCacheInvalidated(_tilesCacheInvalidated)
    , elevationDataCacheInvalidated(_elevationDataCacheInvalidated)
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
    setPreferredTextureDepth(TextureDepth::_16bits);
    setTextureAtlasesUsagePermit(true);
}

OsmAnd::IMapRenderer::~IMapRenderer()
{
}

void OsmAnd::IMapRenderer::setTileProvider( const std::shared_ptr<IMapTileProvider>& source )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = (_pendingConfig.tileProvider != source);
    if(!update)
        return;

    _pendingConfig.tileProvider = source;

    invalidateTileCache();
    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setElevationDataProvider( const std::shared_ptr<IMapElevationDataProvider>& source )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = (_pendingConfig.elevationDataProvider != source);
    if(!update)
        return;

    _pendingConfig.elevationDataProvider = source;

    invalidateElevationDataCache();
    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setPreferredTextureDepth( TextureDepth depth )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = (_pendingConfig.preferredTextureDepth != depth);
    if(!update)
        return;

    _pendingConfig.preferredTextureDepth = depth;

    invalidateTileCache();
    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setWindowSize( const PointI& windowSize )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = (_pendingConfig.windowSize != windowSize);
    if(!update)
        return;

    _pendingConfig.windowSize = windowSize;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setDisplayDensityFactor( const float& factor )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = !qFuzzyCompare(_pendingConfig.displayDensityFactor, factor);
    if(!update)
        return;

    _pendingConfig.displayDensityFactor = factor;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setViewport( const AreaI& viewport )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = (_pendingConfig.viewport != viewport);
    if(!update)
        return;

    _pendingConfig.viewport = viewport;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setFieldOfView( const float& fieldOfView )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = !qFuzzyCompare(_pendingConfig.fieldOfView, fieldOfView);
    if(!update)
        return;

    _pendingConfig.fieldOfView = fieldOfView;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setDistanceToFog( const float& fogDistance )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = !qFuzzyCompare(_pendingConfig.fogDistance, fogDistance);
    if(!update)
        return;

    _pendingConfig.fogDistance = fogDistance;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setAzimuth( const float& azimuth )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = !qFuzzyCompare(_pendingConfig.azimuth, azimuth);
    if(!update)
        return;

    _pendingConfig.azimuth = azimuth;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setElevationAngle( const float& elevationAngle )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = !qFuzzyCompare(_pendingConfig.elevationAngle, elevationAngle);
    if(!update)
        return;

    _pendingConfig.elevationAngle = elevationAngle;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setTarget( const PointI& target31 )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = (_pendingConfig.target31 != target31);
    if(!update)
        return;

    _pendingConfig.target31 = target31;

    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::setZoom( const float& zoom )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

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
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = (_pendingConfig.textureAtlasesAllowed != allow);
    if(!update)
        return;

    _pendingConfig.textureAtlasesAllowed = allow;

    invalidateTileCache();
    invalidateConfiguration();
}

void OsmAnd::IMapRenderer::obtainMissingTiles()
{
    // Cache missing tiles
    auto callback = std::bind(&IMapRenderer::tileReadyCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    for(auto itTileId = _visibleTiles.begin(); itTileId != _visibleTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        // Obtain tile from cache
        bool cacheHit;
        {
            QMutexLocker scopeLock(&_tilesCacheMutex);
            cacheHit = _tilesCache.contains(_activeConfig.zoomBase, tileId);
        }
        if(!cacheHit)
        {
            QMutexLocker scopeLock(&_tilesPendingToCacheMutex);
            cacheHit = _tilesPendingToCache[_activeConfig.zoomBase].contains(tileId);
        }

        if(cacheHit)
            continue;

        //TODO: THIS NEEDS TO BE REDONE!
        // Try to obtain tile from provider immediately
        std::shared_ptr<SkBitmap> tileBitmap;
        cacheHit = _activeConfig.tileProvider->obtainTileImmediate(tileId, _activeConfig.zoomBase, tileBitmap, _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? SkBitmap::kARGB_8888_Config : SkBitmap::kRGB_565_Config);
        if(cacheHit)
        {
            cacheTile(tileId, _activeConfig.zoomBase, tileBitmap);
            continue;
        }

        // If still cache miss, order delayed
        _activeConfig.tileProvider->obtainTileDeffered(tileId, _activeConfig.zoomBase, callback, _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? SkBitmap::kARGB_8888_Config : SkBitmap::kRGB_565_Config);
    }
}

void OsmAnd::IMapRenderer::tileReadyCallback( const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap )
{
    QMutexLocker scopeLock(&_tilesCacheMutex);

    cacheTile(tileId, zoom, tileBitmap);

    requestRedraw();
}

void OsmAnd::IMapRenderer::cacheTile( const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap )
{
    assert(!_tilesCache.contains(zoom, tileId));

    if(_renderThreadId != QThread::currentThreadId())
    {
        QMutexLocker scopeLock(&_tilesPendingToCacheMutex);

        assert(!_tilesPendingToCache[zoom].contains(tileId));

        TilePendingToCache pendingTile;
        pendingTile.tileId = tileId;
        pendingTile.zoom = zoom;
        pendingTile.tileBitmap = tileBitmap;
        _tilesPendingToCacheQueue.enqueue(pendingTile);
        _tilesPendingToCache[zoom].insert(tileId);

        return;
    }

    uploadTileToTexture(tileId, zoom, tileBitmap);
}

void OsmAnd::IMapRenderer::purgeTilesCache()
{
    // Purge all already cached tiles
    {
        QMutexLocker scopeLock(&_tilesCacheMutex);
        _tilesCache.clearAll();
    }

    // Purge all tiles that had to be cached during this frame
    {
        QMutexLocker scopeLock(&_tilesPendingToCacheMutex);
        for(auto itLevel = _tilesPendingToCache.begin(); itLevel != _tilesPendingToCache.end(); ++itLevel)
        {
            auto& level = *itLevel;
            level.clear();
        }
    }
}

int OsmAnd::IMapRenderer::getCachedTilesCount() const
{
    return _tilesCache.tilesCount;
}

void OsmAnd::IMapRenderer::requestRedraw()
{
    _viewIsDirty = true;
    if(redrawRequestCallback)
        redrawRequestCallback();
}

void OsmAnd::IMapRenderer::invalidateTileCache()
{
    _tilesCacheInvalidated = true;
}

void OsmAnd::IMapRenderer::invalidateElevationDataCache()
{
    _elevationDataCacheInvalidated = true;
}

void OsmAnd::IMapRenderer::invalidateConfiguration()
{
    _configInvalidated = true;
    requestRedraw();
}

void OsmAnd::IMapRenderer::updateConfiguration()
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    _activeConfig = _pendingConfig;

    _configInvalidated = false;
}

void OsmAnd::IMapRenderer::purgeElevationDataCache()
{
}

void OsmAnd::IMapRenderer::updateElevationDataCache()
{
}

void OsmAnd::IMapRenderer::initializeRendering()
{
    assert(!_isRenderingInitialized);

    assert(_renderThreadId == nullptr);
    _renderThreadId = QThread::currentThreadId();

    if(_configInvalidated)
        updateConfiguration();

    _isRenderingInitialized = true;
}

void OsmAnd::IMapRenderer::performRendering()
{
    assert(_isRenderingInitialized);

    assert(_renderThreadId == QThread::currentThreadId());

    if(_configInvalidated)
        updateConfiguration();

    if(!viewIsDirty)
        return;

    // If tiles cache was invalidated, purge it first
    if(_tilesCacheInvalidated)
    {
        purgeTilesCache();
        _tilesCacheInvalidated = false;
    }
    // Otherwise, clean-up cache from useless tiles
    else
    {
        QMutexLocker scopeLock(&_tilesCacheMutex);
        _tilesCache.clearExceptInterestSet(_visibleTiles, _activeConfig.zoomBase, qMax(0, _activeConfig.zoomBase - 2), qMin(31, _activeConfig.zoomBase + 2));
    }

    // Process tiles that are in pending-to-cache queue.
    // These are tiles that have been loaded to main memory but not yet uploaded
    // to GPU memory, since this can only be done in render thread.
    processPendingToCacheTiles();

    // Now we need to obtain all tiles that are still missing
    obtainMissingTiles();

    // If evaluation was invalidated, we need to recreate tile patch
    if(_elevationDataCacheInvalidated)
    {
        purgeElevationDataCache();
        _elevationDataCacheInvalidated = false;
    }
    // Otherise, clean-up cache from useless tiles
    {
        //TODO: cleanup
    }
    updateElevationDataCache();
}

void OsmAnd::IMapRenderer::releaseRendering()
{
    assert(isRenderingInitialized);

    purgeTilesCache();
    assert(_renderThreadId == QThread::currentThreadId());

    _renderThreadId = nullptr;

    _isRenderingInitialized = false;
}

void OsmAnd::IMapRenderer::processPendingToCacheTiles()
{
    TilePendingToCache pendingTile;
    bool morePending = false;

    // Work with queue via mutex
    {
        QMutexLocker scopeLock(&_tilesPendingToCacheMutex);

        if(_tilesPendingToCacheQueue.isEmpty())
            return;

        pendingTile = _tilesPendingToCacheQueue.dequeue();
        _tilesPendingToCache[pendingTile.zoom].remove(pendingTile.tileId);

        morePending = !_tilesPendingToCacheQueue.isEmpty();
    }

    // Upload that tile to texture
    {
        QMutexLocker scopeLock(&_tilesCacheMutex);
        uploadTileToTexture(pendingTile.tileId, pendingTile.zoom, pendingTile.tileBitmap);
    }
    
    // If there are more tiles, request redraw to cache new tile on next frame
    if(morePending)
        requestRedraw();
}

OsmAnd::IMapRenderer::CachedTile::CachedTile( const uint32_t& zoom, const TileId& id, const size_t& usedMemory )
    : TileZoomCache::Tile(zoom, id, usedMemory)
{
}

OsmAnd::IMapRenderer::CachedTile::~CachedTile()
{
}

OsmAnd::IMapRenderer::Configuration::Configuration()
{
}

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
#   include "OpenGL/AtlasMapRenderer_OpenGL.h"
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL OsmAnd::createAtlasMapRenderer_OpenGL()
    {
        return std::shared_ptr<OsmAnd::IMapRenderer>(new AtlasMapRenderer_OpenGL());
    }
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
