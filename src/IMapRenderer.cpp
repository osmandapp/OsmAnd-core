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
    , viewIsDirty(_viewIsDirty)
    , tilesCacheInvalidated(_tilesCacheInvalidated)
    , elevationDataCacheInvalidated(_elevationDataCacheInvalidated)
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

void OsmAnd::IMapRenderer::updateTilesCache()
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

        // Try to obtain tile from provider immediately
        std::shared_ptr<SkBitmap> tileBitmap;
        cacheHit = _activeConfig.tileProvider->obtainTile(tileId, _activeConfig.zoomBase, tileBitmap, _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? SkBitmap::kARGB_8888_Config : SkBitmap::kRGB_565_Config);
        if(cacheHit)
        {
            cacheTile(tileId, _activeConfig.zoomBase, tileBitmap);
            continue;
        }

        // If still cache miss, order delayed
        _activeConfig.tileProvider->obtainTile(tileId, _activeConfig.zoomBase, callback, _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? SkBitmap::kARGB_8888_Config : SkBitmap::kRGB_565_Config);
    }
}

void OsmAnd::IMapRenderer::tileReadyCallback( const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap )
{
    QMutexLocker scopeLock(&_tilesCacheMutex);

    cacheTile(tileId, zoom, tileBitmap);

    _viewIsDirty = true;
    if(redrawRequestCallback)
        redrawRequestCallback();
}

void OsmAnd::IMapRenderer::purgeTilesCache()
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

void OsmAnd::IMapRenderer::updateElevationDataCache()
{
    if(_elevationDataCacheInvalidated)
    {
        //purgeTilesCache();
        _elevationDataCacheInvalidated = false;
    }
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
