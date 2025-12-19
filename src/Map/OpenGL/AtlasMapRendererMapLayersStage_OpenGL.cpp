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
    , _supportedMaxNumberOfRasterMapLayersInBatch(0)
    , _numberOfProgramsToLink(0)
    , _rasterTileIndicesCount(-1)
{
}

OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::~AtlasMapRendererMapLayersStage_OpenGL() = default;

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::initialize()
{
    bool ok = true;
    ok = ok && initializeRasterLayers();
    _supportedMaxNumberOfRasterMapLayersInBatch = _maxNumberOfRasterMapLayersInBatch;
    _numberOfProgramsToLink = _maxNumberOfRasterMapLayersInBatch * (RenderingFeatures::All + 1);
    if (_rasterLayerTilePrograms.size() != _maxNumberOfRasterMapLayersInBatch)
        _rasterLayerTilePrograms.clear();
    return ok;
}

OsmAnd::MapRendererStage::StageResult OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::render(
    IMapRenderer_Metrics::Metric_renderFrame* metric_)
{
    const auto metric = dynamic_cast<AtlasMapRenderer_Metrics::Metric_renderFrame*>(metric_);

    if (_maxNumberOfRasterMapLayersInBatch < 1)
        return StageResult::Fail;

    // Initialize programs that support [1 ... _maxNumberOfRasterMapLayersInBatch] as number of layers
    if (_numberOfProgramsToLink > 0)
    {
        int leftToLink = _maxNumberOfRasterMapLayersInBatch * (RenderingFeatures::All + 1);
        for (auto layersInBatch = _maxNumberOfRasterMapLayersInBatch; layersInBatch >= 1; layersInBatch--)
        {
            auto& rasterLayerTilePrograms = _rasterLayerTilePrograms[layersInBatch];
            if (rasterLayerTilePrograms.isEmpty())
                rasterLayerTilePrograms.resize(RenderingFeatures::All + 1);
            bool success = true;
            for (int programFeatures = RenderingFeatures::All; programFeatures >= 0; programFeatures--)
            {
                leftToLink--;
                if (leftToLink >= _numberOfProgramsToLink)
                    continue;
                success = initializeRasterLayersProgram(
                    layersInBatch, static_cast<RenderingFeatures>(programFeatures), rasterLayerTilePrograms);
                if (success)
                {
                    _numberOfProgramsToLink = leftToLink;
                    return StageResult::Wait;
                }
                else
                {
                    for (int linked = RenderingFeatures::All; linked > programFeatures; linked--)
                    {
                        leftToLink++;
                        if (rasterLayerTilePrograms[linked].id.isValid())
                        {
                            glDeleteProgram(rasterLayerTilePrograms[linked].id);
                            GL_CHECK_RESULT;

                            rasterLayerTilePrograms[linked].id = 0;
                        }
                    }
                    break;
                }
            }
            if (!success)
            {
                _numberOfProgramsToLink = leftToLink - RenderingFeatures::All;
                _rasterLayerTilePrograms.remove(layersInBatch);
                _supportedMaxNumberOfRasterMapLayersInBatch--;
                return StageResult::Wait;
            }
        }
    }

    if (_numberOfProgramsToLink == 0)
    {
        _numberOfProgramsToLink--;
        if (_supportedMaxNumberOfRasterMapLayersInBatch != _maxNumberOfRasterMapLayersInBatch)
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Seems like buggy driver. "
                "This device should be capable of rendering %d raster map layers in batch, but only %d compile",
                _maxNumberOfRasterMapLayersInBatch,
                _supportedMaxNumberOfRasterMapLayersInBatch);

            _maxNumberOfRasterMapLayersInBatch = _supportedMaxNumberOfRasterMapLayersInBatch;

            if (_maxNumberOfRasterMapLayersInBatch < 1)
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Maximal number of raster map layers in a batch can't be 0");
                return StageResult::Fail;
            }
        }

        initializeRasterTile();
    }

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
    PointF offsetInTileN;
    const auto tileId = Utilities::normalizeTileId(Utilities::getTileId(
        currentState.myLocation31, currentState.zoomLevel, &offsetInTileN), currentState.zoomLevel);
    double metersPerUnit;
    if (currentState.flatEarth)
    {
        const auto upperMetersPerUnit =
                Utilities::getMetersPerTileUnit(currentState.zoomLevel, tileId.y, AtlasMapRenderer::TileSize3D);
        const auto lowerMetersPerUnit =
                Utilities::getMetersPerTileUnit(currentState.zoomLevel, tileId.y + 1, AtlasMapRenderer::TileSize3D);
        metersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offsetInTileN.y);
    }
    else
        metersPerUnit = internalState.metersPerUnit;
    float myLocationHeight = 0.0;
    const auto myLocationHeightInMeters = renderer->getLocationHeightInMeters(currentState, currentState.myLocation31);
    if (myLocationHeightInMeters > -20000.0f)
    {
        float scaleFactor =
            currentState.elevationConfiguration.dataScaleFactor * currentState.elevationConfiguration.zScaleFactor;
        myLocationHeight = scaleFactor * myLocationHeightInMeters / metersPerUnit;
    }
    myLocationRadius = currentState.myLocationRadiusInMeters / metersPerUnit;
    const float cameraHeight = internalState.distanceFromCameraToGround;
    const float sizeScale = cameraHeight > myLocationHeight && !qFuzzyIsNull(cameraHeight)
        ? 1.0f - myLocationHeight / cameraHeight : 1.0f;
    headingRadius = currentState.myDirectionRadius * internalState.pixelInWorldProjectionScale * sizeScale;
    PointD angles;
    glm::dvec3 n;
    myLocation = currentState.flatEarth
        ? Utilities::planeWorldCoordinates(currentState.myLocation31,
            currentState.target31, currentState.zoomLevel, AtlasMapRenderer::TileSize3D, myLocationHeight)
        : Utilities::sphericalWorldCoordinates(currentState.myLocation31,
            internalState.mGlobeRotationPrecise, internalState.globeRadius, myLocationHeight, &angles, &n);
    const auto directionAngle = qDegreesToRadians(qIsNaN(currentState.myDirection)
        ? Utilities::normalizedAngleDegrees(currentState.azimuth + 180.0f) : currentState.myDirection);
    const auto directionVector = glm::dvec3(qSin(directionAngle), 0.0, -qCos(directionAngle));
    if (currentState.flatEarth)
        headingDirection = directionVector;
    else
        headingDirection = internalState.mGlobeRotation * Utilities::getModelRotationMatrix(angles) * directionVector;

    GLname lastUsedProgram;
    bool withElevation = true;
    QMap<ZoomLevel, QSet<TileId>> elevatedTiles;
    auto tilesBegin = internalState.visibleTiles.cbegin();
    for (auto itTiles = internalState.visibleTiles.cend(); itTiles != tilesBegin; itTiles--)
    {
        const auto& tilesEntry = itTiles - 1;
        const auto zoomLevel = tilesEntry.key();
        const auto& visibleTilesSet = internalState.visibleTilesSet.constFind(zoomLevel);
        if (visibleTilesSet == internalState.visibleTilesSet.cend())
            continue;
        auto& elevatedTilesSet = elevatedTiles[zoomLevel];
        const auto& batchedLayersByTiles = batchLayersByTiles(
            tilesEntry.value(),
            visibleTilesSet.value(),
            zoomLevel,
            elevatedTilesSet,
            withElevation);
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
                    elevatedTiles,
                    itTiles == internalState.visibleTiles.cend(),
                    withElevation,
                    blendingEnabled,
                    zoomLevel);
            }
        }

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

    return StageResult::Success;
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
        1 /*txOffsetAndScale*/;
    const auto fsUniformsPerLayer =
        1 /*transitionPhase*/ +
        1 /*texOffsetAndScale*/ +
        1 /*texelSize*/ +
        1 /*isPremultipliedAlpha*/ +
        1 /*opacityFactor*/ +
        1 /*sampler*/;
    const auto vsOtherUniforms =
        4 /*param_vs_mPerspectiveProjectionView*/ +
        1 /*param_vs_resultScale*/ +
        3 /*param_vs_mGlobeRotation*/ +
        1 /*param_vs_targetInTilePosN*/ +
        1 /*param_vs_objectSizes*/ +
        (!gpuAPI->isSupported_textureLod
            ? 0
            : 1 /*param_vs_distanceFromCameraToTarget*/ +
              1 /*param_vs_cameraElevationAngleN*/ +
              1 /*param_vs_groundCameraPosition*/ +
              1 /*param_vs_scaleToRetainProjectedSize*/) +
        1 /*param_vs_tileCoords31*/ +
        1 /*param_vs_globeTileTL*/ +
        1 /*param_vs_globeTileTR*/ +
        1 /*param_vs_globeTileBL*/ +
        1 /*param_vs_globeTileBR*/ +
        1 /*param_vs_globeTileTLnv*/ +
        1 /*param_vs_globeTileTRnv*/ +
        1 /*param_vs_globeTileBLnv*/ +
        1 /*param_vs_globeTileBRnv*/ +
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
        1 /*param_fs_hillshade*/ +
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
        (setupOptions.elevationVisualizationEnabled
            ? 12 : 0) /*v2f_hsTL + v2f_hsTR + v2f_hsBL + v2f_hsBR + v2f_hsCoordsDir + v2f_hsCoords*/ +
        4 /*v2f_primaryLocation*/ +
        4 /*v2f_secondaryLocation*/ +
        4 /*v2f_position*/ +
        4 /*v2f_normal*/;
    const auto maxBatchSizeByVaryingFloats =
        (gpuAPI->maxVaryingFloats - otherVaryingFloats) / varyingFloatsPerLayer;

    // ... by varying vectors
    const auto varyingVectorsPerLayer =
        1 /*v2f_texCoordsPerLayer_%rasterLayerIndex%*/;
    const auto otherVaryingVectors =
        1 /*v2f_metersPerUnit*/ +
        (gpuAPI->isSupported_textureLod ? 1 : 0) /*v2f_mipmapLOD*/ +
        (setupOptions.elevationVisualizationEnabled
            ? 6 : 0) /*v2f_hsTL + v2f_hsTR + v2f_hsBL + v2f_hsBR + v2f_hsCoordsDir + v2f_hsCoords*/ +
        1 /*v2f_primaryLocation*/ +
        1 /*v2f_secondaryLocation*/ +
        1 /*v2f_position*/ +
        1 /*v2f_normal*/;
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

    return true;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::initializeRasterLayersProgram(
    const unsigned int numberOfLayersInBatch,
    const RenderingFeatures programFeatures,
    QVector<RasterLayerTileProgram>& outRasterLayerTilePrograms)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteShader);
    GL_CHECK_PRESENT(glDeleteProgram);

    auto variableLocations = QList< std::tuple<GlslVariableType, QString, GLint> >({
        { GlslVariableType::In, QStringLiteral("in_vs_vertexPosition"), 0 },
        { GlslVariableType::In, QStringLiteral("in_vs_vertexTexCoords"), 1 },
    });
    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    auto& outRasterLayerTileProgram = outRasterLayerTilePrograms[programFeatures];
    outRasterLayerTileProgram.id = 0;
    if (!outRasterLayerTileProgram.binaryCache.isEmpty())
    {
        outRasterLayerTileProgram.id = getGPUAPI()->linkProgram(0, nullptr, variableLocations,
            outRasterLayerTileProgram.binaryCache, outRasterLayerTileProgram.cacheFormat, true, &variablesMap);
    }
    if (!outRasterLayerTileProgram.id.isValid())
    {
        auto vertexShader = QStringLiteral(
            // Input data
            "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
            "INPUT vec2 in_vs_vertexTexCoords;                                                                                  ""\n"
            "                                                                                                                   ""\n"
            // Output data to next shader stages
            "%UnrolledPerRasterLayerTexCoordsDeclarationCode%                                                                   ""\n"
            "PARAM_OUTPUT float v2f_metersPerUnit;                                                                              ""\n"
            "PARAM_OUTPUT float v2f_colorIntensity;                                                                             ""\n"
            "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
            "    PARAM_OUTPUT float v2f_mipmapLOD;                                                                              ""\n"
            "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n");
        const auto& hillshadeInVertexShader_1 = QStringLiteral(
            "PARAM_FLAT_OUTPUT highp vec2 v2f_hsTL;                                                                             ""\n"
            "PARAM_FLAT_OUTPUT highp vec2 v2f_hsTR;                                                                             ""\n"
            "PARAM_FLAT_OUTPUT highp vec2 v2f_hsBL;                                                                             ""\n"
            "PARAM_FLAT_OUTPUT highp vec2 v2f_hsBR;                                                                             ""\n"
            "PARAM_FLAT_OUTPUT highp vec2 v2f_hsCoordsDir;                                                                      ""\n"
            "PARAM_OUTPUT highp vec2 v2f_hsCoords;                                                                              ""\n");
        const auto& gridsInVertexShader_1 = QStringLiteral(
            "PARAM_OUTPUT vec4 v2f_primaryLocation;                                                                             ""\n"
            "PARAM_OUTPUT vec4 v2f_secondaryLocation;                                                                           ""\n");
        vertexShader.append(QStringLiteral(
            "%HillshadeInVertexShader_1%                                                                                        ""\n"
            "%GridsInVertexShader_1%                                                                                            ""\n"
            "PARAM_OUTPUT vec4 v2f_position;                                                                                    ""\n"
            "PARAM_OUTPUT vec4 v2f_normal;                                                                                      ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform mat4 param_vs_mPerspectiveProjectionView;                                                                  ""\n"
            "uniform vec4 param_vs_resultScale;                                                                                 ""\n"
            "uniform mat3 param_vs_mGlobeRotation;                                                                              ""\n"
            "uniform vec2 param_vs_targetInTilePosN;                                                                            ""\n"
            "uniform vec3 param_vs_objectSizes;                                                                                 ""\n"
            "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
            "    uniform float param_vs_distanceFromCameraToTarget;                                                             ""\n"
            "    uniform float param_vs_cameraElevationAngleN;                                                                  ""\n"
            "    uniform vec2 param_vs_groundCameraPosition;                                                                    ""\n"
            "    uniform float param_vs_scaleToRetainProjectedSize;                                                             ""\n"
            "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
            "uniform highp vec4 param_vs_elevationLayerTexelSize;                                                               ""\n"));
        const auto& gridsInVertexShader_2 = QStringLiteral(
            "uniform vec4 param_vs_primaryGridAxisX;                                                                            ""\n"
            "uniform vec4 param_vs_secondaryGridAxisX;                                                                          ""\n"
            "uniform vec4 param_vs_primaryGridAxisY;                                                                            ""\n"
            "uniform vec4 param_vs_secondaryGridAxisY;                                                                          ""\n"
            "uniform vec4 param_vs_primaryGridTileTop;                                                                          ""\n"
            "uniform vec4 param_vs_primaryGridTileBot;                                                                          ""\n"
            "uniform vec4 param_vs_secondaryGridTileTop;                                                                        ""\n"
            "uniform vec4 param_vs_secondaryGridTileBot;                                                                        ""\n");
        vertexShader.append(QStringLiteral(
            "%GridsInVertexShader_2%                                                                                            ""\n"
            "uniform ivec4 param_vs_tileCoords31;                                                                               ""\n"
            "uniform vec3 param_vs_globeTileTL;                                                                                 ""\n"
            "uniform vec3 param_vs_globeTileTR;                                                                                 ""\n"
            "uniform vec3 param_vs_globeTileBL;                                                                                 ""\n"
            "uniform vec3 param_vs_globeTileBR;                                                                                 ""\n"
            "uniform vec3 param_vs_globeTileTLnv;                                                                               ""\n"
            "uniform vec3 param_vs_globeTileTRnv;                                                                               ""\n"
            "uniform vec3 param_vs_globeTileBLnv;                                                                               ""\n"
            "uniform vec3 param_vs_globeTileBRnv;                                                                               ""\n"
            "uniform vec2 param_vs_tileCoordsOffset;                                                                            ""\n"
            "uniform vec4 param_vs_elevation_scale;                                                                             ""\n"
            "uniform highp sampler2D param_vs_elevation_dataSampler;                                                            ""\n"
            "                                                                                                                   ""\n"
            // Parameters: per-layer-in-tile data
            "struct VsRasterLayerTile                                                                                           ""\n"
            "{                                                                                                                  ""\n"
            "    highp vec4 txOffsetAndScale;                                                                                   ""\n"
            "};                                                                                                                 ""\n"
            "%UnrolledPerRasterLayerParamsDeclarationCode%                                                                      ""\n"
            "uniform VsRasterLayerTile param_vs_elevationLayer;                                                                 ""\n"
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
            "float averageHeight(in mat4 h, in int x, in int y)                                                                 ""\n"
            "{                                                                                                                  ""\n"
            "    int l = max(x - 1, 0);                                                                                         ""\n"
            "    int r = x + 1;                                                                                                 ""\n"
            "    int u = max(y - 1, 0);                                                                                         ""\n"
            "    int d = y + 1;                                                                                                 ""\n"
            "    float e = h[x][y];                                                                                             ""\n"
            "    e += (h[l][y] + h[r][y] + h[x][u] + h[x][d]) / 2.0 + (h[l][u] + h[r][u] + h[l][d] + h[r][d]) / 4.0;            ""\n"
            "    return e / 4.0;                                                                                                ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "float extraUpToSphere(in float tileSize, in vec2 posN, in vec3 nTL, in vec3 nTR, in vec3 nBL, in vec3 nBR)         ""\n"
            "{                                                                                                                  ""\n"
            "    vec2 dF = (1.0 - posN) / 2.0;                                                                                  ""\n"
            "    vec2 xE = tan(acos(min(vec2(dot(nTL, nTR), dot(nBL, nBR)), 1.0)) * dF.x) * posN.x;                             ""\n"
            "    vec2 yE = tan(acos(min(vec2(dot(nTL, nBL), dot(nTR, nBR)), 1.0)) * dF.y) * posN.y;                             ""\n"
            "    return (mix(xE.x, xE.y, posN.y) + mix(yE.x, yE.y, posN.x)) * tileSize * 0.5;                                   ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void calculateTextureCoordinates(in VsRasterLayerTile tileLayer, out vec2 outTexCoords)                            ""\n"
            "{                                                                                                                  ""\n"
            "    vec2 texCoords = in_vs_vertexTexCoords;                                                                        ""\n"
            "    texCoords = texCoords * tileLayer.txOffsetAndScale.zw + tileLayer.txOffsetAndScale.xy;                         ""\n"
            "    outTexCoords = texCoords;                                                                                      ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            //   Define needed constants
            "    const float M_PI = 3.1415926535897932384626433832795;                                                          ""\n"
            "    const float M_PI_2 = M_PI / 2.0;                                                                               ""\n"
            "    const float M_2PI = 2.0 * M_PI;                                                                                ""\n"
            "                                                                                                                   ""\n"
            //   Calculate basic vertex coordinates
            "    int tilePiece = param_vs_tileCoords31.z / %HeixelsPerTileSide%;                                                ""\n"
            "    ivec2 vertexIndex = ivec2(in_vs_vertexPosition * %HeixelsPerTileSide%.0 + 0.5);                                ""\n"
            "    ivec2 loc31 = vertexIndex * tilePiece;                                                                         ""\n"
            "    bool overX = loc31.x - 1 + param_vs_tileCoords31.x == 2147483647;                                              ""\n"
            "    bool overY = loc31.y - 1 + param_vs_tileCoords31.y == 2147483647;                                              ""\n"
            "    loc31.x = overX ? 0 : loc31.x + param_vs_tileCoords31.x;                                                       ""\n"
            "    loc31.y = overY ? 0 : loc31.y + param_vs_tileCoords31.y;                                                       ""\n"
            "    vec2 angles = (vec2(loc31) / 65536.0 / 32768.0 - 0.5) * M_2PI;                                                 ""\n"
            "    angles.y = M_PI_2 - atan(exp(angles.y)) * 2.0;                                                                 ""\n"
            "                                                                                                                   ""\n"
            //   Pre-compute mercator meters for further (possible) grid calculations
            "    vec2 mercMeters = vec2(overX ? 2147483647 : loc31.x, overY ? 2147483647 : loc31.y) / 65536.0 / 32768.0 - 0.5;  ""\n"
            "                                                                                                                   ""\n"
            //   Calculate vertex position on the globe surface
            "    angles.y = overY ? -M_PI_2 : (loc31.y == 0 ? M_PI_2 : angles.y);                                               ""\n"
            "    float csy = cos(angles.y);                                                                                     ""\n"
            "    vec3 nv = param_vs_mGlobeRotation * vec3(csy * sin(angles.x), csy * cos(angles.x), -sin(angles.y));            ""\n"
            "    vec3 cv = vec3(nv.x, nv.y - param_vs_objectSizes.y, nv.z);                                                     ""\n"
            "                                                                                                                   ""\n"
            //   Scale and shift vertex to it's proper position on the plane
            "    vec2 pv = in_vs_vertexPosition;                                                                                ""\n"
            "    pv = (pv + param_vs_tileCoordsOffset - param_vs_targetInTilePosN) * param_vs_objectSizes.x;                    ""\n"
            "    cv = param_vs_objectSizes.z < 0.0 ? vec3(pv.x, 0.0, pv.y) : cv;                                                ""\n"
            "    nv = param_vs_objectSizes.z < 0.0 ? vec3(0.0, 1.0, 0.0) : normalize(nv);                                       ""\n"
            "                                                                                                                   ""\n"
            //   Find proper position of vertex on the tile
            "    vec3 leftVertex = mix(param_vs_globeTileTL, param_vs_globeTileBL, in_vs_vertexPosition.y);                     ""\n"
            "    vec3 rightVertex = mix(param_vs_globeTileTR, param_vs_globeTileBR, in_vs_vertexPosition.y);                    ""\n"
            "    vec3 midVertex = mix(leftVertex, rightVertex, in_vs_vertexPosition.x);                                         ""\n"
            "    vec3 leftN = normalize(mix(param_vs_globeTileTLnv, param_vs_globeTileBLnv, in_vs_vertexPosition.y));           ""\n"
            "    vec3 rightN = normalize(mix(param_vs_globeTileTRnv, param_vs_globeTileBRnv, in_vs_vertexPosition.y));          ""\n"
            "    vec3 midN = normalize(mix(leftN, rightN, in_vs_vertexPosition.x));                                             ""\n"
            "    float extraUp = extraUpToSphere(param_vs_objectSizes.x, in_vs_vertexPosition,                                  ""\n"
            "        param_vs_globeTileTLnv, param_vs_globeTileTRnv, param_vs_globeTileBLnv, param_vs_globeTileBRnv);           ""\n"
            "    midVertex += midN * extraUp;                                                                                   ""\n"
            "    midVertex = overY || loc31.y == 0 ? cv : midVertex;                                                            ""\n"
            "    midN = overY || loc31.y == 0 ? nv : midN;                                                                      ""\n"
            "    vec4 v = vec4(param_vs_objectSizes.z != 0.0 ? cv : midVertex, 1.0);                                            ""\n"
            "    vec3 n = param_vs_objectSizes.z != 0.0 ? nv : midN;                                                            ""\n"
            "                                                                                                                   ""\n"
            //   Get meters per unit, which is needed at both shader stages
            "    v2f_metersPerUnit = mix(param_vs_elevation_scale.x, param_vs_elevation_scale.y, in_vs_vertexTexCoords.t);      ""\n"
            "                                                                                                                   ""\n"
            //   Disable color fade effect by default
            "    v2f_colorIntensity = 1.0;                                                                                      ""\n"
            "                                                                                                                   ""\n"
            //   Process each tile layer texture coordinates (except elevation)
            "%UnrolledPerRasterLayerTexCoordsProcessingCode%                                                                    ""\n"
            "                                                                                                                   ""\n"));
        const auto& hillshadeInVertexShader_2 = QStringLiteral(
            "    v2f_hsTL = vec2(0.0);                                                                                          ""\n"
            "    v2f_hsTR = vec2(0.0);                                                                                          ""\n"
            "    v2f_hsBL = vec2(0.0);                                                                                          ""\n"
            "    v2f_hsBR = vec2(0.0);                                                                                          ""\n"
            "    v2f_hsCoordsDir = vec2(0.0);                                                                                   ""\n"
            "    v2f_hsCoords = vec2(0.0);                                                                                      ""\n");
        vertexShader.append(QStringLiteral(
            "%HillshadeInVertexShader_2%                                                                                        ""\n"
            "    if (abs(param_vs_elevation_scale.w) > 0.0)                                                                     ""\n"
            "    {                                                                                                              ""\n"
            // [0][0] - TL (0); [1][0] - T (1); [2][0] - TR (2)
            // [0][1] -  L (3); [1][1] - O (4); [2][1] -  R (5)
            // [0][2] - BL (6); [1][2] - B (7); [2][2] - BR (8)
            "        mat4 height = mat4(0.0);                                                                                   ""\n"
            "        vec2 elevationTexCoordsO;                                                                                  ""\n"
            "        calculateTextureCoordinates(                                                                               ""\n"
            "            param_vs_elevationLayer,                                                                               ""\n"
            "            elevationTexCoordsO);                                                                                  ""\n"
            "        height[1][1] = interpolatedHeight(elevationTexCoordsO);                                                    ""\n"
            "                                                                                                                   ""\n"
            "        vec2 elevationTexCoords = elevationTexCoordsO;                                                             ""\n"
            "        elevationTexCoords.t -= param_vs_elevationLayerTexelSize.y;                                                ""\n"
            "        height[1][0] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "                                                                                                                   ""\n"
            "        elevationTexCoords = elevationTexCoordsO;                                                                  ""\n"
            "        elevationTexCoords.s -= param_vs_elevationLayerTexelSize.x;                                                ""\n"
            "        height[0][1] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "                                                                                                                   ""\n"
            "        elevationTexCoords = elevationTexCoordsO;                                                                  ""\n"
            "        elevationTexCoords.t += param_vs_elevationLayerTexelSize.y;                                                ""\n"
            "        height[1][2] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "        elevationTexCoords.t += param_vs_elevationLayerTexelSize.y;                                                ""\n"
            "        height[1][3] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "                                                                                                                   ""\n"
            "        elevationTexCoords = elevationTexCoordsO;                                                                  ""\n"
            "        elevationTexCoords.s += param_vs_elevationLayerTexelSize.x;                                                ""\n"
            "        height[2][1] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "        elevationTexCoords.s += param_vs_elevationLayerTexelSize.x;                                                ""\n"
            "        height[3][1] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "                                                                                                                   ""\n"
            "        elevationTexCoords = elevationTexCoordsO;                                                                  ""\n"
            "        elevationTexCoords -= param_vs_elevationLayerTexelSize.xy;                                                 ""\n"
            "        height[0][0] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "                                                                                                                   ""\n"
            "        elevationTexCoords = elevationTexCoordsO;                                                                  ""\n"
            "        elevationTexCoords.s += param_vs_elevationLayerTexelSize.x;                                                ""\n"
            "        elevationTexCoords.t -= param_vs_elevationLayerTexelSize.y;                                                ""\n"
            "        height[2][0] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "        elevationTexCoords.s += param_vs_elevationLayerTexelSize.x;                                                ""\n"
            "        height[3][0] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "                                                                                                                   ""\n"
            "        elevationTexCoords = elevationTexCoordsO;                                                                  ""\n"
            "        elevationTexCoords.s -= param_vs_elevationLayerTexelSize.x;                                                ""\n"
            "        elevationTexCoords.t += param_vs_elevationLayerTexelSize.y;                                                ""\n"
            "        height[0][2] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "        elevationTexCoords.t += param_vs_elevationLayerTexelSize.y;                                                ""\n"
            "        height[0][3] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "                                                                                                                   ""\n"
            "        elevationTexCoords = elevationTexCoordsO;                                                                  ""\n"
            "        elevationTexCoords += param_vs_elevationLayerTexelSize.xy;                                                 ""\n"
            "        height[2][2] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "        elevationTexCoords += param_vs_elevationLayerTexelSize.xy;                                                 ""\n"
            "        height[3][3] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "        elevationTexCoords.s -= param_vs_elevationLayerTexelSize.x;                                                ""\n"
            "        height[2][3] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "        elevationTexCoords.s += param_vs_elevationLayerTexelSize.x;                                                ""\n"
            "        elevationTexCoords.t -= param_vs_elevationLayerTexelSize.y;                                                ""\n"
            "        height[3][2] = interpolatedHeight(elevationTexCoords);                                                     ""\n"
            "                                                                                                                   ""\n"
            "        float elevationScale = param_vs_elevation_scale.w * param_vs_elevation_scale.z;                            ""\n"
            "        height *= elevationScale;                                                                                  ""\n"));
        const auto& hillshadeInVertexShader_3 = QStringLiteral(
            "        vec2 hsTL;                                                                                                 ""\n"
            "        vec2 hsTR;                                                                                                 ""\n"
            "        vec2 hsBL;                                                                                                 ""\n"
            "        vec2 hsBR;                                                                                                 ""\n"
            "                                                                                                                   ""\n"
            "        hsTL.x = height[0][0] + height[0][1] + height[0][1] + height[0][2];                                        ""\n"
            "        hsTL.x -= height[2][0] + height[2][1] + height[2][1] + height[2][2];                                       ""\n"
            "        hsTL.y = height[0][2] + height[1][2] + height[1][2] + height[2][2];                                        ""\n"
            "        hsTL.y -= height[0][0] + height[1][0] + height[1][0] + height[2][0];                                       ""\n"
            "        hsTR.x = height[1][0] + height[1][1] + height[1][1] + height[1][2];                                        ""\n"
            "        hsTR.x -= height[3][0] + height[3][1] + height[3][1] + height[3][2];                                       ""\n"
            "        hsTR.y = height[1][2] + height[2][2] + height[2][2] + height[3][2];                                        ""\n"
            "        hsTR.y -= height[1][0] + height[2][0] + height[2][0] + height[3][0];                                       ""\n"
            "        hsBL.x = height[0][1] + height[0][2] + height[0][2] + height[0][3];                                        ""\n"
            "        hsBL.x -= height[2][1] + height[2][2] + height[2][2] + height[2][3];                                       ""\n"
            "        hsBL.y = height[0][3] + height[1][3] + height[1][3] + height[2][3];                                        ""\n"
            "        hsBL.y -= height[0][1] + height[1][1] + height[1][1] + height[2][1];                                       ""\n"
            "        hsBR.x = height[1][1] + height[1][2] + height[1][2] + height[1][3];                                        ""\n"
            "        hsBR.x -= height[3][1] + height[3][2] + height[3][2] + height[3][3];                                       ""\n"
            "        hsBR.y = height[1][3] + height[2][3] + height[2][3] + height[3][3];                                        ""\n"
            "        hsBR.y -= height[1][1] + height[2][1] + height[2][1] + height[3][1];                                       ""\n"
            "                                                                                                                   ""\n"
            "        float nextHeixel = in_vs_vertexTexCoords.t + 1.0 / %HeixelsPerTileSide%.0;                                 ""\n"
            "        float hf = 16.0 * param_vs_objectSizes.x / %HeixelsPerTileSide%.0;                                         ""\n"
            "        float td = hf * v2f_metersPerUnit;                                                                         ""\n"
            "        float bd = hf * mix(param_vs_elevation_scale.x, param_vs_elevation_scale.y, nextHeixel);                   ""\n"
            "                                                                                                                   ""\n"
            "        v2f_hsTL = atan(hsTL / td);                                                                                ""\n"
            "        v2f_hsTR = atan(hsTR / td);                                                                                ""\n"
            "        v2f_hsBL = atan(hsBL / bd);                                                                                ""\n"
            "        v2f_hsBR = atan(hsBR / bd);                                                                                ""\n"
            "                                                                                                                   ""\n"
            "        ivec2 bitIndex = ivec2(vertexIndex.x & 1, vertexIndex.y & 1);                                              ""\n"
            "        v2f_hsCoordsDir.x = bitIndex.x > 0 ? -1.0 : 1.0;                                                           ""\n"
            "        v2f_hsCoordsDir.y = bitIndex.y > 0 ? -1.0 : 1.0;                                                           ""\n"
            "        v2f_hsCoords = vec2(bitIndex);                                                                             ""\n");
        vertexShader.append(QStringLiteral(
            "%HillshadeInVertexShader_3%                                                                                        ""\n"
            "        float elev = height[1][1] / v2f_metersPerUnit;                                                             ""\n"
            "        bool noElev = false;                                                                                       ""\n"
            "        noElev = noElev || vertexIndex.y == 0 && (param_vs_tileCoords31.w & 2) > 0;                                ""\n"
            "        noElev = noElev || vertexIndex.x == 0 && (param_vs_tileCoords31.w & 8) > 0;                                ""\n"
            "        noElev = noElev || vertexIndex.y == %HeixelsPerTileSide% && (param_vs_tileCoords31.w & 32) > 0;            ""\n"
            "        noElev = noElev || vertexIndex.x == %HeixelsPerTileSide% && (param_vs_tileCoords31.w & 128) > 0;           ""\n"
            "        bool avgHorElev = vertexIndex.y == 0 && (param_vs_tileCoords31.w & 1) > 0;                                 ""\n"
            "        bool avgVerElev = vertexIndex.x == 0 && (param_vs_tileCoords31.w & 4) > 0;                                 ""\n"
            "        avgHorElev = avgHorElev || vertexIndex.y == %HeixelsPerTileSide% && (param_vs_tileCoords31.w & 16) > 0;    ""\n"
            "        avgVerElev = avgVerElev || vertexIndex.x == %HeixelsPerTileSide% && (param_vs_tileCoords31.w & 64) > 0;    ""\n"
            "        bool avgMidElev = avgHorElev && (vertexIndex.x & 1) == 0 || avgVerElev && (vertexIndex.y & 1) == 0;        ""\n"
            "        float ext = 0.5;                                                                                           ""\n"
            "        elev = avgMidElev ? averageHeight(height, 1, 1) / v2f_metersPerUnit + ext : elev;                          ""\n"
            "        elev = avgHorElev && (vertexIndex.x & 1) > 0 ? (averageHeight(height, 0, 1)                                ""\n"
            "            + averageHeight(height, 2, 1)) / 2.0 / v2f_metersPerUnit + ext : elev;                                 ""\n"
            "        elev = avgVerElev && (vertexIndex.y & 1) > 0 ? (averageHeight(height, 1, 0)                                ""\n"
            "            + averageHeight(height, 1, 2)) / 2.0 / v2f_metersPerUnit + ext : elev;                                 ""\n"
            "        v2f_colorIntensity = noElev ? 0.0 : 1.0;                                                                   ""\n"
            "        v.xyz += (noElev ? 0.0 : elev) * n;                                                                        ""\n"
            "    }                                                                                                              ""\n"
            "                                                                                                                   ""\n"
            "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
            //   Calculate mipmap LOD
            "    vec2 groundVertex = v.xz;                                                                                      ""\n"
            "    vec2 groundCameraToVertex = groundVertex - param_vs_groundCameraPosition;                                      ""\n"
            "    float mipmapK = log(1.0 + 10.0 * log2(1.0 + param_vs_cameraElevationAngleN));                                  ""\n"
            "    float mipmapBaseLevelEndDistance = mipmapK * param_vs_distanceFromCameraToTarget;                              ""\n"
            "    v2f_mipmapLOD = 1.0 + (length(groundCameraToVertex) - mipmapBaseLevelEndDistance)                              ""\n"
            "        / (param_vs_scaleToRetainProjectedSize * param_vs_objectSizes.x);                                          ""\n"
            "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n"
            "                                                                                                                   ""\n"));
        const auto& gridsInVertexShader_3 = QStringLiteral(            
            //   Calculate location using selected grid projection
            "    vec2 lonlat = mercMeters * M_2PI;                                                                              ""\n"
            "    lonlat.y = M_PI_2 - atan(exp(lonlat.y)) * 2.0;                                                                 ""\n"
            "    mercMeters.y = -mercMeters.y;                                                                                  ""\n"
            "    mercMeters *= M_2PI * 63.78137;                                                                                ""\n"
            "    vec4 primary;                                                                                                  ""\n"
            "    vec4 secondary;                                                                                                ""\n"
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
            "    zUTM -= vec2(param_vs_tileCoords31.w >> 12 & 63, param_vs_tileCoords31.w >> 18) + 0.5;                         ""\n"
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
            "    int gridConf = param_vs_tileCoords31.w >> 8;                                                                   ""\n"
            "    if ((gridConf & 3) == 0)                                                                                       ""\n"
            "        primary.xy = vec2(1e38);                                                                                   ""\n"
            "    else if ((gridConf & 3) == 1)                                                                                  ""\n"
            "        primary.zw = primary.xy;                                                                                   ""\n"
            "    else if ((gridConf & 3) == 3)                                                                                  ""\n"
            "        primary.xy = primary.zw;                                                                                   ""\n"
            "    if ((gridConf & 12) == 0)                                                                                      ""\n"
            "        secondary.xy = vec2(1e38);                                                                                 ""\n"
            "    else if ((gridConf & 12) == 4)                                                                                 ""\n"
            "        secondary.zw = secondary.xy;                                                                               ""\n"
            "    else if ((gridConf & 12) == 12)                                                                                ""\n"
            "        secondary.xy = secondary.zw;                                                                               ""\n"
            "                                                                                                                   ""\n"
            //   Finally output processed modified vertex
            "    v2f_primaryLocation = primary;                                                                                 ""\n"
            "    v2f_secondaryLocation = secondary;                                                                             ""\n");
        vertexShader.append(QStringLiteral(
            "%GridsInVertexShader_3%                                                                                            ""\n"
            "    v2f_position = v;                                                                                              ""\n"
            "    v2f_normal = vec4(n.xyz, 1.0);                                                                                 ""\n"
            "    v = param_vs_mPerspectiveProjectionView * v;                                                                   ""\n"
            "    gl_Position = v * param_vs_resultScale;                                                                        ""\n"
            "}                                                                                                                  ""\n"));
        const auto& vertexShader_perRasterLayerTexCoordsDeclaration = QString::fromLatin1(
            "PARAM_OUTPUT vec2 v2f_texCoordsPerLayer_%rasterLayerIndex%;                                                        ""\n");
        const auto& vertexShader_perRasterLayerParamsDeclaration = QString::fromLatin1(
            "uniform VsRasterLayerTile param_vs_rasterTileLayer_%rasterLayerIndex%;                                             ""\n");
        const auto& vertexShader_perRasterLayerTexCoordsProcessing = QString::fromLatin1(
            "    calculateTextureCoordinates(                                                                                   ""\n"
            "        param_vs_rasterTileLayer_%rasterLayerIndex%,                                                               ""\n"
            "        v2f_texCoordsPerLayer_%rasterLayerIndex%);                                                                 ""\n"
            "                                                                                                                   ""\n");
        auto fragmentShader = QStringLiteral(
            // Input data
            "%UnrolledPerRasterLayerTexCoordsDeclarationCode%                                                                   ""\n"
            "PARAM_INPUT float v2f_metersPerUnit;                                                                               ""\n"
            "PARAM_INPUT float v2f_colorIntensity;                                                                              ""\n"
            "#if TEXTURE_LOD_SUPPORTED                                                                                          ""\n"
            "    PARAM_INPUT float v2f_mipmapLOD;                                                                               ""\n"
            "#endif // TEXTURE_LOD_SUPPORTED                                                                                    ""\n");
        const auto& hillshadeInFragmentShader_1 = QStringLiteral(
            "PARAM_FLAT_INPUT highp vec2 v2f_hsTL;                                                                              ""\n"
            "PARAM_FLAT_INPUT highp vec2 v2f_hsTR;                                                                              ""\n"
            "PARAM_FLAT_INPUT highp vec2 v2f_hsBL;                                                                              ""\n"
            "PARAM_FLAT_INPUT highp vec2 v2f_hsBR;                                                                              ""\n"
            "PARAM_FLAT_INPUT highp vec2 v2f_hsCoordsDir;                                                                       ""\n"
            "PARAM_INPUT highp vec2 v2f_hsCoords;                                                                               ""\n");
        const auto& gridsInFragmentShader_1 = QStringLiteral(
            "PARAM_INPUT vec4 v2f_primaryLocation;                                                                              ""\n"
            "PARAM_INPUT vec4 v2f_secondaryLocation;                                                                            ""\n");
        fragmentShader.append(QStringLiteral(
            "%HillshadeInFragmentShader_1%                                                                                      ""\n"
            "%GridsInFragmentShader_1%                                                                                          ""\n"
            "PARAM_INPUT vec4 v2f_position;                                                                                     ""\n"
            "PARAM_INPUT vec4 v2f_normal;                                                                                       ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform lowp float param_fs_lastBatch;                                                                             ""\n"
            "uniform lowp float param_fs_blendingEnabled;                                                                       ""\n"
            "uniform lowp vec4 param_fs_backgroundColor;                                                                        ""\n"
            "uniform lowp vec4 param_fs_myLocationColor;                                                                        ""\n"
            "uniform vec4 param_fs_myLocation;                                                                                  ""\n"
            "uniform vec4 param_fs_myDirection;                                                                                 ""\n"));
        const auto& hillshadeInFragmentShader_2 = QStringLiteral(
            "uniform vec4 param_fs_hillshade;                                                                                   ""\n");
        const auto& gridsInFragmentShader_2 = QStringLiteral(
            "uniform vec4 param_fs_gridParameters;                                                                              ""\n"
            "uniform lowp vec4 param_fs_primaryGridColor;                                                                       ""\n"
            "uniform lowp vec4 param_fs_secondaryGridColor;                                                                     ""\n");
        fragmentShader.append(QStringLiteral(
            "%HillshadeInFragmentShader_2%                                                                                      ""\n"
            "%GridsInFragmentShader_2%                                                                                          ""\n"
            "uniform vec4 param_fs_worldCameraPosition;                                                                         ""\n"
            "uniform vec4 param_fs_mistConfiguration;                                                                           ""\n"
            "uniform vec4 param_fs_mistColor;                                                                                   ""\n"
            // Parameters: per-layer data
            "struct FsRasterLayerTile                                                                                           ""\n"
            "{                                                                                                                  ""\n"
            "    highp vec4 transitionPhase;                                                                                    ""\n"
            "    highp vec4 texOffsetAndScale;                                                                                  ""\n"
            "    highp float texelSize;                                                                                         ""\n"
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
            "{                                                                                                                  ""\n"));
        const auto& dynamicsInFragmentShader = QStringLiteral(
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
            "    else                                                                                                           ""\n");
        fragmentShader.append(QStringLiteral(
            "%DynamicsInFragmentShader%                                                                                         ""\n"
            "    {                                                                                                              ""\n"
            "        float fakeValue = max(texCoordsOffsetAndScale.x, 0.0) * max(transitionPhase.x, 0.0) * texelSize;           ""\n"
            "        if (min(fakeValue, 0.0) <= 0.0)                                                                            ""\n"
            "            fromTexture(sampler, texCoords, resultColor);                                                          ""\n"
            "    }                                                                                                              ""\n"
            "}                                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    lowp vec4 finalColor;                                                                                          ""\n"
            "                                                                                                                   ""\n"
            //   Calculate color of accuracy circle
            "    lowp vec4 circle = param_fs_myLocationColor;                                                                   ""\n"
            "    vec3 vMyToPos = v2f_position.xyz - param_fs_myLocation.xyz;                                                    ""\n"
            "    float dist = length(vMyToPos);                                                                                 ""\n"
            "    float fdist = min(pow(min(dist / param_fs_myLocation.w, 1.0), 100.0), 1.0);                                    ""\n"
            "    circle.a = (1.0 - fdist) * (1.0 + 2.0 * fdist) * circle.a;                                                     ""\n"
            "                                                                                                                   ""\n"
            //   Calculate color of heading sector
            "    lowp vec4 sector = param_fs_myLocationColor;                                                                   ""\n"
            "    float fdir = dot(vMyToPos / dist, param_fs_myDirection.xyz);                                                   ""\n"
            "    fdir = pow(min(max(fdir / 0.7071, 0.0), 1.0), 10.0);                                                           ""\n"
            "    sector.a = dist >= param_fs_myDirection.w ? 0.0 : fdir * (1.0 - dist / param_fs_myDirection.w);                ""\n"
            "                                                                                                                   ""\n"));
        const auto& gridsInFragmentShader_3 = QStringLiteral(
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
            "                                                                                                                   ""\n");
        fragmentShader.append(QStringLiteral(
            //   Calculate colors of grids
            "%GridsInFragmentShader_3%                                                                                          ""\n"
            //   Calculate mist color
            "    lowp vec4 mistColor = param_fs_mistColor;                                                                      ""\n"
            "    vec4 infrontPosition = v2f_position;                                                                           ""\n"
            "    infrontPosition.xz = v2f_position.xz * param_fs_mistConfiguration.xy;                                          ""\n"
            "    infrontPosition.xz = v2f_position.xz - (infrontPosition.x + infrontPosition.z) * param_fs_mistConfiguration.xy;""\n"
            "    vec4 worldCameraPosition = vec4(param_fs_worldCameraPosition.xyz, 1.0);                                        ""\n"
            "    float toFog = param_fs_mistConfiguration.z - distance(infrontPosition, worldCameraPosition);                   ""\n"
            "    float expScale = (3.0 - param_fs_mistColor.w ) / param_fs_mistConfiguration.w;                                 ""\n"
            "    float expOffset = 2.354 - param_fs_mistColor.w * 0.5;                                                          ""\n"
            "    float farMist = max(0.0, expOffset - toFog * expScale);                                                        ""\n"
            "    farMist = clamp(1.0 - 1.0 / exp(farMist * farMist), 0.0, 1.0);                                                 ""\n"
            "    float globeMist = 1.0 - dot(v2f_normal, normalize(worldCameraPosition));                                       ""\n"
            "    float mistFactor = min(param_fs_worldCameraPosition.w * 1.5, 1.0);                                             ""\n"
            "    mistColor.a = mix(globeMist * globeMist * globeMist, farMist, mistFactor * mistFactor * mistFactor);           ""\n"
            "                                                                                                                   ""\n"
            //   Mix colors of all layers.
            //   First layer is processed unconditionally, as well as its color is converted to premultiplied alpha.
            "    getTextureColor(param_fs_rasterTileLayer_0.texOffsetAndScale, param_fs_rasterTileLayer_0.transitionPhase,      ""\n"
            "        param_fs_rasterTileLayer_0.texelSize, param_fs_rasterTileLayer_0.sampler,                                  ""\n"
            "        v2f_texCoordsPerLayer_0, finalColor);                                                                      ""\n"
            "    addExtraAlpha(finalColor, param_fs_rasterTileLayer_0.opacityFactor,                                            ""\n"
            "        param_fs_rasterTileLayer_0.isPremultipliedAlpha);                                                          ""\n"
            "    lowp float firstLayerColorFactor = param_fs_rasterTileLayer_0.isPremultipliedAlpha +                           ""\n"
            "        (1.0 - param_fs_rasterTileLayer_0.isPremultipliedAlpha) * finalColor.a;                                    ""\n"
            "    finalColor *= vec4(firstLayerColorFactor, firstLayerColorFactor, firstLayerColorFactor, 1.0);                  ""\n"
            "    lowp vec4 firstColor = mix(vec4(param_fs_backgroundColor.rgb, 1.0), finalColor, finalColor.a);                 ""\n"
            "    firstColor.a = 1.0;                                                                                            ""\n"
            "    finalColor = mix(firstColor, finalColor, param_fs_blendingEnabled);                                            ""\n"
            "                                                                                                                   ""\n"
            "%UnrolledPerRasterLayerProcessingCode%                                                                             ""\n"
            "                                                                                                                   ""\n"));
        const auto& hillshadeInFragmentShader_3 = QStringLiteral(
            "    if (param_fs_lastBatch > 0.0)                                                                                  ""\n"
            "    {                                                                                                              ""\n"
            "#if __VERSION__ >= 140                                                                                             ""\n"
            "        vec2 c = v2f_hsCoords;                                                                                     ""\n"
            "        c.x = v2f_hsCoordsDir.x < 0.0 ? 1.0 - c.x : c.x;                                                           ""\n"
            "        c.y = v2f_hsCoordsDir.y < 0.0 ? 1.0 - c.y : c.y;                                                           ""\n"
            "        vec2 slopes = 2.0 * tan(mix(mix(v2f_hsTL, v2f_hsTR, c.x), mix(v2f_hsBL, v2f_hsBR, c.x), c.y));             ""\n"
            "#else                                                                                                              ""\n"
            "        vec2 slopes = 2.0 * tan(v2f_hsTL);                                                                         ""\n"
            "#endif                                                                                                             ""\n"
            "        float M_PI = 3.1415926535897932384626433832795;                                                            ""\n"
            "        float lsl = length(slopes);                                                                                ""\n"
            "        vec2 sl = slopes * param_fs_hillshade.zw;                                                                  ""\n"
            "        float hsLight = (param_fs_hillshade.x + sl.x - sl.y) / sqrt(1.0 + lsl * lsl) * (1.0 - atan(lsl) / M_PI);   ""\n"
            "        float hsAlpha = hsLight < 0.7 ? (0.7 - hsLight) / 0.7 * 0.6 : (hsLight - 0.7) / 0.3 * 0.2;                 ""\n"
            "        vec4 hsColor = hsLight < 0.7 ? vec4(0.0, 0.0, 0.0, hsAlpha) : vec4(1.0, 1.0, 1.0, hsAlpha);                ""\n"
            "        hsColor.a *= param_fs_hillshade.y;                                                                         ""\n"
            "        mixColors(finalColor, hsColor);                                                                            ""\n"
            "    }                                                                                                              ""\n");
        fragmentShader.append(QStringLiteral(
            "%HillshadeInFragmentShader_3%                                                                                      ""\n"
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
            "                                                                                                                   ""\n"));
        const auto& gridsInFragmentShader_4 = QStringLiteral(
            "    mixColors(finalColor, secondaryColor * param_fs_lastBatch);                                                    ""\n"
            "    mixColors(finalColor, primaryColor * param_fs_lastBatch);                                                      ""\n");
        fragmentShader.append(QStringLiteral(
            "%GridsInFragmentShader_4%                                                                                          ""\n"
            "    mixColors(finalColor, circle * param_fs_lastBatch);                                                            ""\n"
            "    mixColors(finalColor, sector * param_fs_lastBatch);                                                            ""\n"
            "    mixColors(finalColor, mistColor * param_fs_lastBatch);                                                         ""\n"
            "    mixColors(finalColor, vec4(param_fs_backgroundColor.rgb, 1.0 - v2f_colorIntensity) * param_fs_lastBatch);      ""\n"
            "    FRAGMENT_COLOR_OUTPUT = finalColor;                                                                            ""\n"
            "}                                                                                                                  ""\n"));
        const auto& fragmentShader_perRasterLayer = QString::fromLatin1(
            "    {                                                                                                              ""\n"
            "        lowp vec4 tc;                                                                                              ""\n"
            "        getTextureColor(param_fs_rasterTileLayer_%rasterLayerIndex%.texOffsetAndScale,                             ""\n"
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

        // Prepare vertex shader
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
        if ((programFeatures & RenderingFeatures::Hillshade) > 0)
        {

            preprocessedVertexShader.replace("%HillshadeInVertexShader_1%", hillshadeInVertexShader_1);
            preprocessedVertexShader.replace("%HillshadeInVertexShader_2%", hillshadeInVertexShader_2);
            preprocessedVertexShader.replace("%HillshadeInVertexShader_3%", hillshadeInVertexShader_3);
        }
        else
        {
            const auto emptyString = QString();
            preprocessedVertexShader.replace("%HillshadeInVertexShader_1%", emptyString);
            preprocessedVertexShader.replace("%HillshadeInVertexShader_2%", emptyString);
            preprocessedVertexShader.replace("%HillshadeInVertexShader_3%", emptyString);
        }

        if ((programFeatures & RenderingFeatures::Grids) > 0)
        {

            preprocessedVertexShader.replace("%GridsInVertexShader_1%", gridsInVertexShader_1);
            preprocessedVertexShader.replace("%GridsInVertexShader_2%", gridsInVertexShader_2);
            preprocessedVertexShader.replace("%GridsInVertexShader_3%", gridsInVertexShader_3);
        }
        else
        {
            const auto emptyString = QString();
            preprocessedVertexShader.replace("%GridsInVertexShader_1%", emptyString);
            preprocessedVertexShader.replace("%GridsInVertexShader_2%", emptyString);
            preprocessedVertexShader.replace("%GridsInVertexShader_3%", emptyString);
        }
        preprocessedVertexShader.replace("%HeixelsPerTileSide%",
            QString::number(AtlasMapRenderer::HeixelsPerTileSide - 1));
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        // Prepare fragment shader
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
        if ((programFeatures & RenderingFeatures::Hillshade) > 0)
        {
            preprocessedFragmentShader.replace("%HillshadeInFragmentShader_1%", hillshadeInFragmentShader_1);
            preprocessedFragmentShader.replace("%HillshadeInFragmentShader_2%", hillshadeInFragmentShader_2);
            preprocessedFragmentShader.replace("%HillshadeInFragmentShader_3%", hillshadeInFragmentShader_3);
        }
        else
        {
            const auto emptyString = QString();
            preprocessedFragmentShader.replace("%HillshadeInFragmentShader_1%", emptyString);
            preprocessedFragmentShader.replace("%HillshadeInFragmentShader_2%", emptyString);
            preprocessedFragmentShader.replace("%HillshadeInFragmentShader_3%", emptyString);
        }
        if ((programFeatures & RenderingFeatures::Grids) > 0)
        {
            preprocessedFragmentShader.replace("%GridsInFragmentShader_1%", gridsInFragmentShader_1);
            preprocessedFragmentShader.replace("%GridsInFragmentShader_2%", gridsInFragmentShader_2);
            preprocessedFragmentShader.replace("%GridsInFragmentShader_3%", gridsInFragmentShader_3);
            preprocessedFragmentShader.replace("%GridsInFragmentShader_4%", gridsInFragmentShader_4);
        }
        else
        {
            const auto emptyString = QString();
            preprocessedFragmentShader.replace("%GridsInFragmentShader_1%", emptyString);
            preprocessedFragmentShader.replace("%GridsInFragmentShader_2%", emptyString);
            preprocessedFragmentShader.replace("%GridsInFragmentShader_3%", emptyString);
            preprocessedFragmentShader.replace("%GridsInFragmentShader_4%", emptyString);
        }
        if ((programFeatures & RenderingFeatures::Dynamics) > 0)
            preprocessedFragmentShader.replace("%DynamicsInFragmentShader%", dynamicsInFragmentShader);
        else
            preprocessedFragmentShader.replace("%DynamicsInFragmentShader%", QString());
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        // Read precompiled shaders if available or otherwise compile them and put the binary code in cache if possible
        outRasterLayerTileProgram.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, setupOptions.pathToOpenGLShadersCache, outRasterLayerTileProgram.cacheFormat);

        if (!outRasterLayerTileProgram.binaryCache.isEmpty())
        {
            outRasterLayerTileProgram.id = getGPUAPI()->linkProgram(0, nullptr, variableLocations,
                outRasterLayerTileProgram.binaryCache, outRasterLayerTileProgram.cacheFormat, true, &variablesMap);
        }
        if (outRasterLayerTileProgram.binaryCache.isEmpty() || !outRasterLayerTileProgram.id.isValid())
        {
            const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
            if (vsId == 0)
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererMapLayersStage_OpenGL vertex shader for %d raster map layers",
                    numberOfLayersInBatch);
                return false;
            }
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
            if (outRasterLayerTileProgram.id.isValid() && !outRasterLayerTileProgram.binaryCache.isEmpty())
            {
                gpuAPI->writeProgramBinary(
                    preprocessedVertexShader,
                    preprocessedFragmentShader,
                    setupOptions.pathToOpenGLShadersCache,
                    outRasterLayerTileProgram.binaryCache,
                    outRasterLayerTileProgram.cacheFormat);
            }
        }
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
        outRasterLayerTileProgram.vs.param.mGlobeRotation,
        "param_vs_mGlobeRotation",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.targetInTilePosN,
        "param_vs_targetInTilePosN",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.objectSizes,
        "param_vs_objectSizes",
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
        outRasterLayerTileProgram.vs.param.elevationLayerTexelSize,
        "param_vs_elevationLayerTexelSize",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.tileCoords31,
        "param_vs_tileCoords31",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.globeTileTL,
        "param_vs_globeTileTL",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.globeTileTR,
        "param_vs_globeTileTR",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.globeTileBL,
        "param_vs_globeTileBL",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.globeTileBR,
        "param_vs_globeTileBR",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.globeTileTLnv,
        "param_vs_globeTileTLnv",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.globeTileTRnv,
        "param_vs_globeTileTRnv",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.globeTileBLnv,
        "param_vs_globeTileBLnv",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.globeTileBRnv,
        "param_vs_globeTileBRnv",
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
        outRasterLayerTileProgram.vs.param.elevationLayer.txOffsetAndScale,
        "param_vs_elevationLayer.txOffsetAndScale",
        GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(
        outRasterLayerTileProgram.vs.param.elevationLayerDataPlace,
        "param_vs_elevationLayerDataPlace",
        GlslVariableType::Uniform);
    if ((programFeatures & RenderingFeatures::Grids) > 0)
    {
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
    }
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
    if ((programFeatures & RenderingFeatures::Hillshade) > 0)
    {
        ok = ok && lookup->lookupLocation(
            outRasterLayerTileProgram.fs.param.hillshade,
            "param_fs_hillshade",
            GlslVariableType::Uniform);
    }
    if ((programFeatures & RenderingFeatures::Grids) > 0)
    {
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
    }
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
                layerStruct.txOffsetAndScale,
                layerStructName + ".txOffsetAndScale",
                GlslVariableType::Uniform);
        }

        // Fragment shader
        {
            auto layerStructName = QString::fromLatin1("param_fs_rasterTileLayer_%layerIndex%")
                .replace(QLatin1String("%layerIndex%"), QString::number(layerIndex));
            auto& layerStruct = outRasterLayerTileProgram.fs.param.rasterTileLayers[layerIndex];

            ok = ok && lookup->lookupLocation(
                layerStruct.transitionPhase,
                layerStructName + ".transitionPhase",
                GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(
                layerStruct.texOffsetAndScale,
                layerStructName + ".texOffsetAndScale",
                GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(
                layerStruct.texelSize,
                layerStructName + ".texelSize",
                GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(
                layerStruct.isPremultipliedAlpha,
                layerStructName + ".isPremultipliedAlpha",
                GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(
                layerStruct.opacityFactor,
                layerStructName + ".opacityFactor",
                GlslVariableType::Uniform);
            ok = ok && lookup->lookupLocation(
                layerStruct.sampler,
                layerStructName + ".sampler",
                GlslVariableType::Uniform);
        }
    }

    if (!ok)
    {
        glDeleteProgram(outRasterLayerTileProgram.id);
        GL_CHECK_RESULT;

        outRasterLayerTileProgram.id = 0;
    }

    return ok;
}

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::renderRasterLayersBatch(
    const Ref<PerTileBatchedLayers>& batch,
    AlphaChannelType& currentAlphaChannelType,
    GLname& lastUsedProgram,
    const QMap<ZoomLevel, QSet<TileId>>& elevatedTiles,
    const bool isHighDetail,
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
    GL_CHECK_PRESENT(glUniform3f);
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
    const bool withHillshade = currentState.elevationDataProvider &&
        currentState.elevationConfiguration.visualizationStyle != ElevationConfiguration::VisualizationStyle::None;
    const bool withGrids = currentState.gridConfiguration.primaryGrid || currentState.gridConfiguration.secondaryGrid;
    const auto programFeatures = (withHillshade ? static_cast<int>(RenderingFeatures::Hillshade) : 0)
        | (withGrids ? static_cast<int>(RenderingFeatures::Grids) : 0)
        | (batch->hasDynamics ? static_cast<int>(RenderingFeatures::Dynamics) : 0);
    const auto wasActivated = activateRasterLayersProgram(
        batchedLayersCount,
        programFeatures,
        elevationDataSamplerIndex,
        lastUsedProgram,
        zoomLevel);
    const auto& program = _rasterLayerTilePrograms[batchedLayersCount][programFeatures];
    const auto& vao = _rasterTileVAOs[batchedLayersCount][programFeatures];

    // Set tile coordinates
    const auto tileIdN = Utilities::normalizeTileId(batch->tileId, zoomLevel);

    const auto zoomShift = MaxZoomLevel - zoomLevel;
    const PointI tile31(tileIdN.x << zoomShift, tileIdN.y << zoomShift);
    auto tileSize31 = 1u << zoomShift;
    bool isOverX = tileSize31 + static_cast<uint32_t>(tile31.x) > INT32_MAX;
    bool isOverY = tileSize31 + static_cast<uint32_t>(tile31.y) > INT32_MAX;
    PointI nextTile31(isOverX ? INT32_MAX : tile31.x + tileSize31, isOverY ? INT32_MAX : tile31.y + tileSize31);
    PointI nextTile31N(isOverX ? 0 : nextTile31.x, isOverY ? 0 : nextTile31.y);

    // Calculate world coordinates and normal vectors of tile on globe
    const auto globeTileTLnv = internalState.mGlobeRotationPrecise * Utilities::getGlobeRadialVector(tile31);
    auto globeTileTL = globeTileTLnv * internalState.globeRadius;
    globeTileTL.y -= internalState.globeRadius;
    const auto globeTileTRnv =
        internalState.mGlobeRotationPrecise * Utilities::getGlobeRadialVector(PointI(nextTile31N.x, tile31.y));
    auto globeTileTR = globeTileTRnv * internalState.globeRadius;
    globeTileTR.y -= internalState.globeRadius;
    const auto globeTileBLnv =
        internalState.mGlobeRotationPrecise * Utilities::getGlobeRadialVector(PointI(tile31.x, nextTile31N.y));
    auto globeTileBL = globeTileBLnv * internalState.globeRadius;
    globeTileBL.y -= internalState.globeRadius;
    const auto globeTileBRnv =
        internalState.mGlobeRotationPrecise * Utilities::getGlobeRadialVector(nextTile31N);
    auto globeTileBR = globeTileBRnv * internalState.globeRadius;
    globeTileBR.y -= internalState.globeRadius;
    glUniform3f(program.vs.param.globeTileTL,
        static_cast<float>(globeTileTL.x),
        static_cast<float>(globeTileTL.y),
        static_cast<float>(globeTileTL.z));
    GL_CHECK_RESULT;
    glUniform3f(program.vs.param.globeTileTR,
        static_cast<float>(globeTileTR.x),
        static_cast<float>(globeTileTR.y),
        static_cast<float>(globeTileTR.z));
    GL_CHECK_RESULT;
    glUniform3f(program.vs.param.globeTileBL,
        static_cast<float>(globeTileBL.x),
        static_cast<float>(globeTileBL.y),
        static_cast<float>(globeTileBL.z));
    GL_CHECK_RESULT;
    glUniform3f(program.vs.param.globeTileBR,
        static_cast<float>(globeTileBR.x),
        static_cast<float>(globeTileBR.y),
        static_cast<float>(globeTileBR.z));
    GL_CHECK_RESULT;
    glUniform3f(program.vs.param.globeTileTLnv,
        static_cast<float>(globeTileTLnv.x),
        static_cast<float>(globeTileTLnv.y),
        static_cast<float>(globeTileTLnv.z));
    GL_CHECK_RESULT;
    glUniform3f(program.vs.param.globeTileTRnv,
        static_cast<float>(globeTileTRnv.x),
        static_cast<float>(globeTileTRnv.y),
        static_cast<float>(globeTileTRnv.z));
    GL_CHECK_RESULT;
    glUniform3f(program.vs.param.globeTileBLnv,
        static_cast<float>(globeTileBLnv.x),
        static_cast<float>(globeTileBLnv.y),
        static_cast<float>(globeTileBLnv.z));
    GL_CHECK_RESULT;
    glUniform3f(program.vs.param.globeTileBRnv,
        static_cast<float>(globeTileBRnv.x),
        static_cast<float>(globeTileBRnv.y),
        static_cast<float>(globeTileBRnv.z));
    GL_CHECK_RESULT;
    
    int elevatedSides = 0;
    if (!currentState.flatEarth && withElevation)
    {
        // Encode special elevation on the edges of a tile if needed
        bool isUpperVisible = true;
        bool isLeftVisible = true;
        bool isLowerVisible = true;
        bool isRightVisible = true;
        const auto cameraPosition = glm::dvec3(internalState.worldCameraPosition);
        auto planeV = glm::cross(globeTileTLnv, cameraPosition - globeTileTL);
        if (glm::length(planeV) > 0.0)
        {
            const auto planeD = glm::dot(planeV, globeTileTL);
            isUpperVisible = glm::dot(planeV, globeTileTR) < planeD;
            isLeftVisible = glm::dot(planeV, globeTileBL) > planeD;
        }
        planeV = glm::cross(globeTileBRnv, cameraPosition - globeTileBR);
        if (glm::length(planeV) > 0.0)
        {
            const auto planeD = glm::dot(planeV, globeTileBR);
            isLowerVisible = glm::dot(planeV, globeTileBL) < planeD;
            isRightVisible = glm::dot(planeV, globeTileTR) > planeD;
        }
        const auto upperTileIdN = Utilities::normalizeTileId(TileId::fromXY(tileIdN.x, tileIdN.y - 1), zoomLevel);
        const auto leftTileIdN = Utilities::normalizeTileId(TileId::fromXY(tileIdN.x - 1, tileIdN.y), zoomLevel);
        const auto lowerTileIdN = Utilities::normalizeTileId(TileId::fromXY(tileIdN.x, tileIdN.y + 1), zoomLevel);
        const auto rightTileIdN = Utilities::normalizeTileId(TileId::fromXY(tileIdN.x + 1, tileIdN.y), zoomLevel);
        elevatedSides |= isUpperVisible ? (!isNearElevatedTile(
            PointI(0, -1), upperTileIdN, elevatedTiles, isHighDetail, zoomLevel) ? 2 : 0) : (isNearElevatedTile(
                PointI(0, -1), upperTileIdN, elevatedTiles, isHighDetail, zoomLevel) ? 0 : 1);
        elevatedSides |= isLeftVisible ? (!isNearElevatedTile(
            PointI(-1, 0), leftTileIdN, elevatedTiles, isHighDetail, zoomLevel) ? 8 : 0) : (isNearElevatedTile(
                PointI(-1, 0), leftTileIdN, elevatedTiles, isHighDetail, zoomLevel) ? 0 : 4);
        elevatedSides |= isLowerVisible ? (!isNearElevatedTile(
            PointI(0, 1), lowerTileIdN, elevatedTiles, isHighDetail, zoomLevel) ? 32 : 0) : (isNearElevatedTile(
                PointI(0, 1), lowerTileIdN, elevatedTiles, isHighDetail, zoomLevel) ? 0 : 16);
        elevatedSides |= isRightVisible ? (!isNearElevatedTile(
            PointI(1, 0), rightTileIdN, elevatedTiles, isHighDetail, zoomLevel) ? 128 : 0) : (isNearElevatedTile(
                PointI(1, 0), rightTileIdN, elevatedTiles, isHighDetail, zoomLevel) ? 0 : 64);
    }

    int primaryZoom = 0;
    int secondaryZoom = 0;
    int zone = 0;

    if (withGrids)
    {
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
        primaryZoom = zoom > maxZoomForFloat.x ? (zoom > maxZoomForMixed.x ? 3 : 2) : (zoom < minZoom.x ? 0 : 1);
        secondaryZoom = zoom > maxZoomForFloat.y ? (zoom > maxZoomForMixed.y ? 3 : 2) : (zoom < minZoom.y ? 0 : 1);
        zone = Utilities::getCodedZoneUTM(currentState.target31);

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
    }

    glUniform4i(program.vs.param.tileCoords31,
        tile31.x,
        tile31.y,
        zoomShift < 31 ? 1 << zoomShift : INT32_MAX,
        zone << 12 | secondaryZoom << 10 | primaryZoom << 8 | elevatedSides);
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
    bool useElevation = false;
    const auto elevationResources = batch->elevationResourcesInGPU;
    auto tileSize = static_cast<double>(AtlasMapRenderer::TileSize3D * (1 << currentState.zoomLevel - zoomLevel));
    const auto upperMetersPerUnit = currentState.flatEarth ?
        Utilities::getMetersPerTileUnit(zoomLevel, tileIdN.y, tileSize) : internalState.metersPerUnit;
    const auto lowerMetersPerUnit = currentState.flatEarth ?
        Utilities::getMetersPerTileUnit(zoomLevel, tileIdN.y + 1, tileSize) : internalState.metersPerUnit;
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
        useElevation = true;
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
        if (useElevation)
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
                    elevationDataSamplerIndex,
                    withHillshade);
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

                glUniform4f(perTile_vs.txOffsetAndScale,
                    texCoordsOffset.x,
                    texCoordsOffset.y,
                    texCoordsScale.x,
                    texCoordsScale.y);
                GL_CHECK_RESULT;

                glUniform4f(perTile_fs.texOffsetAndScale,
                    texCoordsOffset.x,
                    texCoordsOffset.y,
                    texCoordsScale.x,
                    texCoordsScale.y);
                GL_CHECK_RESULT;
            }
            else // if (batchedResourceInGPU->resourceInGPU->type == GPUAPI::ResourceInGPU::Type::Texture)
            {
                const auto& texture = std::static_pointer_cast<const GPUAPI::TextureInGPU>(batchedResourceInGPU->resourceInGPU);

                glUniform4f(perTile_vs.txOffsetAndScale,
                    batchedResourceInGPU->texCoordsOffset.x,
                    batchedResourceInGPU->texCoordsOffset.y,
                    batchedResourceInGPU->texCoordsScale.x,
                    batchedResourceInGPU->texCoordsScale.y);
                GL_CHECK_RESULT;

                glUniform4f(perTile_fs.texOffsetAndScale,
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
    const int programFeatures,
    const int elevationDataSamplerIndex,
    GLname& lastUsedProgram,
    const ZoomLevel zoomLevel)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniformMatrix3fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform1i);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform2fv);
    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glUniform4fv);

    const auto& currentConfiguration = getCurrentConfiguration();
    const auto& internalState = getInternalState();

    const bool withHillshade = currentState.elevationDataProvider &&
        currentState.elevationConfiguration.visualizationStyle != ElevationConfiguration::VisualizationStyle::None;
    const bool withGrids = currentState.gridConfiguration.primaryGrid || currentState.gridConfiguration.secondaryGrid;
    const auto& program = _rasterLayerTilePrograms[numberOfLayersInBatch][programFeatures];
    const auto& vao = _rasterTileVAOs[numberOfLayersInBatch][programFeatures];

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

    // Set globe positioning matrix
    glUniformMatrix3fv(program.vs.param.mGlobeRotation,
        1, GL_FALSE, glm::value_ptr(internalState.mGlobeRotationWithRadius));
    GL_CHECK_RESULT;

    // Set center offset
    PointF offsetInTileN;
    Utilities::getTileId(currentState.target31, zoomLevel, &offsetInTileN);
    glUniform2f(program.vs.param.targetInTilePosN, offsetInTileN.x, offsetInTileN.y);
    GL_CHECK_RESULT;

    // Set tile and globe sizes
    glUniform3f(program.vs.param.objectSizes,
        AtlasMapRenderer::TileSize3D * (1 << currentState.zoomLevel - zoomLevel),
        static_cast<float>(internalState.globeRadius),
        currentState.flatEarth ? -1.0f : (currentState.zoomLevel < ZoomLevel9 ? 1.0f : 0.0f));
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
    float hillshadeAlpha = 0.0f;
    if (withHillshade)
    {
        hillshadeAlpha = currentState.elevationConfiguration.visualizationAlpha;
        const auto zenith = glm::radians(currentState.elevationConfiguration.hillshadeSunAngle);
        const auto cosZenith = qCos(zenith);
        const auto azimuth = M_PI - glm::radians(currentState.elevationConfiguration.hillshadeSunAzimuth);
        glUniform4f(program.fs.param.hillshade,
            qSin(zenith),
            hillshadeAlpha,
            qSin(azimuth) * cosZenith,
            qCos(azimuth) * cosZenith);
        GL_CHECK_RESULT;
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
        myLocation.y,
        myLocation.z,
        myLocationRadius);
    GL_CHECK_RESULT;

    // Set direction and radius of heading sector
    glUniform4f(program.fs.param.myDirection,
        headingDirection.x,
        headingDirection.y,
        headingDirection.z,
        headingRadius);
    GL_CHECK_RESULT;

    // Set parameters for grids
    if (withGrids)
    {
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
    }

    // Set camera position for mist calculations
    glUniform4f(program.fs.param.worldCameraPosition,
        internalState.worldCameraPosition.x,
        internalState.worldCameraPosition.y,
        internalState.worldCameraPosition.z,
        internalState.factorOfDistance);
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
        currentState.flatEarth ? 1.0f : internalState.fogShiftFactor);
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

    for (auto& rasterLayerTilePrograms : _rasterLayerTilePrograms)
    {
        for (int programFeature = 0; programFeature <= RenderingFeatures::All; programFeature++)
        {
            if (!rasterLayerTilePrograms[programFeature].id.isValid())
                continue;

            if (!gpuContextLost)
            {
                glDeleteProgram(rasterLayerTilePrograms[programFeature].id);
                GL_CHECK_RESULT;
            }
            rasterLayerTilePrograms[programFeature].id = 0;
        }
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
    GL_CHECK_PRESENT(glDisableVertexAttribArray);
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
            pTileIndices[0] = p3;
            pTileIndices[1] = p0;
            pTileIndices[2] = p1;

            // Triangle 1
            pTileIndices[3] = p2;
            pTileIndices[4] = p3;
            pTileIndices[5] = p1;
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
        for (int programFeatures = 0; programFeatures <= RenderingFeatures::All; programFeatures++)
        {
            auto& rasterTileVAO = _rasterTileVAOs[numberOfLayersInBatch][programFeatures];
            const auto& rasterLayerTileProgram =
                constOf(_rasterLayerTilePrograms)[numberOfLayersInBatch][programFeatures];

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

            glDisableVertexAttribArray(*rasterLayerTileProgram.vs.in.vertexTexCoords);
            GL_CHECK_RESULT;
            glDisableVertexAttribArray(*rasterLayerTileProgram.vs.in.vertexPosition);
            GL_CHECK_RESULT;
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            GL_CHECK_RESULT;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            GL_CHECK_RESULT;
        }
    }

    _rasterTileIndicesCount = indicesCount;

    delete[] pVertices;
    delete[] pIndices;
}

void OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::releaseRasterTile(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);

    for (auto& rasterTileVAOs : _rasterTileVAOs)
    {
        for (int programFeatures = 0; programFeatures <= RenderingFeatures::All; programFeatures++)
        {
            if (rasterTileVAOs[programFeatures].isValid())
            {
                gpuAPI->releaseVAO(rasterTileVAOs[programFeatures], gpuContextLost);
                rasterTileVAOs[programFeatures].reset();
            }
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
    const int elevationDataSamplerIndex,
    const bool withHillshade)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glBindTexture);

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

        glUniform4f(perTile_vs.txOffsetAndScale,
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

        glUniform4f(perTile_vs.txOffsetAndScale,
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

bool OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::isNearElevatedTile(
    const PointI& offset,
    const TileId tileIdN,
    const QMap<ZoomLevel, QSet<TileId>>& elevatedTiles,
    const bool isHighDetail,
    const ZoomLevel zoomLevel) const
{
    if (elevatedTiles.value(zoomLevel).contains(tileIdN))
        return true;

    if (isHighDetail)
        return false;

    const auto prevZoom = static_cast<int32_t>(zoomLevel) + 1;
    const auto& itElevatedTileset = elevatedTiles.constFind(static_cast<ZoomLevel>(prevZoom));
    if (itElevatedTileset != elevatedTiles.constEnd())
    {
        const auto& elevatedTileset = itElevatedTileset.value();
        PointI tileId(tileIdN.x << 1, tileIdN.y << 1);
        const auto firstTileId = TileId::fromXY(
            tileId.x + (offset.x != 0 ? (offset.x > 0 ? 0 : 1) : 0),
            tileId.y + (offset.y != 0 ? (offset.y > 0 ? 0 : 1) : 0));
        const auto secondTileId = TileId::fromXY(
            tileId.x + (offset.x != 0 ? (offset.x > 0 ? 0 : 1) : 1),
            tileId.y + (offset.y != 0 ? (offset.y > 0 ? 0 : 1) : 1));
        if (elevatedTileset.contains(firstTileId) || elevatedTileset.contains(secondTileId))
            return true;
    }

    return false;
}

QList< OsmAnd::Ref<OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::PerTileBatchedLayers> >
OsmAnd::AtlasMapRendererMapLayersStage_OpenGL::batchLayersByTiles(
    const QVector<TileId>& tiles,
    const QSet<TileId>& visibleTilesSet,
    const ZoomLevel zoomLevel,
    QSet<TileId>& elevatedTileset,
    bool& withElevation)
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

    bool haveElevation = false;
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
        batch->hasDynamics = false;

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

                            if (gpuResource->dateTimeNext - gpuResource->dateTimePrevious > 0)
                                batch->hasDynamics = true;
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
                    if (exactMatchGpuResource->dateTimeNext - exactMatchGpuResource->dateTimePrevious > 0)
                        batch->hasDynamics = true;
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

                                        if (gpuResource->dateTimeNext - gpuResource->dateTimePrevious > 0)
                                            batch->hasDynamics = true;
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

                                if (gpuResource->dateTimeNext - gpuResource->dateTimePrevious > 0)
                                    batch->hasDynamics = true;

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
                bool hasDynamics = batch->hasDynamics;
                batch = new PerTileBatchedLayers(tileId, false);
                perTileBatchedLayers.push_back(batch);
                batch->elevationResourcesInGPU = elevationResources;
                batch->hasDynamics = hasDynamics;
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

        if (!elevationResources->isEmpty())
        {
            haveElevation = true;
            elevatedTileset.insert(tileIdN);
        }
    }

    withElevation = withElevation && haveElevation;

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
