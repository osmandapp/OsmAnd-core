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
    , _matricesAreDirty(true)
    , _tilesetIsDirty(true)
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
    , matricesAreDirty(_matricesAreDirty)
    , tilesetIsDirty(_tilesetIsDirty)
{
}

OsmAnd::IRenderer::~IRenderer()
{
}

void OsmAnd::IRenderer::setSource( const std::shared_ptr<MapDataCache>& source )
{
    _source = source;

    _tilesetIsDirty = true;
}

bool OsmAnd::IRenderer::updateViewport( const PointI& windowSize, const AreaI& viewport, float fieldOfView, float fogDistance )
{
    _matricesAreDirty = _matricesAreDirty || (_windowSize != windowSize);
    _matricesAreDirty = _matricesAreDirty || (_viewport != viewport);
    _matricesAreDirty = _matricesAreDirty || !qFuzzyCompare(_fieldOfView, fieldOfView);
    _matricesAreDirty = _matricesAreDirty || !qFuzzyCompare(_fogDistance, fogDistance);

    _windowSize = windowSize;
    _viewport = viewport;
    _fieldOfView = fieldOfView;
    _fogDistance = fogDistance;

    return _matricesAreDirty;
}

bool OsmAnd::IRenderer::updateCamera( float distanceFromTarget, float azimuth, float elevationAngle )
{
    _matricesAreDirty = _matricesAreDirty || !qFuzzyCompare(_distanceFromTarget, distanceFromTarget);
    _matricesAreDirty = _matricesAreDirty || !qFuzzyCompare(_azimuth, azimuth);
    _matricesAreDirty = _matricesAreDirty || !qFuzzyCompare(_elevationAngle, elevationAngle);

    _distanceFromTarget = distanceFromTarget;
    _azimuth = azimuth;
    _elevationAngle = elevationAngle;

    return _matricesAreDirty;
}

bool OsmAnd::IRenderer::updateMap( const PointI& target31, uint32_t zoom )
{
    assert(zoom >= 0 && zoom <= 31);

    _matricesAreDirty = _matricesAreDirty || (_target31 != target31);
    _matricesAreDirty = _matricesAreDirty || (_zoom != zoom);

    _target31 = target31;
    _zoom = zoom;

    return _matricesAreDirty;
}

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
OSMAND_CORE_API std::shared_ptr<OsmAnd::IRenderer> OSMAND_CORE_CALL OsmAnd::createRenderer_OpenGL()
{
    return std::shared_ptr<OsmAnd::IRenderer>(new Renderer_OpenGL());
}
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
