#include "AtlasMapRendererMapLayersStage_OpenGL.h"

#include <cassert>
#include <algorithm>

#include "ignore_warnings_on_external_includes.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
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

// Size of wind particle in pixels (default is 5.0f)
const float OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::_particleSize = 5.0f;

// Minimum zoom level for constant speed of particles on screen (if less - speed is constant on map)
const OsmAnd::ZoomLevel OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::_particleConstantSpeedMinZoom = ZoomLevel6;

// Factor for speed of wind particles (default is 1.0f - for 1 hour per second on _particleConstantSpeedMinZoom)
const float OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::_particleSpeedFactor = 1.0f;

OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::AtlasMapRendererMapLayersStage_OpenGL(AtlasMapRenderer_OpenGL* renderer_)
    : AtlasMapRendererMapLayersStage(renderer_)
    , AtlasMapRendererStageHelper_OpenGL(this)
    , _maxNumberOfRasterMapLayersInBatch(0)
    , _rasterTileIndicesCount(-1)
{
}

OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::~AtlasMapRendererMapLayersStage_OpenGL() = default;

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::initialize()
{
    bool ok = true;
    ok = ok && initializeRasterLayers();
    return ok;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* metric_)
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

    // Calculate my location parameters
    const auto offset31 = Utilities::shortestVector31(currentState.target31, currentState.myLocation31);
    const auto offset = Utilities::convert31toFloat(offset31, currentState.zoomLevel) * AtlasMapRenderer::TileSize3D;
    myLocation = glm::vec3(offset.x, renderer->getHeightOfLocation(currentState, currentState.myLocation31), offset.y);
    auto metersPerTile = Utilities::getMetersPerTileUnit(currentState.zoomLevel,
        currentState.myLocation31.y >> (ZoomLevel31 - currentState.zoomLevel), AtlasMapRenderer::TileSize3D);
    myLocationRadius = currentState.myLocationRadiusInMeters / metersPerTile;
    headingDirection = qDegreesToRadians(qIsNaN(currentState.myDirection)
        ? Utilities::normalizedAngleDegrees(currentState.azimuth + 180.0f) : currentState.myDirection);
    const float cameraHeight = internalState.distanceFromCameraToGround;
    const float sizeScale = cameraHeight > myLocation.y && !qFuzzyIsNull(cameraHeight)
        ? 1.0f - myLocation.y / cameraHeight : 1.0f;
    headingRadius = currentState.myDirectionRadius * internalState.pixelInWorldProjectionScale * sizeScale;

    GLname lastUsedProgram;
    bool withElevation = true;
    bool haveElevation = false;
    auto tilesBegin = internalState.frustumTiles.cbegin();
    for (auto itTiles = internalState.frustumTiles.cend(); itTiles != tilesBegin; itTiles--)
    {
        const auto& tilesEntry = itTiles - 1;
        const auto& visibleTilesSet = internalState.visibleTilesSet.constFind(tilesEntry.key());
        if (visibleTilesSet == internalState.visibleTilesSet.cend())
            continue;                
        const auto& batchedLayersByTiles =
            batchLayersByTiles(tilesEntry.value(), visibleTilesSet.value(), tilesEntry.key());
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
                    lastUsedProgram,
                    haveElevation,
                    withElevation,
                    blendingEnabled,
                    tilesEntry.key());
            }
        }

        if (!haveElevation) withElevation = false;

        // Deactivate program
        if (lastUsedProgram.isValid())
        {
            glUseProgram(0);
            GL_CHECK_RESULT;

            // Also un-use any possibly used VAO
            gpuAPI->unuseVAO();

            lastUsedProgram.reset();
        }
    }

    GL_POP_GROUP_MARKER;

    return ok;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::release(bool gpuContextLost)
{
    bool ok = true;
    ok = ok && releaseRasterLayers(gpuContextLost);
    return ok;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::initializeRasterLayers()
{
    const auto gpuAPI = getGPUAPI();

    // Determine maximum number of raster layers in one batch...

    // ... by uniforms
    const auto vsUniformsPerLayer =
        1 /*texCoordsOffsetAndScale*/;
    const auto fsUniformsPerLayer =
        1 /*opacityFactor*/ +
        1 /*texCoordsOffsetAndScale*/;
        1 /*transitionPhase*/ +
        1 /*texelSize*/ +
        1 /*isPremultipliedAlpha*/ +
        1 /*sampler*/;
    const auto vsOtherUniforms =
        4 /*param_vs_mPerspectiveProjectionView*/ +
        1 /*param_vs_resultScale*/ +
        1 /*param_vs_targetInTilePosN*/ +
        1 /*param_vs_tileSize*/ +
        (!gpuAPI->isSupported_textureLod
            ? 0
            : 1 /*param_vs_distanceFromCameraToTarget*/ +
              1 /*param_vs_cameraElevationAngleN*/ +
              1 /*param_vs_groundCameraPosition*/ +
              1 /*param_vs_scaleToRetainProjectedSize*/) +
        1 /*param_vs_elevation_configuration*/ +
        1 /*param_vs_elevation_hillshadeConfiguration*/ +
        8 /*param_vs_elevation_colorMapKeys*/ +
        8 /*param_vs_elevation_colorMapValues*/ +
        1 /*param_vs_tileCoords31*/ +
        1 /*param_vs_primaryGridTileTop*/ +
        1 /*param_vs_primaryGridTileBot*/ +
        1 /*param_vs_secondaryGridTileTop*/ +
        1 /*param_vs_secondaryGridTileBot*/ +
        1 /*param_vs_tileCoordsOffset*/ +
        1 /*param_vs_elevation_scale*/ +
        vsUniformsPerLayer /*param_vs_elevationLayer*/ +
        1 /*param_vs_elevationLayerTexelSize*/ +
        1 /*param_vs_elevationLayerDataPlace*/ +
        1 /*param_vs_primaryGridAxisX*/ +
        1 /*param_vs_secondaryGridAxisX*/ +
        1 /*param_vs_primaryGridAxisY*/ +
        1 /*param_vs_secondaryGridAxisY*/;
    const auto fsOtherUniforms =
        1 /*param_fs_lastBatch*/ +
        1 /*param_fs_blendingEnabled*/ + 
        1 /*param_fs_backgroundColor*/ +
        1 /*param_fs_myLocationColor*/ +
        1 /*param_fs_myLocation*/ +
        1 /*param_fs_myDirection*/ +
        1 /*param_fs_gridParameters*/ +
        1 /*param_fs_primaryGridColor*/ +
        1 /*param_fs_secondaryGridColor*/ +
        1 /*param_fs_worldCameraPosition*/ +
        1 /*param_fs_mistConfiguration*/ +
        1 /*param_fs_mistColor*/;
    const auto maxBatchSizeByUniforms =
        (gpuAPI->maxVertexUniformVectors - vsOtherUniforms - fsOtherUniforms) /
        (vsUniformsPerLayer + fsUniformsPerLayer);

    // ... by varying floats
    const auto varyingFloatsPerLayer =
        2 /*v2f_texCoordsPerLayer_%rasterLayerIndex%*/;
    const auto otherVaryingFloats =
        1 /*v2f_metersPerUnit*/ +
        (gpuAPI->isSupported_textureLod ? 1 : 0) /*v2f_mipmapLOD*/ +
        (setupOptions.elevationVisualizationEnabled ? 4 : 0) /*v2f_elevationColor*/ +
        4 /*v2f_primaryLocation*/ +
        4 /*v2f_secondaryLocation*/ +
        4 /*v2f_position*/;
    const auto maxBatchSizeByVaryingFloats =
        (gpuAPI->maxVaryingFloats - otherVaryingFloats) / varyingFloatsPerLayer;

    // ... by varying vectors
    const auto varyingVectorsPerLayer =
        1 /*v2f_texCoordsPerLayer_%rasterLayerIndex%*/;
    const auto otherVaryingVectors =
        1 /*v2f_metersPerUnit*/ +
        (gpuAPI->isSupported_textureLod ? 1 : 0) /*v2f_mipmapLOD*/ +
        (setupOptions.elevationVisualizationEnabled ? 1 : 0) /*v2f_elevationColor*/ +
        1 /*v2f_primaryLocation*/ +
        1 /*v2f_secondaryLocation*/ +
        1 /*v2f_position*/;
    const auto maxBatchSizeByVaryingVectors =
        (gpuAPI->maxVaryingVectors - otherVaryingVectors) / varyingVectorsPerLayer;

    // ... by texture units
    const auto maxBatchSizeByTextureUnits = std::min({
        gpuAPI->maxTextureUnitsCombined - 1,
        gpuAPI->maxTextureUnitsInFragmentShader
    });

    // Get minimal and limit further with override if set
    const auto previousMaxNumberOfRasterMapLayersInBatch = _maxNumberOfRasterMapLayersInBatch;
    _maxNumberOfRasterMapLayersInBatch = std::min({
        maxBatchSizeByUniforms,
        maxBatchSizeByVaryingFloats,
        maxBatchSizeByVaryingVectors,
        maxBatchSizeByTextureUnits
    });
    if (setupOptions.maxNumberOfRasterMapLayersInBatch != 0 &&
        _maxNumberOfRasterMapLayersInBatch > setupOptions.maxNumberOfRasterMapLayersInBatch)
    {
        _maxNumberOfRasterMapLayersInBatch = setupOptions.maxNumberOfRasterMapLayersInBatch;
    }
    LogPrintf(LogSeverityLevel::Info,
        "Maximal number of raster map layers in a batch is %d (%d by uniforms, %d by varying floats, %d by varying vectors, %d by texture units)",
        _maxNumberOfRasterMapLayersInBatch,
        maxBatchSizeByUniforms,
        maxBatchSizeByVaryingFloats,
        maxBatchSizeByVaryingVectors,
        maxBatchSizeByTextureUnits
    );

    // Initialize programs that support [1 ... _maxNumberOfRasterMapLayersInBatch] as number of layers
    auto supportedMaxNumberOfRasterMapLayersInBatch = _maxNumberOfRasterMapLayersInBatch;
    if (_rasterLayerTilePrograms.size() != _maxNumberOfRasterMapLayersInBatch)
    {
        _rasterLayerTilePrograms.clear();
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
    }
    else
    {
        for (auto numberOfLayersInBatch = _maxNumberOfRasterMapLayersInBatch; numberOfLayersInBatch >= 1; numberOfLayersInBatch--)
        {
            auto& rasterLayerTileProgram = _rasterLayerTilePrograms[numberOfLayersInBatch];
            const auto success = initializeRasterLayersProgram(numberOfLayersInBatch, rasterLayerTileProgram);
            if (!success)
            {
                supportedMaxNumberOfRasterMapLayersInBatch -= 1;
                continue;
            }
        }
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

    auto variableLocations = QList< std::tuple<GlslVariableType, QString, GLint> >({
        { GlslVariableType::In, QStringLiteral("in_vs_vertexPosition"), 0 },
        { GlslVariableType::In, QStringLiteral("in_vs_vertexTexCoords"), 1 },
    });
    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    outRasterLayerTileProgram.id = 0;
    if (!outRasterLayerTileProgram.binaryCache.isEmpty())
    {
        outRasterLayerTileProgram.id = getGPUAPI()->linkProgram(0, nullptr, variableLocations,
            outRasterLayerTileProgram.binaryCache, outRasterLayerTileProgram.cacheFormat, true, &variablesMap);
    }
    if (!outRasterLayerTileProgram.id.isValid())
    {
        const auto& vertexShader = QString::fromLatin1(
            // Definitions
            "#define ELEVATION_VISUALIZATION_ENABLED %ElevationVisualizationEnabled%                                            ""\n"
            "                                                                                                                   ""\n"
            // Input data
            "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
            "INPUT vec2 in_vs_vertexTexCoords;                                                                                  ""\n"
            "                                                                                                                   ""\n"
            // Output data to next shader stages
            "%UnrolledPerRasterLayerTexCoordsDeclarationCode%                                                                   ""\n"
            "PARAM_OUTPUT float v2f_metersPerUnit;                                                                              ""\n"
            "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
            "    PARAM_OUTPUT float v2f_mipmapLOD;                                                                              ""\n"
            "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
            "#if ELEVATION_VISUALIZATION_ENABLED                                                                                ""\n"
            "    PARAM_OUTPUT lowp vec4 v2f_elevationColor;                                                                     ""\n"
            "#endif // ELEVATION_VISUALIZATION_ENABLED                                                                          ""\n"
            "PARAM_OUTPUT vec4 v2f_primaryLocation;                                                                             ""\n"
            "PARAM_OUTPUT vec4 v2f_secondaryLocation;                                                                           ""\n"
            "PARAM_OUTPUT vec4 v2f_position;                                                                                    ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform mat4 param_vs_mPerspectiveProjectionView;                                                                  ""\n"
            "uniform vec4 param_vs_resultScale;                                                                                 ""\n"
            "uniform vec2 param_vs_targetInTilePosN;                                                                            ""\n"
            "uniform float param_vs_tileSize;                                                                                   ""\n"
            "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
            "    uniform float param_vs_distanceFromCameraToTarget;                                                             ""\n"
            "    uniform float param_vs_cameraElevationAngleN;                                                                  ""\n"
            "    uniform vec2 param_vs_groundCameraPosition;                                                                    ""\n"
            "    uniform float param_vs_scaleToRetainProjectedSize;                                                             ""\n"
            "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
            "uniform vec4 param_vs_elevation_configuration;                                                                     ""\n"
            "#if ELEVATION_VISUALIZATION_ENABLED                                                                                ""\n"
            "    uniform vec4 param_vs_elevation_hillshadeConfiguration;                                                        ""\n"
            "    uniform vec4 param_vs_elevation_colorMapKeys[%MaxElevationColorMapEntriesCount%];                              ""\n"
            "    uniform vec4 param_vs_elevation_colorMapValues[%MaxElevationColorMapEntriesCount%];                            ""\n"
            "#endif // ELEVATION_VISUALIZATION_ENABLED                                                                          ""\n"
            "uniform vec4 param_vs_primaryGridAxisX;                                                                            ""\n"
            "uniform vec4 param_vs_secondaryGridAxisX;                                                                          ""\n"
            "uniform vec4 param_vs_primaryGridAxisY;                                                                            ""\n"
            "uniform vec4 param_vs_secondaryGridAxisY;                                                                          ""\n"
            "                                                                                                                   ""\n"
            // Parameters: per-tile data
            "uniform ivec4 param_vs_tileCoords31;                                                                               ""\n"
            "uniform vec4 param_vs_primaryGridTileTop;                                                                          ""\n"
            "uniform vec4 param_vs_primaryGridTileBot;                                                                          ""\n"
            "uniform vec4 param_vs_secondaryGridTileTop;                                                                        ""\n"
            "uniform vec4 param_vs_secondaryGridTileBot;                                                                        ""\n"
            "uniform vec2 param_vs_tileCoordsOffset;                                                                            ""\n"
            "uniform vec4 param_vs_elevation_scale;                                                                             ""\n"
            "uniform highp sampler2D param_vs_elevation_dataSampler;                                                            ""\n"
            "                                                                                                                   ""\n"
            // Parameters: per-layer-in-tile data
            "struct VsRasterLayerTile                                                                                           ""\n"
            "{                                                                                                                  ""\n"
            "    highp vec4 texCoordsOffsetAndScale;                                                                            ""\n"
            "};                                                                                                                 ""\n"
            "%UnrolledPerRasterLayerParamsDeclarationCode%                                                                      ""\n"
            "uniform VsRasterLayerTile param_vs_elevationLayer;                                                                 ""\n"
            "uniform highp vec4 param_vs_elevationLayerTexelSize;                                                               ""\n"
            "uniform highp vec4 param_vs_elevationLayerDataPlace;                                                               ""\n"
            "                                                                                                                   ""\n"
            "float interpolatedHeight(in vec2 inTexCoords)                                                                      ""\n"
            "{                                                                                                                  ""\n"
            "    vec2 heixelSize = param_vs_elevationLayerDataPlace.zw * 2.0;                                                   ""\n"
            "    vec2 texCoords = (inTexCoords - param_vs_elevationLayerDataPlace.zw) / heixelSize;                             ""\n"
            "    vec2 pixOffset = fract(texCoords);                                                                             ""\n"
            "    texCoords = floor(texCoords) * heixelSize + param_vs_elevationLayerDataPlace.zw;                               ""\n"
            "    vec2 minCoords = param_vs_elevationLayerDataPlace.xy - heixelSize;                                             ""\n"
            "    vec2 maxCoords = minCoords + heixelSize * (%HeixelsPerTileSide%.0 + 2.0);                                      ""\n"
            "    float blHeixel = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, clamp(texCoords, minCoords, maxCoords)).r;  ""\n"
            "    texCoords.x += heixelSize.x;                                                                                   ""\n"
            "    float brHeixel = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, clamp(texCoords, minCoords, maxCoords)).r;  ""\n"
            "    texCoords.y += heixelSize.y;                                                                                   ""\n"
            "    float trHeixel = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, clamp(texCoords, minCoords, maxCoords)).r;  ""\n"
            "    texCoords.x -= heixelSize.x;                                                                                   ""\n"
            "    float tlHeixel = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, clamp(texCoords, minCoords, maxCoords)).r;  ""\n"
            "    float avbPixel = mix(blHeixel, brHeixel, pixOffset.x);                                                         ""\n"
            "    float avtPixel = mix(tlHeixel, trHeixel, pixOffset.x);                                                         ""\n"
            "    return mix(avbPixel, avtPixel, pixOffset.y);                                                                   ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void calculateTextureCoordinates(in VsRasterLayerTile tileLayer, out vec2 outTexCoords)                            ""\n"
            "{                                                                                                                  ""\n"
            "    vec2 texCoords = in_vs_vertexTexCoords;                                                                        ""\n"
            "    texCoords = texCoords * tileLayer.texCoordsOffsetAndScale.zw + tileLayer.texCoordsOffsetAndScale.xy;           ""\n"
            "    outTexCoords = texCoords;                                                                                      ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    vec4 v = vec4(in_vs_vertexPosition.x, 0.0, in_vs_vertexPosition.y, 1.0);                                       ""\n"
            "                                                                                                                   ""\n"
            //   Scale and shift vertex to it's proper position
            "    v.xz = param_vs_tileSize * (v.xz + param_vs_tileCoordsOffset - param_vs_targetInTilePosN);                     ""\n"
            "                                                                                                                   ""\n"
            //   Get meters per unit, which is needed at both shader stages
            "    v2f_metersPerUnit = mix(param_vs_elevation_scale.x, param_vs_elevation_scale.y, in_vs_vertexTexCoords.t);      ""\n"
            "                                                                                                                   ""\n"
            //   Define needed constants
            "    const float M_PI = 3.1415926535897932384626433832795;                                                          ""\n"
            "    const float M_PI_2 = M_PI / 2.0;                                                                               ""\n"
            "    const float M_2PI = 2.0 * M_PI;                                                                                ""\n"
            "                                                                                                                   ""\n"
            //   Process each tile layer texture coordinates (except elevation)
            "%UnrolledPerRasterLayerTexCoordsProcessingCode%                                                                    ""\n"
            "                                                                                                                   ""\n"
            //   If elevation data is active, use it
            "#if ELEVATION_VISUALIZATION_ENABLED                                                                                ""\n"
            "    v2f_elevationColor = vec4(0.0, 0.0, 0.0, 0.0);                                                                 ""\n"
            "#endif // ELEVATION_VISUALIZATION_ENABLED                                                                          ""\n"
            "    if (abs(param_vs_elevation_scale.w) > 0.0)                                                                     ""\n"
            "    {                                                                                                              ""\n"
            "        float slopeAlgorithm = param_vs_elevation_configuration.x;                                                 ""\n"
            "                                                                                                                   ""\n"
            // [0][0] - TL (0); [1][0] - T (1); [2][0] - TR (2)
            // [0][1] -  L (3); [1][1] - O (4); [2][1] -  R (5)
            // [0][2] - BL (6); [1][2] - B (7); [2][2] - BR (8)
            "        mat3 heightInMeters = mat3(0.0);                                                                           ""\n"
            "        vec2 elevationTexCoordsO;                                                                                  ""\n"
            "        calculateTextureCoordinates(                                                                               ""\n"
            "            param_vs_elevationLayer,                                                                               ""\n"
            "            elevationTexCoordsO);                                                                                  ""\n"
            "        heightInMeters[1][1] = interpolatedHeight(elevationTexCoordsO);                                            ""\n"
            "                                                                                                                   ""\n"
            "#if ELEVATION_VISUALIZATION_ENABLED                                                                                ""\n"
            "        if (slopeAlgorithm > %SlopeAlgorithm_None%.0)                                                              ""\n"
            "        {                                                                                                          ""\n"
            "            vec2 elevationTexCoordsT = elevationTexCoordsO;                                                        ""\n"
            "            elevationTexCoordsT.t -= param_vs_elevationLayerTexelSize.y;                                           ""\n"
            "            heightInMeters[1][0] = interpolatedHeight(elevationTexCoordsT);                                        ""\n"
            "                                                                                                                   ""\n"
            "            vec2 elevationTexCoordsL = elevationTexCoordsO;                                                        ""\n"
            "            elevationTexCoordsL.s -= param_vs_elevationLayerTexelSize.x;                                           ""\n"
            "            heightInMeters[0][1] = interpolatedHeight(elevationTexCoordsL);                                        ""\n"
            "                                                                                                                   ""\n"
            "            vec2 elevationTexCoordsB = elevationTexCoordsO;                                                        ""\n"
            "            elevationTexCoordsB.t += param_vs_elevationLayerTexelSize.y;                                           ""\n"
            "            heightInMeters[1][2] = interpolatedHeight(elevationTexCoordsB);                                        ""\n"
            "                                                                                                                   ""\n"
            "            vec2 elevationTexCoordsR = elevationTexCoordsO;                                                        ""\n"
            "            elevationTexCoordsR.s += param_vs_elevationLayerTexelSize.x;                                           ""\n"
            "            heightInMeters[2][1] = interpolatedHeight(elevationTexCoordsR);                                        ""\n"
            "                                                                                                                   ""\n"
            "            if (slopeAlgorithm > %SlopeAlgorithm_ZevenbergenThorne%.0)                                             ""\n"
            "            {                                                                                                      ""\n"
            "                vec2 elevationTexCoordsTL = elevationTexCoordsO;                                                   ""\n"
            "                elevationTexCoordsTL.s -= param_vs_elevationLayerTexelSize.x;                                      ""\n"
            "                elevationTexCoordsTL.t -= param_vs_elevationLayerTexelSize.y;                                      ""\n"
            "                heightInMeters[0][0] = interpolatedHeight(elevationTexCoordsTL);                                   ""\n"
            "                                                                                                                   ""\n"
            "                vec2 elevationTexCoordsTR = elevationTexCoordsO;                                                   ""\n"
            "                elevationTexCoordsTR.s += param_vs_elevationLayerTexelSize.x;                                      ""\n"
            "                elevationTexCoordsTR.t -= param_vs_elevationLayerTexelSize.y;                                      ""\n"
            "                heightInMeters[2][0] = interpolatedHeight(elevationTexCoordsTR);                                   ""\n"
            "                                                                                                                   ""\n"
            "                vec2 elevationTexCoordsBL = elevationTexCoordsO;                                                   ""\n"
            "                elevationTexCoordsBL.s -= param_vs_elevationLayerTexelSize.x;                                      ""\n"
            "                elevationTexCoordsBL.t += param_vs_elevationLayerTexelSize.y;                                      ""\n"
            "                heightInMeters[0][2] = interpolatedHeight(elevationTexCoordsBL);                                   ""\n"
            "                                                                                                                   ""\n"
            "                vec2 elevationTexCoordsBR = elevationTexCoordsO;                                                   ""\n"
            "                elevationTexCoordsBR.s += param_vs_elevationLayerTexelSize.x;                                      ""\n"
            "                elevationTexCoordsBR.t += param_vs_elevationLayerTexelSize.y;                                      ""\n"
            "                heightInMeters[2][2] = interpolatedHeight(elevationTexCoordsBR);                                   ""\n"
            "            }                                                                                                      ""\n"
            "        }                                                                                                          ""\n"
            "#endif // ELEVATION_VISUALIZATION_ENABLED                                                                          ""\n"
            "        heightInMeters *= param_vs_elevation_scale.w * param_vs_elevation_scale.z;                                 ""\n"
            "                                                                                                                   ""\n"
            "#if ELEVATION_VISUALIZATION_ENABLED                                                                                ""\n"
            "        float visualizationStyle = param_vs_elevation_configuration.y;                                             ""\n"
            "        if (visualizationStyle > %VisualizationStyle_None%.0)                                                      ""\n"
            "        {                                                                                                          ""\n"
            "            const float M_3PI_2 = 3.0 * M_PI_2;                                                                    ""\n"
            "            const float M_COS_225_D = -0.70710678118;                                                              ""\n"
            "                                                                                                                   ""\n"
            "            float heixelInMeters = v2f_metersPerUnit * (param_vs_tileSize / %HeixelsPerTileSide%.0);               ""\n"
            "                                                                                                                   ""\n"
            "            vec2 slopeInMeters = vec2(0.0);                                                                        ""\n"
            "            float slopeAlgorithmScale = 1.0;                                                                       ""\n"
            "            if (abs(slopeAlgorithm - %SlopeAlgorithm_ZevenbergenThorne%.0) < 0.00001)                              ""\n"
            "            {                                                                                                      ""\n"
            "                slopeInMeters.x = heightInMeters[0][1] - heightInMeters[2][1];                                     ""\n"
            "                slopeInMeters.y = heightInMeters[1][2] - heightInMeters[1][0];                                     ""\n"
            "                                                                                                                   ""\n"
            "                slopeAlgorithmScale = 2.0;                                                                         ""\n"
            "            }                                                                                                      ""\n"
            "            else if (abs(slopeAlgorithm - %SlopeAlgorithm_Horn%.0) < 0.00001)                                      ""\n"
            "            {                                                                                                      ""\n"
            "                slopeInMeters.x = (                                                                                ""\n"
            "                    heightInMeters[0][0] + heightInMeters[0][1] + heightInMeters[0][1] + heightInMeters[0][2]      ""\n"
            "                );                                                                                                 ""\n"
            "                slopeInMeters.x -= (                                                                               ""\n"
            "                    heightInMeters[2][0] + heightInMeters[2][1] + heightInMeters[2][1] + heightInMeters[2][2]      ""\n"
            "                );                                                                                                 ""\n"
            "                                                                                                                   ""\n"
            "                slopeInMeters.y = (                                                                                ""\n"
            "                    heightInMeters[0][2] + heightInMeters[1][2] + heightInMeters[1][2] + heightInMeters[2][2]      ""\n"
            "                );                                                                                                 ""\n"
            "                slopeInMeters.y -= (                                                                               ""\n"
            "                    heightInMeters[0][0] + heightInMeters[1][0] + heightInMeters[1][0] + heightInMeters[2][0]      ""\n"
            "                );                                                                                                 ""\n"
            "                                                                                                                   ""\n"
            "                slopeAlgorithmScale = 8.0;                                                                         ""\n"
            "            }                                                                                                      ""\n"
            "            highp float slope_XX = slopeInMeters.x*slopeInMeters.x;                                                ""\n"
            "            highp float slope_YY = slopeInMeters.y*slopeInMeters.y;                                                ""\n"
            "            highp float slope_XXpYY = slope_XX + slope_YY;                                                         ""\n"
            "                                                                                                                   ""\n"
            "            float zFactor = param_vs_elevation_configuration.w;                                                    ""\n"
            "            float zFactorN = zFactor / (slopeAlgorithmScale * heixelInMeters);                                     ""\n"
            "            float zFactorNSq = zFactorN * zFactorN;                                                                ""\n"
            "            float sunZenith = param_vs_elevation_hillshadeConfiguration.x;                                         ""\n"
            "            float zNCosZenith = zFactorN * cos(sunZenith);                                                         ""\n"
            "                                                                                                                   ""\n"
            "            float colorMapKey_0 = param_vs_elevation_colorMapKeys[0].w;                                            ""\n"
            "            float colorMapKey = colorMapKey_0 - 1.0;                                                               ""\n"
            "            if (abs(visualizationStyle - %VisualizationStyle_SlopeDegrees%.0) < 0.00001)                           ""\n"
            "            {                                                                                                      ""\n"
            "                vec2 slopeN = slopeInMeters / (slopeAlgorithmScale * heixelInMeters);                              ""\n"
            "                float slopeN_XXpYY = slopeN.x*slopeN.x + slopeN.y*slopeN.y;                                        ""\n"
            "                colorMapKey = degrees(atan(zFactor * sqrt(slopeN_XXpYY)));                                         ""\n"
            "                colorMapKey = max(colorMapKey, colorMapKey_0);                                                     ""\n"
            "            }                                                                                                      ""\n"
            "            else if (abs(visualizationStyle - %VisualizationStyle_SlopePercents%.0) < 0.00001)                     ""\n"
            "            {                                                                                                      ""\n"
            "                vec2 slopeN = slopeInMeters / (slopeAlgorithmScale * heixelInMeters);                              ""\n"
            "                float slopeN_XXpYY = slopeN.x*slopeN.x + slopeN.y*slopeN.y;                                        ""\n"
            "                colorMapKey = 100.0 * zFactor * sqrt(slopeN_XXpYY);                                                ""\n"
            "                colorMapKey = max(colorMapKey, colorMapKey_0);                                                     ""\n"
            "            }                                                                                                      ""\n"
            "            else if (abs(visualizationStyle - %VisualizationStyle_HillshadeMultidirectional%.0) < 0.00001)         ""\n"
            "            {                                                                                                      ""\n"
            // NOTE: See http://pubs.usgs.gov/of/1992/of92-422/of92-422.pdf
            "                float sinZenith = sin(sunZenith);                                                                  ""\n"
            "                                                                                                                   ""\n"
            "                if (slope_XXpYY < 0.00001)                                                                         ""\n"
            "                {                                                                                                  ""\n"
            "                    float value = 1.0 + 254.0 * sinZenith;                                                         ""\n"
            "                    colorMapKey = max(value, colorMapKey_0);                                                       ""\n"
            "                }                                                                                                  ""\n"
            "                else                                                                                               ""\n"
            "                {                                                                                                  ""\n"
            "                    highp float weight_225 = 0.5 * slope_XXpYY - slopeInMeters.x * slopeInMeters.y;                ""\n"
            "                    highp float weight_270 = slope_XX;                                                             ""\n"
            "                    highp float weight_315 = slope_XXpYY - weight_225;                                             ""\n"
            "                    highp float weight_360 = slope_YY;                                                             ""\n"
            "                                                                                                                   ""\n"
            "                    float value_225 = sinZenith - (slopeInMeters.x - slopeInMeters.y) * M_COS_225_D * zNCosZenith; ""\n"
            "                    float value_270 = sinZenith - slopeInMeters.x * zNCosZenith;                                   ""\n"
            "                    float value_315 = sinZenith + (slopeInMeters.x + slopeInMeters.y) * M_COS_225_D * zNCosZenith; ""\n"
            "                    float value_360 = sinZenith - slopeInMeters.y * zNCosZenith;                                   ""\n"
            "                                                                                                                   ""\n"
            "                    float value = weight_225 * max(value_225, 0.0);                                                ""\n"
            "                    value += weight_270 * max(value_270, 0.0);                                                     ""\n"
            "                    value += weight_315 * max(value_315, 0.0);                                                     ""\n"
            "                    value += weight_360 * max(value_360, 0.0);                                                     ""\n"
            "                    value *= 127.0;                                                                                ""\n"
            "                    value /= slope_XXpYY;                                                                          ""\n"
            "                    value /= sqrt(1.0 + zFactorNSq * slope_XXpYY);                                                 ""\n"
            "                    value = 1.0 + value;                                                                           ""\n"
            "                    colorMapKey = max(value, colorMapKey_0);                                                       ""\n"
            "                }                                                                                                  ""\n"
            "            }                                                                                                      ""\n"
            "            else                                                                                                   ""\n"
            "            {                                                                                                      ""\n"
            "                float sunAzimuth = M_PI - param_vs_elevation_hillshadeConfiguration.y;                             ""\n"
            "                                                                                                                   ""\n"
            "                float zNCosZenithCosAzimuth = zNCosZenith * cos(sunAzimuth);                                       ""\n"
            "                float zNCosZenithSinAzimuth = zNCosZenith * sin(sunAzimuth);                                       ""\n"
            "                                                                                                                   ""\n"
            "                if (abs(visualizationStyle - %VisualizationStyle_HillshadeTraditional%.0) < 0.00001)               ""\n"
            "                {                                                                                                  ""\n"
            "                    float value = sin(sunZenith);                                                                  ""\n"
            "                    value -= slopeInMeters.y*zNCosZenithCosAzimuth - slopeInMeters.x*zNCosZenithSinAzimuth;        ""\n"
            "                    value = 254.0 * value / sqrt(1.0 + zFactorNSq * slope_XXpYY);                                  ""\n"
            "                    value = 1.0 + max(value, 0.0);                                                                 ""\n"
            "                    colorMapKey = max(value, colorMapKey_0);                                                       ""\n"
            "                }                                                                                                  ""\n"
            "                else if (abs(visualizationStyle - %VisualizationStyle_HillshadeIgor%.0) < 0.00001)                 ""\n"
            "                {                                                                                                  ""\n"
            "                    float slopeAngle = atan(sqrt(slope_XXpYY) * zFactorN);                                         ""\n"
            "                    float aspect = atan(slopeInMeters.y, slopeInMeters.x);                                         ""\n"
            "                    float slopeStrength = slopeAngle / M_PI_2;                                                     ""\n"
            "                    float aspectN = mod(M_2PI + aspect, M_2PI);                                                    ""\n"
            "                    float wAzimuthN = mod(M_2PI + mod(M_3PI_2 - sunAzimuth, M_2PI), M_2PI);                        ""\n"
            "                    float aspectDiffWAzimuth = mod(abs(aspectN - wAzimuthN), M_PI);                                ""\n"
            "                    float value = 255.0 * (1.0 - slopeStrength * (1.0 - aspectDiffWAzimuth / M_PI));               ""\n"
            "                    colorMapKey = max(value, colorMapKey_0);                                                       ""\n"
            "                }                                                                                                  ""\n"
            "                else if (abs(visualizationStyle - %VisualizationStyle_HillshadeCombined%.0) < 0.00001)             ""\n"
            "                {                                                                                                  ""\n"
            // NOTE: First part produces same result as HillshadeTraditional except for mapping to 1..255 scale (oblique shading)
            "                    float value = sin(sunZenith);                                                                  ""\n"
            "                    value -= slopeInMeters.y*zNCosZenithCosAzimuth - slopeInMeters.x*zNCosZenithSinAzimuth;        ""\n"
            "                    value /= sqrt(1.0 + zFactorNSq * slope_XXpYY);                                                 ""\n"
            "                                                                                                                   ""\n"
            // NOTE: Combine with slope shading
            // NOTE: Implemented differently that in GDAL, see source that https://trac.osgeo.org/gdal/ticket/4753 references
            "                    vec2 slopeN = slopeInMeters / (slopeAlgorithmScale * heixelInMeters);                          ""\n"
            "                    float slopeN_XXpYY = slopeN.x*slopeN.x + slopeN.y*slopeN.y;                                    ""\n"
            "                    value *= 1.0 - abs(atan(zFactor * sqrt(slopeN_XXpYY)) / M_PI);                                 ""\n"
            "                                                                                                                   ""\n"
            "                    value = 1.0 + 254.0 * max(value, 0.0);                                                         ""\n"
            "                    colorMapKey = max(value, colorMapKey_0);                                                       ""\n"
            "                }                                                                                                  ""\n"
            "            }                                                                                                      ""\n"
            "                                                                                                                   ""\n"
            "            if (colorMapKey >= colorMapKey_0)                                                                      ""\n"
            "            {                                                                                                      ""\n"
            "                float colorMapEntryKeyA = colorMapKey_0;                                                           ""\n"
            "                float colorMapEntryKeyB = colorMapKey;                                                             ""\n"
            "                vec4 colorMapEntryValueA = vec4(0.0);                                                              ""\n"
            "                vec4 colorMapEntryValueB = vec4(0.0);                                                              ""\n"
            "                                                                                                                   ""\n"
            "%UnrolledPerElevationColorMapEntryCode%                                                                            ""\n"
            "                                                                                                                   ""\n"
            "                float interpolation = (colorMapKey - colorMapEntryKeyA) / (colorMapEntryKeyB - colorMapEntryKeyA); ""\n"
            "                v2f_elevationColor = mix(                                                                          ""\n"
            "                    colorMapEntryValueA,                                                                           ""\n"
            "                    colorMapEntryValueB,                                                                           ""\n"
            "                    clamp(interpolation, 0.0, 1.0));                                                               ""\n"
            "            }                                                                                                      ""\n"
            "            v2f_elevationColor.a *= param_vs_elevation_configuration.z;                                            ""\n"
            "        }                                                                                                          ""\n"
            "#endif // ELEVATION_VISUALIZATION_ENABLED                                                                          ""\n"
            "        v.y = heightInMeters[1][1] / v2f_metersPerUnit;                                                            ""\n"
            "    }                                                                                                              ""\n"
            "                                                                                                                   ""\n"
            "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
            //   Calculate mipmap LOD
            "    vec2 groundVertex = v.xz;                                                                                      ""\n"
            "    vec2 groundCameraToVertex = groundVertex - param_vs_groundCameraPosition;                                      ""\n"
            "    float mipmapK = log(1.0 + 10.0 * log2(1.0 + param_vs_cameraElevationAngleN));                                  ""\n"
            "    float mipmapBaseLevelEndDistance = mipmapK * param_vs_distanceFromCameraToTarget;                              ""\n"
            "    v2f_mipmapLOD = 1.0 + (length(groundCameraToVertex) - mipmapBaseLevelEndDistance)                              ""\n"
            "        / (param_vs_scaleToRetainProjectedSize * param_vs_tileSize);                                               ""\n"
            "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
            "                                                                                                                   ""\n"
            //   Calculate location using selected grid projection
            "    vec4 primary;                                                                                                  ""\n"
            "    vec4 secondary;                                                                                                ""\n"
            "    int tilePiece = param_vs_tileCoords31.z / %HeixelsPerTileSide%;                                                ""\n"
            "    ivec2 locShift31 = ivec2(round(in_vs_vertexPosition * %HeixelsPerTileSide%.0)) * tilePiece;                    ""\n"
            "    locShift31.x -= locShift31.x - 1 + param_vs_tileCoords31.x == 2147483647 ? 1 : 0;                              ""\n"
            "    locShift31.y -= locShift31.y - 1 + param_vs_tileCoords31.y == 2147483647 ? 1 : 0;                              ""\n"
            "    vec2 mercMeters = vec2(param_vs_tileCoords31.xy + locShift31) / 65536.0 / 32768.0 - 0.5;                       ""\n"
            "    vec2 lonlat = mercMeters * M_2PI;                                                                              ""\n"
            "    lonlat.y = M_PI_2 - atan(exp(lonlat.y)) * 2.0;                                                                 ""\n"
            "    mercMeters.y = -mercMeters.y;                                                                                  ""\n"
            "    mercMeters *= M_2PI * 63.78137;                                                                                ""\n"
            "    vec2 zUTM = lonlat * 180.0 / M_PI;                                                                             ""\n"
            "    zUTM = zUTM / vec2(6.0, 8.0) + vec2(31.0, 13.0);                                                               ""\n"
            "    float refLon = (floor(zUTM.x) - 31.0) * 6.0 + 3.0;                                                             ""\n"
            "    float s = zUTM.x < 32.5 ? 31.0 : (zUTM.x < 34.5 ? 33.0 : (zUTM.x < 36.5 ? 35.0 : 37.0));                       ""\n"
            "    if (zUTM.y >= 23.5 || zUTM.y < 3.0)                                                                            ""\n"
            "        zUTM.y = (zUTM.y - 4.0) * 2.0;                                                                             ""\n"
            "    else if (zUTM.y >= 22.0 && zUTM.x >= 31.0 && zUTM.x < 38.0)                                                    ""\n"
            "    {                                                                                                              ""\n"
            "        zUTM.y = (zUTM.y - 22.0) / 1.5 + 22.0;                                                                     ""\n"
            "        zUTM.x = s + (zUTM.x - (zUTM.x < 32.5 ? s : s - 0.5)) / (zUTM.x >= 32.5 && zUTM.x < 36.5 ? 2.0 : 1.5);     ""\n"
            "        refLon = zUTM.x >= 37.0 ? 39.0 : (zUTM.x >= 35.0 ? 27.0 : (zUTM.x >= 33.0 ? 15.0 : 3.0));                  ""\n"
            "    }                                                                                                              ""\n"
            "    else if (zUTM.y >= 20.0 && zUTM.y < 21.0 && zUTM.x >= 31.0 && zUTM.x < 33.0)                                   ""\n"
            "    {                                                                                                              ""\n"
            "        zUTM.x = zUTM.x < 31.5 ? 31.0 + (zUTM.x - 31.0) / 0.5 : 32.0 + (zUTM.x - 31.5) / 1.5;                      ""\n"
            "        refLon = zUTM.x >= 32.0 ? 9.0 : 3.0;                                                                       ""\n"
            "    }                                                                                                              ""\n"
            "    zUTM.y = zUTM.x < 31.0 || zUTM.x >= 38.0 || (zUTM.y >= 3.0 && zUTM.y < 20.0) ? 0.0 : zUTM.y;                   ""\n"
            "    zUTM.y = zUTM.y >= 21.0 && zUTM.y < 22.0 ? 0.0 : zUTM.y;                                                       ""\n"
            "    zUTM.y = zUTM.y >= 20.0 && zUTM.y < 21.0 && zUTM.x >= 33.0 ? 0.0 : zUTM.y;                                     ""\n"
            "    zUTM -= vec2(param_vs_tileCoords31.w >> 4 & 63, param_vs_tileCoords31.w >> 10) + 0.5;                          ""\n"
            "    if (zUTM.x < -0.5 || zUTM.x >= 0.5 || zUTM.y < -0.5 || zUTM.y >= 0.5)                                          ""\n"
            "    {                                                                                                              ""\n"
            "        zUTM.x = zUTM.x < 0.0 ? -1e38 : 1e38;                                                                      ""\n"
            "        zUTM.y = zUTM.y < 0.0 ? -1e38 : 1e38;                                                                      ""\n"
            "    }                                                                                                              ""\n"
            "    else                                                                                                           ""\n"
            "    {                                                                                                              ""\n"
            "        zUTM = vec2(0.0);                                                                                          ""\n"
            "    }                                                                                                              ""\n"
            "    float sinlat = sin(lonlat.y);                                                                                  ""\n"
            "    float nn = 0.081819190842621486;                                                                               ""\n"
            "    float t = nn * sinlat;                                                                                         ""\n"
            "    t = 0.5 * log((1.0 + sinlat) / (1.0 - sinlat)) - nn * 0.5 * log((1.0 + t) / (1.0 - t));                        ""\n"
            "    t = (exp(t) - exp(-t)) / 2.0;                                                                                  ""\n"
            "    refLon *= M_PI / 180.0;                                                                                        ""\n"
            "    vec3 st = vec3(2.0, 4.0, 6.0);                                                                                 ""\n"
            "    float xi = atan(t / cos(lonlat.x - refLon));                                                                   ""\n"
            "    vec3 xin = st * xi;                                                                                            ""\n"
            "    float eta = sin(lonlat.x - refLon) / sqrt(1.0 + t * t);                                                        ""\n"
            "    eta = 0.5 * log((1.0 + eta) / (1.0 - eta));                                                                    ""\n"
            "    vec3 etan = st * eta;                                                                                          ""\n"
            "    vec3 expEtan = exp(etan);                                                                                      ""\n"
            "    vec3 expmEtan = exp(-etan);                                                                                    ""\n"
            "    vec3 alpha = vec3(8.3773181881925413e-4, 7.6084969586991665e-7, 1.2034877875966644e-9);                        ""\n"
            "    vec2 mUTM;                                                                                                     ""\n"
            "    mUTM.x = eta + dot(alpha * cos(xin), (expEtan - expmEtan) / 2.0);                                              ""\n"
            "    mUTM.y = xi + dot(alpha * sin(xin), (expEtan + expmEtan) / 2.0);                                               ""\n"
            "    mUTM *= 63.649021661650868;                                                                                    ""\n"
            "    mUTM += vec2(5.0, 100.0);                                                                                      ""\n"
            "    vec4 axisX = vec4(lonlat.x, mUTM.x, mercMeters.x, 1.0);                                                        ""\n"
            "    vec4 axisY = vec4(lonlat.y, mUTM.y, mercMeters.y, 1.0);                                                        ""\n"
            "    axisX.y += param_vs_primaryGridAxisX.y != 0.0 ? zUTM.x : 0.0;                                                  ""\n"
            "    axisY.y += param_vs_primaryGridAxisY.y != 0.0 ? zUTM.y : 0.0;                                                  ""\n"
            "    primary.xy = vec2(dot(axisX, param_vs_primaryGridAxisX), dot(axisY, param_vs_primaryGridAxisY));               ""\n"
            "    axisX = vec4(lonlat.x, mUTM.x, mercMeters.x, 1.0);                                                             ""\n"
            "    axisY = vec4(lonlat.y, mUTM.y, mercMeters.y, 1.0);                                                             ""\n"
            "    axisX.y += param_vs_secondaryGridAxisX.y != 0.0 ? zUTM.x : 0.0;                                                ""\n"
            "    axisY.y += param_vs_secondaryGridAxisY.y != 0.0 ? zUTM.y : 0.0;                                                ""\n"
            "    secondary.xy = vec2(dot(axisX, param_vs_secondaryGridAxisX), dot(axisY, param_vs_secondaryGridAxisY));         ""\n"
            "    vec2 intp;                                                                                                     ""\n"
            "    intp.x = mix(param_vs_primaryGridTileTop.x, param_vs_primaryGridTileBot.x, in_vs_vertexPosition.y);            ""\n"
            "    intp.y = mix(param_vs_primaryGridTileTop.z, param_vs_primaryGridTileBot.z, in_vs_vertexPosition.y);            ""\n"
            "    primary.z = mix(intp.x, intp.y, in_vs_vertexPosition.x);                                                       ""\n"
            "    intp.x = mix(param_vs_primaryGridTileTop.y, param_vs_primaryGridTileTop.w, in_vs_vertexPosition.x);            ""\n"
            "    intp.y = mix(param_vs_primaryGridTileBot.y, param_vs_primaryGridTileBot.w, in_vs_vertexPosition.x);            ""\n"
            "    primary.w = mix(intp.x, intp.y, in_vs_vertexPosition.y);                                                       ""\n"
            "    intp.x = mix(param_vs_secondaryGridTileTop.x, param_vs_secondaryGridTileBot.x, in_vs_vertexPosition.y);        ""\n"
            "    intp.y = mix(param_vs_secondaryGridTileTop.z, param_vs_secondaryGridTileBot.z, in_vs_vertexPosition.y);        ""\n"
            "    secondary.z = mix(intp.x, intp.y, in_vs_vertexPosition.x);                                                     ""\n"
            "    intp.x = mix(param_vs_secondaryGridTileTop.y, param_vs_secondaryGridTileTop.w, in_vs_vertexPosition.x);        ""\n"
            "    intp.y = mix(param_vs_secondaryGridTileBot.y, param_vs_secondaryGridTileBot.w, in_vs_vertexPosition.x);        ""\n"
            "    secondary.w = mix(intp.x, intp.y, in_vs_vertexPosition.y);                                                     ""\n"
            "    primary.zw += param_vs_primaryGridAxisX.y > 0.0 ? zUTM : vec2(0.0);                                            ""\n"
            "    secondary.zw += param_vs_secondaryGridAxisX.y > 0.0 ? zUTM : vec2(0.0);                                        ""\n"
            "    if ((param_vs_tileCoords31.w & 3) == 0)                                                                        ""\n"
            "        primary.xy = vec2(1e38);                                                                                   ""\n"
            "    else if ((param_vs_tileCoords31.w & 3) == 1)                                                                   ""\n"
            "        primary.zw = primary.xy;                                                                                   ""\n"
            "    else if ((param_vs_tileCoords31.w & 3) == 3)                                                                   ""\n"
            "        primary.xy = primary.zw;                                                                                   ""\n"
            "    if ((param_vs_tileCoords31.w & 12) == 0)                                                                       ""\n"
            "        secondary.xy = vec2(1e38);                                                                                 ""\n"
            "    else if ((param_vs_tileCoords31.w & 12) == 4)                                                                  ""\n"
            "        secondary.zw = secondary.xy;                                                                               ""\n"
            "    else if ((param_vs_tileCoords31.w & 12) == 12)                                                                 ""\n"
            "        secondary.xy = secondary.zw;                                                                               ""\n"
            "                                                                                                                   ""\n"
            //   Finally output processed modified vertex
            "    v2f_primaryLocation = primary;                                                                                 ""\n"
            "    v2f_secondaryLocation = secondary;                                                                             ""\n"
            "    v2f_position = v;                                                                                              ""\n"
            "    v = param_vs_mPerspectiveProjectionView * v;                                                                   ""\n"
            "    gl_Position = v * param_vs_resultScale;                                                                        ""\n"
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
        const auto& vertexShader_perElevationColorMapEntryCode = QString::fromLatin1(
            "                float colorMapKey_%entryIndex% = param_vs_elevation_colorMapKeys[%entryIndex%].w;                  ""\n"
            "                if (colorMapKey_%entryIndex% > colorMapKey_%prevEntryIndex%)                                       ""\n"
            "                {                                                                                                  ""\n"
            "                    if (colorMapKey >= colorMapKey_%prevEntryIndex%)                                               ""\n"
            "                    {                                                                                              ""\n"
            "                        colorMapEntryKeyA = colorMapKey_%prevEntryIndex%;                                          ""\n"
            "                        colorMapEntryValueA = param_vs_elevation_colorMapValues[%prevEntryIndex%];                 ""\n"
            "                        colorMapEntryKeyB = colorMapKey_%entryIndex%;                                              ""\n"
            "                        colorMapEntryValueB = param_vs_elevation_colorMapValues[%entryIndex%];                     ""\n"
            "                    }                                                                                              ""\n"
            "                }                                                                                                  ""\n"
            "                                                                                                                   ""\n");

        const auto& fragmentShader = QString::fromLatin1(
            // Definitions
            "#define ELEVATION_VISUALIZATION_ENABLED %ElevationVisualizationEnabled%                                            ""\n"
            "                                                                                                                   ""\n"
            // Input data
            "%UnrolledPerRasterLayerTexCoordsDeclarationCode%                                                                   ""\n"
            "PARAM_INPUT float v2f_metersPerUnit;                                                                               ""\n"
            "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
            "    PARAM_INPUT float v2f_mipmapLOD;                                                                               ""\n"
            "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
            "#if ELEVATION_VISUALIZATION_ENABLED                                                                                ""\n"
            "    PARAM_INPUT lowp vec4 v2f_elevationColor;                                                                      ""\n"
            "#endif // ELEVATION_VISUALIZATION_ENABLED                                                                          ""\n"
            "PARAM_INPUT vec4 v2f_primaryLocation;                                                                              ""\n"
            "PARAM_INPUT vec4 v2f_secondaryLocation;                                                                            ""\n"

            "PARAM_INPUT vec4 v2f_position;                                                                                     ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform lowp float param_fs_lastBatch;                                                                             ""\n"
            "uniform lowp float param_fs_blendingEnabled;                                                                       ""\n"
            "uniform lowp vec4 param_fs_backgroundColor;                                                                        ""\n"
            "uniform lowp vec4 param_fs_myLocationColor;                                                                        ""\n"
            "uniform vec4 param_fs_myLocation;                                                                                  ""\n"
            "uniform vec2 param_fs_myDirection;                                                                                 ""\n"
            "uniform vec4 param_fs_gridParameters;                                                                              ""\n"
            "uniform lowp vec4 param_fs_primaryGridColor;                                                                       ""\n"
            "uniform lowp vec4 param_fs_secondaryGridColor;                                                                     ""\n"
            "uniform vec4 param_fs_worldCameraPosition;                                                                         ""\n"
            "uniform vec4 param_fs_mistConfiguration;                                                                           ""\n"
            "uniform vec4 param_fs_mistColor;                                                                                   ""\n"
            // Parameters: per-layer data
            "struct FsRasterLayerTile                                                                                           ""\n"
            "{                                                                                                                  ""\n"
            "    lowp float isPremultipliedAlpha;                                                                               ""\n"
            "    lowp float opacityFactor;                                                                                      ""\n"
            "    highp vec4 texCoordsOffsetAndScale;                                                                            ""\n"
            "    highp vec4 transitionPhase;                                                                                    ""\n"
            "    highp float texelSize;                                                                                         ""\n"
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
            "    destColor = destColor * (1.0 - srcColor.a) + srcColor * vec4(srcColor.a, srcColor.a, srcColor.a, 1.0);         ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void mixColors(inout lowp vec4 destColor, in lowp vec4 srcColor, in lowp float isPremultipliedAlpha)               ""\n"
            "{                                                                                                                  ""\n"
            "    lowp float srcColorMultiplier =                                                                                ""\n"
            "        isPremultipliedAlpha + (1.0 - isPremultipliedAlpha) * srcColor.a;                                          ""\n"
            "    destColor *= 1.0 - srcColor.a;                                                                                 ""\n"
            "    destColor += srcColor * vec4(srcColorMultiplier, srcColorMultiplier,  srcColorMultiplier, 1.0);                ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void fromTexture(in lowp sampler2D sampler, in vec2 coords, out lowp vec4 destColor)                               ""\n"
            "{                                                                                                                  ""\n"
            "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
            "    destColor = SAMPLE_TEXTURE_2D_LOD(sampler, coords, v2f_mipmapLOD);                                             ""\n"
            "#else // !TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
            "    destColor = SAMPLE_TEXTURE_2D(sampler, coords);                                                                ""\n"
            "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void getWind(in lowp sampler2D sampler, in vec2 coords, out vec2 windVector)                                       ""\n"
            "{                                                                                                                  ""\n"
            "    lowp vec4 windColor;                                                                                           ""\n"
            "    fromTexture(sampler, coords, windColor);                                                                       ""\n"
            "    vec2 result = (windColor.rb + windColor.ga * 255.0 - 8.0) * 10.0;                                              ""\n"
            "    windVector = vec2(result.x, -result.y);                                                                        ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "float getRandPixel(in vec2 pixCoords, in vec2 tileSize, in float normSeed)                                         ""\n"
            "{                                                                                                                  ""\n"
            "    vec2 intCoords = floor(pixCoords);                                                                             ""\n"
            "    float randValue = abs(fract(sin(dot(intCoords / tileSize + normSeed, vec2(12.9898, 78.233))) * 43758.5453));   ""\n"
            "    return randValue > 0.98 ? 1.0 : 0.0;                                                                           ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void getParticleColor(in vec2 pixCoords, in vec2 windVector, in vec2 tileSize, in float seed, out lowp vec4 color) ""\n"
            "{                                                                                                                  ""\n"
            "    float normSeed = seed * 0.001;                                                                                 ""\n"
            "    float windMagnitude = length(windVector);                                                                      ""\n"
            "    vec2 shift = windMagnitude >= 0.5 ? windVector / windMagnitude : vec2(0.0);                                    ""\n"
            //   Rescale two times: to get out of tile data without overlap (- 0.1 / 0.8) and to access full texture (/ 0.5)
            "    vec2 coords = (pixCoords - 0.05) * tileSize / 0.4;                                                             ""\n"
            "    vec2 center = coords + shift;                                                                                  ""\n"
            "    float nextPixel = 0.5 * getRandPixel(center, tileSize, normSeed);                                              ""\n"
            "    float prevPixel = getRandPixel(coords - shift, tileSize, normSeed);                                            ""\n"
            "    center = prevPixel > 0.0 ? coords - shift : center;                                                            ""\n"
            "    float actPixel = 0.8 * getRandPixel(coords, tileSize, normSeed);                                               ""\n"
            "    center = actPixel > 0.0 ? coords : center;                                                                     ""\n"
            "    float alpha = max(max(actPixel, nextPixel), prevPixel);                                                        ""\n"
            "    vec2 norm = vec2(shift.y, -shift.x);                                                                           ""\n"
            "    alpha *= clamp(1.0 - 2.0 * abs(dot(norm, coords) - dot(norm, floor(center) + 0.5)), 0.0, 1.0);                 ""\n"
            "    color = vec4(1.0, 1.0, 1.0, alpha);                                                                            ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void getTextureColor(in vec4 texCoordsOffsetAndScale, in vec4 transitionPhase, in float texelSize,                 ""\n"
            "        in sampler2D sampler, in vec2 texCoords, out lowp vec4 resultColor)                                        ""\n"
            "{                                                                                                                  ""\n"
            "    if (transitionPhase.x >= 0.0)                                                                                  ""\n"
            "    {                                                                                                              ""\n"
            //       Animate transition between textures
            "        vec2 leftCoords = texCoords;                                                                               ""\n"
            "        vec2 rightCoords;                                                                                          ""\n"
            "        lowp vec4 windColor = vec4(0.0);                                                                           ""\n"
            "        float halfTexelSize = texelSize / 2.0;                                                                     ""\n"
            //       Scale two times: to get into tile data without overlap (* 0.8 + 0.1) and to access quadrants (* 0.5)
            "        leftCoords = leftCoords * 0.4 + 0.05;                                                                      ""\n"
            "        leftCoords.x = clamp(leftCoords.x, halfTexelSize, 0.5 - halfTexelSize);                                    ""\n"
            "        leftCoords.y = clamp(leftCoords.y, halfTexelSize, 0.5 - halfTexelSize);                                    ""\n"
            "        rightCoords = leftCoords + 0.5;                                                                            ""\n"
            "        vec2 rightWind;                                                                                            ""\n"
            "        getWind(sampler, rightCoords, rightWind);                                                                  ""\n"
            //       Scale two times (* 0.8 and * 0.5) and then divide to TileSize3D (/ 100)
            "        float factor = transitionPhase.y / (v2f_metersPerUnit * 250.0);                                            ""\n"
            "        vec2 texShift = rightWind * texCoordsOffsetAndScale.zw * factor;                                           ""\n"
            "        if (transitionPhase.z >= 0.0)                                                                              ""\n"
            "        {                                                                                                          ""\n"
            //           Animate wind particles
            "            float seed = floor(transitionPhase.z) / 10.0;                                                          ""\n"
            "            float shift = floor(texCoordsOffsetAndScale.y * 10.0) / 100.0;                                         ""\n"
            "            shift += floor(texCoordsOffsetAndScale.x * 10.0) / 1000.0;                                             ""\n"
            "            float phase = fract(transitionPhase.z);                                                                ""\n"
            "            vec2 tileSize = transitionPhase.w / texCoordsOffsetAndScale.zw;                                        ""\n"
            "            rightCoords.x -= 0.5;                                                                                  ""\n"
            "            vec2 leftWind;                                                                                         ""\n"
            "            getWind(sampler, rightCoords, leftWind);                                                               ""\n"
            "            vec2 windVector = mix(leftWind, rightWind, transitionPhase.x);                                         ""\n"
            "            vec2 windShift = windVector * texCoordsOffsetAndScale.zw * factor;                                     ""\n"
            "            vec2 leftShift = windShift * phase;                                                                    ""\n"
            "            vec2 rightShift = windShift * (1.0 - phase);                                                           ""\n"
            "            lowp vec4 tmpColor;                                                                                    ""\n"
            "            lowp vec4 leftColor;                                                                                   ""\n"
            "            lowp vec4 rightColor;                                                                                  ""\n"
            "            getParticleColor(leftCoords - leftShift, windVector, tileSize, fract(seed) + shift, leftColor);        ""\n"
            "            getParticleColor(leftCoords - leftShift, windVector, tileSize, fract(seed + 0.1) + shift, tmpColor);   ""\n"
            "            leftColor = mix(tmpColor, leftColor, clamp(phase + 0.5, 0.0, 1.0));                                    ""\n"
            "            getParticleColor(leftCoords + rightShift, windVector, tileSize, fract(seed + 0.1) + shift, rightColor);""\n"
            "            getParticleColor(leftCoords + rightShift, windVector, tileSize, fract(seed + 0.2) + shift, tmpColor);  ""\n"
            "            rightColor = mix(rightColor, tmpColor, clamp(phase - 0.5, 0.0, 1.0));                                  ""\n"
            "            windColor = mix(leftColor, rightColor, phase);                                                         ""\n"
            "        }                                                                                                          ""\n"
            "        rightCoords = leftCoords;                                                                                  ""\n"
            "        rightCoords.x += 0.5;                                                                                      ""\n"
            "        leftCoords -= texShift * transitionPhase.x;                                                                ""\n"
            "        leftCoords.x = clamp(leftCoords.x, halfTexelSize, 0.5 - halfTexelSize);                                    ""\n"
            "        leftCoords.y = clamp(leftCoords.y, halfTexelSize, 0.5 - halfTexelSize);                                    ""\n"
            "        rightCoords += texShift * (1.0 - transitionPhase.x);                                                       ""\n"
            "        rightCoords.x = clamp(rightCoords.x, 0.5 + halfTexelSize, 1.0 - halfTexelSize);                            ""\n"
            "        rightCoords.y = clamp(rightCoords.y, halfTexelSize, 0.5 - halfTexelSize);                                  ""\n"
            "        lowp vec4 texColorLeft;                                                                                    ""\n"
            "        lowp vec4 texColorRight;                                                                                   ""\n"
            "        fromTexture(sampler, leftCoords, texColorLeft);                                                            ""\n"
            "        fromTexture(sampler, rightCoords, texColorRight);                                                          ""\n"
            "        lowp vec4 texColor = mix(texColorLeft, texColorRight, clamp(transitionPhase.x, 0.0, 1.0));                 ""\n"
            "        resultColor = vec4(mix(texColor.rgb, windColor.rgb, windColor.a), texColor.a);                             ""\n"
            "    }                                                                                                              ""\n"
            "    else                                                                                                           ""\n"
            "    {                                                                                                              ""\n"
            "        fromTexture(sampler, texCoords, resultColor);                                                              ""\n"
            "    }                                                                                                              ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    lowp vec4 finalColor;                                                                                          ""\n"
            "                                                                                                                   ""\n"
            //   Calculate color of accuracy circle
            "    lowp vec4 circle = param_fs_myLocationColor;                                                                   ""\n"
            "    vec2 vMyToPos = v2f_position.xz - param_fs_myLocation.xy;                                                      ""\n"
            "    float dist = length(vMyToPos);                                                                                 ""\n"
            "    float fdist = min(pow(min(dist / param_fs_myLocation.z, 1.0), 100.0), 1.0);                                    ""\n"
            "    circle.a = (1.0 - fdist) * (1.0 + 2.0 * fdist) * circle.a;                                                           ""\n"
            "                                                                                                                   ""\n"
            //   Calculate color of heading sector
            "    lowp vec4 sector = param_fs_myLocationColor;                                                                   ""\n"
            "    float fdir = dot(vec2(sin(param_fs_myDirection.x), -cos(param_fs_myDirection.x)), vMyToPos / dist);            ""\n"
            "    fdir = pow(min(max(fdir / 0.7071, 0.0), 1.0), 10.0);                                                           ""\n"
            "    sector.a = dist >= param_fs_myDirection.y ? 0.0 : fdir * (1.0 - dist / param_fs_myDirection.y);                ""\n"
            "                                                                                                                   ""\n"
            //   Calculate colors of grids
            "    lowp vec4 primaryColor = param_fs_primaryGridColor;                                                            ""\n"
            "    primaryColor *= v2f_primaryLocation.x > -1e30 && v2f_primaryLocation.x < 1e30 ? 1.0 : 0.0;                     ""\n"
            "    primaryColor *= v2f_primaryLocation.y > -1e30 && v2f_primaryLocation.y < 1e30 ? 1.0 : 0.0;                     ""\n"
            "    vec2 c = fract(v2f_primaryLocation.xy / param_fs_gridParameters.x);                                            ""\n"
            "    vec2 d = fract(v2f_primaryLocation.zw / param_fs_gridParameters.x);                                            ""\n"
            "    d = vec2(abs(c.x - d.x) < 0.1 ? (c.x + d.x) / 2.0 : c.x, abs(c.y - d.y) < 0.1 ? (c.y + d.y) / 2.0 : c.y);      ""\n"
            "    vec2 limit;                                                                                                    ""\n"
            "    limit.x = length(vec2(dFdx(v2f_primaryLocation.z), dFdy(v2f_primaryLocation.z)));                              ""\n"
            "    limit.y = length(vec2(dFdx(v2f_primaryLocation.w), dFdy(v2f_primaryLocation.w)));                              ""\n"
            "    primaryColor *= limit.x > -1e30 && limit.x < 1e30 ? 1.0 : 0.0;                                                 ""\n"
            "    primaryColor *= limit.y > -1e30 && limit.y < 1e30 ? 1.0 : 0.0;                                                 ""\n"
            "    limit /= param_fs_gridParameters.x;                                                                            ""\n"
            "    vec2 n = limit * param_fs_gridParameters.y;                                                                    ""\n"
            "    vec2 f = limit * (param_fs_gridParameters.y + 1.0);                                                            ""\n"
            "    lowp vec4 halfColor = primaryColor;                                                                            ""\n"
            "    lowp float halfAlfa = halfColor.a / 2.0;                                                                       ""\n"
            "    halfColor.a = halfAlfa;                                                                                        ""\n"
            "    primaryColor = d.x > f.x && d.x < 1.0 - f.x && d.y > f.y && d.y < 1.0 - f.y ? vec4(0.0) : halfColor;           ""\n"
            "    primaryColor.a += d.x > n.x && d.x < 1.0 - n.x && d.y > n.y && d.y < 1.0 - n.y ? 0.0 : halfAlfa;               ""\n"
            "    lowp vec4 secondaryColor = param_fs_secondaryGridColor;                                                        ""\n"
            "    secondaryColor *= v2f_secondaryLocation.x > -1e30 && v2f_secondaryLocation.x < 1e30 ? 1.0 : 0.0;               ""\n"
            "    secondaryColor *= v2f_secondaryLocation.y > -1e30 && v2f_secondaryLocation.y < 1e30 ? 1.0 : 0.0;               ""\n"
            "    c = fract(v2f_secondaryLocation.xy / param_fs_gridParameters.z);                                               ""\n"
            "    d = fract(v2f_secondaryLocation.zw / param_fs_gridParameters.z);                                               ""\n"
            "    d = vec2(abs(c.x - d.x) < 0.1 ? (c.x + d.x) / 2.0 : c.x, abs(c.y - d.y) < 0.1 ? (c.y + d.y) / 2.0 : c.y);      ""\n"
            "    limit.x = length(vec2(dFdx(v2f_secondaryLocation.z), dFdy(v2f_secondaryLocation.z)));                          ""\n"
            "    limit.y = length(vec2(dFdx(v2f_secondaryLocation.w), dFdy(v2f_secondaryLocation.w)));                          ""\n"
            "    secondaryColor *= limit.x > -1e30 && limit.x < 1e30 ? 1.0 : 0.0;                                               ""\n"
            "    secondaryColor *= limit.y > -1e30 && limit.y < 1e30 ? 1.0 : 0.0;                                               ""\n"
            "    limit /= param_fs_gridParameters.z;                                                                            ""\n"
            "    n = limit * param_fs_gridParameters.w;                                                                         ""\n"
            "    f = limit * (param_fs_gridParameters.w + 1.0);                                                                 ""\n"
            "    halfColor = secondaryColor;                                                                                    ""\n"
            "    halfAlfa = halfColor.a / 2.0;                                                                                  ""\n"
            "    halfColor.a = halfAlfa;                                                                                        ""\n"
            "    secondaryColor = d.x > f.x && d.x < 1.0 - f.x && d.y > f.y && d.y < 1.0 - f.y ? vec4(0.0) : halfColor;         ""\n"
            "    secondaryColor.a += d.x > n.x && d.x < 1.0 - n.x && d.y > n.y && d.y < 1.0 - n.y ? 0.0 : halfAlfa;             ""\n"
            "    secondaryColor.a = primaryColor.a < param_fs_primaryGridColor.a ? secondaryColor.a : 0.0;                      ""\n"
            "                                                                                                                   ""\n"
            //   Calculate mist color
            "    lowp vec4 mistColor = param_fs_mistColor;                                                                      ""\n"
            "    vec4 infrontPosition = v2f_position;                                                                           ""\n"
            "    infrontPosition.xz = v2f_position.xz * param_fs_mistConfiguration.xy;                                          ""\n"
            "    infrontPosition.xz = v2f_position.xz - (infrontPosition.x + infrontPosition.z) * param_fs_mistConfiguration.xy;""\n"
            "    float toFog = param_fs_mistConfiguration.z - distance(infrontPosition, param_fs_worldCameraPosition);          ""\n"
            "    float expScale = (3.0 - param_fs_mistColor.w ) / param_fs_mistConfiguration.w;                                 ""\n"
            "    float expOffset = 2.354 - param_fs_mistColor.w * 0.5;                                                          ""\n"
            "    mistColor.a = clamp(1.0 - 1.0 / exp(pow(max(0.0, expOffset - toFog * expScale), 2.0)), 0.0, 1.0);              ""\n"
            "                                                                                                                   ""\n"
            //   Mix colors of all layers.
            //   First layer is processed unconditionally, as well as its color is converted to premultiplied alpha.
            "    getTextureColor(param_fs_rasterTileLayer_0.texCoordsOffsetAndScale, param_fs_rasterTileLayer_0.transitionPhase,""\n"
            "        param_fs_rasterTileLayer_0.texelSize, param_fs_rasterTileLayer_0.sampler,                                  ""\n"
            "        v2f_texCoordsPerLayer_0, finalColor);                                                                      ""\n"
            "    addExtraAlpha(finalColor, param_fs_rasterTileLayer_0.opacityFactor,                                            ""\n"
            "        param_fs_rasterTileLayer_0.isPremultipliedAlpha);                                                          ""\n"
            "    lowp float firstLayerColorFactor = param_fs_rasterTileLayer_0.isPremultipliedAlpha +                           ""\n"
            "        (1.0 - param_fs_rasterTileLayer_0.isPremultipliedAlpha) * finalColor.a;                                    ""\n"
            "    finalColor *= vec4(firstLayerColorFactor, firstLayerColorFactor, firstLayerColorFactor, 1.0);                  ""\n"
            "                                                                                                                   ""\n"
            "%UnrolledPerRasterLayerProcessingCode%                                                                             ""\n"
            "                                                                                                                   ""\n"
            "#if ELEVATION_VISUALIZATION_ENABLED                                                                                ""\n"
            "    mixColors(finalColor, v2f_elevationColor * param_fs_lastBatch);                                                ""\n"
            "#endif // ELEVATION_VISUALIZATION_ENABLED                                                                          ""\n"
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
            "    mixColors(finalColor, secondaryColor * param_fs_lastBatch);                                                    ""\n"
            "    mixColors(finalColor, primaryColor * param_fs_lastBatch);                                                      ""\n"
            "    mixColors(finalColor, circle * param_fs_lastBatch);                                                            ""\n"
            "    mixColors(finalColor, sector * param_fs_lastBatch);                                                            ""\n"
            "    mixColors(finalColor, mistColor * param_fs_lastBatch);                                                         ""\n"
            "    lowp vec4 overColor = mix(param_fs_backgroundColor, finalColor, finalColor.a);                                 ""\n"
            "    FRAGMENT_COLOR_OUTPUT = mix(overColor, finalColor, param_fs_blendingEnabled);                                  ""\n"
            "}                                                                                                                  ""\n");
        const auto& fragmentShader_perRasterLayer = QString::fromLatin1(
            "    {                                                                                                              ""\n"
            "        lowp vec4 tc;                                                                                              ""\n"
            "        getTextureColor(param_fs_rasterTileLayer_%rasterLayerIndex%.texCoordsOffsetAndScale,                       ""\n"
            "            param_fs_rasterTileLayer_%rasterLayerIndex%.transitionPhase,                                           ""\n"
            "            param_fs_rasterTileLayer_%rasterLayerIndex%.texelSize,                                                 ""\n"
            "            param_fs_rasterTileLayer_%rasterLayerIndex%.sampler, v2f_texCoordsPerLayer_%rasterLayerIndex%, tc);    ""\n"
            "        addExtraAlpha(tc, param_fs_rasterTileLayer_%rasterLayerIndex%.opacityFactor,                               ""\n"
            "            param_fs_rasterTileLayer_%rasterLayerIndex%.isPremultipliedAlpha);                                     ""\n"
            "        mixColors(finalColor, tc, param_fs_rasterTileLayer_%rasterLayerIndex%.isPremultipliedAlpha);               ""\n"
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
        QString preprocessedVertexShader_UnrolledPerElevationColorMapEntryCode;
        for (auto colorMapEntryIndex = 1u; colorMapEntryIndex < ElevationConfiguration::MaxColorMapEntries; colorMapEntryIndex++)
        {
            preprocessedVertexShader_UnrolledPerElevationColorMapEntryCode +=
                detachedOf(vertexShader_perElevationColorMapEntryCode)
                .replace("%entryIndex%", QString::number(colorMapEntryIndex))
                .replace("%prevEntryIndex%", QString::number(colorMapEntryIndex - 1));
        }
        preprocessedVertexShader.replace("%UnrolledPerElevationColorMapEntryCode%",
            preprocessedVertexShader_UnrolledPerElevationColorMapEntryCode);
        preprocessedVertexShader.replace("%HeixelsPerTileSide%",
            QString::number(AtlasMapRenderer::HeixelsPerTileSide - 1));
        preprocessedVertexShader.replace("%MaxElevationColorMapEntriesCount%",
            QString::number(ElevationConfiguration::MaxColorMapEntries));
        preprocessedVertexShader.replace("%SlopeAlgorithm_None%",
            QString::number(static_cast<int>(ElevationConfiguration::SlopeAlgorithm::None)));
        preprocessedVertexShader.replace("%SlopeAlgorithm_ZevenbergenThorne%",
            QString::number(static_cast<int>(ElevationConfiguration::SlopeAlgorithm::ZevenbergenThorne)));
        preprocessedVertexShader.replace("%SlopeAlgorithm_Horn%",
            QString::number(static_cast<int>(ElevationConfiguration::SlopeAlgorithm::Horn)));
        preprocessedVertexShader.replace("%VisualizationStyle_None%",
            QString::number(static_cast<int>(ElevationConfiguration::VisualizationStyle::None)));
        preprocessedVertexShader.replace("%VisualizationStyle_SlopeDegrees%",
            QString::number(static_cast<int>(ElevationConfiguration::VisualizationStyle::SlopeDegrees)));
        preprocessedVertexShader.replace("%VisualizationStyle_SlopePercents%",
            QString::number(static_cast<int>(ElevationConfiguration::VisualizationStyle::SlopePercents)));
        preprocessedVertexShader.replace("%VisualizationStyle_HillshadeTraditional%",
            QString::number(static_cast<int>(ElevationConfiguration::VisualizationStyle::HillshadeTraditional)));
        preprocessedVertexShader.replace("%VisualizationStyle_HillshadeIgor%",
            QString::number(static_cast<int>(ElevationConfiguration::VisualizationStyle::HillshadeIgor)));
        preprocessedVertexShader.replace("%VisualizationStyle_HillshadeCombined%",
            QString::number(static_cast<int>(ElevationConfiguration::VisualizationStyle::HillshadeCombined)));
        preprocessedVertexShader.replace("%VisualizationStyle_HillshadeMultidirectional%",
            QString::number(static_cast<int>(ElevationConfiguration::VisualizationStyle::HillshadeMultidirectional)));
        preprocessedVertexShader.replace("%ElevationVisualizationEnabled%",
            QString::number(setupOptions.elevationVisualizationEnabled ? 1 : 0));
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
        preprocessedFragmentShader.replace("%ElevationVisualizationEnabled%",
            QString::number(setupOptions.elevationVisualizationEnabled ? 1 : 0));
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
        GLuint shaders[] = { vsId, fsId };
        outRasterLayerTileProgram.id = getGPUAPI()->linkProgram(2, shaders, variableLocations,
            outRasterLayerTileProgram.binaryCache, outRasterLayerTileProgram.cacheFormat, true, &variablesMap);
    }
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
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.mPerspectiveProjectionView,
        "param_vs_mPerspectiveProjectionView",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.resultScale,
        "param_vs_resultScale",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.targetInTilePosN,
        "param_vs_targetInTilePosN",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.tileSize,
        "param_vs_tileSize",
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
        outRasterLayerTileProgram.vs.param.elevation_hillshadeConfiguration,
        "param_vs_elevation_hillshadeConfiguration",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.elevation_colorMapKeys,
        "param_vs_elevation_colorMapKeys",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.elevation_colorMapValues,
        "param_vs_elevation_colorMapValues",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.tileCoords31,
        "param_vs_tileCoords31",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.primaryGridTileTop,
        "param_vs_primaryGridTileTop",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.primaryGridTileBot,
        "param_vs_primaryGridTileBot",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.secondaryGridTileTop,
        "param_vs_secondaryGridTileTop",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.secondaryGridTileBot,
        "param_vs_secondaryGridTileBot",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.tileCoordsOffset,
        "param_vs_tileCoordsOffset",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.elevation_scale,
        "param_vs_elevation_scale",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.elevation_dataSampler,
        "param_vs_elevation_dataSampler",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.elevationLayer.texCoordsOffsetAndScale,
        "param_vs_elevationLayer.texCoordsOffsetAndScale",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.elevationLayerTexelSize,
        "param_vs_elevationLayerTexelSize",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.elevationLayerDataPlace,
        "param_vs_elevationLayerDataPlace",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.primaryGridAxisX,
        "param_vs_primaryGridAxisX",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.secondaryGridAxisX,
        "param_vs_secondaryGridAxisX",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.primaryGridAxisY,
        "param_vs_primaryGridAxisY",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.secondaryGridAxisY,
        "param_vs_secondaryGridAxisY",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.fs.param.lastBatch,
        "param_fs_lastBatch",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.fs.param.blendingEnabled,
        "param_fs_blendingEnabled",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.fs.param.backgroundColor,
        "param_fs_backgroundColor",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.fs.param.myLocationColor,
        "param_fs_myLocationColor",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.fs.param.myLocation,
        "param_fs_myLocation",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.fs.param.myDirection,
        "param_fs_myDirection",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.fs.param.gridParameters,
        "param_fs_gridParameters",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.fs.param.primaryGridColor,
        "param_fs_primaryGridColor",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.fs.param.secondaryGridColor,
        "param_fs_secondaryGridColor",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.fs.param.worldCameraPosition,
        "param_fs_worldCameraPosition",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.fs.param.mistConfiguration,
        "param_fs_mistConfiguration",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.fs.param.mistColor,
        "param_fs_mistColor",
        GlslVariableType::Uniform);        
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
                layerStruct.texCoordsOffsetAndScale,
                layerStructName + ".texCoordsOffsetAndScale",
                GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(
                layerStruct.transitionPhase,
                layerStructName + ".transitionPhase",
                GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(
                layerStruct.texelSize,
                layerStructName + ".texelSize",
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
    GLname& lastUsedProgram,
    bool& haveElevation,
    const bool withElevation,
    const bool blendingEnabled,
    const ZoomLevel zoomLevel)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform2i);
    GL_CHECK_PRESENT(glUniform2fv);
    GL_CHECK_PRESENT(glUniform4i);
    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);
    GL_CHECK_PRESENT(glDisableVertexAttribArray);

    const auto& currentConfiguration = getCurrentConfiguration();
    const auto& internalState = getInternalState();

    const auto batchedLayersCount = batch->layers.size();
    const auto elevationDataSamplerIndex = batchedLayersCount;
    auto subtilesPerTile = batch->layers.first()->resourcesInGPU.size();

    GL_PUSH_GROUP_MARKER(QStringLiteral("%1x%2@%3").arg(batch->tileId.x).arg(batch->tileId.y).arg(zoomLevel));

    // Activate proper program depending on number of captured layers
    const auto wasActivated = activateRasterLayersProgram(
        batchedLayersCount,
        elevationDataSamplerIndex,
        lastUsedProgram,
        zoomLevel);
    const auto& program = _rasterLayerTilePrograms[batchedLayersCount];
    const auto& vao = _rasterTileVAOs[batchedLayersCount];

    // Set tile coordinates
    const auto tileIdN = Utilities::normalizeTileId(batch->tileId, zoomLevel);
    const auto zoomShift = MaxZoomLevel - zoomLevel;
    PointI tile31(tileIdN.x << zoomShift, tileIdN.y << zoomShift);
    auto tileSize31 = 1u << zoomShift;
    if (tileSize31 + static_cast<uint32_t>(tile31.x) > 2147483647u)
        tileSize31--;
    if (tileSize31 + static_cast<uint32_t>(tile31.y) > 2147483647u)
        tileSize31--;
    PointI nextTile31(tile31.x + tileSize31, tile31.y + tileSize31);
    const auto zoom = static_cast<int32_t>(zoomLevel);
    PointI minZoom(
        currentState.gridConfiguration.gridParameters[0].minZoom,
        currentState.gridConfiguration.gridParameters[1].minZoom);
    PointI maxZoomForFloat(
        currentState.gridConfiguration.gridParameters[0].maxZoomForFloat,
        currentState.gridConfiguration.gridParameters[1].maxZoomForFloat);
    PointI maxZoomForMixed(
        currentState.gridConfiguration.gridParameters[0].maxZoomForMixed,
        currentState.gridConfiguration.gridParameters[1].maxZoomForMixed);
    int primaryZoom = zoom > maxZoomForFloat.x ? (zoom > maxZoomForMixed.x ? 3 : 2) : (zoom < minZoom.x ? 0 : 1);
    int secondaryZoom = zoom > maxZoomForFloat.y ? (zoom > maxZoomForMixed.y ? 3 : 2) : (zoom < minZoom.y ? 0 : 1);
    auto zone = Utilities::getCodedZoneUTM(currentState.target31);

    auto refLon = currentState.gridConfiguration.getPrimaryGridReference(currentState.target31);
    PointI pointTR(nextTile31.x, tile31.y);
    PointI pointBL(tile31.x, nextTile31.y);
    auto refLonTL = currentState.gridConfiguration.getPrimaryGridReference(tile31);
    auto refLonBR = currentState.gridConfiguration.getPrimaryGridReference(nextTile31);
    auto refLonTR = currentState.gridConfiguration.getPrimaryGridReference(pointTR);
    auto refLonBL = currentState.gridConfiguration.getPrimaryGridReference(pointBL);
    auto isSmall = currentState.gridConfiguration.primaryGranularity != 0.0f
        || currentState.gridConfiguration.primaryGap <= 1.0f;
    if (!currentState.gridConfiguration.primaryGrid
        || zoomLevel < currentState.gridConfiguration.primaryMinZoomLevel
        || zoomLevel > currentState.gridConfiguration.primaryMaxZoomLevel
        || (primaryZoom > 2 && !isSmall)
        || (refLonTL != refLon && refLonBR != refLon && refLonTR != refLon && refLonBL != refLon))
        primaryZoom = 0;
    else
    {
        auto gridTileTL = currentState.gridConfiguration.getPrimaryGridLocation(tile31, &refLon);
        auto gridTileBR = currentState.gridConfiguration.getPrimaryGridLocation(nextTile31, &refLon);
        auto gridTileTR = currentState.gridConfiguration.getPrimaryGridLocation(pointTR, &refLon);
        auto gridTileBL = currentState.gridConfiguration.getPrimaryGridLocation(pointBL, &refLon);
        auto tileTX = getGridFractions(gridTileTL.x, gridTileTR.x);
        auto tileBX = getGridFractions(gridTileBL.x, gridTileBR.x);
        auto tileLY = getGridFractions(gridTileTL.y, gridTileBL.y);
        auto tileRY = getGridFractions(gridTileTR.y, gridTileBR.y);
        auto shiftX = getFloatShift(tileTX.x, tileTX.y, tileBX.x, tileBX.y);
        auto shiftY = getFloatShift(tileLY.x, tileLY.y, tileRY.x, tileRY.y);
        glUniform4f(program.vs.param.primaryGridTileTop,
            static_cast<float>(tileTX.x + shiftX.x),
            static_cast<float>(tileLY.x + shiftY.x),
            static_cast<float>(tileTX.y + shiftX.x),
            static_cast<float>(tileRY.x + shiftY.y));
        GL_CHECK_RESULT;
        glUniform4f(program.vs.param.primaryGridTileBot,
            static_cast<float>(tileBX.x + shiftX.y),
            static_cast<float>(tileLY.y + shiftY.x),
            static_cast<float>(tileBX.y + shiftX.y),
            static_cast<float>(tileRY.y + shiftY.y));
        GL_CHECK_RESULT;
    }

    refLon = currentState.gridConfiguration.getSecondaryGridReference(currentState.target31);
    pointTR.x = nextTile31.x;
    pointTR.y = tile31.y;
    pointBL.x = tile31.x;
    pointBL.y = nextTile31.y;
    refLonTL = currentState.gridConfiguration.getSecondaryGridReference(tile31);
    refLonBR = currentState.gridConfiguration.getSecondaryGridReference(nextTile31);
    refLonTR = currentState.gridConfiguration.getSecondaryGridReference(pointTR);
    refLonBL = currentState.gridConfiguration.getSecondaryGridReference(pointBL);
    isSmall = currentState.gridConfiguration.secondaryGranularity != 0.0f
        || currentState.gridConfiguration.secondaryGap <= 1.0f;
    if (!currentState.gridConfiguration.secondaryGrid
        || zoomLevel < currentState.gridConfiguration.secondaryMinZoomLevel
        || zoomLevel > currentState.gridConfiguration.secondaryMaxZoomLevel
        || (secondaryZoom > 2 && !isSmall)
        || (refLonTL != refLon && refLonBR != refLon && refLonTR != refLon && refLonBL != refLon))
        secondaryZoom = 0;
    else
    {
        auto gridTileTL = currentState.gridConfiguration.getSecondaryGridLocation(tile31, &refLon);
        auto gridTileBR = currentState.gridConfiguration.getSecondaryGridLocation(nextTile31, &refLon);
        auto gridTileTR = currentState.gridConfiguration.getSecondaryGridLocation(pointTR, &refLon);
        auto gridTileBL = currentState.gridConfiguration.getSecondaryGridLocation(pointBL, &refLon);
        auto tileTX = getGridFractions(gridTileTL.x, gridTileTR.x);
        auto tileBX = getGridFractions(gridTileBL.x, gridTileBR.x);
        auto tileLY = getGridFractions(gridTileTL.y, gridTileBL.y);
        auto tileRY = getGridFractions(gridTileTR.y, gridTileBR.y);
        auto shiftX = getFloatShift(tileTX.x, tileTX.y, tileBX.x, tileBX.y);
        auto shiftY = getFloatShift(tileLY.x, tileLY.y, tileRY.x, tileRY.y);
        glUniform4f(program.vs.param.secondaryGridTileTop,
            static_cast<float>(tileTX.x + shiftX.x),
            static_cast<float>(tileLY.x + shiftY.x),
            static_cast<float>(tileTX.y + shiftX.x),
            static_cast<float>(tileRY.x + shiftY.y));
        GL_CHECK_RESULT;
        glUniform4f(program.vs.param.secondaryGridTileBot,
            static_cast<float>(tileBX.x + shiftX.y),
            static_cast<float>(tileLY.y + shiftY.x),
            static_cast<float>(tileBX.y + shiftX.y),
            static_cast<float>(tileRY.y + shiftY.y));
        GL_CHECK_RESULT;
    }

    glUniform4i(program.vs.param.tileCoords31,
        tile31.x,
        tile31.y,
        1u << zoomShift & 2147483647u,
        zone << 4 | secondaryZoom << 2 | primaryZoom);
    GL_CHECK_RESULT;

    // Set tile coordinates offset
    const auto tileId = Utilities::getTileId(currentState.target31, zoomLevel);
    glUniform2f(program.vs.param.tileCoordsOffset,
        batch->tileId.x - tileId.x,
        batch->tileId.y - tileId.y);
    GL_CHECK_RESULT;

    // Subtile factors to apply raster/elevation resources of different size
    int subtileRasterFactor = 1;
    int subtileElevationFactor = 1;

    // Configure elevation data
    haveElevation = false;
    const auto elevationResources = batch->elevationResourcesInGPU;
    auto tileSize = static_cast<double>(AtlasMapRenderer::TileSize3D) *
        static_cast<double>(1ull << currentState.zoomLevel - zoomLevel);
    const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(
        zoomLevel,
        tileIdN.y,
        tileSize);
    const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(
        zoomLevel,
        tileIdN.y + 1,
        tileSize);
    float zScaleFactor = 0.0f;
    float dataScaleFactor = 0.0f;
    if (withElevation && currentState.elevationDataProvider && elevationResources && !elevationResources->empty())
    {
        const auto subtilesCount = batch->elevationResourcesInGPU->size();
        if (subtilesCount > subtilesPerTile)
        {
            subtileRasterFactor = subtilesCount / subtilesPerTile;
            subtilesPerTile = subtilesCount;
        }
        else if (subtilesCount < subtilesPerTile)
            subtileElevationFactor = subtilesPerTile / subtilesCount;
        zScaleFactor = currentState.elevationConfiguration.zScaleFactor;
        dataScaleFactor = currentState.elevationConfiguration.dataScaleFactor;
        haveElevation = true;
    }

    // Per-tile elevation data configuration
    glUniform4f(
        program.vs.param.elevation_scale,
        (float)upperMetersPerUnit,
        (float)lowerMetersPerUnit,
        zScaleFactor,
        dataScaleFactor);
    GL_CHECK_RESULT;

    // Shader expects blending to be premultiplied, since output color of fragment shader is premultiplied by alpha
    if (currentAlphaChannelType != AlphaChannelType::Premultiplied)
    {
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        GL_CHECK_RESULT;

        currentAlphaChannelType = AlphaChannelType::Premultiplied;
    }

    // Indicate the last batch to enable elevation colouring in shader
    glUniform1f(program.fs.param.lastBatch, batch->lastBatch ? 1.0f : 0.0f);
    GL_CHECK_RESULT;

    // Fragment shader needs to know if it's drawing on top of previous layers or the background
    glUniform1f(program.fs.param.blendingEnabled, (blendingEnabled ? 1.0f : 0.0f));
    GL_CHECK_RESULT;

    // Perform rendering of exact-scale, overscale and underscale cases. All cases support batching
    const auto indicesPerSubtile = _rasterTileIndicesCount / subtilesPerTile;
    for (auto subtileIndex = 0; subtileIndex < subtilesPerTile; subtileIndex++)
    {
        if (haveElevation)
        {
            const auto& elevationResource = (*elevationResources)[subtileIndex / subtileElevationFactor];
            if (elevationResource->resourceInGPU)
            {
                configureElevationData(elevationResource->resourceInGPU, program,
                    elevationResource->tileIdN,
                    elevationResource->zoomLevel,
                    elevationResource->texCoordsOffset,
                    elevationResource->texCoordsScale,
                    elevationResource->tileSize,
                    elevationDataSamplerIndex);
            }
            else
                cancelElevation(program, elevationDataSamplerIndex);
        }       

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

            bool withWindAnimation = false;
            const auto& batchedResourceInGPU = layer->resourcesInGPU[subtileIndex / subtileRasterFactor];
            switch (gpuAPI->getGpuResourceAlphaChannelType(batchedResourceInGPU->resourceInGPU))
            {
                case AlphaChannelType::Opaque:
                    withWindAnimation = true;
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

            // Calculate texture part-to-part transition phase
            auto dateTimePrevious = batchedResourceInGPU->resourceInGPU->dateTimePrevious;
            auto dateTimeNext = batchedResourceInGPU->resourceInGPU->dateTimeNext;
            auto dateTimeStep = dateTimeNext - dateTimePrevious;
            auto step = static_cast<double>(dateTimeStep);
            float transitionTime = static_cast<float>(step / 1000.0); // Transition time in seconds
            float transitionStage = -1.0; // If no transition is needed
            float animationStage = -1.0f; // If no animation is needed
            float tileSizeInParticles = internalState.referenceTileSizeOnScreenInPixels / _particleSize;
            if (dateTimeStep > 0)
            {
                if (currentState.dateTime <= dateTimePrevious)
                    transitionStage = 0.0f;
                else if (currentState.dateTime >= dateTimeNext)
                    transitionStage = 1.0f;
                else
                {
                    transitionStage =
                        static_cast<float>(static_cast<double>(currentState.dateTime - dateTimePrevious) / step);
                }
                if (withWindAnimation)
                {
                    const auto zoomDelta = 
                        static_cast<int>(currentState.zoomLevel) - static_cast<int>(_particleConstantSpeedMinZoom);
                    const auto zoomSpeedFactor = 360.0 / (zoomDelta > 0 ? static_cast<double>(1u << zoomDelta) : 1.0);
                    const auto realTimeStage = static_cast<double>(QDateTime::currentMSecsSinceEpoch() % dateTimeStep)
                        / step * static_cast<double>(_particleSpeedFactor) * zoomSpeedFactor;
                    animationStage = static_cast<float>((realTimeStage - qFloor(realTimeStage)) * 10.0);
                    if (animationStage >= 10.0f)
                        animationStage = 9.999999f;
                    invalidateFrame();
                }
            }

            glUniform4f(perTile_fs.transitionPhase,
                transitionStage,
                transitionTime,
                animationStage,
                tileSizeInParticles);
            GL_CHECK_RESULT;

            auto texelSize = gpuAPI->getGpuResourceTexelSize(batchedResourceInGPU->resourceInGPU);
            glUniform1f(perTile_fs.texelSize, texelSize);
            GL_CHECK_RESULT;

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

                glUniform4f(perTile_fs.texCoordsOffsetAndScale,
                    texCoordsOffset.x,
                    texCoordsOffset.y,
                    texCoordsScale.x,
                    texCoordsScale.y);
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

                glUniform4f(perTile_fs.texCoordsOffsetAndScale,
                    batchedResourceInGPU->texCoordsOffset.x,
                    batchedResourceInGPU->texCoordsOffset.y,
                    batchedResourceInGPU->texCoordsScale.x,
                    batchedResourceInGPU->texCoordsScale.y);
                GL_CHECK_RESULT;
            }
        }      
        // Perform drawing of a subtile
        glDrawElements(
            GL_TRIANGLES,
            indicesPerSubtile,
            GL_UNSIGNED_SHORT,
            reinterpret_cast<const void*>(sizeof(uint16_t) * indicesPerSubtile * subtileIndex));
        GL_CHECK_RESULT;
    }

    // Unbind textures from texture samplers, that were used
    const auto usedSamplersCount = batchedLayersCount + 1;
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
    const ZoomLevel zoomLevel)
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

    // Set symbol VAO
    gpuAPI->useVAO(vao);

    // Activate program
    glUseProgram(program.id);
    GL_CHECK_RESULT;

    // Set matrices
    glUniformMatrix4fv(program.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;

    // Scale the result
    glUniform4f(program.vs.param.resultScale,
        1.0f,
        currentState.flip ? -1.0f : 1.0f,
        1.0f,
        1.0f);
    GL_CHECK_RESULT;

    // Set center offset
    PointF offsetInTileN;
    Utilities::getTileId(currentState.target31, zoomLevel, &offsetInTileN);
    glUniform2f(program.vs.param.targetInTilePosN, offsetInTileN.x, offsetInTileN.y);
    GL_CHECK_RESULT;

    // Set tile size
    glUniform1f(program.vs.param.tileSize, AtlasMapRenderer::TileSize3D * (1 << currentState.zoomLevel - zoomLevel));
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
            currentState.elevationConfiguration.visualizationZ);
        GL_CHECK_RESULT;

        glUniform4f(program.vs.param.elevation_hillshadeConfiguration,
            glm::radians(currentState.elevationConfiguration.hillshadeSunAngle),
            glm::radians(currentState.elevationConfiguration.hillshadeSunAzimuth),
            0.0f,
            0.0f);
        GL_CHECK_RESULT;

        auto pColorMapEntry = currentState.elevationConfiguration.visualizationColorMap.data();
        for (auto colorMapEntryIndex = 0u; colorMapEntryIndex < ElevationConfiguration::MaxColorMapEntries; colorMapEntryIndex++, pColorMapEntry++)
        {
            glUniform4f(program.vs.param.elevation_colorMapKeys + colorMapEntryIndex,
                0.0f,
                0.0f,
                0.0f,
                pColorMapEntry->first);
            GL_CHECK_RESULT;

            glUniform4fv(program.vs.param.elevation_colorMapValues + colorMapEntryIndex,
                1,
                reinterpret_cast<const GLfloat*>(&pColorMapEntry->second));
            GL_CHECK_RESULT;
        }
    }

    glUniform4f(program.fs.param.backgroundColor,
        currentState.backgroundColor.r,
        currentState.backgroundColor.g,
        currentState.backgroundColor.b,
        1.0f);
    GL_CHECK_RESULT;

    // Set color, location and size of accuracy circle
    glUniform4f(program.fs.param.myLocationColor,
        currentState.myLocationColor.r,
        currentState.myLocationColor.g,
        currentState.myLocationColor.b,
        currentState.myLocationColor.a);
    GL_CHECK_RESULT;
    glUniform4f(program.fs.param.myLocation,
        myLocation.x,
        myLocation.z,
        myLocationRadius,
        1.0f);
    GL_CHECK_RESULT;

    // Set direction and radius of heading sector
    glUniform2f(program.fs.param.myDirection,
        headingDirection,
        headingRadius);
    GL_CHECK_RESULT;

    // Set parameters for grids
    glUniform4f(program.vs.param.primaryGridAxisX,
        currentState.gridConfiguration.gridParameters[0].factorX1,
        currentState.gridConfiguration.gridParameters[0].factorX2,
        currentState.gridConfiguration.gridParameters[0].factorX3,
        currentState.gridConfiguration.gridParameters[0].offsetX);
    GL_CHECK_RESULT;
    glUniform4f(program.vs.param.secondaryGridAxisX,
        currentState.gridConfiguration.gridParameters[1].factorX1,
        currentState.gridConfiguration.gridParameters[1].factorX2,
        currentState.gridConfiguration.gridParameters[1].factorX3,
        currentState.gridConfiguration.gridParameters[1].offsetX);
    GL_CHECK_RESULT;
    glUniform4f(program.vs.param.primaryGridAxisY,
        currentState.gridConfiguration.gridParameters[0].factorY1,
        currentState.gridConfiguration.gridParameters[0].factorY2,
        currentState.gridConfiguration.gridParameters[0].factorY3,
        currentState.gridConfiguration.gridParameters[0].offsetY);
    GL_CHECK_RESULT;
    glUniform4f(program.vs.param.secondaryGridAxisY,
        currentState.gridConfiguration.gridParameters[1].factorY1,
        currentState.gridConfiguration.gridParameters[1].factorY2,
        currentState.gridConfiguration.gridParameters[1].factorY3,
        currentState.gridConfiguration.gridParameters[1].offsetY);
    GL_CHECK_RESULT;
    auto currentGaps = currentState.gridConfiguration.getCurrentGaps(
        currentState.target31, currentState.surfaceZoomLevel);
    auto density = renderer->getSetupOptions().displayDensityFactor;
    glUniform4f(program.fs.param.gridParameters,
        static_cast<float>(currentGaps.x),
        currentState.gridConfiguration.primaryThickness * density / 2.0f,
        static_cast<float>(currentGaps.y),
        currentState.gridConfiguration.secondaryThickness * density / 2.0f);
    GL_CHECK_RESULT;
    glUniform4f(program.fs.param.primaryGridColor,
        currentState.gridConfiguration.primaryColor.r,
        currentState.gridConfiguration.primaryColor.g,
        currentState.gridConfiguration.primaryColor.b,
        currentState.gridConfiguration.primaryColor.a);
    GL_CHECK_RESULT;
    glUniform4f(program.fs.param.secondaryGridColor,
        currentState.gridConfiguration.secondaryColor.r,
        currentState.gridConfiguration.secondaryColor.g,
        currentState.gridConfiguration.secondaryColor.b,
        currentState.gridConfiguration.secondaryColor.a);
    GL_CHECK_RESULT;

    // Set camera position for mist calculations
    glUniform4f(program.fs.param.worldCameraPosition,
        internalState.worldCameraPosition.x,
        internalState.worldCameraPosition.y,
        internalState.worldCameraPosition.z,
        1.0f);
    GL_CHECK_RESULT;

    // Set mist parameters
    glm::vec2 leftDirection = (internalState.mAzimuthInv * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)).xz();
    glUniform4f(program.fs.param.mistConfiguration,
        leftDirection.x,
        leftDirection.y,
        internalState.distanceFromCameraToFog,
        internalState.distanceFromTargetToFog);
    GL_CHECK_RESULT;
    glUniform4f(program.fs.param.mistColor,
        currentState.fogColor.r,
        currentState.fogColor.g,
        currentState.fogColor.b,
        internalState.fogShiftFactor);
    GL_CHECK_RESULT;

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
    glActiveTexture(GL_TEXTURE0 + elevationDataSamplerIndex);
    GL_CHECK_RESULT;

    glUniform1i(program.vs.param.elevation_dataSampler, elevationDataSamplerIndex);
    GL_CHECK_RESULT;

    gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + elevationDataSamplerIndex, GPUAPI_OpenGL::SamplerType::ElevationDataTile);

    lastUsedProgram = program.id;

    GL_POP_GROUP_MARKER;

    return true;
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
        const auto resource = std::static_pointer_cast<MapRendererRasterMapLayerResource>(resource_);

        // Check state and obtain GPU resource
        std::shared_ptr<const GPUAPI::ResourceInGPU> gpuResource;
        auto state = resource->getState();
        if (state == MapRendererResourceState::Uploaded
            || state == MapRendererResourceState::PreparedRenew
            || state == MapRendererResourceState::Outdated
            || state == MapRendererResourceState::Renewing
            || state == MapRendererResourceState::Updating
            || state == MapRendererResourceState::RequestedUpdate
            || state == MapRendererResourceState::ProcessingUpdate
            || state == MapRendererResourceState::ProcessingUpdateWhileRenewing
            || state == MapRendererResourceState::UpdatingCancelledWhileBeingProcessed)
        {
            // Capture GPU resource (if any)
            if (resource->resourceInGPULock.testAndSetAcquire(0, 1))
            {
                gpuResource = resource->resourceInGPU;
                resource->resourceInGPULock.storeRelease(0);
            }

            if (outState != nullptr)
                *outState = state;
        }

        if (gpuResource
            && (gpuResource->dateTimeFirst == 0 || currentState.dateTime >= gpuResource->dateTimeFirst)
            && (gpuResource->dateTimeLast == 0 || currentState.dateTime <= gpuResource->dateTimeLast))
            return gpuResource;
        else if (outState != nullptr)
            *outState = resource->getState();
    }

    return nullptr;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::releaseRasterLayers(bool gpuContextLost)
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
        rasterLayerTileProgram.id = 0;
    }

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
    const GLfloat clusterSize = 1.0f / static_cast<float>(heightPrimitivesPerSide);
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

void OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::releaseRasterTile(bool gpuContextLost)
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
    const std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU>& elevationDataResource,
    const RasterLayerTileProgram& program,
    const TileId tileIdN,
    const ZoomLevel zoomLevel,
    const PointF& texCoordsOffsetN,
    const PointF& texCoordsScaleN,
    const double tileSize,
    const int elevationDataSamplerIndex)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    const auto& perTile_vs = program.vs.param.elevationLayer;

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

        // NOTE: Must be in sync with IMapElevationDataProvider::Data::getValue
        const PointF innerSize(texture->tileSizeN - 3.0f * texture->uTexelSizeN,
            texture->tileSizeN - 3.0f * texture->vTexelSizeN);
        const PointF texCoordsScale(innerSize.x * texCoordsScaleN.x, innerSize.y * texCoordsScaleN.y);
        const PointF texPlace(colIndex * texture->tileSizeN + texture->uHalfTexelSizeN + texture->uTexelSizeN,
            rowIndex * texture->tileSizeN + texture->vHalfTexelSizeN + texture->vTexelSizeN);
        const PointF texCoordsOffset(texPlace.x + innerSize.x * texCoordsOffsetN.x,
            texPlace.y + innerSize.y * texCoordsOffsetN.y);

        glUniform4f(perTile_vs.texCoordsOffsetAndScale,
            texCoordsOffset.x,
            texCoordsOffset.y,
            texCoordsScale.x,
            texCoordsScale.y);
        GL_CHECK_RESULT;
        glUniform4f(program.vs.param.elevationLayerTexelSize,
            texture->uTexelSizeN * texCoordsScaleN.x,
            texture->vTexelSizeN * texCoordsScaleN.y,
            texture->uHalfTexelSizeN * texCoordsScaleN.x,
            texture->vHalfTexelSizeN * texCoordsScaleN.y);
        GL_CHECK_RESULT;
        glUniform4f(program.vs.param.elevationLayerDataPlace,
            texPlace.x,
            texPlace.y,
            texture->uHalfTexelSizeN,
            texture->vHalfTexelSizeN);
        GL_CHECK_RESULT;
    }
    else // if (elevationDataResource->type == GPUAPI::ResourceInGPU::Type::Texture)
    {
        const auto& texture = std::static_pointer_cast<const GPUAPI::TextureInGPU>(elevationDataResource);

        const PointF innerSize(
            1.0f - 3.0f * texture->uTexelSizeN,
            1.0f - 3.0f * texture->vTexelSizeN
        );
        const PointF texCoordsScale(innerSize.x * texCoordsScaleN.x, innerSize.y * texCoordsScaleN.y);
        const PointF texPlace(texture->uHalfTexelSizeN + texture->uTexelSizeN,
            texture->vHalfTexelSizeN + texture->vTexelSizeN);
        const PointF texCoordsOffset(texPlace.x + innerSize.x * texCoordsOffsetN.x,
            texPlace.y + innerSize.y * texCoordsOffsetN.y);

        glUniform4f(perTile_vs.texCoordsOffsetAndScale,
            texCoordsOffset.x,
            texCoordsOffset.y,
            texCoordsScale.x,
            texCoordsScale.y);
        GL_CHECK_RESULT;
        glUniform4f(program.vs.param.elevationLayerTexelSize,
            texture->uTexelSizeN * texCoordsScaleN.x,
            texture->vTexelSizeN * texCoordsScaleN.y,
            texture->uHalfTexelSizeN * texCoordsScaleN.x,
            texture->vHalfTexelSizeN * texCoordsScaleN.y);
        GL_CHECK_RESULT;
        glUniform4f(program.vs.param.elevationLayerDataPlace,
            texPlace.x,
            texPlace.y,
            texture->uHalfTexelSizeN,
            texture->vHalfTexelSizeN);
        GL_CHECK_RESULT;
    }
}

void OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::cancelElevation(
    const RasterLayerTileProgram& program,
    const int elevationDataSamplerIndex)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glDisableVertexAttribArray);

    glActiveTexture(GL_TEXTURE0 + elevationDataSamplerIndex);
    GL_CHECK_RESULT;

    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_RESULT;
}

inline OsmAnd::PointD OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::getGridFractions(
    const double tile, const double nextTile)
{
    //Make 32-bit float coordinates more precise by using only fraction of values
    auto gridTileFraction = tile - std::floor(tile);
    PointD result(gridTileFraction, nextTile - tile + gridTileFraction);
    return result;
}

inline OsmAnd::PointD OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::getFloatShift(
    const double first1, const double second1, const double first2, const double second2)
{
    //Make 32-bit float coordinates more precise by shifting
    auto min1 = std::min(first1, second1);
    auto max1 = std::max(first1, second1);
    auto shift1 = min1 > 0.0 && max1 > 0.0 && 1.0 - min1 < max1 ? -1.0 : 0.0;
    shift1 = min1 < 0.0 && max1 < 0.0 && max1 + 1.0 < -min1 ? 1.0 : shift1;
    auto min2 = std::min(first2, second2);
    auto max2 = std::max(first2, second2);
    auto shift2 = min2 > 0.0 && max2 > 0.0 && 1.0 - min2 < max2 ? -1.0 : 0.0;
    shift2 = min2 < 0.0 && max2 < 0.0 && max2 + 1.0 < -min2 ? 1.0 : shift2;
    if (shift1 != shift2)
    {
        auto minPrev = std::min(min1, min2);
        auto maxPrev = std::max(max1, max2);
        auto minNext = std::min(min1 + shift1, min2 + shift2);
        auto maxNext = std::max(max1 + shift1, max2 + shift2);
        if (maxNext - minNext > maxPrev - minPrev)
        {
            shift1 = minPrev > 0.0 && maxPrev > 0.0 && 1.0 - minPrev < maxPrev ? -1.0 : 0.0;
            shift1 = minPrev < 0.0 && maxPrev < 0.0 && maxPrev + 1.0 < -minPrev ? 1.0 : shift1;
            shift2 = shift1;
        }
    }
    PointD result(shift1, shift2);
    return result;
}

QList< OsmAnd::Ref<OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::PerTileBatchedLayers> >
OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::batchLayersByTiles(
    const QVector<TileId>& tiles, const QSet<TileId>& visibleTilesSet, ZoomLevel zoomLevel)
{
    const auto& internalState = getInternalState();
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

    for (const auto& tileId : constOf(tiles))
    {
        const auto tileIdN = Utilities::normalizeTileId(tileId, zoomLevel);

        // Don't render invisible tiles
        if (!visibleTilesSet.contains(tileIdN))
            continue;

        // Collect elevation resources
        std::shared_ptr<QList<Ref<ElevationResource>>> elevationResources;
        elevationResources.reset(new QList<Ref<ElevationResource>>());
        if (currentState.elevationDataProvider)
        {
            ZoomLevel elevationZoom = zoomLevel;
            auto tileSize = static_cast<double>(AtlasMapRenderer::TileSize3D) *
                static_cast<double>(1ull << currentState.zoomLevel - elevationZoom);
            auto elevationResource = captureElevationDataResource(tileIdN, zoomLevel);
            if (elevationResource)
            {
                elevationResources->push_back(Ref<ElevationResource>::New(
                    elevationResource,
                    tileIdN,
                    elevationZoom,
                    tileSize));                
            }
            else
            {            
                const auto maxMissingDataZoomShift = currentState.elevationDataProvider->getMaxMissingDataZoomShift();
                const auto maxUnderZoomShift = currentState.elevationDataProvider->getMaxMissingDataUnderZoomShift();
                const auto minZoom = currentState.elevationDataProvider->getMinZoom();
                const auto maxZoom = currentState.elevationDataProvider->getMaxZoom();
                for (int absZoomShift = 1; absZoomShift <= maxMissingDataZoomShift; absZoomShift++)
                {
                    // Look for underscaled first. Only full match is accepted.
                    // Don't replace tiles of absent zoom levels by the unserscaled ones
                    const auto underscaledZoom = static_cast<int>(zoomLevel) + absZoomShift;
                    if (underscaledZoom >= minZoom && underscaledZoom <= maxZoom && absZoomShift <= maxUnderZoomShift &&
                        zoomLevel >= minZoom)
                    {
                        elevationZoom = static_cast<ZoomLevel>(underscaledZoom);
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
                            gpuResource = captureElevationDataResource(
                                underscaledTileId,
                                elevationZoom);
                            if (gpuResource)
                                atLeastOnePresent = true;
                        }

                        if (atLeastOnePresent)
                        {
                            tileSize = static_cast<double>(AtlasMapRenderer::TileSize3D) *
                                (elevationZoom > currentState.zoomLevel ?
                                1.0 / static_cast<double>(1ull << elevationZoom - currentState.zoomLevel) :
                                static_cast<double>(1ull << currentState.zoomLevel - elevationZoom));
                            const auto subtilesPerSide = (1u << absZoomShift);
                            const PointF texCoordsScale(subtilesPerSide, subtilesPerSide);
                            auto pGpuResource = gpuResources.constData();
                            pUnderscaledTileIdN = underscaledTileIdsN.constData();
                            for (auto subtileIdx = 0; subtileIdx < subtilesCount; subtileIdx++)
                            {
                                const auto& underscaledTileId = *(pUnderscaledTileIdN++);
                                const auto& gpuResource = *(pGpuResource++);
                                if (gpuResource)
                                {
                                    uint16_t xSubtile;
                                    uint16_t ySubtile;
                                    Utilities::decodeMortonCode(subtileIdx, xSubtile, ySubtile);
                                    elevationResources->push_back(Ref<ElevationResource>::New(
                                        gpuResource,
                                        underscaledTileId,
                                        elevationZoom,
                                        tileSize,
                                        PointF(-xSubtile, -ySubtile),
                                        texCoordsScale));
                                }
                                else
                                {
                                    // Stub texture requires no texture offset or scaling
                                    elevationResources->push_back(Ref<ElevationResource>::New(
                                        nullptr,
                                        underscaledTileId,
                                        elevationZoom,
                                        tileSize));
                                }
                            }
                            break;
                        }
                    }

                    // If underscaled was not found, look for overscaled (surely, if such zoom level exists at all)
                    const auto overscaledZoom = static_cast<int>(zoomLevel) - absZoomShift;
                    if (overscaledZoom >= minZoom && overscaledZoom <= maxZoom)
                    {
                        PointF texCoordsOffset;
                        PointF texCoordsScale;
                        const auto overscaledTileIdN = Utilities::getTileIdOverscaledByZoomShift(
                            tileIdN,
                            absZoomShift,
                            &texCoordsOffset,
                            &texCoordsScale);
                        elevationZoom = static_cast<ZoomLevel>(overscaledZoom);
                        if (elevationResource = captureElevationDataResource(
                            overscaledTileIdN,
                            elevationZoom))
                        {
                            tileSize = static_cast<double>(AtlasMapRenderer::TileSize3D) *
                                static_cast<double>(1ull << currentState.zoomLevel - elevationZoom);
                            elevationResources->push_back(Ref<ElevationResource>::New(
                                elevationResource,
                                overscaledTileIdN,
                                elevationZoom,
                                tileSize,
                                texCoordsOffset,
                                texCoordsScale));                
                            break;
                        }
                    }
                }
            }
        }

        bool atLeastOneNotUnavailable = false;
        MapRendererResourceState resourceState;

        auto batch = Ref<PerTileBatchedLayers>::New(tileId, true);
        perTileBatchedLayers.push_back(batch);
        batch->elevationResourcesInGPU = elevationResources;

        for (const auto& mapLayerEntry : rangeOf(constOf(currentState.mapLayersProviders)))
        {
            const auto layerIndex = mapLayerEntry.key();
            const auto& provider = mapLayerEntry.value();
            const auto resourcesCollection = getResources().getCollectionSnapshot(
                MapRendererResourceType::MapLayer,
                std::dynamic_pointer_cast<IMapDataProvider>(provider));

            // In case there's no resources' collection for this provider, there's nothing to do here, move on
            if (!resourcesCollection)
                continue;

            Ref<BatchedLayer> batchedLayer(new BatchedLayer(
                (std::dynamic_pointer_cast<IRasterMapLayerProvider>(provider) != nullptr)
                    ? BatchedLayerType::Raster
                    : BatchedLayerType::Other,
                layerIndex));

            auto maxMissingDataZoomShift = provider->getMaxMissingDataZoomShift();
            auto maxMissingDataUnderZoomShift = provider->getMaxMissingDataUnderZoomShift();

            // Try to obtain more detailed resource (of higher zoom level) if needed and possible
            int neededZoom = internalState.zoomLevelOffset == 0 ? zoomLevel : std::min(static_cast<int>(MaxZoomLevel),
                zoomLevel + std::min(internalState.zoomLevelOffset, maxMissingDataUnderZoomShift));
            neededZoom = internalState.zoomLevelOffset != 0 || internalState.extraDetailedTiles.empty() ? neededZoom
                : std::min(static_cast<int>(MaxZoomLevel), zoomLevel + std::min(1, maxMissingDataUnderZoomShift));
            bool haveMatch = false;
            while (neededZoom > zoomLevel)
            {
                const int absZoomShift = neededZoom - zoomLevel;
                const auto underscaledTileIdsN = Utilities::getTileIdsUnderscaledByZoomShift(
                    tileIdN,
                    absZoomShift);
                const auto subtilesCount = underscaledTileIdsN.size();

                bool allPresent = true;
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
                        static_cast<ZoomLevel>(neededZoom));
                    if (!gpuResource)
                        allPresent = false;
                }

                if (allPresent)
                {
                    const auto subtilesPerSide = (1u << absZoomShift);
                    const PointF texCoordsScale(subtilesPerSide, subtilesPerSide);

                    const bool isFirst = batch->containsOriginLayer &&
                        layerIndex == currentState.mapLayersProviders.firstKey();

                    const auto& stubResource = isFirst ? (atLeastOneNotUnavailable
                        ? getResources().processingTileStubs[static_cast<int>(stubsStyle)]
                        : getResources().unavailableTileStubs[static_cast<int>(stubsStyle)])
                        : getResources().unavailableTileStubs[static_cast<int>(MapStubStyle::Empty)];

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

                    haveMatch = true;
                }
                if (haveMatch)
                    break;
                neededZoom--;
            }

            if (!haveMatch)
            {
                // Try to obtain exact match resource
                const auto exactMatchGpuResource = captureLayerResource(
                    resourcesCollection,
                    tileIdN,
                    zoomLevel,
                    &resourceState);
                if (resourceState != MapRendererResourceState::Unavailable)
                    atLeastOneNotUnavailable = true;
                if (exactMatchGpuResource)
                {
                    // Exact match, no zoom shift or offset
                    batchedLayer->resourcesInGPU.push_back(Ref<BatchedLayerResource>::New(exactMatchGpuResource));
                    haveMatch = true;
                }
            }
            if (!haveMatch)
            {
                // Exact match was not found, so now try to look for overscaled/underscaled resources, taking into account
                // provider's maxMissingDataZoomShift and current zoom. It's better to show Z-"nearest" resource available,
                // giving preference to underscaled resource
                for (int absZoomShift = 1; absZoomShift <= maxMissingDataZoomShift; absZoomShift++)
                {
                    // Look for underscaled first. Only full match is accepted
                    if (Q_LIKELY(!debugSettings->rasterLayersUnderscaleForbidden))
                    {
                        const auto underscaledZoom = static_cast<int>(zoomLevel) + absZoomShift;
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

                                const bool isFirst = batch->containsOriginLayer &&
                                    layerIndex == currentState.mapLayersProviders.firstKey();

                                const auto& stubResource = isFirst ? (atLeastOneNotUnavailable
                                    ? getResources().processingTileStubs[static_cast<int>(stubsStyle)]
                                    : getResources().unavailableTileStubs[static_cast<int>(stubsStyle)])
                                    : getResources().unavailableTileStubs[static_cast<int>(MapStubStyle::Empty)];

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
                        const auto overscaleZoom = static_cast<int>(zoomLevel) - absZoomShift;
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
                batch->elevationResourcesInGPU = elevationResources;
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

        if (!batch->layers.isEmpty())
            batch->lastBatch = true;

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

OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::ElevationResource::ElevationResource(
    const std::shared_ptr<const GPUAPI::ResourceInGPU>& resourceInGPU_,
    const TileId tileIdN_,
    const ZoomLevel zoomLevel_,
    const double tileSize_,
    const PointF texCoordsOffset_ /*= PointF(0.0f, 0.0f)*/,
    const PointF texCoordsScale_ /*= PointF(1.0f, 1.0f)*/)
    : resourceInGPU(resourceInGPU_)
    , tileIdN(tileIdN_)
    , zoomLevel(zoomLevel_)
    , tileSize(tileSize_)
    , texCoordsOffset(texCoordsOffset_)
    , texCoordsScale(texCoordsScale_)
{
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

OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::BatchedLayer::BatchedLayer(
    const BatchedLayerType type_,
    const int layerIndex_)
    : type(type_)
    , layerIndex(layerIndex_)
{
}

OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::PerTileBatchedLayers::PerTileBatchedLayers(
    const TileId tileId_,
    const bool containsOriginLayer_)
    : tileId(tileId_)
    , lastBatch(false)
    , containsOriginLayer(containsOriginLayer_)
{
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::PerTileBatchedLayers::operator<(
    const PerTileBatchedLayers& that) const
{
    if (this == &that)
        return false;

    return layers.first()->layerIndex < that.layers.first()->layerIndex;
}
