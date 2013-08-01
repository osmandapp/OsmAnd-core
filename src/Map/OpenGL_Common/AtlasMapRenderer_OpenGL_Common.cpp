#include "AtlasMapRenderer_OpenGL_Common.h"

#include <assert.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <QtGlobal>
#include <QtMath>
#include <QThread>

#include <SkBitmap.h>

#include "IMapRenderer.h"
#include "IMapTileProvider.h"
#include "IMapBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "OsmAndCore/Logging.h"
#include "OsmAndCore/Utilities.h"
#include "OpenGL_Common/Utilities_OpenGL_Common.h"

OsmAnd::AtlasMapRenderer_OpenGL_Common::AtlasMapRenderer_OpenGL_Common()
    : _zNear(0.1f)
    , _tilePatchIndicesCount(0)
{
    memset(&_mapStage, 0, sizeof(_mapStage));
    memset(&_skyStage, 0, sizeof(_skyStage));
}

OsmAnd::AtlasMapRenderer_OpenGL_Common::~AtlasMapRenderer_OpenGL_Common()
{
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::allocateTilePatch( MapTileVertex* vertices, GLsizei verticesCount, GLushort* indices, GLsizei indicesCount )
{
    const auto renderAPI = getRenderAPI();

    GL_CHECK_PRESENT(glGenVertexArrays);
    GL_CHECK_PRESENT(glBindVertexArray);
    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    // Create Vertex Array Object
    renderAPI->glGenVertexArrays_wrapper(1, &_mapStage.tilePatchVAO);
    GL_CHECK_RESULT;
    renderAPI->glBindVertexArray_wrapper(_mapStage.tilePatchVAO);
    GL_CHECK_RESULT;

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_mapStage.tilePatchVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _mapStage.tilePatchVBO);
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
    glGenBuffers(1, &_mapStage.tilePatchIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mapStage.tilePatchIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    renderAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::releaseTilePatch()
{
    const auto renderAPI = getRenderAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);

    if(_mapStage.tilePatchIBO)
    {
        glDeleteBuffers(1, &_skyStage.skyplaneIBO);
        GL_CHECK_RESULT;
        _skyStage.skyplaneIBO = 0;
    }

    if(_mapStage.tilePatchVBO)
    {
        glDeleteBuffers(1, &_mapStage.tilePatchVBO);
        GL_CHECK_RESULT;
        _mapStage.tilePatchVBO = 0;
    }

    if(_mapStage.tilePatchVAO)
    {
        renderAPI->glDeleteVertexArrays_wrapper(1, &_mapStage.tilePatchVAO);
        GL_CHECK_RESULT;
        _mapStage.tilePatchVAO = 0;
    }
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::initializeMapStage()
{
    const auto renderAPI = getRenderAPI();

    // Compile vertex shader
    const auto& vertexShader_perTileLayerTexCoordsProcessing = QString::fromLatin1(
        "    calculateTextureCoordinates(                                                                                   ""\n"
        "        param_vs_perTileLayer[%layerId%],                                                                          ""\n"
        "        v2f_texCoordsPerLayer[%layerLinearIndex%]);                                                                ""\n"
        "                                                                                                                   ""\n");
    const auto& vertexShader = QString::fromLatin1(
        // Constants
        "const float floatEpsilon = 0.000001;                                                                               ""\n"
        "                                                                                                                   ""\n"
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "INPUT vec2 in_vs_vertexTexCoords;                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "PARAM_OUTPUT vec2 v2f_texCoordsPerLayer[%RasterTileLayersCount%];                                                  ""\n"
        "PARAM_OUTPUT float v2f_distanceFromTarget;                                                                         ""\n"
        "#if MIPMAPS_SUPPORTED                                                                                              ""\n"
        "    PARAM_OUTPUT float v2f_mipmapLOD;                                                                              ""\n"
        "#endif // MIPMAPS_SUPPORTED                                                                                        ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mProjectionView;                                                                             ""\n"
        "uniform vec2 param_vs_targetInTilePosN;                                                                            ""\n"
        "uniform ivec2 param_vs_targetTile;                                                                                 ""\n"
        "#if MIPMAPS_SUPPORTED                                                                                              ""\n"
        "    uniform mat4 param_vs_mView;                                                                                   ""\n"
        "    uniform float param_vs_cameraElevationAngle;                                                                   ""\n"
        "    uniform float param_vs_mipmapK;                                                                                ""\n"
        "#endif // MIPMAPS_SUPPORTED                                                                                        ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-tile data
        "uniform ivec2 param_vs_tile;                                                                                       ""\n"
        "#if VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                 ""\n"
        "    uniform float param_vs_elevationData_k;                                                                        ""\n"
        "    uniform sampler2D param_vs_elevationData_sampler;                                                              ""\n"
        "    uniform float param_vs_elevationData_upperMetersPerUnit;                                                       ""\n"
        "    uniform float param_vs_elevationData_lowerMetersPerUnit;                                                       ""\n"
        "#endif // VERTEX_TEXTURE_FETCH_SUPPORTED                                                                           ""\n"
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
        "#if VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                 ""\n"
        //   If elevation data is active, use it
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
        "#endif // VERTEX_TEXTURE_FETCH_SUPPORTED                                                                           ""\n"
        "                                                                                                                   ""\n"
        //   Calculate mipmap LOD
        "#if MIPMAPS_SUPPORTED                                                                                              ""\n"
        "    float distanceFromVertexToCamera = length( (param_vs_mView * v).xz );                                          ""\n"
        "    float mipmapThresholdDistance = param_vs_mipmapK * log(param_vs_cameraElevationAngle + 1.0);                   ""\n"
        "    v2f_mipmapLOD = distanceFromVertexToCamera / mipmapThresholdDistance - 1.0;                                    ""\n"
        "    v2f_mipmapLOD = clamp(v2f_mipmapLOD, 0.0, %MipmapLodLevelsMax%.0 - 1.0);                                       ""\n"
        "#endif // MIPMAPS_SUPPORTED                                                                                        ""\n"
        "                                                                                                                   ""\n"
        "    v2f_distanceFromTarget = length(v.xz);                                                                         ""\n"
        "                                                                                                                   ""\n"
        //   Finally output processed modified vertex
        "    gl_Position = param_vs_mProjectionView * v;                                                                    ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    QString preprocessedVertexShader_UnrolledPerLayerTexCoordsProcessingCode;
    for(int layerId = MapTileLayerId::RasterMap, linearIdx = 0; layerId < MapTileLayerIdsCount; layerId++, linearIdx++)
    {
        auto preprocessedVertexShader_perTileLayerTexCoordsProcessing = vertexShader_perTileLayerTexCoordsProcessing;
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
    preprocessedVertexShader.replace("%MipmapLodLevelsMax%", QString::number(RenderAPI_OpenGL_Common::MipmapLodLevelsMax));
    renderAPI->preprocessVertexShader(preprocessedVertexShader);
    _mapStage.vs.id = renderAPI->compileShader(GL_VERTEX_SHADER, preprocessedVertexShader.toStdString().c_str());
    assert(_mapStage.vs.id != 0);

    // Compile fragment shader
    const auto& fragmentShader_perTileLayer = QString::fromLatin1(
        "    if(param_fs_perTileLayer[%layerLinearIdx%].k > floatEpsilon)                                                   ""\n"
        "    {                                                                                                              ""\n"
        "#if MIPMAPS_SUPPORTED                                                                                              ""\n"
        "        vec4 layerColor = SAMPLE_TEXTURE_LOD(                                                                      ""\n"
        "            param_fs_perTileLayer[%layerLinearIdx%].sampler,                                                       ""\n"
        "            v2f_texCoordsPerLayer[%layerLinearIdx%], v2f_mipmapLOD);                                               ""\n"
        "#else //!MIPMAPS_SUPPORTED                                                                                         ""\n"
        "        vec4 layerColor = SAMPLE_TEXTURE(                                                                          ""\n"
        "            param_fs_perTileLayer[%layerLinearIdx%].sampler,                                                       ""\n"
        "            v2f_texCoordsPerLayer[%layerLinearIdx%]);                                                              ""\n"
        "#endif //MIPMAPS_SUPPORTED                                                                                         ""\n"
        "                                                                                                                   ""\n"
        "        baseColor = mix(baseColor, layerColor, layerColor.a * param_fs_perTileLayer[%layerLinearIdx%].k);          ""\n"
        "    }                                                                                                              ""\n");
    const auto& fragmentShader = QString::fromLatin1(
        // Constants
        "const float floatEpsilon = 0.000001;                                                                               ""\n"
        "                                                                                                                   ""\n"
        // Input data
        "PARAM_INPUT vec2 v2f_texCoordsPerLayer[%RasterTileLayersCount%];                                                   ""\n"
        "PARAM_INPUT float v2f_distanceFromTarget;                                                                          ""\n"
        "#if MIPMAPS_SUPPORTED                                                                                              ""\n"
        "    PARAM_INPUT float v2f_mipmapLOD;                                                                               ""\n"
        "#endif // MIPMAPS_SUPPORTED                                                                                        ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
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
        "#if MIPMAPS_SUPPORTED                                                                                              ""\n"
        "    vec4 baseColor = SAMPLE_TEXTURE_LOD(                                                                           ""\n"
        "        param_fs_perTileLayer[0].sampler,                                                                          ""\n"
        "        v2f_texCoordsPerLayer[0], v2f_mipmapLOD);                                                                  ""\n"
        "#else //!MIPMAPS_SUPPORTED                                                                                         ""\n"
        "    vec4 layerColor = SAMPLE_TEXTURE(                                                                              ""\n"
        "        param_fs_perTileLayer[0].sampler,                                                                          ""\n"
        "        v2f_texCoordsPerLayer[0]);                                                                                 ""\n"
        "#endif //MIPMAPS_SUPPORTED                                                                                         ""\n"
        "    baseColor.a *= param_fs_perTileLayer[0].k;                                                                     ""\n"
        "%UnrolledPerLayerProcessingCode%                                                                                   ""\n"
        "                                                                                                                   ""\n"
        //   Apply fog (square exponential)
        "    float fogDistanceScaled = param_fs_fogDistance * param_fs_scaleToRetainProjectedSize;                          ""\n"
        "    float fogStartDistance = fogDistanceScaled * (1.0 - param_fs_fogOriginFactor);                                 ""\n"
        "    float fogLinearFactor = min(max(v2f_distanceFromTarget - fogStartDistance, 0.0) /                              ""\n"
        "        (fogDistanceScaled - fogStartDistance), 1.0);                                                              ""\n"

        "    float fogFactorBase = fogLinearFactor * param_fs_fogDensity;                                                   ""\n"
        "    float fogFactor = clamp(exp(-fogFactorBase*fogFactorBase), 0.0, 1.0);                                          ""\n"
        "    FRAGMENT_COLOR_OUTPUT = mix(baseColor, vec4(param_fs_fogColor, 1.0), 1.0 - fogFactor);                         ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    for(int layerId = MapTileLayerId::MapOverlay0; layerId <= MapTileLayerId::MapOverlay3; layerId++)
    {
        auto linearIdx = layerId - MapTileLayerId::RasterMap;
        auto preprocessedFragmentShader_perTileLayer = fragmentShader_perTileLayer;
        preprocessedFragmentShader_perTileLayer.replace("%layerLinearIdx%", QString::number(linearIdx));

        preprocessedFragmentShader_UnrolledPerLayerProcessingCode += preprocessedFragmentShader_perTileLayer;
    }
    preprocessedFragmentShader.replace("%UnrolledPerLayerProcessingCode%", preprocessedFragmentShader_UnrolledPerLayerProcessingCode);
    preprocessedFragmentShader.replace("%TileLayersCount%", QString::number(MapTileLayerIdsCount));
    preprocessedFragmentShader.replace("%RasterTileLayersCount%", QString::number(MapTileLayerIdsCount - MapTileLayerId::RasterMap));
    preprocessedFragmentShader.replace("%Layer_RasterMap%", QString::number(MapTileLayerId::RasterMap));
    preprocessedFragmentShader.replace("%MipmapLodLevelsMax%", QString::number(RenderAPI_OpenGL_Common::MipmapLodLevelsMax));
    renderAPI->preprocessFragmentShader(preprocessedFragmentShader);
    _mapStage.fs.id = renderAPI->compileShader(GL_FRAGMENT_SHADER, preprocessedFragmentShader.toStdString().c_str());
    assert(_mapStage.fs.id != 0);

    // Link everything into program object
    GLuint shaders[] = {
        _mapStage.vs.id,
        _mapStage.fs.id
    };
    _mapStage.program = getRenderAPI()->linkProgram(2, shaders);
    assert(_mapStage.program != 0);

    renderAPI->clearVariablesLookup();
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.in.vertexPosition, "in_vs_vertexPosition", GLShaderVariableType::In);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.in.vertexTexCoords, "in_vs_vertexTexCoords", GLShaderVariableType::In);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.mProjectionView, "param_vs_mProjectionView", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.targetInTilePosN, "param_vs_targetInTilePosN", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.targetTile, "param_vs_targetTile", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.mView, "param_vs_mView", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.cameraElevationAngle, "param_vs_cameraElevationAngle", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.mipmapK, "param_vs_mipmapK", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.tile, "param_vs_tile", GLShaderVariableType::Uniform);
    if(renderAPI->isSupported_vertexShaderTextureLookup)
    {
        renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.elevationData_sampler, "param_vs_elevationData_sampler", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.elevationData_k, "param_vs_elevationData_k", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.elevationData_upperMetersPerUnit, "param_vs_elevationData_upperMetersPerUnit", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, _mapStage.vs.param.elevationData_lowerMetersPerUnit, "param_vs_elevationData_lowerMetersPerUnit", GLShaderVariableType::Uniform);
    }
    for(int layerId = 0; layerId < MapTileLayerIdsCount; layerId++)
    {
        auto layerStructName =
            QString::fromLatin1("param_vs_perTileLayer[%layerId%]")
            .replace(QString::fromLatin1("%layerId%"), QString::number(layerId));
        auto& layerStruct = _mapStage.vs.param.perTileLayer[layerId];

        renderAPI->findVariableLocation(_mapStage.program, layerStruct.tileSizeN, layerStructName + ".tileSizeN", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, layerStruct.tilePaddingN, layerStructName + ".tilePaddingN", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, layerStruct.slotsPerSide, layerStructName + ".slotsPerSide", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, layerStruct.slotIndex, layerStructName + ".slotIndex", GLShaderVariableType::Uniform);
    }
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.fs.param.fogColor, "param_fs_fogColor", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.fs.param.fogDistance, "param_fs_fogDistance", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.fs.param.fogDensity, "param_fs_fogDensity", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.fs.param.fogOriginFactor, "param_fs_fogOriginFactor", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_mapStage.program, _mapStage.fs.param.scaleToRetainProjectedSize, "param_fs_scaleToRetainProjectedSize", GLShaderVariableType::Uniform);
    for(int layerId = MapTileLayerId::RasterMap, linearIdx = 0; layerId < MapTileLayerIdsCount; layerId++, linearIdx++)
    {
        auto layerStructName =
            QString::fromLatin1("param_fs_perTileLayer[%linearIdx%]")
            .replace(QString::fromLatin1("%linearIdx%"), QString::number(linearIdx));
        auto& layerStruct = _mapStage.fs.param.perTileLayer[linearIdx];

        renderAPI->findVariableLocation(_mapStage.program, layerStruct.k, layerStructName + ".k", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(_mapStage.program, layerStruct.sampler, layerStructName + ".sampler", GLShaderVariableType::Uniform);
    }
    renderAPI->clearVariablesLookup();
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::renderMapStage()
{
    const auto renderAPI = getRenderAPI();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glUniform1i);
    GL_CHECK_PRESENT(glUniform2i);
    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glBindSampler);

    // Set tile patch VAO
    renderAPI->glBindVertexArray_wrapper(_mapStage.tilePatchVAO);
    GL_CHECK_RESULT;

    // Activate program
    glUseProgram(_mapStage.program);
    GL_CHECK_RESULT;

    // Set matrices
    auto mProjectionView = _mProjection * _mView;
    glUniformMatrix4fv(_mapStage.vs.param.mProjectionView, 1, GL_FALSE, glm::value_ptr(mProjectionView));
    GL_CHECK_RESULT;
    glUniformMatrix4fv(_mapStage.vs.param.mView, 1, GL_FALSE, glm::value_ptr(_mView));
    GL_CHECK_RESULT;

    // Set center offset
    glUniform2f(_mapStage.vs.param.targetInTilePosN, _targetInTileOffsetN.x, _targetInTileOffsetN.y);
    GL_CHECK_RESULT;

    // Set target tile
    glUniform2i(_mapStage.vs.param.targetTile, _targetTileId.x, _targetTileId.y);
    GL_CHECK_RESULT;

    // Mipmap support
    if(renderAPI->isSupported_shaderTextureLOD)
    {
        // View matrix
        glUniformMatrix4fv(_mapStage.vs.param.mView, 1, GL_FALSE, glm::value_ptr(_mView));
        GL_CHECK_RESULT;

        // Set camera elevation angle
        glUniform1f(_mapStage.vs.param.cameraElevationAngle, currentState.elevationAngle);
        GL_CHECK_RESULT;

        // Set mipmap K factor
        glUniform1f(_mapStage.vs.param.mipmapK, _mipmapK);
        GL_CHECK_RESULT;
    }

    // Set fog parameters
    glUniform3f(_mapStage.fs.param.fogColor, currentState.fogColor[0], currentState.fogColor[1], currentState.fogColor[2]);
    GL_CHECK_RESULT;
    glUniform1f(_mapStage.fs.param.fogDistance, currentState.fogDistance);
    GL_CHECK_RESULT;
    glUniform1f(_mapStage.fs.param.fogDensity, currentState.fogDensity);
    GL_CHECK_RESULT;
    glUniform1f(_mapStage.fs.param.fogOriginFactor, currentState.fogOriginFactor);
    GL_CHECK_RESULT;
    glUniform1f(_mapStage.fs.param.scaleToRetainProjectedSize, _scaleToRetainProjectedSize);
    GL_CHECK_RESULT;

    // Configure samplers
    if(renderAPI->isSupported_vertexShaderTextureLookup)
    {
        glUniform1i(_mapStage.vs.param.elevationData_sampler, MapTileLayerId::ElevationData);
        GL_CHECK_RESULT;

        renderAPI->setSampler(GL_TEXTURE0 + MapTileLayerId::ElevationData, RenderAPI_OpenGL_Common::SamplerType::ElevationDataTile);
    }
    auto bitmapTileSamplerType = RenderAPI_OpenGL_Common::SamplerType::BitmapTile_Bilinear;
    switch(currentConfiguration.texturesFilteringQuality)
    {
    case MapRendererConfiguration::TextureFilteringQuality::Good:
        bitmapTileSamplerType = RenderAPI_OpenGL_Common::SamplerType::BitmapTile_BilinearMipmap;
    case MapRendererConfiguration::TextureFilteringQuality::Best:
        bitmapTileSamplerType = RenderAPI_OpenGL_Common::SamplerType::BitmapTile_TrilinearMipmap;
    }
    for(int layerId = MapTileLayerId::RasterMap; layerId < MapTileLayerIdsCount; layerId++)
    {
        glUniform1i(_mapStage.fs.param.perTileLayer[layerId - MapTileLayerId::RasterMap].sampler, layerId);
        GL_CHECK_RESULT;

        renderAPI->setSampler(GL_TEXTURE0 + layerId, bitmapTileSamplerType);
    }

    // Check if we need to process elevation data
    const bool elevationDataSupported = renderAPI->isSupported_vertexShaderTextureLookup;
    const bool elevationDataEnabled = elevationDataSupported && currentState.tileProviders[MapTileLayerId::ElevationData];
    if(elevationDataSupported && !elevationDataEnabled)
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
                }
                else
                {
                    const auto& texture = static_cast<RenderAPI::TextureInGPU*>(gpuResource.get());

                    glUniform1i(perTile_vs.slotIndex, 0);
                    GL_CHECK_RESULT;
                    glUniform1f(perTile_vs.tileSizeN, 1.0f);
                    GL_CHECK_RESULT;
                    glUniform1f(perTile_vs.tilePaddingN, texture->halfTexelSizeN);
                    GL_CHECK_RESULT;
                    glUniform1i(perTile_vs.slotsPerSide, 1);
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

            glUniform1f(perTile_fs.k, currentState.tileLayerOpacity[layerId]);
            GL_CHECK_RESULT;

            if(!gpuResource)
                continue;
            
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
            }
            else
            {
                glUniform1i(perTile_vs.slotIndex, 0);
                GL_CHECK_RESULT;
                glUniform1f(perTile_vs.tileSizeN, 1.0f);
                GL_CHECK_RESULT;
                glUniform1f(perTile_vs.tilePaddingN, 0.0f);
                GL_CHECK_RESULT;
                glUniform1i(perTile_vs.slotsPerSide, 1);
                GL_CHECK_RESULT;
            }
        }

        glDrawElements(GL_TRIANGLES, _tilePatchIndicesCount, GL_UNSIGNED_SHORT, nullptr);
        GL_CHECK_RESULT;
    }

    // Disable textures
    for(int layerId = 0; layerId < MapTileLayerIdsCount; layerId++)
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
    renderAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::releaseMapStage()
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

    releaseTilePatch();

    memset(&_mapStage, 0, sizeof(_mapStage));
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::initializeSkyStage()
{
    const auto renderAPI = getRenderAPI();

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
    renderAPI->glGenVertexArrays_wrapper(1, &_skyStage.skyplaneVAO);
    GL_CHECK_RESULT;
    renderAPI->glBindVertexArray_wrapper(_skyStage.skyplaneVAO);
    GL_CHECK_RESULT;

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_skyStage.skyplaneVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _skyStage.skyplaneVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float) * 2, vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(_skyStage.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(_skyStage.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_skyStage.skyplaneIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _skyStage.skyplaneIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    renderAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;

    // Compile vertex shader
    const QString vertexShader = QString::fromLatin1(
        // Constants
        "const float floatEpsilon = 0.000001;                                                                               ""\n"
        "                                                                                                                   ""\n"
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Output data
        "PARAM_OUTPUT float v2f_horizonOffsetN;                                                                             ""\n"
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
    auto preprocessedVertexShader = vertexShader;
    renderAPI->preprocessVertexShader(preprocessedVertexShader);
    _skyStage.vs.id = renderAPI->compileShader(GL_VERTEX_SHADER, preprocessedVertexShader.toStdString().c_str());
    assert(_skyStage.vs.id != 0);

    // Compile fragment shader
    const QString fragmentShader = QString::fromLatin1(
        // Constants
        "const float floatEpsilon = 0.000001;                                                                               ""\n"
        "                                                                                                                   ""\n"
        // Input data
        "PARAM_INPUT float v2f_horizonOffsetN;                                                                              ""\n"
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
        "                                                                                                                   ""\n"
        "    FRAGMENT_COLOR_OUTPUT.rgba = vec4(mixedColor, 1.0);                                                            ""\n"
        "}                                                                                                                  ""\n");
    QString preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    renderAPI->preprocessFragmentShader(preprocessedFragmentShader);
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
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.vs.in.vertexPosition, "in_vs_vertexPosition", GLShaderVariableType::In);
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.vs.param.mProjectionViewModel, "param_vs_mProjectionViewModel", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.vs.param.halfSize, "param_vs_halfSize", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.fs.param.fogColor, "param_fs_fogColor", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.fs.param.skyColor, "param_fs_skyColor", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.fs.param.fogDensity, "param_fs_fogDensity", GLShaderVariableType::Uniform);
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.fs.param.fogHeightOriginFactor, "param_fs_fogHeightOriginFactor", GLShaderVariableType::Uniform);
    renderAPI->clearVariablesLookup();
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::renderSkyStage()
{
    const auto renderAPI = getRenderAPI();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glDrawElements);

    // Set tile patch VAO
    renderAPI->glBindVertexArray_wrapper(_skyStage.skyplaneVAO);
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
    glUniform3f(_skyStage.fs.param.skyColor, currentState.skyColor[0], currentState.skyColor[1], currentState.skyColor[2]);
    GL_CHECK_RESULT;
    glUniform3f(_skyStage.fs.param.fogColor, currentState.fogColor[0], currentState.fogColor[1], currentState.fogColor[2]);
    GL_CHECK_RESULT;
    glUniform1f(_skyStage.fs.param.fogDensity, currentState.fogDensity);
    GL_CHECK_RESULT;
    glUniform1f(_skyStage.fs.param.fogHeightOriginFactor, currentState.fogHeightOriginFactor);
    GL_CHECK_RESULT;

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    GL_CHECK_RESULT;

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    // Deselect VAO
    renderAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::releaseSkyStage()
{
    const auto renderAPI = getRenderAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);
    GL_CHECK_PRESENT(glDeleteShader);

    if(_skyStage.skyplaneIBO)
    {
        glDeleteBuffers(1, &_skyStage.skyplaneIBO);
        GL_CHECK_RESULT;
    }

    if(_skyStage.skyplaneVBO)
    {
        glDeleteBuffers(1, &_skyStage.skyplaneVBO);
        GL_CHECK_RESULT;
    }

    if(_skyStage.skyplaneVAO)
    {
        renderAPI->glDeleteVertexArrays_wrapper(1, &_skyStage.skyplaneVAO);
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

bool OsmAnd::AtlasMapRenderer_OpenGL_Common::doInitializeRendering()
{
    bool ok;

    ok = AtlasMapRenderer::doInitializeRendering();
    if(!ok)
        return false;

    initializeSkyStage();
    initializeMapStage();

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL_Common::doRenderFrame()
{
    GL_CHECK_PRESENT(glViewport);

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

bool OsmAnd::AtlasMapRenderer_OpenGL_Common::doReleaseRendering()
{
    bool ok;

    releaseMapStage();
    releaseSkyStage();

    ok = AtlasMapRenderer::doReleaseRendering();
    if(!ok)
        return false;

    return true;
}


float OsmAnd::AtlasMapRenderer_OpenGL_Common::getReferenceTileSizeOnScreen()
{
    const auto& rasterMapProvider = currentState.tileProviders[RasterMap];
    if(!rasterMapProvider)
        return std::numeric_limits<float>::quiet_NaN();

    auto tileProvider = static_cast<IMapBitmapTileProvider*>(rasterMapProvider.get());
    return tileProvider->getTileSize() * (setupOptions.displayDensityFactor / tileProvider->getTileDensity());
}

float OsmAnd::AtlasMapRenderer_OpenGL_Common::getScaledTileSizeOnScreen()
{
    return getReferenceTileSizeOnScreen() * _tileScaleFactor;
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::validateLayer( const MapTileLayerId& layer )
{
    MapRenderer::validateLayer(layer);

    if(layer == ElevationData)
    {
        // Recreate tile patch since elevation data influences density of tile patch
        releaseTilePatch();
        createTilePatch();
    }
}

bool OsmAnd::AtlasMapRenderer_OpenGL_Common::updateCurrentState()
{
    bool ok;
    ok = AtlasMapRenderer::updateCurrentState();
    if(!ok)
        return false;

    // Prepare values for projection matrix
    const auto viewportWidth = currentState.viewport.width();
    const auto viewportHeight = currentState.viewport.height();
    if(viewportWidth == 0 || viewportHeight == 0)
        return false;
    _aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    _fovInRadians = qDegreesToRadians(currentState.fieldOfView);
    _projectionPlaneHalfHeight = _zNear * _fovInRadians;
    _projectionPlaneHalfWidth = _projectionPlaneHalfHeight * _aspectRatio;

    // Setup projection with fake Z-far plane
    _mProjection = glm::frustum(-_projectionPlaneHalfWidth, _projectionPlaneHalfWidth, -_projectionPlaneHalfHeight, _projectionPlaneHalfHeight, _zNear, 1000.0f);

    // Calculate limits of camera distance to target and actual distance
    const float& screenTile = getReferenceTileSizeOnScreen();
    _nearDistanceFromCameraToTarget = Utilities_OpenGL_Common::calculateCameraDistance(_mProjection, currentState.viewport, TileSide3D / 2.0f, screenTile / 2.0f, 1.5f);
    _baseDistanceFromCameraToTarget = Utilities_OpenGL_Common::calculateCameraDistance(_mProjection, currentState.viewport, TileSide3D / 2.0f, screenTile / 2.0f, 1.0f);
    _farDistanceFromCameraToTarget = Utilities_OpenGL_Common::calculateCameraDistance(_mProjection, currentState.viewport, TileSide3D / 2.0f, screenTile / 2.0f, 0.75f);

    // zoomFraction == [ 0.0 ... 0.5] scales tile [1.0x ... 1.5x]
    // zoomFraction == [-0.5 ...-0.0] scales tile [.75x ... 1.0x]
    if(currentState.zoomFraction >= 0.0f)
        _distanceFromCameraToTarget = _baseDistanceFromCameraToTarget - (_baseDistanceFromCameraToTarget - _nearDistanceFromCameraToTarget) * (2.0f * currentState.zoomFraction);
    else
        _distanceFromCameraToTarget = _baseDistanceFromCameraToTarget - (_farDistanceFromCameraToTarget - _baseDistanceFromCameraToTarget) * (2.0f * currentState.zoomFraction);
    _groundDistanceFromCameraToTarget = _distanceFromCameraToTarget * qCos(qDegreesToRadians(currentState.elevationAngle));
    _tileScaleFactor = ((currentState.zoomFraction >= 0.0f) ? (1.0f + currentState.zoomFraction) : (1.0f + 0.5f * currentState.zoomFraction));
    _scaleToRetainProjectedSize = _distanceFromCameraToTarget / _baseDistanceFromCameraToTarget;

    // Recalculate projection with obtained value
    _zSkyplane = currentState.fogDistance * _scaleToRetainProjectedSize + _distanceFromCameraToTarget;
    _zFar = glm::length(glm::vec3(_projectionPlaneHalfWidth * (_zSkyplane / _zNear), _projectionPlaneHalfHeight * (_zSkyplane / _zNear), _zSkyplane));
    _mProjection = glm::frustum(-_projectionPlaneHalfWidth, _projectionPlaneHalfWidth, -_projectionPlaneHalfHeight, _projectionPlaneHalfHeight, _zNear, _zFar);
    _mProjectionInv = glm::inverse(_mProjection);

    // Setup camera
    _mDistance = glm::translate(0.0f, 0.0f, -_distanceFromCameraToTarget);
    _mElevation = glm::rotate(currentState.elevationAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    _mAzimuth = glm::rotate(currentState.azimuth, glm::vec3(0.0f, 1.0f, 0.0f));
    _mView = _mDistance * _mElevation * _mAzimuth;

    // Get inverse camera
    _mDistanceInv = glm::translate(0.0f, 0.0f, _distanceFromCameraToTarget);
    _mElevationInv = glm::rotate(-currentState.elevationAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    _mAzimuthInv = glm::rotate(-currentState.azimuth, glm::vec3(0.0f, 1.0f, 0.0f));
    _mViewInv = _mAzimuthInv * _mElevationInv * _mDistanceInv;

    // Correct fog distance
    _correctedFogDistance = currentState.fogDistance * _scaleToRetainProjectedSize + (_distanceFromCameraToTarget - _groundDistanceFromCameraToTarget);

    // Calculate skyplane size
    float zSkyplaneK = _zSkyplane / _zNear;
    _skyplaneHalfSize.x = zSkyplaneK * _projectionPlaneHalfWidth;
    _skyplaneHalfSize.y = zSkyplaneK * _projectionPlaneHalfHeight;

    // Update mipmap K
    _mipmapK = static_cast<float>(viewportHeight) / (4.6f * setupOptions.displayDensityFactor);

    // Compute visible tileset
    computeVisibleTileset();

    return true;
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::computeVisibleTileset()
{
    // 4 points of frustum near clipping box in camera coordinate space
    const glm::vec4 nTL_c(-_projectionPlaneHalfWidth, +_projectionPlaneHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nTR_c(+_projectionPlaneHalfWidth, +_projectionPlaneHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nBL_c(-_projectionPlaneHalfWidth, -_projectionPlaneHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nBR_c(+_projectionPlaneHalfWidth, -_projectionPlaneHalfHeight, -_zNear, 1.0f);

    // 4 points of frustum far clipping box in camera coordinate space
    const auto zFar = _zSkyplane;
    const auto zFarK = zFar / _zNear;
    const glm::vec4 fTL_c(zFarK * nTL_c.x, zFarK * nTL_c.y, zFarK * nTL_c.z, 1.0f);
    const glm::vec4 fTR_c(zFarK * nTR_c.x, zFarK * nTR_c.y, zFarK * nTR_c.z, 1.0f);
    const glm::vec4 fBL_c(zFarK * nBL_c.x, zFarK * nBL_c.y, zFarK * nBL_c.z, 1.0f);
    const glm::vec4 fBR_c(zFarK * nBR_c.x, zFarK * nBR_c.y, zFarK * nBR_c.z, 1.0f);

    // Transform 8 frustum vertices + camera center to global space
    const auto eye_g = _mViewInv * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    const auto fTL_g = _mViewInv * fTL_c;
    const auto fTR_g = _mViewInv * fTR_c;
    const auto fBL_g = _mViewInv * fBL_c;
    const auto fBR_g = _mViewInv * fBR_c;
    const auto nTL_g = _mViewInv * nTL_c;
    const auto nTR_g = _mViewInv * nTR_c;
    const auto nBL_g = _mViewInv * nBL_c;
    const auto nBR_g = _mViewInv * nBR_c;

    // Get (up to) 4 points of frustum edges & plane intersection
    const glm::vec3 planeN(0.0f, 1.0f, 0.0f);
    const glm::vec3 planeO(0.0f, 0.0f, 0.0f);
    auto intersectionPointsCounter = 0u;
    glm::vec3 intersectionPoint;
    glm::vec2 intersectionPoints[4];

    if(intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nBL_g.xyz, fBL_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nBR_g.xyz, fBR_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTR_g.xyz, fTR_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTL_g.xyz, fTL_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, fTR_g.xyz, fBR_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, fTL_g.xyz, fBL_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTR_g.xyz, nBR_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTL_g.xyz, nBL_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    
    assert(intersectionPointsCounter == 4);

    // Normalize intersection points to tiles
    intersectionPoints[0] /= static_cast<float>(TileSide3D);
    intersectionPoints[1] /= static_cast<float>(TileSide3D);
    intersectionPoints[2] /= static_cast<float>(TileSide3D);
    intersectionPoints[3] /= static_cast<float>(TileSide3D);

    // "Round"-up tile indices
    // In-tile normalized position is added, since all tiles are going to be
    // translated in opposite direction during rendering
    const auto& ip = intersectionPoints;
    const PointF p[4] =
    {
        PointF(ip[0].x + _targetInTileOffsetN.x, ip[0].y + _targetInTileOffsetN.y),
        PointF(ip[1].x + _targetInTileOffsetN.x, ip[1].y + _targetInTileOffsetN.y),
        PointF(ip[2].x + _targetInTileOffsetN.x, ip[2].y + _targetInTileOffsetN.y),
        PointF(ip[3].x + _targetInTileOffsetN.x, ip[3].y + _targetInTileOffsetN.y),
    };

    //NOTE: so far scanline does not work exactly as expected, so temporary switch to old implementation
    {
        QSet<TileId> visibleTiles;
        PointI p0(qFloor(p[0].x), qFloor(p[0].y));
        PointI p1(qFloor(p[1].x), qFloor(p[1].y));
        PointI p2(qFloor(p[2].x), qFloor(p[2].y));
        PointI p3(qFloor(p[3].x), qFloor(p[3].y));

        const auto xMin = qMin(qMin(p0.x, p1.x), qMin(p2.x, p3.x));
        const auto xMax = qMax(qMax(p0.x, p1.x), qMax(p2.x, p3.x));
        const auto yMin = qMin(qMin(p0.y, p1.y), qMin(p2.y, p3.y));
        const auto yMax = qMax(qMax(p0.y, p1.y), qMax(p2.y, p3.y));
        for(auto x = xMin; x <= xMax; x++)
        {
            for(auto y = yMin; y <= yMax; y++)
            {
                TileId tileId;
                tileId.x = x + _targetTileId.x;
                tileId.y = y + _targetTileId.y;

                visibleTiles.insert(tileId);
            }
        }

        _visibleTiles = visibleTiles.toList();
    }
    /*
    //TODO: Find visible tiles using scanline fill
    _visibleTiles.clear();
    Utilities::scanlineFillPolygon(4, &p[0],
        [this, pC](const PointI& point)
        {
            TileId tileId;
            tileId.x = point.x;// + _targetTile.x;
            tileId.y = point.y;// + _targetTile.y;
            
            _visibleTiles.insert(tileId);
        });
    */
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::validateConfigurationChange( const ConfigurationChange& change )
{
}

bool OsmAnd::AtlasMapRenderer_OpenGL_Common::postInitializeRendering()
{
    bool ok;

    ok = AtlasMapRenderer::postInitializeRendering();
    if(!ok)
        return false;

    createTilePatch();

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL_Common::preReleaseRendering()
{
    bool ok;

    ok = AtlasMapRenderer::preReleaseRendering();
    if(!ok)
        return false;

    releaseTilePatch();

    return true;
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::createTilePatch()
{
    MapTileVertex* pVertices = nullptr;
    GLsizei verticesCount = 0;
    GLushort* pIndices = nullptr;
    GLsizei indicesCount = 0;
    
    if(!currentState.tileProviders[ElevationData])
    {
        // Simple tile patch, that consists of 4 vertices

        // Vertex data
        const GLfloat tsz = static_cast<GLfloat>(TileSide3D);
        MapTileVertex vertices[4] =
        {
            { {0.0f, 0.0f}, {0.0f, 0.0f} },
            { {0.0f,  tsz}, {0.0f, 1.0f} },
            { { tsz,  tsz}, {1.0f, 1.0f} },
            { { tsz, 0.0f}, {1.0f, 0.0f} }
        };
        pVertices = &vertices[0];
        verticesCount = 4;

        // Index data
        GLushort indices[6] =
        {
            0, 1, 2,
            0, 2, 3
        };
        pIndices = &indices[0];
        indicesCount = 6;
    }
    else
    {
        // Complex tile patch, that consists of (TileElevationNodesPerSide*TileElevationNodesPerSide) number of
        // height clusters. Height cluster itself consists of 4 vertices, 6 indices and 2 polygons
        const GLfloat clusterSize = static_cast<GLfloat>(TileSide3D) / static_cast<float>(currentConfiguration.heightmapPatchesPerSide);
        const auto verticesPerLine = currentConfiguration.heightmapPatchesPerSide + 1;
        verticesCount = verticesPerLine * verticesPerLine;
        pVertices = new MapTileVertex[verticesCount];
        indicesCount = (currentConfiguration.heightmapPatchesPerSide * currentConfiguration.heightmapPatchesPerSide) * 6;
        pIndices = new GLushort[indicesCount];

        MapTileVertex* pV = pVertices;

        // Form vertices
        for(auto row = 0u; row < verticesPerLine; row++)
        {
            for(auto col = 0u; col < verticesPerLine; col++, pV++)
            {
                pV->position[0] = static_cast<float>(col) * clusterSize;
                pV->position[1] = static_cast<float>(row) * clusterSize;

                pV->uv[0] = static_cast<float>(col) / static_cast<float>(currentConfiguration.heightmapPatchesPerSide);
                pV->uv[1] = static_cast<float>(row) / static_cast<float>(currentConfiguration.heightmapPatchesPerSide);
            }
        }

        // Form indices
        GLushort* pI = pIndices;
        for(auto row = 0u; row < currentConfiguration.heightmapPatchesPerSide; row++)
        {
            for(auto col = 0u; col < currentConfiguration.heightmapPatchesPerSide; col++)
            {
                // p1 - top left
                // p2 - bottom left
                // p3 - bottom right
                // p4 - top right
                const auto p1 = (row + 0) * verticesPerLine + col + 0;
                const auto p2 = (row + 1) * verticesPerLine + col + 0;
                const auto p3 = (row + 1) * verticesPerLine + col + 1;
                const auto p4 = (row + 0) * verticesPerLine + col + 1;

                // Triangle 0
                pI[0] = p1;
                pI[1] = p2;
                pI[2] = p3;
                pI += 3;

                // Triangle 1
                pI[0] = p1;
                pI[1] = p3;
                pI[2] = p4;
                pI += 3;
            }
        }
    }
    
    _tilePatchIndicesCount = indicesCount;
    allocateTilePatch(pVertices, verticesCount, pIndices, indicesCount);

    if(currentState.tileProviders[ElevationData])
    {
        delete[] pVertices;
        delete[] pIndices;
    }
}

OsmAnd::RenderAPI_OpenGL_Common* OsmAnd::AtlasMapRenderer_OpenGL_Common::getRenderAPI() const
{
    return static_cast<OsmAnd::RenderAPI_OpenGL_Common*>(renderAPI.get());
}
