#include "Renderer_OpenGL.h"

#include <assert.h>
#if defined(WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QtGlobal>

OsmAnd::Renderer_OpenGL::Renderer_OpenGL()
{

}

OsmAnd::Renderer_OpenGL::~Renderer_OpenGL()
{

}

void OsmAnd::Renderer_OpenGL::setSource( const std::shared_ptr<MapDataCache>& source )
{
    IRenderer::setSource(source);
}

void OsmAnd::Renderer_OpenGL::computeMatrices()
{
    // Setup projection
    GLfloat aspectRatio = static_cast<GLfloat>(viewport.width());
    auto viewportHeight = viewport.height();
    if(viewportHeight > 0)
        aspectRatio /= static_cast<GLfloat>(viewportHeight);
    _glProjection = glm::perspective(_fieldOfView, aspectRatio, 0.1f, _fogDistance * 1.2f + _distanceFromTarget);

    // Setup camera
    const auto& c0 = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0.0f, -_distanceFromTarget));
    const auto& c1 = glm::rotate(c0, _elevationAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    _glModelview = glm::rotate(c1, _azimuth, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Setup viewport and window
    _glViewport = _viewport;
    _glWindowSize = _windowSize;
}

void OsmAnd::Renderer_OpenGL::refreshVisibleTileset()
{
    glm::vec4 glViewport
    (
        _glViewport.left,
        _glWindowSize.y - _glViewport.bottom,
        _glViewport.width(),
        _glViewport.height()
    );

    // Top-left far
    const auto& tlf = glm::unProject(
        glm::vec3(_glViewport.left, _glWindowSize.y - _glViewport.bottom + _glViewport.height(), 1.0),
        _glModelview, _glProjection, glViewport);

    // Top-right far
    const auto& trf = glm::unProject(
        glm::vec3(_glViewport.right, _glWindowSize.y - _glViewport.bottom + _glViewport.height(), 1.0),
        _glModelview, _glProjection, glViewport);
    
    // Bottom-left far
    const auto& blf = glm::unProject(
        glm::vec3(_glViewport.left, _glWindowSize.y - _glViewport.bottom, 1.0),
        _glModelview, _glProjection, glViewport);
    
    // Bottom-right far
    const auto& brf = glm::unProject(
        glm::vec3(_glViewport.right, _glWindowSize.y - _glViewport.bottom, 1.0),
        _glModelview, _glProjection, glViewport);
    
    // Top-left near
    const auto& tln = glm::unProject(
        glm::vec3(_glViewport.left, _glWindowSize.y - _glViewport.bottom + _glViewport.height(), 0.0),
        _glModelview, _glProjection, glViewport);
    
    // Top-right near
    const auto& trn = glm::unProject(
        glm::vec3(_glViewport.right, _glWindowSize.y - _glViewport.bottom + _glViewport.height(), 0.0),
        _glModelview, _glProjection, glViewport);
    
    // Bottom-left near
    const auto& bln = glm::unProject(
        glm::vec3(_glViewport.left, _glWindowSize.y - _glViewport.bottom, 0.0),
        _glModelview, _glProjection, glViewport);
    
    // Bottom-right near
    const auto& brn = glm::unProject(
        glm::vec3(_glViewport.right, _glWindowSize.y - _glViewport.bottom, 0.0),
        _glModelview, _glProjection, glViewport);
    
    // Obtain 4 normalized ray directions for each side of viewport
    const auto& tlRayD = glm::normalize(tlf - tln);
    const auto& trRayD = glm::normalize(trf - trn);
    const auto& blRayD = glm::normalize(blf - bln);
    const auto& brRayD = glm::normalize(brf - brn);
    
    // Our plane normal is always Y-up and plane origin is 0
    glm::vec3 planeN(0.0f, 1.0f, 0.0f);

    // Intersect 4 rays with tile-plane
    auto clip = _fogDistance * 1.2f + _distanceFromTarget;
    float tlD, trD, blD, brD;
    bool intersects;
    intersects = rayIntersectPlane(planeN, 0.0f, tlRayD, tln, tlD);
    if(!intersects)
        tlD = clip * 1.5f;
    auto tlP = tln + tlRayD * tlD;
    intersects = rayIntersectPlane(planeN, 0.0f, trRayD, trn, trD);
    if(!intersects)
        trD = clip * 1.5f;
    auto trP = trn + trRayD * trD;
    intersects = rayIntersectPlane(planeN, 0.0f, blRayD, bln, blD);
    if(!intersects)
        blD = clip * 1.5f;
    auto blP = bln + blRayD * blD;
    intersects = rayIntersectPlane(planeN, 0.0f, brRayD, brn, brD);
    if(!intersects)
        brD = clip * 1.5f;
    auto brP = brn + brRayD * brD;

    // Limit all I-points using clip distance
    auto tlLength = glm::length(tlP);
    if(tlLength > clip)
        tlP /= tlLength / clip;

    auto trLength = glm::length(trP);
    if(trLength > clip)
        trP /= trLength / clip;

    auto blLength = glm::length(blP);
    if(blLength > clip)
        blP /= blLength / clip;

    auto brLength = glm::length(brP);
    if(brLength > clip)
        brP /= brLength / clip;

    // Get tile indices
    PointI p0, p1, p2, p3;
    p0.x = static_cast<int32_t>(tlP[0] / TileSide3D);
    p0.y = static_cast<int32_t>(tlP[2] / TileSide3D);
    p1.x = static_cast<int32_t>(trP[0] / TileSide3D);
    p1.y = static_cast<int32_t>(trP[2] / TileSide3D);
    p2.x = static_cast<int32_t>(blP[0] / TileSide3D);
    p2.y = static_cast<int32_t>(blP[2] / TileSide3D);
    p3.x = static_cast<int32_t>(brP[0] / TileSide3D);
    p3.y = static_cast<int32_t>(brP[2] / TileSide3D);

    // Obtain visible tile indices in current zoom
    //TODO: it's not optimal, since dumb AABB takes a lot of unneeded tiles.
    _visibleTiles.clear();
    auto yMax = qMax(qMax(p0.y, p1.y), qMax(p2.y, p3.y));
    auto yMin = qMin(qMin(p0.y, p1.y), qMin(p2.y, p3.y));
    auto xMax = qMax(qMax(p0.x, p1.x), qMax(p2.x, p3.x));
    auto xMin = qMin(qMin(p0.x, p1.x), qMin(p2.x, p3.x));
    PointI centerZ;
    centerZ.x = _target31.x >> (31 - _zoom);
    centerZ.y = _target31.y >> (31 - _zoom);
    for(int32_t y = yMin; y <= yMax; y++)
    {
        for(int32_t x = xMin; x <= xMax; x++)
        {
            PointI tileZ;
            tileZ.x = centerZ.x + x;
            tileZ.y = centerZ.y + y;

            uint64_t tileId = (static_cast<uint64_t>(tileZ.x) << 32) | tileZ.y;
            _visibleTiles.insert(tileId);
        }
    }
    
    _tilesetCacheDirty = false;
}

void OsmAnd::Renderer_OpenGL::performRendering() const
{
    if(_viewIsDirty)
        return;

    // Setup viewport
    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    glViewport(_glViewport.left, _glWindowSize.y - _glViewport.bottom, _glViewport.width(), _glViewport.height());

    // Setup projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(glm::value_ptr(_glProjection));

    // Setup camera
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(glm::value_ptr(_glModelview));
    
    /*
    // Revert camera
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // Revert projection
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    */

    // Revert viewport
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
}

bool OsmAnd::Renderer_OpenGL::rayIntersectPlane( const glm::vec3& planeN, float planeD, const glm::vec3& rayD, const glm::vec3& rayO, float& distance )
{
    auto alpha = glm::dot(planeN, rayD);

    if(!qFuzzyCompare(alpha, 0.0f))
    {
        distance = (-glm::dot(planeN, rayO) + planeD) / alpha;

        return (distance >= 0.0f);
    }

    return false;
}

void OsmAnd::Renderer_OpenGL::refreshView()
{
    if(!_viewIsDirty)
        return;

    computeMatrices();
    refreshVisibleTileset();

    _tilesetCacheDirty = true;
    _viewIsDirty = false;
}
