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
    setDisplayDensityFactor(1.0f);
    setFieldOfView(45.0f);
    setDistanceToFog(1000.0f);
    setAzimuth(0.0f);
    setElevationAngle(45.0f);
    setTarget(PointI(std::numeric_limits<int32_t>::max() / 2, std::numeric_limits<int32_t>::max() / 2));
    setZoom(0);
    setPreferredTextureDepth(TextureDepth::_16bits);
}

OsmAnd::IRenderer::~IRenderer()
{
}

void OsmAnd::IRenderer::setTileProvider( const std::shared_ptr<IMapTileProvider>& source )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = (_pendingConfig.tileProvider != source);
    if(!update)
        return;

    _pendingConfig.tileProvider = source;

    invalidateTileCache();
    invalidateConfiguration();
}

void OsmAnd::IRenderer::setPreferredTextureDepth( TextureDepth depth )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = (_pendingConfig.preferredTextureDepth != depth);
    if(!update)
        return;

    _pendingConfig.preferredTextureDepth = depth;

    invalidateTileCache();
    invalidateConfiguration();
}

void OsmAnd::IRenderer::setWindowSize( const PointI& windowSize )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = (_pendingConfig.windowSize != windowSize);
    if(!update)
        return;

    _pendingConfig.windowSize = windowSize;

    invalidateConfiguration();
}

void OsmAnd::IRenderer::setDisplayDensityFactor( const float& factor )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = !qFuzzyCompare(_pendingConfig.displayDensityFactor, factor);
    if(!update)
        return;

    _pendingConfig.displayDensityFactor = factor;

    invalidateConfiguration();
}

void OsmAnd::IRenderer::setViewport( const AreaI& viewport )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = (_pendingConfig.viewport != viewport);
    if(!update)
        return;

    _pendingConfig.viewport = viewport;

    invalidateConfiguration();
}

void OsmAnd::IRenderer::setFieldOfView( const float& fieldOfView )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = !qFuzzyCompare(_pendingConfig.fieldOfView, fieldOfView);
    if(!update)
        return;

    _pendingConfig.fieldOfView = fieldOfView;

    invalidateConfiguration();
}

void OsmAnd::IRenderer::setDistanceToFog( const float& fogDistance )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = !qFuzzyCompare(_pendingConfig.fogDistance, fogDistance);
    if(!update)
        return;

    _pendingConfig.fogDistance = fogDistance;

    invalidateConfiguration();
}

void OsmAnd::IRenderer::setAzimuth( const float& azimuth )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = !qFuzzyCompare(_pendingConfig.azimuth, azimuth);
    if(!update)
        return;

    _pendingConfig.azimuth = azimuth;

    invalidateConfiguration();
}

void OsmAnd::IRenderer::setElevationAngle( const float& elevationAngle )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = !qFuzzyCompare(_pendingConfig.elevationAngle, elevationAngle);
    if(!update)
        return;

    _pendingConfig.elevationAngle = elevationAngle;

    invalidateConfiguration();
}

void OsmAnd::IRenderer::setTarget( const PointI& target31 )
{
    QMutexLocker scopeLock(&_pendingToActiveConfigMutex);

    bool update = (_pendingConfig.target31 != target31);
    if(!update)
        return;

    _pendingConfig.target31 = target31;

    invalidateConfiguration();
}

void OsmAnd::IRenderer::setZoom( const float& zoom )
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
        _tilesCache.clearExceptInterestSet(_visibleTiles, _activeConfig.zoomBase, qMax(0, _activeConfig.zoomBase - 2), qMin(31, _activeConfig.zoomBase + 2));
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
            cacheHit = _tilesCache.contains(_activeConfig.zoomBase, tileId);
        }
        if(!cacheHit)
        {
            QMutexLocker scopeLock(&_tilesPendingToCacheMutex);
            cacheHit = _tilesPendingToCache[_activeConfig.zoomBase].contains(tileId);
        }

        if(cacheHit)
            continue;

        // Try to obtain tile from provider immediately
        std::shared_ptr<SkBitmap> tileBitmap;
        cacheHit = _activeConfig.tileProvider->obtainTile(tileId, _activeConfig.zoomBase, tileBitmap, _activeConfig.preferredTextureDepth == IRenderer::_32bits ? SkBitmap::kARGB_8888_Config : SkBitmap::kRGB_565_Config);
        if(cacheHit)
        {
            cacheTile(tileId, _activeConfig.zoomBase, tileBitmap);
            continue;
        }

        // If still cache miss, order delayed
        _activeConfig.tileProvider->obtainTile(tileId, _activeConfig.zoomBase, callback, _activeConfig.preferredTextureDepth == IRenderer::_32bits ? SkBitmap::kARGB_8888_Config : SkBitmap::kRGB_565_Config);
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
{
}

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
#   include "Renderer_OpenGL.h"
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IRenderer> OSMAND_CORE_CALL OsmAnd::createRenderer_OpenGL()
    {
        return std::shared_ptr<OsmAnd::IRenderer>(new Renderer_OpenGL());
    }
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
