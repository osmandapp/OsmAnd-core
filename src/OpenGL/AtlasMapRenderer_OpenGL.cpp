#include "AtlasMapRenderer_OpenGL.h"

#include <assert.h>

#include <glm/gtc/type_ptr.hpp>

#include <SkBitmap.h>

#include "IMapTileProvider.h"
#include "OsmAndLogging.h"

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

void OsmAnd::AtlasMapRenderer_OpenGL::initializeRendering()
{
    MapRenderer_OpenGL::initializeRendering();
    /*
    // Compile vertex shader
    QString vertexShader = QString::fromLatin1(
        "#version 430 core                                                                                                  ""\n"
        "                                                                                                                   "
        // Input data
        "in vec3 in_vertexPosition;                                                                                         "
        "in vec2 in_vertexTexCoords;                                                                                        "
        "                                                                                                                   "
        // Output data to next shader stages
        "out vec2 g_vertexUV0;                                                                                              "
        "                                                                                                                   "
        // Common data
        "uniform mat4 param_mProjection;                                                                                    "
        "uniform mat4 param_mView;                                                                                          "
        "uniform vec2 param_centerOffset;                                                                                   "
        "uniform ivec2 param_targetTile;                                                                                    "
        "                                                                                                                   "
        "uniform int param_atlasSlotsInLine;                                                                                "
        "uniform int param_atlasSize;                                                                                       "
        "uniform int param_atlasTextureSize;                                                                                "
        "                                                                                                                   "
        // Per-tile data
        "uniform ivec2 param_tile;                                                                                          "
        "uniform int param_atlasSlotIndex;                                                                                  "
        "                                                                                                                   "
        "void main()                                                                                                        "
        "{                                                                                                                  "
        "    vec4 v = vec4(in_vertexPosition, 1.0);                                                                         "
        "                                                                                                                   "
        //   Shift vertex to it's proper position
        "    float xOffset = float(param_tile.x - param_targetTile.x) - param_centerOffset.x;                               "
        "    v.x += xOffset * %TileSize3D%.0;                                                                               "
        "    float yOffset = float(param_tile.y - param_targetTile.y) - param_centerOffset.y;                               "
        "    v.z += yOffset * %TileSize3D%.0;                                                                               "
        "                                                                                                                   "
        //   Calculate UV. Initially they are 0..1 as if we would process non-atlas texture
        "    g_vertexUV0 = in_vertexTexCoords;                                                                              "
        "    if(param_atlasSlotsInLine > 0)                                                                                 "
        "    {                                                                                                              "
        "        float slotsTotalLength = float(param_atlasSize) / float(param_atlasTextureSize);                           "
        "        float slotLength = slotsTotalLength / float(param_atlasSlotsInLine);                                       "
        "        int rowIndex = param_atlasSlotIndex / param_atlasSlotsInLine;                                              "
        "        int colIndex = int(mod(param_atlasSlotIndex, param_atlasSlotsInLine));                                     "
        "                                                                                                                   "
        "        float texelSize_padding = %TextureTilePixelPadding%.0 / float(param_atlasTextureSize);                     "
        "        float texCoordRescale = (slotLength - 2.0 * texelSize_padding) / slotLength;                               "
        "                                                                                                                   "
        "        float s = float(colIndex) * slotLength;                                                                    "
        "        s += texelSize_padding + (float(g_vertexUV0.s) * slotLength) * texCoordRescale;                            "
        "        g_vertexUV0.s = s;                                                                                         "
        "                                                                                                                   "
        "        float t = float(rowIndex) * slotLength;                                                                    "
        "        t += texelSize_padding + (float(g_vertexUV0.t) * slotLength) * texCoordRescale;                            "
        "        g_vertexUV0.t = t;                                                                                         "
        "    }                                                                                                              "
        "                                                                                                                   "
        //   TODO: process heightmap data
        "    v.y = 0.0;                                                                                                     "
        "                                                                                                                   "
        //   Finally output processed modified vertex
        "    gl_Position = param_mProjection * param_mView * v;                                                             "
        "}                                                                                                                  ");
    QString preprocessedVertexShader = vertexShader;
    preprocessedVertexShader.replace("%TileSize3D%", QString::number(TileSide3D));
    preprocessedVertexShader.replace("%TextureTilePixelPadding%", QString::number(BitmapTileTexelPadding));
    _vertexShader = compileShader(GL_VERTEX_SHADER, preprocessedVertexShader.toStdString().c_str());
    assert(_vertexShader != 0);

    // Compile fragment shader
    const char* const fragmentShader =
        "#version 430 core                                                                                                  ""\n"
        "                                                                                                                   "
        // Input data
        "in vec2 g_vertexUV0;                                                                                               "
        "                                                                                                                   "
        // Output data
        "out vec4 out_color;                                                                                                "
        "                                                                                                                   "
        // Arguments
        "uniform sampler2D param_sampler0;                                                                                  "
        "                                                                                                                   "
        "void main()                                                                                                        "
        "{                                                                                                                  "
        "    out_color = texture(param_sampler0, g_vertexUV0);                                                              "
        "}                                                                                                                  ";
    _fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShader);
    assert(_fragmentShader != 0);

    // Link everything into program object
    GLuint shaders[] = {
        _vertexShader,
        _fragmentShader
    };
    _programObject = linkProgram(2, shaders);
    assert(_programObject != 0);

    _vertexShader_in_vertexPosition = glGetAttribLocation(_programObject, "in_vertexPosition");
    GL_CHECK_RESULT;
    assert(_vertexShader_in_vertexPosition != -1);

    _vertexShader_in_vertexTexCoords = glGetAttribLocation(_programObject, "in_vertexUV0");
    GL_CHECK_RESULT;
    assert(_vertexShader_in_vertexTexCoords != -1);

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

    _vertexShader_param_atlasSize = glGetUniformLocation(_programObject, "param_atlasSize");
    GL_CHECK_RESULT;
    assert(_vertexShader_param_atlasSize != -1);

    _vertexShader_param_atlasTextureSize = glGetUniformLocation(_programObject, "param_atlasTextureSize");
    GL_CHECK_RESULT;
    assert(_vertexShader_param_atlasTextureSize != -1);

    _fragmentShader_param_sampler0 = glGetUniformLocation(_programObject, "param_sampler0");
    GL_CHECK_RESULT;
    assert(_fragmentShader_param_sampler0 != -1);
    */
    AtlasMapRenderer_BaseOpenGL::initializeRendering();
}

void OsmAnd::AtlasMapRenderer_OpenGL::performRendering()
{
    AtlasMapRenderer_BaseOpenGL::performRendering();
    GL_CHECK_RESULT;
    /*
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
    glUniform2f(_vertexShader_param_centerOffset, _normalizedTargetInTileOffset.x, _normalizedTargetInTileOffset.y);
    GL_CHECK_RESULT;

    // Set target tile
    PointI targetTile;
    targetTile.x = _activeConfig.target31.x >> (31 - _activeConfig.zoomBase);
    targetTile.y = _activeConfig.target31.y >> (31 - _activeConfig.zoomBase);
    glUniform2i(_vertexShader_param_targetTile, targetTile.x, targetTile.y);
    GL_CHECK_RESULT;
    /*
    if(_activeConfig.textureAtlasesAllowed && _maxTextureSize != 0)
    {
        // Set atlas slots per row
        const auto atlasSlotsInLine = _maxTextureSize / (_activeConfig.tileProvider->getTileSize() + 2 * BitmapTileTexelPadding);
        glUniform1i(_vertexShader_param_atlasSlotsInLine, atlasSlotsInLine);
        GL_CHECK_RESULT;

        // Set atlas size
        const auto atlasSize = atlasSlotsInLine * (_activeConfig.tileProvider->getTileSize() + 2 * BitmapTileTexelPadding);
        glUniform1i(_vertexShader_param_atlasSize, atlasSize);
        GL_CHECK_RESULT;

        // Set atlas size
        glUniform1i(_vertexShader_param_atlasTextureSize, _maxTextureSize);
        GL_CHECK_RESULT;
    }
    else
    {
        // Set atlas slots per row
        glUniform1i(_vertexShader_param_atlasSlotsInLine, 0);
        GL_CHECK_RESULT;

        // Set atlas size
        glUniform1i(_vertexShader_param_atlasSize, 0);
        GL_CHECK_RESULT;

        // Set atlas size
        glUniform1i(_vertexShader_param_atlasTextureSize, 0);
        GL_CHECK_RESULT;
    }*/
    /*
    // Set tile patch VAO
    assert(glBindVertexArray);
    glBindVertexArray(_tilePatchVAO);
    GL_CHECK_RESULT;

    // Set sampler index
    glUniform1i(_fragmentShader_param_sampler0, 0);
    /*
    // For each visible tile, render it
    for(auto itTileId = _visibleTiles.begin(); itTileId != _visibleTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        // Set tile id
        glUniform2i(_vertexShader_param_tile, tileId.x, tileId.y);
        GL_CHECK_RESULT;

        // We need to pass each layer of this tile to shader
        for(int layerId = 0; layerId < TileLayerId::IdsCount; layerId++)
        {

        }


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
                glBindSampler(0, (_maxTextureSize != 0 && _activeConfig.textureAtlasesAllowed) ? _tileTextureSampler_Atlas : _tileTextureSampler_NoAtlas);
                GL_CHECK_RESULT;

                const auto verticesCount = _activeConfig.elevationDataProvider
                    ? (TileElevationNodesPerSide * TileElevationNodesPerSide) * 4 * 3
                    : 6;
                glDrawElements(GL_TRIANGLES, verticesCount, GL_UNSIGNED_SHORT, nullptr);
                GL_CHECK_RESULT;
            }
        }
    }
    *//*
    // Deselect VAO
    glBindVertexArray(0);
    GL_CHECK_RESULT;

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    // Revert viewport
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    GL_CHECK_RESULT;*/
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

    AtlasMapRenderer_BaseOpenGL::releaseRendering();
    MapRenderer_OpenGL::releaseRendering();
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
    glEnableVertexAttribArray(_vertexShader_in_vertexTexCoords);
    GL_CHECK_RESULT;
    assert(glVertexAttribPointer);
    glVertexAttribPointer(_vertexShader_in_vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, uv)));
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
