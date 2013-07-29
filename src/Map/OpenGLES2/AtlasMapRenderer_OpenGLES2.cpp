#include "AtlasMapRenderer_OpenGLES2.h"

#include <assert.h>

#include <QtMath>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <SkBitmap.h>

#include "IMapTileProvider.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::AtlasMapRenderer_OpenGLES2::AtlasMapRenderer_OpenGLES2()
    : _tilePatchVAO(0)
    , _tilePatchVBO(0)
    , _tilePatchIBO(0)
{
    memset(&_mapStage, 0, sizeof(_mapStage));
    memset(&_skyStage, 0, sizeof(_skyStage));
}

OsmAnd::AtlasMapRenderer_OpenGLES2::~AtlasMapRenderer_OpenGLES2()
{
}

OsmAnd::RenderAPI* OsmAnd::AtlasMapRenderer_OpenGLES2::allocateRenderAPI()
{
    auto api = new RenderAPI_OpenGLES2();
    api->initialize(configuration.textureAtlasesAllowed ? OptimalTilesPerAtlasTextureSqrt : 1);
    return api;
}

bool OsmAnd::AtlasMapRenderer_OpenGLES2::doInitializeRendering()
{
    bool ok;

    ok = AtlasMapRenderer_OpenGL_Common::doInitializeRendering();
    if(!ok)
        return false;

    initializeSkyStage();
    initializeMapStage();

    return true;
}


bool OsmAnd::AtlasMapRenderer_OpenGLES2::doRenderFrame()
{
    // Setup viewport
    glViewport(
        currentState.viewport.left,
        currentState.windowSize.y - currentState.viewport.bottom,
        currentState.viewport.width(),
        currentState.viewport.height());
    GL_CHECK_RESULT;

    renderSkyStage();
    renderMapStage();

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGLES2::doReleaseRendering()
{
    bool ok;

    releaseMapStage();
    releaseSkyStage();

    ok = AtlasMapRenderer_OpenGL_Common::doReleaseRendering();
    if(!ok)
        return false;

    return true;
}

void OsmAnd::AtlasMapRenderer_OpenGLES2::initializeMapStage()
{
    auto renderAPI = getRenderAPI();

    // Compile vertex shader
    const QString vertexShader_perTileLayerTexCoordsProcessing = QString::fromLatin1(
        "    calculateTextureCoordinates(                                                                                   ""\n"
        "        param_vs_perTileLayer[%layerId%],                                                                          ""\n"
        "        v2f_texCoordsPerLayer[%layerLinearIndex%]);                                                                ""\n"
        "                                                                                                                   ""\n");
    const QString vertexShader = QString::fromLatin1(
        "#version 100                                                                                                       ""\n"
        "                                                                                                                   ""\n"
        // Set default precisions
        "precision highp float;                                                                                             ""\n"
        "precision highp int;                                                                                               ""\n"
        "precision highp sampler2D;                                                                                         ""\n"
        "                                                                                                                   ""\n"
        // Constants
        "const float floatEpsilon = 0.000001;                                                                               ""\n"
        "                                                                                                                   ""\n"
        // Input data
        "attribute vec2 in_vs_vertexPosition;                                                                               ""\n"
        "attribute vec2 in_vs_vertexTexCoords;                                                                              ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "varying vec2 v2f_texCoordsPerLayer[%RasterTileLayersCount%];                                                       ""\n"
        "#ifdef GL_EXT_shader_texture_lod                                                                                   ""\n"
        "varying float v2f_mipmapLOD;                                                                                       ""\n"
        "#endif                                                                                                             ""\n"
        "varying float v2f_distanceFromTarget;                                                                              ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mProjectionView;                                                                             ""\n"
        "uniform vec2 param_vs_targetInTilePosN;                                                                            ""\n"
        "uniform ivec2 param_vs_targetTile;                                                                                 ""\n"
        "#ifdef GL_EXT_shader_texture_lod                                                                                   ""\n"
        "uniform mat4 param_vs_mView;                                                                                       ""\n"
        "uniform float param_vs_cameraElevationAngle;                                                                       ""\n"
        "uniform float param_vs_mipmapK;                                                                                    ""\n"
        "#endif                                                                                                             ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-tile data
        "uniform ivec2 param_vs_tile;                                                                                       ""\n"
/*
        "uniform float param_vs_elevationData_k;                                                                            ""\n"
        "uniform sampler2D param_vs_elevationData_sampler;                                                                  ""\n"
        "uniform float param_vs_elevationData_upperMetersPerUnit;                                                           ""\n"
        "uniform float param_vs_elevationData_lowerMetersPerUnit;                                                           ""\n"
*/
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
        "    int rowIndex = perTile.slotIndex / perTile.slotsPerSide;                                                       ""\n"
        "    int colIndex = perTile.slotIndex - rowIndex * perTile.slotsPerSide;                                            ""\n"
        "                                                                                                                   ""\n"
        "    float texCoordRescale = (perTile.tileSizeN - 2.0 * perTile.tilePaddingN) / perTile.tileSizeN;                  ""\n"
        "                                                                                                                   ""\n"
        "    outTexCoords.s = float(colIndex) * perTile.tileSizeN;                                                          ""\n"
        "    outTexCoords.s += perTile.tilePaddingN + (in_vs_vertexTexCoords.s * perTile.tileSizeN) * texCoordRescale;      ""\n"
        "                                                                                                                   ""\n"
        "    outTexCoords.t = float(rowIndex) * perTile.tileSizeN;                                                          ""\n"
        "    outTexCoords.t += perTile.tilePaddingN + (in_vs_vertexTexCoords.t * perTile.tileSizeN) * texCoordRescale;      ""\n"
        "}                                                                                                                  ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    vec4 v = vec4(in_vs_vertexPosition.x, 0.0, in_vs_vertexPosition.y, 1.0);                                       ""\n"
        "                                                                                                                   ""\n"
        //   Shift vertex to it's proper position
        "    float xOffset = float(param_vs_tile.x - param_vs_targetTile.x) - param_vs_targetInTilePosN.x;                  ""\n"
        "    v.x += xOffset * %TileSize3D%.0;                                                                               ""\n"
        "    float yOffset = float(param_vs_tile.y - param_vs_targetTile.y) - param_vs_targetInTilePosN.y;                  ""\n"
        "    v.z += yOffset * %TileSize3D%.0;                                                                               ""\n"
        "                                                                                                                   ""\n"
        //   Process each tile layer texture coordinates (except elevation)
        "%UnrolledPerLayerTexCoordsProcessingCode%                                                                          ""\n"
        "                                                                                                                   ""\n"
        //   If elevation data is active, use it
/*
        "    if(abs(param_vs_elevationData_k) > floatEpsilon)                                                               ""\n"
        "    {                                                                                                              ""\n"
        "        float metersToUnits = mix(param_vs_elevationData_upperMetersPerUnit,                                       ""\n"
        "            param_vs_elevationData_lowerMetersPerUnit, in_vs_vertexTexCoords.t);                                   ""\n"
        "                                                                                                                   ""\n"
        //       Calculate texcoords for elevation data (pixel-is-area)
        "        vec2 elevationDataTexCoords;                                                                               ""\n"
        "        calculateTextureCoordinates(                                                                               ""\n"
        "            param_vs_perTileLayer[0],                                                                              ""\n"
        "            elevationDataTexCoords);                                                                               ""\n"
        "                                                                                                                   ""\n"
        "        float heightInMeters = texture(param_vs_elevationData_sampler, elevationDataTexCoords).r;                  ""\n"
        "        v.y = heightInMeters / metersToUnits;                                                                      ""\n"
        "        v.y *= param_vs_elevationData_k;                                                                           ""\n"
        "    }                                                                                                              ""\n"
        "    else                                                                                                           ""\n"
        "    {                                                                                                              ""\n"
        "        v.y = 0.0;                                                                                                 ""\n"
        "    }                                                                                                              ""\n"
*/
        "                                                                                                                   ""\n"
        //   Calculate mipmap LOD
        "#ifdef GL_EXT_shader_texture_lod                                                                                   ""\n"
        "    float distanceFromVertexToCamera = length( (param_vs_mView * v).xz );                                          ""\n"
        "    float mipmapThresholdDistance = param_vs_mipmapK * log(param_vs_cameraElevationAngle + 1.0);                   ""\n"
        "    v2f_mipmapLOD = distanceFromVertexToCamera / mipmapThresholdDistance - 1.0;                                    ""\n"
        "    v2f_mipmapLOD = clamp(v2f_mipmapLOD, 0.0, %MipmapLodLevelsMax%.0 - 1.0);                                       ""\n"
        "#endif                                                                                                             ""\n"
        "                                                                                                                   ""\n"
        "    v2f_distanceFromTarget = length(v.xz);                                                                         ""\n"
        "                                                                                                                   ""\n"
        //   Finally output processed modified vertex
        "    gl_Position = param_vs_mProjectionView * v;                                                                    ""\n"
        "}                                                                                                                  ""\n");
    QString preprocessedVertexShader = vertexShader;
    QString preprocessedVertexShader_UnrolledPerLayerTexCoordsProcessingCode;
    for(int layerId = MapTileLayerId::RasterMap, linearIdx = 0; layerId < MapTileLayerIdsCount; layerId++, linearIdx++)
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
    preprocessedVertexShader.replace("%TileLayersCount%", QString::number(MapTileLayerIdsCount));
    preprocessedVertexShader.replace("%RasterTileLayersCount%", QString::number(MapTileLayerIdsCount - MapTileLayerId::RasterMap));
    preprocessedVertexShader.replace("%Layer_ElevationData%", QString::number(MapTileLayerId::ElevationData));
    preprocessedVertexShader.replace("%Layer_RasterMap%", QString::number(MapTileLayerId::RasterMap));
    preprocessedVertexShader.replace("%MipmapLodLevelsMax%", QString::number(RenderAPI_OpenGLES2::MipmapLodLevelsMax));
    _mapStage.vs.id = renderAPI->compileShader(GL_VERTEX_SHADER, preprocessedVertexShader.toStdString().c_str());
    assert(_mapStage.vs.id != 0);

    // Compile fragment shader
    const QString fragmentShader_perTileLayer = QString::fromLatin1(
        "    if(param_fs_perTileLayer[%layerLinearIdx%].k > floatEpsilon)                                                   ""\n"
        "    {                                                                                                              ""\n"
        "#ifdef GL_EXT_shader_texture_lod                                                                                   ""\n"
        "        vec4 layerColor = texture2DLodEXT(                                                                         ""\n"
        "            param_fs_perTileLayer[%layerLinearIdx%].sampler,                                                       ""\n"
        "            v2f_texCoordsPerLayer[%layerLinearIdx%], v2f_mipmapLOD);                                               ""\n"
        "#else                                                                                                              ""\n"
        "        vec4 layerColor = texture2D(                                                                               ""\n"
        "            param_fs_perTileLayer[%layerLinearIdx%].sampler,                                                       ""\n"
        "            v2f_texCoordsPerLayer[%layerLinearIdx%]);                                                              ""\n"
        "#endif                                                                                                             ""\n"
        "                                                                                                                   ""\n"
        "        baseColor = mix(baseColor, layerColor, layerColor.a * param_fs_perTileLayer[%layerLinearIdx%].k);          ""\n"
        "    }                                                                                                              ""\n");
    const QString fragmentShader = QString::fromLatin1(
        "#version 100                                                                                                       ""\n"
        "                                                                                                                   ""\n"
        "#ifdef GL_EXT_shader_texture_lod                                                                                   ""\n"
        "    #extension GL_EXT_shader_texture_lod : enable                                                                  ""\n"
        "#endif                                                                                                             ""\n"
        "                                                                                                                   ""\n"
        // Set default precisions
        "precision highp float;                                                                                             ""\n"
        "precision highp int;                                                                                               ""\n"
        "precision highp sampler2D;                                                                                         ""\n"
        "                                                                                                                   ""\n"
        // Constants
        "const float floatEpsilon = 0.000001;                                                                               ""\n"
        "                                                                                                                   ""\n"
        // Input data
        "varying vec2 v2f_texCoordsPerLayer[%RasterTileLayersCount%];                                                       ""\n"
        "varying float v2f_distanceFromTarget;                                                                              ""\n"
        "#ifdef GL_EXT_shader_texture_lod                                                                                   ""\n"
        "varying float v2f_mipmapLOD;                                                                                       ""\n"
        "#endif                                                                                                             ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "#ifdef GL_EXT_shader_texture_lod                                                                                   ""\n"
        "uniform float param_fs_distanceFromCameraToTarget;                                                                 ""\n"
        "uniform float param_fs_cameraElevationAngle;                                                                       ""\n"
        "#endif                                                                                                             ""\n"
        "uniform vec3 param_fs_fogColor;                                                                                    ""\n"
        "uniform float param_fs_fogDistance;                                                                                ""\n"
        "uniform float param_fs_fogDensity;                                                                                 ""\n"
        "uniform float param_fs_fogOriginFactor;                                                                            ""\n"
        "uniform float param_fs_scaleToRetainProjectedSize;                                                                 ""\n"
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
        //   Mix colors of all layers
        "#ifdef GL_EXT_shader_texture_lod                                                                                   ""\n"
        "    vec4 baseColor = texture2DLodEXT(                                                                              ""\n"
        "        param_fs_perTileLayer[0].sampler,                                                                          ""\n"
        "        v2f_texCoordsPerLayer[0], v2f_mipmapLOD);                                                                  ""\n"
        "#else                                                                                                              ""\n"
        "    vec4 baseColor = texture2D(                                                                                    ""\n"
        "        param_fs_perTileLayer[0].sampler,                                                                          ""\n"
        "        v2f_texCoordsPerLayer[0]);                                                                                 ""\n"
        "#endif                                                                                                             ""\n"
        "    baseColor.a *= param_fs_perTileLayer[0].k;                                                                     ""\n"
        "%UnrolledPerLayerProcessingCode%                                                                                   ""\n"
        "                                                                                                                   ""\n"
        //   Apply fog (square exponential)
        "    float fogDistanceScaled = param_fs_fogDistance * param_fs_scaleToRetainProjectedSize;                          ""\n"
        "    float fogStartDistance = fogDistanceScaled * (1.0 - param_fs_fogOriginFactor);                                 ""\n"
        //TODO: take into account that v2f_positionRelativeToTarget also makes fog in reverse area
        "    float fogLinearFactor = min(max(v2f_distanceFromTarget - fogStartDistance, 0.0) /                              ""\n"
        "        (fogDistanceScaled - fogStartDistance), 1.0);                                                              ""\n"

        "    float fogFactorBase = fogLinearFactor * param_fs_fogDensity;                                                   ""\n"
        "    float fogFactor = clamp(exp(-fogFactorBase*fogFactorBase), 0.0, 1.0);                                          ""\n"
        "    gl_FragColor = mix(baseColor, vec4(param_fs_fogColor, 1.0), 1.0 - fogFactor);                                  ""\n"
        "}                                                                                                                  ""\n");
    QString preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    for(int layerId = MapTileLayerId::MapOverlay0; layerId <= MapTileLayerId::MapOverlay3; layerId++)
    {
        auto linearIdx = layerId - MapTileLayerId::RasterMap;
        QString preprocessedFragmentShader_perTileLayer = fragmentShader_perTileLayer;
        preprocessedFragmentShader_perTileLayer.replace("%layerLinearIdx%", QString::number(linearIdx));

        preprocessedFragmentShader_UnrolledPerLayerProcessingCode += preprocessedFragmentShader_perTileLayer;
    }
    preprocessedFragmentShader.replace("%UnrolledPerLayerProcessingCode%", preprocessedFragmentShader_UnrolledPerLayerProcessingCode);
    preprocessedFragmentShader.replace("%TileLayersCount%", QString::number(MapTileLayerIdsCount));
    preprocessedFragmentShader.replace("%RasterTileLayersCount%", QString::number(MapTileLayerIdsCount - MapTileLayerId::RasterMap));
    preprocessedFragmentShader.replace("%Layer_RasterMap%", QString::number(MapTileLayerId::RasterMap));
    preprocessedFragmentShader.replace("%MipmapLodLevelsMax%", QString::number(RenderAPI_OpenGLES2::MipmapLodLevelsMax));
    _mapStage.fs.id = renderAPI->compileShader(GL_FRAGMENT_SHADER, preprocessedFragmentShader.toStdString().c_str());
    assert(_mapStage.fs.id != 0);

    // Link everything into program object
    GLuint shaders[] = {
        _mapStage.vs.id,
        _mapStage.fs.id
    };
    _mapStage.program = renderAPI->linkProgram(2, shaders);
    assert(_mapStage.program != 0);

    renderAPI->clearVariablesLookup();
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.in.vertexPosition, "in_vs_vertexPosition", RenderAPI_OpenGLES2::In);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.in.vertexTexCoords, "in_vs_vertexTexCoords", RenderAPI_OpenGLES2::In);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.mProjectionView, "param_vs_mProjectionView", RenderAPI_OpenGLES2::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.targetInTilePosN, "param_vs_targetInTilePosN", RenderAPI_OpenGLES2::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.targetTile, "param_vs_targetTile", RenderAPI_OpenGLES2::Uniform);
    if(renderAPI->isSupported_EXT_shader_texture_lod)
    {
        renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.mView, "param_vs_mView", RenderAPI_OpenGLES2::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.cameraElevationAngle, "param_vs_cameraElevationAngle", RenderAPI_OpenGLES2::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.mipmapK, "param_vs_mipmapK", RenderAPI_OpenGLES2::Uniform);
    }
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.tile, "param_vs_tile", RenderAPI_OpenGLES2::Uniform);
    if(renderAPI->isSupported_vertexShaderTextureLookup)
    {
        renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.elevationData_sampler, "param_vs_elevationData_sampler", RenderAPI_OpenGLES2::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.elevationData_k, "param_vs_elevationData_k", RenderAPI_OpenGLES2::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.elevationData_upperMetersPerUnit, "param_vs_elevationData_upperMetersPerUnit", RenderAPI_OpenGLES2::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.elevationData_lowerMetersPerUnit, "param_vs_elevationData_lowerMetersPerUnit", RenderAPI_OpenGLES2::Uniform);
    }
    for(int layerId = 0; layerId < MapTileLayerIdsCount; layerId++)
    {
        const auto layerStructName =
            QString::fromLatin1("param_vs_perTileLayer[%layerId%]")
            .replace(QString::fromLatin1("%layerId%"), QString::number(layerId));
        auto& layerStruct = _mapStage.vs.param.perTileLayer[layerId];

        renderAPI->findVariableLocation(_mapStage.program, layerStruct.tileSizeN, layerStructName + ".tileSizeN", RenderAPI_OpenGLES2::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, layerStruct.tilePaddingN, layerStructName + ".tilePaddingN", RenderAPI_OpenGLES2::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, layerStruct.slotsPerSide, layerStructName + ".slotsPerSide", RenderAPI_OpenGLES2::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, layerStruct.slotIndex, layerStructName + ".slotIndex", RenderAPI_OpenGLES2::Uniform);
    }
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.fs.param.fogColor, "param_fs_fogColor", RenderAPI_OpenGLES2::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.fs.param.fogDistance, "param_fs_fogDistance", RenderAPI_OpenGLES2::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.fs.param.fogDensity, "param_fs_fogDensity", RenderAPI_OpenGLES2::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.fs.param.fogOriginFactor, "param_fs_fogOriginFactor", RenderAPI_OpenGLES2::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.fs.param.scaleToRetainProjectedSize, "param_fs_scaleToRetainProjectedSize", RenderAPI_OpenGLES2::Uniform);
    for(int layerId = MapTileLayerId::RasterMap, linearIdx = 0; layerId < MapTileLayerIdsCount; layerId++, linearIdx++)
    {
        const auto layerStructName =
            QString::fromLatin1("param_fs_perTileLayer[%linearIdx%]")
            .replace(QString::fromLatin1("%linearIdx%"), QString::number(linearIdx));
        auto& layerStruct = _mapStage.fs.param.perTileLayer[linearIdx];

        renderAPI->findVariableLocation(_mapStage.program, layerStruct.k, layerStructName + ".k", RenderAPI_OpenGLES2::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, layerStruct.sampler, layerStructName + ".sampler", RenderAPI_OpenGLES2::Uniform);
    }
    renderAPI->clearVariablesLookup();
}

void OsmAnd::AtlasMapRenderer_OpenGLES2::renderMapStage()
{
    auto renderAPI = static_cast<RenderAPI_OpenGLES2*>(getRenderAPI());

    GL_CHECK_PRESENT(glBindVertexArrayOES);
    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glUniform1i);
    GL_CHECK_PRESENT(glUniform2i);
    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glTexParameteri);

    // Set tile patch VAO
    glBindVertexArrayOES(_tilePatchVAO);
    GL_CHECK_RESULT;

    // Activate program
    glUseProgram(_mapStage.program);
    GL_CHECK_RESULT;

    // Set matrices
    auto mProjectionView = _mProjection * _mView;
    glUniformMatrix4fv(_mapStage.vs.param.mProjectionView, 1, GL_FALSE, glm::value_ptr(mProjectionView));
    GL_CHECK_RESULT;
    
    // Set center offset
    glUniform2f(_mapStage.vs.param.targetInTilePosN, _targetInTileOffsetN.x, _targetInTileOffsetN.y);
    GL_CHECK_RESULT;

    // Set target tile
    glUniform2i(_mapStage.vs.param.targetTile, _targetTileId.x, _targetTileId.y);
    GL_CHECK_RESULT;

    if(renderAPI->isSupported_EXT_shader_texture_lod)
    {
        glUniformMatrix4fv(_mapStage.vs.param.mView, 1, GL_FALSE, glm::value_ptr(_mView));
        GL_CHECK_RESULT;

        // Set camera elevation angle
        glUniform1f(_mapStage.vs.param.cameraElevationAngle, _activeConfig.elevationAngle);
        GL_CHECK_RESULT;

        // Set mipmap K factor
        glUniform1f(_mapStage.vs.param.mipmapK, _mipmapK);
    }
    
    // Set fog parameters
    glUniform3f(_mapStage.fs.param.fogColor, _activeConfig.fogColor[0], _activeConfig.fogColor[1], _activeConfig.fogColor[2]);
    GL_CHECK_RESULT;
    glUniform1f(_mapStage.fs.param.fogDistance, _activeConfig.fogDistance);
    GL_CHECK_RESULT;
    glUniform1f(_mapStage.fs.param.fogDensity, _activeConfig.fogDensity);
    GL_CHECK_RESULT;
    glUniform1f(_mapStage.fs.param.fogOriginFactor, _activeConfig.fogOriginFactor);
    GL_CHECK_RESULT;
    glUniform1f(_mapStage.fs.param.scaleToRetainProjectedSize, _scaleToRetainProjectedSize);
    GL_CHECK_RESULT;

    // Set samplers
    if(renderAPI->isSupported_vertexShaderTextureLookup)
    {
        glUniform1i(_mapStage.vs.param.elevationData_sampler, MapTileLayerId::ElevationData);
        GL_CHECK_RESULT;
    }
    for(int layerId = MapTileLayerId::RasterMap; layerId < MapTileLayerIdsCount; layerId++)
    {
        glUniform1i(_mapStage.fs.param.perTileLayer[layerId - MapTileLayerId::RasterMap].sampler, layerId);
        GL_CHECK_RESULT;
    }

    // Check if we need to process elevation data
    const bool elevationDataEnabled = renderAPI->isSupported_vertexShaderTextureLookup && currentState.tileProviders[MapTileLayerId::ElevationData];
    if(!elevationDataEnabled)
    {
        // We have no elevation data provider, so we can not do anything
        glUniform1f(_mapStage.vs.param.elevationData_k, 0.0f);
        GL_CHECK_RESULT;
    }

    // For each visible tile, render it
    for(auto itTileId = _visibleTiles.begin(); itTileId != _visibleTiles.end(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        // Get normalized tile index
        auto tileIdN = Utilities::normalizeTileId(tileId, currentState.zoomBase);

        // Set tile id
        glUniform2i(_mapStage.vs.param.tile, tileId.x, tileId.y);
        GL_CHECK_RESULT;

        // Set elevation data
        if(elevationDataEnabled)
        {
            const auto& layer = layers[MapTileLayerId::ElevationData];

            // We're obtaining tile entry by normalized tile coordinates, since tile may repeat several times
            const auto& tileEntry = layer->obtainTileEntry(tileIdN, currentState.zoomBase);

            // Try lock tile entry for reading state and obtaining GPU resource
            std::shared_ptr< RenderAPI::ResourceInGPU > gpuResource;
            if(tileEntry && tileEntry->stateLock.tryLockForRead())
            {
                if(tileEntry->state == MapRendererTileLayer::TileEntry::State::Uploaded)
                    gpuResource = tileEntry->resourceInGPU;
                tileEntry->stateLock.unlock();
            }

            if(!gpuResource)
            {
                // We have no elevation data, so we can not do anything
                glUniform1f(_mapStage.vs.param.elevationData_k, 0.0f);
                GL_CHECK_RESULT;
            }
            else
            {
                glUniform1f(_mapStage.vs.param.elevationData_k, currentState.heightScaleFactor);
                GL_CHECK_RESULT;

                auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(currentState.zoomBase, tileIdN.y, TileSide3D);
                glUniform1f(_mapStage.vs.param.elevationData_upperMetersPerUnit, upperMetersPerUnit);
                auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(currentState.zoomBase, tileIdN.y + 1, TileSide3D);
                glUniform1f(_mapStage.vs.param.elevationData_lowerMetersPerUnit, lowerMetersPerUnit);

                glActiveTexture(GL_TEXTURE0 + MapTileLayerId::ElevationData);
                GL_CHECK_RESULT;

                glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
                GL_CHECK_RESULT;

                const auto& perTile_vs = _mapStage.vs.param.perTileLayer[MapTileLayerId::ElevationData];
                if(gpuResource->type == RenderAPI::ResourceInGPU::TileOnAtlasTexture)
                {
                    const auto& tileOnAtlasTexture = static_cast<RenderAPI::TileOnAtlasTextureInGPU*>(gpuResource.get());

                    glUniform1i(perTile_vs.slotIndex, tileOnAtlasTexture->slotIndex);
                    GL_CHECK_RESULT;
                    glUniform1f(perTile_vs.tileSizeN, tileOnAtlasTexture->atlasTexture->tileSizeN);
                    GL_CHECK_RESULT;
                    glUniform1f(perTile_vs.tilePaddingN, tileOnAtlasTexture->atlasTexture->halfTexelSizeN);
                    GL_CHECK_RESULT;
                    glUniform1i(perTile_vs.slotsPerSide, tileOnAtlasTexture->atlasTexture->slotsPerSide);
                    GL_CHECK_RESULT;

                    // ElevationData (Atlas)
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, /*GL_CLAMP*/GL_CLAMP_TO_EDGE);
                    GL_CHECK_RESULT;
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, /*GL_CLAMP*/GL_CLAMP_TO_EDGE);
                    GL_CHECK_RESULT;
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    GL_CHECK_RESULT;
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    GL_CHECK_RESULT;
                }
                else
                {
                    const auto& texture = static_cast<RenderAPI::TextureInGPU*>(gpuResource.get());

                    glUniform1i(perTile_vs.slotIndex, -1);
                    GL_CHECK_RESULT;
                    glUniform1f(perTile_vs.tileSizeN, 1.0f);
                    GL_CHECK_RESULT;
                    glUniform1f(perTile_vs.tilePaddingN, texture->halfTexelSizeN);
                    GL_CHECK_RESULT;
                    glUniform1i(perTile_vs.slotsPerSide, 1);
                    GL_CHECK_RESULT;

                    // ElevationData (No atlas)
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    GL_CHECK_RESULT;
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    GL_CHECK_RESULT;
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    GL_CHECK_RESULT;
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    GL_CHECK_RESULT;
                }
            }
        }

        // We need to pass each layer of this tile to shader
        for(int layerId = MapTileLayerId::RasterMap; layerId < MapTileLayerIdsCount; layerId++)
        {
            if(!currentState.tileProviders[layerId])
                continue;

            const auto& layer = layers[layerId];
            const auto& perTile_vs = _mapStage.vs.param.perTileLayer[layerId];
            const auto& perTile_fs = _mapStage.fs.param.perTileLayer[layerId - MapTileLayerId::RasterMap];

            // We're obtaining tile entry by normalized tile coordinates, since tile may repeat several times
            const auto& tileEntry = layer->obtainTileEntry(tileIdN, currentState.zoomBase);

            // Try lock tile entry for reading state and obtaining GPU resource
            std::shared_ptr< RenderAPI::ResourceInGPU > gpuResource;
            if(tileEntry && tileEntry->stateLock.tryLockForRead())
            {
                if(tileEntry->state == MapRendererTileLayer::TileEntry::State::Uploaded)
                    gpuResource = tileEntry->resourceInGPU;
                else if(tileEntry->state == MapRendererTileLayer::TileEntry::State::Unavailable)
                    gpuResource.reset();//TODO: use texture that indicates "NO DATA"
                else
                    gpuResource.reset();//TODO: use texture that indicates "PROCESSING"

                tileEntry->stateLock.unlock();
            }

            if(!gpuResource)
            {
                glUniform1f(perTile_fs.k, 0.0f);//TODO: layer transparency
                GL_CHECK_RESULT;
                continue;
            }

            glUniform1f(perTile_fs.k, 1.0f);//TODO: layer transparency
            GL_CHECK_RESULT;

            glActiveTexture(GL_TEXTURE0 + layerId);
            GL_CHECK_RESULT;

            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
            GL_CHECK_RESULT;

            if(gpuResource->type == RenderAPI::ResourceInGPU::TileOnAtlasTexture)
            {
                const auto& tileOnAtlasTexture = static_cast<RenderAPI::TileOnAtlasTextureInGPU*>(gpuResource.get());

                glUniform1i(perTile_vs.slotIndex, tileOnAtlasTexture->slotIndex);
                GL_CHECK_RESULT;
                glUniform1f(perTile_vs.tileSizeN, tileOnAtlasTexture->atlasTexture->tileSizeN);
                GL_CHECK_RESULT;
                glUniform1f(perTile_vs.tilePaddingN, tileOnAtlasTexture->atlasTexture->tilePaddingN);
                GL_CHECK_RESULT;
                glUniform1i(perTile_vs.slotsPerSide, tileOnAtlasTexture->atlasTexture->slotsPerSide);
                GL_CHECK_RESULT;

                // Bitmap (Atlas)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, /*GL_CLAMP*/GL_CLAMP_TO_EDGE);
                GL_CHECK_RESULT;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, /*GL_CLAMP*/GL_CLAMP_TO_EDGE);
                GL_CHECK_RESULT;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                GL_CHECK_RESULT;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                GL_CHECK_RESULT;
            }
            else
            {
                glUniform1i(perTile_vs.slotIndex, -1);
                GL_CHECK_RESULT;
                glUniform1f(perTile_vs.tileSizeN, 1.0f);
                GL_CHECK_RESULT;
                glUniform1f(perTile_vs.tilePaddingN, 0.0f);
                GL_CHECK_RESULT;
                glUniform1i(perTile_vs.slotsPerSide, 1);
                GL_CHECK_RESULT;

                // Bitmap (No atlas)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                GL_CHECK_RESULT;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                GL_CHECK_RESULT;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                GL_CHECK_RESULT;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                GL_CHECK_RESULT;
            }
        }

        glDrawElements(GL_TRIANGLES, _tilePatchIndicesCount, GL_UNSIGNED_SHORT, nullptr);
        GL_CHECK_RESULT;
    }

    // Disable textures
    for(int layerId = 0; layerId < TileLayerId::IdsCount; layerId++)
    {
        glActiveTexture(GL_TEXTURE0 + layerId);
        GL_CHECK_RESULT;

        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_RESULT;
    }

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    // Deselect VAO
    glBindVertexArrayOES(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRenderer_OpenGLES2::releaseMapStage()
{
    GL_CHECK_PRESENT(glDeleteProgram);
    GL_CHECK_PRESENT(glDeleteShader);

    if(_mapStage.program)
    {
        glDeleteProgram(_mapStage.program);
        GL_CHECK_RESULT;
    }
    if(_mapStage.fs.id)
    {
        glDeleteShader(_mapStage.fs.id);
        GL_CHECK_RESULT;
    }
    if(_mapStage.vs.id)
    {
        glDeleteShader(_mapStage.vs.id);
        GL_CHECK_RESULT;
    }
    memset(&_mapStage, 0, sizeof(_mapStage));
}

void OsmAnd::AtlasMapRenderer_OpenGLES2::allocateTilePatch( MapTileVertex* vertices, GLsizei verticesCount, GLushort* indices, GLsizei indicesCount )
{
    GL_CHECK_PRESENT(glGenVertexArraysOES);
    GL_CHECK_PRESENT(glBindVertexArrayOES);
    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    // Create Vertex Array Object
    glGenVertexArraysOES(1, &_tilePatchVAO);
    GL_CHECK_RESULT;
    glBindVertexArrayOES(_tilePatchVAO);
    GL_CHECK_RESULT;

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_tilePatchVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _tilePatchVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(MapTileVertex), vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(_mapStage.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(_mapStage.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(MapTileVertex), reinterpret_cast<GLvoid*>(offsetof(MapTileVertex, position)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(_mapStage.vs.in.vertexTexCoords);
    GL_CHECK_RESULT;
    glVertexAttribPointer(_mapStage.vs.in.vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(MapTileVertex), reinterpret_cast<GLvoid*>(offsetof(MapTileVertex, uv)));
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_tilePatchIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _tilePatchIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    glBindVertexArrayOES(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRenderer_OpenGLES2::releaseTilePatch()
{
    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteVertexArraysOES);

    if(_tilePatchIBO)
    {
        glDeleteBuffers(1, &_tilePatchIBO);
        GL_CHECK_RESULT;
        _tilePatchIBO = 0;
    }

    if(_tilePatchVBO)
    {
        glDeleteBuffers(1, &_tilePatchVBO);
        GL_CHECK_RESULT;
        _tilePatchVBO = 0;
    }

    if(_tilePatchVAO)
    {
        glDeleteVertexArraysOES(1, &_tilePatchVAO);
        GL_CHECK_RESULT;
        _tilePatchVAO = 0;
    }
}

void OsmAnd::AtlasMapRenderer_OpenGLES2::initializeSkyStage()
{
    auto renderAPI = getRenderAPI();

    GL_CHECK_PRESENT(glGenVertexArraysOES);
    GL_CHECK_PRESENT(glBindVertexArrayOES);
    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    // Vertex data (x,y)
    float vertices[4][2] =
    {
        {-1.0f,-1.0f},
        {-1.0f, 1.0f},
        { 1.0f, 1.0f},
        { 1.0f,-1.0f}
    };
    const auto verticesCount = 4;

    // Index data
    GLushort indices[6] =
    {
        0, 1, 2,
        0, 2, 3
    };
    const auto indicesCount = 6;

    // Create Vertex Array Object
    glGenVertexArraysOES(1, &_skyStage.vao);
    GL_CHECK_RESULT;
    glBindVertexArrayOES(_skyStage.vao);
    GL_CHECK_RESULT;

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_skyStage.vbo);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _skyStage.vbo);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float) * 2, vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(_skyStage.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(_skyStage.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
    GL_CHECK_RESULT;
        
    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_skyStage.ibo);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _skyStage.ibo);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    glBindVertexArrayOES(0);
    GL_CHECK_RESULT;

    // Compile vertex shader
    const QString vertexShader = QString::fromLatin1(
        "#version 100                                                                                                       ""\n"
        "                                                                                                                   ""\n"
        // Set default precisions
        "precision highp float;                                                                                             ""\n"
        "precision highp int;                                                                                               ""\n"
        "precision highp sampler2D;                                                                                         ""\n"
        "                                                                                                                   ""\n"
        // Constants
        "const float floatEpsilon = 0.000001;                                                                               ""\n"
        "                                                                                                                   ""\n"
        // Input data
        "attribute vec2 in_vs_vertexPosition;                                                                               ""\n"
        "                                                                                                                   ""\n"
        // Output data
        "varying float v2f_horizonOffsetN;                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mProjectionViewModel;                                                                        ""\n"
        "uniform vec2 param_vs_halfSize;                                                                                    ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    vec4 v = vec4(in_vs_vertexPosition.x * param_vs_halfSize.x,                                                    ""\n"
        "        in_vs_vertexPosition.y * param_vs_halfSize.y, 0.0, 1.0);                                                   ""\n"
        "                                                                                                                   ""\n"
        //   Horizon offset is in range [-1.0 ... +1.0], what is the same as input vertex data
        "    v2f_horizonOffsetN = in_vs_vertexPosition.y;                                                                   ""\n"
        "    gl_Position = param_vs_mProjectionViewModel * v;                                                               ""\n"
        "}                                                                                                                  ""\n");
    QString preprocessedVertexShader = vertexShader;
    _skyStage.vs.id = renderAPI->compileShader(GL_VERTEX_SHADER, preprocessedVertexShader.toStdString().c_str());
    assert(_skyStage.vs.id != 0);

    // Compile fragment shader
    const QString fragmentShader = QString::fromLatin1(
        "#version 100                                                                                                       ""\n"
        "                                                                                                                   ""\n"
        // Set default precisions
        "precision highp float;                                                                                             ""\n"
        "precision highp int;                                                                                               ""\n"
        "precision highp sampler2D;                                                                                         ""\n"
        "                                                                                                                   ""\n"
        // Constants
        "const float floatEpsilon = 0.000001;                                                                               ""\n"
        "                                                                                                                   ""\n"
        // Input data
        "varying float v2f_horizonOffsetN;                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform vec3 param_fs_skyColor;                                                                                    ""\n"
        "uniform vec3 param_fs_fogColor;                                                                                    ""\n"
        "uniform float param_fs_fogDensity;                                                                                 ""\n"
        "uniform float param_fs_fogHeightOriginFactor;                                                                      ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    float fogHeight = 1.0;                                                                                         ""\n"
        "    float fogStartHeight = fogHeight * (1.0 - param_fs_fogHeightOriginFactor);                                     ""\n"
        "    float fragmentHeight = 1.0 - v2f_horizonOffsetN;                                                               ""\n"
        //   Fog linear is factor in range [0.0 ... 1.0]
        "    float fogLinearFactor = min(max(fragmentHeight - fogStartHeight, 0.0) /                                        ""\n"
        "        (fogHeight - fogStartHeight), 1.0);                                                                        ""\n"
        "    float fogFactorBase = fogLinearFactor * param_fs_fogDensity;                                                   ""\n"
        "    float fogFactor = clamp(exp(-fogFactorBase*fogFactorBase), 0.0, 1.0);                                          ""\n"
        "    vec3 mixedColor = mix(param_fs_skyColor, param_fs_fogColor, 1.0 - fogFactor);                                  ""\n"
        "    gl_FragColor.rgba = vec4(mixedColor, 1.0);                                                                     ""\n"
        "}                                                                                                                  ""\n");
    QString preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    _skyStage.fs.id = renderAPI->compileShader(GL_FRAGMENT_SHADER, preprocessedFragmentShader.toStdString().c_str());
    assert(_skyStage.fs.id != 0);

    // Link everything into program object
    GLuint shaders[] = {
        _skyStage.vs.id,
        _skyStage.fs.id
    };
    _skyStage.program = renderAPI->linkProgram(2, shaders);
    assert(_skyStage.program != 0);

    renderAPI->clearVariablesLookup();
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.vs.in.vertexPosition, "in_vs_vertexPosition", RenderAPI_OpenGLES2::In);
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.vs.param.mProjectionViewModel, "param_vs_mProjectionViewModel", RenderAPI_OpenGLES2::Uniform);
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.vs.param.halfSize, "param_vs_halfSize", RenderAPI_OpenGLES2::Uniform);
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.fs.param.fogColor, "param_fs_fogColor", RenderAPI_OpenGLES2::Uniform);
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.fs.param.skyColor, "param_fs_skyColor", RenderAPI_OpenGLES2::Uniform);
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.fs.param.fogDensity, "param_fs_fogDensity", RenderAPI_OpenGLES2::Uniform);
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.fs.param.fogHeightOriginFactor, "param_fs_fogHeightOriginFactor", RenderAPI_OpenGLES2::Uniform);
    renderAPI->clearVariablesLookup();
}

void OsmAnd::AtlasMapRenderer_OpenGLES2::renderSkyStage()
{
    GL_CHECK_PRESENT(glBindVertexArrayOES);
    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glDrawElements);
    
    // Set tile patch VAO
    glBindVertexArrayOES(_skyStage.vao);
    GL_CHECK_RESULT;

    // Activate program
    glUseProgram(_skyStage.program);
    GL_CHECK_RESULT;

    // Set projection*view*model matrix:
    const auto mFogTranslate = glm::translate(0.0f, 0.0f, -_correctedFogDistance);
    const auto mModel = _mAzimuthInv * mFogTranslate;
    const auto mProjectionViewModel = _mProjection * _mView * mModel;
    glUniformMatrix4fv(_skyStage.vs.param.mProjectionViewModel, 1, GL_FALSE, glm::value_ptr(mProjectionViewModel));
    GL_CHECK_RESULT;

    // Set halfsize
    glUniform2f(_skyStage.vs.param.halfSize, _skyplaneHalfSize.x, _skyplaneHalfSize.y);
    GL_CHECK_RESULT;

    // Set fog and sky parameters
    glUniform3f(_skyStage.fs.param.skyColor, _activeConfig.skyColor[0], _activeConfig.skyColor[1], _activeConfig.skyColor[2]);
    GL_CHECK_RESULT;
    glUniform3f(_skyStage.fs.param.fogColor, _activeConfig.fogColor[0], _activeConfig.fogColor[1], _activeConfig.fogColor[2]);
    GL_CHECK_RESULT;
    glUniform1f(_skyStage.fs.param.fogDensity, _activeConfig.fogDensity);
    GL_CHECK_RESULT;
    glUniform1f(_skyStage.fs.param.fogHeightOriginFactor, _activeConfig.fogHeightOriginFactor);
    GL_CHECK_RESULT;
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    GL_CHECK_RESULT;

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    // Deselect VAO
    glBindVertexArrayOES(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRenderer_OpenGLES2::releaseSkyStage()
{
    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteVertexArraysOES);
    GL_CHECK_PRESENT(glDeleteProgram);
    GL_CHECK_PRESENT(glDeleteShader);

    if(_skyStage.ibo)
    {
        glDeleteBuffers(1, &_skyStage.ibo);
        GL_CHECK_RESULT;
    }

    if(_skyStage.vbo)
    {
        glDeleteBuffers(1, &_skyStage.vbo);
        GL_CHECK_RESULT;
    }

    if(_skyStage.vao)
    {
        glDeleteVertexArraysOES(1, &_skyStage.vao);
        GL_CHECK_RESULT;
    }

    if(_skyStage.program)
    {
        glDeleteProgram(_skyStage.program);
        GL_CHECK_RESULT;
    }
    if(_skyStage.fs.id)
    {
        glDeleteShader(_skyStage.fs.id);
        GL_CHECK_RESULT;
    }
    if(_skyStage.vs.id)
    {
        glDeleteShader(_skyStage.vs.id);
        GL_CHECK_RESULT;
    }
    memset(&_skyStage, 0, sizeof(_skyStage));
}
