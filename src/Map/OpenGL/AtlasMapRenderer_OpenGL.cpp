#include "AtlasMapRenderer_OpenGL.h"

#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "QtExtensions.h"
#include <QtGlobal>
#include <QtNumeric>
#include <QtMath>
#include <QThread>
#include <QLinkedList>

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include "restore_internal_warnings.h"

#include "AtlasMapRenderer_Metrics.h"
#include "AtlasMapRendererConfiguration.h"
#include "AtlasMapRendererSkyStage_OpenGL.h"
#include "AtlasMapRendererMapLayersStage_OpenGL.h"
#include "AtlasMapRendererSymbolsStage_OpenGL.h"
#include "AtlasMapRendererDebugStage_OpenGL.h"
#include "IMapRenderer.h"
#include "IMapTiledDataProvider.h"
#include "IRasterMapLayerProvider.h"
#include "IMapElevationDataProvider.h"
#include "Logging.h"
#include "Stopwatch.h"
#include "GlmExtensions.h"
#include "Utilities.h"

#include "OpenGL/Utilities_OpenGL.h"

// Set camera's near depth limit
const float OsmAnd::AtlasMapRenderer_OpenGL::_zNear = 0.1f;
// Set average radius of Earth
const double OsmAnd::AtlasMapRenderer_OpenGL::_radius = 6371e3;
// Set minimum earth angle for advanced horizon (non-realistic)
const double OsmAnd::AtlasMapRenderer_OpenGL::_minimumAngleForAdvancedHorizon = M_PI / 15.0;
// Set distance-per-angle factor for smooth horizon transition beyond minimum earth angle
const double OsmAnd::AtlasMapRenderer_OpenGL::_distancePerAngleFactor = 
    _minimumAngleForAdvancedHorizon / _radius / (1.0 / qCos(_minimumAngleForAdvancedHorizon) - 1.0);
// Set latitude limit for the camera where realistic horizon will be displayed
const double OsmAnd::AtlasMapRenderer_OpenGL::_maximumAbsoluteLatitudeForRealHorizon = 70.0;
// Set minimum height of visible sky for colouring
const double OsmAnd::AtlasMapRenderer_OpenGL::_minimumSkyHeightInKilometers = 8.0;
// Set maximum height of terrain to render
const double OsmAnd::AtlasMapRenderer_OpenGL::_maximumHeightFromSeaLevelInMeters = 10000.0;
// Set maximum depth of terrain to render
const double OsmAnd::AtlasMapRenderer_OpenGL::_maximumDepthFromSeaLevelInMeters = 12000.0;
// Set minimal distance factor for tiles of each detail level (SQRT(2) * 2)
const double OsmAnd::AtlasMapRenderer_OpenGL::_detailDistanceFactor = 2.8284271247461900976033774484194;
// Set invalid value for elevation of terrain
const float OsmAnd::AtlasMapRenderer_OpenGL::_invalidElevationValue = -20000.0f;

OsmAnd::AtlasMapRenderer_OpenGL::AtlasMapRenderer_OpenGL(GPUAPI_OpenGL* const gpuAPI_)
    : AtlasMapRenderer(
        gpuAPI_,
        std::unique_ptr<const MapRendererConfiguration>(new AtlasMapRendererConfiguration()),
        std::unique_ptr<const MapRendererDebugSettings>(new MapRendererDebugSettings()))
    , terrainDepthBuffer(_terrainDepthBuffer)
    , terrainDepthBufferSize(_terrainDepthBufferSize)
{
}

OsmAnd::AtlasMapRenderer_OpenGL::~AtlasMapRenderer_OpenGL()
{
}

bool OsmAnd::AtlasMapRenderer_OpenGL::doInitializeRendering()
{
    GL_CHECK_PRESENT(glClearColor);

    const auto gpuAPI = getGPUAPI();

    bool ok;

    ok = AtlasMapRenderer::doInitializeRendering();
    if (!ok)
        return false;

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    GL_CHECK_RESULT;

    gpuAPI->glClearDepth_wrapper(1.0f);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::doRenderFrame(IMapRenderer_Metrics::Metric_renderFrame* const metric_)
{
    const auto gpuAPI = getGPUAPI();

    bool ok = true;

    const auto metric = dynamic_cast<AtlasMapRenderer_Metrics::Metric_renderFrame*>(metric_);

    GL_PUSH_GROUP_MARKER(QLatin1String("OsmAndCore"));

    GL_CHECK_PRESENT(glViewport);
    GL_CHECK_PRESENT(glEnable);
    GL_CHECK_PRESENT(glDisable);
    GL_CHECK_PRESENT(glBlendFunc);
    GL_CHECK_PRESENT(glClear);
    GL_CHECK_PRESENT(glClearColor);

    _debugStage->clear();

    // Setup viewport
    glViewport(_internalState.glmViewport[0], _internalState.glmViewport[1], _internalState.glmViewport[2], _internalState.glmViewport[3]);
    GL_CHECK_RESULT;

    // Set background color
    glClearColor(currentState.backgroundColor.r, currentState.backgroundColor.g, currentState.backgroundColor.b, 1.0f);
    GL_CHECK_RESULT;

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_RESULT;

    // Unbind any textures from texture samplers
    for (int samplerIndex = 0; samplerIndex < gpuAPI->maxTextureUnitsCombined; samplerIndex++)
    {
        glActiveTexture(GL_TEXTURE0 + samplerIndex);
        GL_CHECK_RESULT;

        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_RESULT;
    }

    // Turn off blending for sky
    glDisable(GL_BLEND);
    GL_CHECK_RESULT;

    // Turn off depth testing and writing to depth buffer for sky stage since sky is on top of everything
    glDisable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;

//    // Turn off writing to the color buffer for the sky (depth only)
//    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//    GL_CHECK_RESULT;

    // Render the sky (depth only) -- ???
    // Render the sky
    if (!currentDebugSettings->disableSkyStage)
    {
        Stopwatch skyStageStopwatch(metric != nullptr);
        if (!_skyStage->render(metric))
            ok = false;
        if (metric)
            metric->elapsedTimeForSkyStage = skyStageStopwatch.elapsed();
    }

    // Turn on depth testing prior to raster map stage and further stages
    glEnable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;

    // Change depth test function prior to raster map stage and further stages
    glDepthFunc(GL_LEQUAL);
    GL_CHECK_RESULT;

//    // Enable writing to color buffer
//    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//    GL_CHECK_RESULT;

    // Raster map stage is rendered without blending, since it's done in fragment shader
    if (!currentDebugSettings->disableMapLayersStage)
    {

        // Don't render the inside of map surface
        glEnable(GL_CULL_FACE);
        GL_CHECK_RESULT;

        glCullFace(GL_FRONT);
        GL_CHECK_RESULT;

        Stopwatch mapLayersStageStopwatch(metric != nullptr);
        if (!_mapLayersStage->render(metric))
            ok = false;
        if (metric)
            metric->elapsedTimeForMapLayersStage = mapLayersStageStopwatch.elapsed();

        // Continue to render everything
        glDisable(GL_CULL_FACE);
        GL_CHECK_RESULT;

    }
    /* Disable depth buffer reading
    // Capture terrain depth buffer
    if (_terrainDepthBuffer.size() > 0 && _terrainDepthBufferSize == currentState.windowSize)
    {
        Stopwatch terrainDepthBufferCaptureStopwatch(metric != nullptr);

        gpuAPI->readFramebufferDepth(0, 0, _terrainDepthBufferSize.x, _terrainDepthBufferSize.y, _terrainDepthBuffer);
        GL_CHECK_RESULT;

        if (metric)
            metric->elapsedTimeForTerrainDepthBufferCapture = terrainDepthBufferCaptureStopwatch.elapsed();
    }
    */
    // Turn on blending since now objects with transparency are going to be rendered
    glEnable(GL_BLEND);
    GL_CHECK_RESULT;

    // Set premultiplied alpha color blending
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Render map symbols without writing depth buffer, since symbols use own sorting and intersection checking
    if (!currentDebugSettings->disableSymbolsStage && !qFuzzyIsNull(currentState.symbolsOpacity))
    {
        Stopwatch symbolsStageStopwatch(metric != nullptr);

        glDepthMask(GL_FALSE);
        GL_CHECK_RESULT;

        if (!_symbolsStage->render(metric))
            ok = false;

        glDepthMask(GL_TRUE);
        GL_CHECK_RESULT;

        if (metric)
            metric->elapsedTimeForSymbolsStage = symbolsStageStopwatch.elapsed();
    }

    // Restore straight color blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_RESULT;

    //TODO: render special fog object some day

    // Render debug stage
    Stopwatch debugStageStopwatch(metric != nullptr);
    if (currentDebugSettings->debugStageEnabled)
    {
        glDisable(GL_DEPTH_TEST);
        GL_CHECK_RESULT;

        if (!_debugStage->render(metric))
            ok = false;

        glEnable(GL_DEPTH_TEST);
        GL_CHECK_RESULT;
    }
    if (metric)
        metric->elapsedTimeForDebugStage = debugStageStopwatch.elapsed();

    // Turn off blending
    glDisable(GL_BLEND);
    GL_CHECK_RESULT;

    GL_POP_GROUP_MARKER;

    return ok;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::doReleaseRendering(bool gpuContextLost)
{
    return AtlasMapRenderer::doReleaseRendering(gpuContextLost);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::handleStateChange(const MapRendererState& state, MapRendererStateChanges mask)
{
    const auto gpuAPI = getGPUAPI();

    bool ok = AtlasMapRenderer::handleStateChange(state, mask);

    if (mask.isSet(MapRendererStateChange::WindowSize))
    {
        const auto depthBufferSize = state.windowSize.x * state.windowSize.y * gpuAPI->framebufferDepthBytes;
        if (depthBufferSize != _terrainDepthBuffer.size())
        {
            _terrainDepthBuffer.resize(depthBufferSize);
            _terrainDepthBufferSize = state.windowSize;
        }
    }

    return ok;
}

void OsmAnd::AtlasMapRenderer_OpenGL::onValidateResourcesOfType(const MapRendererResourceType type)
{
    AtlasMapRenderer::onValidateResourcesOfType(type);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::updateInternalState(
    MapRendererInternalState& outInternalState_,
    const MapRendererState& state,
    const MapRendererConfiguration& configuration_,
    const bool skipTiles /*=false*/, const bool sortTiles /*=false*/) const
{
    bool ok;
    ok = AtlasMapRenderer::updateInternalState(outInternalState_, state, configuration_);
    if (!ok)
        return false;

    const auto internalState = static_cast<InternalState*>(&outInternalState_);
    const auto configuration = static_cast<const AtlasMapRendererConfiguration*>(&configuration_);

    // Prepare values for projection matrix
    const auto viewportWidth = state.viewport.width();
    const auto viewportHeight = state.viewport.height();
    if (viewportWidth == 0 || viewportHeight == 0)
        return false;
    internalState->glmViewport = glm::vec4(
        state.viewport.left(),
        state.windowSize.y - state.viewport.bottom(),
        state.viewport.width(),
        state.viewport.height());
    internalState->aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    internalState->fovInRadians = qDegreesToRadians(state.fieldOfView);
    internalState->projectionPlaneHalfHeight = _zNear * qTan(internalState->fovInRadians);
    internalState->projectionPlaneHalfWidth = internalState->projectionPlaneHalfHeight * internalState->aspectRatio;

    // Setup perspective projection with fake Z-far plane
    internalState->mPerspectiveProjection = glm::frustum(
        -internalState->projectionPlaneHalfWidth, internalState->projectionPlaneHalfWidth,
        -internalState->projectionPlaneHalfHeight, internalState->projectionPlaneHalfHeight,
        _zNear, 1000.0f);

    // Calculate distance from camera to target based on visual zoom and visual zoom shift
    internalState->tileOnScreenScaleFactor = state.visualZoom * (1.0f + state.visualZoomShift);
    internalState->referenceTileSizeOnScreenInPixels = configuration->referenceTileSizeOnScreenInPixels;
    internalState->distanceFromCameraToTarget = Utilities_OpenGL_Common::calculateCameraDistance(
        internalState->mPerspectiveProjection,
        state.viewport,
        TileSize3D / 2.0f,
        internalState->referenceTileSizeOnScreenInPixels / 2.0f,
        internalState->tileOnScreenScaleFactor);
    const auto elevationAngleInRadians = qDegreesToRadians(static_cast<double>(state.elevationAngle));
    const auto elevationSine = qSin(elevationAngleInRadians);
    const auto elevationCosine = qCos(elevationAngleInRadians);
    const auto elevationTangent = elevationSine / elevationCosine;
    internalState->groundDistanceFromCameraToTarget = internalState->distanceFromCameraToTarget * elevationCosine;
    internalState->distanceFromCameraToGround = internalState->distanceFromCameraToTarget * elevationSine;    
    const auto distanceFromCameraToTargetWithNoVisualScale = Utilities_OpenGL_Common::calculateCameraDistance(
        internalState->mPerspectiveProjection,
        state.viewport,
        TileSize3D / 2.0f,
        internalState->referenceTileSizeOnScreenInPixels / 2.0f,
        1.0f);
    internalState->scaleToRetainProjectedSize =
        internalState->distanceFromCameraToTarget / distanceFromCameraToTargetWithNoVisualScale;
    internalState->pixelInWorldProjectionScale = static_cast<float>(AtlasMapRenderer::TileSize3D)
        / (internalState->referenceTileSizeOnScreenInPixels*internalState->tileOnScreenScaleFactor);

    // Setup camera
    internalState->mDistance = glm::translate(glm::vec3(0.0f, 0.0f, -internalState->distanceFromCameraToTarget));
    internalState->mElevation = glm::rotate(glm::radians(state.elevationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    internalState->mAzimuth = glm::rotate(glm::radians(state.azimuth), glm::vec3(0.0f, 1.0f, 0.0f));
    internalState->mCameraView = internalState->mDistance * internalState->mElevation * internalState->mAzimuth;

    // Get inverse camera
    internalState->mDistanceInv = glm::translate(glm::vec3(0.0f, 0.0f, internalState->distanceFromCameraToTarget));
    internalState->mElevationInv = glm::rotate(glm::radians(-state.elevationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    internalState->mAzimuthInv = glm::rotate(glm::radians(-state.azimuth), glm::vec3(0.0f, 1.0f, 0.0f));
    internalState->mCameraViewInv = internalState->mAzimuthInv * internalState->mElevationInv * internalState->mDistanceInv;

    // Get camera positions
    internalState->worldCameraPosition = (internalState->mCameraViewInv * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz();
    internalState->groundCameraPosition = internalState->worldCameraPosition.xz();

    // Get camera global coordinates and height
    PointD groundPos(
        internalState->groundCameraPosition.x / TileSize3D + internalState->targetInTileOffsetN.x,
        internalState->groundCameraPosition.y / TileSize3D + internalState->targetInTileOffsetN.y);
    PointD offsetInTileN(groundPos.x - floor(groundPos.x), groundPos.y - floor(groundPos.y));
    PointI64 tileId(floor(groundPos.x) + internalState->targetTileId.x,
        floor(groundPos.y) + internalState->targetTileId.y);
    const auto tileIdN = Utilities::normalizeCoordinates(tileId, state.zoomLevel);
    groundPos.x = static_cast<double>(tileIdN.x) + offsetInTileN.x;
    groundPos.y = static_cast<double>(tileIdN.y) + offsetInTileN.y;
    const auto metersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, groundPos.y, TileSize3D);
    internalState->metersPerUnit = metersPerUnit;
    internalState->distanceFromCameraToGroundInMeters =
        qMax(0.0, static_cast<double>(internalState->distanceFromCameraToGround) * metersPerUnit);
    internalState->cameraCoordinates = LatLon(Utilities::getLatitudeFromTile(state.zoomLevel, groundPos.y),
        Utilities::getLongitudeFromTile(state.zoomLevel, groundPos.x));

    // Calculate distance to horizon
    const auto inglobeAngle = qAcos(_radius / (_radius + internalState->distanceFromCameraToGroundInMeters));
    const auto referenceDistance = _radius * inglobeAngle / metersPerUnit;
    double groundDistanceToHorizon;
    if (inglobeAngle < _minimumAngleForAdvancedHorizon)
    {
        // Get real distance to horizon
        groundDistanceToHorizon =
            getRealDistanceToHorizon(*internalState, state, groundPos, inglobeAngle, referenceDistance);
    }
    else
    {
        // Use advanced horizon at great heights
        const auto lastDistanceToHorizon = getRealDistanceToHorizon(*internalState, state,
            groundPos, _minimumAngleForAdvancedHorizon, referenceDistance);
        const auto lastDistanceToHorizonInMeters = _radius * _minimumAngleForAdvancedHorizon;
        const auto borderMetersPerUnit = lastDistanceToHorizonInMeters / lastDistanceToHorizon;
        const auto transFactor = qMin(1.0, (inglobeAngle / _minimumAngleForAdvancedHorizon - 1.0) * 2.0);
        const auto smoothMetersPerUnit = metersPerUnit * transFactor + borderMetersPerUnit * (1.0 - transFactor);
        groundDistanceToHorizon =
            _radius * _distancePerAngleFactor * internalState->distanceFromCameraToGroundInMeters
            / smoothMetersPerUnit;
    }   

    // Get the farthest edge for terrain to render
    const auto distanceFromTargetToHorizon = static_cast<float>(groundDistanceToHorizon -
        static_cast<double>(internalState->groundDistanceFromCameraToTarget));
    const auto farEnd = qMin(distanceFromTargetToHorizon,
        state.visibleDistance * internalState->scaleToRetainProjectedSize);
    const auto deepEnd = qMin(static_cast<float>(_maximumDepthFromSeaLevelInMeters / metersPerUnit),
        state.visibleDistance) * internalState->scaleToRetainProjectedSize;
    const auto additionalDistanceToZFar =
        qMax(static_cast<double>(farEnd) * elevationCosine, static_cast<double>(deepEnd) * elevationSine);
    auto zFar = static_cast<double>(internalState->distanceFromCameraToTarget) + additionalDistanceToZFar;
    internalState->zFar = static_cast<float>(zFar);
    internalState->zNear = _zNear;

    // Calculate skyplane position
    internalState->skyShift = static_cast<float>(additionalDistanceToZFar * elevationTangent);

    // Calculate approximate height of the visible sky
    internalState->skyHeightInKilometers = static_cast<float>(
        (internalState->distanceFromCameraToGroundInMeters / 1000.0 + _minimumSkyHeightInKilometers) *
        qTan(internalState->fovInRadians));

    // Calculate distance for tiles of high detail
    const auto distanceToScreenTop = internalState->distanceFromCameraToTarget *
        static_cast<float>(qSin(internalState->fovInRadians) /
        qMax(0.01, qSin(elevationAngleInRadians - internalState->fovInRadians)));
    const auto visibleDistance = (distanceToScreenTop < farEnd ? distanceToScreenTop :
        static_cast<float>((zFar - internalState->distanceFromCameraToTarget) / elevationCosine)) /
        internalState->scaleToRetainProjectedSize;
    const auto minDistanceGap = static_cast<float>(_detailDistanceFactor * TileSize3D);
    internalState->zLowerDetail = state.detailedDistance < visibleDistance - minDistanceGap ?
        internalState->distanceFromCameraToTarget / internalState->scaleToRetainProjectedSize + static_cast<float>(
        qMax(0.0, static_cast<double>(state.detailedDistance) * elevationCosine)) :
        internalState->zFar;

    // Recalculate perspective projection
    internalState->mPerspectiveProjection = glm::frustum(
        -internalState->projectionPlaneHalfWidth, internalState->projectionPlaneHalfWidth,
        -internalState->projectionPlaneHalfHeight, internalState->projectionPlaneHalfHeight,
        _zNear, internalState->zFar);
    internalState->mPerspectiveProjectionInv = glm::inverse(internalState->mPerspectiveProjection);

    // Calculate orthographic projection
    const auto viewportBottom = state.windowSize.y - state.viewport.bottom();
    internalState->mOrthographicProjection = glm::ortho(
        static_cast<float>(state.viewport.left()), static_cast<float>(state.viewport.right()),
        static_cast<float>(viewportBottom) /*bottom*/, static_cast<float>(viewportBottom + viewportHeight) /*top*/,
        _zNear, internalState->zFar);

    // Convenience precalculations
    internalState->mPerspectiveProjectionView = internalState->mPerspectiveProjection * internalState->mCameraView;

    // Calculate skyplane size
    float zSkyplaneK = internalState->zFar / _zNear;
    internalState->skyplaneSize.x = zSkyplaneK * internalState->projectionPlaneHalfWidth;
    internalState->skyplaneSize.y = zSkyplaneK * internalState->projectionPlaneHalfHeight;

    // Determine skyline position and fog parameters    
    const auto horizonShift = static_cast<float>(zFar * qTan(qMax(0.0, elevationAngleInRadians - inglobeAngle)));
    internalState->skyLine = qMax(0.0f, horizonShift - internalState->skyShift) / internalState->skyplaneSize.y;

    internalState->distanceFromCameraToFog =
        qSqrt(internalState->skyShift * internalState->skyShift + internalState->zFar * internalState->zFar);
    internalState->distanceFromTargetToFog = static_cast<float>(additionalDistanceToZFar);
    internalState->fogShiftFactor = qBound(0.0f, (internalState->skyShift - horizonShift) / TileSize3D, 1.0f);


    // Update frustum
    updateFrustum(internalState, state);

    // Compute visible tileset
    if (!skipTiles)
        computeVisibleTileset(internalState, state, visibleDistance, elevationCosine, sortTiles);

    return true;
}

const OsmAnd::MapRendererInternalState* OsmAnd::AtlasMapRenderer_OpenGL::getInternalStateRef() const
{
    return &_internalState;
}

OsmAnd::MapRendererInternalState* OsmAnd::AtlasMapRenderer_OpenGL::getInternalStateRef()
{
    return &_internalState;
}

const OsmAnd::MapRendererInternalState& OsmAnd::AtlasMapRenderer_OpenGL::getInternalState() const
{
    return _internalState;
}

OsmAnd::MapRendererInternalState& OsmAnd::AtlasMapRenderer_OpenGL::getInternalState()
{
    return _internalState;
}

void OsmAnd::AtlasMapRenderer_OpenGL::updateFrustum(InternalState* internalState, const MapRendererState& state) const
{
    // 4 points of frustum near clipping box in camera coordinate space
    const glm::vec4 nTL_c(-internalState->projectionPlaneHalfWidth, +internalState->projectionPlaneHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nTR_c(+internalState->projectionPlaneHalfWidth, +internalState->projectionPlaneHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nBL_c(-internalState->projectionPlaneHalfWidth, -internalState->projectionPlaneHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nBR_c(+internalState->projectionPlaneHalfWidth, -internalState->projectionPlaneHalfHeight, -_zNear, 1.0f);

    // 4 points of frustum lower detail plane in camera coordinate space
    const auto zMid = internalState->zLowerDetail;
    const auto zMidK = zMid / _zNear;
    const glm::vec4 mTL_c(zMidK * nTL_c.x, zMidK * nTL_c.y, -zMid, 1.0f);
    const glm::vec4 mTR_c(zMidK * nTR_c.x, zMidK * nTR_c.y, -zMid, 1.0f);
    const glm::vec4 mBL_c(zMidK * nBL_c.x, zMidK * nBL_c.y, -zMid, 1.0f);
    const glm::vec4 mBR_c(zMidK * nBR_c.x, zMidK * nBR_c.y, -zMid, 1.0f);

    // Transform 8 frustum vertices + camera center to global space
    const auto eye_g = internalState->mCameraViewInv * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    const auto mTL_g = internalState->mCameraViewInv * mTL_c;
    const auto mTR_g = internalState->mCameraViewInv * mTR_c;
    const auto mBL_g = internalState->mCameraViewInv * mBL_c;
    const auto mBR_g = internalState->mCameraViewInv * mBR_c;
    const auto nTL_g = internalState->mCameraViewInv * nTL_c;
    const auto nTR_g = internalState->mCameraViewInv * nTR_c;
    const auto nBL_g = internalState->mCameraViewInv * nBL_c;
    const auto nBR_g = internalState->mCameraViewInv * nBR_c;

    // Get (up to) 4 points of frustum edges & plane intersection
    const glm::vec3 planeN(0.0f, 1.0f, 0.0f);
    const glm::vec3 planeO(0.0f, 0.0f, 0.0f);
    auto intersectionPointsCounter = 0u;
    glm::vec3 intersectionPoint;
    glm::vec2 intersectionPoints[4];

    // Get (up to) 2 additional points of frustum edges & plane intersection
    auto middleIntersectionsCounter = 0;
    glm::vec2 middleIntersections[2];

    // Get extra tiling field for elevated terrain
    int extraIntersectionsCounter = state.elevationAngle < 90.0f - state.fieldOfView ? 0 : -1;
    glm::vec2 extraIntersections[4];

    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nBL_g.xyz(), mBL_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
        if (extraIntersectionsCounter == 0)
            extraIntersections[extraIntersectionsCounter++] = intersectionPoint.xz();
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nBR_g.xyz(), mBR_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
        if (extraIntersectionsCounter == 1)
            extraIntersections[extraIntersectionsCounter++] = intersectionPoint.xz();
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTR_g.xyz(), mTR_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
        if (middleIntersectionsCounter < 2)
            middleIntersections[middleIntersectionsCounter++] = intersectionPoint.xz();
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTL_g.xyz(), mTL_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
        if (middleIntersectionsCounter < 2)
            middleIntersections[middleIntersectionsCounter++] = intersectionPoint.xz();
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, mTR_g.xyz(), mBR_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
        if (middleIntersectionsCounter < 2)
            middleIntersections[middleIntersectionsCounter++] = intersectionPoint.xz();
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, mTL_g.xyz(), mBL_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
        if (middleIntersectionsCounter < 2)
            middleIntersections[middleIntersectionsCounter++] = intersectionPoint.xz();
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTL_g.xyz(), nBL_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTR_g.xyz(), nBR_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
    }
    assert(intersectionPointsCounter == 4);

    internalState->frustum2D.p0 = PointF(intersectionPoints[0].x, intersectionPoints[0].y);
    internalState->frustum2D.p1 = PointF(intersectionPoints[1].x, intersectionPoints[1].y);
    internalState->frustum2D.p2 = PointF(intersectionPoints[2].x, intersectionPoints[2].y);
    internalState->frustum2D.p3 = PointF(intersectionPoints[3].x, intersectionPoints[3].y);

    const auto tileSize31 = (1u << (ZoomLevel::MaxZoomLevel - state.zoomLevel));
    internalState->frustum2D31.p0 = PointI64((internalState->frustum2D.p0 / TileSize3D) * static_cast<double>(tileSize31));
    internalState->frustum2D31.p1 = PointI64((internalState->frustum2D.p1 / TileSize3D) * static_cast<double>(tileSize31));
    internalState->frustum2D31.p2 = PointI64((internalState->frustum2D.p2 / TileSize3D) * static_cast<double>(tileSize31));
    internalState->frustum2D31.p3 = PointI64((internalState->frustum2D.p3 / TileSize3D) * static_cast<double>(tileSize31));

    internalState->globalFrustum2D31 = internalState->frustum2D31 + state.target31;

    // Get maximum height of terrain below camera
    const auto maxTerrainHeight = static_cast<float>(_maximumHeightFromSeaLevelInMeters / internalState->metersPerUnit);    
    
    // Get intersection points on elevated plane
    if (internalState->distanceFromCameraToGround > maxTerrainHeight)
    {
        const glm::vec3 planeE(0.0f, maxTerrainHeight, 0.0f);
        if (extraIntersectionsCounter == 2 &&
            Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeE, eye_g.xyz(), mBR_g.xyz(), intersectionPoint))
        {
            extraIntersections[extraIntersectionsCounter++] = intersectionPoint.xz();
        }
        if (extraIntersectionsCounter == 3 &&
            Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeE, eye_g.xyz(), mBL_g.xyz(), intersectionPoint))
        {
            extraIntersections[extraIntersectionsCounter++] = intersectionPoint.xz();
        }
    }
    else if (extraIntersectionsCounter == 2)
    {
        extraIntersections[extraIntersectionsCounter++] = eye_g.xz();
        extraIntersections[extraIntersectionsCounter++] = eye_g.xz();
    }

    if (extraIntersectionsCounter == 4)
    {
        internalState->extraField2D.p0 = PointF(extraIntersections[0].x, extraIntersections[0].y);
        internalState->extraField2D.p1 = PointF(extraIntersections[1].x, extraIntersections[1].y);
        internalState->extraField2D.p2 = PointF(extraIntersections[2].x, extraIntersections[2].y);
        internalState->extraField2D.p3 = PointF(extraIntersections[3].x, extraIntersections[3].y);
    }
    else
        internalState->extraField2D.p0 = PointF(NAN, NAN);

    internalState->rightMiddlePoint = PointF(middleIntersections[0].x, middleIntersections[0].y);
    internalState->leftMiddlePoint = PointF(middleIntersections[1].x, middleIntersections[1].y);
}

inline void OsmAnd::AtlasMapRenderer_OpenGL::computeTileset(const TileId targetTileId, const PointF targetInTileOffsetN,
    const PointF* points, QSet<TileId>* visibleTiles) const
{
    // "Round"-up tile indices
    // In-tile normalized position is added, since all tiles are going to be
    // translated in opposite direction during rendering
    PointF p[4];    
    for(int i = 0; i < 4; i++)
        p[i] = points[i] +  targetInTileOffsetN;

    const int yMin = qCeil(qMin(qMin(p[0].y, p[1].y), qMin(p[2].y, p[3].y)));
    const int yMax = qFloor(qMax(qMax(p[0].y + 1, p[1].y + 1), qMax(p[2].y + 1, p[3].y + 1)));
    int pxMin = std::numeric_limits<int32_t>::max();
    int pxMax = std::numeric_limits<int32_t>::min();
    float x;
    for (int y = yMin; y <= yMax; y++)
    {
        int xMin = std::numeric_limits<int32_t>::max();
        int xMax = std::numeric_limits<int32_t>::min();
        for (int k = 0; k < 4; k++)
        {
            if (Utilities::rayIntersectX(p[k % 4], p[(k + 1) % 4], y, x))
            {
                xMin = qMin(xMin, qFloor(x));
                xMax = qMax(xMax, qFloor(x));
            }
            if (p[k % 4].y > y - 1 && p[k % 4].y < y)
            {
                xMin = qMin(xMin, qFloor(p[k % 4].x));
                xMax = qMax(xMax, qFloor(p[k % 4].x));
            }
        }
        for (auto x = qMin(xMin, pxMin); x <= qMax(xMax, pxMax); x++)
        {
            TileId tileId;
            tileId.x = x + targetTileId.x;
            tileId.y = y - 1 + targetTileId.y;
            visibleTiles->insert(tileId);
        }
        pxMin = xMin;
        pxMax = xMax;
    }
}

void OsmAnd::AtlasMapRenderer_OpenGL::computeVisibleTileset(
    InternalState* internalState, const MapRendererState& state,
    const float visibleDistance, const double elevationCosine, const bool sortTiles) const
{
    float tileSize = TileSize3D;
    internalState->visibleTiles.clear();
    internalState->uniqueTiles.clear();

    // Normalize 2D-frustum points to tiles
    PointF p[4];
    p[0] = internalState->frustum2D.p0 / tileSize;
    p[1] = internalState->frustum2D.p1 / tileSize;
    p[2] = internalState->frustum2D.p2 / tileSize;
    p[3] = internalState->frustum2D.p3 / tileSize;

    // Determine tileset of high detail level
    QSet<TileId> preciseTiles;
    auto higherDetailTiles = &preciseTiles;
    computeTileset(internalState->targetTileId, internalState->targetInTileOffsetN, p, higherDetailTiles);
    if (!isnan(internalState->extraField2D.p0.x))
    {
        p[0] = internalState->extraField2D.p0 / tileSize;
        p[1] = internalState->extraField2D.p1 / tileSize;
        p[2] = internalState->extraField2D.p2 / tileSize;
        p[3] = internalState->extraField2D.p3 / tileSize;
        // Add extra tiles to tileset of high detail level
        computeTileset(internalState->targetTileId, internalState->targetInTileOffsetN, p, higherDetailTiles);
    }

    auto higherZoomLevel = state.zoomLevel;
    auto higherTargetTileId = internalState->targetTileId;
    QSet<TileId> additionalTiles;
    auto currentDetailTiles = &additionalTiles;

    // Add tiles of lower detail levels
    if (internalState->zLowerDetail != internalState->zFar && higherZoomLevel > MinZoomLevel)
    {            
        // 4 points of frustum near clipping box in camera coordinate space
        const glm::vec4 nTL_c(-internalState->projectionPlaneHalfWidth, +internalState->projectionPlaneHalfHeight, -_zNear, 1.0f);
        const glm::vec4 nTR_c(+internalState->projectionPlaneHalfWidth, +internalState->projectionPlaneHalfHeight, -_zNear, 1.0f);
        const glm::vec4 nBL_c(-internalState->projectionPlaneHalfWidth, -internalState->projectionPlaneHalfHeight, -_zNear, 1.0f);
        const glm::vec4 nBR_c(+internalState->projectionPlaneHalfWidth, -internalState->projectionPlaneHalfHeight, -_zNear, 1.0f);
    
        // Transform 2 frustum vertices to global space
        const auto nTL_g = internalState->mCameraViewInv * nTL_c;
        const auto nTR_g = internalState->mCameraViewInv * nTR_c;

        // 4 points of frustum edges & plane intersection
        const glm::vec3 planeN(0.0f, 1.0f, 0.0f);
        const glm::vec3 planeO(0.0f, 0.0f, 0.0f);
        glm::vec3 intersectionPoint;
        auto middleIntersectionsCounter = 2;
        glm::vec2 middleIntersections[4];
        middleIntersections[0] = glm::vec2(internalState->rightMiddlePoint.x, internalState->rightMiddlePoint.y);
        middleIntersections[1] = glm::vec2(internalState->leftMiddlePoint.x, internalState->leftMiddlePoint.y);

        QSet<TileId> visibleTiles;
        PointF offsetInTileN;
        TileId currentTargetTileId;
        auto distanceToLowerDetail = state.detailedDistance;
        for (auto zoomLevel = state.zoomLevel - 1; zoomLevel >= MinZoomLevel; zoomLevel--)
        {
            // Calculate distance to tiles of lower detail level
            tileSize *= 2.0f;
            distanceToLowerDetail += static_cast<float>(_detailDistanceFactor * tileSize);
            const auto zLowerDetail =
                distanceToLowerDetail < visibleDistance ?
                internalState->distanceFromCameraToTarget / internalState->scaleToRetainProjectedSize +
                static_cast<float>(qMax(0.0, static_cast<double>(distanceToLowerDetail) * elevationCosine)) :
                internalState->zFar;

            // 4 points of frustum lower detail plane in camera coordinate space
            const auto zMid = zLowerDetail;
            const auto zMidK = zMid / _zNear;
            const glm::vec4 mTL_c(zMidK * nTL_c.x, zMidK * nTL_c.y, -zMid, 1.0f);
            const glm::vec4 mTR_c(zMidK * nTR_c.x, zMidK * nTR_c.y, -zMid, 1.0f);
            const glm::vec4 mBL_c(zMidK * nBL_c.x, zMidK * nBL_c.y, -zMid, 1.0f);
            const glm::vec4 mBR_c(zMidK * nBR_c.x, zMidK * nBR_c.y, -zMid, 1.0f);

            // Transform 4 frustum vertices between detail levels to global space
            const auto mTL_g = internalState->mCameraViewInv * mTL_c;
            const auto mTR_g = internalState->mCameraViewInv * mTR_c;
            const auto mBL_g = internalState->mCameraViewInv * mBL_c;
            const auto mBR_g = internalState->mCameraViewInv * mBR_c;

            if (middleIntersectionsCounter < 4 &&
                Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTL_g.xyz(), mTL_g.xyz(), intersectionPoint))
            {
                middleIntersections[middleIntersectionsCounter++] = intersectionPoint.xz();
            }
            if (middleIntersectionsCounter < 4 &&
                Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTR_g.xyz(), mTR_g.xyz(), intersectionPoint))
            {
                middleIntersections[middleIntersectionsCounter++] = intersectionPoint.xz();
            }
            if (middleIntersectionsCounter < 4 &&
                Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, mTL_g.xyz(), mBL_g.xyz(), intersectionPoint))
            {
                middleIntersections[middleIntersectionsCounter++] = intersectionPoint.xz();
            }
            if (middleIntersectionsCounter < 4 &&
                Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, mTR_g.xyz(), mBR_g.xyz(), intersectionPoint))
            {
                middleIntersections[middleIntersectionsCounter++] = intersectionPoint.xz();
            }
            if (middleIntersectionsCounter < 4)
                break;

            // Determine tileset of current detail level
            p[0] = PointF(middleIntersections[0].x, middleIntersections[0].y) / tileSize;
            p[1] = PointF(middleIntersections[1].x, middleIntersections[1].y) / tileSize;
            p[2] = PointF(middleIntersections[2].x, middleIntersections[2].y) / tileSize;
            p[3] = PointF(middleIntersections[3].x, middleIntersections[3].y) / tileSize;
            currentTargetTileId =
                Utilities::getTileId(state.target31, static_cast<ZoomLevel>(zoomLevel), &offsetInTileN);
            computeTileset(currentTargetTileId, offsetInTileN, p, currentDetailTiles);

            // Remove overlapping tiles of higher detail level
            const auto tilesEnd = currentDetailTiles->cend();
            if (tilesEnd != currentDetailTiles->cbegin())
            {
                visibleTiles.clear();
                for (const auto& tileId : constOf(*higherDetailTiles))
                    if (currentDetailTiles->constFind(TileId::fromXY(tileId.x >> 1, tileId.y >> 1)) == tilesEnd)
                        visibleTiles.insert(tileId);
                internalState->visibleTiles[higherZoomLevel] = 
                    QVector<TileId>(visibleTiles.begin(), visibleTiles.end());
            }
            else
                internalState->visibleTiles[higherZoomLevel] =
                    QVector<TileId>(higherDetailTiles->begin(), higherDetailTiles->end());

            computeUniqueTileset(internalState, higherZoomLevel, higherTargetTileId, sortTiles);
    
            higherZoomLevel = static_cast<ZoomLevel>(zoomLevel);
            higherTargetTileId = currentTargetTileId;
            std::swap(higherDetailTiles, currentDetailTiles);
            if (zLowerDetail == internalState->zFar)
                break;

            currentDetailTiles->clear();

            // Prepare points for next detail level calculations
            middleIntersectionsCounter = 2;
            middleIntersections[0] = middleIntersections[3];
            middleIntersections[1] = middleIntersections[2];
        }   
    }

    internalState->visibleTiles[higherZoomLevel] =
        QVector<TileId>(higherDetailTiles->begin(), higherDetailTiles->end());
    computeUniqueTileset(internalState, higherZoomLevel, higherTargetTileId, sortTiles);
}

void OsmAnd::AtlasMapRenderer_OpenGL::computeUniqueTileset(InternalState* internalState,
    ZoomLevel zoomLevel, TileId targetTileId, const bool sortTiles) const
{
    // Normalize and make unique visible tiles
    QSet<TileId> uniqueTiles;
    for (const auto& tileId : constOf(internalState->visibleTiles[zoomLevel]))
        uniqueTiles.insert(Utilities::normalizeTileId(tileId, zoomLevel));
    internalState->uniqueTiles[zoomLevel] = QVector<TileId>(uniqueTiles.begin(), uniqueTiles.end());
    internalState->uniqueTilesTargets[zoomLevel] = targetTileId;
    if (sortTiles)
    {
        // Sort visible tiles by distance from target
        std::sort(internalState->uniqueTiles[zoomLevel],
            [targetTileId]
            (const TileId& l, const TileId& r) -> bool
            {
                const auto lx = l.x - targetTileId.x;
                const auto ly = l.y - targetTileId.y;

                const auto rx = r.x - targetTileId.x;
                const auto ry = r.y - targetTileId.y;

                return (lx*lx + ly*ly) < (rx*rx + ry*ry);
            });
    }
}

double OsmAnd::AtlasMapRenderer_OpenGL::getRealDistanceToHorizon(
    const InternalState& internalState, const MapRendererState& state,
     const PointD& groundPosition, const double inglobeAngle, const double referenceDistance) const
{
    const auto farthestLocation = Utilities::rhumbDestinationPoint(internalState.cameraCoordinates,
        _radius * inglobeAngle, state.azimuth);
    const auto farthestPos =
        Utilities::convert31toDouble(Utilities::convertLatLonTo31(farthestLocation), state.zoomLevel);
    auto delta = farthestPos - groundPosition;
    const auto tileCount = static_cast<double>(1u << state.zoomLevel);
    const auto absAzimuth = std::fabs(state.azimuth);
    if ((absAzimuth > 89.99f && absAzimuth < 90.01f && std::fabs(delta.y) > tileCount / 2.0) ||
        ((state.azimuth < -90.0f || state.azimuth > 90.0f) && delta.y < 0.0) ||
        (state.azimuth > -90.0f && state.azimuth < 90.0f && delta.y > 0.0))
        delta.y = tileCount - std::fabs(delta.y);
    const auto shortestDistance = qSqrt(delta.x * delta.x + delta.y * delta.y) * TileSize3D;
    delta.x = tileCount - std::fabs(delta.x);
    const auto alternativeDistance = qSqrt(delta.x * delta.x + delta.y * delta.y) * TileSize3D;
    const auto distance =
        std::fabs(shortestDistance - referenceDistance) < std::fabs(alternativeDistance - referenceDistance) ? 
        shortestDistance : alternativeDistance;

    // Slowly start using simple horizon distance instead (at pole regions)
    const auto absLatitude = std::fabs(internalState.cameraCoordinates.latitude);
    if (absLatitude > _maximumAbsoluteLatitudeForRealHorizon)
    {
        const auto referenceFactor = qMin(1.0, (absLatitude - _maximumAbsoluteLatitudeForRealHorizon) / 5.0);
        return referenceDistance * referenceFactor + distance * (1.0 - referenceFactor);
    }

    return distance;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getPositionFromScreenPoint(const InternalState& internalState,
    const MapRendererState& state, const PointI& screenPoint, PointD& position,
    const float height /*=0.0f*/, float* distance /*=nullptr*/) const
{
    if (screenPoint.x < 0 || screenPoint.y < 0)
        return false;
    const auto nearInWorld = glm::unProject(
        glm::vec3(screenPoint.x, state.windowSize.y - screenPoint.y, 0.0f),
        internalState.mCameraView,
        internalState.mPerspectiveProjection,
        internalState.glmViewport);
    const auto farInWorld = glm::unProject(
        glm::vec3(screenPoint.x, state.windowSize.y - screenPoint.y, 1.0f),
        internalState.mCameraView,
        internalState.mPerspectiveProjection,
        internalState.glmViewport);
    const auto rayD = glm::normalize(farInWorld - nearInWorld);

    const glm::vec3 planeN(0.0f, 1.0f, 0.0f);
    const glm::vec3 planeO(0.0f, 0.0f, 0.0f);
    float length;
    const auto intersects = Utilities_OpenGL_Common::rayIntersectPlane(planeN, planeO, rayD, nearInWorld, length);
    if (!intersects)
        return false;

    auto intersection = nearInWorld + length*rayD;
    auto pointOnPlane = intersection.xz();

    if (height != 0.0f)
    {
        auto pointOffset = glm::normalize(rayD.xz());
        const auto offsetLength = height * qTan(qAcos(-rayD.y));
        pointOffset *= offsetLength;
        pointOnPlane -= pointOffset;
    }

    position = pointOnPlane / static_cast<float>(TileSize3D);

    if (distance)
        *distance = length;

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getNearestLocationFromScreenPoint(
    const InternalState& internalState, const MapRendererState& state,
    const PointI& location31, const PointI& screenPoint,
    PointI64& fixedLocation, PointI64& currentLocation) const
{
    PointD position;
    bool ok = getPositionFromScreenPoint(internalState, state, screenPoint, position);
    if (!ok)
        return false;

    position += internalState.targetInTileOffsetN;
    const auto zoomLevelDiff = ZoomLevel::MaxZoomLevel - state.zoomLevel;
    const auto tileSize31 = (1u << zoomLevelDiff);
    position *= tileSize31;

    currentLocation.x = static_cast<int64_t>(position.x)+(internalState.targetTileId.x << zoomLevelDiff);
    currentLocation.y = static_cast<int64_t>(position.y)+(internalState.targetTileId.y << zoomLevelDiff);

    auto offset = currentLocation - location31;
    const auto intHalf = INT32_MAX / 2 + 1;
    if (offset.x >= intHalf)
        offset.x = offset.x - INT32_MAX - 1;
    else if (offset.x < -intHalf)
        offset.x = offset.x + INT32_MAX + 1;
    if (offset.y >= intHalf)
        offset.y = offset.y - INT32_MAX - 1;
    else if (offset.y < -intHalf)
        offset.y = offset.y + INT32_MAX + 1;
    
    fixedLocation = currentLocation - offset;
    
    return true;
}

std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU> OsmAnd::AtlasMapRenderer_OpenGL::captureElevationDataResource(
    const MapRendererState& state, TileId normalizedTileId, ZoomLevel zoomLevel,
    std::shared_ptr<const IMapElevationDataProvider::Data>* pOutSource /*= nullptr*/) const
{
    if (!isRenderingInitialized() || !state.elevationDataProvider)
        return nullptr;

    const auto& resourcesCollection_ = getResources().getCollectionSnapshot(MapRendererResourceType::ElevationData, state.elevationDataProvider);

    if (!resourcesCollection_)
        return nullptr;

    const auto& resourcesCollection = std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection_);

    // Obtain tile entry by normalized tile coordinates, since tile may repeat several times
    std::shared_ptr<MapRendererBaseTiledResource> resource_;
    if (resourcesCollection && resourcesCollection->obtainResource(normalizedTileId, zoomLevel, resource_))
    {
        const auto resource = std::static_pointer_cast<MapRendererElevationDataResource>(resource_);

        // Check state and obtain GPU resource
        if (resource->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
        {
            // Capture GPU resource
            auto gpuResource = resource->resourceInGPU;
            if (pOutSource) {
                *pOutSource = resource->sourceData;
            }

            resource->setState(MapRendererResourceState::Uploaded);

            return gpuResource;
        }
    }

    return nullptr;
}

OsmAnd::ZoomLevel OsmAnd::AtlasMapRenderer_OpenGL::getElevationData(const MapRendererState& state,
    TileId normalizedTileId, ZoomLevel zoomLevel, PointF& offsetInTileN, bool noUnderscaled,
    std::shared_ptr<const IMapElevationDataProvider::Data>* pOutSource /*= nullptr*/) const
{
    if (!currentState.elevationDataProvider)
        return ZoomLevel::InvalidZoomLevel;

    if (captureElevationDataResource(state, normalizedTileId, zoomLevel, pOutSource))
        return zoomLevel;

    const auto maxMissingDataZoomShift = currentState.elevationDataProvider->getMaxMissingDataZoomShift();
    const auto maxUnderZoomShift = currentState.elevationDataProvider->getMaxMissingDataUnderZoomShift();
    const auto minZoom = currentState.elevationDataProvider->getMinZoom();
    const auto maxZoom = currentState.elevationDataProvider->getMaxZoom();
    for (int absZoomShift = 1; absZoomShift <= maxMissingDataZoomShift; absZoomShift++)
    {
        // Look for underscaled first. Only full match is accepted.
        // Don't replace tiles of absent zoom levels by the unserscaled ones
        const auto underscaledZoom = static_cast<int>(zoomLevel) + absZoomShift;
        if (!noUnderscaled && underscaledZoom >= minZoom && underscaledZoom <= maxZoom &&
            absZoomShift <= maxUnderZoomShift && zoomLevel >= minZoom)
        {
            auto underscaledTileIdN =
                TileId::fromXY(normalizedTileId.x << absZoomShift, normalizedTileId.y << absZoomShift);
            PointF offsetN;
            if (absZoomShift < 20)
                offsetN = offsetInTileN * static_cast<float>(1u << absZoomShift);
            else
            {
                const double tileSize = 1ull << absZoomShift;
                offsetN.x = static_cast<double>(offsetInTileN.x) * tileSize;
                offsetN.y = static_cast<double>(offsetInTileN.y) * tileSize;
            }
            const auto innerOffset =
                PointI(static_cast<int32_t>(std::floor(offsetN.x)), static_cast<int32_t>(std::floor(offsetN.y)));
            underscaledTileIdN.x += innerOffset.x;
            underscaledTileIdN.y += innerOffset.y;
            const auto underscaledZoomLevel = static_cast<ZoomLevel>(underscaledZoom);
            if (captureElevationDataResource(state, underscaledTileIdN, underscaledZoomLevel, pOutSource))
            {
                offsetInTileN.x = offsetN.x - static_cast<float>(innerOffset.x);
                offsetInTileN.y = offsetN.y - static_cast<float>(innerOffset.y);
                return underscaledZoomLevel;
            }
        }

        // If underscaled was not found, look for overscaled (surely, if such zoom level exists at all)
        const auto overscaledZoom = static_cast<int>(zoomLevel) - absZoomShift;
        if (overscaledZoom >= minZoom && overscaledZoom <= maxZoom)
        {
            PointF texCoordsOffset;
            PointF texCoordsScale;
            const auto overscaledTileIdN = Utilities::getTileIdOverscaledByZoomShift(
                normalizedTileId,
                absZoomShift,
                &texCoordsOffset,
                &texCoordsScale);
            const auto overscaledZoomLevel = static_cast<ZoomLevel>(overscaledZoom);
            if (captureElevationDataResource(state, overscaledTileIdN, overscaledZoomLevel, pOutSource))
            {
                offsetInTileN.x = texCoordsOffset.x + offsetInTileN.x * texCoordsScale.x;
                offsetInTileN.y = texCoordsOffset.y + offsetInTileN.y * texCoordsScale.y;
                return overscaledZoomLevel;
            }
        }
    }

    return ZoomLevel::InvalidZoomLevel;
}

OsmAnd::GPUAPI_OpenGL* OsmAnd::AtlasMapRenderer_OpenGL::getGPUAPI() const
{
    return static_cast<OsmAnd::GPUAPI_OpenGL*>(gpuAPI.get());
}

float OsmAnd::AtlasMapRenderer_OpenGL::getTileSizeOnScreenInPixels() const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, getState(), *getConfiguration(), true);

    return ok ? internalState.referenceTileSizeOnScreenInPixels * internalState.tileOnScreenScaleFactor : 0.0;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getWorldPointFromScreenPoint(
    const MapRendererInternalState& internalState_,
    const MapRendererState& state,
    const PointI& screenPoint,
    PointF& outWorldPoint) const
{
    const auto& internalState = static_cast<const InternalState&>(internalState_);

    PointD position;
    bool ok = getPositionFromScreenPoint(internalState, state, screenPoint, position);
    if (!ok)
        return false;
    position *= AtlasMapRenderer::TileSize3D;

    outWorldPoint = PointF(static_cast<float>(position.x), static_cast<float>(position.y));
    return true;
}

float OsmAnd::AtlasMapRenderer_OpenGL::getWorldElevationOfLocation(const MapRendererState& state,
    const float elevationInMeters, const PointI& location31_) const
{
    if (elevationInMeters != 0.0f)
    {
        const auto location31 = Utilities::normalizeCoordinates(location31_, ZoomLevel31);   
        PointF offsetInTileN;
        TileId tileId = Utilities::getTileId(location31, state.zoomLevel, &offsetInTileN);
        const auto scaledElevationInMeters = elevationInMeters * state.elevationConfiguration.dataScaleFactor;
        const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, tileId.y, TileSize3D);
        const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, tileId.y + 1, TileSize3D);
        const auto metersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offsetInTileN.y);
        return scaledElevationInMeters / metersPerUnit * state.elevationConfiguration.zScaleFactor;
    }
    else
        return 0.0f;
}

float OsmAnd::AtlasMapRenderer_OpenGL::getElevationOfLocationInMeters(const MapRendererState& state,
    const float elevation, const ZoomLevel zoom, const PointI& location31_) const
{
    if (elevation != 0.0f)
    {
        const auto location31 = Utilities::normalizeCoordinates(location31_, ZoomLevel31);   
        PointF offsetInTileN;
        TileId tileId = Utilities::getTileId(location31, zoom, &offsetInTileN);
        const auto scaledElevation = elevation / state.elevationConfiguration.zScaleFactor;
        const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(zoom, tileId.y, TileSize3D);
        const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(zoom, tileId.y + 1, TileSize3D);
        const auto metersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offsetInTileN.y);
        return scaledElevation * metersPerUnit / state.elevationConfiguration.dataScaleFactor;
    }
    else
        return 0.0f;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getLocationFromScreenPoint(const PointI& screenPoint, PointI& location31) const
{
    PointI64 location;
    if (!getLocationFromScreenPoint(screenPoint, location))
        return false;
    location31 = Utilities::normalizeCoordinates(location, ZoomLevel31);

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getLocationFromScreenPoint(const PointI& screenPoint, PointI64& location) const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), true);
    if (!ok)
        return false;
    
    PointD position;
    ok = getPositionFromScreenPoint(internalState, state, screenPoint, position);
    if (!ok)
        return false;

    position += internalState.targetInTileOffsetN;
    const auto zoomLevelDiff = ZoomLevel::MaxZoomLevel - state.zoomLevel;
    const auto tileSize31 = (1u << zoomLevelDiff);
    position *= tileSize31;

    location.x = static_cast<int64_t>(position.x)+(internalState.targetTileId.x << zoomLevelDiff);
    location.y = static_cast<int64_t>(position.y)+(internalState.targetTileId.y << zoomLevelDiff);

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getLocationFromElevatedPoint(const MapRendererState& state,
    const PointI& screenPoint, PointI& location31, float* heightInMeters /*=nullptr*/) const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration());
    if (!ok)
        return false;
    
    PointD position;
    float distance = 0.0f;
    ok = getPositionFromScreenPoint(internalState, state, screenPoint, position, 0.0f, &distance);
    if (!ok)
        return false;

    position += internalState.targetInTileOffsetN;
    const auto zoomLevelDiff = ZoomLevel::MaxZoomLevel - state.zoomLevel;
    const auto tileSize31 = (1u << zoomLevelDiff);
    position *= tileSize31;

    PointI64 location;
    location.x = static_cast<int64_t>(position.x)+(internalState.targetTileId.x << zoomLevelDiff);
    location.y = static_cast<int64_t>(position.y)+(internalState.targetTileId.y << zoomLevelDiff);

    position = PointD(internalState.groundCameraPosition);
    position /= static_cast<double>(TileSize3D);
    position += internalState.targetInTileOffsetN;
    position *= tileSize31;

    PointI64 cameraLocation;
    cameraLocation.x = static_cast<int64_t>(position.x)+(internalState.targetTileId.x << zoomLevelDiff);
    cameraLocation.y = static_cast<int64_t>(position.y)+(internalState.targetTileId.y << zoomLevelDiff);

    if (cameraLocation.x == location.x && cameraLocation.y == location.y)
    {
        location31 = Utilities::normalizeCoordinates(location, ZoomLevel31);
        if (heightInMeters)
            *heightInMeters = getLocationHeightInMeters(state, location31);
        return true;
    }

    // Find the intersection point with elevated surface
    const auto elevationScaleFactor =
        state.elevationConfiguration.zScaleFactor * state.elevationConfiguration.dataScaleFactor;
    auto endPoint =
        Utilities::convert31toDouble(location, state.zoomLevel);
    auto startPoint =
        Utilities::convert31toDouble(cameraLocation, state.zoomLevel);
    auto startPointZ = static_cast<double>(internalState.worldCameraPosition.y);
    double endPointZ = 0.0;
    if (distance < 0.0f)
    {
        endPoint = startPoint + startPoint - endPoint;
        endPointZ = startPointZ * 2.0;
    }
    const auto endTileId = PointD(std::floor(endPoint.x), std::floor(endPoint.y));
    const auto deltaX = endPoint.x - startPoint.x;
    const auto deltaY = endPoint.y - startPoint.y;
    const auto deltaZ = endPointZ - startPointZ;
    const auto factorX = deltaX / deltaY;
    const auto factorY = deltaY / deltaX;
    const auto factorZX = deltaZ / deltaX;
    const auto factorZY = deltaZ / deltaY;
    auto midPoint = startPoint;
    auto midPointZ = startPointZ;
    auto tmpPoint = midPoint;
    auto tmpPointZ = midPointZ;
    int tilesCount = 0;
    const auto tiles = internalState.uniqueTiles.cend();
    if (tiles != internalState.uniqueTiles.cbegin())
        tilesCount = (tiles - 1)->size();
    do
    {
        auto startTileId = PointD(std::floor(startPoint.x), std::floor(startPoint.y));
        if (startTileId.x != endTileId.x && startPoint.x != endTileId.x + 1.0 && endPoint.x != startTileId.x + 1.0)
        {
            midPoint.x = startTileId.x +
                (endTileId.x > startTileId.x ? 1.0 : startPoint.x > startTileId.x ? 0.0 : -1.0);
            midPoint.y = startPoint.y + (midPoint.x - startPoint.x) * factorY;
            midPointZ = startPointZ + (midPoint.x - startPoint.x) * factorZX;
        }
        if (startTileId.y != endTileId.y && startPoint.y != endTileId.y + 1.0 && endPoint.y != startTileId.y + 1.0)
        {
            tmpPoint.y = startTileId.y +
                (endTileId.y > startTileId.y ? 1.0 : startPoint.y > startTileId.y ? 0.0 : -1.0);
            tmpPoint.x = startPoint.x + (tmpPoint.y - startPoint.y) * factorX;
            tmpPointZ = startPointZ + (tmpPoint.y - startPoint.y) * factorZY;
            if (midPoint == startPoint || std::fabs(tmpPoint.x - startPoint.x) + std::fabs(tmpPoint.y - startPoint.y) <
                std::fabs(midPoint.x - startPoint.x) + std::fabs(midPoint.y - startPoint.y))
            {
                midPoint = tmpPoint;
                midPointZ = tmpPointZ;
            }
        }
        if (midPoint == startPoint)
        {
            midPoint = endPoint;
            midPointZ = 0.0;
        }
        startTileId =
            PointD(std::floor(std::min(startPoint.x, midPoint.x)), std::floor(std::min(startPoint.y, midPoint.y)));
        const double maxInt = std::numeric_limits<int32_t>::max();
        if (startTileId.x > maxInt)
            startTileId.x -= maxInt;
        if (startTileId.y > maxInt)
            startTileId.y -= maxInt;
        const auto tileId = Utilities::normalizeTileId(
            TileId::fromXY(static_cast<int32_t>(startTileId.x), static_cast<int32_t>(startTileId.y)), state.zoomLevel);
        const PointD startPointOffset(startPoint.x - startTileId.x,startPoint.y - startTileId.y);
        std::shared_ptr<const IMapElevationDataProvider::Data> elevationData;
        PointF scaledStart(startPointOffset.x, startPointOffset.y);
        const PointD midPointOffset(midPoint.x - startTileId.x, midPoint.y - startTileId.y);
        const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, tileId.y, TileSize3D);
        const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, tileId.y + 1, TileSize3D);
        const auto midMetersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, midPointOffset.y);
        const auto midElevationFactor = midMetersPerUnit / elevationScaleFactor;
        auto scaledZoom = InvalidZoomLevel;
        if (midPointZ * midElevationFactor < _maximumHeightFromSeaLevelInMeters)
            scaledZoom = getElevationData(state, tileId, state.zoomLevel, scaledStart, true, &elevationData);
        if (scaledZoom != InvalidZoomLevel && elevationData)
        {
            const double tileSize = 1ull << (state.zoomLevel - scaledZoom);
            const auto scaledDistance = (midPointOffset - startPointOffset) / tileSize;
            PointF scaledEnd(scaledStart.x + scaledDistance.x, scaledStart.y + scaledDistance.y);
            const auto startMetersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, startPointOffset.y);
            const auto startElevationFactor = startMetersPerUnit / elevationScaleFactor;
            PointF exactLocation;
            float exactHeight = 0.0f;
            if (elevationData->getClosestPoint(
                static_cast<float>(startElevationFactor), static_cast<float>(midElevationFactor),
                scaledStart, startPointZ, scaledEnd, midPointZ, tileSize, exactLocation, &exactHeight))
            {
                const PointD finalLocation(startPointOffset.x + tileSize * (exactLocation.x - scaledStart.x),
                    startPointOffset.y + tileSize * (exactLocation.y - scaledStart.y));
                location31 = Utilities::normalizeCoordinates(PointI64(
                    static_cast<int64_t>((static_cast<double>(finalLocation.x) + startTileId.x) * tileSize31),
                    static_cast<int64_t>((static_cast<double>(finalLocation.y) + startTileId.y) * tileSize31)),
                    ZoomLevel31);
                if (heightInMeters)
                    *heightInMeters = exactHeight;
                return true;
            }
        }
        startPoint = midPoint;
        startPointZ = midPointZ;
        tilesCount--;
    }
    while (midPoint != endPoint && tilesCount > 0);

    // If no intersections with elevated surface was found
    location31 = Utilities::normalizeCoordinates(location, ZoomLevel31);

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getLocationFromElevatedPoint(
    const PointI& screenPoint, PointI& location31, float* heightInMeters /*=nullptr*/) const
{
    const auto state = getState();

    return getLocationFromElevatedPoint(state, screenPoint, location31, heightInMeters);
}

float OsmAnd::AtlasMapRenderer_OpenGL::getHeightAndLocationFromElevatedPoint(
    const PointI& screenPoint, PointI& location31) const
{
    const auto state = getState();

    float elevationInMeters = 0.0f;
    if (!getLocationFromElevatedPoint(state, screenPoint, location31, &elevationInMeters))
        elevationInMeters = _invalidElevationValue;
    return elevationInMeters;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getZoomAndRotationAfterPinch(
    const PointI& firstLocation31, const float firstHeight, const PointI& firstPoint,
    const PointI& secondLocation31, const float secondHeight, const PointI& secondPoint,
    PointD& zoomAndRotate) const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), true);
    if (!ok)
        return false;

    // Calculate location on the plane for the first touch
    PointI64 firstNeeded;
    PointI64 firstCurrent;
    ok = getNearestLocationFromScreenPoint(internalState, state,
        firstLocation31, firstPoint, firstNeeded, firstCurrent);
    if (!ok)
        return false;
        
    // Calculate location on the plane for the second touch
    PointI64 secondNeeded;
    PointI64 secondCurrent;
    ok = getNearestLocationFromScreenPoint(internalState, state,
        secondLocation31, secondPoint, secondNeeded, secondCurrent);
    if (!ok)
        return false;

    // Calculate camera location on the plane
    PointD position(internalState.groundCameraPosition);
    position /= static_cast<double>(TileSize3D);
    position += internalState.targetInTileOffsetN;
    const auto zoomLevelDiff = ZoomLevel::MaxZoomLevel - state.zoomLevel;
    const auto tileSize31 = (1u << zoomLevelDiff);
    position *= tileSize31;
    PointI64 cameraLocation;
    cameraLocation.x = static_cast<int64_t>(position.x)+(internalState.targetTileId.x << zoomLevelDiff);
    cameraLocation.y = static_cast<int64_t>(position.y)+(internalState.targetTileId.y << zoomLevelDiff);
    
    // Calculate distance factor to get correct zoom shift
    const PointD firstSegment(cameraLocation - firstCurrent);
    const PointD secondSegment(cameraLocation - secondCurrent);
    const PointD segmentLength(firstSegment.norm(), secondSegment.norm());
    PointD segmentRatio(firstHeight, secondHeight);
    segmentRatio /= internalState.distanceFromCameraToGroundInMeters;
    if (segmentRatio.x >= 1.0 || segmentRatio.y >= 1.0)
        segmentRatio = PointD();
    const PointD segmentOffset(segmentLength.x * segmentRatio.x, segmentLength.y * segmentRatio.y);
    const PointD currentSegment(secondCurrent - firstCurrent);
    const auto currentDistance = currentSegment.norm();
    if (currentDistance == 0.0)
        return false;
    const PointD currentSegmentN = currentSegment / currentDistance;
    const auto firstAngle = qAcos(Utilities::dotProduct(firstSegment / segmentLength.x, currentSegmentN));
    const auto secondAngle = qAcos(Utilities::dotProduct(secondSegment / segmentLength.y, PointD() - currentSegmentN));
    const PointD neededSegment(secondNeeded - firstNeeded);
    const auto sqrNeededDistance = neededSegment.squareNorm();
    if (sqrNeededDistance == 0.0)
        return false;
    const auto range = segmentOffset.x * qSin(firstAngle) - segmentOffset.y * qSin(secondAngle);
    const auto sqrRange = range * range;
    const auto width = segmentOffset.x * qCos(firstAngle) + segmentOffset.y * qCos(secondAngle);
    if (sqrNeededDistance < sqrRange)
        return false;
    const auto zoomedDistance = qSqrt(sqrNeededDistance - sqrRange) + width;
    if (zoomedDistance <= 0.0)
        return false;
    auto distanceFactor = currentDistance / zoomedDistance;
    auto zoomedRatio = segmentRatio * distanceFactor;
    if (zoomedRatio.x > 1.0 || zoomedRatio.y > 1.0)
    {
        distanceFactor = 1.0 / qMax(segmentRatio.x, segmentRatio.y);
        zoomedRatio = segmentRatio * distanceFactor;
    }

    // Calculate needed offsets for zoom and azimuth to show the same location
    // using new screen coordinates of the second finger, assuming the first one is a pivot point
    const auto upScale = distanceFactor > 1.0;
    const auto factor = upScale ? distanceFactor : 1.0 / distanceFactor;
    const auto zoomLevel = std::floor(std::log2(factor));
    if (zoomLevel > MaxZoomLevel)
        return false;
    const auto zoom = static_cast<double>(1u << static_cast<int>(zoomLevel));
    zoomAndRotate.x = upScale ? zoomLevel + factor / zoom - 1.0 : (zoom / factor - 1.0) * 2.0 - zoomLevel;
    const auto actualSegmentN = PointD(PointD(secondCurrent) + secondSegment * zoomedRatio.y -
        PointD(firstCurrent) - firstSegment * zoomedRatio.x).normalized();
    const auto neededSegmentN = neededSegment / qSqrt(sqrNeededDistance);
    const auto angle = qRadiansToDegrees(qAcos(Utilities::dotProduct(actualSegmentN, neededSegmentN))) *
        (Utilities::dotProduct(actualSegmentN, PointD(neededSegmentN.y, -neededSegmentN.x)) < 0.0 ? -1.0 : 1.0);
    zoomAndRotate.y = Utilities::normalizedAngleDegrees(angle);

    return true;
}

float OsmAnd::AtlasMapRenderer_OpenGL::getLocationHeightInMeters(const MapRendererState& state,
    const PointI& location31_) const
{
    const auto location31 = Utilities::normalizeCoordinates(location31_, ZoomLevel31);   

    // Get elevation data
    float elevationInMeters = _invalidElevationValue;
    PointF offsetInTileN;
    TileId tileId = Utilities::getTileId(location31, state.zoomLevel, &offsetInTileN);
    std::shared_ptr<const IMapElevationDataProvider::Data> elevationData;
    PointF offsetInScaledTileN = offsetInTileN;
    if (getElevationData(state, tileId, state.zoomLevel, offsetInScaledTileN, false, &elevationData) != InvalidZoomLevel &&
        elevationData)
    {
        elevationData->getValue(offsetInScaledTileN, elevationInMeters);
    }
    return elevationInMeters;
}

float OsmAnd::AtlasMapRenderer_OpenGL::getLocationHeightInMeters(const PointI& location31) const
{
    const auto state = getState();

    return getLocationHeightInMeters(state, location31);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getNewTargetByScreenPoint(const MapRendererState& state,
    const PointI& screenPoint, const PointI& location31, PointI& target31, const float height /*=0.0f*/) const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), true);
    if (!ok)
        return false;

    PointD position;
    ok = getPositionFromScreenPoint(internalState, state, screenPoint, position, height);
    if (!ok)
        return false;

    const auto zoomLevelDiff = ZoomLevel::MaxZoomLevel - state.zoomLevel;
    const auto tileSize31 = (1u << zoomLevelDiff);
    position *= tileSize31;

    PointI64 target;
    target.x = static_cast<int64_t>(location31.x) + (1ull << ZoomLevel::MaxZoomLevel) - static_cast<int64_t>(position.x);
    target.y = static_cast<int64_t>(location31.y) + (1ull << ZoomLevel::MaxZoomLevel) - static_cast<int64_t>(position.y);

    target31 = Utilities::normalizeCoordinates(target, ZoomLevel31);

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getNewTargetByScreenPoint(const PointI& screenPoint, const PointI& location31,
    PointI& target31, const float height /*=0.0f*/) const
{
    const auto state = getState();

    return getNewTargetByScreenPoint(state, screenPoint, location31, target31, height);
}

float OsmAnd::AtlasMapRenderer_OpenGL::getHeightOfLocation(const MapRendererState& state,
    const PointI& location31_) const
{
    const auto location31 = Utilities::normalizeCoordinates(location31_, ZoomLevel31);   

    // Get elevation data
    float height = 0.0f;
    PointF offsetInTileN;
    TileId tileId = Utilities::getTileId(location31, state.zoomLevel, &offsetInTileN);
    std::shared_ptr<const IMapElevationDataProvider::Data> elevationData;
    PointF offsetInScaledTileN = offsetInTileN;
    if (getElevationData(state, tileId, state.zoomLevel, offsetInScaledTileN, false, &elevationData) != InvalidZoomLevel &&
        elevationData)
    {
        float elevationInMeters = 0.0f;
        if (elevationData->getValue(offsetInScaledTileN, elevationInMeters))
        {
            const auto scaledElevationInMeters = elevationInMeters * state.elevationConfiguration.dataScaleFactor;

            const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, tileId.y, TileSize3D);
            const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, tileId.y + 1, TileSize3D);
            const auto metersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offsetInTileN.y);

            height = static_cast<float>((scaledElevationInMeters / metersPerUnit) * state.elevationConfiguration.zScaleFactor);
        }
    }   
    
    return height;
}

float OsmAnd::AtlasMapRenderer_OpenGL::getHeightOfLocation(const PointI& location31) const
{
    const auto state = getState();

    return getHeightOfLocation(state, location31);
}

float OsmAnd::AtlasMapRenderer_OpenGL::getMapTargetDistance(const PointI& location31, bool checkOffScreen /*=false*/) const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), true);
    if (!ok)
        return false;

    if (!checkOffScreen && !internalState.globalFrustum2D31.test(location31))
        return false;

    const auto height = getLocationHeightInMeters(state, location31);
    const auto locationVerticalDistance = _radius +
        static_cast<double>(height > _invalidElevationValue ? height : 0.0f);
    const auto groundDistanceToTarget =
        Utilities::distance(internalState.cameraCoordinates, Utilities::convert31ToLatLon(location31));
    const auto inglobeAngle = groundDistanceToTarget / _radius;
    const auto cameraVerticalDistance = _radius + internalState.distanceFromCameraToGroundInMeters;
    const auto verticalDelta = cameraVerticalDistance - locationVerticalDistance * qCos(inglobeAngle);
    const auto horizontalDelta = locationVerticalDistance * qSin(inglobeAngle);
    const auto distance =
        static_cast<float>(qSqrt(verticalDelta * verticalDelta + horizontalDelta * horizontalDelta) / 1000.0);

    return distance;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getProjectedLocation(const MapRendererInternalState& internalState_,
    const MapRendererState& state, const PointI& location31, const float height, PointI& outLocation31) const
{
    const auto internalState = static_cast<const InternalState*>(&internalState_);

    const auto offsetFromTarget = Utilities::convert31toFloat(location31 - state.target31, state.zoomLevel);
    const auto positionInWorld = glm::vec3(
        offsetFromTarget.x * AtlasMapRenderer::TileSize3D,
        height,
        offsetFromTarget.y * AtlasMapRenderer::TileSize3D);


    const auto rayD = glm::normalize(positionInWorld - internalState->worldCameraPosition);

    const glm::vec3 planeN(0.0f, 1.0f, 0.0f);
    const glm::vec3 planeO(0.0f, 0.0f, 0.0f);
    float length;
    const auto intersects =
        Utilities_OpenGL_Common::rayIntersectPlane(planeN, planeO, rayD, internalState->worldCameraPosition, length);
    if (!intersects)
        return false;

    auto intersection = internalState->worldCameraPosition + length * rayD;
    auto pointOnPlane = intersection.xz();

    PointD position;
    position.x = pointOnPlane.x / static_cast<float>(TileSize3D);
    position.y = pointOnPlane.y / static_cast<float>(TileSize3D);
    position += internalState->targetInTileOffsetN;
    const auto zoomLevelDiff = ZoomLevel::MaxZoomLevel - state.zoomLevel;
    const auto tileSize31 = (1u << zoomLevelDiff);
    position *= tileSize31;

    PointI64 location(static_cast<int64_t>(position.x)+(internalState->targetTileId.x << zoomLevelDiff),
        static_cast<int64_t>(position.y)+(internalState->targetTileId.y << zoomLevelDiff));

    outLocation31 = Utilities::normalizeCoordinates(location, ZoomLevel31);

    return true;
}

OsmAnd::AreaI OsmAnd::AtlasMapRenderer_OpenGL::getVisibleBBox31() const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, getState(), *getConfiguration(), true);
    if (!ok)
        return AreaI::largest();

    return internalState.globalFrustum2D31.getBBox31();
}

OsmAnd::AreaI OsmAnd::AtlasMapRenderer_OpenGL::getVisibleBBox31(const MapRendererInternalState& internalState_) const
{
    const auto internalState = static_cast<const InternalState*>(&internalState_);
    return internalState->globalFrustum2D31.getBBox31();
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isPositionVisible(const PointI64& position) const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, getState(), *getConfiguration(), true);
    if (!ok)
        return false;

    return static_cast<const Frustum2DI64*>(&internalState.globalFrustum2D31)->test(position);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isPositionVisible(const PointI& position31) const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, getState(), *getConfiguration(), true);
    if (!ok)
        return false;

    return internalState.globalFrustum2D31.test(position31);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isPathVisible(const QVector<PointI>& path31) const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, getState(), *getConfiguration(), true);
    if (!ok)
        return false;

    if (!internalState.globalFrustum2D31.test(path31))
        return false;

    // TODO: Check pixelsize of path

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isAreaVisible(const AreaI& area31) const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, getState(), *getConfiguration(), true);
    if (!ok)
        return false;

    return internalState.globalFrustum2D31.test(area31);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isTileVisible(const int tileX, const int tileY, const int zoom) const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, getState(), *getConfiguration());
    if (!ok)
        return false;

    const auto& tilesOfZoom = internalState.uniqueTiles.constFind(static_cast<ZoomLevel>(zoom));
    if (tilesOfZoom != internalState.uniqueTiles.cend())
        return tilesOfZoom->contains(TileId::fromXY(tileX, tileY));

    return false;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::obtainScreenPointFromPosition(const PointI64& position, PointI& outScreenPoint) const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), true);
    if (!ok)
        return false;

    if (!static_cast<const Frustum2DI64*>(&internalState.globalFrustum2D31)->test(position))
        return false;

    const auto offsetFromTarget31 = position - state.target31;
    const auto offsetFromTarget = Utilities::convert31toDouble(offsetFromTarget31, state.zoomLevel);
    const auto positionInWorld = glm::vec3(
        offsetFromTarget.x * AtlasMapRenderer::TileSize3D,
        0.0f, // TODO: this is not compatible with elevation
        offsetFromTarget.y * AtlasMapRenderer::TileSize3D);

    const auto projectedPosition = glm_extensions::project(
        positionInWorld,
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport);
    outScreenPoint.x = projectedPosition.x;
    outScreenPoint.y = state.windowSize.y - projectedPosition.y;
    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::obtainScreenPointFromPosition(const PointI& position31, PointI& outScreenPoint, bool checkOffScreen) const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), true);
    if (!ok)
        return false;

    if (!checkOffScreen && !internalState.globalFrustum2D31.test(position31))
        return false;

    const auto offsetFromTarget31 = position31 - state.target31;
    const auto offsetFromTarget = Utilities::convert31toFloat(offsetFromTarget31, state.zoomLevel);
    const auto positionInWorld = glm::vec3(
        offsetFromTarget.x * AtlasMapRenderer::TileSize3D,
        0.0f, // TODO: this is not compatible with elevation
        offsetFromTarget.y * AtlasMapRenderer::TileSize3D);

    const auto projectedPosition = glm_extensions::project(
        positionInWorld,
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport);
    outScreenPoint.x = projectedPosition.x;
    outScreenPoint.y = state.windowSize.y - projectedPosition.y;
    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::obtainElevatedPointFromPosition(const PointI& position31,
    PointI& outScreenPoint, bool checkOffScreen) const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), true);
    if (!ok)
        return false;

    if (!checkOffScreen && !internalState.globalFrustum2D31.test(position31))
        return false;

    float height = getHeightOfLocation(state, position31);
    const auto offsetFromTarget31 = position31 - state.target31;
    const auto offsetFromTarget = Utilities::convert31toFloat(offsetFromTarget31, state.zoomLevel);
    const auto positionInWorld = glm::vec3(
        offsetFromTarget.x * AtlasMapRenderer::TileSize3D,
        height,
        offsetFromTarget.y * AtlasMapRenderer::TileSize3D);

    const auto projectedPosition = glm_extensions::project(
        positionInWorld,
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport);
    outScreenPoint.x = projectedPosition.x;
    outScreenPoint.y = state.windowSize.y - projectedPosition.y;
    return true;
}

float OsmAnd::AtlasMapRenderer_OpenGL::getCameraHeightInMeters() const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), true);

    return ok ? internalState.distanceFromCameraToGroundInMeters : 0.0f;
}

double OsmAnd::AtlasMapRenderer_OpenGL::getTileSizeInMeters() const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), true);

    const auto metersPerTile = Utilities::getMetersPerTileUnit(state.zoomLevel, internalState.targetTileId.y, 1);

    return ok ? metersPerTile : 0.0;
}

double OsmAnd::AtlasMapRenderer_OpenGL::getPixelsToMetersScaleFactor() const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), true);

    const auto tileSizeOnScreenInPixels = internalState.referenceTileSizeOnScreenInPixels * internalState.tileOnScreenScaleFactor;
    const auto metersPerPixel = Utilities::getMetersPerTileUnit(state.zoomLevel, internalState.targetTileId.y, tileSizeOnScreenInPixels);

    return ok ? metersPerPixel : 0.0;
}

double OsmAnd::AtlasMapRenderer_OpenGL::getPixelsToMetersScaleFactor(const MapRendererState& state, const MapRendererInternalState& internalState_) const
{
    const auto internalState = static_cast<const InternalState*>(&internalState_);
    const auto tileSizeOnScreenInPixels = internalState->referenceTileSizeOnScreenInPixels * internalState->tileOnScreenScaleFactor;
    const auto metersPerPixel = Utilities::getMetersPerTileUnit(state.zoomLevel, internalState->targetTileId.y, tileSizeOnScreenInPixels);
    return metersPerPixel;
}

OsmAnd::AtlasMapRendererSkyStage* OsmAnd::AtlasMapRenderer_OpenGL::createSkyStage()
{
    return new AtlasMapRendererSkyStage_OpenGL(this);
}

OsmAnd::AtlasMapRendererMapLayersStage* OsmAnd::AtlasMapRenderer_OpenGL::createMapLayersStage()
{
    return new AtlasMapRendererMapLayersStage_OpenGL(this);
}

OsmAnd::AtlasMapRendererSymbolsStage* OsmAnd::AtlasMapRenderer_OpenGL::createSymbolsStage()
{
    return new AtlasMapRendererSymbolsStage_OpenGL(this);
}

OsmAnd::AtlasMapRendererDebugStage* OsmAnd::AtlasMapRenderer_OpenGL::createDebugStage()
{
    return new AtlasMapRendererDebugStage_OpenGL(this);
}
