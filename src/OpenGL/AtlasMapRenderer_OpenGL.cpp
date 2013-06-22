#include "AtlasMapRenderer_OpenGL.h"

#include <assert.h>

#include <glm/gtc/type_ptr.hpp>

#include <SkBitmap.h>

#include "IMapTileProvider.h"
#include "OsmAndLogging.h"

#if !defined(NDEBUG)
#   define GL_CHECK_RESULT validateResult()
#else
#   define GL_CHECK_RESULT
#endif

OsmAnd::AtlasMapRenderer_OpenGL::AtlasMapRenderer_OpenGL()
    : _tilePatchVAO(0)
    , _tilePatchVBO(0)
    , _tilePatchIBO(0)
    , _vertexShader(0)
    , _fragmentShader(0)
    , _programObject(0)
{
}

OsmAnd::AtlasMapRenderer_OpenGL::~AtlasMapRenderer_OpenGL()
{
}

void OsmAnd::AtlasMapRenderer_OpenGL::performRendering()
{
    BaseAtlasMapRenderer_OpenGL::performRendering();
    GL_CHECK_RESULT;

    if(!viewIsDirty)
        return;

    // Setup viewport
    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    GL_CHECK_RESULT;
    glViewport(
        _activeConfig.viewport.left,
        _activeConfig.windowSize.y - _activeConfig.viewport.bottom,
        _activeConfig.viewport.width(),
        _activeConfig.viewport.height());
    GL_CHECK_RESULT;

    // Activate program
    assert(glUseProgram);
    glUseProgram(_programObject);
    GL_CHECK_RESULT;

    // Set projection matrix
    glUniformMatrix4fv(_vertexShader_param_mProjection, 1, GL_FALSE, glm::value_ptr(_mProjection));
    GL_CHECK_RESULT;

    // Set view matrix
    glUniformMatrix4fv(_vertexShader_param_mView, 1, GL_FALSE, glm::value_ptr(_mView));
    GL_CHECK_RESULT;

    // Set center offset
    glUniform2f(_vertexShader_param_centerOffset, _targetInTile.x, _targetInTile.y);
    GL_CHECK_RESULT;

    // Set target tile
    PointI targetTile;
    targetTile.x = _activeConfig.target31.x >> (31 - _activeConfig.zoomBase);
    targetTile.y = _activeConfig.target31.y >> (31 - _activeConfig.zoomBase);
    glUniform2i(_vertexShader_param_targetTile, targetTile.x, targetTile.y);
    GL_CHECK_RESULT;

    // Set atlas slots per row
    auto atlastSlotsInLine = _maxTextureDimension / _activeConfig.tileProvider->getTileDimension();
    glUniform1i(_vertexShader_param_atlasSlotsInLine, atlastSlotsInLine);
    GL_CHECK_RESULT;

    // Set tile patch VAO
    assert(glBindVertexArray);
    glBindVertexArray(_tilePatchVAO);
    GL_CHECK_RESULT;

    // Set sampler index
    glUniform1i(_fragmentShader_param_sampler0, 0);

    // For each visible tile, render it
    for(auto itTileId = _visibleTiles.begin(); itTileId != _visibleTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        // Set tile id
        glUniform2i(_vertexShader_param_tile, tileId.x, tileId.y);
        GL_CHECK_RESULT;

        // Obtain tile from cache
        std::shared_ptr<TileZoomCache::Tile> cachedTile_;
        bool cacheHit;
        {
            QMutexLocker scopeLock(&_tilesCacheMutex);

            cacheHit = _tilesCache.getTile(_activeConfig.zoomBase, tileId, cachedTile_);
        }

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
                // Set atlas slot index
                glUniform1i(_vertexShader_param_atlasSlotIndex, cachedTile->atlasSlotIndex);
                GL_CHECK_RESULT;

                // Bind to texture
                glActiveTexture(GL_TEXTURE0);
                GL_CHECK_RESULT;
                glBindTexture(GL_TEXTURE_2D, cachedTile->textureId);
                GL_CHECK_RESULT;
                glBindSampler(0, _sampler0);
                GL_CHECK_RESULT;

                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
                GL_CHECK_RESULT;
            }
        }
    }

    // Deselect VAO
    glBindVertexArray(0);
    GL_CHECK_RESULT;

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    // Revert viewport
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRenderer_OpenGL::initializeRendering()
{
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

    GL_CHECK_RESULT;
    glewExperimental = GL_TRUE;
    glewInit();
    // For now, silence OpenGL error here, it's inside GLEW, so it's not ours
    (void)glGetError();
    //GL_CHECK_RESULT;

    // Compile vertex shader
    const char* const vertexShader =
        "#version 430 core                                                                                  ""\n"
        "                                                                                                   "
        // Input data
        "in vec3 in_vertexPosition;                                                                         "
        "in vec2 in_vertexUV0;                                                                              "
        "                                                                                                   "
        // Output data to next shader stages
        "out vec2 g_vertexUV0;                                                                              "
        "                                                                                                   "
        // Common data
        "uniform mat4 param_mProjection;                                                                    "
        "uniform mat4 param_mView;                                                                          "
        "uniform vec2 param_centerOffset;                                                                   "
        "uniform ivec2 param_targetTile;                                                                    "
        "uniform int param_atlasSlotsInLine;                                                                "
        "                                                                                                   "
        // Per-tile data
        "uniform ivec2 param_tile;                                                                          "
        "uniform int param_atlasSlotIndex;                                                                  "
        "                                                                                                   "
        "void main()                                                                                        "
        "{                                                                                                  "
        "    vec4 v = vec4(in_vertexPosition, 1.0);                                                         "
        "                                                                                                   "
        //   TODO: process heightmap data
        "    v.y = 0.0;                                                                                     "
        "                                                                                                   "
        //   Shift vertex to it's proper position
        "    float xOffset = float(param_tile.x - param_targetTile.x) - param_centerOffset.x;               "
        "    v.x += xOffset * 100.0;                                                                        "
        "    float yOffset = float(param_tile.y - param_targetTile.y) - param_centerOffset.y;               "
        "    v.z += yOffset * 100.0;                                                                        "
        "                                                                                                   "
        //   Calculate UV. Initially they are 0..1 as if we would process non-atlas texture
        "    g_vertexUV0 = in_vertexUV0;                                                                    "
        "    if(param_atlasSlotsInLine > 0)                                                                 "
        "    {                                                                                              "
        "        float slotLength = 1.0 / float(param_atlasSlotsInLine);                                    "
        "        float rowIndex = float(param_atlasSlotIndex / param_atlasSlotsInLine);                     "
        "        float colIndex = mod(param_atlasSlotIndex, param_atlasSlotsInLine);                        "
        "        g_vertexUV0.x = colIndex * slotLength + g_vertexUV0.x * slotLength;                        "
        "        g_vertexUV0.y = rowIndex * slotLength + g_vertexUV0.y * slotLength;                        "
        "    }                                                                                              "
        "                                                                                                   "
        "                                                                                                   "
        "                                                                                                   "
        "                                                                                                   "
        "                                                                                                   "
        "                                                                                                   "
        "    gl_Position = param_mProjection * param_mView * v;                                             "
        "}                                                                                                  ";
    _vertexShader = compileShader(GL_VERTEX_SHADER, vertexShader);
    assert(_vertexShader != 0);

    // Compile fragment shader
    const char* const fragmentShader =
        "#version 430 core                                                                                  ""\n"
        "                                                                                                   "
        // Input data
        "in vec2 g_vertexUV0;                                                                               "
        "                                                                                                   "
        // Output data
        "out vec4 out_color;                                                                                "
        "                                                                                                   "
        // Arguments
        "uniform sampler2D param_sampler0;                                                                  "
        "                                                                                                   "
        "void main()                                                                                        "
        "{                                                                                                  "
        "    out_color = texture(param_sampler0, g_vertexUV0);                                              "
        "}                                                                                                  ";
    _fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShader);
    assert(_fragmentShader != 0);

    // Link everything into program object
    assert(glCreateProgram);
    _programObject = glCreateProgram();
    GL_CHECK_RESULT;

    assert(glAttachShader);
    glAttachShader(_programObject, _vertexShader);
    GL_CHECK_RESULT;
    glAttachShader(_programObject, _fragmentShader);
    GL_CHECK_RESULT;

    assert(glLinkProgram);
    glLinkProgram(_programObject);
    GL_CHECK_RESULT;

    GLint linkSuccessful;
    assert(glGetProgramiv);
    glGetProgramiv(_programObject, GL_LINK_STATUS, &linkSuccessful);
    GL_CHECK_RESULT;
    if(!linkSuccessful && glGetShaderInfoLog != nullptr)
    {
        GLint logBufferLen = 0;	
        GLsizei logLen = 0;
        glGetProgramiv(_programObject, GL_INFO_LOG_LENGTH, &logBufferLen);       
        GL_CHECK_RESULT;
        if (logBufferLen > 1)
        {
            GLchar* log = (GLchar*)malloc(logBufferLen);
            glGetShaderInfoLog(_programObject, logBufferLen, &logLen, log);
            GL_CHECK_RESULT;
            LogPrintf(LogSeverityLevel::Error, "Failed to link GLSL shader: %s", log);
            free(log);
        }
    }

    _vertexShader_in_vertexPosition = glGetAttribLocation(_programObject, "in_vertexPosition");
    GL_CHECK_RESULT;
    assert(_vertexShader_in_vertexPosition != -1);

    _vertexShader_in_vertexUV0 = glGetAttribLocation(_programObject, "in_vertexUV0");
    GL_CHECK_RESULT;
    assert(_vertexShader_in_vertexUV0 != -1);

    _vertexShader_param_mProjection = glGetUniformLocation(_programObject, "param_mProjection");
    GL_CHECK_RESULT;
    assert(_vertexShader_param_mProjection != -1);

    _vertexShader_param_mView = glGetUniformLocation(_programObject, "param_mView");
    GL_CHECK_RESULT;
    assert(_vertexShader_param_mView != -1);

    _vertexShader_param_centerOffset = glGetUniformLocation(_programObject, "param_centerOffset");
    GL_CHECK_RESULT;
    assert(_vertexShader_param_centerOffset != -1);

    _vertexShader_param_targetTile = glGetUniformLocation(_programObject, "param_targetTile");
    GL_CHECK_RESULT;
    assert(_vertexShader_param_targetTile != -1);

    _vertexShader_param_atlasSlotsInLine = glGetUniformLocation(_programObject, "param_atlasSlotsInLine");
    GL_CHECK_RESULT;
    assert(_vertexShader_param_atlasSlotsInLine != -1);

    _vertexShader_param_tile = glGetUniformLocation(_programObject, "param_tile");
    GL_CHECK_RESULT;
    assert(_vertexShader_param_tile != -1);

    _vertexShader_param_atlasSlotIndex = glGetUniformLocation(_programObject, "param_atlasSlotIndex");
    GL_CHECK_RESULT;
    assert(_vertexShader_param_atlasSlotIndex != -1);

    _fragmentShader_param_sampler0 = glGetUniformLocation(_programObject, "param_sampler0");
    GL_CHECK_RESULT;
    assert(_fragmentShader_param_sampler0 != -1);

    glGenSamplers(1, &_sampler0);
    GL_CHECK_RESULT;
    glSamplerParameteri(_sampler0, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GL_CHECK_RESULT;
    glSamplerParameteri(_sampler0, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GL_CHECK_RESULT;
    glSamplerParameteri(_sampler0, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL_CHECK_RESULT;
    glSamplerParameteri(_sampler0, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK_RESULT;

    BaseAtlasMapRenderer_OpenGL::initializeRendering();
}

void OsmAnd::AtlasMapRenderer_OpenGL::releaseRendering()
{
    if(_programObject)
    {
        assert(glDeleteProgram);
        glDeleteProgram(_programObject);
        GL_CHECK_RESULT;
        _programObject = 0;
    }

    if(_fragmentShader)
    {
        assert(glDeleteShader);
        glDeleteShader(_fragmentShader);
        GL_CHECK_RESULT;
        _fragmentShader = 0;
    }

    if(_vertexShader)
    {
        glDeleteShader(_vertexShader);
        GL_CHECK_RESULT;
        _vertexShader = 0;
    }

    BaseAtlasMapRenderer_OpenGL::releaseRendering();
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
    if(_maxTextureDimension == 0)
    {
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&_maxTextureDimension));
        GL_CHECK_RESULT;
    }

    if(_maxTextureDimension != 0)
    {
        auto tilesPerRow = _maxTextureDimension / _activeConfig.tileProvider->getTileDimension();

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
            glTexStorage2D(GL_TEXTURE_2D, 1, _activeConfig.preferredTextureDepth == IMapRenderer::_32bits ? GL_RGBA8 : GL_RGB5, _maxTextureDimension, _maxTextureDimension);
            GL_CHECK_RESULT;
            _unfinishedAtlasFirstFreeSlot = 0;
            _texturesRefCounts.insert(textureName, 0);
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
        GL_CHECK_RESULT;
        glPixelStorei(GL_UNPACK_ROW_LENGTH, tileBitmap->rowBytesAsPixels());
        GL_CHECK_RESULT;
        auto yOffset = (freeSlotIndex / tilesPerRow) * _activeConfig.tileProvider->getTileDimension();
        auto xOffset = (freeSlotIndex % tilesPerRow) * _activeConfig.tileProvider->getTileDimension();
        glTexSubImage2D(GL_TEXTURE_2D, 0,
            xOffset, yOffset, (GLsizei)_activeConfig.tileProvider->getTileDimension(), (GLsizei)_activeConfig.tileProvider->getTileDimension(),
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
        // Fallback to 1 texture per tile
        GLuint textureName;
        glGenTextures(1, &textureName);
        GL_CHECK_RESULT;
        assert(textureName != 0);
        glBindTexture(GL_TEXTURE_2D, textureName);
        GL_CHECK_RESULT;
        glPixelStorei(GL_UNPACK_ROW_LENGTH, tileBitmap->rowBytesAsPixels());
        GL_CHECK_RESULT;
        if(_activeConfig.preferredTextureDepth == IMapRenderer::_32bits)
        {
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, _activeConfig.tileProvider->getTileDimension(), _activeConfig.tileProvider->getTileDimension());
            GL_CHECK_RESULT;
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)_activeConfig.tileProvider->getTileDimension(), (GLsizei)_activeConfig.tileProvider->getTileDimension(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, tileBitmap->getPixels());
            GL_CHECK_RESULT;
        }
        else
        {
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB5, _activeConfig.tileProvider->getTileDimension(), _activeConfig.tileProvider->getTileDimension());
            GL_CHECK_RESULT;
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)_activeConfig.tileProvider->getTileDimension(), (GLsizei)_activeConfig.tileProvider->getTileDimension(), GL_RGB, GL_UNSIGNED_SHORT_5_6_5, tileBitmap->getPixels());
            GL_CHECK_RESULT;
        }

        _tilesCache.putTile(std::shared_ptr<TileZoomCache::Tile>(static_cast<TileZoomCache::Tile*>(
            new CachedTile_OpenGL(this, zoom, tileId, 0, textureName, 0)
            )));
        _texturesRefCounts.insert(textureName, 1);
    }
}

void OsmAnd::AtlasMapRenderer_OpenGL::releaseTexture( const uint32_t& texture )
{
    glDeleteTextures(1, &texture);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRenderer_OpenGL::allocateTilePatch( Vertex* vertices, size_t verticesCount, GLushort* indices, size_t indicesCount )
{
    // Create Vertex Array Object
    assert(glGenVertexArrays);
    glGenVertexArrays(1, &_tilePatchVAO);
    GL_CHECK_RESULT;
    assert(glBindVertexArray);
    glBindVertexArray(_tilePatchVAO);
    GL_CHECK_RESULT;

    // Create vertex buffer and associate it with VAO
    assert(glGenBuffers);
    glGenBuffers(1, &_tilePatchVBO);
    GL_CHECK_RESULT;
    assert(glBindBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _tilePatchVBO);
    GL_CHECK_RESULT;
    assert(glBufferData);
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    assert(glEnableVertexAttribArray);
    glEnableVertexAttribArray(_vertexShader_in_vertexPosition);
    GL_CHECK_RESULT;
    assert(glVertexAttribPointer);
    glVertexAttribPointer(_vertexShader_in_vertexPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, position)));
    GL_CHECK_RESULT;
    assert(glEnableVertexAttribArray);
    glEnableVertexAttribArray(_vertexShader_in_vertexUV0);
    GL_CHECK_RESULT;
    assert(glVertexAttribPointer);
    glVertexAttribPointer(_vertexShader_in_vertexUV0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, uv)));
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    assert(glGenBuffers);
    glGenBuffers(1, &_tilePatchIBO);
    GL_CHECK_RESULT;
    assert(glBindBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _tilePatchIBO);
    GL_CHECK_RESULT;
    assert(glBufferData);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    glBindVertexArray(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRenderer_OpenGL::releaseTilePatch()
{
    if(_tilePatchIBO)
    {
        assert(glDeleteBuffers);
        glDeleteBuffers(1, &_tilePatchIBO);
        GL_CHECK_RESULT;
        _tilePatchIBO = 0;
    }

    if(_tilePatchVBO)
    {
        assert(glDeleteBuffers);
        glDeleteBuffers(1, &_tilePatchVBO);
        GL_CHECK_RESULT;
        _tilePatchVBO = 0;
    }

    if(_tilePatchVAO)
    {
        assert(glDeleteVertexArrays);
        glDeleteVertexArrays(1, &_tilePatchVAO);
        GL_CHECK_RESULT;
        _tilePatchVAO = 0;
    }
}

GLuint OsmAnd::AtlasMapRenderer_OpenGL::compileShader( GLenum shaderType, const char* source )
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
        if(!didCompile && glGetShaderInfoLog != nullptr)
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
    }

    return shader;
}

void OsmAnd::AtlasMapRenderer_OpenGL::validateResult()
{
    auto result = glGetError();
    if(result == GL_NO_ERROR)
        return;

    LogPrintf(LogSeverityLevel::Error, "OpenGL error 0x%08x : %s\n", result, gluErrorString(result));
}
