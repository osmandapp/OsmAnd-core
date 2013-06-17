#include "IRenderer.h"

#include <assert.h>

#include "IMapTileProvider.h"

OsmAnd::IRenderer::IRenderer()
    : _viewIsDirty(true)
    , _tilesCacheInvalidated(false)
    , _pendingToActiveConfigMutex(QMutex::Recursive)
    , _configInvalidated(true)
    , _tilesCacheMutex(QMutex::Recursive)
    , _tilesPendingToCacheMutex(QMutex::Recursive)
    , _isRenderingInitialized(false)
    , viewIsDirty(_viewIsDirty)
    , tilesCacheInvalidated(_tilesCacheInvalidated)
    , visibleTiles(_visibleTiles)
    , configuration(_pendingConfig)
    , isRenderingInitialized(_isRenderingInitialized)
{
}

OsmAnd::IRenderer::~IRenderer()
{
}

void OsmAnd::IRenderer::setTileProvider( const std::shared_ptr<IMapTileProvider>& source )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = false;
    update = update || (_pendingConfig.tileProvider != source);
    
    _pendingConfig.tileProvider = source;

    if(update)
    {
        invalidateTileCache();
        invalidateConfiguration();
    }
}

void OsmAnd::IRenderer::updateViewport( const PointI& windowSize, const AreaI& viewport, float fieldOfView, float fogDistance )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = false;
    update = update || (_pendingConfig.windowSize != windowSize);
    update = update || (_pendingConfig.viewport != viewport);
    update = update || !qFuzzyCompare(_pendingConfig.fieldOfView, fieldOfView);
    update = update || !qFuzzyCompare(_pendingConfig.fogDistance, fogDistance);

    _pendingConfig.windowSize = windowSize;
    _pendingConfig.viewport = viewport;
    _pendingConfig.fieldOfView = fieldOfView;
    _pendingConfig.fogDistance = fogDistance;

    if(update)
        invalidateConfiguration();
}

void OsmAnd::IRenderer::updateCamera( float distanceFromTarget, float azimuth, float elevationAngle )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = false;
    update = update || !qFuzzyCompare(_pendingConfig.distanceFromTarget, distanceFromTarget);
    update = update || !qFuzzyCompare(_pendingConfig.azimuth, azimuth);
    update = update || !qFuzzyCompare(_pendingConfig.elevationAngle, elevationAngle);

    _pendingConfig.distanceFromTarget = distanceFromTarget;
    _pendingConfig.azimuth = azimuth;
    _pendingConfig.elevationAngle = elevationAngle;

    if(update)
        invalidateConfiguration();
}

void OsmAnd::IRenderer::updateMap( const PointI& target31, uint32_t zoom )
{
    assert(zoom >= 0 && zoom <= 31);

    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = false;
    update = update || (_pendingConfig.target31 != target31);
    update = update || (_pendingConfig.zoom != zoom);

    _pendingConfig.target31 = target31;
    _pendingConfig.zoom = zoom;

    if(update)
        invalidateConfiguration();
}

void OsmAnd::IRenderer::setPreferredTextureDepth( TextureDepth depth )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = false;
    update = update || (_pendingConfig.preferredTextureDepth != depth);

    _pendingConfig.preferredTextureDepth = depth;
    
    if(update)
    {
        invalidateTileCache();
        invalidateConfiguration();
    }
}

void OsmAnd::IRenderer::updateTilesCache()
{
    // If tiles cache was invalidated, purge it
    if(_tilesCacheInvalidated)
    {
        purgeTilesCache();
        _tilesCacheInvalidated = false;
    }

    // Clean-up cache from useless tiles
    {
        QMutexLocker scopeLock(&_tilesCacheMutex);
        _tilesCache.clearExceptInterestSet(_visibleTiles, _activeConfig.zoom, qMax(0, static_cast<signed>(_activeConfig.zoom) - 2), qMin(31u, _activeConfig.zoom + 2));
    }

    // Cache missing tiles
    auto callback = std::bind(&IRenderer::tileReadyCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    for(auto itTileId = _visibleTiles.begin(); itTileId != _visibleTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        // Obtain tile from cache
        bool cacheHit;
        {
            QMutexLocker scopeLock(&_tilesCacheMutex);
            cacheHit = _tilesCache.contains(_activeConfig.zoom, tileId);
        }
        if(!cacheHit)
        {
            QMutexLocker scopeLock(&_tilesPendingToCacheMutex);
            cacheHit = _tilesPendingToCache[_activeConfig.zoom].contains(tileId);
        }

        if(cacheHit)
            continue;

        // Try to obtain tile from provider immediately
        std::shared_ptr<SkBitmap> tileBitmap;
        cacheHit = _activeConfig.tileProvider->obtainTile(tileId, _activeConfig.zoom, tileBitmap, _activeConfig.preferredTextureDepth == IRenderer::_32bits ? SkBitmap::kARGB_8888_Config : SkBitmap::kRGB_565_Config);
        if(cacheHit)
        {
            cacheTile(tileId, _activeConfig.zoom, tileBitmap);
            continue;
        }

        // If still cache miss, order delayed
        _activeConfig.tileProvider->obtainTile(tileId, _activeConfig.zoom, callback, _activeConfig.preferredTextureDepth == IRenderer::_32bits ? SkBitmap::kARGB_8888_Config : SkBitmap::kRGB_565_Config);
    }
}

void OsmAnd::IRenderer::tileReadyCallback( const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap )
{
    QMutexLocker scopeLock(&_tilesCacheMutex);

    cacheTile(tileId, zoom, tileBitmap);

    _viewIsDirty = true;
    if(redrawRequestCallback)
        redrawRequestCallback();
}

void OsmAnd::IRenderer::purgeTilesCache()
{
    {
        QMutexLocker scopeLock(&_tilesCacheMutex);
        _tilesCache.clearAll();
    }
    {
        QMutexLocker scopeLock(&_tilesPendingToCacheMutex);
        for(auto itLevel = _tilesPendingToCache.begin(); itLevel != _tilesPendingToCache.end(); ++itLevel)
        {
            auto& level = *itLevel;
            level.clear();
        }
    }
}

int OsmAnd::IRenderer::getCachedTilesCount() const
{
    return _tilesCache.tilesCount;
}

void OsmAnd::IRenderer::requestRedraw()
{
    _viewIsDirty = true;
    if(redrawRequestCallback)
        redrawRequestCallback();
}

void OsmAnd::IRenderer::invalidateTileCache()
{
    _tilesCacheInvalidated = true;
}

void OsmAnd::IRenderer::invalidateConfiguration()
{
    _configInvalidated = true;
    requestRedraw();
}

void OsmAnd::IRenderer::updateConfiguration()
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    _activeConfig = _pendingConfig;

    _configInvalidated = false;
}

OsmAnd::IRenderer::CachedTile::CachedTile( const uint32_t& zoom, const TileId& id, const size_t& usedMemory )
    : TileZoomCache::Tile(zoom, id, usedMemory)
{
}

OsmAnd::IRenderer::CachedTile::~CachedTile()
{
}

OsmAnd::IRenderer::Configuration::Configuration()
    : fieldOfView(45.0f)
    , fogDistance(1000.0f)
    , distanceFromTarget(500.0f)
    , azimuth(0.0f)
    , elevationAngle(45.0f)
    , target31( std::numeric_limits<int32_t>::max() / 2, std::numeric_limits<int32_t>::max() / 2 )
    , zoom(15)
    , preferredTextureDepth(TextureDepth::_16bits)
{
}

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
#   include "Renderer_OpenGL.h"
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IRenderer> OSMAND_CORE_CALL OsmAnd::createRenderer_OpenGL()
    {
        return std::shared_ptr<OsmAnd::IRenderer>(new Renderer_OpenGL());
    }
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
