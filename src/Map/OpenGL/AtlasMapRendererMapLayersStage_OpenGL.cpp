#include "AtlasMapRendererMapLayersStage_OpenGL.h"

#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "ignore_warnings_on_external_includes.h"
#include <SkColor.h>
#include "restore_internal_warnings.h"

#include "AtlasMapRenderer_OpenGL.h"
#include "AtlasMapRenderer_Metrics.h"
#include "IMapTiledDataProvider.h"
#include "IMapRasterBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
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

    GL_PUSH_GROUP_MARKER(QLatin1String("mapLayers"));

    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    QVector<unsigned int> rasterMapLayersBatch;
    rasterMapLayersBatch.reserve(_maxNumberOfRasterMapLayersInBatch);

    int lastUsedProgram = -1;
    const auto itMapLayersEnd = currentState.mapLayersProviders.cend();
    for (auto itMapLayerEntry = currentState.mapLayersProviders.cbegin(); itMapLayerEntry != itMapLayersEnd; ++itMapLayerEntry)
    {
        const auto layerIndex = itMapLayerEntry.key();
        const auto& provider = itMapLayerEntry.value();

        // Check that resources exist for this provider
        if (!getResources().getCollectionSnapshot(MapRendererResourceType::MapLayer, std::dynamic_pointer_cast<IMapDataProvider>(provider)))
            continue;

        if (const auto rasterMapLayerProvider = std::dynamic_pointer_cast<IMapRasterBitmapTileProvider>(provider))
        {
            if (!canRasterMapLayerBeBatched(rasterMapLayersBatch, layerIndex))
            {
                ok = ok && renderRasterLayers(rasterMapLayersBatch, lastUsedProgram);
                rasterMapLayersBatch.clear();
            }

            rasterMapLayersBatch.push_back(layerIndex);
        }
        else
        {
            if (!canRasterMapLayerBeBatched(rasterMapLayersBatch, layerIndex))
            {
                ok = ok && renderRasterLayers(rasterMapLayersBatch, lastUsedProgram);
                rasterMapLayersBatch.clear();
            }

            //TODO: process?
        }
    }

    if (!rasterMapLayersBatch.isEmpty())
    {
        ok = ok && renderRasterLayers(rasterMapLayersBatch, lastUsedProgram);
        rasterMapLayersBatch.clear();
    }

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    gpuAPI->unuseVAO();

    GL_POP_GROUP_MARKER;

    return ok;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::release()
{
    bool ok = true;
    ok = ok && releaseRasterLayers();
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
        1 /*tileSizeN*/ +
        1 /*tilePaddingN*/ +
        1 /*slotsPerSide*/ +
        1 /*slotIndex*/;
    const auto fsUniformsPerLayer =
        1 /*k*/ +
        1 /*sampler*/;
    const auto alreadyOccupiedUniforms =
        4 /*param_vs_mProjectionView*/ +
        1 /*param_vs_targetInTilePosN*/ +
        (!gpuAPI->isSupported_textureLod ? 0 :
            1 /*param_vs_distanceFromCameraToTarget*/ +
            1 /*param_vs_cameraElevationAngleN*/ +
            1 /*param_vs_groundCameraPosition*/ +
            1 /*param_vs_scaleToRetainProjectedSize*/
        ) +
        1 /*param_vs_tileCoordsOffset*/ +
        1 /*param_vs_elevationData_scaleFactor*/ +
        1 /*param_vs_elevationData_upperMetersPerUnit*/ +
        1 /*param_vs_elevationData_lowerMetersPerUnit*/ +
        (gpuAPI->isSupported_vertexShaderTextureLookup ? vsUniformsPerLayer : 0) /*param_vs_elevationDataLayer*/;
    _maxNumberOfRasterMapLayersInBatch = (gpuAPI->maxVertexUniformVectors - alreadyOccupiedUniforms) / (vsUniformsPerLayer + fsUniformsPerLayer);
    if (_maxNumberOfRasterMapLayersInBatch > gpuAPI->maxTextureUnitsInFragmentShader)
        _maxNumberOfRasterMapLayersInBatch = gpuAPI->maxTextureUnitsInFragmentShader;
    if (setupOptions.maxNumberOfRasterMapLayersInBatch != 0 && _maxNumberOfRasterMapLayersInBatch > setupOptions.maxNumberOfRasterMapLayersInBatch)
        _maxNumberOfRasterMapLayersInBatch = setupOptions.maxNumberOfRasterMapLayersInBatch;
    
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
            "Seems like buggy driver. This device should be capable of rendering %d raster map layers in batch, but only %d variant compiles",
            _maxNumberOfRasterMapLayersInBatch,
            supportedMaxNumberOfRasterMapLayersInBatch);
        _maxNumberOfRasterMapLayersInBatch = supportedMaxNumberOfRasterMapLayersInBatch;
    }
    if (_maxNumberOfRasterMapLayersInBatch < 1)
        return false;

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
        "    INPUT float in_vs_vertexElevation;                                                                             ""\n"
        "#endif // !VERTEX_TEXTURE_FETCH_SUPPORTED                                                                          ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "%UnrolledPerRasterLayerTexCoordsDeclarationCode%                                                                   ""\n"
        "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
        "    PARAM_OUTPUT float v2f_mipmapLOD;                                                                              ""\n"
        "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
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
        "                                                                                                                   ""\n"
        // Parameters: per-tile data
        "uniform ivec2 param_vs_tileCoordsOffset;                                                                           ""\n"
        "uniform float param_vs_elevationData_scaleFactor;                                                                  ""\n"
        "uniform float param_vs_elevationData_upperMetersPerUnit;                                                           ""\n"
        "uniform float param_vs_elevationData_lowerMetersPerUnit;                                                           ""\n"
        "#if VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                 ""\n"
        "    uniform highp sampler2D param_vs_elevationData_sampler;                                                        ""\n"
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
        "%UnrolledPerRasterLayerParamsDeclarationCode%                                                                      ""\n"
        "#if VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                 ""\n"
        "    uniform RasterLayerTile param_vs_elevationDataLayer;                                                           ""\n"
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
        "    float xOffset = float(param_vs_tileCoordsOffset.x) - param_vs_targetInTilePosN.x;                              ""\n"
        "    v.x += xOffset * %TileSize3D%.0;                                                                               ""\n"
        "    float yOffset = float(param_vs_tileCoordsOffset.y) - param_vs_targetInTilePosN.y;                              ""\n"
        "    v.z += yOffset * %TileSize3D%.0;                                                                               ""\n"
        "                                                                                                                   ""\n"
        //   Process each tile layer texture coordinates (except elevation)
        "%UnrolledPerRasterLayerTexCoordsProcessingCode%                                                                    ""\n"
        "                                                                                                                   ""\n"
        //   If elevation data is active, use it
        "    if (abs(param_vs_elevationData_scaleFactor) > 0.0)                                                             ""\n"
        "    {                                                                                                              ""\n"
        "        float metersToUnits = mix(param_vs_elevationData_upperMetersPerUnit,                                       ""\n"
        "            param_vs_elevationData_lowerMetersPerUnit, in_vs_vertexTexCoords.t);                                   ""\n"
        "                                                                                                                   ""\n"
        //       Calculate texcoords for elevation data (pixel-is-area)
        "        float heightInMeters;                                                                                      ""\n"
        "#if VERTEX_TEXTURE_FETCH_SUPPORTED                                                                                 ""\n"
        "        vec2 elevationDataTexCoords;                                                                               ""\n"
        "        calculateTextureCoordinates(                                                                               ""\n"
        "            param_vs_elevationDataLayer,                                                                           ""\n"
        "            elevationDataTexCoords);                                                                               ""\n"
        "        heightInMeters = SAMPLE_TEXTURE_2D(param_vs_elevationData_sampler, elevationDataTexCoords).r;              ""\n"
        "#else // !VERTEX_TEXTURE_FETCH_SUPPORTED                                                                           ""\n"
        "        heightInMeters = in_vs_vertexElevation;                                                                    ""\n"
        "#endif // VERTEX_TEXTURE_FETCH_SUPPORTED                                                                           ""\n"
        "                                                                                                                   ""\n"
        "        v.y = heightInMeters / metersToUnits;                                                                      ""\n"
        "        v.y *= param_vs_elevationData_scaleFactor;                                                                 ""\n"
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
        "uniform RasterLayerTile param_vs_rasterTileLayer_%rasterLayerIndex%;                                               ""\n");
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
        "                                                                                                                   ""\n"
        // Parameters: per-layer data
        "struct RasterLayerTile                                                                                             ""\n"
        "{                                                                                                                  ""\n"
        "    lowp float opacity;                                                                                            ""\n"
        "    lowp sampler2D sampler;                                                                                        ""\n"
        "};                                                                                                                 ""\n"
        "%UnrolledPerRasterLayerParamsDeclarationCode%                                                                      ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    lowp vec4 finalColor;                                                                                          ""\n"
        "                                                                                                                   ""\n"
        //   Mix colors of all layers
        "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
        "    finalColor = SAMPLE_TEXTURE_2D_LOD(                                                                            ""\n"
        "        param_fs_rasterTileLayer_0.sampler,                                                                        ""\n"
        "        v2f_texCoordsPerLayer_0, v2f_mipmapLOD);                                                                   ""\n"
        "#else // !TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
        "    finalColor = SAMPLE_TEXTURE_2D(                                                                                ""\n"
        "        param_fs_rasterTileLayer_0.sampler,                                                                        ""\n"
        "        v2f_texCoordsPerLayer_0);                                                                                  ""\n"
        "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
        "    finalColor.a *= param_fs_rasterTileLayer_0.opacity;                                                            ""\n"
        "%UnrolledPerRasterLayerProcessingCode%                                                                             ""\n"
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
        "            v2f_texCoordsPerLayer_%rasterLayerIndex%];                                                             ""\n"
        "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
        "                                                                                                                   ""\n"
        "        layerColor.a *= param_fs_rasterTileLayer_%rasterLayerIndex%.opacity;                                       ""\n"
        "        finalColor = mix(finalColor, layerColor, layerColor.a);                                                    ""\n"
        "    }                                                                                                              ""\n");
    const auto& fragmentShader_perRasterLayerTexCoordsDeclaration = QString::fromLatin1(
        "PARAM_INPUT vec2 v2f_texCoordsPerLayer_%rasterLayerIndex%;                                                         ""\n");
    const auto& fragmentShader_perRasterLayerParamsDeclaration = QString::fromLatin1(
        "uniform RasterLayerTile param_fs_rasterTileLayer_%rasterLayerIndex%;                                               ""\n");

    // Compile vertex shader
    auto preprocessedVertexShader = vertexShader;
    QString preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsProcessingCode;
    QString preprocessedVertexShader_UnrolledPerRasterLayerParamsDeclarationCode;
    QString preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsDeclarationCode;
    for (auto layerIndex = 0u; layerIndex < numberOfLayersInBatch; layerIndex++)
    {
        preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsProcessingCode +=
            detachedOf(vertexShader_perRasterLayerTexCoordsProcessing).replace("%rasterLayerIndex%", QString::number(layerIndex));

        preprocessedVertexShader_UnrolledPerRasterLayerParamsDeclarationCode +=
            detachedOf(vertexShader_perRasterLayerParamsDeclaration).replace("%rasterLayerIndex%", QString::number(layerIndex));

        preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsDeclarationCode +=
            detachedOf(vertexShader_perRasterLayerTexCoordsDeclaration).replace("%rasterLayerIndex%", QString::number(layerIndex));
    }
    preprocessedVertexShader.replace("%UnrolledPerRasterLayerTexCoordsProcessingCode%",
        preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsProcessingCode);
    preprocessedVertexShader.replace("%UnrolledPerRasterLayerParamsDeclarationCode%",
        preprocessedVertexShader_UnrolledPerRasterLayerParamsDeclarationCode);
    preprocessedVertexShader.replace("%UnrolledPerRasterLayerTexCoordsDeclarationCode%",
        preprocessedVertexShader_UnrolledPerRasterLayerTexCoordsDeclarationCode);
    preprocessedVertexShader.replace("%TileSize3D%", QString::number(AtlasMapRenderer::TileSize3D));
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    if (vsId == 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererMapLayersStage_OpenGL vertex shader for %d raster map layers", numberOfLayersInBatch);
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
            detachedOf(fragmentShader_perRasterLayerTexCoordsDeclaration).replace("%rasterLayerIndex%", QString::number(layerIndex));

        preprocessedFragmentShader_UnrolledPerRasterLayerParamsDeclarationCode +=
            detachedOf(fragmentShader_perRasterLayerParamsDeclaration).replace("%rasterLayerIndex%", QString::number(layerIndex));

        if (layerIndex > 0)
        {
            preprocessedFragmentShader_UnrolledPerRasterLayerProcessingCode +=
                detachedOf(fragmentShader_perRasterLayer).replace("%rasterLayerIndex%", QString::number(layerIndex));;
        }
    }
    preprocessedFragmentShader.replace("%UnrolledPerRasterLayerTexCoordsDeclarationCode%", preprocessedFragmentShader_UnrolledPerRasterLayerTexCoordsDeclarationCode);
    preprocessedFragmentShader.replace("%UnrolledPerRasterLayerParamsDeclarationCode%", preprocessedFragmentShader_UnrolledPerRasterLayerParamsDeclarationCode);
    preprocessedFragmentShader.replace("%UnrolledPerRasterLayerProcessingCode%", preprocessedFragmentShader_UnrolledPerRasterLayerProcessingCode);
    gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
    gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    if (fsId == 0)
    {
        glDeleteShader(vsId);
        GL_CHECK_RESULT;

        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererMapLayersStage_OpenGL fragment shader for %d raster map layers", numberOfLayersInBatch);
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
    ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.in.vertexTexCoords, "in_vs_vertexTexCoords", GlslVariableType::In);
    if (!gpuAPI->isSupported_vertexShaderTextureLookup)
    {
        ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.in.vertexElevation, "in_vs_vertexElevation", GlslVariableType::In);
    }
    ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.mProjectionView, "param_vs_mProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.targetInTilePosN, "param_vs_targetInTilePosN", GlslVariableType::Uniform);
    if (gpuAPI->isSupported_textureLod)
    {
        ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.distanceFromCameraToTarget, "param_vs_distanceFromCameraToTarget", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.cameraElevationAngleN, "param_vs_cameraElevationAngleN", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.groundCameraPosition, "param_vs_groundCameraPosition", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.scaleToRetainProjectedSize, "param_vs_scaleToRetainProjectedSize", GlslVariableType::Uniform);
    }
    ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.tileCoordsOffset, "param_vs_tileCoordsOffset", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.elevationData_scaleFactor, "param_vs_elevationData_scaleFactor", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.elevationData_upperMetersPerUnit, "param_vs_elevationData_upperMetersPerUnit", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.elevationData_lowerMetersPerUnit, "param_vs_elevationData_lowerMetersPerUnit", GlslVariableType::Uniform);
    if (gpuAPI->isSupported_vertexShaderTextureLookup)
    {
        ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.elevationData_sampler, "param_vs_elevationData_sampler", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.elevationDataLayer.tileSizeN, "param_vs_elevationDataLayer.tileSizeN", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.elevationDataLayer.tilePaddingN, "param_vs_elevationDataLayer.tilePaddingN", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.elevationDataLayer.slotsPerSide, "param_vs_elevationDataLayer.slotsPerSide", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(outRasterLayerTileProgram.vs.param.elevationDataLayer.slotIndex, "param_vs_elevationDataLayer.slotIndex", GlslVariableType::Uniform);
    }
    outRasterLayerTileProgram.vs.param.rasterTileLayers.resize(numberOfLayersInBatch);
    outRasterLayerTileProgram.fs.param.rasterTileLayers.resize(numberOfLayersInBatch);
    for (auto layerIndex = 0u; layerIndex < numberOfLayersInBatch; layerIndex++)
    {
        // Vertex shader
        {
            auto layerStructName =
                QString::fromLatin1("param_vs_rasterTileLayer_%layerIndex%")
                .replace(QLatin1String("%layerIndex%"), QString::number(layerIndex));
            auto& layerStruct = outRasterLayerTileProgram.vs.param.rasterTileLayers[layerIndex];

            ok = ok && lookup->lookupLocation(layerStruct.tileSizeN, layerStructName + ".tileSizeN", GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(layerStruct.tilePaddingN, layerStructName + ".tilePaddingN", GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(layerStruct.slotsPerSide, layerStructName + ".slotsPerSide", GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(layerStruct.slotIndex, layerStructName + ".slotIndex", GlslVariableType::Uniform);
        }

        // Fragment shader
        {
            auto layerStructName =
                QString::fromLatin1("param_fs_rasterTileLayer_%layerIndex%")
                .replace(QLatin1String("%layerIndex%"), QString::number(layerIndex));
            auto& layerStruct = outRasterLayerTileProgram.fs.param.rasterTileLayers[layerIndex];

            ok = ok && lookup->lookupLocation(layerStruct.opacity, layerStructName + ".opacity", GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(layerStruct.sampler, layerStructName + ".sampler", GlslVariableType::Uniform);
        }
    }

    return ok;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::canRasterMapLayerBeBatched(
    const QVector<unsigned int>& batchedLayerIndices,
    const unsigned int layerIndex)
{
    if (batchedLayerIndices.size() >= _maxNumberOfRasterMapLayersInBatch)
        return false;

    return true;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::renderRasterLayers(
    const QVector<unsigned int>& batchedLayerIndices,
    int& lastUsedProgram)
{
    const auto gpuAPI = getGPUAPI();

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

    const auto& currentConfiguration = getCurrentConfiguration();
    const auto& internalState = getInternalState();

    const auto batchedLayersCount = batchedLayerIndices.size();
    const auto& program = _rasterLayerTilePrograms[batchedLayersCount];
    const auto& vao = _rasterTileVAOs[batchedLayersCount];
    const bool elevationDataEnabled = static_cast<bool>(currentState.elevationDataProvider);

    // Check if correct program is being used
    if (lastUsedProgram != *program.id)
    {
        GL_PUSH_GROUP_MARKER(QString("use '%1-batched-raster-map-layers' program").arg(batchedLayersCount));

        // Raster symbols use premultiplied alpha (due to SKIA)
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        GL_CHECK_RESULT;

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

        // Configure samplers
        if (gpuAPI->isSupported_vertexShaderTextureLookup)
        {
            glUniform1i(program.vs.param.elevationData_sampler, 0);
            GL_CHECK_RESULT;

            gpuAPI->setTextureBlockSampler(GL_TEXTURE0, GPUAPI_OpenGL::SamplerType::ElevationDataTile);
        }
        auto bitmapTileSamplerType = GPUAPI_OpenGL::SamplerType::BitmapTile_Bilinear;
        if (gpuAPI->isSupported_textureLod)
        {
            switch(currentConfiguration.texturesFilteringQuality)
            {
            case TextureFilteringQuality::Good:
                bitmapTileSamplerType = GPUAPI_OpenGL::SamplerType::BitmapTile_BilinearMipmap;
                break;
            case TextureFilteringQuality::Best:
                bitmapTileSamplerType = GPUAPI_OpenGL::SamplerType::BitmapTile_TrilinearMipmap;
                break;
            }
        }

        for (auto layerLinearIdx = 0u; layerLinearIdx < batchedLayersCount; layerLinearIdx++)
        {
            const auto samplerIndex = (gpuAPI->isSupported_vertexShaderTextureLookup ? 1 : 0) + layerLinearIdx;

            glUniform1i(program.fs.param.rasterTileLayers[layerLinearIdx].sampler, samplerIndex);
            GL_CHECK_RESULT;

            gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + samplerIndex, bitmapTileSamplerType);
        }

        // Check if we need to process elevation data
        if (!elevationDataEnabled)
        {
            // We have no elevation data provider, so we can not do anything
            glUniform1f(program.vs.param.elevationData_scaleFactor, 0.0f);
            GL_CHECK_RESULT;
        }

        lastUsedProgram = program.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QLatin1String("tiles"));

    bool elevationVertexAttribArrayEnabled = false;
    if (!gpuAPI->isSupported_vertexShaderTextureLookup)
    {
        glDisableVertexAttribArray(*program.vs.in.vertexElevation);
        GL_CHECK_RESULT;
    }

    // For each visible tile, render it
    for(const auto& tileId : constOf(internalState.visibleTiles))
    {
        // Get normalized tile index
        auto tileIdN = Utilities::normalizeTileId(tileId, currentState.zoomBase);

        GL_PUSH_GROUP_MARKER(QString("%1x%2@%3").arg(tileIdN.x).arg(tileIdN.y).arg(currentState.zoomBase));

        // Set tile coordinates offset
        glUniform2i(program.vs.param.tileCoordsOffset,
            tileId.x - internalState.targetTileId.x,
            tileId.y - internalState.targetTileId.y);
        GL_CHECK_RESULT;

        // Set elevation data
        bool appliedElevationVertexAttribArray = false;
        if (elevationDataEnabled)
        {
            const auto& resourcesCollection_ = getResources().getCollectionSnapshot(MapRendererResourceType::ElevationData, currentState.elevationDataProvider);
            const auto& resourcesCollection = std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection_);

            // Obtain tile entry by normalized tile coordinates, since tile may repeat several times
            std::shared_ptr<const GPUAPI::ResourceInGPU> gpuResource;
            std::shared_ptr<MapRendererBaseTiledResource> resource_;
            if (resourcesCollection->obtainResource(tileIdN, currentState.zoomBase, resource_))
            {
                const auto resource = std::static_pointer_cast<MapRendererElevationDataResource>(resource_);

                // Check state and obtain GPU resource
                if (resource->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
                {
                    // Capture GPU resource
                    gpuResource = resource->resourceInGPU;

                    resource->setState(MapRendererResourceState::Uploaded);
                }
            }

            if (!gpuResource)
            {
                // We have no elevation data, so we can not do anything
                glUniform1f(program.vs.param.elevationData_scaleFactor, 0.0f);
                GL_CHECK_RESULT;
            }
            else
            {
                glUniform1f(program.vs.param.elevationData_scaleFactor, currentState.elevationDataConfiguration.scaleFactor);
                GL_CHECK_RESULT;

                const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(currentState.zoomBase, tileIdN.y, AtlasMapRenderer::TileSize3D);
                glUniform1f(program.vs.param.elevationData_upperMetersPerUnit, upperMetersPerUnit);
                const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(currentState.zoomBase, tileIdN.y + 1, AtlasMapRenderer::TileSize3D);
                glUniform1f(program.vs.param.elevationData_lowerMetersPerUnit, lowerMetersPerUnit);

                const auto& perTile_vs = program.vs.param.elevationDataLayer;

                if (gpuAPI->isSupported_vertexShaderTextureLookup)
                {
                    glActiveTexture(GL_TEXTURE0);
                    GL_CHECK_RESULT;

                    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
                    GL_CHECK_RESULT;

                    gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0);

                    if (gpuResource->type == GPUAPI::ResourceInGPU::Type::SlotOnAtlasTexture)
                    {
                        const auto tileOnAtlasTexture = std::static_pointer_cast<const GPUAPI::SlotOnAtlasTextureInGPU>(gpuResource);

                        glUniform1i(perTile_vs.slotIndex, tileOnAtlasTexture->slotIndex);
                        GL_CHECK_RESULT;
                        glUniform1f(perTile_vs.tileSizeN, tileOnAtlasTexture->atlasTexture->tileSizeN);
                        GL_CHECK_RESULT;
                        glUniform1f(perTile_vs.tilePaddingN, tileOnAtlasTexture->atlasTexture->uHalfTexelSizeN);
                        GL_CHECK_RESULT;
                        glUniform1i(perTile_vs.slotsPerSide, tileOnAtlasTexture->atlasTexture->slotsPerSide);
                        GL_CHECK_RESULT;
                    }
                    else
                    {
                        const auto& texture = std::static_pointer_cast<const GPUAPI::TextureInGPU>(gpuResource);

                        glUniform1i(perTile_vs.slotIndex, 0);
                        GL_CHECK_RESULT;
                        glUniform1f(perTile_vs.tileSizeN, 1.0f);
                        GL_CHECK_RESULT;
                        glUniform1f(perTile_vs.tilePaddingN, texture->uHalfTexelSizeN);
                        GL_CHECK_RESULT;
                        glUniform1i(perTile_vs.slotsPerSide, 1);
                        GL_CHECK_RESULT;
                    }
                }
                else
                {
                    assert(gpuResource->type == GPUAPI::ResourceInGPU::Type::ArrayBuffer);

                    const auto& arrayBuffer = std::static_pointer_cast<const GPUAPI::ArrayBufferInGPU>(gpuResource);
                    assert(arrayBuffer->itemsCount == currentConfiguration.heixelsPerTileSide*currentConfiguration.heixelsPerTileSide);

                    if (!elevationVertexAttribArrayEnabled)
                    {
                        glEnableVertexAttribArray(*program.vs.in.vertexElevation);
                        GL_CHECK_RESULT;

                        elevationVertexAttribArrayEnabled = true;
                    }

                    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
                    GL_CHECK_RESULT;

                    glVertexAttribPointer(*program.vs.in.vertexElevation, 1, GL_FLOAT, GL_FALSE, sizeof(float), nullptr);
                    GL_CHECK_RESULT;
                    appliedElevationVertexAttribArray = true;
                }
            }
        }

        // We need to pass each layer of this tile to shader
        for (int layerLinearIdx = 0; layerLinearIdx < batchedLayersCount; layerLinearIdx++)
        {
            const auto layerIndex = batchedLayerIndices[layerLinearIdx];
            const auto layerConfiguration = currentState.mapLayersConfigurations[layerIndex];

            // Get resources collection
            const auto& resourcesCollection_ = getResources().getCollectionSnapshot(
                MapRendererResourceType::MapLayer,
                std::dynamic_pointer_cast<IMapDataProvider>(currentState.mapLayersProviders[layerIndex]));
            const auto& resourcesCollection = std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection_);

            const auto& perTile_vs = program.vs.param.rasterTileLayers[layerLinearIdx];
            const auto& perTile_fs = program.fs.param.rasterTileLayers[layerLinearIdx];
            const auto samplerIndex = (gpuAPI->isSupported_vertexShaderTextureLookup ? 1 : 0) + layerLinearIdx;

            // Obtain tile entry by normalized tile coordinates, since tile may repeat several times
            std::shared_ptr<const GPUAPI::ResourceInGPU> gpuResource;
            std::shared_ptr<MapRendererBaseTiledResource> resource_;
            if (resourcesCollection->obtainResource(tileIdN, currentState.zoomBase, resource_))
            {
                const auto resource = std::static_pointer_cast<MapRendererRasterBitmapTileResource>(resource_);

                // Check state and obtain GPU resource
                if (resource->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
                {
                    // Capture GPU resource
                    gpuResource = resource->resourceInGPU;

                    resource->setState(MapRendererResourceState::Uploaded);
                }
                else if (resource->getState() == MapRendererResourceState::Unavailable)
                    gpuResource = getResources().unavailableTileStub;
                else
                    gpuResource = getResources().processingTileStub;
            }
            else
            {
                // No resource entry means that it's not yet processed
                gpuResource = getResources().processingTileStub;
            }

            glUniform1f(perTile_fs.opacity, layerConfiguration.opacity);
            GL_CHECK_RESULT;

            glActiveTexture(GL_TEXTURE0 + samplerIndex);
            GL_CHECK_RESULT;

            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
            GL_CHECK_RESULT;

            gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + samplerIndex);

            if (gpuResource->type == GPUAPI::ResourceInGPU::Type::SlotOnAtlasTexture)
            {
                const auto tileOnAtlasTexture = std::static_pointer_cast<const GPUAPI::SlotOnAtlasTextureInGPU>(gpuResource);

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

        if (!appliedElevationVertexAttribArray && elevationVertexAttribArrayEnabled)
        {
            elevationVertexAttribArrayEnabled = false;

            glDisableVertexAttribArray(*program.vs.in.vertexElevation);
            GL_CHECK_RESULT;
        }

        glDrawElements(GL_TRIANGLES, _rasterTileIndicesCount, GL_UNSIGNED_SHORT, nullptr);
        GL_CHECK_RESULT;

        GL_POP_GROUP_MARKER;
    }

    // Disable textures
    for(int layerLinearIdx = 0, count = (gpuAPI->isSupported_vertexShaderTextureLookup ? 1 : 0) + batchedLayersCount; layerLinearIdx < count; layerLinearIdx++)
    {
        glActiveTexture(GL_TEXTURE0 + layerLinearIdx);
        GL_CHECK_RESULT;

        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_RESULT;
    }

    // Unbind any binded buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    // Disable elevation vertex attrib array (if enabled)
    if (elevationVertexAttribArrayEnabled)
    {
        glDisableVertexAttribArray(*program.vs.in.vertexElevation);
        GL_CHECK_RESULT;
    }

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::releaseRasterLayers()
{
    GL_CHECK_PRESENT(glDeleteProgram);

    _maxNumberOfRasterMapLayersInBatch = 0;

    releaseRasterTile();

    for (auto& rasterLayerTileProgram : _rasterLayerTilePrograms)
    {
        if (rasterLayerTileProgram.id.isValid())
        {
            glDeleteProgram(rasterLayerTileProgram.id);
            GL_CHECK_RESULT;
            rasterLayerTileProgram = RasterLayerTileProgram();
        }
    }
    _rasterLayerTilePrograms.clear();

    return true;
}

void OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::initializeRasterTile()
{
    const auto gpuAPI = getGPUAPI();
    const auto& currentConfiguration = *this->currentConfiguration;

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

    if (!currentState.elevationDataProvider)
    {
        // Simple tile patch, that consists of 4 vertices

        // Vertex data
        const GLfloat tsz = static_cast<GLfloat>(AtlasMapRenderer::TileSize3D);
        static Vertex vertices[4] =
        {
            // In OpenGL, UV origin is BL. But since same rule applies to uploading texture data,
            // texture in memory is vertically flipped, so swap bottom and top UVs
            { { 0.0f,  tsz }, { 0.0f, 1.0f } },//BL
            { { 0.0f, 0.0f }, { 0.0f, 0.0f } },//TL
            { {  tsz, 0.0f }, { 1.0f, 0.0f } },//TR
            { {  tsz,  tsz }, { 1.0f, 1.0f } } //BR
        };
        pVertices = new Vertex[4];
        memcpy(pVertices, vertices, 4 * sizeof(Vertex));
        verticesCount = 4;

        // Index data
        static GLushort indices[6] =
        {
            0, 1, 2,
            0, 2, 3
        };
        pIndices = new GLushort[6];
        memcpy(pIndices, indices, 6 * sizeof(GLushort));
        indicesCount = 6;
    }
    else
    {
        // Complex tile patch, that consists of (heightPrimitivesPerSide*heightPrimitivesPerSide) number of
        // height clusters. Height cluster itself consists of 4 vertices, 6 indices and 2 polygons
        const auto heightPrimitivesPerSide = currentConfiguration.heixelsPerTileSide - 1;
        const GLfloat clusterSize = static_cast<GLfloat>(AtlasMapRenderer::TileSize3D) / static_cast<float>(heightPrimitivesPerSide);
        verticesCount = currentConfiguration.heixelsPerTileSide * currentConfiguration.heixelsPerTileSide;
        pVertices = new Vertex[verticesCount];
        indicesCount = (heightPrimitivesPerSide * heightPrimitivesPerSide) * 6;
        pIndices = new GLushort[indicesCount];

        Vertex* pV = pVertices;

        // Form vertices
        assert(verticesCount <= std::numeric_limits<GLushort>::max());
        for(auto row = 0u, count = currentConfiguration.heixelsPerTileSide; row < count; row++)
        {
            for(auto col = 0u, count = currentConfiguration.heixelsPerTileSide; col < count; col++, pV++)
            {
                pV->positionXZ[0] = static_cast<float>(col)* clusterSize;
                pV->positionXZ[1] = static_cast<float>(row)* clusterSize;

                pV->textureUV[0] = static_cast<float>(col) / static_cast<float>(heightPrimitivesPerSide);
                pV->textureUV[1] = static_cast<float>(row) / static_cast<float>(heightPrimitivesPerSide);
            }
        }

        // Form indices
        GLushort* pI = pIndices;
        for(auto row = 0u; row < heightPrimitivesPerSide; row++)
        {
            for(auto col = 0u; col < heightPrimitivesPerSide; col++)
            {
                const auto p0 = (row + 1) * currentConfiguration.heixelsPerTileSide + col + 0;//BL
                const auto p1 = (row + 0) * currentConfiguration.heixelsPerTileSide + col + 0;//TL
                const auto p2 = (row + 0) * currentConfiguration.heixelsPerTileSide + col + 1;//TR
                const auto p3 = (row + 1) * currentConfiguration.heixelsPerTileSide + col + 1;//BR
                assert(p0 <= verticesCount);
                assert(p1 <= verticesCount);
                assert(p2 <= verticesCount);
                assert(p3 <= verticesCount);

                // Triangle 0
                pI[0] = p0;
                pI[1] = p1;
                pI[2] = p2;
                pI += 3;

                // Triangle 1
                pI[0] = p0;
                pI[1] = p2;
                pI[2] = p3;
                pI += 3;
            }
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
        glVertexAttribPointer(*rasterLayerTileProgram.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, positionXZ)));
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*rasterLayerTileProgram.vs.in.vertexTexCoords);
        GL_CHECK_RESULT;
        glVertexAttribPointer(*rasterLayerTileProgram.vs.in.vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, textureUV)));
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

void OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::releaseRasterTile()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);

    for (auto& rasterTileVAO : _rasterTileVAOs)
    {
        if (rasterTileVAO.isValid())
        {
            gpuAPI->releaseVAO(rasterTileVAO);
            rasterTileVAO.reset();
        }
    }

    if (_rasterTileIBO.isValid())
    {
        glDeleteBuffers(1, &_rasterTileIBO);
        GL_CHECK_RESULT;
        _rasterTileIBO.reset();
    }
    if (_rasterTileVBO.isValid())
    {
        glDeleteBuffers(1, &_rasterTileVBO);
        GL_CHECK_RESULT;
        _rasterTileVBO.reset();
    }
    _rasterTileIndicesCount = -1;
}

void OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::updateRasterTile()
{
    releaseRasterTile();
    initializeRasterTile();
}
