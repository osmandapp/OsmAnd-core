#include "MapRenderer_OpenGL.h"

#include <assert.h>

#include "IMapRenderer.h"
#include "IMapTileProvider.h"
#include "OsmAndLogging.h"

OsmAnd::MapRenderer_OpenGL::MapRenderer_OpenGL()
    : _tileTextureSampler_Atlas(0)
    , _tileTextureSampler_NoAtlas(0)
{
}

OsmAnd::MapRenderer_OpenGL::~MapRenderer_OpenGL()
{
}

void OsmAnd::MapRenderer_OpenGL::validateResult()
{
    auto result = glGetError();
    if(result == GL_NO_ERROR)
        return;

    LogPrintf(LogSeverityLevel::Error, "OpenGL error 0x%08x : %s\n", result, gluErrorString(result));
}

GLuint OsmAnd::MapRenderer_OpenGL::compileShader( GLenum shaderType, const char* source )
{
    GLuint shader;

    assert(glCreateShader);
    shader = glCreateShader(shaderType);
    GL_CHECK_RESULT;

    const GLint sourceLen = static_cast<GLint>(strlen(source));
    assert(glShaderSource);
    glShaderSource(shader, 1, &source, &sourceLen);
    GL_CHECK_RESULT;

    assert(glCompileShader);
    glCompileShader(shader);
    GL_CHECK_RESULT;

    // Check if compiled (if possible)
    if(glGetObjectParameterivARB != nullptr)
    {
        GLint didCompile;
        glGetObjectParameterivARB(shader, GL_COMPILE_STATUS, &didCompile);
        GL_CHECK_RESULT;
        if(!didCompile)
        {
            if(glGetShaderInfoLog != nullptr)
            {
                GLint logBufferLen = 0;	
                GLsizei logLen = 0;
                assert(glGetShaderiv);
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logBufferLen);       
                GL_CHECK_RESULT;
                if (logBufferLen > 1)
                {
                    GLchar* log = (GLchar*)malloc(logBufferLen);
                    glGetShaderInfoLog(shader, logBufferLen, &logLen, log);
                    GL_CHECK_RESULT;
                    LogPrintf(LogSeverityLevel::Error, "Failed to compile GLSL shader: %s", log);
                    free(log);
                }
            }

            glDeleteShader(shader);
            shader = 0;
        }
    }

    return shader;
}

void OsmAnd::MapRenderer_OpenGL::initializeRendering()
{
    MapRenderer_BaseOpenGL::initializeRendering();

    // Get OpenGL version
    const auto glVersionString = glGetString(GL_VERSION);
    GL_CHECK_RESULT;
    GLint glVersion[2];
    glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
    GL_CHECK_RESULT;
    glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "Using OpenGL version %d.%d [%s]\n", glVersion[0], glVersion[1], glVersionString);
    assert(glVersion[0] >= 3);

    // Get maximal texture size if not yet determined
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&_maxTextureSize));
    GL_CHECK_RESULT;
    if(_maxTextureSize <= _activeConfig.tileProvider->getTileSize())
        _maxTextureSize = 0;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture size %dx%d\n", _maxTextureSize, _maxTextureSize);

    GL_CHECK_RESULT;
    glewExperimental = GL_TRUE;
    glewInit();
    // For now, silence OpenGL error here, it's inside GLEW, so it's not ours
    (void)glGetError();
    //GL_CHECK_RESULT;

    glGenSamplers(1, &_tileTextureSampler_NoAtlas);
    GL_CHECK_RESULT;
    glSamplerParameteri(_tileTextureSampler_NoAtlas, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GL_CHECK_RESULT;
    glSamplerParameteri(_tileTextureSampler_NoAtlas, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GL_CHECK_RESULT;
    glSamplerParameteri(_tileTextureSampler_NoAtlas, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL_CHECK_RESULT;
    glSamplerParameteri(_tileTextureSampler_NoAtlas, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK_RESULT;

    glGenSamplers(1, &_tileTextureSampler_Atlas);
    GL_CHECK_RESULT;
    glSamplerParameteri(_tileTextureSampler_Atlas, GL_TEXTURE_WRAP_S, GL_CLAMP);
    GL_CHECK_RESULT;
    glSamplerParameteri(_tileTextureSampler_Atlas, GL_TEXTURE_WRAP_T, GL_CLAMP);
    GL_CHECK_RESULT;
    glSamplerParameteri(_tileTextureSampler_Atlas, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL_CHECK_RESULT;
    glSamplerParameteri(_tileTextureSampler_Atlas, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK_RESULT;

    glShadeModel(GL_SMOOTH); 
    GL_CHECK_RESULT;

    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    GL_CHECK_RESULT;
    glEnable(GL_POLYGON_SMOOTH);
    GL_CHECK_RESULT;

    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    GL_CHECK_RESULT;
    glEnable(GL_POINT_SMOOTH);
    GL_CHECK_RESULT;

    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    GL_CHECK_RESULT;
    glEnable(GL_LINE_SMOOTH);
    GL_CHECK_RESULT;

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    GL_CHECK_RESULT;

    glClearDepth(1.0f);
    GL_CHECK_RESULT;

    glEnable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;

    glDepthFunc(GL_LEQUAL);
    GL_CHECK_RESULT;
}

void OsmAnd::MapRenderer_OpenGL::releaseRendering()
{
    if(_tileTextureSampler_Atlas)
    {
        glDeleteSamplers(1, &_tileTextureSampler_Atlas);
        GL_CHECK_RESULT;
        _tileTextureSampler_Atlas = 0;
    }

    if(_tileTextureSampler_NoAtlas)
    {
        glDeleteSamplers(1, &_tileTextureSampler_NoAtlas);
        GL_CHECK_RESULT;
        _tileTextureSampler_NoAtlas = 0;
    }

    MapRenderer_BaseOpenGL::releaseRendering();
}

void OsmAnd::MapRenderer_OpenGL::uploadTileToTexture( const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap )
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

    const auto tileSize = _activeConfig.tileProvider->getTileSize();

    if(_maxTextureSize != 0 && _activeConfig.textureAtlasesAllowed)
    {
        const auto tilesPerRow = _maxTextureSize / (tileSize + 2 * TextureTilePixelPadding);
        _atlasSizeOnTexture = (tileSize + 2 * TextureTilePixelPadding) * tilesPerRow;

        // If we have no unfinished atlas yet, create one
        uint32_t freeSlotIndex;
        uint32_t atlasId;
        if(!_freeAtlasSlots.isEmpty())
        {
            const auto& itFreeSlot = _freeAtlasSlots.begin();
            atlasId = itFreeSlot.key();
            freeSlotIndex = itFreeSlot.value();
        }
        else if(_lastUnfinishedAtlas == 0 || _unfinishedAtlasFirstFreeSlot == tilesPerRow * tilesPerRow)
        {
            GLuint textureName;
            glGenTextures(1, &textureName);
            GL_CHECK_RESULT;
            assert(textureName != 0);
            glBindTexture(GL_TEXTURE_2D, textureName);
            GL_CHECK_RESULT;
            assert(glTexStorage2D);
            glTexStorage2D(GL_TEXTURE_2D, 1, _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_RGBA8 : GL_RGB5_A1, _maxTextureSize, _maxTextureSize);
            GL_CHECK_RESULT;
            _unfinishedAtlasFirstFreeSlot = 0;
            _texturesRefCounts.insert(textureName, 0);
            _lastUnfinishedAtlas = textureName;

#if 0
            {
                // In debug mode, fill entire texture with single RED color
                uint8_t* fillBuffer = new uint8_t[_maxTextureSize * _maxTextureSize * (_activeConfig.preferredTextureDepth == IMapRenderer::_32bits? 4 : 2)];
                for(uint32_t idx = 0; idx < _maxTextureSize * _maxTextureSize; idx++)
                {
                    if(_activeConfig.preferredTextureDepth == IMapRenderer::_32bits)
                        *reinterpret_cast<uint32_t*>(&fillBuffer[idx * 4]) = 0xFFFF0000;
                    else
                        *reinterpret_cast<uint16_t*>(&fillBuffer[idx * 2]) = 0xFC00;
                }
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0, _maxTextureSize, _maxTextureSize,
                    GL_BGRA,
                    _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_SHORT_1_5_5_5_REV,
                    fillBuffer);
                delete[] fillBuffer;
            }
#endif

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
        GL_CHECK_RESULT;

        // Tile area offset
        auto yOffset = (freeSlotIndex / tilesPerRow) * (tileSize + 2 * TextureTilePixelPadding);
        auto xOffset = (freeSlotIndex % tilesPerRow) * (tileSize + 2 * TextureTilePixelPadding);

        // Set stride
        glPixelStorei(GL_UNPACK_ROW_LENGTH, tileBitmap->rowBytesAsPixels());
        GL_CHECK_RESULT;

        //TODO: fill corners!

        // Left column duplicate
        for(int idx = 0; idx < TextureTilePixelPadding; idx++)
        {
            glTexSubImage2D(GL_TEXTURE_2D, 0,
                xOffset + idx, yOffset + TextureTilePixelPadding, 1, (GLsizei)tileSize,
                _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_BGRA : GL_RGB,
                _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_SHORT_5_6_5,
                tileBitmap->getPixels());
            GL_CHECK_RESULT;
        }
        
        // Top row duplicate
        for(int idx = 0; idx < TextureTilePixelPadding; idx++)
        {
            glTexSubImage2D(GL_TEXTURE_2D, 0,
                xOffset + TextureTilePixelPadding, yOffset + idx, (GLsizei)tileSize, 1,
                _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_BGRA : GL_RGB,
                _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_SHORT_5_6_5,
                tileBitmap->getPixels());
            GL_CHECK_RESULT;
        }

        // Right column duplicate
        for(int idx = 0; idx < TextureTilePixelPadding; idx++)
        {
            glTexSubImage2D(GL_TEXTURE_2D, 0,
                xOffset + TextureTilePixelPadding + tileSize + idx, yOffset + TextureTilePixelPadding, 1, (GLsizei)tileSize,
                _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_BGRA : GL_RGB,
                _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_SHORT_5_6_5,
                reinterpret_cast<uint8_t*>(tileBitmap->getPixels()) + (tileSize - 1) * (_activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? 4 : 2));
            GL_CHECK_RESULT;
        }

        // Bottom row duplicate
        for(int idx = 0; idx < TextureTilePixelPadding; idx++)
        {
            glTexSubImage2D(GL_TEXTURE_2D, 0,
                xOffset + TextureTilePixelPadding, yOffset + TextureTilePixelPadding + tileSize + idx, (GLsizei)tileSize, 1,
                _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_BGRA : GL_RGB,
                _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_SHORT_5_6_5,
                reinterpret_cast<uint8_t*>(tileBitmap->getPixels()) + (tileSize - 1) * tileBitmap->rowBytes());
            GL_CHECK_RESULT;
        }

        // Main data
        glTexSubImage2D(GL_TEXTURE_2D, 0,
            xOffset + TextureTilePixelPadding, yOffset + TextureTilePixelPadding, (GLsizei)tileSize, (GLsizei)tileSize,
            _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_BGRA : GL_RGB,
            _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_SHORT_5_6_5,
            tileBitmap->getPixels());
        GL_CHECK_RESULT;

        _tilesCache.putTile(std::shared_ptr<TileZoomCache::Tile>(static_cast<TileZoomCache::Tile*>(
            new CachedTile_OpenGL(this, zoom, tileId, 0, atlasId, freeSlotIndex)
            )));
        _texturesRefCounts[atlasId] += 1;
    }
    else
    {
        // Fallback to texture-per-tile mode
        GLuint textureName;
        glGenTextures(1, &textureName);
        GL_CHECK_RESULT;
        assert(textureName != 0);
        glBindTexture(GL_TEXTURE_2D, textureName);
        GL_CHECK_RESULT;
        glPixelStorei(GL_UNPACK_ROW_LENGTH, tileBitmap->rowBytesAsPixels());
        GL_CHECK_RESULT;

        glTexStorage2D(GL_TEXTURE_2D, 1,
            _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_RGBA8 : GL_RGB5_A1,
            tileSize, tileSize);
        GL_CHECK_RESULT;
        glTexSubImage2D(GL_TEXTURE_2D, 0,
            0, 0, (GLsizei)tileSize, (GLsizei)tileSize,
            _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_BGRA : GL_RGB,
            _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_SHORT_5_6_5,
            tileBitmap->getPixels());
        GL_CHECK_RESULT;

        _tilesCache.putTile(std::shared_ptr<TileZoomCache::Tile>(static_cast<TileZoomCache::Tile*>(
            new CachedTile_OpenGL(this, zoom, tileId, 0, textureName, 0)
            )));
        _texturesRefCounts.insert(textureName, 1);
    }
}

void OsmAnd::MapRenderer_OpenGL::releaseTexture( const GLuint& texture )
{
    glDeleteTextures(1, &texture);
    GL_CHECK_RESULT;
}
