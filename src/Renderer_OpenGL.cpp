#include "Renderer_OpenGL.h"

#include <assert.h>
#if defined(WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QtGlobal>
#include <QtCore/qmath.h>
#include <QThread>

#include <SkBitmap.h>

#include "IMapTileProvider.h"
#include "OsmAndLogging.h"

#if !defined(NDEBUG)
#   define OPENGL_CHECK_RESULT validateResult()
#else
#   define OPENGL_CHECK_RESULT
#endif

OsmAnd::Renderer_OpenGL::Renderer_OpenGL()
    : _glMaxTextureDimension(0)
    , _lastUnfinishedAtlas(0)
    , _glRenderThreadId(nullptr)
{
}

OsmAnd::Renderer_OpenGL::~Renderer_OpenGL()
{
}

void OsmAnd::Renderer_OpenGL::computeMatrices()
{
    // Setup projection
    GLfloat aspectRatio = static_cast<GLfloat>(_activeConfig.viewport.width());
    auto viewportHeight = _activeConfig.viewport.height();
    if(viewportHeight > 0)
        aspectRatio /= static_cast<GLfloat>(viewportHeight);
    _glProjection = glm::perspective(_activeConfig.fieldOfView, aspectRatio, 0.1f, _activeConfig.fogDistance * 1.2f + _activeConfig.distanceFromTarget);

    // Setup camera
    const auto& c0 = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0.0f, -_activeConfig.distanceFromTarget));
    const auto& c1 = glm::rotate(c0, _activeConfig.elevationAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    _glModelview = glm::rotate(c1, _activeConfig.azimuth, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Setup viewport and window
    _glViewport = _activeConfig.viewport;
    _glWindowSize = _activeConfig.windowSize;
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
    auto clip = _activeConfig.fogDistance * 1.2f + _activeConfig.distanceFromTarget;
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
    centerZ.x = _activeConfig.target31.x >> (31 - _activeConfig.zoom);
    centerZ.y = _activeConfig.target31.y >> (31 - _activeConfig.zoom);
    for(int32_t y = yMin; y <= yMax; y++)
    {
        for(int32_t x = xMin; x <= xMax; x++)
        {
            TileId tileId;
            tileId.x = centerZ.x + x;
            if(tileId.x < 0 || tileId.x > (1u << _activeConfig.zoom) - 1)
                continue;
            tileId.y = centerZ.y + y;
            if(tileId.y < 0 || tileId.y > (1u << _activeConfig.zoom) - 1)
                continue;

            _visibleTiles.insert(tileId);
        }
    }

    // Compute in-tile offset
    auto zoomTileMask = ((1u << _activeConfig.zoom) - 1) << (31 - _activeConfig.zoom);
    auto tileXo31 = _activeConfig.target31.x & zoomTileMask;
    auto tileYo31 = _activeConfig.target31.y & zoomTileMask;
    auto div = 1u << (31 - _activeConfig.zoom);
    if(div > 1)
        div -= 1;
    _targetInTile.x = static_cast<double>(_activeConfig.target31.x - tileXo31) / div;
    _targetInTile.y = static_cast<double>(_activeConfig.target31.y - tileYo31) / div;
}

void OsmAnd::Renderer_OpenGL::performRendering()
{
    assert(_isRenderingInitialized);

    if(_configInvalidated)
        updateConfiguration();

    if(!viewIsDirty)
        return;

    {
        QMutexLocker scopeLock(&_tilesPendingToCacheMutex);

        while(!_tilesPendingToCacheQueue.isEmpty())
        {
            const auto& pendingTile = _tilesPendingToCacheQueue.dequeue();
            _tilesPendingToCache[pendingTile.zoom].remove(pendingTile.tileId);

            // This tile may already have been loaded earlier (since it's already on disk!)
            {
                QMutexLocker scopeLock(&_tilesCacheMutex);
                if(_tilesCache.contains(pendingTile.zoom, pendingTile.tileId))
                    continue;
            }
            
            uploadTileToTexture(pendingTile.tileId, pendingTile.zoom, pendingTile.tileBitmap);
        }
    }
    updateTilesCache();

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
    centerZ.x = _activeConfig.target31.x >> (31 - _activeConfig.zoom);
    centerZ.y = _activeConfig.target31.y >> (31 - _activeConfig.zoom);
    for(auto itTileId = _visibleTiles.begin(); itTileId != _visibleTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        float x = tileId.x - centerZ.x;
        float y = tileId.y - centerZ.y;

        float tx = (x - _targetInTile.x) * TileSide3D;
        float ty = (y - _targetInTile.y) * TileSide3D;

        // Obtain tile from cache
        std::shared_ptr<TileZoomCache::Tile> cachedTile_;
        bool cacheHit;
        {
            QMutexLocker scopeLock(&_tilesCacheMutex);

            cacheHit = _tilesCache.getTile(_activeConfig.zoom, tileId, cachedTile_);
        }
        
        glPushMatrix();
        glTranslatef(tx, 0.0f, ty);
        if(!cacheHit)
        {
            //TODO: render stub
            glBegin(GL_QUADS);
            
            glColor3d(0,1,1);
            glVertex3f(0,0,0);
            
            glColor3d(1,0,0);
            glVertex3f(0,0,TileSide3D);

            glColor3d(1,1,0);
            glVertex3f(TileSide3D,0,TileSide3D);

            glColor3d(1,1,1);
            glVertex3f(TileSide3D,0,0);

            glEnd();
        }
        else
        {
            auto cachedTile = static_cast<CachedTile_OpenGL*>(cachedTile_.get());
            if(cachedTile->textureId == 0)
            {
                //TODO: render non existent tile
            }
            else
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, cachedTile->textureId);
                //////////////////////////////////////////////////////////////////////////
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//?
                //////////////////////////////////////////////////////////////////////////
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
                glEnable(GL_TEXTURE_2D);
                //////////////////////////////////////////////////////////////////////////
                glBegin(GL_QUADS);

                glTexCoord2f(0, 0);
                glVertex3f(0,0,0);

                glTexCoord2f(0, 1);
                glVertex3f(0,0,TileSide3D);

                glTexCoord2f(1, 1);
                glVertex3f(TileSide3D,0,TileSide3D);

                glTexCoord2f(1, 0);
                glVertex3f(TileSide3D,0,0);

                glEnd();
                //////////////////////////////////////////////////////////////////////////
                glDisable(GL_TEXTURE_2D);
            }
        }
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
}

void OsmAnd::Renderer_OpenGL::initializeRendering()
{
    assert(!_isRenderingInitialized);

    glewInit();
    _glRenderThreadId = QThread::currentThreadId();

    _isRenderingInitialized = true;
}

void OsmAnd::Renderer_OpenGL::releaseRendering()
{
    assert(_isRenderingInitialized);

    purgeTilesCache();

    _isRenderingInitialized = false;
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

void OsmAnd::Renderer_OpenGL::updateConfiguration()
{
    IRenderer::updateConfiguration();

    computeMatrices();
    refreshVisibleTileset();
}

void OsmAnd::Renderer_OpenGL::cacheTile( const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap )
{
    assert(!_tilesCache.contains(zoom, tileId));

    if(_glRenderThreadId != QThread::currentThreadId())
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

void OsmAnd::Renderer_OpenGL::uploadTileToTexture( const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap )
{
    assert(!_tilesCache.contains(zoom, tileId));

    LogPrintf(LogSeverityLevel::Debug, "Uploading tile %dx%d@%d as texture\n", tileId.x, tileId.y, zoom);

    if(!tileBitmap)
    {
        // Non-existent tile
        _tilesCache.putTile(std::shared_ptr<TileZoomCache::Tile>(static_cast<TileZoomCache::Tile*>(
            new CachedTile_OpenGL(this, zoom, tileId, 0, 0, 0)
            )));
        return;
    }

    const auto skConfig = tileBitmap->getConfig();
    assert( _activeConfig.preferredTextureDepth == IRenderer::_32bits ? skConfig == SkBitmap::kARGB_8888_Config : skConfig == SkBitmap::kRGB_565_Config );

    // Get maximal texture size if not yet determined
    /*if(_glMaxTextureDimension == 0)
    {
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&_glMaxTextureDimension));
        OPENGL_CHECK_RESULT;
    }*/

    if(_glMaxTextureDimension != 0)
    {
        auto tilesPerRow = _glMaxTextureDimension / _activeConfig.tileProvider->getTileDimension();

        // If we have no unfinished atlas yet, create one
        uint32_t freeSlotIndex;
        uint32_t atlasId;
        if(!_freeAtlasSlots.isEmpty())
        {
            auto slotId = _freeAtlasSlots.dequeue();
            atlasId = slotId >> 32;
            freeSlotIndex = slotId & 0xFFFFFFFF;
        }
        else if(_lastUnfinishedAtlas == 0 || _unfinishedAtlasFirstFreeSlot == tilesPerRow * tilesPerRow)
        {
            GLuint textureName;
            glGenTextures(1, &textureName);
            OPENGL_CHECK_RESULT;
            assert(textureName != 0);
            glBindTexture(GL_TEXTURE_2D, textureName);
            OPENGL_CHECK_RESULT;
            glTexStorage2D(GL_TEXTURE_2D, 1, _activeConfig.preferredTextureDepth == IRenderer::_32bits ? GL_RGBA8 : GL_RGB5, _glMaxTextureDimension, _glMaxTextureDimension);
            OPENGL_CHECK_RESULT;
            _unfinishedAtlasFirstFreeSlot = 0;
            _glTexturesRefCounts.insert(textureName, 0);
            _lastUnfinishedAtlas = textureName;

            freeSlotIndex = 0;
            atlasId = textureName;
            _unfinishedAtlasFirstFreeSlot++;
        }
        else
        {
            atlasId = _lastUnfinishedAtlas;
            freeSlotIndex = _unfinishedAtlasFirstFreeSlot;
            _unfinishedAtlasFirstFreeSlot++;
        }

        glBindTexture(GL_TEXTURE_2D, atlasId);
        OPENGL_CHECK_RESULT;
        glPixelStorei(GL_UNPACK_ROW_LENGTH, tileBitmap->rowBytesAsPixels());
        OPENGL_CHECK_RESULT;
        auto yOffset = (freeSlotIndex / tilesPerRow) * _activeConfig.tileProvider->getTileDimension();
        auto xOffset = (freeSlotIndex % tilesPerRow) * _activeConfig.tileProvider->getTileDimension();
        glTexSubImage2D(GL_TEXTURE_2D, 0,
            xOffset, yOffset, (GLsizei)_activeConfig.tileProvider->getTileDimension(), (GLsizei)_activeConfig.tileProvider->getTileDimension(),
            _activeConfig.preferredTextureDepth == IRenderer::_32bits ? GL_BGRA : GL_RGB,
            _activeConfig.preferredTextureDepth == IRenderer::_32bits ? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_SHORT_5_6_5,
            tileBitmap->getPixels());
        OPENGL_CHECK_RESULT;

        _tilesCache.putTile(std::shared_ptr<TileZoomCache::Tile>(static_cast<TileZoomCache::Tile*>(
            new CachedTile_OpenGL(this, zoom, tileId, 0, atlasId, freeSlotIndex)
            )));
        _glTexturesRefCounts[atlasId] += 1;
    }
    else
    {
        // Fallback to 1 texture per tile
        GLuint textureName;
        glGenTextures(1, &textureName);
        OPENGL_CHECK_RESULT;
        assert(textureName != 0);
        glBindTexture(GL_TEXTURE_2D, textureName);
        OPENGL_CHECK_RESULT;
        glPixelStorei(GL_UNPACK_ROW_LENGTH, tileBitmap->rowBytesAsPixels());
        OPENGL_CHECK_RESULT;
        if(_activeConfig.preferredTextureDepth == IRenderer::_32bits)
        {
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, _activeConfig.tileProvider->getTileDimension(), _activeConfig.tileProvider->getTileDimension());
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)_activeConfig.tileProvider->getTileDimension(), (GLsizei)_activeConfig.tileProvider->getTileDimension(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, tileBitmap->getPixels());
        }
        else
        {
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB5, _activeConfig.tileProvider->getTileDimension(), _activeConfig.tileProvider->getTileDimension());
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)_activeConfig.tileProvider->getTileDimension(), (GLsizei)_activeConfig.tileProvider->getTileDimension(), GL_RGB, GL_UNSIGNED_SHORT_5_6_5, tileBitmap->getPixels());
        }
        OPENGL_CHECK_RESULT;

        _tilesCache.putTile(std::shared_ptr<TileZoomCache::Tile>(static_cast<TileZoomCache::Tile*>(
            new CachedTile_OpenGL(this, zoom, tileId, 0, textureName, 0)
            )));
        _glTexturesRefCounts.insert(textureName, 1);
    }
}

int OsmAnd::Renderer_OpenGL::getCachedTilesCount()
{
    return _glTexturesRefCounts.size();
}

void OsmAnd::Renderer_OpenGL::validateResult()
{
    auto result = glGetError();
    if(result == GL_NO_ERROR)
        return;

    LogPrintf(LogSeverityLevel::Error, "OpenGL error %08x : %s\n", result, gluErrorString(result));
}

OsmAnd::Renderer_OpenGL::CachedTile_OpenGL::CachedTile_OpenGL( Renderer_OpenGL* owner_, const uint32_t& zoom, const TileId& id, const size_t& usedMemory, uint32_t textureId_, uint32_t atlasSlotIndex_ )
    : IRenderer::CachedTile(zoom, id, usedMemory)
    , owner(owner_)
    , textureId(textureId_)
    , atlasSlotIndex(atlasSlotIndex_)
{
}

OsmAnd::Renderer_OpenGL::CachedTile_OpenGL::~CachedTile_OpenGL()
{
    assert(owner->_glRenderThreadId == QThread::currentThreadId());

    const auto& itRefCnt = owner->_glTexturesRefCounts.find(textureId);
    auto& refCnt = *itRefCnt;
    refCnt -= 1;

    if(owner->_glMaxTextureDimension != 0 && refCnt > 0)
    {
        // A free atlas slot
        uint64_t slotId = (static_cast<uint64_t>(textureId) << 32) | atlasSlotIndex;
        owner->_freeAtlasSlots.enqueue(slotId);
    }

    if(refCnt == 0)
    {
        glDeleteTextures(1, &textureId);
        OPENGL_CHECK_RESULT;
        owner->_glTexturesRefCounts.remove(textureId);
    }
}
