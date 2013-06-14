#include "IRenderer.h"

#include <assert.h>

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
    , _tilesetCacheDirty(true)
    , source(_source)
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

void OsmAnd::IRenderer::setSource( const std::shared_ptr<MapDataCache>& source )
{
    _source = source;

    _tilesetCacheDirty = true;
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

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
OSMAND_CORE_API std::shared_ptr<OsmAnd::IRenderer> OSMAND_CORE_CALL OsmAnd::createRenderer_OpenGL()
{
    return std::shared_ptr<OsmAnd::IRenderer>(new Renderer_OpenGL());
}
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
