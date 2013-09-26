#include "AtlasMapRenderer_OpenGL_Common.h"

#include <cassert>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <QtGlobal>
#include <QtNumeric>
#include <QtMath>
#include <QThread>

#include <SkBitmap.h>

#include "IMapRenderer.h"
#include "IMapTileProvider.h"
#include "IMapBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "Logging.h"
#include "Utilities.h"
#include "OpenGL_Common/Utilities_OpenGL_Common.h"

OsmAnd::AtlasMapRenderer_OpenGL_Common::AtlasMapRenderer_OpenGL_Common()
    : _zNear(0.1f)
    , _tilePatchIndicesCount(0)
{
    memset(&_rasterMapStage, 0, sizeof(_rasterMapStage));
    memset(&_skyStage, 0, sizeof(_skyStage));
}

OsmAnd::AtlasMapRenderer_OpenGL_Common::~AtlasMapRenderer_OpenGL_Common()
{
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::allocateTilePatch( MapTileVertex* vertices, GLsizei verticesCount, GLushort* indices, GLsizei indicesCount )
{
    const auto renderAPI = getRenderAPI();

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_rasterMapStage.tilePatchVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _rasterMapStage.tilePatchVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(MapTileVertex), vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    
    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_rasterMapStage.tilePatchIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _rasterMapStage.tilePatchIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    for(auto variationId = 0u, maxActiveMapLayers = 1u; variationId < RasterMapLayersCount; variationId++, maxActiveMapLayers++)
    {
        auto& stageVariation = _rasterMapStage.variations[variationId];

        // Create Vertex Array Object
        renderAPI->glGenVertexArrays_wrapper(1, &stageVariation.tilePatchVAO);
        GL_CHECK_RESULT;
        renderAPI->glBindVertexArray_wrapper(stageVariation.tilePatchVAO);
        GL_CHECK_RESULT;

        // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, _rasterMapStage.tilePatchVBO);
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(stageVariation.vs.in.vertexPosition);
        GL_CHECK_RESULT;
        glVertexAttribPointer(stageVariation.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(MapTileVertex), reinterpret_cast<GLvoid*>(offsetof(MapTileVertex, position)));
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(stageVariation.vs.in.vertexTexCoords);
        GL_CHECK_RESULT;
        glVertexAttribPointer(stageVariation.vs.in.vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(MapTileVertex), reinterpret_cast<GLvoid*>(offsetof(MapTileVertex, uv)));
        GL_CHECK_RESULT;

        // Bind IBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _rasterMapStage.tilePatchIBO);
        GL_CHECK_RESULT;
    }

    renderAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::releaseTilePatch()
{
    const auto renderAPI = getRenderAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);

    for(auto variationId = 0u, maxActiveMapLayers = 1u; variationId < RasterMapLayersCount; variationId++, maxActiveMapLayers++)
    {
        auto& stageVariation = _rasterMapStage.variations[variationId];

        if(stageVariation.tilePatchVAO)
        {
            renderAPI->glDeleteVertexArrays_wrapper(1, &stageVariation.tilePatchVAO);
            GL_CHECK_RESULT;
            stageVariation.tilePatchVAO = 0;
        }
    }

    if(_rasterMapStage.tilePatchIBO)
    {
        glDeleteBuffers(1, &_skyStage.skyplaneIBO);
        GL_CHECK_RESULT;
        _skyStage.skyplaneIBO = 0;
    }

    if(_rasterMapStage.tilePatchVBO)
    {
        glDeleteBuffers(1, &_rasterMapStage.tilePatchVBO);
        GL_CHECK_RESULT;
        _rasterMapStage.tilePatchVBO = 0;
    }
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::initializeRasterMapStage()
{
    const auto renderAPI = getRenderAPI();

    const auto& vertexShader = QString::fromLatin1(
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "INPUT vec2 in_vs_vertexTexCoords;                                                                                  ""\n"
        "#if !VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                ""\n"
        "    INPUT float in_vs_vertexElevation;                                                                             ""\n"
        "#endif // !VERTEX_TEXTURE_FETCH_SUPPORTED                                                                          ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "PARAM_OUTPUT vec2 v2f_texCoordsPerLayer[%RasterLayersCount%];                                                      ""\n"
        "PARAM_OUTPUT float v2f_mipmapLOD;                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mProjectionView;                                                                             ""\n"
        "uniform vec2 param_vs_targetInTilePosN;                                                                            ""\n"
        "uniform ivec2 param_vs_targetTile;                                                                                 ""\n"
        "uniform float param_vs_distanceFromCameraToTarget;                                                                 ""\n"
        "uniform float param_vs_cameraElevationAngleN;                                                                      ""\n"
        "uniform vec2 param_vs_groundCameraPosition;                                                                        ""\n"
        "uniform float param_vs_scaleToRetainProjectedSize;                                                                 ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-tile data
        "uniform ivec2 param_vs_tile;                                                                                       ""\n"
        "uniform float param_vs_elevationData_k;                                                                            ""\n"
        "uniform float param_vs_elevationData_upperMetersPerUnit;                                                           ""\n"
        "uniform float param_vs_elevationData_lowerMetersPerUnit;                                                           ""\n"
        "#if VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                 ""\n"
        "    uniform sampler2D param_vs_elevationData_sampler;                                                              ""\n"
        "#endif // VERTEX_TEXTURE_FETCH_SUPPORTED                                                                           ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-layer-in-tile data
        "struct RasterLayerTile                                                                                             ""\n"
        "{                                                                                                                  ""\n"
        "    float tileSizeN;                                                                                               ""\n"
        "    float tilePaddingN;                                                                                            ""\n"
        "    int slotsPerSide;                                                                                              ""\n"
        "    int slotIndex;                                                                                                 ""\n"
        "};                                                                                                                 ""\n"
        "uniform RasterLayerTile param_vs_rasterTileLayers[%RasterLayersCount%];                                            ""\n"
        "#if VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                 ""\n"
        "    uniform RasterLayerTile param_vs_elevationTileLayer;                                                           ""\n"
        "#endif // !VERTEX_TEXTURE_FETCH_SUPPORTED                                                                          ""\n"
        "                                                                                                                   ""\n"
        "void calculateTextureCoordinates(in RasterLayerTile tileLayer, out vec2 outTexCoords)                              ""\n"
        "{                                                                                                                  ""\n"
        "    int rowIndex = tileLayer.slotIndex / tileLayer.slotsPerSide;                                                   ""\n"
        "    int colIndex = tileLayer.slotIndex - rowIndex * tileLayer.slotsPerSide;                                        ""\n"
        "                                                                                                                   ""\n"
        "    float texCoordRescale = (tileLayer.tileSizeN - 2.0 * tileLayer.tilePaddingN) / tileLayer.tileSizeN;            ""\n"
        "                                                                                                                   ""\n"
        "    outTexCoords.s = float(colIndex) * tileLayer.tileSizeN;                                                        ""\n"
        "    outTexCoords.s += tileLayer.tilePaddingN + (in_vs_vertexTexCoords.s * tileLayer.tileSizeN) * texCoordRescale;  ""\n"
        "                                                                                                                   ""\n"
        "    outTexCoords.t = float(rowIndex) * tileLayer.tileSizeN;                                                        ""\n"
        "    outTexCoords.t += tileLayer.tilePaddingN + (in_vs_vertexTexCoords.t * tileLayer.tileSizeN) * texCoordRescale;  ""\n"
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
        "%UnrolledPerRasterLayerTexCoordsProcessingCode%                                                                    ""\n"
        "                                                                                                                   ""\n"
        //   If elevation data is active, use it
        "    if(abs(param_vs_elevationData_k) > 0.0)                                                                        ""\n"
        "    {                                                                                                              ""\n"
        "        float metersToUnits = mix(param_vs_elevationData_upperMetersPerUnit,                                       ""\n"
        "            param_vs_elevationData_lowerMetersPerUnit, in_vs_vertexTexCoords.t);                                   ""\n"
        "                                                                                                                   ""\n"
        //       Calculate texcoords for elevation data (pixel-is-area)
        "        float heightInMeters;                                                                                      ""\n"
        "#if VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                 ""\n"
        "        vec2 elevationDataTexCoords;                                                                               ""\n"
        "        calculateTextureCoordinates(                                                                               ""\n"
        "            param_vs_elevationTileLayer,                                                                           ""\n"
        "            elevationDataTexCoords);                                                                               ""\n"
        "        heightInMeters = SAMPLE_TEXTURE_2D(param_vs_elevationData_sampler, elevationDataTexCoords).r;              ""\n"
        "#else // !VERTEX_TEXTURE_FETCH_SUPPORTED                                                                           ""\n"
        "        heightInMeters = in_vs_vertexElevation;                                                                    ""\n"
        "#endif // VERTEX_TEXTURE_FETCH_SUPPORTED                                                                           ""\n"
        "                                                                                                                   ""\n"
        "        v.y = heightInMeters / metersToUnits;                                                                      ""\n"
        "        v.y *= param_vs_elevationData_k;                                                                           ""\n"
        "    }                                                                                                              ""\n"
        "                                                                                                                   ""\n"
        //   Calculate mipmap LOD
        "    vec2 groundVertex = v.xz;                                                                                      ""\n"
        "    vec2 groundCameraToVertex = groundVertex - param_vs_groundCameraPosition;                                      ""\n"
        "    float mipmapK = log(1.0 + 10.0 * log2(1.0 + param_vs_cameraElevationAngleN));                                  ""\n"
        "    float mipmapBaseLevelEndDistance = mipmapK * param_vs_distanceFromCameraToTarget;                              ""\n"
        "    v2f_mipmapLOD = 1.0 + (length(groundCameraToVertex) - mipmapBaseLevelEndDistance)                              ""\n"
        "        / (param_vs_scaleToRetainProjectedSize * %TileSize3D%.0);                                                  ""\n"
        "                                                                                                                   ""\n"
        //   Finally output processed modified vertex
        "    gl_Position = param_vs_mProjectionView * v;                                                                    ""\n"
        "}                                                                                                                  ""\n");
    const auto& vertexShader_perRasterLayerTexCoordsProcessing = QString::fromLatin1(
        "    calculateTextureCoordinates(                                                                                   ""\n"
        "        param_vs_rasterTileLayers[%rasterLayerId%],                                                                ""\n"
        "        v2f_texCoordsPerLayer[%rasterLayerId%]);                                                                   ""\n"
        "                                                                                                                   ""\n");

    const auto& fragmentShader = QString::fromLatin1(
        // Input data
        "PARAM_INPUT vec2 v2f_texCoordsPerLayer[%RasterLayersCount%];                                                       ""\n"
        "PARAM_INPUT float v2f_mipmapLOD;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-layer data
        "struct RasterLayerTile                                                                                             ""\n"
        "{                                                                                                                  ""\n"
        "    lowp float k;                                                                                                  ""\n"
        "    lowp sampler2D sampler;                                                                                        ""\n"
        "};                                                                                                                 ""\n"
        "uniform RasterLayerTile param_fs_rasterTileLayers[%RasterLayersCount%];                                            ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    lowp vec4 finalColor;                                                                                          ""\n"
        "                                                                                                                   ""\n"
        //   Mix colors of all layers
        "    finalColor = SAMPLE_TEXTURE_2D_LOD(                                                                            ""\n"
        "        param_fs_rasterTileLayers[0].sampler,                                                                      ""\n"
        "        v2f_texCoordsPerLayer[0], v2f_mipmapLOD);                                                                  ""\n"
        "    finalColor.a *= param_fs_rasterTileLayers[0].k;                                                                ""\n"
        "%UnrolledPerRasterLayerProcessingCode%                                                                             ""\n"
        "                                                                                                                   ""\n"
#if 0
        //   NOTE: Useful for debugging mipmap levels
        "    {                                                                                                              ""\n"
        "        vec4 mipmapDebugColor;                                                                                     ""\n"
        "        mipmapDebugColor.a = 1.0;                                                                                  ""\n"
        "        float value = v2f_mipmapLOD;                                                                               ""\n"
        //"        float value = textureQueryLod(param_vs_rasterTileLayers[0].sampler, v2f_texCoordsPerLayer[0]).x;           ""\n"
        "        mipmapDebugColor.r = clamp(value, 0.0, 1.0);                                                               ""\n"
        "        value -= 1.0;                                                                                              ""\n"
        "        mipmapDebugColor.g = clamp(value, 0.0, 1.0);                                                               ""\n"
        "        value -= 1.0;                                                                                              ""\n"
        "        mipmapDebugColor.b = clamp(value, 0.0, 1.0);                                                               ""\n"
        "        finalColor = mix(finalColor, mipmapDebugColor, 0.5);                                                       ""\n"
        "    }                                                                                                              ""\n"
#endif
        "    FRAGMENT_COLOR_OUTPUT = finalColor;                                                                            ""\n"
        "}                                                                                                                  ""\n");
    const auto& fragmentShader_perRasterLayer = QString::fromLatin1(
        "    {                                                                                                              ""\n"
        "        lowp vec4 layerColor = SAMPLE_TEXTURE_2D_LOD(                                                              ""\n"
        "            param_fs_rasterTileLayers[%rasterLayerId%].sampler,                                                    ""\n"
        "            v2f_texCoordsPerLayer[%rasterLayerId%], v2f_mipmapLOD);                                                ""\n"
        "                                                                                                                   ""\n"
        "        layerColor.a *= param_fs_rasterTileLayers[%rasterLayerId%].k;                                              ""\n"
        "        finalColor = mix(finalColor, layerColor, layerColor.a);                                                    ""\n"
        "    }                                                                                                              ""\n");
    for(auto variationId = 0u, maxActiveMapLayers = 1u; variationId < RasterMapLayersCount; variationId++, maxActiveMapLayers++)
    {
        auto& stageVariation = _rasterMapStage.variations[variationId];

        // Compile vertex shader
        auto preprocessedVertexShader = vertexShader;
        QString preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsProcessingCode;
        for(int layerId = 0; layerId < maxActiveMapLayers; layerId++)
        {
            auto preprocessedVertexShader_perRasterLayerTexCoordsProcessing = vertexShader_perRasterLayerTexCoordsProcessing;
            preprocessedVertexShader_perRasterLayerTexCoordsProcessing.replace("%rasterLayerId%", QString::number(layerId));

            preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsProcessingCode +=
                preprocessedVertexShader_perRasterLayerTexCoordsProcessing;
        }
        preprocessedVertexShader.replace("%UnrolledPerRasterLayerTexCoordsProcessingCode%",
            preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsProcessingCode);
        preprocessedVertexShader.replace("%TileSize3D%", QString::number(TileSize3D));
        preprocessedVertexShader.replace("%RasterLayersCount%", QString::number(maxActiveMapLayers));
        renderAPI->preprocessVertexShader(preprocessedVertexShader);
        renderAPI->optimizeVertexShader(preprocessedVertexShader);
        stageVariation.vs.id = renderAPI->compileShader(GL_VERTEX_SHADER, preprocessedVertexShader.toStdString().c_str());
        assert(stageVariation.vs.id != 0);

        // Compile fragment shader
        auto preprocessedFragmentShader = fragmentShader;
        QString preprocessedFragmentShader_UnrolledPerRasterLayerProcessingCode;
        for(int layerId = static_cast<int>(RasterMapLayerId::BaseLayer) + 1; layerId < maxActiveMapLayers; layerId++)
        {
            auto linearIdx = layerId - static_cast<int>(RasterMapLayerId::BaseLayer);
            auto preprocessedFragmentShader_perRasterLayer = fragmentShader_perRasterLayer;
            preprocessedFragmentShader_perRasterLayer.replace("%rasterLayerId%", QString::number(linearIdx));

            preprocessedFragmentShader_UnrolledPerRasterLayerProcessingCode += preprocessedFragmentShader_perRasterLayer;
        }
        preprocessedFragmentShader.replace("%UnrolledPerRasterLayerProcessingCode%", preprocessedFragmentShader_UnrolledPerRasterLayerProcessingCode);
        preprocessedFragmentShader.replace("%RasterLayersCount%", QString::number(maxActiveMapLayers));
        renderAPI->preprocessFragmentShader(preprocessedFragmentShader);
        renderAPI->optimizeFragmentShader(preprocessedFragmentShader);
        stageVariation.fs.id = renderAPI->compileShader(GL_FRAGMENT_SHADER, preprocessedFragmentShader.toStdString().c_str());
        assert(stageVariation.fs.id != 0);

        // Link everything into program object
        GLuint shaders[] = {
            stageVariation.vs.id,
            stageVariation.fs.id
        };
        stageVariation.program = getRenderAPI()->linkProgram(2, shaders);
        assert(stageVariation.program != 0);

        renderAPI->clearVariablesLookup();
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.in.vertexPosition, "in_vs_vertexPosition", GLShaderVariableType::In);
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.in.vertexTexCoords, "in_vs_vertexTexCoords", GLShaderVariableType::In);
        if(!renderAPI->isSupported_vertexShaderTextureLookup)
        {
            renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.in.vertexElevation, "in_vs_vertexElevation", GLShaderVariableType::In);
        }
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.mProjectionView, "param_vs_mProjectionView", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.targetInTilePosN, "param_vs_targetInTilePosN", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.targetTile, "param_vs_targetTile", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.distanceFromCameraToTarget, "param_vs_distanceFromCameraToTarget", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.cameraElevationAngleN, "param_vs_cameraElevationAngleN", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.groundCameraPosition, "param_vs_groundCameraPosition", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.scaleToRetainProjectedSize, "param_vs_scaleToRetainProjectedSize", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.tile, "param_vs_tile", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.elevationData_k, "param_vs_elevationData_k", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.elevationData_upperMetersPerUnit, "param_vs_elevationData_upperMetersPerUnit", GLShaderVariableType::Uniform);
        renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.elevationData_lowerMetersPerUnit, "param_vs_elevationData_lowerMetersPerUnit", GLShaderVariableType::Uniform);
        if(renderAPI->isSupported_vertexShaderTextureLookup)
        {
            renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.elevationData_sampler, "param_vs_elevationData_sampler", GLShaderVariableType::Uniform);
            renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.elevationTileLayer.tileSizeN, "param_vs_elevationTileLayer.tileSizeN", GLShaderVariableType::Uniform);
            renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.elevationTileLayer.tilePaddingN, "param_vs_elevationTileLayer.tilePaddingN", GLShaderVariableType::Uniform);
            renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.elevationTileLayer.slotsPerSide, "param_vs_elevationTileLayer.slotsPerSide", GLShaderVariableType::Uniform);
            renderAPI->findVariableLocation(stageVariation.program, stageVariation.vs.param.elevationTileLayer.slotIndex, "param_vs_elevationTileLayer.slotIndex", GLShaderVariableType::Uniform);
        }
        for(int layerId = 0; layerId < maxActiveMapLayers; layerId++)
        {
            // Vertex shader
            {
                auto layerStructName =
                    QString::fromLatin1("param_vs_rasterTileLayers[%layerId%]")
                    .replace(QString::fromLatin1("%layerId%"), QString::number(layerId));
                auto& layerStruct = stageVariation.vs.param.rasterTileLayers[layerId];

                renderAPI->findVariableLocation(stageVariation.program, layerStruct.tileSizeN, layerStructName + ".tileSizeN", GLShaderVariableType::Uniform);
                renderAPI->findVariableLocation(stageVariation.program, layerStruct.tilePaddingN, layerStructName + ".tilePaddingN", GLShaderVariableType::Uniform);
                renderAPI->findVariableLocation(stageVariation.program, layerStruct.slotsPerSide, layerStructName + ".slotsPerSide", GLShaderVariableType::Uniform);
                renderAPI->findVariableLocation(stageVariation.program, layerStruct.slotIndex, layerStructName + ".slotIndex", GLShaderVariableType::Uniform);
            }

            // Fragment shader
            {
                auto layerStructName =
                    QString::fromLatin1("param_fs_rasterTileLayers[%layerId%]")
                    .replace(QString::fromLatin1("%layerId%"), QString::number(layerId));
                auto& layerStruct = stageVariation.fs.param.rasterTileLayers[layerId];

                renderAPI->findVariableLocation(stageVariation.program, layerStruct.k, layerStructName + ".k", GLShaderVariableType::Uniform);
                renderAPI->findVariableLocation(stageVariation.program, layerStruct.sampler, layerStructName + ".sampler", GLShaderVariableType::Uniform);
            }
        }
        renderAPI->clearVariablesLookup();
    }
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::renderRasterMapStage()
{
    const auto renderAPI = getRenderAPI();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glUniform1i);
    GL_CHECK_PRESENT(glUniform2i);
    GL_CHECK_PRESENT(glUniform2fv);
    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);
    GL_CHECK_PRESENT(glDisableVertexAttribArray);

    // Count active raster tile providers
    auto activeRasterTileProvidersCount = 1;
    for(int layerIdx = 1, layerId = static_cast<int>(RasterMapLayerId::BaseLayer) + 1; layerIdx < RasterMapLayersCount; layerIdx++, layerId++)
    {
        if(!currentState.rasterLayerProviders[layerId])
            continue;

        activeRasterTileProvidersCount++;
    }

    // Get proper program variation
    const auto& stageVariation = _rasterMapStage.variations[activeRasterTileProvidersCount - 1];
    
    // Set tile patch VAO
    renderAPI->glBindVertexArray_wrapper(stageVariation.tilePatchVAO);
    GL_CHECK_RESULT;

    // Activate program
    glUseProgram(stageVariation.program);
    GL_CHECK_RESULT;

    // Set matrices
    auto mProjectionView = _mProjection * _mView;
    glUniformMatrix4fv(stageVariation.vs.param.mProjectionView, 1, GL_FALSE, glm::value_ptr(mProjectionView));
    GL_CHECK_RESULT;

    // Set center offset
    glUniform2f(stageVariation.vs.param.targetInTilePosN, _targetInTileOffsetN.x, _targetInTileOffsetN.y);
    GL_CHECK_RESULT;

    // Set target tile
    glUniform2i(stageVariation.vs.param.targetTile, _targetTileId.x, _targetTileId.y);
    GL_CHECK_RESULT;

    // Set distance from camera to target
    glUniform1f(stageVariation.vs.param.distanceFromCameraToTarget, _distanceFromCameraToTarget);
    GL_CHECK_RESULT;

    // Set normalized [0.0 .. 1.0] angle of camera elevation
    glUniform1f(stageVariation.vs.param.cameraElevationAngleN, currentState.elevationAngle / 90.0f);
    GL_CHECK_RESULT;

    // Set position of camera in ground place
    glUniform2fv(stageVariation.vs.param.groundCameraPosition, 1, glm::value_ptr(_groundCameraPosition));
    GL_CHECK_RESULT;

    // Set scale to retain projected size
    glUniform1f(stageVariation.vs.param.scaleToRetainProjectedSize, _scaleToRetainProjectedSize);
    GL_CHECK_RESULT;

    // Configure samplers
    if(renderAPI->isSupported_vertexShaderTextureLookup)
    {
        glUniform1i(stageVariation.vs.param.elevationData_sampler, 0);
        GL_CHECK_RESULT;

        renderAPI->setSampler(GL_TEXTURE0, RenderAPI_OpenGL_Common::SamplerType::ElevationDataTile);
    }
    auto bitmapTileSamplerType = RenderAPI_OpenGL_Common::SamplerType::BitmapTile_Bilinear;
    switch(currentConfiguration.texturesFilteringQuality)
    {
    case TextureFilteringQuality::Good:
        bitmapTileSamplerType = RenderAPI_OpenGL_Common::SamplerType::BitmapTile_BilinearMipmap;
        break;
    case TextureFilteringQuality::Best:
        bitmapTileSamplerType = RenderAPI_OpenGL_Common::SamplerType::BitmapTile_TrilinearMipmap;
        break;
    }
    for(int layerIdx = 0, layerId = static_cast<int>(RasterMapLayerId::BaseLayer); layerIdx < activeRasterTileProvidersCount; layerIdx++, layerId++)
    {
        const auto samplerIndex = (renderAPI->isSupported_vertexShaderTextureLookup ? 1 : 0) + layerId;

        glUniform1i(stageVariation.fs.param.rasterTileLayers[layerId].sampler, samplerIndex);
        GL_CHECK_RESULT;

        renderAPI->setSampler(GL_TEXTURE0 + samplerIndex, bitmapTileSamplerType);
    }

    // Check if we need to process elevation data
    const bool elevationDataEnabled = static_cast<bool>(currentState.elevationDataProvider);
    if(!elevationDataEnabled)
    {
        // We have no elevation data provider, so we can not do anything
        glUniform1f(stageVariation.vs.param.elevationData_k, 0.0f);
        GL_CHECK_RESULT;
    }
    bool elevationVertexAttribArrayEnabled = false;
    if(!renderAPI->isSupported_vertexShaderTextureLookup)
    {
        glDisableVertexAttribArray(stageVariation.vs.in.vertexElevation);
        GL_CHECK_RESULT;
    }

    // For each visible tile, render it
    for(auto itTileId = _visibleTiles.cbegin(); itTileId != _visibleTiles.cend(); ++itTileId)
    {
        const auto& tileId = *itTileId;

        // Get normalized tile index
        auto tileIdN = Utilities::normalizeTileId(tileId, currentState.zoomBase);

        // Set tile id
        glUniform2i(stageVariation.vs.param.tile, tileId.x, tileId.y);
        GL_CHECK_RESULT;

        // Set elevation data
        bool appliedElevationVertexAttribArray = false;
        if(elevationDataEnabled)
        {
            // We're obtaining tile entry by normalized tile coordinates, since tile may repeat several times
            std::shared_ptr<TiledResourceEntry> tileEntry;
            tiledResources[TiledResourceType::ElevationData]->obtainTileEntry(tileEntry, tileIdN, currentState.zoomBase);

            // Try lock tile entry for reading state and obtaining GPU resource
            std::shared_ptr< RenderAPI::ResourceInGPU > gpuResource;
            if(tileEntry && tileEntry->stateLock.tryLockForRead())
            {
                if(tileEntry->state == ResourceState::Uploaded)
                    gpuResource = tileEntry->resourceInGPU;
                tileEntry->stateLock.unlock();
            }

            if(!gpuResource)
            {
                // We have no elevation data, so we can not do anything
                glUniform1f(stageVariation.vs.param.elevationData_k, 0.0f);
                GL_CHECK_RESULT;
            }
            else
            {
                glUniform1f(stageVariation.vs.param.elevationData_k, currentState.elevationDataScaleFactor);
                GL_CHECK_RESULT;

                auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(currentState.zoomBase, tileIdN.y, TileSize3D);
                glUniform1f(stageVariation.vs.param.elevationData_upperMetersPerUnit, upperMetersPerUnit);
                auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(currentState.zoomBase, tileIdN.y + 1, TileSize3D);
                glUniform1f(stageVariation.vs.param.elevationData_lowerMetersPerUnit, lowerMetersPerUnit);

                const auto& perTile_vs = stageVariation.vs.param.elevationTileLayer;

                if(renderAPI->isSupported_vertexShaderTextureLookup)
                {
                    glActiveTexture(GL_TEXTURE0);
                    GL_CHECK_RESULT;

                    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
                    GL_CHECK_RESULT;

                    if(gpuResource->type == RenderAPI::ResourceInGPU::TileOnAtlasTexture)
                    {
                        const auto& tileOnAtlasTexture = std::static_pointer_cast<RenderAPI::TileOnAtlasTextureInGPU>(gpuResource);

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
                        const auto& texture = std::static_pointer_cast<RenderAPI::TextureInGPU>(gpuResource);

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
                else
                {
                    assert(gpuResource->type == RenderAPI::ResourceInGPU::ArrayBuffer);

                    const auto& arrayBuffer = std::static_pointer_cast<RenderAPI::ArrayBufferInGPU>(gpuResource);
                    assert(arrayBuffer->itemsCount == currentConfiguration.heixelsPerTileSide*currentConfiguration.heixelsPerTileSide);

                    if(!elevationVertexAttribArrayEnabled)
                    {
                        glEnableVertexAttribArray(stageVariation.vs.in.vertexElevation);
                        GL_CHECK_RESULT;

                        elevationVertexAttribArrayEnabled = true;
                    }

                    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
                    GL_CHECK_RESULT;
                    
                    glVertexAttribPointer(stageVariation.vs.in.vertexElevation, 1, GL_FLOAT, GL_FALSE, sizeof(float), nullptr);
                    GL_CHECK_RESULT;
                    appliedElevationVertexAttribArray = true;
                }
            }
        }

        // We need to pass each layer of this tile to shader
        for(int layerIdx = 0, layerId = static_cast<int>(RasterMapLayerId::BaseLayer), layerLinearIdx = -1; layerIdx < RasterMapLayersCount; layerIdx++, layerId++)
        {
            if(!currentState.rasterLayerProviders[layerId])
                continue;
            layerLinearIdx++;

            const auto& layerResources = tiledResources[RasterBaseLayer + layerId];
            const auto& perTile_vs = stageVariation.vs.param.rasterTileLayers[layerLinearIdx];
            const auto& perTile_fs = stageVariation.fs.param.rasterTileLayers[layerLinearIdx];
            const auto samplerIndex = (renderAPI->isSupported_vertexShaderTextureLookup ? 1 : 0) + layerLinearIdx;

            // We're obtaining tile entry by normalized tile coordinates, since tile may repeat several times
            std::shared_ptr<TiledResourceEntry> tileEntry;
            layerResources->obtainTileEntry(tileEntry, tileIdN, currentState.zoomBase);

            // Try lock tile entry for reading state and obtaining GPU resource
            std::shared_ptr< RenderAPI::ResourceInGPU > gpuResource;
            if(tileEntry && tileEntry->stateLock.tryLockForRead())
            {
                if(tileEntry->state == ResourceState::Uploaded)
                    gpuResource = tileEntry->resourceInGPU;
                else if(tileEntry->state == ResourceState::Unavailable)
                    gpuResource = _unavailableTileStub;
                else
                    gpuResource = _processingTileStub;

                tileEntry->stateLock.unlock();
            }

            glUniform1f(perTile_fs.k, currentState.rasterLayerOpacity[layerId]);
            GL_CHECK_RESULT;

            if(!gpuResource)
                continue;
            
            glActiveTexture(GL_TEXTURE0 + samplerIndex);
            GL_CHECK_RESULT;

            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
            GL_CHECK_RESULT;

            if(gpuResource->type == RenderAPI::ResourceInGPU::TileOnAtlasTexture)
            {
                const auto& tileOnAtlasTexture = std::static_pointer_cast<RenderAPI::TileOnAtlasTextureInGPU>(gpuResource);

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

        if(!appliedElevationVertexAttribArray && elevationVertexAttribArrayEnabled)
        {
            elevationVertexAttribArrayEnabled = false;

            glDisableVertexAttribArray(stageVariation.vs.in.vertexElevation);
            GL_CHECK_RESULT;
        }

        glDrawElements(GL_TRIANGLES, _tilePatchIndicesCount, GL_UNSIGNED_SHORT, nullptr);
        GL_CHECK_RESULT;
    }

    // Disable textures
    for(int layerIdx = 0; layerIdx < (renderAPI->isSupported_vertexShaderTextureLookup ? 1 : 0) + activeRasterTileProvidersCount; layerIdx++)
    {
        glActiveTexture(GL_TEXTURE0 + layerIdx);
        GL_CHECK_RESULT;

        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_RESULT;
    }

    // Unbind any binded buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    // Disable elevation vertex attrib array (if enabled)
    if(elevationVertexAttribArrayEnabled)
    {
        glDisableVertexAttribArray(stageVariation.vs.in.vertexElevation);
        GL_CHECK_RESULT;
    }

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    // Deselect VAO
    renderAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::releaseRasterMapStage()
{
    GL_CHECK_PRESENT(glDeleteProgram);
    GL_CHECK_PRESENT(glDeleteShader);

    for(auto variationId = 0u, maxActiveMapLayers = 1u; variationId < RasterMapLayersCount; variationId++, maxActiveMapLayers++)
    {
        auto& stageVariation = _rasterMapStage.variations[variationId];
    
        if(stageVariation.program)
        {
            glDeleteProgram(stageVariation.program);
            GL_CHECK_RESULT;
        }
        if(stageVariation.fs.id)
        {
            glDeleteShader(stageVariation.fs.id);
            GL_CHECK_RESULT;
        }
        if(stageVariation.vs.id)
        {
            glDeleteShader(stageVariation.vs.id);
            GL_CHECK_RESULT;
        }
    }

    releaseTilePatch();

    memset(&_rasterMapStage, 0, sizeof(_rasterMapStage));
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
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mProjectionViewModel;                                                                        ""\n"
        "uniform vec2 param_vs_halfSize;                                                                                    ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    vec4 v;                                                                                                        ""\n"
        "    v.xy = in_vs_vertexPosition * param_vs_halfSize;                                                               ""\n"
        "    v.w = 1.0;                                                                                                     ""\n"
        "                                                                                                                   ""\n"
        "    gl_Position = param_vs_mProjectionViewModel * v;                                                               ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    renderAPI->preprocessVertexShader(preprocessedVertexShader);
    renderAPI->optimizeVertexShader(preprocessedVertexShader);
    _skyStage.vs.id = renderAPI->compileShader(GL_VERTEX_SHADER, preprocessedVertexShader.toStdString().c_str());
    assert(_skyStage.vs.id != 0);

    // Compile fragment shader
    const QString fragmentShader = QString::fromLatin1(
        // Parameters: common data
        "uniform lowp vec4 param_fs_skyColor;                                                                               ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    FRAGMENT_COLOR_OUTPUT = param_fs_skyColor;                                                                     ""\n"
        "}                                                                                                                  ""\n");
    QString preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    renderAPI->preprocessFragmentShader(preprocessedFragmentShader);
    renderAPI->optimizeFragmentShader(preprocessedFragmentShader);
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
    renderAPI->findVariableLocation(_skyStage.program, _skyStage.fs.param.skyColor, "param_fs_skyColor", GLShaderVariableType::Uniform);
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

    // Set sky parameters
    glUniform4f(_skyStage.fs.param.skyColor, currentState.skyColor.r, currentState.skyColor.g, currentState.skyColor.b, 1.0f);
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
    GL_CHECK_PRESENT(glClearColor);
    GL_CHECK_PRESENT(glClearDepthf);

    bool ok;

    ok = AtlasMapRenderer::doInitializeRendering();
    if(!ok)
        return false;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    GL_CHECK_RESULT;

    glClearDepthf(1.0f);
    GL_CHECK_RESULT;

    initializeSkyStage();
    initializeRasterMapStage();

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL_Common::doRenderFrame()
{
    GL_CHECK_PRESENT(glViewport);
    GL_CHECK_PRESENT(glEnable);
    GL_CHECK_PRESENT(glDisable);
    GL_CHECK_PRESENT(glBlendFunc);
    GL_CHECK_PRESENT(glClear);

    // Setup viewport
    glViewport(
        currentState.viewport.left,
        currentState.windowSize.y - currentState.viewport.bottom,
        currentState.viewport.width(),
        currentState.viewport.height());
    GL_CHECK_RESULT;

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_RESULT;

    // Turn off blending for sky and map stages
    glDisable(GL_BLEND);
    GL_CHECK_RESULT;

    // Turn off depth testing for sky stage
    glEnable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;
    glDepthFunc(GL_LEQUAL);
    GL_CHECK_RESULT;

    renderSkyStage();

    // Turn on depth prior to raster map stage and further stages
    glEnable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;
    glDepthFunc(GL_LEQUAL);
    GL_CHECK_RESULT;

    // Raster map stage is rendered without blending, since it's done in fragment shader
    renderRasterMapStage();

    // Turn on blending since now objects with transparency are going to be rendered
    glEnable(GL_BLEND);
    GL_CHECK_RESULT;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_RESULT;

    //TODO: render special fog object some day

    // If we have offline map data provider, render text?, icons? (actually, map markers should be available w/o offline data - favorites, etc.)


    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL_Common::doReleaseRendering()
{
    bool ok;

    releaseRasterMapStage();
    releaseSkyStage();

    ok = AtlasMapRenderer::doReleaseRendering();
    if(!ok)
        return false;

    return true;
}

void OsmAnd::AtlasMapRenderer_OpenGL_Common::validateElevationDataResources()
{
    MapRenderer::validateElevationDataResources();

    // Recreate tile patch since elevation data influences density of tile patch
    releaseTilePatch();
    createTilePatch();
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
    const auto screenTileSize = getReferenceTileSizeOnScreen();
    _nearDistanceFromCameraToTarget = Utilities_OpenGL_Common::calculateCameraDistance(_mProjection, currentState.viewport, TileSize3D / 2.0f, screenTileSize / 2.0f, 1.5f);
    _baseDistanceFromCameraToTarget = Utilities_OpenGL_Common::calculateCameraDistance(_mProjection, currentState.viewport, TileSize3D / 2.0f, screenTileSize / 2.0f, 1.0f);
    _farDistanceFromCameraToTarget = Utilities_OpenGL_Common::calculateCameraDistance(_mProjection, currentState.viewport, TileSize3D / 2.0f, screenTileSize / 2.0f, 0.75f);

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

    // Get camera position on the ground
    _groundCameraPosition = (_mAzimuthInv * glm::vec4(0.0f, 0.0f, _distanceFromCameraToTarget, 1.0f)).xz;

    // Correct fog distance
    _correctedFogDistance = currentState.fogDistance * _scaleToRetainProjectedSize + (_distanceFromCameraToTarget - _groundDistanceFromCameraToTarget);

    // Calculate skyplane size
    float zSkyplaneK = _zSkyplane / _zNear;
    _skyplaneHalfSize.x = zSkyplaneK * _projectionPlaneHalfWidth;
    _skyplaneHalfSize.y = zSkyplaneK * _projectionPlaneHalfHeight;

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
    intersectionPoints[0] /= static_cast<float>(TileSize3D);
    intersectionPoints[1] /= static_cast<float>(TileSize3D);
    intersectionPoints[2] /= static_cast<float>(TileSize3D);
    intersectionPoints[3] /= static_cast<float>(TileSize3D);

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
    
    if(!currentState.elevationDataProvider)
    {
        // Simple tile patch, that consists of 4 vertices

        // Vertex data
        const GLfloat tsz = static_cast<GLfloat>(TileSize3D);
        static MapTileVertex vertices[4] =
        {
            { {0.0f, 0.0f}, {0.0f, 0.0f} },
            { {0.0f,  tsz}, {0.0f, 1.0f} },
            { { tsz,  tsz}, {1.0f, 1.0f} },
            { { tsz, 0.0f}, {1.0f, 0.0f} }
        };
        pVertices = new MapTileVertex[4];
        memcpy(pVertices, vertices, 4*sizeof(MapTileVertex));
        verticesCount = 4;

        // Index data
        static GLushort indices[6] =
        {
            0, 1, 2,
            0, 2, 3
        };
        pIndices = new GLushort[6];
        memcpy(pIndices, indices, 6*sizeof(GLushort));
        indicesCount = 6;
    }
    else
    {
        // Complex tile patch, that consists of (TileElevationNodesPerSide*TileElevationNodesPerSide) number of
        // height clusters. Height cluster itself consists of 4 vertices, 6 indices and 2 polygons
        const auto heightPrimitivesPerSide = currentConfiguration.heixelsPerTileSide - 1;
        const GLfloat clusterSize = static_cast<GLfloat>(TileSize3D) / static_cast<float>(heightPrimitivesPerSide);
        verticesCount = currentConfiguration.heixelsPerTileSide * currentConfiguration.heixelsPerTileSide;
        pVertices = new MapTileVertex[verticesCount];
        indicesCount = (heightPrimitivesPerSide * heightPrimitivesPerSide) * 6;
        pIndices = new GLushort[indicesCount];

        MapTileVertex* pV = pVertices;

        // Form vertices
        assert(verticesCount <= std::numeric_limits<GLushort>::max());
        for(auto row = 0u; row < currentConfiguration.heixelsPerTileSide; row++)
        {
            for(auto col = 0u; col < currentConfiguration.heixelsPerTileSide; col++, pV++)
            {
                pV->position[0] = static_cast<float>(col) * clusterSize;
                pV->position[1] = static_cast<float>(row) * clusterSize;

                pV->uv[0] = static_cast<float>(col) / static_cast<float>(heightPrimitivesPerSide);
                pV->uv[1] = static_cast<float>(row) / static_cast<float>(heightPrimitivesPerSide);
            }
        }

        // Form indices
        GLushort* pI = pIndices;
        for(auto row = 0u; row < heightPrimitivesPerSide; row++)
        {
            for(auto col = 0u; col < heightPrimitivesPerSide; col++)
            {
                // p1 - top left
                // p2 - bottom left
                // p3 - bottom right
                // p4 - top right
                const auto p1 = (row + 0) * currentConfiguration.heixelsPerTileSide + col + 0;
                const auto p2 = (row + 1) * currentConfiguration.heixelsPerTileSide + col + 0;
                const auto p3 = (row + 1) * currentConfiguration.heixelsPerTileSide + col + 1;
                const auto p4 = (row + 0) * currentConfiguration.heixelsPerTileSide + col + 1;
                assert(p1 <= verticesCount);
                assert(p2 <= verticesCount);
                assert(p3 <= verticesCount);
                assert(p4 <= verticesCount);

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

    delete[] pVertices;
    delete[] pIndices;
}

OsmAnd::RenderAPI_OpenGL_Common* OsmAnd::AtlasMapRenderer_OpenGL_Common::getRenderAPI() const
{
    return static_cast<OsmAnd::RenderAPI_OpenGL_Common*>(renderAPI.get());
}

float OsmAnd::AtlasMapRenderer_OpenGL_Common::getReferenceTileSizeOnScreen()
{
    const auto& rasterMapProvider = currentState.rasterLayerProviders[static_cast<int>(RasterMapLayerId::BaseLayer)];
    if(!rasterMapProvider)
        return static_cast<float>(DefaultReferenceTileSizeOnScreen) * setupOptions.displayDensityFactor;

    auto tileProvider = std::static_pointer_cast<IMapBitmapTileProvider>(rasterMapProvider);
    return tileProvider->getTileSize() * (setupOptions.displayDensityFactor / tileProvider->getTileDensity());
}

float OsmAnd::AtlasMapRenderer_OpenGL_Common::getScaledTileSizeOnScreen()
{
    return getReferenceTileSizeOnScreen() * _tileScaleFactor;
}

bool OsmAnd::AtlasMapRenderer_OpenGL_Common::getLocationFromScreenPoint( const PointI& screenPoint, PointI& location31 )
{
    glm::vec4 viewport(
        currentState.viewport.left,
        currentState.windowSize.y - currentState.viewport.bottom,
        currentState.viewport.width(),
        currentState.viewport.height());
    const auto nearInWorld = glm::unProject(glm::vec3(screenPoint.x, currentState.windowSize.y - screenPoint.y, 0.0f), _mView, _mProjection, viewport);
    const auto farInWorld = glm::unProject(glm::vec3(screenPoint.x, currentState.windowSize.y - screenPoint.y, 1.0f), _mView, _mProjection, viewport);
    const auto rayD = glm::normalize(farInWorld - nearInWorld);

    const glm::vec3 planeN(0.0f, 1.0f, 0.0f);
    const glm::vec3 planeO(0.0f, 0.0f, 0.0f);
    float distance;
    const auto intersects = Utilities_OpenGL_Common::rayIntersectPlane(planeN, planeO, rayD, nearInWorld, distance);
    if(!intersects)
        return false;

    auto intersection = nearInWorld + distance*rayD;
    intersection /= static_cast<float>(TileSize3D);
    auto tileWidth31 = (1u << (ZoomLevel::MaxZoomLevel - currentState.zoomBase)) - 1;
    intersection.x = (intersection.x + _targetInTileOffsetN.x + _targetTileId.x) * tileWidth31;
    intersection.z = (intersection.z + _targetInTileOffsetN.y + _targetTileId.y) * tileWidth31;
    location31.x = static_cast<int32_t>(intersection.x);
    location31.y = static_cast<int32_t>(intersection.z);
    location31 = Utilities::normalizeCoordinates(location31, ZoomLevel31);

    return true;
}
