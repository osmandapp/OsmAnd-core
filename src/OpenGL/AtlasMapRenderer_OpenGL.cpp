#include "AtlasMapRenderer_OpenGL.h"

#include <assert.h>
#if defined(WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <glm/gtc/type_ptr.hpp>

#include <SkBitmap.h>

#include "IMapTileProvider.h"
#include "OsmAndLogging.h"

#if !defined(NDEBUG)
#   define OPENGL_CHECK_RESULT validateResult()
#else
#   define OPENGL_CHECK_RESULT
#endif

OsmAnd::AtlasMapRenderer_OpenGL::AtlasMapRenderer_OpenGL()
{
}

OsmAnd::AtlasMapRenderer_OpenGL::~AtlasMapRenderer_OpenGL()
{
}

void OsmAnd::AtlasMapRenderer_OpenGL::performRendering()
{
    BaseAtlasMapRenderer_OpenGL::performRendering();
    
    if(!viewIsDirty)
        return;

    // Setup viewport
    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    glViewport(_activeConfig.viewport.left, _activeConfig.windowSize.y - _activeConfig.viewport.bottom, _activeConfig.viewport.width(), _activeConfig.viewport.height());

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
    centerZ.x = _activeConfig.target31.x >> (31 - _activeConfig.zoomBase);
    centerZ.y = _activeConfig.target31.y >> (31 - _activeConfig.zoomBase);
    for(auto itTileId = _visibleTiles.begin(); itTileId != _visibleTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        float x = tileId.x - centerZ.x;
        float y = tileId.y - centerZ.y;

        float tx = (x - _targetInTile.x) * TileDimension3D;
        float ty = (y - _targetInTile.y) * TileDimension3D;

        // Obtain tile from cache
        std::shared_ptr<TileZoomCache::Tile> cachedTile_;
        bool cacheHit;
        {
            QMutexLocker scopeLock(&_tilesCacheMutex);

            cacheHit = _tilesCache.getTile(_activeConfig.zoomBase, tileId, cachedTile_);
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
            glVertex3f(0,0,TileDimension3D);

            glColor3d(1,1,0);
            glVertex3f(TileDimension3D,0,TileDimension3D);

            glColor3d(1,1,1);
            glVertex3f(TileDimension3D,0,0);

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
                glVertex3f(0,0,TileDimension3D);

                glTexCoord2f(1, 1);
                glVertex3f(TileDimension3D,0,TileDimension3D);

                glTexCoord2f(1, 0);
                glVertex3f(TileDimension3D,0,0);

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

void OsmAnd::AtlasMapRenderer_OpenGL::initializeRendering()
{
    BaseAtlasMapRenderer_OpenGL::initializeRendering();

    glewInit();
}

void OsmAnd::AtlasMapRenderer_OpenGL::uploadTileToTexture( const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap )
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
    assert( _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? skConfig == SkBitmap::kARGB_8888_Config : skConfig == SkBitmap::kRGB_565_Config );

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
            glTexStorage2D(GL_TEXTURE_2D, 1, _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_RGBA8 : GL_RGB5, _glMaxTextureDimension, _glMaxTextureDimension);
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
            _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_BGRA : GL_RGB,
            _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_SHORT_5_6_5,
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
        if(_activeConfig.preferredTextureDepth == IMapRenderer::_32bits)
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

void OsmAnd::AtlasMapRenderer_OpenGL::purgeTexture( const uint32_t& texture )
{
    glDeleteTextures(1, &texture);
    OPENGL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRenderer_OpenGL::validateResult()
{
    auto result = glGetError();
    if(result == GL_NO_ERROR)
        return;

    LogPrintf(LogSeverityLevel::Error, "OpenGL error %08x : %s\n", result, gluErrorString(result));
}
