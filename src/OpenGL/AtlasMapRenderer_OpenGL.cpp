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

    // Compile vertex shader
    const QString vertexShader_perTileLayerTexCoordsProcessing = QString::fromLatin1(
        "    calculateTextureCoordinates(                                                                                   ""\n"
        "        param_vs_perTileLayer[%layerId%],                                                                          ""\n"
        "        v2f_texCoordsPerLayer[%layerLinearIndex%]);                                                                ""\n"
        "                                                                                                                   ""\n");
    const QString vertexShader = QString::fromLatin1(
        "#version 430 core                                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Constants
        "const float floatEpsilon = 0.000001;                                                                               ""\n"
        "                                                                                                                   ""\n"
        // Input data
        "in vec2 in_vs_vertexPosition;                                                                                      ""\n"
        "in vec2 in_vs_vertexTexCoords;                                                                                     ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "out vec2 v2f_texCoordsPerLayer[%RasterTileLayersCount%];                                                           ""\n"
        "out float v2f_distanceFromCamera;                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mProjection;                                                                                 ""\n"
        "uniform mat4 param_vs_mView;                                                                                       ""\n"
        "uniform vec2 param_vs_centerOffset;                                                                                ""\n"
        "uniform ivec2 param_vs_targetTile;                                                                                 ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-tile data
        "uniform ivec2 param_vs_tile;                                                                                       ""\n"
        "uniform float param_vs_elevationData_k;                                                                            ""\n"
        "uniform sampler2D param_vs_elevationData_sampler;                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-layer-in-tile data
        "struct LayerInputPerTile                                                                                           ""\n"
        "{                                                                                                                  ""\n"
        "    float tileSizeN;                                                                                               ""\n"
        "    float tilePaddingN;                                                                                            ""\n"
        "    int slotsPerSide;                                                                                              ""\n"
        "    int slotIndex;                                                                                                 ""\n"
        "};                                                                                                                 ""\n"
        "uniform LayerInputPerTile param_vs_perTileLayer[%TileLayersCount%];                                                ""\n"
        "                                                                                                                   ""\n"
        "void calculateTextureCoordinates(in LayerInputPerTile perTile, out vec2 outTexCoords)                              ""\n"
        "{                                                                                                                  ""\n"
        "    if(perTile.slotIndex >= 0)                                                                                     ""\n"
        "    {                                                                                                              ""\n"
        "        const int rowIndex = perTile.slotIndex / perTile.slotsPerSide;                                             ""\n"
        "        const int colIndex = int(mod(perTile.slotIndex, perTile.slotsPerSide));                                    ""\n"
        "                                                                                                                   ""\n"
        "        const float texCoordRescale = (perTile.tileSizeN - 2.0 * perTile.tilePaddingN) / perTile.tileSizeN;        ""\n"
        "                                                                                                                   ""\n"
        "        outTexCoords.s = float(colIndex) * perTile.tileSizeN;                                                      ""\n"
        "        outTexCoords.s += perTile.tilePaddingN + (in_vs_vertexTexCoords.s * perTile.tileSizeN) * texCoordRescale;  ""\n"
        "                                                                                                                   ""\n"
        "        outTexCoords.t = float(rowIndex) * perTile.tileSizeN;                                                      ""\n"
        "        outTexCoords.t += perTile.tilePaddingN + (in_vs_vertexTexCoords.t * perTile.tileSizeN) * texCoordRescale;  ""\n"
        "    }                                                                                                              ""\n"
        "    else                                                                                                           ""\n"
        "    {                                                                                                              ""\n"
        "        outTexCoords = in_vs_vertexTexCoords;                                                                      ""\n"
        "    }                                                                                                              ""\n"
        "}                                                                                                                  ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    vec4 v = vec4(in_vs_vertexPosition.x, 0.0, in_vs_vertexPosition.y, 1.0);                                       ""\n"
        "                                                                                                                   ""\n"
        //   Shift vertex to it's proper position
        "    float xOffset = float(param_vs_tile.x - param_vs_targetTile.x) - param_vs_centerOffset.x;                      ""\n"
        "    v.x += xOffset * %TileSize3D%.0;                                                                               ""\n"
        "    float yOffset = float(param_vs_tile.y - param_vs_targetTile.y) - param_vs_centerOffset.y;                      ""\n"
        "    v.z += yOffset * %TileSize3D%.0;                                                                               ""\n"
        "                                                                                                                   ""\n"
        //   Process each tile layer texture coordinates (except elevation)
        "%UnrolledPerLayerTexCoordsProcessingCode%                                                                          ""\n"
        "                                                                                                                   ""\n"
        //   If elevation data is active, use it
        "    if(abs(param_vs_elevationData_k) > floatEpsilon)                                                               ""\n"
        "    {                                                                                                              ""\n"
        "        vec2 elevationDataTexCoords;                                                                               ""\n"
        "        calculateTextureCoordinates(                                                                               ""\n"
        "            param_vs_perTileLayer[0],                                                                              ""\n"
        "            elevationDataTexCoords);                                                                               ""\n"
        "                                                                                                                   ""\n"
        "        float height = texture(param_vs_elevationData_sampler, elevationDataTexCoords).r;                          ""\n"
        //TODO: remap meters to units 
        "        v.y = height * 1.0;                                                                                            ""\n"
        "        v.y *= param_vs_elevationData_k;                                                                           ""\n"
        "    }                                                                                                              ""\n"
        "    else                                                                                                           ""\n"
        "    {                                                                                                              ""\n"
        "        v.y = 0.0;                                                                                                 ""\n"
        "    }                                                                                                              ""\n"
        "                                                                                                                   ""\n"
        //   Finally output processed modified vertex
        "    vec4 vInCamera = param_vs_mView * v;                                                                           ""\n"
        "    v2f_distanceFromCamera = -vInCamera.z;                                                                         ""\n"
        "    gl_Position = param_vs_mProjection * vInCamera;                                                                ""\n"
        "}                                                                                                                  ""\n");
    QString preprocessedVertexShader = vertexShader;
    QString preprocessedVertexShader_UnrolledPerLayerTexCoordsProcessingCode;
    for(int layerId = TileLayerId::RasterMap, linearIdx = 0; layerId < TileLayerId::IdsCount; layerId++, linearIdx++)
    {
        QString preprocessedVertexShader_perTileLayerTexCoordsProcessing = vertexShader_perTileLayerTexCoordsProcessing;
        preprocessedVertexShader_perTileLayerTexCoordsProcessing.replace("%layerId%", QString::number(layerId));
        preprocessedVertexShader_perTileLayerTexCoordsProcessing.replace("%layerLinearIndex%", QString::number(linearIdx));

        preprocessedVertexShader_UnrolledPerLayerTexCoordsProcessingCode +=
            preprocessedVertexShader_perTileLayerTexCoordsProcessing;
    }
    preprocessedVertexShader.replace("%UnrolledPerLayerTexCoordsProcessingCode%",
        preprocessedVertexShader_UnrolledPerLayerTexCoordsProcessingCode);
    preprocessedVertexShader.replace("%TileSize3D%", QString::number(TileSide3D));
    preprocessedVertexShader.replace("%TileLayersCount%", QString::number(TileLayerId::IdsCount));
    preprocessedVertexShader.replace("%RasterTileLayersCount%", QString::number(TileLayerId::IdsCount - TileLayerId::RasterMap));
    preprocessedVertexShader.replace("%Layer_ElevationData%", QString::number(TileLayerId::ElevationData));
    preprocessedVertexShader.replace("%Layer_RasterMap%", QString::number(TileLayerId::RasterMap));
    _vertexShader = compileShader(GL_VERTEX_SHADER, preprocessedVertexShader.toStdString().c_str());
    assert(_vertexShader != 0);

    // Compile fragment shader
    const QString fragmentShader_perTileLayer = QString::fromLatin1(
        "    if(param_fs_perTileLayer[%layerLinearIdx%].k > floatEpsilon)                                                   ""\n"
        "    {                                                                                                              ""\n"
        "        vec4 color = textureLod(                                                                                   ""\n"
        "            param_fs_perTileLayer[%layerLinearIdx%].sampler,                                                       ""\n"
        "            v2f_texCoordsPerLayer[%layerLinearIdx%], mipmapLod);                                                   ""\n"
        "                                                                                                                   ""\n"
        //TODO: alpha*k is the proper alpha for mixing
        "        out_color += color;                                                                                        ""\n"
        "                                                                                                                   ""\n"
        "                                                                                                                   ""\n"
        "                                                                                                                   ""\n"
        "    }                                                                                                              ""\n");
    const QString fragmentShader = QString::fromLatin1(
        "#version 430 core                                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Constants
        "const float floatEpsilon = 0.000001;                                                                               ""\n"
        "                                                                                                                   ""\n"
        // Input data
        "in vec2 v2f_texCoordsPerLayer[%RasterTileLayersCount%];                                                            ""\n"
        "in float v2f_distanceFromCamera;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Output data
        "out vec4 out_color;                                                                                                ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform float param_fs_distanceFromCameraToTarget;                                                                 ""\n"
        "uniform float param_fs_cameraElevationAngle;                                                                       ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-layer data
        "struct LayerInputPerTile                                                                                           ""\n"
        "{                                                                                                                  ""\n"
        "    float k;                                                                                                       ""\n"
        "    sampler2D sampler;                                                                                             ""\n"
        "};                                                                                                                 ""\n"
        "uniform LayerInputPerTile param_fs_perTileLayer[%RasterTileLayersCount%];                                          ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        //   Calculate normalized camera elevation and recalculate lod0 distance
        "    const float cameraElevationN = param_fs_cameraElevationAngle / 90.0;                                           ""\n"
        "    const float cameraBaseDistance = param_fs_distanceFromCameraToTarget * 1.25;                                   ""\n"
        "    const float zeroLodDistanceShift = 4.0 * (0.2 - cameraElevationN) * param_fs_distanceFromCameraToTarget;       ""\n"
        "    const float cameraDistanceLOD0 = cameraBaseDistance - zeroLodDistanceShift;                                    ""\n"
        "                                                                                                                   ""\n"
        //   Calculate mipmap LOD
        "    float mipmapLod = 0.0;                                                                                         ""\n"
        "    if(v2f_distanceFromCamera > cameraDistanceLOD0)                                                                ""\n"
        "    {                                                                                                              ""\n"
        //       Calculate distance factor that is in range (0.0 ... +inf)
        "        const float distanceF = 1.0 - cameraDistanceLOD0 / v2f_distanceFromCamera;                                 ""\n"
        "                                                                                                                   ""\n"
        "        mipmapLod = distanceF * ((1.0 - cameraElevationN) * 10.0);                                                 ""\n"
        "                                                                                                                   ""\n"
        "        mipmapLod = clamp(mipmapLod, 0.0, %MipmapLodLevelsMax%.0 - 1.0);                                           ""\n"
        "    }                                                                                                              ""\n"
        "                                                                                                                   ""\n"
        //   Take base color from RasterMap layer
        "    out_color = textureLod(                                                                                        ""\n"
        "        param_fs_perTileLayer[0].sampler,                                                                          ""\n"
        "        v2f_texCoordsPerLayer[0], mipmapLod);                                                                      ""\n"
        "    out_color.a *= param_fs_perTileLayer[0].k;                                                                     ""\n"
        "%UnrolledPerLayerProcessingCode%                                                                                   ""\n"
        "                                                                                                                   ""\n"
        //   Remove pixel if it's completely transparent
        "    if(out_color.a < floatEpsilon)                                                                                 ""\n"
        "        discard;                                                                                                   ""\n"
        "}                                                                                                                  ""\n");
    QString preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    for(int layerId = TileLayerId::MapOverlay0; layerId <= TileLayerId::MapOverlay3; layerId++)
    {
        auto linearIdx = layerId - TileLayerId::RasterMap;
        QString preprocessedFragmentShader_perTileLayer = fragmentShader_perTileLayer;
        preprocessedFragmentShader_perTileLayer.replace("%layerLinearIdx%", QString::number(linearIdx));

        preprocessedFragmentShader_UnrolledPerLayerProcessingCode += preprocessedFragmentShader_perTileLayer;
    }
    preprocessedFragmentShader.replace("%UnrolledPerLayerProcessingCode%", preprocessedFragmentShader_UnrolledPerLayerProcessingCode);
    preprocessedFragmentShader.replace("%TileLayersCount%", QString::number(TileLayerId::IdsCount));
    preprocessedFragmentShader.replace("%RasterTileLayersCount%", QString::number(TileLayerId::IdsCount - TileLayerId::RasterMap));
    preprocessedFragmentShader.replace("%Layer_RasterMap%", QString::number(TileLayerId::RasterMap));
    preprocessedFragmentShader.replace("%MipmapLodLevelsMax%", QString::number(MipmapLodLevelsMax));
    _fragmentShader = compileShader(GL_FRAGMENT_SHADER, preprocessedFragmentShader.toStdString().c_str());
    assert(_fragmentShader != 0);

    // Link everything into program object
    GLuint shaders[] = {
        _vertexShader,
        _fragmentShader
    };
    _programObject = linkProgram(2, shaders);
    assert(_programObject != 0);

    _programVariables.clear();
    findVariableLocation(_programObject, _vertexShader_in_vertexPosition, "in_vs_vertexPosition", In);
    findVariableLocation(_programObject, _vertexShader_in_vertexTexCoords, "in_vs_vertexTexCoords", In);
    findVariableLocation(_programObject, _vertexShader_param_mProjection, "param_vs_mProjection", Uniform);
    findVariableLocation(_programObject, _vertexShader_param_mView, "param_vs_mView", Uniform);
    findVariableLocation(_programObject, _vertexShader_param_centerOffset, "param_vs_centerOffset", Uniform);
    findVariableLocation(_programObject, _vertexShader_param_targetTile, "param_vs_targetTile", Uniform);
    findVariableLocation(_programObject, _vertexShader_param_tile, "param_vs_tile", Uniform);
    findVariableLocation(_programObject, _vertexShader_param_elevationData_sampler, "param_vs_elevationData_sampler", Uniform);
    findVariableLocation(_programObject, _vertexShader_param_elevationData_k, "param_vs_elevationData_k", Uniform);
    for(int layerId = 0; layerId < TileLayerId::IdsCount; layerId++)
    {
        const auto layerStructName =
            QString::fromLatin1("param_vs_perTileLayer[%layerId%]")
            .replace(QString::fromLatin1("%layerId%"), QString::number(layerId));
        auto& layerStruct = _vertexShader_param_perTileLayer[layerId];

        findVariableLocation(_programObject, layerStruct.tileSizeN, layerStructName + ".tileSizeN", Uniform);
        findVariableLocation(_programObject, layerStruct.tilePaddingN, layerStructName + ".tilePaddingN", Uniform);
        findVariableLocation(_programObject, layerStruct.slotsPerSide, layerStructName + ".slotsPerSide", Uniform);
        findVariableLocation(_programObject, layerStruct.slotIndex, layerStructName + ".slotIndex", Uniform);
    }
    findVariableLocation(_programObject, _fragmentShader_param_distanceFromCameraToTarget, "param_fs_distanceFromCameraToTarget", Uniform);
    findVariableLocation(_programObject, _fragmentShader_param_cameraElevationAngle, "param_fs_cameraElevationAngle", Uniform);
    for(int layerId = TileLayerId::RasterMap, linearIdx = 0; layerId < TileLayerId::IdsCount; layerId++, linearIdx++)
    {
        const auto layerStructName =
            QString::fromLatin1("param_fs_perTileLayer[%linearIdx%]")
            .replace(QString::fromLatin1("%linearIdx%"), QString::number(linearIdx));
        auto& layerStruct = _fragmentShader_param_perTileLayer[linearIdx];

        findVariableLocation(_programObject, layerStruct.k, layerStructName + ".k", Uniform);
        findVariableLocation(_programObject, layerStruct.sampler, layerStructName + ".sampler", Uniform);
    }
    _programVariables.clear();

    AtlasMapRenderer_BaseOpenGL::initializeRendering();
}

void OsmAnd::AtlasMapRenderer_OpenGL::performRendering()
{
    AtlasMapRenderer_BaseOpenGL::performRendering();
    GL_CHECK_RESULT;
 
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

    // Set distance to camera from target
    glUniform1f(_fragmentShader_param_distanceFromCameraToTarget, _distanceFromCameraToTarget);
    GL_CHECK_RESULT;

    // Set camera elevation angle
    glUniform1f(_fragmentShader_param_cameraElevationAngle, _activeConfig.elevationAngle);
    GL_CHECK_RESULT;
    
    // Set tile patch VAO
    assert(glBindVertexArray);
    glBindVertexArray(_tilePatchVAO);
    GL_CHECK_RESULT;

    // Set samplers
    glUniform1i(_vertexShader_param_elevationData_sampler, TileLayerId::ElevationData);
    GL_CHECK_RESULT;
    for(int layerId = TileLayerId::RasterMap; layerId < TileLayerId::IdsCount; layerId++)
    {
        glUniform1i(_fragmentShader_param_perTileLayer[layerId - TileLayerId::RasterMap].sampler, layerId);
        GL_CHECK_RESULT;
    }

    // For each visible tile, render it
    for(auto itTileId = _visibleTiles.begin(); itTileId != _visibleTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        // Set tile id
        glUniform2i(_vertexShader_param_tile, tileId.x, tileId.y);
        GL_CHECK_RESULT;

        // Set elevation data
        {
            auto& tileLayer = _tileLayers[TileLayerId::ElevationData];

            QMutexLocker scopeLock(&tileLayer._cacheModificationMutex);

            std::shared_ptr<TileZoomCache::Tile> cachedTile_;
            bool cacheHit = tileLayer._cache.getTile(_activeConfig.zoomBase, tileId, cachedTile_);
            if(cacheHit)
            {
                auto cachedTile = static_cast<CachedTile*>(cachedTile_.get());

                glUniform1f(_vertexShader_param_elevationData_k, 1.0f);
                GL_CHECK_RESULT;

                glActiveTexture(GL_TEXTURE0 + TileLayerId::ElevationData);
                GL_CHECK_RESULT;

                glEnable(GL_TEXTURE_2D);
                GL_CHECK_RESULT;

                glBindTexture(GL_TEXTURE_2D, reinterpret_cast<GLuint>(cachedTile->textureRef));
                GL_CHECK_RESULT;

                const auto& perTile_vs = _vertexShader_param_perTileLayer[TileLayerId::ElevationData];
                glUniform1i(perTile_vs.slotIndex, cachedTile->atlasSlotIndex);
                GL_CHECK_RESULT;
                if(cachedTile->atlasSlotIndex >= 0)
                {
                    const auto& atlas = tileLayer._atlasTexturePools[cachedTile->atlasPoolId];
                    glUniform1f(perTile_vs.tileSizeN, atlas._tileSizeN);
                    GL_CHECK_RESULT;
                    glUniform1f(perTile_vs.tilePaddingN, atlas._tilePaddingN);
                    GL_CHECK_RESULT;
                    glUniform1i(perTile_vs.slotsPerSide, atlas._slotsPerSide);
                    GL_CHECK_RESULT;

                    glBindSampler(TileLayerId::ElevationData, _textureSampler_ElevationData_Atlas);
                    GL_CHECK_RESULT;
                }
                else
                {
                    glBindSampler(TileLayerId::ElevationData, _textureSampler_ElevationData_NoAtlas);
                    GL_CHECK_RESULT;
                }
            }
            else
            {
                glUniform1f(_vertexShader_param_elevationData_k, 0.0f);
                GL_CHECK_RESULT;
            }
        }
        
        // We need to pass each layer of this tile to shader
        for(int layerId = TileLayerId::RasterMap; layerId < TileLayerId::IdsCount; layerId++)
        {
            auto& tileLayer = _tileLayers[layerId];
            const auto& perTile_vs = _vertexShader_param_perTileLayer[layerId];
            const auto& perTile_fs = _fragmentShader_param_perTileLayer[layerId - TileLayerId::RasterMap];

            QMutexLocker scopeLock(&tileLayer._cacheModificationMutex);

            std::shared_ptr<TileZoomCache::Tile> cachedTile_;
            bool cacheHit = tileLayer._cache.getTile(_activeConfig.zoomBase, tileId, cachedTile_);
            if(cacheHit)
            {
                auto cachedTile = static_cast<CachedTile*>(cachedTile_.get());

                if(cachedTile->textureRef == nullptr)
                {
                    //TODO: render "not available" stub
                    glUniform1f(perTile_fs.k, 0.0f);
                    GL_CHECK_RESULT;
                }
                else
                {
                    glUniform1f(perTile_fs.k, 1.0f);
                    GL_CHECK_RESULT;

                    glActiveTexture(GL_TEXTURE0 + layerId);
                    GL_CHECK_RESULT;

                    glEnable(GL_TEXTURE_2D);
                    GL_CHECK_RESULT;

                    glBindTexture(GL_TEXTURE_2D, reinterpret_cast<GLuint>(cachedTile->textureRef));
                    GL_CHECK_RESULT;

                    glUniform1i(perTile_vs.slotIndex, cachedTile->atlasSlotIndex);
                    GL_CHECK_RESULT;
                    if(cachedTile->atlasSlotIndex >= 0)
                    {
                        const auto& atlas = tileLayer._atlasTexturePools[cachedTile->atlasPoolId];

                        glUniform1f(perTile_vs.tileSizeN, atlas._tileSizeN);
                        GL_CHECK_RESULT;
                        glUniform1f(perTile_vs.tilePaddingN, atlas._tilePaddingN);
                        GL_CHECK_RESULT;
                        glUniform1i(perTile_vs.slotsPerSide, atlas._slotsPerSide);
                        GL_CHECK_RESULT;

                        glBindSampler(layerId, _textureSampler_Bitmap_Atlas);
                        GL_CHECK_RESULT;
                    }
                    else
                    {
                        glBindSampler(layerId, _textureSampler_Bitmap_NoAtlas);
                        GL_CHECK_RESULT;
                    }
                }
            }
            else
            {
                //TODO: render "in-progress" stub
                glUniform1f(perTile_fs.k, 0.0f);
                GL_CHECK_RESULT;
            }
        }

        const auto verticesCount = _activeConfig.tileProviders[TileLayerId::ElevationData]
            ? (_activeConfig.heightmapPatchesPerSide * _activeConfig.heightmapPatchesPerSide) * 4 * 3
            : 6;
        glDrawElements(GL_TRIANGLES, verticesCount, GL_UNSIGNED_SHORT, nullptr);
        GL_CHECK_RESULT;
    }

    // Disable textures
    for(int layerId = 0; layerId < TileLayerId::IdsCount; layerId++)
    {
        glActiveTexture(GL_TEXTURE0 + layerId);
        GL_CHECK_RESULT;

        glDisable(GL_TEXTURE_2D);
        GL_CHECK_RESULT;
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
    glVertexAttribPointer(_vertexShader_in_vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, position)));
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
