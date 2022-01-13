#include "AtlasMapRendererMapLayersStage_OpenGL.h"

#include <cassert>
#include <algorithm>

#include "ignore_warnings_on_external_includes.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkColor.h>
#include "restore_internal_warnings.h"

#include "AtlasMapRenderer_OpenGL.h"
#include "AtlasMapRenderer_Metrics.h"
#include "IMapTiledDataProvider.h"
#include "IRasterMapLayerProvider.h"
#include "IMapElevationDataProvider.h"
#include "QKeyValueIterator.h"
#include "Utilities.h"

OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::AtlasMapRendererMapLayersStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer_)
    : AtlasMapRendererMapLayersStage(renderer_)
    , AtlasMapRendererStageHelper_OpenGL(this)
    , _maxNumberOfRasterMapLayersInBatch(0)
    , _rasterTileIndicesCount(-1)
{
}

OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::~AtlasMapRendererMapLayersStage_OpenGL()
{
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::initialize()
{
    bool ok = true;
    ok = ok && initializeRasterLayers();
    return ok;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* const metric_)
{
    const auto metric = dynamic_cast<AtlasMapRenderer_Metrics::Metric_renderFrame*>(metric_);
    bool ok = true;

    const auto& internalState = getInternalState();
    const auto gpuAPI = getGPUAPI();

    GL_PUSH_GROUP_MARKER(QLatin1String("mapLayers"));

    // First vector layer or first raster layers batch should be rendered without blending,
    // since blending is performed inside shader itself.
    bool blendingEnabled = false;
    glDisable(GL_BLEND);
    GL_CHECK_RESULT;

    // Initially, configure for premultiplied alpha channel type
    auto currentAlphaChannelType = AlphaChannelType::Premultiplied;
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_RESULT;

    GLname lastUsedProgram;
    GLlocation elevationDataVertexAttribArray;
    const auto& batchedLayersByTiles = batchLayersByTiles(internalState);
    for (const auto& batchedLayersByTile : constOf(batchedLayersByTiles))
    {
        // Any layer or layers batch after first one has to be rendered using blending,
        // since output color of new batch needs to be blended with destination color.
        if (!batchedLayersByTile->containsOriginLayer != blendingEnabled)
        {
            if (batchedLayersByTile->containsOriginLayer)
            {
                glDisable(GL_BLEND);
                GL_CHECK_RESULT;
            }
            else
            {
                glEnable(GL_BLEND);
                GL_CHECK_RESULT;
            }

            blendingEnabled = !batchedLayersByTile->containsOriginLayer;
        }

        // Depending on type of first (and all others) batched layer, batch is rendered differently
        if (batchedLayersByTile->layers.first()->type == BatchedLayerType::Raster)
        {
            renderRasterLayersBatch(
                batchedLayersByTile,
                currentAlphaChannelType,
                elevationDataVertexAttribArray,
                lastUsedProgram);
        }
    }

    // Deactivate program
    if (lastUsedProgram.isValid())
    {
        // Elevation vertex attrib is bound to program
        if (elevationDataVertexAttribArray.isValid())
        {
            glDisableVertexAttribArray(*elevationDataVertexAttribArray);
            GL_CHECK_RESULT;

            elevationDataVertexAttribArray.reset();
        }

        glUseProgram(0);
        GL_CHECK_RESULT;

        // Also un-use any possibly used VAO
        gpuAPI->unuseVAO();

        lastUsedProgram.reset();
    }

    GL_POP_GROUP_MARKER;

    return ok;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::release(const bool gpuContextLost)
{
    bool ok = true;
    ok = ok && releaseRasterLayers(gpuContextLost);
    return ok;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::initializeRasterLayers()
{
    const auto gpuAPI = getGPUAPI();

    // Determine maximum number of raster layers in one batch. It's determined by minimal of following numbers:
    //  - (maxVertexUniformVectors - alreadyOccupiedUniforms) / (vsUniformsPerLayer + fsUniformsPerLayer)
    //  - maxTextureUnitsInFragmentShader
    //  - setupOptions.maxNumberOfRasterMapLayersInBatch
    const auto vsUniformsPerLayer =
        1 /*texCoordsOffsetAndScale*/ +
        1 /*texelSize*/;
    const auto fsUniformsPerLayer =
        1 /*opacity*/ +
        1 /*isPremultipliedAlpha*/ +
        1 /*sampler*/;
    const auto alreadyOccupiedUniforms =
        4 /*param_vs_mProjectionView*/ +
        1 /*param_vs_targetInTilePosN*/ +
        (!gpuAPI->isSupported_textureLod
            ? 0
            : 1 /*param_vs_distanceFromCameraToTarget*/ +
              1 /*param_vs_cameraElevationAngleN*/ +
              1 /*param_vs_groundCameraPosition*/ +
              1 /*param_vs_scaleToRetainProjectedSize*/) +
        1 /*param_vs_elevation_configuration*/ +
        8 /*param_vs_elevation_colorMap*/ +
        1 /*param_vs_tileCoordsOffset*/ +
        1 /*param_vs_elevation_scale*/ +
        (gpuAPI->isSupported_vertexShaderTextureLookup ? vsUniformsPerLayer : 0) /*param_vs_elevationDataLayer*/;
    _maxNumberOfRasterMapLayersInBatch =
        (gpuAPI->maxVertexUniformVectors - alreadyOccupiedUniforms) / (vsUniformsPerLayer + fsUniformsPerLayer);
    if (_maxNumberOfRasterMapLayersInBatch > gpuAPI->maxTextureUnitsInFragmentShader)
        _maxNumberOfRasterMapLayersInBatch = gpuAPI->maxTextureUnitsInFragmentShader;
    if (gpuAPI->isSupported_vertexShaderTextureLookup &&
        _maxNumberOfRasterMapLayersInBatch + 1 > gpuAPI->maxTextureUnitsCombined)
    {
        _maxNumberOfRasterMapLayersInBatch = gpuAPI->maxTextureUnitsCombined - 1;
    }
    if (setupOptions.maxNumberOfRasterMapLayersInBatch != 0 &&
        _maxNumberOfRasterMapLayersInBatch > setupOptions.maxNumberOfRasterMapLayersInBatch)
    {
        _maxNumberOfRasterMapLayersInBatch = setupOptions.maxNumberOfRasterMapLayersInBatch;
    }

    // Initialize programs that support [1 ... _maxNumberOfRasterMapLayersInBatch] as number of layers
    auto supportedMaxNumberOfRasterMapLayersInBatch = _maxNumberOfRasterMapLayersInBatch;
    for (auto numberOfLayersInBatch = _maxNumberOfRasterMapLayersInBatch; numberOfLayersInBatch >= 1; numberOfLayersInBatch--)
    {
        RasterLayerTileProgram rasterLayerTileProgram;
        const auto success = initializeRasterLayersProgram(numberOfLayersInBatch, rasterLayerTileProgram);
        if (!success)
        {
            supportedMaxNumberOfRasterMapLayersInBatch -= 1;
            continue;
        }

        _rasterLayerTilePrograms.insert(numberOfLayersInBatch, rasterLayerTileProgram);
    }
    if (supportedMaxNumberOfRasterMapLayersInBatch != _maxNumberOfRasterMapLayersInBatch)
    {
        LogPrintf(LogSeverityLevel::Warning,
            "Seems like buggy driver. "
            "This device should be capable of rendering %d raster map layers in batch, but only %d variant compiles",
            _maxNumberOfRasterMapLayersInBatch,
            supportedMaxNumberOfRasterMapLayersInBatch);
        _maxNumberOfRasterMapLayersInBatch = supportedMaxNumberOfRasterMapLayersInBatch;
    }
    if (_maxNumberOfRasterMapLayersInBatch < 1)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Maximal number of raster map layers in a batch must be not less than 1");
        return false;
    }
    LogPrintf(LogSeverityLevel::Info,
        "Maximal number of raster map layers in a batch is %d", _maxNumberOfRasterMapLayersInBatch);

    initializeRasterTile();

    return true;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::initializeRasterLayersProgram(
    const unsigned int numberOfLayersInBatch,
    RasterLayerTileProgram& outRasterLayerTileProgram)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteShader);
    GL_CHECK_PRESENT(glDeleteProgram);

    const auto& vertexShader = QString::fromLatin1(
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "INPUT vec2 in_vs_vertexTexCoords;                                                                                  ""\n"
        "#if !VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                ""\n"
        "    INPUT mat3 in_vs_vertexElevation;                                                                          ""\n"
        "#endif // !VERTEX_TEXTURE_FETCH_SUPPORTED                                                                          ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "%UnrolledPerRasterLayerTexCoordsDeclarationCode%                                                                   ""\n"
        "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
        "    PARAM_OUTPUT float v2f_mipmapLOD;                                                                              ""\n"
        "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
        "PARAM_OUTPUT lowp vec4 v2f_elevationColor;                                                                         ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mProjectionView;                                                                             ""\n"
        "uniform vec2 param_vs_targetInTilePosN;                                                                            ""\n"
        "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
        "    uniform float param_vs_distanceFromCameraToTarget;                                                             ""\n"
        "    uniform float param_vs_cameraElevationAngleN;                                                                  ""\n"
        "    uniform vec2 param_vs_groundCameraPosition;                                                                    ""\n"
        "    uniform float param_vs_scaleToRetainProjectedSize;                                                             ""\n"
        "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
        "uniform vec4 param_vs_elevation_configuration;                                                                     ""\n"
        "uniform vec4 param_vs_elevation_colorMap[8];                                                                       ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-tile data
        "uniform vec2 param_vs_tileCoordsOffset;                                                                            ""\n"
        "uniform vec4 param_vs_elevation_scale;                                                                             ""\n"
        "#if VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                 ""\n"
        "    uniform highp sampler2D param_vs_elevation_dataSampler;                                                        ""\n"
        "#endif // VERTEX_TEXTURE_FETCH_SUPPORTED                                                                           ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-layer-in-tile data
        "struct VsRasterLayerTile                                                                                           ""\n"
        "{                                                                                                                  ""\n"
        "    mediump vec4 texCoordsOffsetAndScale;                                                                          ""\n"
        "    highp vec4 texelSize;                                                                                          ""\n"
        "};                                                                                                                 ""\n"
        "%UnrolledPerRasterLayerParamsDeclarationCode%                                                                      ""\n"
        "#if VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                 ""\n"
        "    uniform VsRasterLayerTile param_vs_elevationDataLayer;                                                         ""\n"
        "#endif // !VERTEX_TEXTURE_FETCH_SUPPORTED                                                                          ""\n"
        "                                                                                                                   ""\n"
        "void calculateTextureCoordinates(in VsRasterLayerTile tileLayer, out vec2 outTexCoords)                            ""\n"
        "{                                                                                                                  ""\n"
        "    mediump vec2 texCoords = in_vs_vertexTexCoords;                                                                ""\n"
        "    texCoords = texCoords * tileLayer.texCoordsOffsetAndScale.zw + tileLayer.texCoordsOffsetAndScale.xy;           ""\n"
        "    outTexCoords = texCoords;                                                                                      ""\n"
        "}                                                                                                                  ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    vec4 v = vec4(in_vs_vertexPosition.x, 0.0, in_vs_vertexPosition.y, 1.0);                                       ""\n"
        "                                                                                                                   ""\n"
        //   Shift vertex to it's proper position
        "    v.xz += %TileSize3D%.0 * (param_vs_tileCoordsOffset - param_vs_targetInTilePosN);                              ""\n"
        "                                                                                                                   ""\n"
        //   Process each tile layer texture coordinates (except elevation)
        "%UnrolledPerRasterLayerTexCoordsProcessingCode%                                                                    ""\n"
        "                                                                                                                   ""\n"
        //   If elevation data is active, use it
        "    v2f_elevationColor = vec4(0.0, 0.0, 0.0, 0.0);                                                                 ""\n"
        "    if (abs(param_vs_elevation_scale.w) > 0.0)                                                                     ""\n"
        "    {                                                                                                              ""\n"
        "        float slopeAlgorithm = param_vs_elevation_configuration.x;                                                 ""\n"
        "                                                                                                                   ""\n"
        "        float metersToUnits = mix(param_vs_elevation_scale.x, param_vs_elevation_scale.y, in_vs_vertexTexCoords.t);""\n"
        "                                                                                                                   ""\n"
        // [0] - TL; [1] - T; [2] - TR
        // [3] -  L; [4] - O; [5] -  R
        // [6] - BL; [7] - B; [8] - BR
        "        float heightInMeters[9];                                                                                   ""\n"
        "#if VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                 ""\n"
        "        vec2 elevationDataTexCoordsO;                                                                              ""\n"
        "        calculateTextureCoordinates(                                                                               ""\n"
        "            param_vs_elevationDataLayer,                                                                           ""\n"
        "            elevationDataTexCoordsO);                                                                              ""\n"
        "        heightInMeters[4] = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, elevationDataTexCoordsO).r;          ""\n"
        "        heightInMeters[4] *= param_vs_elevation_scale.w;                                                           ""\n"
        "                                                                                                                   ""\n"
        "        if (slopeAlgorithm > %SlopeAlgorithm_None%.0)                                                              ""\n"
        "        {                                                                                                          ""\n"
        "            vec2 elevationDataTexCoordsT = elevationDataTexCoordsO;                                                ""\n"
        "            elevationDataTexCoordsT.t -= param_vs_elevationDataLayer.texelSize.y;                                  ""\n"
        "            heightInMeters[1] = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, elevationDataTexCoordsT).r;      ""\n"
        "            heightInMeters[1] *= param_vs_elevation_scale.w;                                                       ""\n"
        "                                                                                                                   ""\n"
        "            vec2 elevationDataTexCoordsL = elevationDataTexCoordsO;                                                ""\n"
        "            elevationDataTexCoordsL.s -= param_vs_elevationDataLayer.texelSize.x;                                  ""\n"
        "            heightInMeters[3] = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, elevationDataTexCoordsL).r;      ""\n"
        "            heightInMeters[3] *= param_vs_elevation_scale.w;                                                       ""\n"
        "                                                                                                                   ""\n"
        "            vec2 elevationDataTexCoordsB = elevationDataTexCoordsO;                                                ""\n"
        "            elevationDataTexCoordsB.t += param_vs_elevationDataLayer.texelSize.y;                                  ""\n"
        "            heightInMeters[7] = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, elevationDataTexCoordsB).r;      ""\n"
        "            heightInMeters[7] *= param_vs_elevation_scale.w;                                                       ""\n"
        "                                                                                                                   ""\n"
        "            vec2 elevationDataTexCoordsR = elevationDataTexCoordsO;                                                ""\n"
        "            elevationDataTexCoordsR.s += param_vs_elevationDataLayer.texelSize.x;                                  ""\n"
        "            heightInMeters[5] = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, elevationDataTexCoordsR).r;      ""\n"
        "            heightInMeters[5] *= param_vs_elevation_scale.w;                                                       ""\n"
        "                                                                                                                   ""\n"
        "            if (slopeAlgorithm > %SlopeAlgorithm_ZevenbergenThorne%.0)                                             ""\n"
        "            {                                                                                                      ""\n"
        "                vec2 elevationDataTexCoordsTL = elevationDataTexCoordsO;                                           ""\n"
        "                elevationDataTexCoordsTL.s -= param_vs_elevationDataLayer.texelSize.x;                             ""\n"
        "                elevationDataTexCoordsTL.t -= param_vs_elevationDataLayer.texelSize.y;                             ""\n"
        "                heightInMeters[0] = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, elevationDataTexCoordsTL).r; ""\n"
        "                heightInMeters[0] *= param_vs_elevation_scale.w;                                                   ""\n"
        "                                                                                                                   ""\n"
        "                vec2 elevationDataTexCoordsTR = elevationDataTexCoordsO;                                           ""\n"
        "                elevationDataTexCoordsTR.s += param_vs_elevationDataLayer.texelSize.x;                             ""\n"
        "                elevationDataTexCoordsTR.t -= param_vs_elevationDataLayer.texelSize.y;                             ""\n"
        "                heightInMeters[2] = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, elevationDataTexCoordsTR).r; ""\n"
        "                heightInMeters[2] *= param_vs_elevation_scale.w;                                                   ""\n"
        "                                                                                                                   ""\n"
        "                vec2 elevationDataTexCoordsBL = elevationDataTexCoordsO;                                           ""\n"
        "                elevationDataTexCoordsBL.s -= param_vs_elevationDataLayer.texelSize.x;                             ""\n"
        "                elevationDataTexCoordsBL.t += param_vs_elevationDataLayer.texelSize.y;                             ""\n"
        "                heightInMeters[6] = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, elevationDataTexCoordsBL).r; ""\n"
        "                heightInMeters[6] *= param_vs_elevation_scale.w;                                                   ""\n"
        "                                                                                                                   ""\n"
        "                vec2 elevationDataTexCoordsBR = elevationDataTexCoordsO;                                           ""\n"
        "                elevationDataTexCoordsBR.s += param_vs_elevationDataLayer.texelSize.x;                             ""\n"
        "                elevationDataTexCoordsBR.t += param_vs_elevationDataLayer.texelSize.y;                             ""\n"
        "                heightInMeters[8] = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, elevationDataTexCoordsBR).r; ""\n"
        "                heightInMeters[8] *= param_vs_elevation_scale.w;                                                   ""\n"
        "            }                                                                                                      ""\n"
        "        }                                                                                                          ""\n"
        "#else // !VERTEX_TEXTURE_FETCH_SUPPORTED                                                                           ""\n"
        "        heightInMeters[0] = in_vs_vertexElevation[0][0] * param_vs_elevation_scale.w;                              ""\n"
        "        heightInMeters[1] = in_vs_vertexElevation[1][0] * param_vs_elevation_scale.w;                              ""\n"
        "        heightInMeters[2] = in_vs_vertexElevation[2][0] * param_vs_elevation_scale.w;                              ""\n"
        "        heightInMeters[3] = in_vs_vertexElevation[0][1] * param_vs_elevation_scale.w;                              ""\n"
        "        heightInMeters[4] = in_vs_vertexElevation[1][1] * param_vs_elevation_scale.w;                              ""\n"
        "        heightInMeters[5] = in_vs_vertexElevation[2][1] * param_vs_elevation_scale.w;                              ""\n"
        "        heightInMeters[6] = in_vs_vertexElevation[0][2] * param_vs_elevation_scale.w;                              ""\n"
        "        heightInMeters[7] = in_vs_vertexElevation[1][2] * param_vs_elevation_scale.w;                              ""\n"
        "        heightInMeters[8] = in_vs_vertexElevation[2][2] * param_vs_elevation_scale.w;                              ""\n"
        "#endif // VERTEX_TEXTURE_FETCH_SUPPORTED                                                                           ""\n"
        "                                                                                                                   ""\n"
        "        float visualizationStyle = param_vs_elevation_configuration.y;                                             ""\n"
        "        if (visualizationStyle > %VisualizationStyle_None%.0)                                                      ""\n"
        "        {                                                                                                          ""\n"
        "            vec2 slope;                                                                                            ""\n"
        "            if (abs(slopeAlgorithm - %SlopeAlgorithm_ZevenbergenThorne%.0) < 0.00001)                              ""\n"
        "            {                                                                                                      ""\n"
        "                slope.x = (heightInMeters[3] - heightInMeters[5]) / metersToUnits;                                 ""\n"
        "                slope.y = (heightInMeters[7] - heightInMeters[1]) / metersToUnits;                                 ""\n"
        "            }                                                                                                      ""\n"
        "            else if (abs(slopeAlgorithm - %SlopeAlgorithm_Horn%.0) < 0.00001)                                      ""\n"
        "            {                                                                                                      ""\n"
        "                slope.x = heightInMeters[0] + heightInMeters[3] + heightInMeters[3] + heightInMeters[6];           ""\n"
        "                slope.x -= heightInMeters[2] + heightInMeters[5] + heightInMeters[5] + heightInMeters[8];          ""\n"
        "                slope.x /= metersToUnits;                                                                          ""\n"
        "                                                                                                                   ""\n"
        "                slope.y = heightInMeters[6] + heightInMeters[7] + heightInMeters[7] + heightInMeters[8];           ""\n"
        "                slope.y -= heightInMeters[0] + heightInMeters[1] + heightInMeters[1] + heightInMeters[2];          ""\n"
        "                slope.y /= metersToUnits;                                                                          ""\n"
        "            }                                                                                                      ""\n"
        "                                                                                                                   ""\n"
        "            float colorMapKey;                                                                                     ""\n"
        "            if (abs(visualizationStyle - %VisualizationStyle_HillshadeTraditional%.0) < 0.00001)                   ""\n"
        "            {                                                                                                      ""\n"
        "            }                                                                                                      ""\n"
        "            else if (abs(visualizationStyle - %VisualizationStyle_HillshadeIgor%.0) < 0.00001)                     ""\n"
        "            {                                                                                                      ""\n"
        "            }                                                                                                      ""\n"
        "            else if (abs(visualizationStyle - %VisualizationStyle_HillshadeCombined%.0) < 0.00001)                 ""\n"
        "            {                                                                                                      ""\n"
        "            }                                                                                                      ""\n"
        "            else if (abs(visualizationStyle - %VisualizationStyle_HillshadeMultidirectional%.0) < 0.00001)         ""\n"
        "            {                                                                                                      ""\n"
        "            }                                                                                                      ""\n"
        "                                                                                                                   ""\n"
        "            lowp vec4 colorMapEntryA = param_vs_elevation_colorMap[0];                                             ""\n"
        "            lowp vec4 colorMapEntryB = colorMapEntryA;                                                             ""\n"
        "            v2f_elevationColor.rgb = mix(colorMapEntryA.yzw, colorMapEntryB.yzw, colorMapKey - colorMapEntryA.x);  ""\n"
        "            v2f_elevationColor.a *= param_vs_elevation_configuration.z;                                            ""\n"
        "        }                                                                                                          ""\n"
        "        v.y = (heightInMeters[4] / metersToUnits) * param_vs_elevation_scale.z;                                    ""\n"
        "    }                                                                                                              ""\n"
        "                                                                                                                   ""\n"
        "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
        //   Calculate mipmap LOD
        "    vec2 groundVertex = v.xz;                                                                                      ""\n"
        "    vec2 groundCameraToVertex = groundVertex - param_vs_groundCameraPosition;                                      ""\n"
        "    float mipmapK = log(1.0 + 10.0 * log2(1.0 + param_vs_cameraElevationAngleN));                                  ""\n"
        "    float mipmapBaseLevelEndDistance = mipmapK * param_vs_distanceFromCameraToTarget;                              ""\n"
        "    v2f_mipmapLOD = 1.0 + (length(groundCameraToVertex) - mipmapBaseLevelEndDistance)                              ""\n"
        "        / (param_vs_scaleToRetainProjectedSize * %TileSize3D%.0);                                                  ""\n"
        "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
        "                                                                                                                   ""\n"
        //   Finally output processed modified vertex
        "    gl_Position = param_vs_mProjectionView * v;                                                                    ""\n"
        "}                                                                                                                  ""\n");
    const auto& vertexShader_perRasterLayerTexCoordsDeclaration = QString::fromLatin1(
        "PARAM_OUTPUT vec2 v2f_texCoordsPerLayer_%rasterLayerIndex%;                                                        ""\n");
    const auto& vertexShader_perRasterLayerParamsDeclaration = QString::fromLatin1(
        "uniform VsRasterLayerTile param_vs_rasterTileLayer_%rasterLayerIndex%;                                             ""\n");
    const auto& vertexShader_perRasterLayerTexCoordsProcessing = QString::fromLatin1(
        "    calculateTextureCoordinates(                                                                                   ""\n"
        "        param_vs_rasterTileLayer_%rasterLayerIndex%,                                                               ""\n"
        "        v2f_texCoordsPerLayer_%rasterLayerIndex%);                                                                 ""\n"
        "                                                                                                                   ""\n");

    const auto& fragmentShader = QString::fromLatin1(
        // Input data
        "%UnrolledPerRasterLayerTexCoordsDeclarationCode%                                                                   ""\n"
        "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
        "    PARAM_INPUT float v2f_mipmapLOD;                                                                               ""\n"
        "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
        "PARAM_INPUT lowp vec4 v2f_elevationColor;                                                                          ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-layer data
        "struct FsRasterLayerTile                                                                                           ""\n"
        "{                                                                                                                  ""\n"
        "    lowp float isPremultipliedAlpha;                                                                               ""\n"
        "    lowp float opacityFactor;                                                                                      ""\n"
        "    lowp sampler2D sampler;                                                                                        ""\n"
        "};                                                                                                                 ""\n"
        "%UnrolledPerRasterLayerParamsDeclarationCode%                                                                      ""\n"
        "                                                                                                                   ""\n"
        "void addExtraAlpha(inout lowp vec4 color, in lowp float alpha, in lowp float isPremultipliedAlpha)                 ""\n"
        "{                                                                                                                  ""\n"
        "    lowp float colorAlpha = 1.0 - isPremultipliedAlpha + isPremultipliedAlpha * alpha;                             ""\n"
        "    color *= vec4(colorAlpha, colorAlpha, colorAlpha, alpha);                                                      ""\n"
        "}                                                                                                                  ""\n"
        "                                                                                                                   ""\n"
        "void mixColors(inout lowp vec4 destColor, in lowp vec4 srcColor)                                                   ""\n"
        "{                                                                                                                  ""\n"
        "    destColor = destColor * (1.0 - srcColor.a) + srcColor * srcColor.a;                                            ""\n"
        "}                                                                                                                  ""\n"
        "                                                                                                                   ""\n"
        "void mixColors(inout lowp vec4 destColor, in lowp vec4 srcColor, in lowp float isPremultipliedAlpha)               ""\n"
        "{                                                                                                                  ""\n"
        "    lowp float srcColorMultiplier =                                                                                ""\n"
        "        isPremultipliedAlpha + (1.0 - isPremultipliedAlpha) * srcColor.a;                                          ""\n"
        "    destColor = destColor * (1.0 - srcColor.a) + srcColor * srcColorMultiplier;                                    ""\n"
        "}                                                                                                                  ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    lowp vec4 finalColor;                                                                                          ""\n"
        "                                                                                                                   ""\n"
        //   Mix colors of all layers.
        //   First layer is processed unconditionally, as well as its color is converted to premultiplied alpha.
        "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
        "    finalColor = SAMPLE_TEXTURE_2D_LOD(                                                                            ""\n"
        "        param_fs_rasterTileLayer_0.sampler,                                                                        ""\n"
        "        v2f_texCoordsPerLayer_0, v2f_mipmapLOD);                                                                   ""\n"
        "#else // !TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
        "    finalColor = SAMPLE_TEXTURE_2D(                                                                                ""\n"
        "        param_fs_rasterTileLayer_0.sampler,                                                                        ""\n"
        "        v2f_texCoordsPerLayer_0);                                                                                  ""\n"
        "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
        "    addExtraAlpha(finalColor, param_fs_rasterTileLayer_0.opacityFactor,                                            ""\n"
        "        param_fs_rasterTileLayer_0.isPremultipliedAlpha);                                                          ""\n"
        "    lowp float firstLayerColorFactor = param_fs_rasterTileLayer_0.isPremultipliedAlpha +                           ""\n"
        "        (1.0 - param_fs_rasterTileLayer_0.isPremultipliedAlpha) * finalColor.a;                                    ""\n"
        "    finalColor *= vec4(firstLayerColorFactor, firstLayerColorFactor, firstLayerColorFactor, 1.0);                  ""\n"
        "                                                                                                                   ""\n"
        "%UnrolledPerRasterLayerProcessingCode%                                                                             ""\n"
        "                                                                                                                   ""\n"
        "    mixColors(finalColor, v2f_elevationColor);                                                                     ""\n"
        "                                                                                                                   ""\n"
#if 0
        //   NOTE: Useful for debugging mipmap levels
        "    {                                                                                                              ""\n"
        "        vec4 mipmapDebugColor;                                                                                     ""\n"
        "        mipmapDebugColor.a = 1.0;                                                                                  ""\n"
        "        float value = v2f_mipmapLOD;                                                                               ""\n"
        //"        float value = textureQueryLod(param_vs_rasterTileLayer[0].sampler, v2f_texCoordsPerLayer[0]).x;          ""\n"
        "        mipmapDebugColor.r = clamp(value, 0.0, 1.0);                                                               ""\n"
        "        value -= 1.0;                                                                                              ""\n"
        "        mipmapDebugColor.g = clamp(value, 0.0, 1.0);                                                               ""\n"
        "        value -= 1.0;                                                                                              ""\n"
        "        mipmapDebugColor.b = clamp(value, 0.0, 1.0);                                                               ""\n"
        "        finalColor = mix(finalColor, mipmapDebugColor, 0.5);                                                       ""\n"
        "    }                                                                                                              ""\n"
#endif
        "                                                                                                                   ""\n"
        "    FRAGMENT_COLOR_OUTPUT = finalColor;                                                                            ""\n"
        "}                                                                                                                  ""\n");
    const auto& fragmentShader_perRasterLayer = QString::fromLatin1(
        "    {                                                                                                              ""\n"
        "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
        "        lowp vec4 layerColor = SAMPLE_TEXTURE_2D_LOD(                                                              ""\n"
        "            param_fs_rasterTileLayer_%rasterLayerIndex%.sampler,                                                   ""\n"
        "            v2f_texCoordsPerLayer_%rasterLayerIndex%, v2f_mipmapLOD);                                              ""\n"
        "#else // !TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
        "        lowp vec4 layerColor = SAMPLE_TEXTURE_2D(                                                                  ""\n"
        "            param_fs_rasterTileLayer_%rasterLayerIndex%.sampler,                                                   ""\n"
        "            v2f_texCoordsPerLayer_%rasterLayerIndex%);                                                             ""\n"
        "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
        "                                                                                                                   ""\n"
        "        addExtraAlpha(layerColor, param_fs_rasterTileLayer_%rasterLayerIndex%.opacityFactor,                       ""\n"
        "            param_fs_rasterTileLayer_%rasterLayerIndex%.isPremultipliedAlpha);                                     ""\n"
        "        mixColors(finalColor, layerColor, param_fs_rasterTileLayer_%rasterLayerIndex%.isPremultipliedAlpha);       ""\n"
        "    }                                                                                                              ""\n");
    const auto& fragmentShader_perRasterLayerTexCoordsDeclaration = QString::fromLatin1(
        "PARAM_INPUT vec2 v2f_texCoordsPerLayer_%rasterLayerIndex%;                                                         ""\n");
    const auto& fragmentShader_perRasterLayerParamsDeclaration = QString::fromLatin1(
        "uniform FsRasterLayerTile param_fs_rasterTileLayer_%rasterLayerIndex%;                                             ""\n");

    // Compile vertex shader
    auto preprocessedVertexShader = vertexShader;
    QString preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsProcessingCode;
    QString preprocessedVertexShader_UnrolledPerRasterLayerParamsDeclarationCode;
    QString preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsDeclarationCode;
    for (auto layerIndex = 0u; layerIndex < numberOfLayersInBatch; layerIndex++)
    {
        preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsProcessingCode +=
            detachedOf(vertexShader_perRasterLayerTexCoordsProcessing)
            .replace("%rasterLayerIndex%", QString::number(layerIndex));

        preprocessedVertexShader_UnrolledPerRasterLayerParamsDeclarationCode +=
            detachedOf(vertexShader_perRasterLayerParamsDeclaration)
            .replace("%rasterLayerIndex%", QString::number(layerIndex));

        preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsDeclarationCode +=
            detachedOf(vertexShader_perRasterLayerTexCoordsDeclaration)
            .replace("%rasterLayerIndex%", QString::number(layerIndex));
    }
    preprocessedVertexShader.replace("%UnrolledPerRasterLayerTexCoordsProcessingCode%",
        preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsProcessingCode);
    preprocessedVertexShader.replace("%UnrolledPerRasterLayerParamsDeclarationCode%",
        preprocessedVertexShader_UnrolledPerRasterLayerParamsDeclarationCode);
    preprocessedVertexShader.replace("%UnrolledPerRasterLayerTexCoordsDeclarationCode%",
        preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsDeclarationCode);
    preprocessedVertexShader.replace("%TileSize3D%", QString::number(AtlasMapRenderer::TileSize3D));
    preprocessedVertexShader.replace("%SlopeAlgorithm_None%",
        QString::number(static_cast<int>(ElevationConfiguration::SlopeAlgorithm::None)));
    preprocessedVertexShader.replace("%SlopeAlgorithm_ZevenbergenThorne%",
        QString::number(static_cast<int>(ElevationConfiguration::SlopeAlgorithm::ZevenbergenThorne)));
    preprocessedVertexShader.replace("%SlopeAlgorithm_Horn%",
        QString::number(static_cast<int>(ElevationConfiguration::SlopeAlgorithm::Horn)));
    preprocessedVertexShader.replace("%VisualizationStyle_None%",
        QString::number(static_cast<int>(ElevationConfiguration::VisualizationStyle::None)));
    preprocessedVertexShader.replace("%VisualizationStyle_HillshadeTraditional%",
        QString::number(static_cast<int>(ElevationConfiguration::VisualizationStyle::HillshadeTraditional)));
    preprocessedVertexShader.replace("%VisualizationStyle_HillshadeIgor%",
        QString::number(static_cast<int>(ElevationConfiguration::VisualizationStyle::HillshadeIgor)));
    preprocessedVertexShader.replace("%VisualizationStyle_HillshadeCombined%",
        QString::number(static_cast<int>(ElevationConfiguration::VisualizationStyle::HillshadeCombined)));
    preprocessedVertexShader.replace("%VisualizationStyle_HillshadeMultidirectional%",
        QString::number(static_cast<int>(ElevationConfiguration::VisualizationStyle::HillshadeMultidirectional)));
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    if (vsId == 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererMapLayersStage_OpenGL vertex shader for %d raster map layers",
            numberOfLayersInBatch);
        return false;
    }

    // Compile fragment shader
    auto preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerRasterLayerTexCoordsDeclarationCode;
    QString preprocessedFragmentShader_UnrolledPerRasterLayerParamsDeclarationCode;
    QString preprocessedFragmentShader_UnrolledPerRasterLayerProcessingCode;
    for (auto layerIndex = 0u; layerIndex < numberOfLayersInBatch; layerIndex++)
    {
        preprocessedFragmentShader_UnrolledPerRasterLayerTexCoordsDeclarationCode +=
            detachedOf(fragmentShader_perRasterLayerTexCoordsDeclaration)
            .replace("%rasterLayerIndex%", QString::number(layerIndex));

        preprocessedFragmentShader_UnrolledPerRasterLayerParamsDeclarationCode +=
            detachedOf(fragmentShader_perRasterLayerParamsDeclaration)
            .replace("%rasterLayerIndex%", QString::number(layerIndex));

        if (layerIndex > 0)
        {
            preprocessedFragmentShader_UnrolledPerRasterLayerProcessingCode +=
                detachedOf(fragmentShader_perRasterLayer).replace("%rasterLayerIndex%", QString::number(layerIndex));
        }
    }
    preprocessedFragmentShader.replace(
        "%UnrolledPerRasterLayerTexCoordsDeclarationCode%",
        preprocessedFragmentShader_UnrolledPerRasterLayerTexCoordsDeclarationCode);
    preprocessedFragmentShader.replace(
        "%UnrolledPerRasterLayerParamsDeclarationCode%",
        preprocessedFragmentShader_UnrolledPerRasterLayerParamsDeclarationCode);
    preprocessedFragmentShader.replace(
        "%UnrolledPerRasterLayerProcessingCode%",
        preprocessedFragmentShader_UnrolledPerRasterLayerProcessingCode);
    gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
    gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    if (fsId == 0)
    {
        glDeleteShader(vsId);
        GL_CHECK_RESULT;

        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererMapLayersStage_OpenGL fragment shader for %d raster map layers",
            numberOfLayersInBatch);
        return false;
    }

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    outRasterLayerTileProgram.id = getGPUAPI()->linkProgram(2, shaders, true, &variablesMap);
    if (!outRasterLayerTileProgram.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererMapLayersStage_OpenGL program for %d raster map layers", numberOfLayersInBatch);
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(outRasterLayerTileProgram.id, variablesMap);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.in.vertexPosition,
        "in_vs_vertexPosition",
        GlslVariableType::In);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.in.vertexTexCoords,
        "in_vs_vertexTexCoords",
        GlslVariableType::In);
    if (!gpuAPI->isSupported_vertexShaderTextureLookup)
    {
        ok = ok && lookup->lookupLocation(
            outRasterLayerTileProgram.vs.in.vertexElevation,
            "in_vs_vertexElevation",
            GlslVariableType::In);
    }
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.mProjectionView,
        "param_vs_mProjectionView",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.targetInTilePosN,
        "param_vs_targetInTilePosN",
        GlslVariableType::Uniform);
    if (gpuAPI->isSupported_textureLod)
    {
        ok = ok && lookup->lookupLocation(
            outRasterLayerTileProgram.vs.param.distanceFromCameraToTarget,
            "param_vs_distanceFromCameraToTarget",
            GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(
            outRasterLayerTileProgram.vs.param.cameraElevationAngleN,
            "param_vs_cameraElevationAngleN",
            GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(
            outRasterLayerTileProgram.vs.param.groundCameraPosition,
            "param_vs_groundCameraPosition",
            GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(
            outRasterLayerTileProgram.vs.param.scaleToRetainProjectedSize,
            "param_vs_scaleToRetainProjectedSize",
            GlslVariableType::Uniform);
    }
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.elevation_configuration,
        "param_vs_elevation_configuration",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.elevation_colorMap,
        "param_vs_elevation_colorMap",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.tileCoordsOffset,
        "param_vs_tileCoordsOffset",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.elevation_scale,
        "param_vs_elevation_scale",
        GlslVariableType::Uniform);
    if (gpuAPI->isSupported_vertexShaderTextureLookup)
    {
        ok = ok && lookup->lookupLocation(
            outRasterLayerTileProgram.vs.param.elevation_dataSampler,
            "param_vs_elevation_dataSampler",
            GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(
            outRasterLayerTileProgram.vs.param.elevationDataLayer.texCoordsOffsetAndScale,
            "param_vs_elevationDataLayer.texCoordsOffsetAndScale",
            GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(
            outRasterLayerTileProgram.vs.param.elevationDataLayer.texelSize,
            "param_vs_elevationDataLayer.texelSize",
            GlslVariableType::Uniform);
    }
    outRasterLayerTileProgram.vs.param.rasterTileLayers.resize(numberOfLayersInBatch);
    outRasterLayerTileProgram.fs.param.rasterTileLayers.resize(numberOfLayersInBatch);
    for (auto layerIndex = 0u; layerIndex < numberOfLayersInBatch; layerIndex++)
    {
        // Vertex shader
        {
            auto layerStructName = QString::fromLatin1("param_vs_rasterTileLayer_%layerIndex%")
                .replace(QLatin1String("%layerIndex%"), QString::number(layerIndex));
            auto& layerStruct = outRasterLayerTileProgram.vs.param.rasterTileLayers[layerIndex];

            ok = ok && lookup->lookupLocation(
                layerStruct.texCoordsOffsetAndScale,
                layerStructName + ".texCoordsOffsetAndScale",
                GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(
                layerStruct.texelSize,
                layerStructName + ".texelSize",
                GlslVariableType::Uniform);
        }

        // Fragment shader
        {
            auto layerStructName = QString::fromLatin1("param_fs_rasterTileLayer_%layerIndex%")
                .replace(QLatin1String("%layerIndex%"), QString::number(layerIndex));
            auto& layerStruct = outRasterLayerTileProgram.fs.param.rasterTileLayers[layerIndex];

            ok = ok && lookup->lookupLocation(
                layerStruct.sampler,
                layerStructName + ".sampler",
                GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(
                layerStruct.opacityFactor,
                layerStructName + ".opacityFactor",
                GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(
                layerStruct.isPremultipliedAlpha,
                layerStructName + ".isPremultipliedAlpha",
                GlslVariableType::Uniform);
        }
    }

    return ok;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::renderRasterLayersBatch(
    const Ref<PerTileBatchedLayers>& batch,
    AlphaChannelType& currentAlphaChannelType,
    GLlocation& activeElevationVertexAttribArray,
    GLname& lastUsedProgram)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform2i);
    GL_CHECK_PRESENT(glUniform2fv);
    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);
    GL_CHECK_PRESENT(glDisableVertexAttribArray);

    const auto& currentConfiguration = getCurrentConfiguration();
    const auto& internalState = getInternalState();

    const auto batchedLayersCount = batch->layers.size();
    const auto elevationDataSamplerIndex = gpuAPI->isSupported_vertexShaderTextureLookup ? batchedLayersCount : -1;

    GL_PUSH_GROUP_MARKER(QString("%1x%2@%3").arg(batch->tileId.x).arg(batch->tileId.y).arg(currentState.zoomLevel));

    // Activate proper program depending on number of captured layers
    const auto wasActivated = activateRasterLayersProgram(
        batchedLayersCount,
        elevationDataSamplerIndex,
        lastUsedProgram,
        activeElevationVertexAttribArray);
    const auto& program = _rasterLayerTilePrograms[batchedLayersCount];
    const auto& vao = _rasterTileVAOs[batchedLayersCount];

    // Set tile coordinates offset
    glUniform2f(program.vs.param.tileCoordsOffset,
        batch->tileId.x - internalState.targetTileId.x,
        batch->tileId.y - internalState.targetTileId.y);
    GL_CHECK_RESULT;

    // Configure elevation data
    if (currentState.elevationDataProvider)
    {
        configureElevationData(
            program,
            batch->tileId,
            elevationDataSamplerIndex,
            activeElevationVertexAttribArray);
    }

    // Shader expects blending to be premultiplied, since output color of fragment shader is premultiplied by alpha
    if (currentAlphaChannelType != AlphaChannelType::Premultiplied)
    {
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        GL_CHECK_RESULT;

        currentAlphaChannelType = AlphaChannelType::Premultiplied;
    }

    // Perform rendering of exact-scale, overscale and underscale cases. All cases support batching
    const auto subtilesPerTile = batch->layers.first()->resourcesInGPU.size();
    const auto indicesPerSubtile = _rasterTileIndicesCount / subtilesPerTile;
    for (auto subtileIndex = 0; subtileIndex < subtilesPerTile; subtileIndex++)
    {
        for (int layerIndexInBatch = 0; layerIndexInBatch < batchedLayersCount; layerIndexInBatch++)
        {
            const auto& layer = batch->layers[layerIndexInBatch];
            const auto samplerIndex = layerIndexInBatch;

            const auto& perTile_vs = program.vs.param.rasterTileLayers[layerIndexInBatch];
            const auto& perTile_fs = program.fs.param.rasterTileLayers[layerIndexInBatch];

            const auto citMapLayerConfiguration = currentState.mapLayersConfigurations.constFind(layer->layerIndex);
            if (citMapLayerConfiguration == currentState.mapLayersConfigurations.cend() ||
                layer->layerIndex == currentState.mapLayersProviders.firstKey())
            {
                glUniform1f(perTile_fs.opacityFactor, 1.0f);
                GL_CHECK_RESULT;
            }
            else
            {
                const auto& layerConfiguration = *citMapLayerConfiguration;
                glUniform1f(perTile_fs.opacityFactor, layerConfiguration.opacityFactor);
                GL_CHECK_RESULT;
            }

            glActiveTexture(GL_TEXTURE0 + samplerIndex);
            GL_CHECK_RESULT;

            const auto& batchedResourceInGPU = layer->resourcesInGPU[subtileIndex];
            switch (gpuAPI->getGpuResourceAlphaChannelType(batchedResourceInGPU->resourceInGPU))
            {
                case AlphaChannelType::Premultiplied:
                    glUniform1f(perTile_fs.isPremultipliedAlpha, 1.0f);
                    GL_CHECK_RESULT;
                    break;
                case AlphaChannelType::Straight:
                    glUniform1f(perTile_fs.isPremultipliedAlpha, 0.0f);
                    GL_CHECK_RESULT;
                    break;
                default:
                    break;
            }

            glBindTexture(GL_TEXTURE_2D,
                static_cast<GLuint>(reinterpret_cast<intptr_t>(batchedResourceInGPU->resourceInGPU->refInGPU)));
            GL_CHECK_RESULT;

            gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + samplerIndex);

            if (batchedResourceInGPU->resourceInGPU->type == GPUAPI::ResourceInGPU::Type::SlotOnAtlasTexture)
            {
                const auto tileOnAtlasTexture =
                    std::static_pointer_cast<const GPUAPI::SlotOnAtlasTextureInGPU>(batchedResourceInGPU->resourceInGPU);
                const auto rowIndex = tileOnAtlasTexture->slotIndex / tileOnAtlasTexture->atlasTexture->slotsPerSide;
                const auto colIndex = tileOnAtlasTexture->slotIndex - rowIndex * tileOnAtlasTexture->atlasTexture->slotsPerSide;
                const auto tileSizeN = tileOnAtlasTexture->atlasTexture->tileSizeN;
                const auto tilePaddingN = tileOnAtlasTexture->atlasTexture->tilePaddingN;
                const auto nSizeInAtlas = tileSizeN - 2.0f * tilePaddingN;
                PointF texCoordsOffset(
                    colIndex * tileSizeN + tilePaddingN,
                    rowIndex * tileSizeN + tilePaddingN);

                texCoordsOffset += batchedResourceInGPU->texCoordsOffset * nSizeInAtlas;
                const auto texCoordsScale = batchedResourceInGPU->texCoordsScale * nSizeInAtlas;

                glUniform4f(perTile_vs.texCoordsOffsetAndScale,
                    texCoordsOffset.x,
                    texCoordsOffset.y,
                    texCoordsScale.x,
                    texCoordsScale.y);
                GL_CHECK_RESULT;
                glUniform4f(perTile_vs.texelSize,
                    tileOnAtlasTexture->atlasTexture->uTexelSizeN,
                    tileOnAtlasTexture->atlasTexture->vTexelSizeN,
                    tileOnAtlasTexture->atlasTexture->uHalfTexelSizeN,
                    tileOnAtlasTexture->atlasTexture->vHalfTexelSizeN);
                GL_CHECK_RESULT;
            }
            else // if (batchedResourceInGPU->resourceInGPU->type == GPUAPI::ResourceInGPU::Type::Texture)
            {
                const auto& texture = std::static_pointer_cast<const GPUAPI::TextureInGPU>(batchedResourceInGPU->resourceInGPU);

                glUniform4f(perTile_vs.texCoordsOffsetAndScale,
                    batchedResourceInGPU->texCoordsOffset.x,
                    batchedResourceInGPU->texCoordsOffset.y,
                    batchedResourceInGPU->texCoordsScale.x,
                    batchedResourceInGPU->texCoordsScale.y);
                GL_CHECK_RESULT;
                glUniform4f(perTile_vs.texelSize,
                    texture->uTexelSizeN,
                    texture->vTexelSizeN,
                    texture->uHalfTexelSizeN,
                    texture->vHalfTexelSizeN);
                GL_CHECK_RESULT;
            }

            // Perform drawing of a subtile
            glDrawElements(
                GL_TRIANGLES,
                indicesPerSubtile,
                GL_UNSIGNED_SHORT,
                reinterpret_cast<const void*>(sizeof(uint16_t) * indicesPerSubtile * subtileIndex));
            GL_CHECK_RESULT;
        }
    }

    // Unbind textures from texture samplers, that were used
    const auto usedSamplersCount = batchedLayersCount + (gpuAPI->isSupported_vertexShaderTextureLookup ? 1 : 0);
    for (int samplerIndex = 0; samplerIndex < usedSamplersCount; samplerIndex++)
    {
        glActiveTexture(GL_TEXTURE0 + samplerIndex);
        GL_CHECK_RESULT;

        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_RESULT;
    }

    // Unbind any binded buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::activateRasterLayersProgram(
    const unsigned int numberOfLayersInBatch,
    const int elevationDataSamplerIndex,
    GLname& lastUsedProgram,
    GLlocation& activeElevationVertexAttribArray)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform1i);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform2fv);
    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glUniform4fv);

    const auto& currentConfiguration = getCurrentConfiguration();
    const auto& internalState = getInternalState();

    const auto& program = _rasterLayerTilePrograms[numberOfLayersInBatch];
    const auto& vao = _rasterTileVAOs[numberOfLayersInBatch];

    if (lastUsedProgram == program.id)
        return false;

    GL_PUSH_GROUP_MARKER(QString("use '%1-batched-raster-map-layers' program").arg(numberOfLayersInBatch));

    if (lastUsedProgram.isValid())
    {
        if (activeElevationVertexAttribArray.isValid())
        {
            glDisableVertexAttribArray(*activeElevationVertexAttribArray + 0);
            GL_CHECK_RESULT;

            glDisableVertexAttribArray(*activeElevationVertexAttribArray + 1);
            GL_CHECK_RESULT;

            glDisableVertexAttribArray(*activeElevationVertexAttribArray + 2);
            GL_CHECK_RESULT;

            activeElevationVertexAttribArray.reset();
        }
    }

    // Set symbol VAO
    gpuAPI->useVAO(vao);

    // Activate program
    glUseProgram(program.id);
    GL_CHECK_RESULT;

    // Set matrices
    glUniformMatrix4fv(program.vs.param.mProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;

    // Set center offset
    glUniform2f(program.vs.param.targetInTilePosN, internalState.targetInTileOffsetN.x, internalState.targetInTileOffsetN.y);
    GL_CHECK_RESULT;

    if (gpuAPI->isSupported_textureLod)
    {
        // Set distance from camera to target
        glUniform1f(program.vs.param.distanceFromCameraToTarget, internalState.distanceFromCameraToTarget);
        GL_CHECK_RESULT;

        // Set normalized [0.0 .. 1.0] angle of camera elevation
        glUniform1f(program.vs.param.cameraElevationAngleN, currentState.elevationAngle / 90.0f);
        GL_CHECK_RESULT;

        // Set position of camera in ground place
        glUniform2fv(program.vs.param.groundCameraPosition, 1, glm::value_ptr(internalState.groundCameraPosition));
        GL_CHECK_RESULT;

        // Set scale to retain projected size
        glUniform1f(program.vs.param.scaleToRetainProjectedSize, internalState.scaleToRetainProjectedSize);
        GL_CHECK_RESULT;
    }

    // Configure elevation data
    if (currentState.elevationDataProvider)
    {
        glUniform4f(program.vs.param.elevation_configuration,
            static_cast<float>(currentState.elevationConfiguration.slopeAlgorithm),
            static_cast<float>(currentState.elevationConfiguration.visualizationStyle),
            currentState.elevationConfiguration.visualizationAlpha,
            0.0f);
        GL_CHECK_RESULT;

        glUniform4fv(program.vs.param.elevation_colorMap,
            8,
            reinterpret_cast<const GLfloat*>(&currentState.elevationConfiguration.colorMap));
        GL_CHECK_RESULT;
    }

    // Configure samplers
    auto bitmapTileSamplerType = GPUAPI_OpenGL::SamplerType::BitmapTile_Bilinear;
    if (gpuAPI->isSupported_textureLod)
    {
        switch (currentConfiguration.texturesFilteringQuality)
        {
            case TextureFilteringQuality::Good:
                bitmapTileSamplerType = GPUAPI_OpenGL::SamplerType::BitmapTile_BilinearMipmap;
                break;
            case TextureFilteringQuality::Best:
                bitmapTileSamplerType = GPUAPI_OpenGL::SamplerType::BitmapTile_TrilinearMipmap;
                break;
        }
    }
    for (auto layerIndexInBatch = 0u; layerIndexInBatch < numberOfLayersInBatch; layerIndexInBatch++)
    {
        const auto samplerIndex = layerIndexInBatch;

        glActiveTexture(GL_TEXTURE0 + samplerIndex);
        GL_CHECK_RESULT;

        glUniform1i(program.fs.param.rasterTileLayers[layerIndexInBatch].sampler, samplerIndex);
        GL_CHECK_RESULT;

        gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + samplerIndex, bitmapTileSamplerType);
    }
    if (gpuAPI->isSupported_vertexShaderTextureLookup)
    {
        glActiveTexture(GL_TEXTURE0 + elevationDataSamplerIndex);
        GL_CHECK_RESULT;

        glUniform1i(program.vs.param.elevation_dataSampler, elevationDataSamplerIndex);
        GL_CHECK_RESULT;

        gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + elevationDataSamplerIndex, GPUAPI_OpenGL::SamplerType::ElevationDataTile);
    }

    lastUsedProgram = program.id;

    GL_POP_GROUP_MARKER;

    return true;
}

std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU>
OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::captureElevationDataResource(
    const TileId normalizedTileId,
    const ZoomLevel zoomLevel)
{
    if (!currentState.elevationDataProvider)
        return nullptr;

    const auto& resourcesCollection_ = getResources().getCollectionSnapshot(
        MapRendererResourceType::ElevationData,
        currentState.elevationDataProvider);
    const auto& resourcesCollection =
        std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection_);

    // Obtain tile entry by normalized tile coordinates, since tile may repeat several times
    std::shared_ptr<MapRendererBaseTiledResource> resource_;
    if (resourcesCollection->obtainResource(normalizedTileId, zoomLevel, resource_))
    {
        const auto resource = std::static_pointer_cast<MapRendererElevationDataResource>(resource_);

        // Check state and obtain GPU resource
        if (resource->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
        {
            // Capture GPU resource
            const auto gpuResource = resource->resourceInGPU;

            resource->setState(MapRendererResourceState::Uploaded);

            return gpuResource;
        }
    }

    return nullptr;
}

std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU> OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::captureLayerResource(
    const std::shared_ptr<const IMapRendererResourcesCollection>& resourcesCollection_,
    const TileId normalizedTileId,
    const ZoomLevel zoomLevel,
    MapRendererResourceState* const outState /*= nullptr*/)
{
    if (outState != nullptr)
        *outState = MapRendererResourceState::Unknown;

    const auto& resourcesCollection =
        std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection_);

    // Obtain tile entry by normalized tile coordinates, since tile may repeat several times
    std::shared_ptr<MapRendererBaseTiledResource> resource_;
    if (resourcesCollection->obtainResource(normalizedTileId, zoomLevel, resource_))
    {
        const auto resource = std::static_pointer_cast<MapRendererElevationDataResource>(resource_);

        // Check state and obtain GPU resource
        if (resource->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
        {
            // Capture GPU resource
            const auto gpuResource = resource->resourceInGPU;

            resource->setState(MapRendererResourceState::Uploaded);

            if (outState != nullptr)
                *outState = MapRendererResourceState::Uploaded;
            return gpuResource;
        }

        if (outState != nullptr)
            *outState = resource->getState();
    }

    return nullptr;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::releaseRasterLayers(const bool gpuContextLost)
{
    GL_CHECK_PRESENT(glDeleteProgram);

    _maxNumberOfRasterMapLayersInBatch = 0;

    releaseRasterTile(gpuContextLost);

    for (auto& rasterLayerTileProgram : _rasterLayerTilePrograms)
    {
        if (!rasterLayerTileProgram.id.isValid())
            continue;

        if (!gpuContextLost)
        {
            glDeleteProgram(rasterLayerTileProgram.id);
            GL_CHECK_RESULT;
        }
        rasterLayerTileProgram = RasterLayerTileProgram();
    }
    _rasterLayerTilePrograms.clear();

    return true;
}

void OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::initializeRasterTile()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

#pragma pack(push, 1)
    struct Vertex
    {
        GLfloat positionXZ[2];
        GLfloat textureUV[2];
    };
#pragma pack(pop)

    Vertex* pVertices = nullptr;
    GLsizei verticesCount = 0;
    GLushort* pIndices = nullptr;
    GLsizei indicesCount = 0;

    // Complex tile patch, that consists of (heightPrimitivesPerSide*heightPrimitivesPerSide) number of
    // height clusters. Height cluster itself consists of 4 vertices and 6 indices (2 polygons)
    const auto heightPrimitivesPerSide = AtlasMapRenderer::HeixelsPerTileSide - 1;
    const GLfloat clusterSize =
        static_cast<GLfloat>(AtlasMapRenderer::TileSize3D) / static_cast<float>(heightPrimitivesPerSide);
    verticesCount = AtlasMapRenderer::HeixelsPerTileSide * AtlasMapRenderer::HeixelsPerTileSide;
    pVertices = new Vertex[verticesCount];
    indicesCount = (heightPrimitivesPerSide * heightPrimitivesPerSide) * 6;
    pIndices = new GLushort[indicesCount];

    Vertex* pV = pVertices;

    // Form vertices
    assert(verticesCount <= std::numeric_limits<GLushort>::max());
    for (int row = 0, count = AtlasMapRenderer::HeixelsPerTileSide; row < count; row++)
    {
        for (int col = 0, count = AtlasMapRenderer::HeixelsPerTileSide; col < count; col++, pV++)
        {
            pV->positionXZ[0] = static_cast<float>(col) * clusterSize;
            pV->positionXZ[1] = static_cast<float>(row) * clusterSize;

            pV->textureUV[0] = static_cast<float>(col) / static_cast<float>(heightPrimitivesPerSide);
            pV->textureUV[1] = static_cast<float>(row) / static_cast<float>(heightPrimitivesPerSide);
        }
    }

    // Form indices
    for (int row = 0; row < heightPrimitivesPerSide; row++)
    {
        for (int col = 0; col < heightPrimitivesPerSide; col++)
        {
            const auto p0 = (row + 1) * AtlasMapRenderer::HeixelsPerTileSide + col + 0;//BL
            const auto p1 = (row + 0) * AtlasMapRenderer::HeixelsPerTileSide + col + 0;//TL
            const auto p2 = (row + 0) * AtlasMapRenderer::HeixelsPerTileSide + col + 1;//TR
            const auto p3 = (row + 1) * AtlasMapRenderer::HeixelsPerTileSide + col + 1;//BR
            assert(p0 <= verticesCount);
            assert(p1 <= verticesCount);
            assert(p2 <= verticesCount);
            assert(p3 <= verticesCount);

            // Get indices position using Morton code
            const auto tileIndex = Utilities::encodeMortonCode(col, row);
            const auto pTileIndices = pIndices + tileIndex * 6;

            // Triangle 0
            pTileIndices[0] = p0;
            pTileIndices[1] = p1;
            pTileIndices[2] = p2;

            // Triangle 1
            pTileIndices[3] = p0;
            pTileIndices[4] = p2;
            pTileIndices[5] = p3;
        }
    }

    // Create VBO
    glGenBuffers(1, &_rasterTileVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _rasterTileVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(Vertex), pVertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    // Create IBO
    glGenBuffers(1, &_rasterTileIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _rasterTileIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), pIndices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    for (auto numberOfLayersInBatch = _maxNumberOfRasterMapLayersInBatch; numberOfLayersInBatch >= 1; numberOfLayersInBatch--)
    {
        auto& rasterTileVAO = _rasterTileVAOs[numberOfLayersInBatch];
        const auto& rasterLayerTileProgram = constOf(_rasterLayerTilePrograms)[numberOfLayersInBatch];

        rasterTileVAO = gpuAPI->allocateUninitializedVAO();

        // Bind IBO to VAO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _rasterTileIBO);
        GL_CHECK_RESULT;

        // Bind VBO to VAO
        glBindBuffer(GL_ARRAY_BUFFER, _rasterTileVBO);
        GL_CHECK_RESULT;

        glEnableVertexAttribArray(*rasterLayerTileProgram.vs.in.vertexPosition);
        GL_CHECK_RESULT;
        glVertexAttribPointer(*rasterLayerTileProgram.vs.in.vertexPosition,
            2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, positionXZ)));
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*rasterLayerTileProgram.vs.in.vertexTexCoords);
        GL_CHECK_RESULT;
        glVertexAttribPointer(*rasterLayerTileProgram.vs.in.vertexTexCoords,
            2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, textureUV)));
        GL_CHECK_RESULT;

        gpuAPI->initializeVAO(rasterTileVAO);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        GL_CHECK_RESULT;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        GL_CHECK_RESULT;
    }

    _rasterTileIndicesCount = indicesCount;

    delete[] pVertices;
    delete[] pIndices;
}

void OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::releaseRasterTile(const bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);

    for (auto& rasterTileVAO : _rasterTileVAOs)
    {
        if (rasterTileVAO.isValid())
        {
            gpuAPI->releaseVAO(rasterTileVAO, gpuContextLost);
            rasterTileVAO.reset();
        }
    }

    if (_rasterTileIBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_rasterTileIBO);
            GL_CHECK_RESULT;
        }
        _rasterTileIBO.reset();
    }
    if (_rasterTileVBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_rasterTileVBO);
            GL_CHECK_RESULT;
        }
        _rasterTileVBO.reset();
    }
    _rasterTileIndicesCount = -1;
}

void OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::configureElevationData(
    const RasterLayerTileProgram& program,
    const TileId tileId,
    const int elevationDataSamplerIndex,
    GLlocation& activeElevationVertexAttribArray)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    const auto tileIdN = Utilities::normalizeTileId(tileId, currentState.zoomLevel);

    // In case there's no elevation data provider or if there's no data for this tile,
    // deactivate elevation data
    const auto elevationDataResource = captureElevationDataResource(tileIdN, currentState.zoomLevel);
    if (!elevationDataResource)
    {
        glUniform4f(program.vs.param.elevation_scale, 0.0f, 0.0f, 0.0f, 0.0f);
        GL_CHECK_RESULT;

        if (gpuAPI->isSupported_vertexShaderTextureLookup)
        {
            glActiveTexture(GL_TEXTURE0 + elevationDataSamplerIndex);
            GL_CHECK_RESULT;

            glBindTexture(GL_TEXTURE_2D, 0);
            GL_CHECK_RESULT;
        }
        else
        {
            if (activeElevationVertexAttribArray.isValid())
            {
                glDisableVertexAttribArray(*activeElevationVertexAttribArray + 0);
                GL_CHECK_RESULT;

                glDisableVertexAttribArray(*activeElevationVertexAttribArray + 1);
                GL_CHECK_RESULT;

                glDisableVertexAttribArray(*activeElevationVertexAttribArray + 2);
                GL_CHECK_RESULT;

                activeElevationVertexAttribArray.reset();
            }
        }

        return;
    }

    // Per-tile elevation data configuration
    const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(
        currentState.zoomLevel,
        tileIdN.y,
        AtlasMapRenderer::TileSize3D);
    const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(
        currentState.zoomLevel,
        tileIdN.y + 1,
        AtlasMapRenderer::TileSize3D);
    glUniform4f(program.vs.param.elevation_scale,
        upperMetersPerUnit,
        lowerMetersPerUnit,
        currentState.elevationConfiguration.zScaleFactor,
        currentState.elevationConfiguration.dataScaleFactor
    );
    GL_CHECK_RESULT;

    const auto& perTile_vs = program.vs.param.elevationDataLayer;

    if (gpuAPI->isSupported_vertexShaderTextureLookup)
    {
        glActiveTexture(GL_TEXTURE0 + elevationDataSamplerIndex);
        GL_CHECK_RESULT;

        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(elevationDataResource->refInGPU)));
        GL_CHECK_RESULT;

        gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + elevationDataSamplerIndex);

        if (elevationDataResource->type == GPUAPI::ResourceInGPU::Type::SlotOnAtlasTexture)
        {
            const auto tileOnAtlasTexture =
                std::static_pointer_cast<const GPUAPI::SlotOnAtlasTextureInGPU>(elevationDataResource);
            const auto& texture = tileOnAtlasTexture->atlasTexture;

            const auto rowIndex = tileOnAtlasTexture->slotIndex / texture->slotsPerSide;
            const auto colIndex = tileOnAtlasTexture->slotIndex - rowIndex * texture->slotsPerSide;
            const PointF texCoordsScale(
                texture->tileSizeN - 2.0f * texture->uHalfTexelSizeN - 2.0f * texture->uTexelSizeN,
                texture->tileSizeN - 2.0f * texture->vHalfTexelSizeN - 2.0f * texture->vTexelSizeN
            );
            const PointF texCoordsOffset(
                colIndex * texture->tileSizeN + texture->uHalfTexelSizeN,
                rowIndex * texture->tileSizeN + texture->vHalfTexelSizeN
            );

            glUniform4f(perTile_vs.texCoordsOffsetAndScale,
                texCoordsOffset.x,
                texCoordsOffset.y,
                texCoordsScale.x,
                texCoordsScale.y);
            GL_CHECK_RESULT;
            glUniform4f(perTile_vs.texelSize,
                texture->uTexelSizeN,
                texture->vTexelSizeN,
                texture->uHalfTexelSizeN,
                texture->vHalfTexelSizeN);
            GL_CHECK_RESULT;
        }
        else // if (elevationDataResource->type == GPUAPI::ResourceInGPU::Type::Texture)
        {
            const auto& texture = std::static_pointer_cast<const GPUAPI::TextureInGPU>(elevationDataResource);

            const PointF texCoordsScale(
                1.0f - 2.0f * texture->uHalfTexelSizeN - 2.0f * texture->uTexelSizeN,
                1.0f - 2.0f * texture->vHalfTexelSizeN - 2.0f * texture->vTexelSizeN
            );
            const PointF texCoordsOffset(
                texture->uHalfTexelSizeN + texture->uTexelSizeN,
                texture->vHalfTexelSizeN + texture->vTexelSizeN
            );

            glUniform4f(perTile_vs.texCoordsOffsetAndScale,
                texCoordsOffset.x,
                texCoordsOffset.y,
                texCoordsScale.x,
                texCoordsScale.y);
            GL_CHECK_RESULT;
            glUniform4f(perTile_vs.texelSize,
                texture->uTexelSizeN,
                texture->vTexelSizeN,
                texture->uHalfTexelSizeN,
                texture->vHalfTexelSizeN);
            GL_CHECK_RESULT;
        }
    }
    else
    {
        assert(elevationDataResource->type == GPUAPI::ResourceInGPU::Type::ArrayBuffer);

        const auto& arrayBuffer = std::static_pointer_cast<const GPUAPI::ArrayBufferInGPU>(elevationDataResource);
        assert(arrayBuffer->itemsCount == AtlasMapRenderer::HeixelsPerTileSide * AtlasMapRenderer::HeixelsPerTileSide);

        if (!activeElevationVertexAttribArray.isValid())
        {
            glEnableVertexAttribArray(*program.vs.in.vertexElevation + 0);
            GL_CHECK_RESULT;

            glEnableVertexAttribArray(*program.vs.in.vertexElevation + 1);
            GL_CHECK_RESULT;

            glEnableVertexAttribArray(*program.vs.in.vertexElevation + 2);
            GL_CHECK_RESULT;

            activeElevationVertexAttribArray = program.vs.in.vertexElevation;
        }

        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<intptr_t>(elevationDataResource->refInGPU)));
        GL_CHECK_RESULT;

        glVertexAttribPointer(*program.vs.in.vertexElevation + 0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(float) * 3 * 3,
            reinterpret_cast<void*>(sizeof(float) * 3 * 0));
        GL_CHECK_RESULT;

        glVertexAttribPointer(*program.vs.in.vertexElevation + 1,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(float) * 3 * 3,
            reinterpret_cast<void*>(sizeof(float) * 3 * 1));
        GL_CHECK_RESULT;

        glVertexAttribPointer(*program.vs.in.vertexElevation + 2,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(float) * 3 * 3,
            reinterpret_cast<void*>(sizeof(float) * 3 * 2));
        GL_CHECK_RESULT;
    }
}

QList< OsmAnd::Ref<OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::PerTileBatchedLayers> >
OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::batchLayersByTiles(const AtlasMapRendererInternalState& internalState)
{
    const auto gpuAPI = getGPUAPI();

    QList< Ref<PerTileBatchedLayers> > perTileBatchedLayers;
    auto stubsStyle = currentState.stubsStyle;
    for (const auto& mapLayerProvider : constOf(currentState.mapLayersProviders))
    {
        const auto layerStubsStyle = mapLayerProvider->getDesiredStubsStyle();
        if (layerStubsStyle == MapStubStyle::Unspecified)
            continue;

        stubsStyle = layerStubsStyle;
        break;
    }

    for (const auto& tileId : constOf(internalState.visibleTiles))
    {
        const auto tileIdN = Utilities::normalizeTileId(tileId, currentState.zoomLevel);

        bool atLeastOneNotUnavailable = false;
        MapRendererResourceState resourceState;

        auto batch = Ref<PerTileBatchedLayers>::New(tileId, true);
        perTileBatchedLayers.push_back(batch);

        for (const auto& mapLayerEntry : rangeOf(constOf(currentState.mapLayersProviders)))
        {
            const auto layerIndex = mapLayerEntry.key();
            const auto& provider = mapLayerEntry.value();
            const auto resourcesCollection = getResources().getCollectionSnapshot(
                MapRendererResourceType::MapLayer,
                std::dynamic_pointer_cast<IMapDataProvider>(provider));

            // In case there's no resources collection for this provider, there's nothing to do here, move on
            if (!resourcesCollection)
                continue;

            Ref<BatchedLayer> batchedLayer(new BatchedLayer(
                (std::dynamic_pointer_cast<IRasterMapLayerProvider>(provider) != nullptr)
                    ? BatchedLayerType::Raster
                    : BatchedLayerType::Other,
                layerIndex));

            // Try to obtain exact match resource
            const auto exactMatchGpuResource = captureLayerResource(
                resourcesCollection,
                tileIdN,
                currentState.zoomLevel,
                &resourceState);
            if (resourceState != MapRendererResourceState::Unavailable)
                atLeastOneNotUnavailable = true;
            if (exactMatchGpuResource)
            {
                // Exact match, no zoom shift or offset
                batchedLayer->resourcesInGPU.push_back(Ref<BatchedLayerResource>::New(exactMatchGpuResource));
            }
            else
            {
                // Exact match was not found, so now try to look for overscaled/underscaled resources, taking into account
                // MaxMissingDataZoomShift and current zoom. It's better to show Z-"nearest" resource available,
                // giving preference to underscaled resource
                for (int absZoomShift = 1; absZoomShift <= MapRenderer::MaxMissingDataZoomShift; absZoomShift++)
                {
                    // Look for underscaled first. Only full match is accepted
                    if (Q_LIKELY(!debugSettings->rasterLayersUnderscaleForbidden))
                    {
                        const auto underscaledZoom = static_cast<int>(currentState.zoomLevel) + absZoomShift;
                        if (underscaledZoom <= static_cast<int>(MaxZoomLevel))
                        {
                            const auto underscaledTileIdsN = Utilities::getTileIdsUnderscaledByZoomShift(
                                tileIdN,
                                absZoomShift);
                            const auto subtilesCount = underscaledTileIdsN.size();

                            bool atLeastOnePresent = false;
                            QVector< std::shared_ptr<const GPUAPI::ResourceInGPU> > gpuResources(subtilesCount);
                            auto pUnderscaledTileIdN = underscaledTileIdsN.constData();
                            auto pGpuResource = gpuResources.data();
                            for (auto tileIdx = 0; tileIdx < subtilesCount; tileIdx++)
                            {
                                const auto& underscaledTileId = *(pUnderscaledTileIdN++);
                                auto& gpuResource = *(pGpuResource++);

                                gpuResource = captureLayerResource(
                                    resourcesCollection,
                                    underscaledTileId,
                                    static_cast<ZoomLevel>(underscaledZoom));
                                if (gpuResource)
                                    atLeastOnePresent = true;
                            }

                            if (atLeastOnePresent)
                            {
                                const auto subtilesPerSide = (1u << absZoomShift);
                                const PointF texCoordsScale(subtilesPerSide, subtilesPerSide);

                                const auto& stubResource = atLeastOneNotUnavailable
                                    ? getResources().processingTileStubs[static_cast<int>(stubsStyle)]
                                    : getResources().unavailableTileStubs[static_cast<int>(stubsStyle)];

                                auto pGpuResource = gpuResources.constData();
                                for (auto subtileIdx = 0; subtileIdx < subtilesCount; subtileIdx++)
                                {
                                    const auto& gpuResource = *(pGpuResource++);

                                    if (gpuResource)
                                    {
                                        uint16_t xSubtile;
                                        uint16_t ySubtile;
                                        Utilities::decodeMortonCode(subtileIdx, xSubtile, ySubtile);

                                        batchedLayer->resourcesInGPU.push_back(Ref<BatchedLayerResource>::New(
                                            gpuResource,
                                            absZoomShift,
                                            PointF(-xSubtile, -ySubtile),
                                            texCoordsScale));
                                    }
                                    else
                                    {
                                        // Stub texture requires no texture offset or scaling
                                        batchedLayer->resourcesInGPU.push_back(Ref<BatchedLayerResource>::New(
                                            stubResource,
                                            absZoomShift));
                                    }
                                }

                                break;
                            }
                        }
                    }
                    
                    // If underscaled was not found, look for overscaled (surely, if such zoom level exists at all)
                    if (Q_LIKELY(!debugSettings->rasterLayersOverscaleForbidden))
                    {
                        const auto overscaleZoom = static_cast<int>(currentState.zoomLevel) - absZoomShift;
                        if (overscaleZoom >= static_cast<int>(MinZoomLevel))
                        {
                            PointF texCoordsOffset;
                            PointF texCoordsScale;
                            const auto overscaledTileIdN = Utilities::getTileIdOverscaledByZoomShift(
                                tileIdN,
                                absZoomShift,
                                &texCoordsOffset,
                                &texCoordsScale);
                            if (const auto gpuResource = captureLayerResource(
                                resourcesCollection,
                                overscaledTileIdN,
                                static_cast<ZoomLevel>(overscaleZoom)))
                            {
                                batchedLayer->resourcesInGPU.push_back(Ref<BatchedLayerResource>::New(
                                    gpuResource,
                                    -absZoomShift,
                                    texCoordsOffset,
                                    texCoordsScale));
                                break;
                            }
                        }
                    }
                }
            }
            if (!batchedLayer || batchedLayer->resourcesInGPU.isEmpty())
            {
                // In case this batch contains origin layer, and first layer is unavailable, then add a corresponding
                // stub into the batch
                if (batch->containsOriginLayer && layerIndex == currentState.mapLayersProviders.firstKey())
                {
                    const auto& stubResource = atLeastOneNotUnavailable
                        ? getResources().processingTileStubs[static_cast<int>(stubsStyle)]
                        : getResources().unavailableTileStubs[static_cast<int>(stubsStyle)];

                    auto batchedLayer = Ref<BatchedLayer>::New(BatchedLayerType::Raster, layerIndex);
                    batchedLayer->resourcesInGPU.push_back(Ref<BatchedLayerResource>::New(stubResource));
                    batch->layers.push_back(qMove(batchedLayer));
                }

                continue;
            }

            bool canBeBatched = !debugSettings->mapLayersBatchingForbidden;
            if (!batch->layers.isEmpty())
            {
                const auto& lastBatchedLayer = batch->layers.last();

                // Only raster layers can be batched
                canBeBatched = (lastBatchedLayer->type == BatchedLayerType::Raster);

                // Number of batched raster layers is limited
                canBeBatched = canBeBatched && (batch->layers.size() < _maxNumberOfRasterMapLayersInBatch);

                // Batching is possible only if all BatchedLayerResources are compatible
                canBeBatched = canBeBatched && (batchedLayer->resourcesInGPU.size() == lastBatchedLayer->resourcesInGPU.size());
                if (canBeBatched)
                {
                    const auto resourcesCount = batchedLayer->resourcesInGPU.size();
                    auto itResource1 = batchedLayer->resourcesInGPU.cbegin();
                    auto itResource2 = lastBatchedLayer->resourcesInGPU.cbegin();

                    for (auto resourceIndex = 0; resourceIndex < resourcesCount; resourceIndex++)
                    {
                        const auto& resource1 = *(itResource1++);
                        const auto& resource2 = *(itResource2++);

                        if (!resource1->canBeBatchedWith(*resource2) || !resource2->canBeBatchedWith(*resource1))
                        {
                            canBeBatched = false;
                            break;
                        }
                    }
                }
            }

            if (!canBeBatched && !batch->layers.isEmpty())
            {
                batch = new PerTileBatchedLayers(tileId, false);
                perTileBatchedLayers.push_back(batch);
            }
            batch->layers.push_back(qMove(batchedLayer));
        }

        // If there are no resources inside batch (and that batch is the only one),
        // insert an "unavailable" or "processing" stub for first provider
        if (batch->layers.isEmpty())
        {
            const auto& stubResource = atLeastOneNotUnavailable
                ? getResources().processingTileStubs[static_cast<int>(stubsStyle)]
                : getResources().unavailableTileStubs[static_cast<int>(stubsStyle)];

            auto batchedLayer = Ref<BatchedLayer>::New(BatchedLayerType::Raster, std::numeric_limits<int>::min());
            batchedLayer->resourcesInGPU.push_back(Ref<BatchedLayerResource>::New(stubResource));
            batch->layers.push_back(qMove(batchedLayer));
        }
    }

    // Finally sort per-tile batched layers, so that batches were rendered by layer indices order
    std::sort(perTileBatchedLayers.begin(), perTileBatchedLayers.end(),
        []
        (const Ref<PerTileBatchedLayers>& l, const Ref<PerTileBatchedLayers>& r) -> bool
        {
            return *l < *r;
        });

    return perTileBatchedLayers;
}

OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::BatchedLayerResource::BatchedLayerResource(
    const std::shared_ptr<const GPUAPI::ResourceInGPU>& resourceInGPU_,
    const int zoomShift_ /*= 0*/,
    const PointF texCoordsOffset_ /*= PointF(0.0f, 0.0f)*/,
    const PointF texCoordsScale_ /*= PointF(1.0f, 1.0f)*/)
    : resourceInGPU(resourceInGPU_)
    , zoomShift(zoomShift_)
    , texCoordsOffset(texCoordsOffset_)
    , texCoordsScale(texCoordsScale_)
{
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::BatchedLayerResource::canBeBatchedWith(
    const BatchedLayerResource& that) const
{
    return
        zoomShift == that.zoomShift &&
        texCoordsOffset == that.texCoordsOffset &&
        texCoordsScale == that.texCoordsScale;
}

OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::BatchedLayer::BatchedLayer(const BatchedLayerType type_, const int layerIndex_)
    : type(type_)
    , layerIndex(layerIndex_)
{
}

OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::PerTileBatchedLayers::PerTileBatchedLayers(
    const TileId tileId_,
    const bool containsOriginLayer_)
    : tileId(tileId_)
    , containsOriginLayer(containsOriginLayer_)
{
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::PerTileBatchedLayers::operator<(const PerTileBatchedLayers& that) const
{
    if (this == &that)
        return false;

    return layers.first()->layerIndex < that.layers.first()->layerIndex;
}
