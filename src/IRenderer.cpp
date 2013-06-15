#include "IRenderer.h"

#include <assert.h>

#include "IMapTileProvider.h"

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
#   include "Renderer_OpenGL.h"
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED

OsmAnd::IRenderer::IRenderer()
    : _fieldOfView(45.0f)
    , _fogDistance(1000.0f)
    , _distanceFromTarget(500.0f)
    , _azimuth(0.0f)
    , _elevationAngle(45.0f)
    , _target31( std::numeric_limits<int32_t>::max() / 2, std::numeric_limits<int32_t>::max() / 2 )
    , _zoom(15)
    , _viewIsDirty(true)
    , tileProvider(_tileProvider)
    , windowSize(_windowSize)
    , viewport(_viewport)
    , fieldOfView(_fieldOfView)
    , fogDistance(_fogDistance)
    , distanceFromTarget(_distanceFromTarget)
    , azimuth(_azimuth)
    , elevationAngle(_elevationAngle)
    , target31(_target31)
    , zoom(_zoom)
    , visibleTiles(_visibleTiles)
    , viewIsDirty(_viewIsDirty)
{
}

OsmAnd::IRenderer::~IRenderer()
{
}

void OsmAnd::IRenderer::setTileProvider( const std::shared_ptr<IMapTileProvider>& source )
{
    _tileProvider = source;
    purgeTilesCache();
}

bool OsmAnd::IRenderer::updateViewport( const PointI& windowSize, const AreaI& viewport, float fieldOfView, float fogDistance )
{
    _viewIsDirty = _viewIsDirty || (_windowSize != windowSize);
    _viewIsDirty = _viewIsDirty || (_viewport != viewport);
    _viewIsDirty = _viewIsDirty || !qFuzzyCompare(_fieldOfView, fieldOfView);
    _viewIsDirty = _viewIsDirty || !qFuzzyCompare(_fogDistance, fogDistance);

    _windowSize = windowSize;
    _viewport = viewport;
    _fieldOfView = fieldOfView;
    _fogDistance = fogDistance;

    return _viewIsDirty;
}

bool OsmAnd::IRenderer::updateCamera( float distanceFromTarget, float azimuth, float elevationAngle )
{
    _viewIsDirty = _viewIsDirty || !qFuzzyCompare(_distanceFromTarget, distanceFromTarget);
    _viewIsDirty = _viewIsDirty || !qFuzzyCompare(_azimuth, azimuth);
    _viewIsDirty = _viewIsDirty || !qFuzzyCompare(_elevationAngle, elevationAngle);

    _distanceFromTarget = distanceFromTarget;
    _azimuth = azimuth;
    _elevationAngle = elevationAngle;

    return _viewIsDirty;
}

bool OsmAnd::IRenderer::updateMap( const PointI& target31, uint32_t zoom )
{
    assert(zoom >= 0 && zoom <= 31);

    _viewIsDirty = _viewIsDirty || (_target31 != target31);
    _viewIsDirty = _viewIsDirty || (_zoom != zoom);

    _target31 = target31;
    _zoom = zoom;

    return _viewIsDirty;
}

void OsmAnd::IRenderer::cacheMissingTiles()
{
    auto callback = std::bind(&IRenderer::tileReadyCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    for(auto itTileId = _visibleTiles.begin(); itTileId != _visibleTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        // Obtain tile from cache
        QMap< uint64_t, std::shared_ptr<CachedTile> >::const_iterator itCachedTile;
        bool cacheHit;
        {
            QMutexLocker scopeLock(&_tileCacheMutex);
            itCachedTile = _cachedTiles.find(tileId);
            cacheHit = (itCachedTile != _cachedTiles.end());
        }

        if(cacheHit)
            continue;

        // Try to obtain tile from provider immediately
        std::shared_ptr<SkBitmap> tileBitmap;
        cacheHit = _tileProvider->obtainTile(tileId, _zoom, tileBitmap);
        if(cacheHit)
        {
            cacheTile(tileId, _zoom, tileBitmap);
            continue;
        }

        // If still cache miss, order delayed
        _tileProvider->obtainTile(tileId, _zoom, callback);
    }
}

void OsmAnd::IRenderer::tileReadyCallback( const uint64_t& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap )
{
    cacheTile(tileId, zoom, tileBitmap);

    _viewIsDirty = true;
}

void OsmAnd::IRenderer::purgeTilesCache()
{
    QMutexLocker scopeLock(&_tileCacheMutex);
    _cachedTiles.clear();
}

OsmAnd::IRenderer::CachedTile::~CachedTile()
{
}

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
OSMAND_CORE_API std::shared_ptr<OsmAnd::IRenderer> OSMAND_CORE_CALL OsmAnd::createRenderer_OpenGL()
{
    return std::shared_ptr<OsmAnd::IRenderer>(new Renderer_OpenGL());
}
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
