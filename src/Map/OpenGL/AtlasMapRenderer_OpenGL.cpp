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
// Set minimum earth angle for advanced horizon
const double OsmAnd::AtlasMapRenderer_OpenGL::_minimumAngleForAdvancedHorizon = M_PI / 12.0;
// Set distance-per-angle factor for smooth horizon slide beyond minimum earth angle
const double OsmAnd::AtlasMapRenderer_OpenGL::_distancePerAngleFactor = 1.1648753803647327974577447574825e-6;
// Set minimum height of visible sky for colouring
const double OsmAnd::AtlasMapRenderer_OpenGL::_minimumSkyHeightInKilometers = 8.0;
// Set the maximum height of terrain to render
const double OsmAnd::AtlasMapRenderer_OpenGL::_maximumHeightFromGroundInMeters = 10000.0;

OsmAnd::AtlasMapRenderer_OpenGL::AtlasMapRenderer_OpenGL(GPUAPI_OpenGL* const gpuAPI_)
    : AtlasMapRenderer(
        gpuAPI_,
        std::unique_ptr<const MapRendererConfiguration>(new AtlasMapRendererConfiguration()),
        std::unique_ptr<const MapRendererDebugSettings>(new MapRendererDebugSettings()))
    , depthBufferRange(_depthBufferRange)
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

    glClearColor(0.937f, 0.906f, 0.906f, 1.0f);
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

    _debugStage->clear();

    // Setup viewport
    glViewport(_internalState.glmViewport[0], _internalState.glmViewport[1], _internalState.glmViewport[2], _internalState.glmViewport[3]);
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

    // Turn on depth testing as early as for sky stage since with disabled depth test, write to depth buffer is blocked
    // but since sky is on top of everything, accept all fragments regardless of depth value
    glEnable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;
    glDepthFunc(GL_ALWAYS);
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

    // Render map symbols without writing depth buffer, since symbols use own sorting and intersection checking
    if (!currentDebugSettings->disableSymbolsStage)
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
        GLint depthBits;
        glGetIntegerv(GL_DEPTH_BITS, &depthBits);
        GL_CHECK_RESULT;

        _depthBufferRange = static_cast<double>(1ull << depthBits);

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
    const MapRendererConfiguration& configuration_) const
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
    internalState->worldCameraPosition = (_internalState.mCameraViewInv * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz();
    internalState->groundCameraPosition = internalState->worldCameraPosition.xz();

    // Get camera global coordinates and height
    PointD groundPos, offsetInTileN;
    groundPos.x = internalState->groundCameraPosition.x / TileSize3D + internalState->targetInTileOffsetN.x;
    groundPos.y = internalState->groundCameraPosition.y / TileSize3D + internalState->targetInTileOffsetN.y;
    offsetInTileN.x = groundPos.x - floor(groundPos.x);
    offsetInTileN.y = groundPos.y - floor(groundPos.y);
    TileId tileId;
    tileId.x = floor(groundPos.x) + internalState->targetTileId.x;
    tileId.y = floor(groundPos.y) + internalState->targetTileId.y;
    const auto tileIdN = Utilities::normalizeTileId(tileId, state.zoomLevel);
    groundPos.x = static_cast<double>(tileIdN.x) + offsetInTileN.x;
    groundPos.y = static_cast<double>(tileIdN.y) + offsetInTileN.y;
    const auto metersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, groundPos.y, TileSize3D);
    internalState->metersPerUnit = metersPerUnit;
    internalState->distanceFromCameraToGroundInMeters =
        qMax(0.0, static_cast<double>(internalState->distanceFromCameraToGround) * metersPerUnit);
    internalState->cameraCoordinates.x = Utilities::getLongitudeFromTile(state.zoomLevel, groundPos.x);
    internalState->cameraCoordinates.y = Utilities::getLatitudeFromTile(state.zoomLevel, groundPos.y);
    
    // Calculate distance to horizon
    auto inglobeAngle = qAcos(_radius / (_radius + internalState->distanceFromCameraToGroundInMeters));
    const auto skylineAngle = qMax(0.0, elevationAngleInRadians - inglobeAngle);
    // Tweak angle for low zoom levels
    inglobeAngle = inglobeAngle < _minimumAngleForAdvancedHorizon ? inglobeAngle :
        _distancePerAngleFactor * internalState->distanceFromCameraToGroundInMeters;
    const auto groundDistanceToHorizonInMeters = _radius * inglobeAngle;   

    // Get the farthest edge for terrain to render
    const auto distanceFromCameraToTarget = static_cast<double>(internalState->distanceFromCameraToTarget);
    const auto distanceFromTargetToHorizon = groundDistanceToHorizonInMeters / metersPerUnit -
        static_cast<double>(internalState->groundDistanceFromCameraToTarget);
    const auto additionalDistanceToSkyplane =
        qMax(0.01, qMin(static_cast<double>(state.fogConfiguration.distanceToFog) *
        internalState->scaleToRetainProjectedSize, distanceFromTargetToHorizon) * elevationCosine);
    const auto distanceToSkyplane = distanceFromCameraToTarget + additionalDistanceToSkyplane;
    internalState->zSkyplane = static_cast<float>(distanceToSkyplane);
    internalState->skyShift = static_cast<float>(additionalDistanceToSkyplane * elevationSine / elevationCosine);
    internalState->distanceFromCameraToFog = qSqrt(internalState->zSkyplane * internalState->zSkyplane +
        internalState->skyShift * internalState->skyShift);
    internalState->distanceFromTargetToFog = static_cast<float>(additionalDistanceToSkyplane);

    // Calculate approximate height of the visible sky
    internalState->skyHeightInKilometers = static_cast<float>(
        (internalState->distanceFromCameraToGroundInMeters / 1000.0 + _minimumSkyHeightInKilometers) *
        qTan(internalState->fovInRadians));

    // Calculate maximum renderable distance from camera
    const auto zNear = static_cast<double>(_zNear);
    // Get depth buffer value range (shouldn't be less than 2^16)
    const auto zRange = qMax(_depthBufferRange, 65536.0);
    // Calculate zFar to let skyplane have the maximum z for a particular depth value
    // in order to avoid z-fighting with terrain at the far end
    internalState->zFar = static_cast<float>(
        zNear / (1.0 - (1.0 - zNear / distanceToSkyplane) * zRange / (zRange - 1.50001)));

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

    // Correct fog distance
    internalState->correctedFogDistance = state.fogConfiguration.distanceToFog * internalState->scaleToRetainProjectedSize
        + (internalState->distanceFromCameraToTarget - internalState->groundDistanceFromCameraToTarget);

    // Calculate skyplane size
    float zSkyplaneK = internalState->zSkyplane / _zNear;
    internalState->skyplaneSize.x = zSkyplaneK * internalState->projectionPlaneHalfWidth;
    internalState->skyplaneSize.y = zSkyplaneK * internalState->projectionPlaneHalfHeight;

    // Determine skyline position
    internalState->skyLine =
        qMax(0.0f, static_cast<float>(distanceToSkyplane * qTan(skylineAngle)) - internalState->skyShift) /
        internalState->skyplaneSize.y;


    // Update frustum
    updateFrustum(internalState, state);

    // Compute visible tileset
    computeVisibleTileset(internalState, state);

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

    // 4 points of frustum far clipping box in camera coordinate space
    const auto zFar = internalState->zFar;
    const auto zFarK = zFar / _zNear;
    const glm::vec4 fTL_c(zFarK * nTL_c.x, zFarK * nTL_c.y, zFarK * nTL_c.z, 1.0f);
    const glm::vec4 fTR_c(zFarK * nTR_c.x, zFarK * nTR_c.y, zFarK * nTR_c.z, 1.0f);
    const glm::vec4 fBL_c(zFarK * nBL_c.x, zFarK * nBL_c.y, zFarK * nBL_c.z, 1.0f);
    const glm::vec4 fBR_c(zFarK * nBR_c.x, zFarK * nBR_c.y, zFarK * nBR_c.z, 1.0f);

    // Transform 8 frustum vertices + camera center to global space
    const auto eye_g = internalState->mCameraViewInv * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    const auto fTL_g = internalState->mCameraViewInv * fTL_c;
    const auto fTR_g = internalState->mCameraViewInv * fTR_c;
    const auto fBL_g = internalState->mCameraViewInv * fBL_c;
    const auto fBR_g = internalState->mCameraViewInv * fBR_c;
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

    // Get extra tiling field for elevated terrain
    int extraIntersectionsCounter = state.elevationAngle < 90.0f - state.fieldOfView ? 0 : -1;
    glm::vec2 extraIntersections[4];

    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nBL_g.xyz(), fBL_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
        if (extraIntersectionsCounter == 0)
            extraIntersections[extraIntersectionsCounter++] = intersectionPoint.xz();
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nBR_g.xyz(), fBR_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
        if (extraIntersectionsCounter == 1)
            extraIntersections[extraIntersectionsCounter++] = intersectionPoint.xz();
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTR_g.xyz(), fTR_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTL_g.xyz(), fTL_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, fTR_g.xyz(), fBR_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, fTL_g.xyz(), fBL_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTL_g.xyz(), nBL_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
    }
    if (intersectionPointsCounter < 4 &&
        Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTR_g.xyz(), nBR_g.xyz(), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz();
        intersectionPointsCounter++;
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
    const auto maxTerrainHeight = static_cast<float>(_maximumHeightFromGroundInMeters / internalState->metersPerUnit);

    // Get intersection points on elevated plane
    if (internalState->distanceFromCameraToGround > maxTerrainHeight)
    {
        const glm::vec3 planeE(0.0f, maxTerrainHeight, 0.0f);
        if (extraIntersectionsCounter == 2 &&
            Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeE, eye_g.xyz(), fBL_g.xyz(), intersectionPoint))
        {
            extraIntersections[extraIntersectionsCounter] = intersectionPoint.xz();
            extraIntersectionsCounter++;
        }
        if (extraIntersectionsCounter == 3 &&
            Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeE, eye_g.xyz(), fBR_g.xyz(), intersectionPoint))
        {
            extraIntersections[extraIntersectionsCounter] = intersectionPoint.xz();
            extraIntersectionsCounter++;
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
}

void OsmAnd::AtlasMapRenderer_OpenGL::computeVisibleTileset(InternalState* internalState, const MapRendererState& state) const
{
    // Normalize 2D-frustum points to tiles
    PointF p[4];
    p[0] = internalState->frustum2D.p0 / TileSize3D;
    p[1] = internalState->frustum2D.p1 / TileSize3D;
    p[2] = internalState->frustum2D.p2 / TileSize3D;
    p[3] = internalState->frustum2D.p3 / TileSize3D;

    // Determine visible tileset
    internalState->visibleTiles.resize(0);
    bool mainPart = true;
    bool extraPart = false;
    while (mainPart || extraPart)
    {
        // "Round"-up tile indices
        // In-tile normalized position is added, since all tiles are going to be
        // translated in opposite direction during rendering
        for(int i = 0; i < 4; i++) {
            p[i].x += internalState->targetInTileOffsetN.x ;
            p[i].y += internalState->targetInTileOffsetN.y ;
        }

        QSet<TileId> visibleTiles;
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
                tileId.x = x + internalState->targetTileId.x;
                tileId.y = y - 1 + internalState->targetTileId.y;
                visibleTiles.insert(tileId);
            }
            pxMin = xMin;
            pxMax = xMax;
        }

        for (const auto& tileId : constOf(visibleTiles))
            internalState->visibleTiles.push_back(tileId);

        // Add the bottom tileset
        extraPart = mainPart && !isnan(internalState->extraField2D.p0.x);
        if (extraPart)
        {
            p[0] = internalState->extraField2D.p0 / TileSize3D;
            p[1] = internalState->extraField2D.p1 / TileSize3D;
            p[2] = internalState->extraField2D.p2 / TileSize3D;
            p[3] = internalState->extraField2D.p3 / TileSize3D;
        }

        mainPart = false;
    }

    // Normalize and make unique visible tiles
    QSet<TileId> uniqueTiles;
    for (const auto& tileId : constOf(internalState->visibleTiles))
        uniqueTiles.insert(Utilities::normalizeTileId(tileId, state.zoomLevel));
    internalState->uniqueTiles.resize(0);
    for (const auto& tileId : constOf(uniqueTiles))
        internalState->uniqueTiles.push_back(tileId);

    // Sort visible tiles by distance from target
    std::sort(internalState->uniqueTiles,
        [internalState]
        (const TileId& l, const TileId& r) -> bool
        {
            const auto lx = l.x - internalState->targetTileId.x;
            const auto ly = l.y - internalState->targetTileId.y;

            const auto rx = r.x - internalState->targetTileId.x;
            const auto ry = r.y - internalState->targetTileId.y;

            return (lx*lx + ly*ly) < (rx*rx + ry*ry);
        });
}

OsmAnd::GPUAPI_OpenGL* OsmAnd::AtlasMapRenderer_OpenGL::getGPUAPI() const
{
    return static_cast<OsmAnd::GPUAPI_OpenGL*>(gpuAPI.get());
}

float OsmAnd::AtlasMapRenderer_OpenGL::getTileSizeOnScreenInPixels() const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, getState(), *getConfiguration());

    return ok ? internalState.referenceTileSizeOnScreenInPixels * internalState.tileOnScreenScaleFactor : 0.0;
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
    bool ok = updateInternalState(internalState, state, *getConfiguration());
    if (!ok)
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
    float distance;
    const auto intersects = Utilities_OpenGL_Common::rayIntersectPlane(planeN, planeO, rayD, nearInWorld, distance);
    if (!intersects)
        return false;

    auto intersection = nearInWorld + distance*rayD;
    intersection /= static_cast<float>(TileSize3D);

    double x = intersection.x + internalState.targetInTileOffsetN.x;
    double y = intersection.z + internalState.targetInTileOffsetN.y;

    const auto zoomLevelDiff = ZoomLevel::MaxZoomLevel - state.zoomLevel;
    const auto tileSize31 = (1u << zoomLevelDiff);
    x *= tileSize31;
    y *= tileSize31;

    location.x = static_cast<int64_t>(x)+(internalState.targetTileId.x << zoomLevelDiff);
    location.y = static_cast<int64_t>(y)+(internalState.targetTileId.y << zoomLevelDiff);

    return true;
}

OsmAnd::AreaI OsmAnd::AtlasMapRenderer_OpenGL::getVisibleBBox31() const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, getState(), *getConfiguration());
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
    bool ok = updateInternalState(internalState, getState(), *getConfiguration());
    if (!ok)
        return false;

    return static_cast<const Frustum2DI64*>(&internalState.globalFrustum2D31)->test(position);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isPositionVisible(const PointI& position31) const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, getState(), *getConfiguration());
    if (!ok)
        return false;

    return internalState.globalFrustum2D31.test(position31);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::obtainScreenPointFromPosition(const PointI64& position, PointI& outScreenPoint) const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration());
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
    bool ok = updateInternalState(internalState, state, *getConfiguration());
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

double OsmAnd::AtlasMapRenderer_OpenGL::getTileSizeInMeters() const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration());

    const auto metersPerTile = Utilities::getMetersPerTileUnit(state.zoomLevel, internalState.targetTileId.y, 1);

    return ok ? metersPerTile : 0.0;
}

double OsmAnd::AtlasMapRenderer_OpenGL::getPixelsToMetersScaleFactor() const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration());

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
