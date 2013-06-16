#include "Renderer_OpenGL.h"

#include <assert.h>
#if defined(WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glew.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QtGlobal>
#include <QtCore/qmath.h>

#include <SkBitmap.h>

OsmAnd::Renderer_OpenGL::Renderer_OpenGL()
{
}

OsmAnd::Renderer_OpenGL::~Renderer_OpenGL()
{
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
    tlP /= TileSide3D;
    trP /= TileSide3D;
    blP /= TileSide3D;
    brP /= TileSide3D;
    
    // Obtain visible tile indices in current zoom
    //TODO: it's not optimal, since dumb AABB takes a lot of unneeded tiles.
    _visibleTiles.clear();
    PointI p0, p1, p2, p3;
    p0.x = tlP[0] > 0.0f ? qCeil(tlP[0]) : qFloor(tlP[0]);
    p0.y = tlP[2] > 0.0f ? qCeil(tlP[2]) : qFloor(tlP[2]);
    p1.x = trP[0] > 0.0f ? qCeil(trP[0]) : qFloor(trP[0]);
    p1.y = trP[2] > 0.0f ? qCeil(trP[2]) : qFloor(trP[2]);
    p2.x = blP[0] > 0.0f ? qCeil(blP[0]) : qFloor(blP[0]);
    p2.y = blP[2] > 0.0f ? qCeil(blP[2]) : qFloor(blP[2]);
    p3.x = brP[0] > 0.0f ? qCeil(brP[0]) : qFloor(brP[0]);
    p3.y = brP[2] > 0.0f ? qCeil(brP[2]) : qFloor(brP[2]);
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
            if(tileZ.x < 0 || tileZ.x > (1u << _zoom) - 1)
                continue;
            tileZ.y = centerZ.y + y;
            if(tileZ.y < 0 || tileZ.y > (1u << _zoom) - 1)
                continue;

            uint64_t tileId = (static_cast<uint64_t>(tileZ.x) << 32) | tileZ.y;
            _visibleTiles.insert(tileId);
        }
    }

    // Compute in-tile offset
    auto zoomTileMask = ((1u << _zoom) - 1) << (31 - _zoom);
    auto tileXo31 = _target31.x & zoomTileMask;
    auto tileYo31 = _target31.y & zoomTileMask;
    auto div = 1u << (31 - _zoom);
    if(div > 1)
        div -= 1;
    _targetInTile.x = static_cast<double>(_target31.x - tileXo31) / div;
    _targetInTile.y = static_cast<double>(_target31.y - tileYo31) / div;
}

void OsmAnd::Renderer_OpenGL::performRendering()
{
    if(_viewIsDirty)
        return;

    assert(glGetError() == GL_NO_ERROR);

    cacheMissingTiles();

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
    
    // For each visible tile, render it
    PointI centerZ;
    centerZ.x = _target31.x >> (31 - _zoom);
    centerZ.y = _target31.y >> (31 - _zoom);
    for(auto itTileId = _visibleTiles.begin(); itTileId != _visibleTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        float x = static_cast<int32_t>(tileId >> 32) - centerZ.x;
        float y = static_cast<int32_t>(tileId & 0xFFFFFFFF) - centerZ.y;

        float tx = (x - _targetInTile.x) * TileSide3D;
        float ty = (y - _targetInTile.y) * TileSide3D;

        // Obtain tile from cache
        QMap< uint64_t, std::shared_ptr<CachedTile> >::const_iterator itCachedTile;
        bool cacheHit;
        {
            QMutexLocker scopeLock(&_tileCacheMutex);
            itCachedTile = _cachedTiles.find(tileId);
            cacheHit = (itCachedTile != _cachedTiles.end());
        }

        glPushMatrix();
        glTranslatef(tx, 0.0f, ty);
        if(!cacheHit)
        {
            //TODO: render stub
            glBegin(GL_QUADS);
                glColor3d(1,0,0);
                glVertex3f(0,0,TileSide3D);
                glColor3d(1,1,0);
                glVertex3f(TileSide3D,0,TileSide3D);
                glColor3d(1,1,1);
                glVertex3f(TileSide3D,0,0);
                glColor3d(0,1,1);
                glVertex3f(0,0,0);
            glEnd();
        }
        else
        {
            auto cachedTile = static_cast<CachedTile_OpenGL*>((*itCachedTile).get());
            //TODO: render using in-cache texture
        }
        //TODO:DRAW!
        //1. map data cache
        //2. rasterized tile textures (in gpu?)
        //3. where rasterization should take place?
        glPopMatrix();
    }

    // Revert camera
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // Revert projection
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    // Revert viewport
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);

    assert(glGetError() == GL_NO_ERROR);
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

    _viewIsDirty = false;
}

void OsmAnd::Renderer_OpenGL::cacheTile( const uint64_t& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap )
{
    auto cachedTile = new CachedTile_OpenGL();
    std::shared_ptr<IRenderer::CachedTile> cachedTile_(static_cast<IRenderer::CachedTile*>(cachedTile));

    assert(glGetError() == GL_NO_ERROR);

    GLuint textureName;
    glGenTextures(1, &textureName);
    glBindTexture(GL_TEXTURE_2D, textureName);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, tileBitmap->rowBytesAsPixels());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)tileBitmap->width(), (GLsizei)tileBitmap->height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, tileBitmap->getPixels());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    assert(glGetError() == GL_NO_ERROR);

    cachedTile->textureId = textureName;
    _cachedTiles.insert(tileId, cachedTile_);
}

OsmAnd::Renderer_OpenGL::CachedTile_OpenGL::~CachedTile_OpenGL()
{
    //TODO: remove texture from video meomry
}
