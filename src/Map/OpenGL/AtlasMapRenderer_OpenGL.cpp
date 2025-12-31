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
#include "AtlasMapRendererMap3DObjectsStage_OpenGL.h"
#include "IMapRenderer.h"
#include "IMapTiledDataProvider.h"
#include "IRasterMapLayerProvider.h"
#include "IMapElevationDataProvider.h"
#include "Logging.h"
#include "Stopwatch.h"
#include "GlmExtensions.h"
#include "Utilities.h"

#include "OpenGL/Utilities_OpenGL.h"
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#define MIN_COS_SURFACE 0.03

// Set camera's near depth limit
const float OsmAnd::AtlasMapRenderer_OpenGL::_zNear = 1.0f;
// Set average radius of Earth
const double OsmAnd::AtlasMapRenderer_OpenGL::_radius = 6371e3;
// Set maximum height of terrain to render
const double OsmAnd::AtlasMapRenderer_OpenGL::_maximumHeightFromSeaLevelInMeters = 10000.0;
// Set maximum depth of terrain to render
const double OsmAnd::AtlasMapRenderer_OpenGL::_maximumDepthFromSeaLevelInMeters = 12000.0;
// Set minimal distance factor for tiles of each detail level
const double OsmAnd::AtlasMapRenderer_OpenGL::_detailDistanceFactor = 3.0 * TileSize3D * M_SQRT2;
// Set invalid value for elevation of terrain
const float OsmAnd::AtlasMapRenderer_OpenGL::_invalidElevationValue = -20000.0f;
// Set minimum visual zoom
const float OsmAnd::AtlasMapRenderer_OpenGL::_minimumVisualZoom = 0.7f; // -0.6 fractional part of float zoom
// Set maximum visual zoom
const float OsmAnd::AtlasMapRenderer_OpenGL::_maximumVisualZoom = 1.6f; // 0.6 fractional part of float zoom
// Set minimum elevation angle
const double OsmAnd::AtlasMapRenderer_OpenGL::_minimumElevationAngle = 10.0f;
// Set minimum elevation angle
const OsmAnd::ZoomLevel OsmAnd::AtlasMapRenderer_OpenGL::_zoomForFlattening = ZoomLevel22;

OsmAnd::AtlasMapRenderer_OpenGL::AtlasMapRenderer_OpenGL(GPUAPI_OpenGL* const gpuAPI_)
    : AtlasMapRenderer(
        gpuAPI_,
        std::unique_ptr<const MapRendererConfiguration>(new AtlasMapRendererConfiguration()),
        std::unique_ptr<const MapRendererDebugSettings>(new MapRendererDebugSettings()))
{
}

OsmAnd::AtlasMapRenderer_OpenGL::~AtlasMapRenderer_OpenGL()
{
}

bool OsmAnd::AtlasMapRenderer_OpenGL::doInitializeRendering(bool reinitialize)
{
    GL_CHECK_PRESENT(glClearColor);

    const auto gpuAPI = getGPUAPI();

    bool ok;

    ok = AtlasMapRenderer::doInitializeRendering(reinitialize);
    if (!ok)
        return false;

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::doRenderFrame(IMapRenderer_Metrics::Metric_renderFrame* const metric_)
{
    const auto gpuAPI = getGPUAPI();

    bool ok = true;
    bool skip = false;

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
    const double viewportScale = getViewportScale();
    const glm::vec2 viewportShift = getViewportShift();

    const auto scaleCenterX = (_internalState.glmViewport[2] * 0.5 - viewportShift.x) * (viewportScale - 1);
    const auto scaleCentery = (_internalState.glmViewport[3] * 0.5 - viewportShift.y) * (viewportScale - 1);
    const auto shiftX = (_internalState.glmViewport[2] - _internalState.glmViewport[2] * viewportScale) * 0.5 + scaleCenterX;
    const auto shiftY = (_internalState.glmViewport[3] - _internalState.glmViewport[3] * viewportScale) * 0.5 + scaleCentery;

    const auto x = _internalState.glmViewport[0] + shiftX;
    const auto y = _internalState.glmViewport[1] + shiftY;
    const auto w = _internalState.glmViewport[2] * viewportScale;
    const auto h = _internalState.glmViewport[3] * viewportScale;

    glViewport(x, y, w, h);
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

    // Render the sky
    if (!currentDebugSettings->disableSkyStage)
    {
        Stopwatch skyStageStopwatch(metric != nullptr);
        if (_skyStage->render(metric) != MapRendererStage::StageResult::Success)
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

        glCullFace(currentState.flip ? GL_BACK : GL_FRONT);
        GL_CHECK_RESULT;

        Stopwatch mapLayersStageStopwatch(metric != nullptr);
        const auto stageResult = _mapLayersStage->render(metric);
        if (stageResult == MapRendererStage::StageResult::Fail)
            ok = false;
        if (metric)
            metric->elapsedTimeForMapLayersStage = mapLayersStageStopwatch.elapsed();

        // Continue to render everything
        glDisable(GL_CULL_FACE);
        GL_CHECK_RESULT;

        if (!ok || stageResult == MapRendererStage::StageResult::Wait)
            skip = true;
    }

    // Turn on blending since now objects with transparency are going to be rendered
    glEnable(GL_BLEND);
    GL_CHECK_RESULT;

    if (!skip && !currentDebugSettings->disableSymbolsStage && !qFuzzyIsNull(currentState.symbolsOpacity))
    {
        Stopwatch symbolsPreapareStageStopwatch(metric != nullptr);

        auto stageResult = _symbolsStage->prepareSymbols(metric);

        if (stageResult == MapRendererStage::StageResult::Success)
            stageResult = _symbolsStage->renderWithDepth(metric);

        if (stageResult == MapRendererStage::StageResult::Fail)
            ok = false;

        if (!ok || stageResult == MapRendererStage::StageResult::Wait)
            skip = true;

        if (metric)
        {
            metric->elapsedTimeForSymbolsStage = symbolsPreapareStageStopwatch.elapsed();
        }
    }

    if (!skip && _map3DObjectsStage && !currentDebugSettings->disable3DMapObjectsStage)
    {
        Stopwatch map3DStageStopwatch(metric != nullptr);

        const auto stageResult = _map3DObjectsStage->render(metric);

        if (stageResult == MapRendererStage::StageResult::Fail)
            ok = false;
        if (metric)
            metric->elapsedTimeForObjects3DStage += map3DStageStopwatch.elapsed();

        if (!ok || stageResult == MapRendererStage::StageResult::Wait)
            skip = true;
    }

    // Render map symbols without writing depth buffer, since symbols use own sorting and intersection checking
    if (!skip && !currentDebugSettings->disableSymbolsStage && !qFuzzyIsNull(currentState.symbolsOpacity))
    {
        // Set premultiplied alpha color blending
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        Stopwatch symbolsDrawStageStopwatch(metric != nullptr);
        const auto stageResult = _symbolsStage->render(metric);
        if (stageResult == MapRendererStage::StageResult::Fail)
            ok = false;
        if (metric)
        {
            metric->elapsedTimeForSymbolsStage += symbolsDrawStageStopwatch.elapsed();
            // Disable for now
            //_symbolsStage->drawDebugMetricSymbol(metric);
        }

        glDepthMask(GL_TRUE);
        GL_CHECK_RESULT;

        if (!ok || stageResult == MapRendererStage::StageResult::Wait)
            skip = true;
    }

    // Restore straight color blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_RESULT;

    //TODO: render special fog object some day

    // Render debug stage
    if (!skip && currentDebugSettings->debugStageEnabled)
    {
        glDisable(GL_DEPTH_TEST);
        GL_CHECK_RESULT;

        Stopwatch debugStageStopwatch(metric != nullptr);
        const auto stageResult = _debugStage->render(metric);
        if (stageResult == MapRendererStage::StageResult::Fail)
            ok = false;
        if (metric)
            metric->elapsedTimeForDebugStage = debugStageStopwatch.elapsed();

        glEnable(GL_DEPTH_TEST);
        GL_CHECK_RESULT;
    }

    // Turn off blending
    glDisable(GL_BLEND);
    GL_CHECK_RESULT;

    GL_POP_GROUP_MARKER;

    return ok && !skip;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::doReleaseRendering(bool gpuContextLost)
{
    return AtlasMapRenderer::doReleaseRendering(gpuContextLost);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::handleStateChange(const MapRendererState& state, MapRendererStateChanges mask)
{
    bool ok = AtlasMapRenderer::handleStateChange(state, mask);

    return ok;
}

void OsmAnd::AtlasMapRenderer_OpenGL::flushRenderCommands()
{
#if defined(__APPLE__) && (TARGET_OS_IPHONE)
    // On iOS, presenting the renderbuffer flushes the pipeline.
    // Avoid explicit glFlush to prevent driver assertions/crashes in AppleMetalGLRenderer.
    return;
#else
    glFlush();
    GL_CHECK_RESULT;
#endif
}

void OsmAnd::AtlasMapRenderer_OpenGL::onValidateResourcesOfType(const MapRendererResourceType type)
{
    AtlasMapRenderer::onValidateResourcesOfType(type);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::updateInternalState(
    MapRendererInternalState& outInternalState_,
    const MapRendererState& state,
    const MapRendererConfiguration& configuration_,
    const CalculationSteps neededSteps /* = Complete */) const
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
    const auto fovTangent = qTan(internalState->fovInRadians);
    internalState->projectionPlaneHalfHeight = static_cast<float>(static_cast<double>(_zNear) * fovTangent);
    internalState->sizeOfPixelInWorld = internalState->projectionPlaneHalfHeight * 2.0f / state.viewport.height();
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
    internalState->scaleToRetainProjectedSize = 1.0f / internalState->tileOnScreenScaleFactor;
    internalState->pixelInWorldProjectionScale = static_cast<float>(AtlasMapRenderer::TileSize3D)
        / internalState->referenceTileSizeOnScreenInPixels * internalState->scaleToRetainProjectedSize;

    // Setup camera
    internalState->mDistance = glm::translate(glm::vec3(0.0f, 0.0f, -internalState->distanceFromCameraToTarget));
    internalState->mElevation = glm::rotate(glm::radians(state.elevationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    internalState->mAzimuth = glm::rotate(glm::radians(state.azimuth), glm::vec3(0.0f, 1.0f, 0.0f));
    internalState->mCameraView = internalState->mDistance * internalState->mElevation * internalState->mAzimuth;

    // Get inverse camera
    internalState->mDistanceInv = glm::translate(glm::vec3(0.0f, 0.0f, internalState->distanceFromCameraToTarget));
    internalState->mElevationInv = glm::rotate(glm::radians(-state.elevationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    internalState->mAzimuthInv = glm::rotate(glm::radians(-state.azimuth), glm::vec3(0.0f, 1.0f, 0.0f));
    internalState->mCameraViewInv =
        internalState->mAzimuthInv * internalState->mElevationInv * internalState->mDistanceInv;

    // Get camera position
    internalState->worldCameraPosition = (internalState->mCameraViewInv * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz();

    if (neededSteps == RequiredToUnproject)
        return true;

    // Calculate synthetic parameters to make map flat if needed
    const auto shiftToFlatten = state.flatEarth ? qMax(_zoomForFlattening - state.zoomLevel, 0) : 0;
    internalState->synthZoomLevel = static_cast<ZoomLevel>(state.zoomLevel + shiftToFlatten);

    // Calculate current global scale factor (meters per unit)
    internalState->synthTileId = internalState->targetTileId;
    auto synthTarget31 = state.target31;
    if (state.flatEarth && shiftToFlatten > 0)
    {
        if (internalState->synthZoomLevel > state.zoomLevel)
        {
            const auto toCenter = 1 << (internalState->synthZoomLevel - 1);
            internalState->synthTileId.x |= toCenter;
            internalState->synthTileId.y |= toCenter;
        }
        const auto tileShift =
            PointD(internalState->targetInTileOffsetN) * static_cast<double>(1 << (ZoomLevel31 - _zoomForFlattening));
        const auto zShift = ZoomLevel31 - internalState->synthZoomLevel;
        synthTarget31.x = (internalState->synthTileId.x << zShift) + static_cast<int>(qFloor(tileShift.x));
        synthTarget31.y = (internalState->synthTileId.y << zShift) + static_cast<int>(qFloor(tileShift.y));
    }
    auto targetY = static_cast<double>(internalState->targetTileId.y) + internalState->targetInTileOffsetN.y;
    auto metersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, targetY, TileSize3D);
    internalState->metersPerUnit = metersPerUnit;
    const auto synthScaleFactor31 = state.flatEarth && shiftToFlatten > 0
        ? metersPerUnit / Utilities::getMetersPerTileUnit(internalState->synthZoomLevel, synthTarget31, TileSize3D)
        : 1.0;

    // Calculate globe rotation matrix
    const auto radiusInWorld = _radius * synthScaleFactor31 / metersPerUnit;
    internalState->globeRadius = radiusInWorld;
    const auto angles = Utilities::getAnglesFrom31(synthTarget31);
    internalState->mGlobeRotationPrecise = Utilities::getGlobalRotationMatrix(angles);
    internalState->mGlobeRotation = glm::mat3(internalState->mGlobeRotationPrecise);
    internalState->mGlobeRotationWithRadius = glm::mat3(internalState->mGlobeRotationPrecise * radiusInWorld);
    internalState->mGlobeRotationPreciseInv = glm::transpose(internalState->mGlobeRotationPrecise);

    // Get camera coordinates
    const auto vectorFromTargetToCamera = glm::dvec3(internalState->worldCameraPosition);
    const auto distanceToTarget = glm::length(vectorFromTargetToCamera);
    const auto vectorFromTargetToCameraN = vectorFromTargetToCamera / distanceToTarget;
    const auto vectorToCamera = vectorFromTargetToCamera + glm::dvec3(0.0, radiusInWorld, 0.0);
    const auto distanceToCamera = qMax(glm::length(vectorToCamera), radiusInWorld);

    //TODO: Certain methods need to be refactored to avoid using this old flat-earth value
    const auto distanceFromCameraToGround =
        static_cast<double>(internalState->distanceFromCameraToTarget * elevationSine);
    //const auto distanceFromCameraToGround = distanceToCamera - radiusInWorld;

    internalState->distanceFromCameraToGround = static_cast<float>(distanceFromCameraToGround);

    const auto vectorToCameraN = vectorToCamera / distanceToCamera;
    const auto groundDistanceFromCameraToTarget =
        qAcos(qBound(0.0, vectorToCameraN.y, 1.0)) * radiusInWorld;
    internalState->cameraRotatedPosition = internalState->mGlobeRotationPreciseInv * (vectorToCamera / radiusInWorld);
    internalState->cameraRotatedDirection = internalState->mGlobeRotationPreciseInv * vectorFromTargetToCameraN;
    const auto camPosition = internalState->mGlobeRotationPreciseInv * vectorToCameraN;
    auto camAngles = PointD(qAtan2(camPosition.x, camPosition.y), -qAsin(qBound(-1.0, camPosition.z, 1.0)));
    internalState->cameraAngles = camAngles;

    //TODO: Certain methods need to be refactored to avoid using this old flat-earth value
    internalState->groundCameraPosition = internalState->worldCameraPosition.xz();

    if (state.flatEarth)
    {
        PointD groundPos(
            internalState->worldCameraPosition.x / TileSize3D + internalState->targetInTileOffsetN.x,
            internalState->worldCameraPosition.z / TileSize3D + internalState->targetInTileOffsetN.y);
        PointD offsetInTileN(groundPos.x - qFloor(groundPos.x), groundPos.y - qFloor(groundPos.y));
        PointI64 tileId(qFloor(groundPos.x) + internalState->targetTileId.x,
            qFloor(groundPos.y) + internalState->targetTileId.y);
        const auto tileIdN = Utilities::normalizeCoordinates(tileId, state.zoomLevel);
        groundPos.x = static_cast<double>(tileIdN.x) + offsetInTileN.x;
        groundPos.y = static_cast<double>(tileIdN.y) + offsetInTileN.y;
        internalState->distanceFromCameraToGroundInMeters = qMax(0.0,
            distanceFromCameraToGround * Utilities::getMetersPerTileUnit(state.zoomLevel, groundPos.y, TileSize3D));
        internalState->cameraCoordinates = LatLon(Utilities::getLatitudeFromTile(state.zoomLevel, groundPos.y),
            Utilities::getLongitudeFromTile(state.zoomLevel, groundPos.x));
    }
    else
    {
        camAngles *= 180.0 / M_PI;
        internalState->cameraCoordinates = LatLon(camAngles.y, camAngles.x);
        internalState->distanceFromCameraToGroundInMeters = distanceFromCameraToGround * metersPerUnit;
    }

    if (neededSteps == RequiredToProject)
        return true;

    // Calculate maximum visible distance
    const auto factorOfDistance = radiusInWorld / distanceToCamera;
    internalState->factorOfDistance = static_cast<float>(factorOfDistance);
    const auto inglobeAngle = qAcos(qBound(0.0, factorOfDistance, 1.0));
    const auto cameraAngle = M_PI_2 - inglobeAngle;
    const auto targetAngle = qAcos(qBound(0.0, glm::dot(vectorFromTargetToCameraN, vectorToCameraN), 1.0));
    const auto distanceFromCameraToHorizon = distanceToCamera * qCos(cameraAngle);
    const auto groundDistanceToHorizon = inglobeAngle * radiusInWorld;
    internalState->tilesToHorizon =
        static_cast<int>(qCeil(qMin(static_cast<float>(groundDistanceToHorizon), state.visibleDistance) / TileSize3D));
    const auto groundDistanceFromTargetToSkyplane = groundDistanceToHorizon - groundDistanceFromCameraToTarget;
    const auto targetHorizonAngle = cameraAngle - targetAngle;
    const auto distanceFromTargetToHorizonZ =
        distanceFromCameraToHorizon * qCos(targetHorizonAngle) - distanceToTarget;
    const auto farEnd = qMin(distanceFromTargetToHorizonZ,
        static_cast<double>(state.visibleDistance * internalState->scaleToRetainProjectedSize) * elevationCosine)
        * elevationCosine + distanceFromTargetToHorizonZ * (1.0 - elevationCosine);
    const auto deepEnd = static_cast<double>(internalState->scaleToRetainProjectedSize) * elevationSine
        * qMin(_maximumDepthFromSeaLevelInMeters / metersPerUnit, static_cast<double>(state.visibleDistance));
    const auto additionalDistanceToZFar = qMax(0.1, qMax(farEnd, deepEnd));
    auto zFar = distanceToTarget + additionalDistanceToZFar;
    internalState->zFar = static_cast<float>(zFar);
    internalState->zNear = _zNear;
    internalState->isNotTopDownProjection = targetAngle > internalState->fovInRadians;
    
    // Calculate distance for tiles of high detail
    const auto distanceToScreenTop = internalState->distanceFromCameraToTarget *
        static_cast<float>(qSin(internalState->fovInRadians) /
        qMax(0.01, qSin(elevationAngleInRadians - internalState->fovInRadians)));
    const auto visibleDistance = qMin(distanceToScreenTop,
        static_cast<float>(qMin(groundDistanceFromTargetToSkyplane, additionalDistanceToZFar / elevationCosine)));
    const auto highDetail = state.detailedDistance * internalState->distanceFromCameraToTarget /
        internalState->scaleToRetainProjectedSize * static_cast<float>(fovTangent)
        + static_cast<float>(_detailDistanceFactor / 3.0);
    const auto zFactor = state.zoomLevel < ZoomLevel4 && !state.flatEarth ? 0.0f : 1.0f;
    const auto zLowerDetail = highDetail < visibleDistance
        ? qMin(internalState->zFar, internalState->distanceFromCameraToTarget + static_cast<float>(qMax(0.01,
            static_cast<double>(highDetail) * elevationCosine))) * zFactor + (1.0f - zFactor) * internalState->zFar
        : internalState->zFar;

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

    if (neededSteps == LocalGeometry)
    {
        // 4 points of frustum near clipping box in camera coordinate space
        const auto planeHalfWidth = internalState->projectionPlaneHalfWidth;
        const auto planeHalfHeight = internalState->projectionPlaneHalfHeight;
        const glm::vec4 nTL_c(-planeHalfWidth, +planeHalfHeight, -_zNear, 1.0f);
        const glm::vec4 nTR_c(+planeHalfWidth, +planeHalfHeight, -_zNear, 1.0f);
        const glm::vec4 nBL_c(-planeHalfWidth, -planeHalfHeight, -_zNear, 1.0f);
        const glm::vec4 nBR_c(+planeHalfWidth, -planeHalfHeight, -_zNear, 1.0f);

        // Transform 4 frustum vertices + camera center to global space
        const auto eye_g = internalState->worldCameraPosition;
        const auto nTL_g = internalState->mCameraViewInv * nTL_c;
        const auto nTR_g = internalState->mCameraViewInv * nTR_c;
        const auto nBL_g = internalState->mCameraViewInv * nBL_c;
        const auto nBR_g = internalState->mCameraViewInv * nBR_c;

        const auto frontVisibleEdgeN =
            glm::normalize(glm::cross(nBL_g.xyz() - nTL_g.xyz(), nTR_g.xyz() - nTL_g.xyz()));
        internalState->frontVisibleEdgeN = frontVisibleEdgeN;
        internalState->frontVisibleEdgeD = glm::dot(frontVisibleEdgeN, nTL_g.xyz());

        return true;
    }

    // Calculate skyplane size
    float zSkyplaneK = internalState->zFar / _zNear;
    internalState->skyplaneSize.x = zSkyplaneK * internalState->projectionPlaneHalfWidth;
    internalState->skyplaneSize.y = zSkyplaneK * internalState->projectionPlaneHalfHeight;

    // Determine skyline position and fog parameters    
    const auto skyplaneHeight = static_cast<double>(internalState->skyplaneSize.y);
    const auto skyLine = zFar * qTan(qMax(0.0, targetHorizonAngle));
    internalState->skyLine = static_cast<float>(qMax(0.0, skyLine) / skyplaneHeight);
    const auto skyTargetToCenter = radiusInWorld * elevationCosine;
    internalState->skyTargetToCenter = static_cast<float>(skyTargetToCenter / skyplaneHeight);
    const auto extraGap = radiusInWorld * elevationSine - additionalDistanceToZFar;
    const auto skyShift = qSqrt(radiusInWorld * radiusInWorld - extraGap * extraGap) - skyTargetToCenter;
    const auto distanceFromCameraToFog = qSqrt(skyShift * skyShift + zFar * zFar);
    internalState->distanceFromCameraToFog = static_cast<float>(distanceFromCameraToFog);
    internalState->distanceFromTargetToFog = static_cast<float>(additionalDistanceToZFar);
    internalState->fogShiftFactor = static_cast<float>(qBound(0.0, (skyLine - skyShift) / TileSize3D, 1.0));
    const auto fogAngle = targetAngle + qAcos(qBound(0.0, zFar / distanceFromCameraToFog, 1.0));
    const auto distanceFromCameraToCenter = distanceFromCameraToFog * qCos(fogAngle);
    const auto distanceFromFogToCenter = distanceFromCameraToFog * qSin(fogAngle);
    const auto skyBackToCenter =
        distanceFromFogToCenter * zFar / qCos(targetAngle) / distanceFromCameraToCenter - qMax(0.0, skyShift);
    internalState->skyBackToCenter = static_cast<float>(skyBackToCenter / skyplaneHeight);

    // Calculate height of the visible sky
    internalState->skyHeightInKilometers =
        static_cast<float>((zFar * fovTangent - skyLine) * metersPerUnit / 1000.0);

    // Compute visible area
    computeVisibleArea(internalState, state, zLowerDetail, state.flatEarth ? 0.0 : elevationCosine, neededSteps);

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

OsmAnd::AtlasMapRendererInternalState_OpenGL OsmAnd::AtlasMapRenderer_OpenGL::getRequiredInternalState(
    bool& result, MapRendererState* ptrState /* = nullptr*/) const
{
    result = true;

    MapRendererState state;
    auto pState = ptrState != nullptr ? ptrState : &state;

    bool isFresh;
    *pState = getFreshState(isFresh);

    if (isFresh)
    {
        QWriteLocker scopedLocker(&_requiredInternalStateLock);

        result = updateInternalState(
            _requiredInternalState, *pState, *getConfiguration(), FullGeometry);

        return _requiredInternalState;
    }

    QReadLocker scopedLocker(&_requiredInternalStateLock);

    return _requiredInternalState;
}



void OsmAnd::AtlasMapRenderer_OpenGL::computeVisibleArea(InternalState* internalState, const MapRendererState& state,
    const float lowerDetail, const double tiltFactor, const CalculationSteps neededSteps) const
{
    using namespace Utilities_OpenGL_Common;

    const auto globeRadius = internalState->globeRadius;

    // 4 points of frustum near clipping box in camera coordinate space
    const auto planeHalfWidth = internalState->projectionPlaneHalfWidth;
    const auto planeHalfHeight = internalState->projectionPlaneHalfHeight;
    const glm::vec4 nTL_c(-planeHalfWidth, +planeHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nTR_c(+planeHalfWidth, +planeHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nBL_c(-planeHalfWidth, -planeHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nBR_c(+planeHalfWidth, -planeHalfHeight, -_zNear, 1.0f);

    // 4 points of frustum lower detail plane in camera coordinate space
    const auto zMid = lowerDetail;
    const auto zMidK = zMid / _zNear;
    const glm::vec4 mTL_c(zMidK * nTL_c.x, zMidK * nTL_c.y, -zMid, 1.0f);
    const glm::vec4 mTR_c(zMidK * nTR_c.x, zMidK * nTR_c.y, -zMid, 1.0f);
    const glm::vec4 mBL_c(zMidK * nBL_c.x, zMidK * nBL_c.y, -zMid, 1.0f);
    const glm::vec4 mBR_c(zMidK * nBR_c.x, zMidK * nBR_c.y, -zMid, 1.0f);

    // Transform 8 frustum vertices + camera center to global space
    const auto eye_g = internalState->worldCameraPosition;
    const auto mTL_g = internalState->mCameraViewInv * mTL_c;
    const auto mTR_g = internalState->mCameraViewInv * mTR_c;
    const auto mBL_g = internalState->mCameraViewInv * mBL_c;
    const auto mBR_g = internalState->mCameraViewInv * mBR_c;
    const auto nTL_g = internalState->mCameraViewInv * nTL_c;
    const auto nTR_g = internalState->mCameraViewInv * nTR_c;
    const auto nBL_g = internalState->mCameraViewInv * nBL_c;
    const auto nBR_g = internalState->mCameraViewInv * nBR_c;

    // Get normals and distances to frustum edges
    const auto rayTL = glm::dvec3(mTL_g.xyz() - eye_g);
    const auto rayTR = glm::dvec3(mTR_g.xyz() - eye_g);
    const auto rayBL = glm::dvec3(mBL_g.xyz() - eye_g);
    const auto rayBR = glm::dvec3(mBR_g.xyz() - eye_g);
    const auto topVisibleEdgeN = glm::normalize(glm::cross(rayTR, rayTL));
    const auto leftVisibleEdgeN = glm::normalize(glm::cross(rayTL, rayBL));
    const auto bottomVisibleEdgeN = glm::normalize(glm::cross(rayBL, rayBR));
    const auto rightVisibleEdgeN = glm::normalize(glm::cross(rayBR, rayTR));
    const auto frontVisibleEdgeN = glm::normalize(glm::cross(nBL_g.xyz() - nTL_g.xyz(), nTR_g.xyz() - nTL_g.xyz()));
    const auto backVisibleEdgeN = glm::normalize(glm::cross(mBR_g.xyz() - mTR_g.xyz(), mTL_g.xyz() - mTR_g.xyz()));
    internalState->topVisibleEdgeN = glm::vec3(topVisibleEdgeN);
    internalState->leftVisibleEdgeN = glm::vec3(leftVisibleEdgeN);
    internalState->bottomVisibleEdgeN = glm::vec3(bottomVisibleEdgeN);
    internalState->rightVisibleEdgeN = glm::vec3(rightVisibleEdgeN);
    internalState->frontVisibleEdgeN = frontVisibleEdgeN;
    internalState->backVisibleEdgeN = backVisibleEdgeN;
    internalState->frontVisibleEdgeP = nTL_g.xyz();
    internalState->topVisibleEdgeD = glm::dot(internalState->topVisibleEdgeN, eye_g);
    internalState->leftVisibleEdgeD = glm::dot(internalState->leftVisibleEdgeN, eye_g);
    internalState->bottomVisibleEdgeD = glm::dot(internalState->bottomVisibleEdgeN, eye_g);
    internalState->rightVisibleEdgeD = glm::dot(internalState->rightVisibleEdgeN, eye_g);
    internalState->frontVisibleEdgeD = glm::dot(frontVisibleEdgeN, nTL_g.xyz());
    internalState->backVisibleEdgeD = glm::dot(backVisibleEdgeN, mTR_g.xyz());
    internalState->extraElevation = 0.0f;
    internalState->maxElevation = 0.0f;

    const auto defPoint = PointI64(INT64_MIN, INT64_MIN);
    internalState->elevatedFrustum2D31.p0 = defPoint;
    internalState->elevatedFrustum2D31.p1 = defPoint;
    internalState->elevatedFrustum2D31.p2 = defPoint;
    internalState->elevatedFrustum2D31.p3 = defPoint;
    internalState->targetOffset = PointI64(0, 0);

    // Calculate visible area for plane
    if (state.flatEarth)
    {
        // Get (up to) 4 points of frustum edges & plane intersection
        const glm::vec3 planeN(0.0f, 1.0f, 0.0f);
        const glm::vec3 planeO(0.0f, 0.0f, 0.0f);
        auto intersectionPointsCounter = 0u;
        glm::vec3 intersectionPoint;
        glm::vec2 intersectionPoints[4];

        // Get extra tiling field for elevated terrain
        int extraIntersectionsCounter = state.elevationAngle < 90.0f - state.fieldOfView ? 0 : -1;
        glm::vec2 extraIntersections[4];

        if (intersectionPointsCounter < 4 && lineSegmentIntersectPlane(
            planeN, planeO, nBL_g.xyz(), mBL_g.xyz(), intersectionPoint))
        {
            intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
            if (extraIntersectionsCounter == 0)
                extraIntersections[extraIntersectionsCounter++] = intersectionPoint.xz();
        }
        if (intersectionPointsCounter < 4 && lineSegmentIntersectPlane(
            planeN, planeO, nBR_g.xyz(), mBR_g.xyz(), intersectionPoint))
        {
            intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
            if (extraIntersectionsCounter == 1)
                extraIntersections[extraIntersectionsCounter++] = intersectionPoint.xz();
        }
        if (intersectionPointsCounter < 4 && lineSegmentIntersectPlane(
            planeN, planeO, nTR_g.xyz(), mTR_g.xyz(), intersectionPoint))
            intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
        if (intersectionPointsCounter < 4 && lineSegmentIntersectPlane(
            planeN, planeO, nTL_g.xyz(), mTL_g.xyz(), intersectionPoint))
            intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
        if (intersectionPointsCounter < 4 && lineSegmentIntersectPlane(
            planeN, planeO, mTR_g.xyz(), mBR_g.xyz(), intersectionPoint))
            intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
        if (intersectionPointsCounter < 4 && lineSegmentIntersectPlane(
            planeN, planeO, mTL_g.xyz(), mBL_g.xyz(), intersectionPoint))
            intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
        if (intersectionPointsCounter < 4 && lineSegmentIntersectPlane(
            planeN, planeO, nTL_g.xyz(), nBL_g.xyz(), intersectionPoint))
            intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
        if (intersectionPointsCounter < 4 && lineSegmentIntersectPlane(
            planeN, planeO, nTR_g.xyz(), nBR_g.xyz(), intersectionPoint))
            intersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();

        Frustum2DF frustum2D;
        Frustum2D31 frustum2D31;
        frustum2D.p0 = PointF(intersectionPoints[0].x, intersectionPoints[0].y);
        frustum2D.p1 = PointF(intersectionPoints[1].x, intersectionPoints[1].y);
        frustum2D.p2 = PointF(intersectionPoints[2].x, intersectionPoints[2].y);
        frustum2D.p3 = PointF(intersectionPoints[3].x, intersectionPoints[3].y);

        const auto tileSize31 = (1u << (ZoomLevel::MaxZoomLevel - state.zoomLevel));
        frustum2D31.p0 = PointI64((frustum2D.p0 / TileSize3D) * static_cast<double>(tileSize31));
        frustum2D31.p1 = PointI64((frustum2D.p1 / TileSize3D) * static_cast<double>(tileSize31));
        frustum2D31.p2 = PointI64((frustum2D.p2 / TileSize3D) * static_cast<double>(tileSize31));
        frustum2D31.p3 = PointI64((frustum2D.p3 / TileSize3D) * static_cast<double>(tileSize31));

        internalState->globalFrustum2D31 = frustum2D31 + state.target31;

        // Get maximum height of terrain below camera
        const auto maxTerrainHeight =
            static_cast<float>(_maximumHeightFromSeaLevelInMeters / internalState->metersPerUnit);
        
        // Get intersection points on elevated plane
        if (internalState->distanceFromCameraToGround > maxTerrainHeight)
        {
            const glm::vec3 planeE(0.0f, maxTerrainHeight, 0.0f);
            if (extraIntersectionsCounter == 2 && lineSegmentIntersectPlane(
                planeN, planeE, eye_g, mBR_g.xyz(), intersectionPoint))
                extraIntersections[extraIntersectionsCounter++] = intersectionPoint.xz();
            if (extraIntersectionsCounter == 3 && lineSegmentIntersectPlane(
                planeN, planeE, eye_g, mBL_g.xyz(), intersectionPoint))
                extraIntersections[extraIntersectionsCounter++] = intersectionPoint.xz();
        }
        else if (extraIntersectionsCounter == 2)
        {
            extraIntersections[extraIntersectionsCounter++] = eye_g.xz();
            extraIntersections[extraIntersectionsCounter++] = eye_g.xz();
        }

        if (extraIntersectionsCounter == 4)
        {
            frustum2D.p0 = PointF(extraIntersections[0].x, extraIntersections[0].y);
            frustum2D.p1 = PointF(extraIntersections[1].x, extraIntersections[1].y);
            frustum2D.p2 = PointF(extraIntersections[2].x, extraIntersections[2].y);
            frustum2D.p3 = PointF(extraIntersections[3].x, extraIntersections[3].y);
            internalState->extraFrustum2D31.p0 =
                PointI64((frustum2D.p0 / TileSize3D) * static_cast<double>(tileSize31)) + state.target31;
            internalState->extraFrustum2D31.p1 =
                PointI64((frustum2D.p1 / TileSize3D) * static_cast<double>(tileSize31)) + state.target31;
            internalState->extraFrustum2D31.p2 =
                PointI64((frustum2D.p2 / TileSize3D) * static_cast<double>(tileSize31)) + state.target31;
            internalState->extraFrustum2D31.p3 =
                PointI64((frustum2D.p3 / TileSize3D) * static_cast<double>(tileSize31)) + state.target31;
        }
        else
            internalState->extraFrustum2D31 = Frustum2D31();

        if (neededSteps == FullGeometry)
            return;
    }

    const auto checkVert = internalState->isNotTopDownProjection;
    const auto ca = internalState->cameraAngles;
    const auto camPos = internalState->cameraRotatedPosition;
    const auto camDir = -internalState->cameraRotatedDirection;
    const auto cameraDistance = glm::length(camPos);
    const auto cameraHeight = cameraDistance - 1.0;
    const auto backN = camPos / cameraDistance;
    const auto camDown = -backN;
    const auto camTL = glm::normalize(internalState->mGlobeRotationPreciseInv * rayTL);
    const auto camTR = glm::normalize(internalState->mGlobeRotationPreciseInv * rayTR);
    const auto camBL = glm::normalize(internalState->mGlobeRotationPreciseInv * rayBL);
    const auto camBR = glm::normalize(internalState->mGlobeRotationPreciseInv * rayBR);
    const auto topN = internalState->mGlobeRotationPreciseInv * glm::dvec3(topVisibleEdgeN);
    const auto leftN = internalState->mGlobeRotationPreciseInv * glm::dvec3(leftVisibleEdgeN);
    const auto bottomN = internalState->mGlobeRotationPreciseInv * glm::dvec3(bottomVisibleEdgeN);
    const auto rightN = internalState->mGlobeRotationPreciseInv * glm::dvec3(rightVisibleEdgeN);
    const auto botLeftN = glm::normalize(glm::cross(camBL, camDown));
    const auto botRightN = glm::normalize(glm::cross(camDown, camBR));
    const auto topD = glm::dot(topN, camPos);
    const auto leftD = glm::dot(leftN, camPos);
    const auto bottomD = glm::dot(bottomN, camPos);
    const auto rightD = glm::dot(rightN, camPos);
    const auto botLeftD = glm::dot(botLeftN, camPos);
    const auto botRightD = glm::dot(botRightN, camPos);
    bool withNorthPole = false;
    bool withSouthPole = false;

    // Calculate visible area for sphere
    if (!state.flatEarth)
    {
        const auto backP = backN * (1.0 / cameraDistance);
        const auto backD = glm::dot(backN, backP);
        double length;
        auto rayD = camTL;
        if (!rayIntersectPlane(backN, backP, rayD, camPos, length) || length < 0.0)
        {
            rayD = glm::normalize((camTL + camBL) / 2.0);
            rayIntersectPlane(backN, backP, rayD, camPos, length);
        }
        const auto MTL = camPos + rayD * length;
        rayD = camTR;
        if (!rayIntersectPlane(backN, backP, rayD, camPos, length) || length < 0.0)
        {
            rayD = glm::normalize((camTR + camBR) / 2.0);
            rayIntersectPlane(backN, backP, rayD, camPos, length);
        }
        const auto MTR = camPos + rayD * length;
        rayIntersectPlane(backN, backP, camBL, camPos, length);
        const auto MBL = camPos + camBL * length;
        rayIntersectPlane(backN, backP, camBR, camPos, length);
        const auto MBR = camPos + camBR * length;
        auto minVectorX = glm::dvec2(0.0, 0.0);
        auto maxVectorX = minVectorX;
        auto minCoordY = std::numeric_limits<double>::infinity();
        auto maxCoordY = -minCoordY;
        glm::dvec3 FTL, FTR, FBL, FBR;
        const bool withFTL = rayIntersectsSphere(camPos, camTL, 1.0, FTL);
        const bool withFTR = rayIntersectsSphere(camPos, camTR, 1.0, FTR);
        const bool withFBL = rayIntersectsSphere(camPos, camBL, 1.0, FBL);
        const bool withFBR = rayIntersectsSphere(camPos, camBR, 1.0, FBR);
        const auto TRTL = glm::normalize(MTL - MTR);
        const auto TLBL = glm::normalize(MBL - MTL);
        const auto BLBR = glm::normalize(MBR - MBL);
        const auto BRTR = glm::normalize(MTR - MBR);
        glm::dvec3 HTL, HTR, HBL, HBR, VTL, VTR, VBL, VBR, FD;
        bool withHBL = false, withHBR = false;

        bool checkExtra = false;
        if (withFBL && withFBR && withFTR && withFTL)
        {
            planeIntersectsSphere(bottomN, bottomD, minVectorX, maxVectorX, minCoordY, maxCoordY, &FBL, &FBR);
            planeIntersectsSphere(rightN, rightD, minVectorX, maxVectorX, minCoordY, maxCoordY, &FBR, &FTR);
            planeIntersectsSphere(topN, topD, minVectorX, maxVectorX, minCoordY, maxCoordY, &FTR, &FTL);
            planeIntersectsSphere(leftN, leftD, minVectorX, maxVectorX, minCoordY, maxCoordY, &FTL, &FBL);
            checkExtra = true;
        }
        else if (withFBL && withFBR)
        {
            planeIntersectsSphere(bottomN, bottomD, minVectorX, maxVectorX, minCoordY, maxCoordY, &FBL, &FBR);
            auto& p = FBR;
            if (rayIntersectsSphere(MTR, -BRTR, 1.0, VTR, true))
            {
                planeIntersectsSphere(rightN, rightD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &VTR);
                p = VTR;
            }
            if (rayIntersectsSphere(MTR, TRTL, 1.0, HTR, true))
            {
                planeIntersectsSphere(backN, backD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &HTR);
                p = HTR;
                if (rayIntersectsSphere(MTL, -TRTL, 1.0, HTL, true))
                {
                    planeIntersectsSphere(topN, topD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &HTL);
                    p = HTL;
                }
            }
            if (rayIntersectsSphere(MTL, TLBL, 1.0, VTL, true))
            {
                planeIntersectsSphere(backN, backD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &VTL);
                planeIntersectsSphere(leftN, leftD, minVectorX, maxVectorX, minCoordY, maxCoordY, &VTL, &FBL);
            }
            checkExtra = true;
        }
        else
        {
            withHBL = rayIntersectsSphere(MBL, BLBR, 1.0, HBL, true);
            withHBR = rayIntersectsSphere(MBR, -BLBR, 1.0, HBR, true);
            if (withHBL && withHBR)
            {
                planeIntersectsSphere(bottomN, bottomD, minVectorX, maxVectorX, minCoordY, maxCoordY, &HBL, &HBR);
                if (isPointVisible(glm::dvec3(0.0),
                    topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, true, true, false, true))
                {
                    auto& p = HBR;
                    if (rayIntersectsSphere(MBR, BRTR, 1.0, VBR, true))
                    {
                        planeIntersectsSphere(backN, backD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &VBR);
                        p = VBR;
                        if (rayIntersectsSphere(MTR, -BRTR, 1.0, VTR, true))
                        {
                            planeIntersectsSphere(
                                rightN, rightD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &VTR);
                            p = VTR;
                        }
                    }
                    if (rayIntersectsSphere(MTR, TRTL, 1.0, HTR, true))
                    {
                        planeIntersectsSphere(backN, backD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &HTR);
                        p = HTR;
                        if (rayIntersectsSphere(MTL, -TRTL, 1.0, HTL, true))
                        {
                            planeIntersectsSphere(topN, topD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &HTL);
                            p = HTL;
                        }
                    }
                    if (rayIntersectsSphere(MTL, TLBL, 1.0, VTL, true))
                    {
                        planeIntersectsSphere(backN, backD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &VTL);
                        p = VTL;
                        if (rayIntersectsSphere(MBL, -TLBL, 1.0, VBL, true))
                        {
                            planeIntersectsSphere(
                                leftN, leftD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &VBL);
                            p = VBL;
                        }
                    }
                    planeIntersectsSphere(backN, backD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &HBL);
                }
                else
                {
                    planeIntersectsSphere(backN, backD, minVectorX, maxVectorX, minCoordY, maxCoordY, &HBR, &HBL);
                    checkExtra = true;
                }
            }
            else if (rayIntersectsSphere(MBR, BRTR, 1.0, VBR, true) && rayIntersectsSphere(MTR, -BRTR, 1.0, VTR, true))
            {
                planeIntersectsSphere(rightN, rightD, minVectorX, maxVectorX, minCoordY, maxCoordY, &VBR, &VTR);
                auto& p = VTR;
                if (rayIntersectsSphere(MTL, TLBL, 1.0, VTL, true))
                {
                    planeIntersectsSphere(backN, backD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &VTL);
                    p = VTL;
                    if (rayIntersectsSphere(MBL, -TLBL, 1.0, VBL, true))
                    {
                        planeIntersectsSphere(leftN, leftD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &VBL);
                        p = VBL;
                    }
                }
                planeIntersectsSphere(backN, backD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &VBR);
            }
            else if (rayIntersectsSphere(MTL, TLBL, 1.0, VTL, true) && rayIntersectsSphere(MBL, -TLBL, 1.0, VBL, true))
            {
                planeIntersectsSphere(leftN, leftD, minVectorX, maxVectorX, minCoordY, maxCoordY, &VTL, &VBL);
                auto& p = VBL;
                if (rayIntersectsSphere(MBR, BRTR, 1.0, VBR, true))
                {
                    planeIntersectsSphere(backN, backD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &VBR);
                    p = VBR;
                    if (rayIntersectsSphere(MTR, -BRTR, 1.0, VTR, true))
                    {
                        planeIntersectsSphere(rightN, rightD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &VTR);
                        p = VTR;
                    }
                }
                planeIntersectsSphere(backN, backD, minVectorX, maxVectorX, minCoordY, maxCoordY, &p, &VTL);
            }
            else
                planeIntersectsSphere(backN, backD, minVectorX, maxVectorX, minCoordY, maxCoordY);
        }

        if (checkExtra && checkVert && rayIntersectsSphere(camPos, camDown, 1.0, FD))
        {
            if (withFBL && withFBR)
            {
                planeIntersectsSphere(botLeftN, botLeftD, minVectorX, maxVectorX, minCoordY, maxCoordY, &FD, &FBL);
                planeIntersectsSphere(botRightN, botRightD, minVectorX, maxVectorX, minCoordY, maxCoordY, &FD, &FBR);
            }
            else if (withHBL && withHBR)
            {
                const auto hBotLeftN = glm::normalize(glm::cross(HBL - camPos, camDown));
                const auto hBotRightN = glm::normalize(glm::cross(camDown, HBR - camPos));
                const auto hBotLeftD = glm::dot(hBotLeftN, FD);
                const auto hBotRightD = glm::dot(hBotRightN, FD);
                planeIntersectsSphere(hBotLeftN, hBotLeftD, minVectorX, maxVectorX, minCoordY, maxCoordY, &FD, &HBL);
                planeIntersectsSphere(hBotRightN, hBotRightD, minVectorX, maxVectorX, minCoordY, maxCoordY, &FD, &HBR);
            }
        }

        const auto npN = glm::dvec3(0.0, 0.0, -1.0);
        const auto spN = glm::dvec3(0.0, 0.0, 1.0);
        withNorthPole = glm::dot(glm::normalize(npN - camPos), npN) < 0.0 && isPointVisible(
            npN, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, false);
        withSouthPole = !withNorthPole && glm::dot(glm::normalize(spN - camPos), spN) < 0.0 && isPointVisible(
            spN, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, false);

        if (withNorthPole || withSouthPole)
        {
            minCoordY = withNorthPole ? -1.0 : minCoordY;
            maxCoordY = withSouthPole ? 1.0 : maxCoordY;
            minVectorX = maxVectorX = glm::dvec2(0.0, -1.0);
        }

        const auto minAngleX = qAtan2(minVectorX.x, minVectorX.y);
        const auto maxAngleX = qAtan2(maxVectorX.x, maxVectorX.y);
        const auto minAngleY = -qAsin(qBound(-1.0, maxCoordY, 1.0));
        const auto maxAngleY = -qAsin(qBound(-1.0, minCoordY, 1.0));
        auto min64 = Utilities::get64FromAngles(PointD(minAngleX, maxAngleY));
        auto max64 = Utilities::get64FromAngles(PointD(maxAngleX, minAngleY));
        if (max64.x == 0)
            max64.x = INT32_MAX;
        if (min64.x >= max64.x)
            min64.x = min64.x - INT32_MAX - 1;

        internalState->globalFrustum2D31.p0.x = min64.x;
        internalState->globalFrustum2D31.p0.y = max64.y;
        internalState->globalFrustum2D31.p1 = max64;
        internalState->globalFrustum2D31.p2.x = max64.x;
        internalState->globalFrustum2D31.p2.y = min64.y;
        internalState->globalFrustum2D31.p3 = min64;
        internalState->extraFrustum2D31 = Frustum2D31();

        if (neededSteps == FullGeometry)
            return;
    }

    // Compute visible tileset
    const auto zLower = static_cast<double>(lowerDetail) / globeRadius;
    const auto zFar = static_cast<double>(internalState->zFar) / globeRadius;
    const auto detailThickness = _detailDistanceFactor / globeRadius;
    const auto extraDetailDistance = static_cast<float>(
        static_cast<double>(internalState->distanceFromCameraToTarget) / globeRadius / 2.0);

    const auto zoomDelta = internalState->synthZoomLevel - state.zoomLevel;
    const PointI tileDelta(internalState->synthTileId.x - internalState->targetTileId.x,
        internalState->synthTileId.y - internalState->targetTileId.y);

    const auto dirAngle = static_cast<double>(state.azimuth) * M_PI / 180.0;
    const auto dirX = qSin(dirAngle);
    const auto dirY = qCos(dirAngle);

    auto maxDistance = 0.0;
    auto zoomLevel = internalState->synthZoomLevel;
    QMap<int32_t, QSet<TileId>> tilesN;
    QMap<int32_t, QHash<TileId, TileVisibility>> tiles, testTiles1, testTiles2;
    const auto sZoom = ZoomLevel31 - zoomLevel;
    const auto cameraTile31 = Utilities::get31FromAngles(internalState->cameraAngles);
    auto camTileId = TileId::fromXY(cameraTile31.x >> sZoom, cameraTile31.y >> sZoom);
    PointI toCamera(camTileId.x - internalState->synthTileId.x, camTileId.y - internalState->synthTileId.y);
    if (!state.flatEarth && zoomLevel < ZoomLevel9)
    {
        toCamera.x = 0;
        toCamera.y = 0;
    }
    if (abs(toCamera.x) == 1)
        toCamera.x = 0;
    if (abs(toCamera.y) == 1)
        toCamera.y = 0;
    if (toCamera.x > 0 && dirX > 0.0)
        toCamera.x -= 1 << zoomLevel;
    else if (toCamera.x < 0 && dirX < 0.0)
        toCamera.x += 1 << zoomLevel;
    if (toCamera.y > 0 && dirY < 0.0)
        toCamera.y -= 1 << zoomLevel;
    else if (toCamera.y < 0 && dirY > 0.0)
        toCamera.y += 1 << zoomLevel;
    camTileId = TileId::fromXY(internalState->synthTileId.x + toCamera.x, internalState->synthTileId.y + toCamera.y);
    const PointD tilesToTarget(toCamera);
    const auto distanceLimit = tilesToTarget.norm() - 1.0;
    bool lookForStrictlyVisible = distanceLimit > 0.0;
    auto targetTileId = lookForStrictlyVisible ? camTileId : internalState->synthTileId;
    testTiles1[zoomLevel].insert(targetTileId, NotTestedYet);
    glm::dvec3 ITL, ITR, IBL, IBR, OTL, OTR, OBL, OBR;
    bool visTL, visTM, visTR, visML, visMR, visBL, visBM, visBR;
    auto procTiles = &testTiles1;
    auto nextTiles = &testTiles2;
    const auto topLeft = internalState->globalFrustum2D31.p3;
    const auto bottomRight = internalState->globalFrustum2D31.p1;
    auto topLeftEx = topLeft;
    auto bottomRightEx = bottomRight;
    topLeftEx.x += 1ll + INT32_MAX;
    bottomRightEx.x += 1ll + INT32_MAX;
    double distanceFromTarget = 0.0;
    bool atLeastOneVisibleFound = false;
    bool atLeastOneFlatVisibleFound = false;
    bool shouldRepeat = true;
    while (shouldRepeat)
    {
        shouldRepeat = false;
        bool atLeastOneAdded = false;    
        for (const auto& setEntry : rangeOf(constOf(*procTiles)))
        {
            const auto currentZoom = setEntry.key();
            const auto currentZoomLevel = static_cast<ZoomLevel>(currentZoom);
            const auto zoomShift = MaxZoomLevel - currentZoomLevel;
            const auto zShift = zoomLevel - currentZoom;
            const auto tileSize31 = 1u << zoomShift;
            const auto realZoomLevel = static_cast<ZoomLevel>(currentZoom - zoomDelta);
            const auto rShift = state.zoomLevel - realZoomLevel;
            for (const auto& tileEntry : rangeOf(constOf(setEntry.value())))
            {
                const auto tileId = tileEntry.key();
                const auto tileState = tileEntry.value();
                const auto tileIdN = Utilities::normalizeTileId(tileId, currentZoomLevel);
                if (tilesN[currentZoom].contains(tileIdN))
                    continue;
                float minHeightInWorld = 0.0f;
                float maxHeightInWorld = 0.0f;
                const auto realTileIdN = state.flatEarth ? Utilities::normalizeTileId(TileId::fromXY(
                    ((tileIdN.x << rShift) - tileDelta.x) >> rShift, ((tileIdN.y << rShift) - tileDelta.y) >> rShift),
                        realZoomLevel) : tileIdN;
                getHeightLimits(state, realTileIdN, internalState->metersPerUnit,
                    realZoomLevel, minHeightInWorld, maxHeightInWorld);
                const auto minHeight = static_cast<double>(minHeightInWorld) / globeRadius;
                const auto maxHeight = static_cast<double>(maxHeightInWorld) / globeRadius;
                const auto minD = minHeight + 1.0;
                const auto maxD = maxHeight + 1.0;
                PointI tile31(tileIdN.x << zoomShift, tileIdN.y << zoomShift);
                bool isOverX = tileSize31 + static_cast<uint32_t>(tile31.x) > INT32_MAX;
                bool isOverY = tileSize31 + static_cast<uint32_t>(tile31.y) > INT32_MAX;
                PointI nextTile31(
                    isOverX ? INT32_MAX : tile31.x + tileSize31, isOverY ? INT32_MAX : tile31.y + tileSize31);
                const auto angTL = Utilities::getAnglesFrom31(tile31);
                const auto angBR = Utilities::getAnglesFrom31(nextTile31);
                const auto normalTL = Utilities::getGlobeRadialVector(angTL);
                const auto normalTR = Utilities::getGlobeRadialVector(PointD(angBR.x, angTL.y));
                const auto normalBL = Utilities::getGlobeRadialVector(PointD(angTL.x, angBR.y));
                const auto normalBR = Utilities::getGlobeRadialVector(angBR);
                if (minHeight != 0.0 || maxHeight != 0.0)
                {
                    OTL = normalTL * maxD;
                    OTR = normalTR * maxD;
                    OBL = normalBL * maxD;
                    OBR = normalBR * maxD;
                }

                // Check tile visibility
                bool isNorthPoleTile = tile31.y == 0;
                bool isSouthPoleTile = nextTile31.y == INT32_MAX;
                bool isFlatVisible = false;
                bool isVisible = false;
                do
                {
                    if (zoomLevel < ZoomLevel2)
                    {
                        isVisible = true;
                        break;
                    }

                    // Add border tiles near the poles
                    if (!currentState.flatEarth
                        && ((isNorthPoleTile && topLeft.y < 0) || (isSouthPoleTile && bottomRight.y > INT32_MAX))
                        && ((topLeft.x <= nextTile31.x && tile31.x <= bottomRight.x)
                            || (topLeftEx.x <= nextTile31.x && tile31.x <= bottomRightEx.x)))
                    {
                        isVisible = true;
                        break;
                    }

                    // Check occlusion of all corners of the tile:
                    // a tile should be considered invisible if all its corners are hidden by the horizon
                    if (glm::dot(glm::normalize(normalTL - camPos), normalTL) > 0.0
                        && glm::dot(glm::normalize(normalTR - camPos), normalTR) > 0.0
                        && glm::dot(glm::normalize(normalBL - camPos), normalBL) > 0.0
                        && glm::dot(glm::normalize(normalBR - camPos), normalBR) > 0.0)
                        break;

                    // Check map border tiles near the poles
                    if (!currentState.flatEarth && ((isNorthPoleTile && internalState->globalFrustum2D31.p2.y == 0)
                        || (isSouthPoleTile && internalState->globalFrustum2D31.p0.y == INT32_MAX))
                        && ((static_cast<int64_t>(tile31.x) < internalState->globalFrustum2D31.p2.x
                        && static_cast<int64_t>(nextTile31.x) > internalState->globalFrustum2D31.p0.x)
                        || (internalState->globalFrustum2D31.p0.x < 0
                        && static_cast<int64_t>(tile31.x) < internalState->globalFrustum2D31.p2.x + INT32_MAX + 1
                        && static_cast<int64_t>(nextTile31.x) > internalState->globalFrustum2D31.p0.x + INT32_MAX + 1))
                        )
                    {
                        isVisible = true;
                        break;
                    }

                    visTL = false;
                    visTM = false;
                    visTR = false;
                    visML = false;
                    visMR = false;
                    visBL = false;
                    visBM = false;
                    visBR = false;
        
                    // The tile should be considered visible
                    // if it was found out during the visibility check of its neighbour
                    if (minHeight == 0.0 && maxHeight == 0.0 && tileState == AlmostVisible)
                    {
                        isVisible = true;
                        break;
                    }

                    // Check position of the camera, which may be put inside the tile:
                    // in this case, the tile should be considered visible
                    if (cameraHeight <= maxHeight
                        && ca.x >= angTL.x && ca.x <= angBR.x && ca.y <= angTL.y && ca.y >= angBR.y)
                    {
                        isVisible = true;
                        break;
                    }

                    // Check distance of base corners of the tile:
                    // a tile should be considered invisible if its base corners are too far
                    if (zoomLevel > MinZoomLevel
                        && glm::dot(camDir, normalTL - camPos) > zFar && glm::dot(camDir, normalTR - camPos) > zFar
                        && glm::dot(camDir, normalBL - camPos) > zFar && glm::dot(camDir, normalBR - camPos) > zFar)
                        break;

                    // Check visibility of base corners of the tile:
                    // a tile should be considered flat visible if any of these corners is visible
                    if (isPointVisible(normalTL, topN, leftN, checkVert ? botLeftN : bottomN, rightN, topD, leftD,
                        checkVert ? botLeftD : bottomD, rightD, false, false, false, false,
                        checkVert ? &botRightN : nullptr, checkVert ? &botRightD : nullptr))
                    {
                        if (minHeight == 0.0)
                        {
                            visTL = true;
                            visTM = true;
                            visML = true;
                            isVisible = true;
                            break;
                        }
                        else
                            isFlatVisible = true;
                    }
                    if (isPointVisible(normalTR, topN, leftN, checkVert ? botLeftN : bottomN, rightN, topD, leftD,
                        checkVert ? botLeftD : bottomD, rightD, false, false, false, false,
                        checkVert ? &botRightN : nullptr, checkVert ? &botRightD : nullptr))
                    {
                        if (minHeight == 0.0)
                        {
                            visTM = true;
                            visTR = true;
                            visMR = true;
                            isVisible = true;
                            break;
                        }
                        else
                            isFlatVisible = true;
                    }
                    if (isPointVisible(normalBL, topN, leftN, checkVert ? botLeftN : bottomN, rightN, topD, leftD,
                        checkVert ? botLeftD : bottomD, rightD, false, false, false, false,
                        checkVert ? &botRightN : nullptr, checkVert ? &botRightD : nullptr))
                    {
                        if (minHeight == 0.0)
                        {
                            visML = true;
                            visBL = true;
                            visBM = true;
                            isVisible = true;
                            break;
                        }
                        else
                            isFlatVisible = true;
                    }
                    if (isPointVisible(normalBR, topN, leftN, checkVert ? botLeftN : bottomN, rightN, topD, leftD,
                        checkVert ? botLeftD : bottomD, rightD, false, false, false, false,
                        checkVert ? &botRightN : nullptr, checkVert ? &botRightD : nullptr))
                    {
                        if (minHeight == 0.0)
                        {
                            visMR = true;
                            visBM = true;
                            visBR = true;
                            isVisible = true;
                            break;
                        }
                        else
                            isFlatVisible = true;
                    }

                    ITL = normalTL * minD;
                    ITR = normalTR * minD;
                    IBL = normalBL * minD;
                    IBR = normalBR * minD;

                    // Check visibility of inner corners of the tile:
                    // a tile should be considered visible if any of its corners is visible
                    if (minHeight != 0.0)
                    {
                        if (isPointVisible(ITL,
                            topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, false)
                            || isPointVisible(ITR,
                            topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, false)
                            || isPointVisible(IBL,
                            topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, false)
                            || isPointVisible(IBR,
                            topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, false))
                        {
                            isVisible = true;
                            break;
                        }
                    }

                    if (maxD != minD)
                    {
                        // Check visibility of outer corners of the tile:
                        // a tile should be considered visible if any of its corners is visible
                        if (isPointVisible(OTL,
                            topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, false)
                            || isPointVisible(OTR,
                                topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, false)
                            || isPointVisible(OBL,
                                topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, false)
                            || isPointVisible(OBR,
                                topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, false)
                            )
                        {
                            isVisible = true;
                            break;
                        }
                    }

                    // Tile should be considered visible if any corner ray of frustum intersects surface of the tile
                    if (checkVert && rayIntersectsTileSurface(camPos, camDown, angTL.x, angBR.x, angTL.y, angBR.y, 1.0)
                        || rayIntersectsTileSurface(camPos, camTL, angTL.x, angBR.x, angTL.y, angBR.y, 1.0)
                        || rayIntersectsTileSurface(camPos, camTR, angTL.x, angBR.x, angTL.y, angBR.y, 1.0)
                        || rayIntersectsTileSurface(camPos, camBL, angTL.x, angBR.x, angTL.y, angBR.y, 1.0)
                        || rayIntersectsTileSurface(camPos, camBR, angTL.x, angBR.x, angTL.y, angBR.y, 1.0))
                    {
                        if (minHeight == 0.0)
                        {
                            isVisible = true;
                            break;
                        }
                        else
                            isFlatVisible = true;
                    }

                    if (minHeight != 0.0
                        && (rayIntersectsTileSurface(camPos, camTL, angTL.x, angBR.x, angTL.y, angBR.y, minD)
                        || rayIntersectsTileSurface(camPos, camTR, angTL.x, angBR.x, angTL.y, angBR.y, minD)
                        || rayIntersectsTileSurface(camPos, camBL, angTL.x, angBR.x, angTL.y, angBR.y, minD)
                        || rayIntersectsTileSurface(camPos, camBR, angTL.x, angBR.x, angTL.y, angBR.y, minD)))
                    {
                        isVisible = true;
                        break;
                    }

                    if (maxD != minD)
                    {
                        // Tile should be considered visible
                        // if any corner ray of frustum intersects top surface of the tile
                        if (rayIntersectsTileSurface(camPos, camTL, angTL.x, angBR.x, angTL.y, angBR.y, maxD)
                            || rayIntersectsTileSurface(camPos, camTR, angTL.x, angBR.x, angTL.y, angBR.y, maxD)
                            || rayIntersectsTileSurface(camPos, camBL, angTL.x, angBR.x, angTL.y, angBR.y, maxD)
                            || rayIntersectsTileSurface(camPos, camBR, angTL.x, angBR.x, angTL.y, angBR.y, maxD))
                        {
                            isVisible = true;
                            break;
                        }

                        // Tile should be considered visible
                        // if any corner ray of frustum intersects left side face of the tile
                        auto n = glm::normalize(glm::cross(normalBL, normalTL));
                        if (rayIntersectsTileSide(camPos, camTL, normalTL, n, angTL.y, angBR.y, minD, maxD)
                            || rayIntersectsTileSide(camPos, camTR, normalTL, n, angTL.y, angBR.y, minD, maxD)
                            || rayIntersectsTileSide(camPos, camBL, normalTL, n, angTL.y, angBR.y, minD, maxD)
                            || rayIntersectsTileSide(camPos, camBR, normalTL, n, angTL.y, angBR.y, minD, maxD))
                        {
                            isVisible = true;
                            break;
                        }

                        // Tile should be considered visible
                        // if any corner ray of frustum intersects right side face of the tile
                        n = glm::normalize(glm::cross(normalTR, normalBR));
                        if (rayIntersectsTileSide(camPos, camTL, normalBR, n, angTL.y, angBR.y, minD, maxD)
                            || rayIntersectsTileSide(camPos, camTR, normalBR, n, angTL.y, angBR.y, minD, maxD)
                            || rayIntersectsTileSide(camPos, camBL, normalBR, n, angTL.y, angBR.y, minD, maxD)
                            || rayIntersectsTileSide(camPos, camBR, normalBR, n, angTL.y, angBR.y, minD, maxD))
                        {
                            isVisible = true;
                            break;
                        }

                        // Tile should be considered visible
                        // if any corner ray of frustum intersects north side of the tile
                        auto nSqrTanLat = 1.0 / qTan(angTL.y);
                        nSqrTanLat *= nSqrTanLat;
                        nSqrTanLat *= angTL.y > 0.0 ? -1.0 : 1.0;
                        if (rayIntersectsTileCut(camPos, camTL, nSqrTanLat, angTL.x, angBR.x, minD, maxD)
                            || rayIntersectsTileCut(camPos, camTR, nSqrTanLat, angTL.x, angBR.x, minD, maxD)
                            || rayIntersectsTileCut(camPos, camBL, nSqrTanLat, angTL.x, angBR.x, minD, maxD)
                            || rayIntersectsTileCut(camPos, camBR, nSqrTanLat, angTL.x, angBR.x, minD, maxD))
                        {
                            isVisible = true;
                            break;
                        }

                        // Tile should be considered visible
                        // if any corner ray of frustum intersects south side of the tile
                        nSqrTanLat = 1.0 / qTan(angBR.y);
                        nSqrTanLat *= nSqrTanLat;
                        nSqrTanLat *= angBR.y > 0.0 ? -1.0 : 1.0;
                        if (rayIntersectsTileCut(camPos, camTL, nSqrTanLat, angTL.x, angBR.x, minD, maxD)
                            || rayIntersectsTileCut(camPos, camTR, nSqrTanLat, angTL.x, angBR.x, minD, maxD)
                            || rayIntersectsTileCut(camPos, camBL, nSqrTanLat, angTL.x, angBR.x, minD, maxD)
                            || rayIntersectsTileCut(camPos, camBR, nSqrTanLat, angTL.x, angBR.x, minD, maxD))
                        {
                            isVisible = true;
                            break;
                        }

                    }

                    // Check visibility of side edges of the tile:
                    // a tile should be considered visible if any of its side edge intersects any side of frustum
                    if (isEdgeVisible(camPos, topN, leftN, checkVert ? botLeftN : bottomN, rightN, topD, leftD,
                        checkVert ? botLeftD : bottomD, rightD, normalTL, normalBL,
                        checkVert ? &botRightN : nullptr, checkVert ? &botRightD : nullptr))
                    {
                        if (minHeight == 0.0)
                        {
                            visML = true;
                            isVisible = true;
                            break;
                        }
                        else
                            isFlatVisible = true;
                    }
                    if (isEdgeVisible(camPos, topN, leftN, checkVert ? botLeftN : bottomN, rightN, topD, leftD,
                        checkVert ? botLeftD : bottomD, rightD, normalBR, normalTR,
                        checkVert ? &botRightN : nullptr, checkVert ? &botRightD : nullptr))
                    {
                        if (minHeight == 0.0)
                        {
                            visMR = true;
                            isVisible = true;
                            break;
                        }
                        else
                            isFlatVisible = true;
                    }

                    if (minHeight != 0.0)
                    {
                        if (isEdgeVisible(
                            camPos, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, ITL, IBL))
                        {
                            isVisible = true;
                            break;
                        }
                        if (isEdgeVisible(
                            camPos, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, IBR, ITR))
                        {
                            isVisible = true;
                            break;
                        }
                    }

                    if (maxD != minD)
                    {
                        // Check visibility of side edges of the top surface of tile:
                        // a tile should be considered visible if any of its side edge intersects any side of frustum
                        if (isEdgeVisible(camPos, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, OTL, OBL)
                        || isEdgeVisible(camPos, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, OBR, OTR))
                        {
                            isVisible = true;
                            break;
                        }

                    }

                    // Check visibility of north edge of the tile:
                    // a tile should be considered visible if its north edge intersects any side of frustum
                    const auto cosTL = qCos(angTL.y);
                    auto sqrR = cosTL;
                    sqrR *= sqrR;
                    if (isArcVisible(camPos, topN, leftN, checkVert ? botLeftN : bottomN, rightN, topD, leftD,
                        checkVert ? botLeftD : bottomD, rightD, angTL.x, angBR.x, normalTL.z, sqrR,
                        checkVert ? &botRightN : nullptr, checkVert ? &botRightD : nullptr))
                    {
                        if (minHeight == 0.0)
                        {
                            visTM = true;
                            isVisible = true;
                            break;
                        }
                        else
                            isFlatVisible = true;
                    }

                    // Check visibility of south edge of the tile:
                    // a tile should be considered visible if its south edge intersects any side of frustum
                    const auto cosBR = qCos(angBR.y);
                    sqrR = cosBR;
                    sqrR *= sqrR;
                    if (isArcVisible(camPos, topN, leftN, checkVert ? botLeftN : bottomN, rightN, topD, leftD,
                        checkVert ? botLeftD : bottomD, rightD, angTL.x, angBR.x, normalBR.z, sqrR,
                        checkVert ? &botRightN : nullptr, checkVert ? &botRightD : nullptr))
                    {
                        if (minHeight == 0.0)
                        {
                            visBM = true;
                            isVisible = true;
                            break;
                        }
                        else
                            isFlatVisible = true;
                    }

                    if (minHeight != 0.0)
                    {
                        sqrR = minD * cosTL;
                        sqrR *= sqrR;
                        if (isArcVisible(camPos, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD,
                            angTL.x, angBR.x, ITL.z, sqrR))
                        {
                            isVisible = true;
                            break;
                        }

                        // Check visibility of south edge of the tile:
                        // a tile should be considered visible if its south edge intersects any side of frustum
                        sqrR = minD * cosBR;
                        sqrR *= sqrR;
                        if (isArcVisible(camPos, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD,
                            angTL.x, angBR.x, IBR.z, sqrR))
                        {
                            isVisible = true;
                            break;
                        }
                    }

                    if (maxD != minD)
                    {
                        // Check visibility of north edge of the top surface of tile:
                        // a tile should be considered visible
                        // if north edge of its top surface intersects any side of frustum
                        sqrR = maxD * cosTL;
                        sqrR *= sqrR;
                        if (isArcVisible(camPos, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD,
                            angTL.x, angBR.x, OTL.z, sqrR))
                        {
                            isVisible = true;
                            break;
                        }

                        // Check visibility of south edge of the top surface of tile:
                        // a tile should be considered visible
                        // if south edge of its top surface intersects any side of frustum
                        sqrR = maxD * cosBR;
                        sqrR *= sqrR;
                        if (isArcVisible(camPos, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD,
                            angTL.x, angBR.x, OBR.z, sqrR))
                        {
                            isVisible = true;
                            break;
                        }
                    }
                } while (false);
                if (isVisible || isFlatVisible)
                {
                    // Check distance of middle point of the super-tile:
                    // it should be considered visible instead if its central point is far enough
                    const bool oddX = tileIdN.x & 1 > 0;
                    const bool oddY = tileIdN.y & 1 > 0;
                    const auto centralPoint = oddY ? (oddX ? normalTL : normalTR) : (oddX ? normalBL : normalBR);
                    const auto toCenter = centralPoint - camPos;
                    const auto distance =
                        glm::dot(toCenter, camDir) * (1.0 - tiltFactor) + tiltFactor * glm::length(toCenter);
                    if (currentZoom > MinZoomLevel && zLower != zFar
                        && distance > zLower + detailDistanceFactor(zShift) * detailThickness)
                    {
                        tilesN[currentZoom].insert(tileIdN);
                        (*nextTiles)[currentZoom - 1].insert(TileId::fromXY(tileId.x / 2 - (tileId.x < 0 ? 1 : 0),
                            tileId.y / 2 - (tileId.y < 0 ? 1 : 0)), AlmostVisible);
                        shouldRepeat = true;
                        continue;
                    }
                    TileVisibility visibility = isVisible ? Visible : VisibleFlat;
                    if (isVisible && (minHeight != 0.0 || maxHeight != 0.0))
                    {
                        internalState->maxElevation = qMax(internalState->maxElevation, maxHeightInWorld);
                        if (currentZoomLevel == zoomLevel)
                        {
                            const auto center = (OTL + OTR + OBL + OBR) / 4.0;
                            const auto toCenter = glm::distance(camPos, center);
                            if (toCenter < extraDetailDistance)
                                visibility = ExtraDetail;
                            const auto gap = 0.1 / globeRadius;
                            const auto difference = maxD - cameraDistance;
                            if (difference + gap > 0.0)
                            {
                                const auto under = qMax(0.0, difference);
                                const auto maxR = qMax(glm::distance(center, OTL), glm::distance(center, OBR));
                                const auto sqrMax = toCenter * toCenter - under * under;
                                const auto delta = sqrMax > 0.0 ? qMax(qSqrt(sqrMax) - maxR, 0.0) : 0.0;
                                const auto extra = (difference + gap - delta * 4.0) * globeRadius;
                                if (extra > 0.0 && extra > internalState->extraElevation)
                                    internalState->extraElevation = static_cast<float>(extra);
                            }
                        }
                    }

                    atLeastOneAdded = true;
                    atLeastOneFlatVisibleFound = true;

                    if (!lookForStrictlyVisible || (isVisible && (minHeight != 0.0 || maxHeight != 0.0)))
                    {
                        tiles[currentZoom].insert(tileId, visibility);
                        tilesN[currentZoom].insert(tileIdN);

                        if (lookForStrictlyVisible)
                            lookForStrictlyVisible = false;
                    }
                    else
                        continue;

                    if (!isVisible)
                        continue;

                    maxDistance = qMax(maxDistance, maxD);

                    const auto zDetail = zShift > 0 && currentZoom < MaxZoomLevel && zLower != zFar
                        ? zLower + detailDistanceFactor(zShift - 1) * detailThickness : 0.0;
                    const auto leftX = tileId.x - 1;
                    const auto rightX = tileId.x + 1;
                    const auto topY = tileId.y - 1;
                    const auto bottomY = tileId.y + 1;
                    insertTileId((*nextTiles)[currentZoom],
                        TileId::fromXY(leftX, tileId.y), zDetail, tiltFactor, zoomShift, camPos, camDir, visML);
                    insertTileId((*nextTiles)[currentZoom],
                        TileId::fromXY(rightX, tileId.y), zDetail, tiltFactor, zoomShift, camPos, camDir, visMR);
                    if (topY >= 0)
                    {
                        insertTileId((*nextTiles)[currentZoom],
                            TileId::fromXY(leftX, topY), zDetail, tiltFactor, zoomShift, camPos, camDir, visTL);
                        insertTileId((*nextTiles)[currentZoom],
                            TileId::fromXY(tileId.x, topY), zDetail, tiltFactor, zoomShift, camPos, camDir, visTM);
                        insertTileId((*nextTiles)[currentZoom],
                            TileId::fromXY(rightX, topY), zDetail, tiltFactor, zoomShift, camPos, camDir, visTR);
                    }
                    if (bottomY < static_cast<int32_t>(1u << currentZoom))
                    {
                        insertTileId((*nextTiles)[currentZoom],
                            TileId::fromXY(leftX, bottomY), zDetail, tiltFactor, zoomShift, camPos, camDir, visBL);
                        insertTileId((*nextTiles)[currentZoom],
                            TileId::fromXY(tileId.x, bottomY), zDetail, tiltFactor, zoomShift, camPos, camDir, visBM);
                        insertTileId((*nextTiles)[currentZoom],
                            TileId::fromXY(rightX, bottomY), zDetail, tiltFactor, zoomShift, camPos, camDir, visBR);
                    }
                    shouldRepeat = true;
                    atLeastOneVisibleFound = true;
                }
                else
                    tilesN[currentZoom].insert(tileIdN);
            }
        }
        if (!atLeastOneVisibleFound && !shouldRepeat)
        {
            const auto dir = lookForStrictlyVisible ? -1.0 : 1.0;
            const auto deltaX = dirX * dir;
            const auto deltaY = dirY * dir;
            if ((!atLeastOneAdded && atLeastOneFlatVisibleFound)
                || (lookForStrictlyVisible && distanceFromTarget >= distanceLimit))
            {
                if (lookForStrictlyVisible)
                {
                    lookForStrictlyVisible = false;
                    atLeastOneFlatVisibleFound = false;
                    maxDistance = 0.0;
                    distanceFromTarget = 0.0;
                    targetTileId = internalState->synthTileId;
                    (*nextTiles)[zoomLevel].insert(targetTileId, NotTestedYet);
                    shouldRepeat = true;
                }
                else
                {
                    tiles.last().clear();    
                    tiles.last().insert(targetTileId, Visible);
                    double distance = 1.0;
                    auto prevTileId = targetTileId;
                    TileId nextTileId;
                    for (int i = 0; i < 10; i++)
                    {
                        nextTileId = TileId::fromXY(
                            static_cast<int>(qRound(static_cast<double>(targetTileId.x) - deltaX * distance)),
                            static_cast<int>(qRound(static_cast<double>(targetTileId.y) + deltaY * distance)));
                        if (nextTileId == prevTileId)
                        {
                            distance += 1.0;
                            nextTileId = TileId::fromXY(
                                static_cast<int>(qRound(static_cast<double>(targetTileId.x) - deltaX * distance)),
                                static_cast<int>(qRound(static_cast<double>(targetTileId.y) + deltaY * distance)));
                        }
                        tiles.last().insert(nextTileId, Visible);
                        distance += 1.0;
                        prevTileId = nextTileId;
                    }
                }
            }
            else
            {
                const auto firstTileId = (*procTiles)[zoomLevel].begin().key();
                distanceFromTarget += 1.0;
                auto nearTileId = TileId::fromXY(
                    static_cast<int>(qRound(static_cast<double>(targetTileId.x) - deltaX * distanceFromTarget)),
                    static_cast<int>(qRound(static_cast<double>(targetTileId.y) + deltaY * distanceFromTarget)));
                if (nearTileId == firstTileId && distanceFromTarget < distanceLimit)
                {
                    distanceFromTarget += 1.0;
                    nearTileId = TileId::fromXY(
                        static_cast<int>(qRound(static_cast<double>(targetTileId.x) - deltaX * distanceFromTarget)),
                        static_cast<int>(qRound(static_cast<double>(targetTileId.y) + deltaY * distanceFromTarget)));
                }
                if (nearTileId.y >= 0 && nearTileId.y < static_cast<int32_t>(1u << zoomLevel))
                {
                    (*nextTiles)[zoomLevel].insert(nearTileId, NotTestedYet);
                    shouldRepeat = true;
                }
            }
        }
        procTiles->clear();
        std::swap(procTiles, nextTiles);
    }

    // Normalize and make unique visible tiles
    internalState->visibleTilesCount = 0;
    internalState->visibleTiles.clear();
    internalState->visibleTilesSet.clear();
    internalState->uniqueTiles.clear();
    internalState->extraDetailedTiles.clear();
    for (const auto& setEntry : rangeOf(constOf(tiles)))
    {
        const auto realZoom = setEntry.key() - zoomDelta;
        if (realZoom < 0)
            continue;
        zoomLevel = static_cast<ZoomLevel>(realZoom);
        QSet<TileId> uniqueTiles;
        for (const auto& tileEntry : rangeOf(constOf(setEntry.value())))
        {
            const auto visibility = tileEntry.value();
            auto tileId = tileEntry.key();
            if (state.flatEarth)
            {
                const auto zoomShift = state.zoomLevel - zoomLevel;
                tileId.x = ((tileId.x << zoomShift) - tileDelta.x) >> zoomShift;
                tileId.y = ((tileId.y << zoomShift) - tileDelta.y) >> zoomShift;
            }
            const auto tileIdN = Utilities::normalizeTileId(tileId, zoomLevel);
            uniqueTiles.insert(tileIdN);
            if (visibility != VisibleFlat)
            {
                internalState->visibleTiles[zoomLevel].push_back(tileId);
                internalState->visibleTilesSet[zoomLevel].insert(tileIdN);
            }
            if (visibility == ExtraDetail)
                internalState->extraDetailedTiles.insert(tileIdN);
        }
        internalState->visibleTilesCount += uniqueTiles.size();
        internalState->uniqueTiles[zoomLevel] = QVector<TileId>(uniqueTiles.begin(), uniqueTiles.end());
        const auto zoomShift = MaxZoomLevel - zoomLevel;
        const auto targetTileId = TileId::fromXY(state.target31.x >> zoomShift, state.target31.y >> zoomShift);
        internalState->uniqueTilesTargets[zoomLevel] = targetTileId;
        // Sort visible tiles by distance from target
        std::sort(internalState->uniqueTiles[zoomLevel],
            [targetTileId]
            (const TileId& l, const TileId& r) -> bool
            {
                const auto lx = l.x - targetTileId.x;
                const auto ly = l.y - targetTileId.y;

                const auto rx = r.x - targetTileId.x;
                const auto ry = r.y - targetTileId.y;

                return (lx * lx + ly * ly) < (rx * rx + ry * ry);
            });
    }

    if (internalState->visibleTilesCount > 0)
    {
        // Use underscaled resources carefully in accordance to total number of visible tiles
        const auto zoomDelta = qFloor(log2(qMax(1.0,
            static_cast<double>(1u << state.surfaceZoomLevel) * static_cast<double>(state.surfaceVisualZoom)
            / (static_cast<double>(1u << state.zoomLevel) * static_cast<double>(state.visualZoom)))));
        int zoomLevelOffset = qMin(zoomDelta, static_cast<int>(MaxMissingDataUnderZoomShift));
        if (zoomLevelOffset > 2 && internalState->visibleTilesCount > 1)
            zoomLevelOffset = 2;
        if (zoomLevelOffset > 1 && internalState->visibleTilesCount > MaxNumberOfTilesToUseUnderscaledTwice)
            zoomLevelOffset = 1;
        if (zoomLevelOffset > 0 && internalState->visibleTilesCount > MaxNumberOfTilesToUseUnderscaledOnce)
            zoomLevelOffset = 0;
        internalState->zoomLevelOffset = zoomLevelOffset;
        if (!internalState->extraDetailedTiles.empty() && (zoomLevelOffset > 0 || internalState->visibleTilesCount
            + internalState->extraDetailedTiles.size() * 3 > MaxNumberOfTilesAllowed))
        {
            internalState->extraDetailedTiles.clear();
        }
    }
    // Compute visible area on elevated plane or bloated sphere if possible
    if (state.flatEarth)
    {
        if (internalState->worldCameraPosition.y > internalState->maxElevation)
        {
            // Get (up to) 4 points of frustum edges & plane intersection
            const glm::vec3 planeN(0.0f, 1.0f, 0.0f);
            const glm::vec3 planeE(0.0f, internalState->maxElevation, 0.0f);
            auto intersectionPointsCounter = 0u;
            glm::vec3 intersectionPoint;
            glm::vec2 bottomIntersectionPoints[2];
            glm::vec2 topIntersectionPoints[2];

            if (intersectionPointsCounter < 2 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(
                planeN, planeE, nBL_g.xyz(), mBL_g.xyz(), intersectionPoint))
                bottomIntersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
            if (intersectionPointsCounter < 2 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(
                planeN, planeE, nBR_g.xyz(), mBR_g.xyz(), intersectionPoint))
                bottomIntersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
            if (intersectionPointsCounter < 2 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(
                planeN, planeE, nTL_g.xyz(), nBL_g.xyz(), intersectionPoint))
                bottomIntersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
            if (intersectionPointsCounter < 2 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(
                planeN, planeE, nTR_g.xyz(), nBR_g.xyz(), intersectionPoint))
                bottomIntersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
            intersectionPointsCounter = 0;
            if (intersectionPointsCounter < 2 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(
                planeN, planeE, nTR_g.xyz(), mTR_g.xyz(), intersectionPoint))
                topIntersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
            if (intersectionPointsCounter < 2 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(
                planeN, planeE, nTL_g.xyz(), mTL_g.xyz(), intersectionPoint))
                topIntersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
            if (intersectionPointsCounter < 2 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(
                planeN, planeE, mTR_g.xyz(), mBR_g.xyz(), intersectionPoint))
                topIntersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
            if (intersectionPointsCounter < 2 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(
                planeN, planeE, mTL_g.xyz(), mBL_g.xyz(), intersectionPoint))
                topIntersectionPoints[intersectionPointsCounter++] = intersectionPoint.xz();
            const auto tileSize31 = static_cast<double>(1u << (ZoomLevel::MaxZoomLevel - state.zoomLevel));
            internalState->elevatedFrustum2D31.p0 = PointI64((PointD(bottomIntersectionPoints[0].x,
                bottomIntersectionPoints[0].y) / TileSize3D) * tileSize31) + state.target31;
            internalState->elevatedFrustum2D31.p1 = PointI64((PointD(bottomIntersectionPoints[1].x,
                bottomIntersectionPoints[1].y) / TileSize3D) * tileSize31) + state.target31;
            internalState->elevatedFrustum2D31.p2 = PointI64((PointD(topIntersectionPoints[0].x,
                topIntersectionPoints[0].y) / TileSize3D) * tileSize31) + state.target31;
            internalState->elevatedFrustum2D31.p3 = PointI64((PointD(topIntersectionPoints[1].x,
                topIntersectionPoints[1].y) / TileSize3D) * tileSize31) + state.target31;
            internalState->targetOffset = internalState->elevatedFrustum2D31.clampCoordinates();
        }
    }
    else if (!withNorthPole && !withSouthPole)
    {
        const auto cornerCosine = glm::dot(camDir, camTL);
        const auto cameraNearDistance = static_cast<double>(_zNear) / globeRadius / cornerCosine;
        const auto cameraFarDistance = zFar / cornerCosine;
        const auto nTL = camPos + camTL * cameraNearDistance;
        const auto nTR = camPos + camTR * cameraNearDistance;
        const auto nBL = camPos + camBL * cameraNearDistance;
        const auto nBR = camPos + camBR * cameraNearDistance;
        const auto fTL = camPos + camTL * cameraFarDistance;
        const auto fTR = camPos + camTR * cameraFarDistance;
        const auto fBL = camPos + camBL * cameraFarDistance;
        const auto fBR = camPos + camBR * cameraFarDistance;
        auto bottomPointsCounter = 0;
        auto topPointsCounter = 0;
        PointI64 bottomIntersectionPoint[2];
        PointI64 topIntersectionPoint[2];
        PointD iAngles;
        if (lineSegmentIntersectsSphere(nBL, fBL, maxDistance, iAngles.x, iAngles.y))
            bottomIntersectionPoint[bottomPointsCounter++] = Utilities::get31FromAngles(iAngles);
        if (bottomPointsCounter < 1
            && lineSegmentIntersectsSphere(nTL, nBL, maxDistance, iAngles.x, iAngles.y))
            bottomIntersectionPoint[bottomPointsCounter++] = Utilities::get31FromAngles(iAngles);
        if (lineSegmentIntersectsSphere(nBR, fBR, maxDistance, iAngles.x, iAngles.y))
            bottomIntersectionPoint[bottomPointsCounter++] = Utilities::get31FromAngles(iAngles);
        if (bottomPointsCounter < 2
            && lineSegmentIntersectsSphere(nTR, nBR, maxDistance, iAngles.x, iAngles.y))
            bottomIntersectionPoint[bottomPointsCounter++] = Utilities::get31FromAngles(iAngles);
        if (lineSegmentIntersectsSphere(nTR, fTR, maxDistance, iAngles.x, iAngles.y))
            topIntersectionPoint[topPointsCounter++] = Utilities::get31FromAngles(iAngles);
        if (topPointsCounter < 1
            && lineSegmentIntersectsSphere(fTR, fBR, maxDistance, iAngles.x, iAngles.y))
            topIntersectionPoint[topPointsCounter++] = Utilities::get31FromAngles(iAngles);
        if (lineSegmentIntersectsSphere(nTL, fTL, maxDistance, iAngles.x, iAngles.y))
            topIntersectionPoint[topPointsCounter++] = Utilities::get31FromAngles(iAngles);
        if (topPointsCounter < 2
            && lineSegmentIntersectsSphere(fTL, fBL, maxDistance, iAngles.x, iAngles.y))
            topIntersectionPoint[topPointsCounter++] = Utilities::get31FromAngles(iAngles);
        if (topPointsCounter == 2 && bottomPointsCounter == 2)
        {
            const auto intHalf = INT32_MAX / 2 + 1;
            if (bottomIntersectionPoint[0].x > bottomIntersectionPoint[1].x
                && bottomIntersectionPoint[0].x - bottomIntersectionPoint[1].x > intHalf)
                bottomIntersectionPoint[0].x = bottomIntersectionPoint[0].x - INT32_MAX - 1;
            else if (bottomIntersectionPoint[1].x > bottomIntersectionPoint[0].x
                && bottomIntersectionPoint[1].x - bottomIntersectionPoint[0].x > intHalf)
                bottomIntersectionPoint[1].x = bottomIntersectionPoint[1].x - INT32_MAX - 1;
            if (topIntersectionPoint[0].x > topIntersectionPoint[1].x
                && topIntersectionPoint[0].x - topIntersectionPoint[1].x > intHalf)
                topIntersectionPoint[0].x = topIntersectionPoint[0].x - INT32_MAX - 1;
            else if (topIntersectionPoint[1].x > topIntersectionPoint[0].x
                && topIntersectionPoint[1].x - topIntersectionPoint[0].x > intHalf)
                topIntersectionPoint[1].x = topIntersectionPoint[1].x - INT32_MAX - 1;
            if (qMax(bottomIntersectionPoint[0].x, bottomIntersectionPoint[1].x)
                - qMin(topIntersectionPoint[0].x, topIntersectionPoint[1].x) > intHalf)
            {
                if (bottomIntersectionPoint[1].x <= bottomIntersectionPoint[0].x)
                    bottomIntersectionPoint[0].x = bottomIntersectionPoint[0].x - INT32_MAX - 1;
                if (bottomIntersectionPoint[0].x <= bottomIntersectionPoint[1].x)
                    bottomIntersectionPoint[1].x = bottomIntersectionPoint[1].x - INT32_MAX - 1;
            }
            else if (qMax(topIntersectionPoint[0].x, topIntersectionPoint[1].x)
                - qMin(bottomIntersectionPoint[0].x, bottomIntersectionPoint[1].x) > intHalf)
            {
                if (topIntersectionPoint[1].x <= topIntersectionPoint[0].x)
                    topIntersectionPoint[0].x = topIntersectionPoint[0].x - INT32_MAX - 1;
                if (topIntersectionPoint[0].x <= topIntersectionPoint[1].x)
                    topIntersectionPoint[1].x = topIntersectionPoint[1].x - INT32_MAX - 1;
            }
            internalState->elevatedFrustum2D31.p0 = bottomIntersectionPoint[0];
            internalState->elevatedFrustum2D31.p1 = bottomIntersectionPoint[1];
            internalState->elevatedFrustum2D31.p2 = topIntersectionPoint[0];
            internalState->elevatedFrustum2D31.p3 = topIntersectionPoint[1];
            const auto centerX = static_cast<int>((bottomIntersectionPoint[0].x + bottomIntersectionPoint[1].x
                + topIntersectionPoint[0].x + topIntersectionPoint[1].x) / 4);
            const auto targetX = state.target31.x - INT32_MAX - 1;
            internalState->targetOffset =
                PointI64(qAbs(centerX - state.target31.x) > qAbs(centerX - targetX) ? 1u + INT32_MAX : 0, 0);
        }
    }
}

inline double OsmAnd::AtlasMapRenderer_OpenGL::detailDistanceFactor(const int zoomShift) const
{
    return zoomShift > 0 ? qPow(2.0, zoomShift) - 1.0 : 0.0;
}

inline void OsmAnd::AtlasMapRenderer_OpenGL::insertTileId(QHash<TileId, TileVisibility>& nextTiles,
    const TileId& tileId, const double zDetail, const double tiltFactor, const int32_t zoomShift,
    const glm::dvec3& camPos, const glm::dvec3& camDir, const bool almostVisible) const
{
    if (zDetail > 0.0)
    {
        const auto tileIdTL = TileId::fromXY(tileId.x * 2, tileId.y * 2);
        const auto tileIdBR = TileId::fromXY(tileIdTL.x + 1, tileIdTL.y + 1);
        const auto center31 = PointI(tileIdBR.x << (zoomShift - 1), tileIdBR.y << (zoomShift - 1));
        const auto angCenter = Utilities::getAnglesFrom31(center31);
        const auto centralPoint = Utilities::getGlobeRadialVector(angCenter);
        const auto toCenter = centralPoint - camPos;
        const auto distance = glm::dot(toCenter, camDir) * (1.0 - tiltFactor) + tiltFactor * glm::length(toCenter);
        if (distance <= zDetail)
            return;
    }
    if (almostVisible)
        nextTiles.insert(tileId, AlmostVisible);
    else if (!nextTiles.contains(tileId))
        nextTiles.insert(tileId, NotTestedYet);
}

inline bool OsmAnd::AtlasMapRenderer_OpenGL::isPointVisible(const glm::dvec3& point,
    const glm::dvec3& topN, const glm::dvec3& leftN, const glm::dvec3& bottomN, const glm::dvec3& rightN,
    const double topD, const double leftD, const double bottomD, const double rightD,
    bool skipTop, bool skipLeft, bool skipBottom, bool skipRight,
    const glm::dvec3* botRightN /* = nullptr */, const double* botRightD /* = nullptr */) const
{
    const auto top = skipTop || glm::dot(point, topN) <= topD;
    const auto left = skipLeft || glm::dot(point, leftN) <= leftD;
    const auto bottom = skipBottom || glm::dot(point, bottomN) <= bottomD;
    const auto right = skipRight || glm::dot(point, rightN) <= rightD;
    const auto botRight = botRightN == nullptr || botRightD == nullptr || glm::dot(point, *botRightN) <= *botRightD;
    return top && left && bottom && right && botRight;
}

inline bool OsmAnd::AtlasMapRenderer_OpenGL::rayIntersectsTileSurface(
    const glm::dvec3& rayStart, const glm::dvec3& rayVector,
    const double left, const double right, const double top, const double bottom, const double radius) const
{
    double angleX, angleY;
    bool result = Utilities_OpenGL_Common::rayIntersectsSphere(rayStart, rayVector, radius, angleX, angleY)
        && angleX >= left && angleX <= right && angleY <= top && angleY >= bottom;
    return result;
}

inline bool OsmAnd::AtlasMapRenderer_OpenGL::rayIntersectsTileSide(
    const glm::dvec3& rayStart, const glm::dvec3& rayVector, const glm::dvec3& planeO, const glm::dvec3& planeN,
    const double top, const double bottom, const double minRadius, const double maxRadius) const
{
    double d;
    if (Utilities_OpenGL_Common::rayIntersectPlane(planeN, planeO, rayVector, rayStart, d) && d > 0.0)
    {
        const auto point = rayStart + rayVector * d;
        const auto angleY = -qAsin(qBound(-1.0, glm::normalize(point).z, 1.0));
        if (angleY <= top && angleY >= bottom)
        {
            const auto dist = glm::length(point);
            if (dist >= minRadius && dist <= maxRadius)
                return true;
        }
    }
    return false;
}

inline bool OsmAnd::AtlasMapRenderer_OpenGL::rayIntersectsTileCut(
    const glm::dvec3& rayStart, const glm::dvec3& rayVector, const double nSqrTanLat,
    const double left, const double right, const double minRadius, const double maxRadius) const
{
    double angleX, dist;
    if (Utilities_OpenGL_Common::rayIntersectsCone(rayStart, rayVector, nSqrTanLat, angleX, dist))
    {
        if (angleX >= left && angleX <= right)
        {
            if (dist >= minRadius && dist <= maxRadius)
                return true;
        }
    }
    return false;
}

inline bool OsmAnd::AtlasMapRenderer_OpenGL::isEdgeVisible(const glm::dvec3& cp,
    const glm::dvec3& topN, const glm::dvec3& leftN, const glm::dvec3& bottomN, const glm::dvec3& rightN,
    const double topD, const double leftD, const double bottomD, const double rightD,
    const glm::dvec3& start, const glm::dvec3& end,
    const glm::dvec3* botRightN /* = nullptr */, const double* botRightD /* = nullptr */) const
{
    glm::dvec3 ip;
    if ((Utilities_OpenGL_Common::lineSegmentIntersectsPlane(topN, cp, start, end, ip)
        && isPointVisible(ip, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD,
            true, false, false, false, botRightN, botRightD))
        || (Utilities_OpenGL_Common::lineSegmentIntersectsPlane(leftN, cp, start, end, ip)
        && isPointVisible(ip, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD,
            false, true, false, false, botRightN, botRightD))
        || (Utilities_OpenGL_Common::lineSegmentIntersectsPlane(bottomN, cp, start, end, ip)
        && isPointVisible(ip, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD,
            false, false, true, false, botRightN, botRightD))
        || (Utilities_OpenGL_Common::lineSegmentIntersectsPlane(rightN, cp, start, end, ip)
        && isPointVisible(ip, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD,
            false, false, false, true, botRightN, botRightD))
        || (botRightN != nullptr && botRightD != nullptr
        && Utilities_OpenGL_Common::lineSegmentIntersectsPlane(*botRightN, cp, start, end, ip)
        && isPointVisible(ip, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD,
            false, false, false, false, nullptr, nullptr)))
        return true;
    return false;
}

inline bool OsmAnd::AtlasMapRenderer_OpenGL::isArcVisible(const glm::dvec3& cp,
    const glm::dvec3& topN, const glm::dvec3& leftN, const glm::dvec3& bottomN, const glm::dvec3& rightN,
    const double topD, const double leftD, const double bottomD, const double rightD,
    const double minAngleX, const double maxAngleX, const double arcZ, const double sqrRadius,
    const glm::dvec3* botRightN /* = nullptr */, const double* botRightD /* = nullptr */) const
{
    glm::dvec3 ip1, ip2;
    if ((Utilities_OpenGL_Common::arcIntersectsPlane(topN, topD, minAngleX, maxAngleX, arcZ, sqrRadius, ip1, ip2)
    && (isPointVisible(ip1, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, true, false, false, false,
        botRightN, botRightD) || (ip1 != ip2
    && isPointVisible(ip2, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, true, false, false, false,
        botRightN, botRightD))))
    || (Utilities_OpenGL_Common::arcIntersectsPlane(leftN, leftD, minAngleX, maxAngleX, arcZ, sqrRadius, ip1, ip2)
    && (isPointVisible(ip1, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, true, false, false,
        botRightN, botRightD) || (ip1 != ip2
    && isPointVisible(ip2, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, true, false, false,
        botRightN, botRightD))))
    || (Utilities_OpenGL_Common::arcIntersectsPlane(bottomN, bottomD, minAngleX, maxAngleX, arcZ, sqrRadius, ip1, ip2)
    && (isPointVisible(ip1, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, true, false,
        botRightN, botRightD) || (ip1 != ip2
    && isPointVisible(ip2, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, true, false,
        botRightN, botRightD))))
    || (Utilities_OpenGL_Common::arcIntersectsPlane(rightN, rightD, minAngleX, maxAngleX, arcZ, sqrRadius, ip1, ip2)
    && (isPointVisible(ip1, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, true,
        botRightN, botRightD) || (ip1 != ip2
    && isPointVisible(ip2, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, true,
        botRightN, botRightD))))
    || (botRightN != nullptr && botRightD != nullptr && Utilities_OpenGL_Common::arcIntersectsPlane(
        *botRightN, *botRightD, minAngleX, maxAngleX, arcZ, sqrRadius, ip1, ip2)
    && (isPointVisible(ip1, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, false,
        nullptr, nullptr) || (ip1 != ip2
    && isPointVisible(ip2, topN, leftN, bottomN, rightN, topD, leftD, bottomD, rightD, false, false, false, false,
        nullptr, nullptr)))))
        return true;
    return false;
}

inline bool OsmAnd::AtlasMapRenderer_OpenGL::isPointVisible(const InternalState& internalState, const glm::vec3& p,
    bool skipTop, bool skipLeft, bool skipBottom, bool skipRight, bool skipFront, bool skipBack, float tolerance) const
{
    const auto top =
        skipTop || glm::dot(p, internalState.topVisibleEdgeN) <= internalState.topVisibleEdgeD + tolerance;
    const auto left =
        skipLeft || glm::dot(p, internalState.leftVisibleEdgeN) <= internalState.leftVisibleEdgeD + tolerance;
    const auto bottm =
        skipBottom || glm::dot(p, internalState.bottomVisibleEdgeN) <= internalState.bottomVisibleEdgeD + tolerance;
    const auto right =
        skipRight || glm::dot(p, internalState.rightVisibleEdgeN) <= internalState.rightVisibleEdgeD + tolerance;
    const auto front =
        skipFront || glm::dot(p, internalState.frontVisibleEdgeN) <= internalState.frontVisibleEdgeD + tolerance;
    const auto back =
        skipBack || glm::dot(p, internalState.backVisibleEdgeN) <= internalState.backVisibleEdgeD + tolerance;
    return top && left && bottm && right && front && back;
}

inline bool OsmAnd::AtlasMapRenderer_OpenGL::isEdgeVisible(const MapRendererInternalState& internalState_,
    const glm::vec3& start, const glm::vec3& end) const
{
    const auto internalState = static_cast<const InternalState*>(&internalState_);

    glm::vec3 ip;
    const auto cp = internalState->worldCameraPosition;
    if (isPointVisible(*internalState, start, false, false, false, false, true, true)
        || isPointVisible(*internalState, end, false, false, false, false, true, true))
        return true;
    if ((Utilities_OpenGL_Common::lineSegmentIntersectPlane(internalState->topVisibleEdgeN, cp, start, end, ip)
        && isPointVisible(*internalState, ip, true, false, false, false, true, true))
        || (Utilities_OpenGL_Common::lineSegmentIntersectPlane(internalState->leftVisibleEdgeN, cp, start, end, ip)
        && isPointVisible(*internalState, ip, false, true, false, false, true, true))
        || (Utilities_OpenGL_Common::lineSegmentIntersectPlane(internalState->bottomVisibleEdgeN, cp, start, end, ip)
        && isPointVisible(*internalState, ip, false, false, true, false, true, true))
        || (Utilities_OpenGL_Common::lineSegmentIntersectPlane(internalState->rightVisibleEdgeN, cp, start, end, ip)
        && isPointVisible(*internalState, ip, false, false, false, true, true, true)))
        return true;
    return false;
}

void OsmAnd::AtlasMapRenderer_OpenGL::getElevationDataLimits(const MapRendererState& state,
    std::shared_ptr<const IMapElevationDataProvider::Data>& elevationData,
    const TileId& tileId, const ZoomLevel zoomLevel, float& minHeight, float& maxHeight) const
{
    const auto scaledMinValue = elevationData->minValue * state.elevationConfiguration.dataScaleFactor;
    const auto scaledMaxValue = elevationData->maxValue * state.elevationConfiguration.dataScaleFactor;
    const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(zoomLevel, tileId.y, TileSize3D);
    const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(zoomLevel, tileId.y + 1, TileSize3D);
    const auto minMetersPerUnit = qMin(upperMetersPerUnit, lowerMetersPerUnit);
    const auto maxMetersPerUnit = qMax(upperMetersPerUnit, lowerMetersPerUnit);
    minHeight = static_cast<float>((scaledMinValue / maxMetersPerUnit) * state.elevationConfiguration.zScaleFactor);
    maxHeight = static_cast<float>((scaledMaxValue / minMetersPerUnit) * state.elevationConfiguration.zScaleFactor);
}

void OsmAnd::AtlasMapRenderer_OpenGL::getElevationDataLimits(const MapRendererState& state,
    std::shared_ptr<const IMapElevationDataProvider::Data>& elevationData,
    const double metersPerUnit, float& minHeight, float& maxHeight) const
{
    const auto factor = state.elevationConfiguration.dataScaleFactor / metersPerUnit
        * state.elevationConfiguration.zScaleFactor;
    minHeight = static_cast<float>(elevationData->minValue * factor);
    maxHeight = static_cast<float>(elevationData->maxValue * factor);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getHeightLimits(const MapRendererState& state, const TileId& tileId,
    const double metersPerUnit, const ZoomLevel zoomLevel, float& minHeight, float& maxHeight) const
{
    if (!state.elevationDataProvider)
        return false;

    const auto minZoom = state.elevationDataProvider->getMinZoom();
    if (zoomLevel < minZoom)
        return false;

    std::shared_ptr<const IMapElevationDataProvider::Data> elevationData;
    if (captureElevationDataResource(state, tileId, zoomLevel, &elevationData))
    {
        if (state.flatEarth)
            getElevationDataLimits(state, elevationData, tileId, zoomLevel, minHeight, maxHeight);
        else
            getElevationDataLimits(state, elevationData, metersPerUnit, minHeight, maxHeight);
        return true;
    }

    const auto maxMissingDataZoomShift = state.elevationDataProvider->getMaxMissingDataZoomShift();
    const auto maxUnderZoomShift = state.elevationDataProvider->getMaxMissingDataUnderZoomShift();
    const auto maxZoom = state.elevationDataProvider->getMaxZoom();
    float minimum, maximum, minValue, maxValue;
    for (int absZoomShift = 1; absZoomShift <= maxMissingDataZoomShift; absZoomShift++)
    {
        // Look for underscaled first. Only full match is accepted.
        // Don't replace tiles of absent zoom levels by the underscaled ones
        const auto underscaledZoom = static_cast<int>(zoomLevel) + absZoomShift;
        if (zoomLevel >= minZoom && underscaledZoom <= maxZoom && absZoomShift <= maxUnderZoomShift)
        {
            auto underscaledTileIdN =
                TileId::fromXY(tileId.x << absZoomShift, tileId.y << absZoomShift);
            const auto underscaledZoomLevel = static_cast<ZoomLevel>(underscaledZoom);
            const int subCount = 1 << absZoomShift;
            minimum = std::numeric_limits<float>::max();
            maximum = std::numeric_limits<float>::lowest();
            bool complete = true;
            for (int yShift = 0; yShift < subCount; yShift++)
            {
                for (int xShift = 0; xShift < subCount; xShift++)
                {
                    if (captureElevationDataResource(state, underscaledTileIdN, underscaledZoomLevel, &elevationData))
                    {
                        if (state.flatEarth)
                            getElevationDataLimits(
                                state, elevationData, underscaledTileIdN, underscaledZoomLevel, minValue, maxValue);
                        else
                            getElevationDataLimits(state, elevationData, metersPerUnit, minValue, maxValue);
                        minimum = qMin(minimum, minValue);
                        maximum = qMax(maximum, maxValue);
                    }
                    else
                        complete = false;
                    underscaledTileIdN.x++;
                }
                underscaledTileIdN.y++;
            }
            if (complete)
            {
                if (state.flatEarth)
                {
                    const auto scaleFactor = static_cast<float>(subCount);
                    minHeight = minimum / scaleFactor;
                    maxHeight = maximum / scaleFactor;
                }
                return true;
            }
        }
        // If underscaled was not found, look for overscaled (surely, if such zoom level exists at all)
        const auto overscaledZoom = static_cast<int>(zoomLevel) - absZoomShift;
        if (overscaledZoom >= minZoom && overscaledZoom <= maxZoom)
        {
            PointF texCoordsOffset;
            PointF texCoordsScale;
            const auto overscaledTileIdN = Utilities::getTileIdOverscaledByZoomShift(
                tileId,
                absZoomShift,
                &texCoordsOffset,
                &texCoordsScale);
            const auto overscaledZoomLevel = static_cast<ZoomLevel>(overscaledZoom);
            if (captureElevationDataResource(state, overscaledTileIdN, overscaledZoomLevel, &elevationData))
            {
                if (state.flatEarth)
                    getElevationDataLimits(
                        state, elevationData, overscaledTileIdN, overscaledZoomLevel, minValue, maxValue);
                else
                    getElevationDataLimits(state, elevationData, metersPerUnit, minValue, maxValue);
                if (state.flatEarth)
                {
                    const auto scaleFactor = static_cast<float>(1 << absZoomShift);
                    minHeight = minValue * scaleFactor;
                    maxHeight = maxValue * scaleFactor;
                }
                return true;
            }
        }
    }

    return false;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getPositionFromScreenPoint(const InternalState& internalState,
    const MapRendererState& state, const PointD& screenPoint, PointI64& location,
    const float height /* = 0.0f */) const
{
    const auto camPos = glm::dvec3(internalState.worldCameraPosition);
    const auto farInWorld = glm::unProject(
        glm::dvec3(screenPoint.x, static_cast<double>(state.windowSize.y) - screenPoint.y, 1.0),
        glm::dmat4(internalState.mCameraView),
        glm::dmat4(internalState.mPerspectiveProjection),
        glm::dvec4(internalState.glmViewport));
    const auto rayD = glm::normalize(farInWorld - camPos);

    if (state.flatEarth)
    {
        const glm::dvec3 planeN(0.0, 1.0, 0.0);
        const glm::dvec3 planeO(0.0, static_cast<double>(height), 0.0);
        double length;
        const auto intersects = Utilities_OpenGL_Common::rayIntersectPlane(planeN, planeO, rayD, camPos, length);
        if (!intersects || !(length > 0.0 && internalState.worldCameraPosition.y > height
            || length < 0.0 && internalState.worldCameraPosition.y < height)
            || (height == 0.0f && glm::dot(rayD, -planeN) < MIN_COS_SURFACE))
            return false;

        const auto point = camPos + length * rayD;

        PointD position = point.xz() / static_cast<double>(TileSize3D);

        position += internalState.targetInTileOffsetN;
        const auto zoomLevelDiff = ZoomLevel::MaxZoomLevel - state.zoomLevel;
        const auto tileSize31 = (1u << zoomLevelDiff);
        position *= tileSize31;

        location.x = static_cast<int64_t>(position.x) + (internalState.targetTileId.x << zoomLevelDiff);
        location.y = static_cast<int64_t>(position.y) + (internalState.targetTileId.y << zoomLevelDiff);
    }
    else
    {
        glm::dvec3 position;
        const auto rayG = internalState.mGlobeRotationPreciseInv * rayD;
        const auto intersects = Utilities_OpenGL_Common::rayIntersectsSphere(
            internalState.cameraRotatedPosition,
            rayG,
            1.0 + static_cast<double>(height) / internalState.globeRadius,
            position,
            true);
        if (!intersects || glm::dot(position - internalState.cameraRotatedPosition, rayG) <= 0.0
            || (height == 0.0f && glm::dot(rayG, -position) < MIN_COS_SURFACE))
            return false;
        
        const auto point = height == 0.0f ? position : glm::normalize(position);

        const auto position31 =
            Utilities::get31FromAngles(PointD(qAtan2(point.x, point.y), -qAsin(qBound(-1.0, point.z, 1.0))));

        location = PointI64(state.target31) + Utilities::shortestLongitudeVector(position31 - state.target31);
    }

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getPositionFromScreenPoint(const InternalState& internalState,
    const MapRendererState& state, const PointD& screenPoint, glm::dvec3& position,
    const float height /* = 0.0f */) const
{
    const auto camPos = glm::dvec3(internalState.worldCameraPosition);
    const auto farInWorld = glm::unProject(
        glm::dvec3(screenPoint.x, static_cast<double>(state.windowSize.y) - screenPoint.y, 1.0),
        glm::dmat4(internalState.mCameraView),
        glm::dmat4(internalState.mPerspectiveProjection),
        glm::dvec4(internalState.glmViewport));
    const auto rayD = glm::normalize(farInWorld - camPos);

    if (state.flatEarth)
    {
        const glm::dvec3 planeN(0.0, 1.0, 0.0);
        const glm::dvec3 planeO(0.0, static_cast<double>(height), 0.0);
        double length;
        const auto intersects = Utilities_OpenGL_Common::rayIntersectPlane(planeN, planeO, rayD, camPos, length);
        if (!intersects || !(length > 0.0 && internalState.worldCameraPosition.y > height
            || length < 0.0 && internalState.worldCameraPosition.y < height)
            || (height == 0.0f && glm::dot(rayD, -planeN) < MIN_COS_SURFACE))
            return false;

        position = camPos + length * rayD;
    }
    else
    {
        const auto intersects = Utilities_OpenGL_Common::rayIntersectsSphere(
            camPos / internalState.globeRadius + glm::dvec3(0.0, 1.0, 0.0),
            rayD,
            1.0 + static_cast<double>(height) / internalState.globeRadius,
            position,
            true);

        position.y -= 1.0;
        position *= internalState.globeRadius;

        if (!intersects || glm::dot(position - camPos, rayD) <= 0.0
            || (height == 0.0f && glm::dot(glm::normalize(glm::dvec3(0.0, -internalState.globeRadius, 0.0) - position),
                rayD) < MIN_COS_SURFACE))
            return false;
    }

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getNearestLocationFromScreenPoint(
    const InternalState& internalState, const MapRendererState& state,
    const PointI& location31, const PointI& screenPoint,
    PointI64& fixedLocation, PointI64& currentLocation) const
{
    bool ok = getPositionFromScreenPoint(internalState, state, PointD(screenPoint), currentLocation);
    if (!ok)
        return false;

    auto offset = Utilities::wrapCoordinates(currentLocation - location31);
    offset = state.flatEarth ? Utilities::shortestVector31(offset) : Utilities::shortestLongitudeVector(offset);
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
    if (!state.elevationDataProvider)
        return ZoomLevel::InvalidZoomLevel;

    const auto minZoom = state.elevationDataProvider->getMinZoom();
    if (zoomLevel < minZoom)
        return ZoomLevel::InvalidZoomLevel;

    const auto maxZoom = state.elevationDataProvider->getMaxZoom();
    if (zoomLevel >= minZoom && zoomLevel <= maxZoom
        && captureElevationDataResource(state, normalizedTileId, zoomLevel, pOutSource))
        return zoomLevel;

    const auto maxMissingDataZoomShift = state.elevationDataProvider->getMaxMissingDataZoomShift();
    const auto maxUnderZoomShift = state.elevationDataProvider->getMaxMissingDataUnderZoomShift();
    for (int absZoomShift = 1; absZoomShift <= maxMissingDataZoomShift; absZoomShift++)
    {
        // Look for underscaled first. Only full match is accepted.
        // Don't replace tiles of absent zoom levels by the underscaled ones
        const auto underscaledZoom = static_cast<int>(zoomLevel) + absZoomShift;
        if (!noUnderscaled && zoomLevel >= minZoom && underscaledZoom <= maxZoom && absZoomShift <= maxUnderZoomShift)
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
    if (const auto& configuration = std::dynamic_pointer_cast<const AtlasMapRendererConfiguration>(getConfiguration()))
    {
        const auto state = getState();
        return configuration->referenceTileSizeOnScreenInPixels * state.visualZoom * (1.0f + state.visualZoomShift);
    }

    return 0.0;
}

float OsmAnd::AtlasMapRenderer_OpenGL::getWorldElevationOfLocation(const MapRendererState& state,
    const float elevationInMeters, const PointI& location31_) const
{
    if (elevationInMeters != 0.0f && elevationInMeters > _invalidElevationValue)
    {
        const auto location31 = Utilities::normalizeCoordinates(location31_, ZoomLevel31);   
        const auto scaledElevationInMeters = elevationInMeters * state.elevationConfiguration.dataScaleFactor;
        double metersPerUnit;
        if (state.flatEarth)
        {
            PointF offsetInTileN;
            TileId tileId = Utilities::getTileId(location31, state.zoomLevel, &offsetInTileN);
            const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, tileId.y, TileSize3D);
            const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, tileId.y + 1, TileSize3D);
            metersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offsetInTileN.y);
        }
        else
            metersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, state.target31, TileSize3D);
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
        const auto scaledElevation = elevation / state.elevationConfiguration.zScaleFactor;
        double metersPerUnit;
        if (state.flatEarth)
        {
            PointF offsetInTileN;
            TileId tileId = Utilities::getTileId(location31, zoom, &offsetInTileN);
            const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(zoom, tileId.y, TileSize3D);
            const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(zoom, tileId.y + 1, TileSize3D);
            metersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offsetInTileN.y);
        }
        else
            metersPerUnit = Utilities::getMetersPerTileUnit(zoom, state.target31, TileSize3D);
        return scaledElevation * metersPerUnit / state.elevationConfiguration.dataScaleFactor;
    }
    else
        return 0.0f;
}

double OsmAnd::AtlasMapRenderer_OpenGL::getDistanceFactor(const MapRendererState& state, const float tileSize,
    double& baseUnits, float& sinAngleToPlane) const
{
    const auto viewportWidth = state.viewport.width();
    const auto viewportHeight = state.viewport.height();
    if (viewportWidth == 0 || viewportHeight == 0)
        return 1.0;
    const auto glmViewport = glm::vec4(
        state.viewport.left(),
        state.windowSize.y - state.viewport.bottom(),
        state.viewport.width(),
        state.viewport.height());
    const auto aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    const auto projectionPlaneHalfHeight = static_cast<float>(_zNear * qTan(qDegreesToRadians(state.fieldOfView)));
    const auto projectionPlaneHalfWidth = static_cast<float>(projectionPlaneHalfHeight * aspectRatio);
    const auto mPerspectiveProjection = glm::frustum(
        -projectionPlaneHalfWidth, projectionPlaneHalfWidth,
        -projectionPlaneHalfHeight, projectionPlaneHalfHeight,
        _zNear, 1000.0f);
    baseUnits = static_cast<double>(TileSize3D)
        * 0.5 * mPerspectiveProjection[0][0] * state.viewport.width()
        / ((1.0f + state.visualZoomShift) * tileSize);
    const auto distanceToTarget = static_cast<float>(baseUnits / state.visualZoom);
    const auto mDistance = glm::translate(glm::vec3(0.0f, 0.0f, -distanceToTarget));
    const auto mElevation = glm::rotate(glm::radians(state.elevationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    const auto mAzimuth = glm::rotate(glm::radians(state.azimuth), glm::vec3(0.0f, 1.0f, 0.0f));
    const auto mCameraView = mDistance * mElevation * mAzimuth;
    const auto mDistanceInv = glm::translate(glm::vec3(0.0f, 0.0f, distanceToTarget));
    const auto mElevationInv = glm::rotate(glm::radians(-state.elevationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    const auto mAzimuthInv = glm::rotate(glm::radians(-state.azimuth), glm::vec3(0.0f, 1.0f, 0.0f));
    const auto mCameraViewInv = mAzimuthInv * mElevationInv * mDistanceInv;
    const auto worldCameraPosition = (mCameraViewInv * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz();
    double distanceToPoint = 0.0;
    const auto camPos = glm::dvec3(worldCameraPosition);
    const auto farInWorld = glm::unProject(
        glm::dvec3(state.fixedPixel.x, static_cast<double>(state.windowSize.y) - state.fixedPixel.y, 1.0),
        glm::dmat4(mCameraView),
        glm::dmat4(mPerspectiveProjection),
        glm::dvec4(glmViewport));
    const auto rayD = glm::normalize(farInWorld - camPos);

    const glm::dvec3 planeN(0.0f, 1.0f, 0.0f);
    if (state.flatEarth)
    {
        const glm::dvec3 planeO(0.0f, 0.0f, 0.0f);
        const auto ok = Utilities_OpenGL_Common::rayIntersectPlane(planeN, planeO, rayD, camPos, distanceToPoint);
        if (!ok || distanceToPoint <= 0.0)
            return 1.0;
    }
    else
    {
        const auto metersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, state.target31, TileSize3D);
        const auto radiusInWorld = _radius / metersPerUnit;

        glm::dvec3 position;
        const auto ok = Utilities_OpenGL_Common::rayIntersectsSphere(
            camPos / radiusInWorld + glm::dvec3(0.0, 1.0, 0.0),
            rayD,
            1.0,
            position,
            true);
        if (!ok)
            return false;
        
        position.y -= 1.0;
        position *= radiusInWorld;

        distanceToPoint = qMax(glm::dot(position - camPos, rayD), 0.0);
    }

    sinAngleToPlane = -static_cast<float>(glm::dot(rayD, planeN));

    return static_cast<double>(distanceToTarget) / distanceToPoint;
}

OsmAnd::ZoomLevel OsmAnd::AtlasMapRenderer_OpenGL::getSurfaceZoom(
    const MapRendererState& state, float& surfaceVisualZoom) const
{
    if (state.fixedPixel.x < 0 || state.fixedPixel.y < 0)
    {
        surfaceVisualZoom = state.visualZoom;
        return state.zoomLevel;
    }

    // Calculate distance factor
    const auto referenceTileSizeOnScreenInPixels =
        static_cast<const AtlasMapRendererConfiguration*>(&(*getConfiguration()))->referenceTileSizeOnScreenInPixels;
    double baseUnits = 0.0;
    float sinAngle = 0.0f;
    const auto distanceFactor = getDistanceFactor(state, referenceTileSizeOnScreenInPixels, baseUnits, sinAngle);
    if (sinAngle == 0.0f)
    {
        surfaceVisualZoom = state.visualZoom;
        return state.zoomLevel;
    }

    // Calculate zoom to the surface
    const auto elevation = static_cast<double>(state.fixedHeight)
        / (state.fixedHeight == 0.0f ? 1.0 : static_cast<double>(1u << state.fixedZoomLevel));
    const auto distanceToSurface = baseUnits
        / (state.visualZoom * static_cast<double>(1u << state.zoomLevel) * distanceFactor) - elevation / sinAngle;
    if (distanceToSurface <= 0.0)
    {
        surfaceVisualZoom = state.visualZoom;
        return state.zoomLevel;
    }
    const auto scaleFactor = baseUnits / distanceToSurface;
    const auto minZoom = qCeil(log2(scaleFactor / _maximumVisualZoom));
    const auto maxZoom = qFloor(log2(scaleFactor / _minimumVisualZoom));
    auto resultZoom = maxZoom;
    if (state.surfaceZoomLevel >= MinZoomLevel && state.surfaceZoomLevel <= MaxZoomLevel
        && qAbs(state.surfaceZoomLevel - minZoom) < qAbs(state.surfaceZoomLevel - maxZoom))
        resultZoom = minZoom;
    if (resultZoom < state.zoomLevel)
        resultZoom = state.zoomLevel;

    float visZoom = 1.0f;
    if (resultZoom > state.maxZoomLimit)
        resultZoom = state.maxZoomLimit;
    else if (resultZoom < state.minZoomLimit)
        resultZoom = state.minZoomLimit;
    else
        visZoom = static_cast<float>(scaleFactor / static_cast<double>(1u << resultZoom));

    if ((resultZoom == state.maxZoomLimit && visZoom > 1.0f) || (resultZoom == state.minZoomLimit && visZoom < 1.0f))
        surfaceVisualZoom = 1.0f;
    else
        surfaceVisualZoom = visZoom;

    return static_cast<ZoomLevel>(resultZoom);
}

OsmAnd::ZoomLevel OsmAnd::AtlasMapRenderer_OpenGL::getFlatZoom(const MapRendererState& state,
    const ZoomLevel surfaceZoomLevel, const float surfaceVisualZoom,
    const double pointElevation, float& flatVisualZoom) const
{
    if (state.fixedPixel.x < 0 || state.fixedPixel.y < 0)
    {
        flatVisualZoom = surfaceVisualZoom;
        return surfaceZoomLevel;
    }

    // Calculate distance factor
    const auto referenceTileSizeOnScreenInPixels =
        static_cast<const AtlasMapRendererConfiguration*>(&(*getConfiguration()))->referenceTileSizeOnScreenInPixels;
    double baseUnits = 0.0;
    float sinAngle = 0.0f;
    const auto distanceFactor = getDistanceFactor(state, referenceTileSizeOnScreenInPixels, baseUnits, sinAngle);
    if (sinAngle == 0.0f)
    {
        flatVisualZoom = surfaceVisualZoom;
        return surfaceZoomLevel;
    }

    // Calculate native zoom on the plane ("flat")
    const auto distanceToCenter = (baseUnits / (surfaceVisualZoom * static_cast<double>(1u << surfaceZoomLevel))
        + pointElevation / sinAngle) * distanceFactor;
    const auto scaleFactor = baseUnits / distanceToCenter;
    const auto minZoom = qCeil(log2(scaleFactor / _maximumVisualZoom));
    const auto maxZoom = qFloor(log2(scaleFactor / _minimumVisualZoom));
    auto resultZoom = qAbs(state.zoomLevel - minZoom) < qAbs(state.zoomLevel - maxZoom) ? minZoom : maxZoom;
    if (resultZoom > surfaceZoomLevel)
        resultZoom = surfaceZoomLevel;

    float visZoom = 1.0f;
    if (resultZoom > state.maxZoomLimit)
        resultZoom = state.maxZoomLimit;
    else if (resultZoom < state.minZoomLimit)
        resultZoom = state.minZoomLimit;
    else
        visZoom = static_cast<float>(scaleFactor / static_cast<double>(1u << resultZoom));

    if ((resultZoom == state.maxZoomLimit && visZoom > 1.0f) || (resultZoom == state.minZoomLimit && visZoom < 1.0f))
        flatVisualZoom = 1.0f;
    else
        flatVisualZoom = visZoom;

    return static_cast<ZoomLevel>(resultZoom);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getLocationFromScreenPoint(const PointI& screenPoint, PointI& location31) const
{
    PointI64 location;
    if (!getLocationFromScreenPoint(PointD(screenPoint), location))
        return false;
    location31 = Utilities::normalizeCoordinates(location, ZoomLevel31);

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getLocationFromScreenPoint(const PointD& screenPoint, PointI& location31) const
{
    PointI64 location;
    if (!getLocationFromScreenPoint(screenPoint, location))
        return false;
    location31 = Utilities::normalizeCoordinates(location, ZoomLevel31);

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getLocationFromScreenPoint(const PointD& screenPoint, PointI64& location) const
{
    const auto state = getState();

    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), RequiredToUnproject);
    if (!ok)
        return false;

    if (!state.flatEarth)
    {
        internalState.metersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, state.target31, TileSize3D);
        internalState.globeRadius = _radius / internalState.metersPerUnit;
        internalState.mGlobeRotationPrecise =
            Utilities::getGlobalRotationMatrix(Utilities::getAnglesFrom31(state.target31));
        internalState.mGlobeRotationPreciseInv = glm::transpose(internalState.mGlobeRotationPrecise);
        internalState.cameraRotatedPosition = internalState.mGlobeRotationPreciseInv
            * (glm::dvec3(internalState.worldCameraPosition) / internalState.globeRadius + glm::dvec3(0.0, 1.0, 0.0));
    }

    ok = getPositionFromScreenPoint(internalState, state, screenPoint, location);
    if (!ok)
        return false;

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getLocationFromElevatedPoint(
    const PointI& screenPoint, PointI& location31, float* heightInMeters /*=nullptr*/) const
{
    bool ok;
    MapRendererState state;
    const auto internalState = getRequiredInternalState(ok, &state);
    if (!ok)
        return false;
    
    PointI64 location;
    ok = getPositionFromScreenPoint(internalState, state, PointD(screenPoint), location);
    if (!ok)
        return false;

    if (!state.elevationDataProvider || state.zoomLevel < state.elevationDataProvider->getMinZoom())
    {
        location31 = Utilities::normalizeCoordinates(location, ZoomLevel31);
        return true;
    }

    const auto zoomLevelDiff = ZoomLevel::MaxZoomLevel - state.zoomLevel;
    const auto tileSize31 = (1u << zoomLevelDiff);

    PointI64 cameraLocation;
    if (state.flatEarth)
    {
        auto position = PointD(internalState.groundCameraPosition);
        position /= static_cast<double>(TileSize3D);
        position += internalState.targetInTileOffsetN;
        position *= tileSize31;

        cameraLocation.x = static_cast<int64_t>(position.x) + (internalState.targetTileId.x << zoomLevelDiff);
        cameraLocation.y = static_cast<int64_t>(position.y) + (internalState.targetTileId.y << zoomLevelDiff);
    }
    else
        cameraLocation = location - Utilities::shortestLongitudeVector(
            Utilities::wrapCoordinates(location - Utilities::get31FromAngles(internalState.cameraAngles)));

    if (cameraLocation.x == location.x && cameraLocation.y == location.y)
    {
        location31 = Utilities::normalizeCoordinates(location, ZoomLevel31);
        if (heightInMeters)
        {
            const auto height = getLocationHeightInMeters(state, location31);
            if (height > _invalidElevationValue)
                *heightInMeters = height;
        }
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
    auto tilesCount = 1 + internalState.tilesToHorizon;
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
        double upperMetersPerUnit, lowerMetersPerUnit, midMetersPerUnit;
        if (state.flatEarth)
        {
            upperMetersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, tileId.y, TileSize3D);
            lowerMetersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, tileId.y + 1, TileSize3D);
            midMetersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, midPointOffset.y);
        }
        else
            midMetersPerUnit = internalState.metersPerUnit;
        const auto midElevationFactor = midMetersPerUnit / elevationScaleFactor;
        auto scaledZoom = InvalidZoomLevel;
        if (midPointZ * midElevationFactor < _maximumHeightFromSeaLevelInMeters)
            scaledZoom = getElevationData(state, tileId, state.zoomLevel, scaledStart, true, &elevationData);
        if (scaledZoom != InvalidZoomLevel && elevationData)
        {
            const double tileSize = 1ull << (state.zoomLevel - scaledZoom);
            const auto scaledDistance = (midPointOffset - startPointOffset) / tileSize;
            PointF scaledEnd(scaledStart.x + scaledDistance.x, scaledStart.y + scaledDistance.y);
            const auto startMetersPerUnit = state.flatEarth
                ? glm::mix(upperMetersPerUnit, lowerMetersPerUnit, startPointOffset.y) : internalState.metersPerUnit;
            const auto startElevationFactor = startMetersPerUnit / elevationScaleFactor;
            PointF exactLocation;
            float exactHeight = 0.0f;
            if (elevationData->getClosestPoint(
                static_cast<float>(startElevationFactor), static_cast<float>(midElevationFactor),
                scaledStart, startPointZ, scaledEnd, midPointZ, tileSize, exactLocation, &exactHeight))
            {
                const PointD scaledLocation(startPointOffset.x + tileSize * (exactLocation.x - scaledStart.x),
                    startPointOffset.y + tileSize * (exactLocation.y - scaledStart.y));
                PointI64 finalLocation(
                    static_cast<int64_t>((static_cast<double>(scaledLocation.x) + startTileId.x) * tileSize31),
                    static_cast<int64_t>((static_cast<double>(scaledLocation.y) + startTileId.y) * tileSize31));
                if (finalLocation.x != cameraLocation.x || finalLocation.y != cameraLocation.y)
                {
                    location31 = Utilities::normalizeCoordinates(finalLocation, ZoomLevel31);
                    if (heightInMeters)
                        *heightInMeters = exactHeight;
                }
                else
                {
                    location31 = Utilities::normalizeCoordinates(location, ZoomLevel31);
                    if (heightInMeters)
                    {
                        const auto height = getLocationHeightInMeters(state, location31);
                        if (height > _invalidElevationValue)
                            *heightInMeters = height;
                    }
                }
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

bool OsmAnd::AtlasMapRenderer_OpenGL::getExtraZoomAndTiltForRelief(
    const MapRendererState& state, PointF& zoomAndTilt) const
{
    const auto extraElevation = _internalState.extraElevation;
    if (extraElevation > 0.0f)
    {
        const auto currentPoint = _internalState.worldCameraPosition;
        const auto elevatedPoint = glm::vec3(currentPoint.x, currentPoint.y + extraElevation, currentPoint.z);
        const auto elevatedLength = glm::length(elevatedPoint);
        const auto currentLength = glm::length(currentPoint);
        if (qFuzzyIsNull(elevatedLength) || qFuzzyIsNull(currentLength))
            return false;

        const auto deltaZoom = getZoomOffset(state.zoomLevel, state.visualZoom, currentLength / elevatedLength);
        if (qIsNaN(deltaZoom) || deltaZoom > 0.0)
            return false;

        const auto neededAngle = qRadiansToDegrees(
            qAcos(qBound(0.0f, glm::dot(elevatedPoint / elevatedLength, currentPoint / currentLength), 1.0f)));

        if (deltaZoom > -0.01 && neededAngle < 0.01)
            return false;

        zoomAndTilt.x = static_cast<float>(deltaZoom / 10.0);
        zoomAndTilt.y = static_cast<float>(neededAngle / 10.0);

        return true;
    }
    return false;
}

float OsmAnd::AtlasMapRenderer_OpenGL::getHeightAndLocationFromElevatedPoint(
    const PointI& screenPoint, PointI& location31) const
{
    float elevationInMeters = 0.0f;
    if (!getLocationFromElevatedPoint(screenPoint, location31, &elevationInMeters))
        elevationInMeters = _invalidElevationValue;
    return elevationInMeters;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getExtraZoomAndRotationForAiming(const MapRendererState& state,
    const PointI& firstLocation31, const float firstHeightInMeters, const PointI& firstPoint,
    const PointI& secondLocation31, const float secondHeightInMeters, const PointI& secondPoint,
    PointD& zoomAndRotate) const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), RequiredToProject);
    if (!ok)
        return false;

    // Calculate location on the plane for the first target point
    PointI64 firstNeeded;
    PointI64 firstCurrent;
    ok = getNearestLocationFromScreenPoint(internalState, state,
        firstLocation31, firstPoint, firstNeeded, firstCurrent);
    if (!ok)
        return false;
        
    // Calculate location on the plane for the second target point
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
    PointD segmentRatio(firstHeightInMeters, secondHeightInMeters);
    segmentRatio /= internalState.distanceFromCameraToGroundInMeters / state.elevationConfiguration.zScaleFactor;
    if (segmentRatio.x >= 1.0 || segmentRatio.y >= 1.0)
        segmentRatio = PointD();
    const PointD segmentOffset(segmentLength.x * segmentRatio.x, segmentLength.y * segmentRatio.y);
    const PointD currentSegment(secondCurrent - firstCurrent);
    const auto currentDistance = currentSegment.norm();
    if (currentDistance == 0.0)
        return false;
    const PointD currentSegmentN = currentSegment / currentDistance;
    const auto firstAngle =
        qAcos(qBound(-1.0, Utilities::dotProduct(firstSegment / segmentLength.x, currentSegmentN), 1.0));
    const auto secondAngle =
        qAcos(qBound(-1.0, Utilities::dotProduct(secondSegment / segmentLength.y, PointD() - currentSegmentN), 1.0));
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
    // using new screen coordinates of the second target point, assuming the first one is a pivot point
    const auto deltaZoom = getZoomOffset(state.zoomLevel, state.visualZoom, distanceFactor);
    if (qIsNaN(deltaZoom))
        return false;

    const auto actualSegment = PointD(PointD(secondCurrent) + secondSegment * zoomedRatio.y -
        PointD(firstCurrent) - firstSegment * zoomedRatio.x);
    const auto sqrActualDistance = actualSegment.squareNorm();
    if (sqrActualDistance == 0.0)
        return false;
    const auto actualSegmentN = actualSegment / qSqrt(sqrActualDistance);
    const auto neededSegmentN = neededSegment / qSqrt(sqrNeededDistance);
    const auto neededAngle = qRadiansToDegrees(Utilities::getSignedAngle(actualSegmentN, neededSegmentN));
    if (qIsNaN(neededAngle))
        return false;

    zoomAndRotate.x = deltaZoom;
    zoomAndRotate.y = neededAngle;

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getTiltAndRotationForAiming(const MapRendererState& state,
    const PointI& firstLocation31, const float firstHeight, const PointI& firstPoint,
    const PointI& secondLocation31, const float secondHeight, const PointI& secondPoint,
    PointD& tiltAndRotate) const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), LocalGeometry);
    if (!ok)
        return false;

    // Get first (fixed) point in world coordinates
    auto onPlane = Utilities::convert31toDouble(Utilities::shortestVector31(state.target31, firstLocation31),
        state.zoomLevel) * static_cast<float>(AtlasMapRenderer::TileSize3D);
    const auto firstCurrentPoint = glm::dvec3(onPlane.x, firstHeight, onPlane.y);

    // Get camera position in world coordinates
    const auto cameraPosition = glm::dvec3(internalState.worldCameraPosition);
    ok = firstCurrentPoint != cameraPosition;
    if (!ok)
        return false;

    // Get distance from camera to fixed point
    const auto camFixDistance = glm::distance(cameraPosition, firstCurrentPoint);

    // Get second point (aim) in world coordinates
    onPlane = Utilities::convert31toDouble(Utilities::shortestVector31(state.target31, secondLocation31),
        state.zoomLevel) * static_cast<float>(AtlasMapRenderer::TileSize3D);
    const auto secondCurrentPoint = glm::dvec3(onPlane.x, secondHeight, onPlane.y);
    ok = firstCurrentPoint != secondCurrentPoint;
    if (!ok)
        return false;

    // Get distance between points
    const auto distance = glm::distance(firstCurrentPoint, secondCurrentPoint);

    // Get both points projected to near plane
    ok = isPointProjectable(internalState, firstCurrentPoint) && isPointProjectable(internalState, secondCurrentPoint);
    if (!ok)
        return false;
    const auto mCameraView = glm::dmat4(internalState.mCameraView);
    const auto mPerspectiveProjection = glm::dmat4(internalState.mPerspectiveProjection);
    const auto glmViewport = glm::dvec4(internalState.glmViewport);
    const auto firstPointNear = glm::unProject(glm::dvec3(firstPoint.x, state.windowSize.y - firstPoint.y, 0.0),
        mCameraView, mPerspectiveProjection, glmViewport);
    const auto secondPointNear = glm::unProject(glm::dvec3(secondPoint.x, state.windowSize.y - secondPoint.y, 0.0),
        mCameraView, mPerspectiveProjection, glmViewport);
    ok = firstPointNear != secondPointNear;
    if (!ok)
        return false;

    // Get angle between two rays (from camera to fixed point and to aim)
    const auto firstVector = firstPointNear - cameraPosition;
    const auto firstLength = glm::length(firstVector);
    const auto firstVectorN = firstVector / firstLength;
    const auto secondVector = secondPointNear - cameraPosition;
    const auto secondLength = glm::length(secondVector);
    const auto secondVectorN = secondVector / secondLength;
    const auto lengthFactor = secondLength / firstLength;
    const auto camAngleCos = glm::dot(firstVectorN, secondVectorN);
    ok = camAngleCos > -1.0 && camAngleCos < 1.0;
    if (!ok)
        return false;

    // Compute distance to aim
    const auto firstPart = camFixDistance * camAngleCos;
    const auto height = camFixDistance * qSin(qAcos(camAngleCos));
    const auto sqrHeight = height * height;
    auto sqrDistance = distance * distance;
    const auto withTilt = sqrDistance > sqrHeight;
    const auto secondPart = withTilt ? qSqrt(sqrDistance - sqrHeight) : 0.0;
    auto camAimDistance = firstPart + secondPart;
    ok = camAimDistance > _zNear;
    if (!ok)
        return false;

    // Prepare necessary vectors to get azimuth and elevation angles
    const auto currentVectorN = (secondCurrentPoint - firstCurrentPoint) / distance;
    ok = currentVectorN.x != 0.0 || currentVectorN.y != 0.0 || currentVectorN.z != 0.0;
    if (!ok)
        return false;
    const auto firstNeededPoint = cameraPosition + firstVectorN * camFixDistance;
    double angles[2];
    double tilts[2];
    double yaws[2];
    int count = 0;
    for (int i = 0; i < 3; i++)
    {
        if (i == 2 || !withTilt)
        {            
            i = 2;
            if (count == 0)
                camAimDistance = camFixDistance * lengthFactor;
            else
                break;
        }
        else if (i == 1)
            camAimDistance = firstPart - secondPart;

        ok = camAimDistance > _zNear;
        if (!ok)
            continue;

        // Get aiming point coordinates as if necessary world rotation had been made
        const auto secondNeededPoint = cameraPosition + secondVectorN * camAimDistance;
        ok = firstNeededPoint != secondNeededPoint;
        if (!ok)
            continue;
        const auto neededVectorN = glm::normalize(secondNeededPoint - firstNeededPoint);
        ok = neededVectorN.x != 0.0 || neededVectorN.y != 0.0 || neededVectorN.z != 0.0;
        if (!ok)
            continue;

        // Get rotation angle
        const auto rotationAngleCos = qMax(glm::dot(neededVectorN, currentVectorN), -1.0);
        if (rotationAngleCos >= 1.0)
        {
            // Keep same angles
            angles[count] = 0.0;
            tilts[count] = state.elevationAngle;
            yaws[count++] = state.azimuth;
            break;
        }

        // Obtain azimuth delta from vector projections on horizontal plane
        auto deltaAzimuth = 0.0;
        if ((currentVectorN.x != 0.0 || currentVectorN.z != 0.0) && (neededVectorN.x != 0.0 || neededVectorN.z != 0.0))
        {
            const auto currentOnPlaneN = glm::normalize(glm::dvec3(currentVectorN.x, 0.0, currentVectorN.z));
            const auto neededOnPlaneN = glm::normalize(glm::dvec3(neededVectorN.x, 0.0, neededVectorN.z));
            deltaAzimuth = (glm::cross(currentOnPlaneN, neededOnPlaneN).y < 0.0 ? -1.0 : 1.0) *
                qAcos(qBound(-1.0, glm::dot(currentOnPlaneN, neededOnPlaneN), 1.0));
        }
        auto currentAzimuth = glm::radians(static_cast<double>(state.azimuth));
        auto changedAzimuth = Utilities::normalizedAngleRadians(currentAzimuth + deltaAzimuth);

        // Obtain elevation angle delta from vector projections on vertical planes
        auto deltaElevationAngle = 0.0;
        auto mAzimuth = glm::rotate(changedAzimuth, glm::dvec3(0.0f, 1.0f, 0.0f));
        const auto currentReadyN = mAzimuth * glm::dvec4(currentVectorN, 1.0);
        mAzimuth = glm::rotate(currentAzimuth, glm::dvec3(0.0f, 1.0f, 0.0f));
        const auto neededReadyN = mAzimuth * glm::dvec4(neededVectorN, 1.0);
        if ((currentReadyN.y != 0.0 || currentReadyN.z != 0.0) && (neededReadyN.y != 0.0 || neededReadyN.z != 0.0))
        {
            const auto currentOnVPlaneN = glm::normalize(glm::dvec3(0.0, currentReadyN.y, currentReadyN.z));
            const auto neededOnVPlaneN = glm::normalize(glm::dvec3(0.0, neededReadyN.y, neededReadyN.z));
            deltaElevationAngle = (glm::cross(currentOnVPlaneN, neededOnVPlaneN).x < 0.0 ? -1.0 : 1.0) *
                qAcos(qBound(-1.0, glm::dot(currentOnVPlaneN, neededOnVPlaneN), 1.0));
        }
        auto currentElevationAngleDeg = static_cast<double>(state.elevationAngle);
        const auto changedElevationAngleDeg =
            Utilities::normalizedAngleDegrees(currentElevationAngleDeg + glm::degrees(deltaElevationAngle));

        // Ignore irrelevant elevation angles
        if (changedElevationAngleDeg < 0.0 || changedElevationAngleDeg > (i < 2 ? 90.0 : 90.01))
            continue;

        angles[count] = qAcos(rotationAngleCos);
        tilts[count] = changedElevationAngleDeg;
        yaws[count++] = glm::degrees(changedAzimuth);
    };

    if (count == 0)
        return false;

    int resultIndex = count > 1 && qAbs(angles[1]) < qAbs(angles[0]) ? 1 : 0;

    if (tilts[resultIndex] < _minimumElevationAngle)
        return false;

    tiltAndRotate.x = qMin(tilts[resultIndex], 90.0);
    tiltAndRotate.y = yaws[resultIndex];

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getZoomAndRotationAfterPinch(
    const PointI& firstLocation31, const float firstHeightInMeters, const PointI& firstPoint,
    const PointI& secondLocation31, const float secondHeightInMeters, const PointI& secondPoint,
    PointD& zoomAndRotate) const
{
    const auto state = getState();
   
    auto result = getExtraZoomAndRotationForAiming(state,
        firstLocation31, firstHeightInMeters, firstPoint,
        secondLocation31, secondHeightInMeters, secondPoint, zoomAndRotate);

    return result;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getTiltAndRotationAfterMove(
    const PointI& firstLocation31, const float firstHeightInMeters, const PointI& firstPoint,
    const PointI& secondLocation31, const float secondHeightInMeters, const PointI& secondPoint,
    PointD& tiltAndRotate) const
{
    const auto state = getState();

    auto result = getTiltAndRotationForAiming(state,
        firstLocation31,
        getWorldElevationOfLocation(state, firstHeightInMeters, firstLocation31),
        firstPoint,
        secondLocation31,
        getWorldElevationOfLocation(state, secondHeightInMeters, secondLocation31),
        secondPoint,
        tiltAndRotate);

    return result;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isLocationHeightAvailable(const MapRendererState& state,
    const PointI& location31_) const
{
    const auto location31 = Utilities::normalizeCoordinates(location31_, ZoomLevel31);   

    // Get elevation data
    PointF offsetInTileN;
    TileId tileId = Utilities::getTileId(location31, state.zoomLevel, &offsetInTileN);
    std::shared_ptr<const IMapElevationDataProvider::Data> elevationData;
    if (getElevationData(state, tileId, state.zoomLevel, offsetInTileN, false, &elevationData) != InvalidZoomLevel &&
        elevationData)
    {
        return true;
    }
    return false;
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
    if (getElevationData(state, tileId, state.zoomLevel, offsetInTileN, false, &elevationData) != InvalidZoomLevel &&
        elevationData)
    {
        elevationData->getValue(offsetInTileN, elevationInMeters);
    }
    return elevationInMeters;
}

float OsmAnd::AtlasMapRenderer_OpenGL::getLocationHeightInMeters(const PointI& location31) const
{
    const auto state = getState();

    return getLocationHeightInMeters(state, location31);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getNewTargetAndZoom(const MapRendererState& state,
    const PointI& screenPoint, const PointI& location31, const float height,
    PointI& target31, ZoomLevel& zoomLevel, float& visualZoom, double* shiftInPixels /* = nullptr */) const
{
    InternalState internalState;
    bool ok = updateInternalState(internalState, state, *getConfiguration(), RequiredToUnproject);
    if (!ok)
        return false;

    internalState.metersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, state.target31, TileSize3D);
    if (!state.flatEarth)
        internalState.globeRadius = _radius / internalState.metersPerUnit;

    glm::dvec3 point;
    ok = getPositionFromScreenPoint(internalState, state, PointD(screenPoint), point, height);
    if (!ok)
    {
        target31 = PointI(-1, -1);
        return true;
    }

    if (state.flatEarth)
    {
        PointD position = point.xz() / static_cast<double>(TileSize3D);

        const auto zoomLevelDiff = ZoomLevel::MaxZoomLevel - state.zoomLevel;
        const auto tileSize31 = (1u << zoomLevelDiff);
        position *= tileSize31;

        PointI64 target;
        target.x = static_cast<int64_t>(location31.x) - static_cast<int64_t>(position.x);
        target.y = static_cast<int64_t>(location31.y) - static_cast<int64_t>(position.y);

        target31 = Utilities::normalizeCoordinates(target, ZoomLevel31);

        if (shiftInPixels)
        {
            const auto angles = Utilities::getAnglesFrom31(target31) / M_PI * 180.0;
            const auto prevAngles = Utilities::getAnglesFrom31(state.target31) / M_PI * 180.0;
            const auto distanceInMeters =
                Utilities::distance(OsmAnd::LatLon(angles.y, angles.x), OsmAnd::LatLon(prevAngles.y, prevAngles.x));
            const auto pTile = internalState.referenceTileSizeOnScreenInPixels * internalState.tileOnScreenScaleFactor;
            *shiftInPixels = distanceInMeters * pTile / (internalState.metersPerUnit * TileSize3D);
            target31 = PointI(0, 0);
            return true;
        }
    }
    else
    {
        point /= internalState.globeRadius;
        point.y += 1.0;
        if (height != 0.0f)
            point = glm::normalize(point);

        const auto mGlobeRotation = Utilities::getGlobalRotationMatrix(Utilities::getAnglesFrom31(state.target31));
        const auto mGlobeRotationInv = glm::transpose(mGlobeRotation);

        const auto currentV = mGlobeRotationInv * point;
        const auto neededV =  Utilities::getGlobeRadialVector(location31);
        if (shiftInPixels)
        {
            const auto v = glm::cross(neededV, currentV);
            const auto l = glm::length(v);
            if (qFuzzyIsNull(l))
            {
                target31 = PointI(-1, -1);
                return true;
            }
            const auto a = qAtan2(l, glm::dot(neededV, currentV));
            const auto pTile = internalState.referenceTileSizeOnScreenInPixels * internalState.tileOnScreenScaleFactor;
            *shiftInPixels = a * _radius * pTile / (internalState.metersPerUnit * TileSize3D);
            target31 = PointI(0, 0);
            return true;
        }
        const auto z = glm::dvec3(0.0, 0.0, 1.0);
        const auto currentN = glm::normalize(glm::cross(z, currentV));
        const auto n = glm::normalize(glm::cross(z, neededV));
        const auto d = glm::normalize(glm::cross(n, neededV));
        const auto m = mGlobeRotation * glm::dmat3(glm::normalize(glm::cross(currentN, currentV)), currentN, currentV)
            * glm::dmat3(d.x, n.x, neededV.x, d.y, n.y, neededV.y, d.z, n.z, neededV.z);
        target31 = Utilities::get31FromAngles(PointD(qAtan2(-m[1].x, m[0].x), qAsin(qBound(-1.0, -m[2].y, 1.0))));

        const auto futureMetersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, target31, TileSize3D);
        auto scaleFactor = futureMetersPerUnit / internalState.metersPerUnit;
        scaleFactor *= static_cast<double>(1u << state.zoomLevel) * state.visualZoom;
        const auto minZoom = qCeil(log2(scaleFactor / _maximumVisualZoom));
        const auto maxZoom = qFloor(log2(scaleFactor / _minimumVisualZoom));
        auto resultZoom = qAbs(state.zoomLevel - minZoom) < qAbs(state.zoomLevel - maxZoom) ? minZoom : maxZoom;

        float visZoom = 1.0f;
        if (resultZoom > state.maxZoomLimit)
            resultZoom = state.maxZoomLimit;
        else if (resultZoom < state.minZoomLimit)
            resultZoom = state.minZoomLimit;
        else
            visZoom = static_cast<float>(scaleFactor / static_cast<double>(1u << resultZoom));

        if ((resultZoom == state.maxZoomLimit && visZoom > 1.0f)
            || (resultZoom == state.minZoomLimit && visZoom < 1.0f))
            visualZoom = 1.0f;
        else
            visualZoom = visZoom;

        zoomLevel = static_cast<ZoomLevel>(resultZoom);
    }

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getNewTargetAndZoom(const MapRendererState& state,
    const PointI& location31, const float height,
    PointI& target31, ZoomLevel& zoomLevel, float& visualZoom, PointI& screenPoint) const
{
    bool res;
    const auto point = screenPoint;
    if (state.fixedLocation31 == location31 && state.fixedPixel.x >= 0 && state.fixedPixel.y >= 0
        && (state.fixedPixel.x != screenPoint.x || state.fixedPixel.y != screenPoint.y))
    {
        double shiftInPixels;
        res = getNewTargetAndZoom(state, point, location31, height, target31, zoomLevel, visualZoom, &shiftInPixels);
        if (res && target31.x >= 0)
        {
            const auto v = PointD(screenPoint - state.fixedPixel);
            const auto f = 1.0 - (shiftInPixels >= 1.0 ? qMin(v.norm() / shiftInPixels, 1.0) : 1.0);
            const auto r = PointD(state.fixedPixel) + v * (1.0 - f * f * f * f);
            const auto midPoint = PointI(static_cast<int>(qRound(r.x)), static_cast<int>(qRound(r.y)));
            res = getNewTargetAndZoom(state, midPoint, location31, height, target31, zoomLevel, visualZoom);
            if (res && target31.x >= 0)
                screenPoint = midPoint;
        }
    }
    else
        res = getNewTargetAndZoom(state, point, location31, height, target31, zoomLevel, visualZoom);

    return res;
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

            double metersPerUnit;
            if (state.flatEarth)
            {
                const auto upperMetersPerUnit =
                    Utilities::getMetersPerTileUnit(state.zoomLevel, tileId.y, TileSize3D);
                const auto lowerMetersPerUnit =
                    Utilities::getMetersPerTileUnit(state.zoomLevel, tileId.y + 1, TileSize3D);
                metersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offsetInTileN.y);
            }
            else
                metersPerUnit = Utilities::getMetersPerTileUnit(state.zoomLevel, state.target31, TileSize3D);

            height = static_cast<float>(
                (scaledElevationInMeters / metersPerUnit) * state.elevationConfiguration.zScaleFactor);
        }
    }   
    
    return height;
}

float OsmAnd::AtlasMapRenderer_OpenGL::getHeightOfLocation(const PointI& location31) const
{
    const auto state = getState();

    return getHeightOfLocation(state, location31);
}

float OsmAnd::AtlasMapRenderer_OpenGL::getMapTargetDistance(
    const PointI& location31, bool checkOffScreen /*=false*/) const
{
    bool ok;
    MapRendererState state;
    const auto internalState = getRequiredInternalState(ok, &state);
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

    const auto offsetFromTarget31 = Utilities::shortestVector31(state.target31, location31);
    const auto offsetFromTarget = Utilities::convert31toFloat(offsetFromTarget31, state.zoomLevel);
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

bool OsmAnd::AtlasMapRenderer_OpenGL::getLastProjectablePoint(const MapRendererInternalState& internalState_,
    const glm::vec3& startPoint, const glm::vec3& endPoint, glm::vec3& visiblePoint) const
{
    const auto internalState = static_cast<const InternalState*>(&internalState_);

    glm::vec3 frontIntersection;
    if (Utilities_OpenGL_Common::lineSegmentIntersectPlane(internalState->frontVisibleEdgeN,
        internalState->frontVisibleEdgeP, startPoint, endPoint, frontIntersection))
    {
        visiblePoint = frontIntersection;
        return true;
    }

    return false;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isPointProjectable(
    const MapRendererInternalState& internalState_, const glm::vec3& p) const
{
    const auto internalState = static_cast<const InternalState*>(&internalState_);

    const auto frontN = internalState->frontVisibleEdgeN;
    return frontN.x * p.x + frontN.y * p.y + frontN.z * p.z <= internalState->frontVisibleEdgeD;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isPointVisible(
    const MapRendererInternalState& internalState_, const glm::vec3& p, bool skipTop, bool skipLeft,
        bool skipBottom, bool skipRight, bool skipFront, bool skipBack, float tolerance) const
{
    const auto internalState = static_cast<const InternalState*>(&internalState_);

    return isPointVisible(*internalState, p, skipTop, skipLeft, skipBottom, skipRight, skipFront, skipBack, tolerance);
}

OsmAnd::AreaI OsmAnd::AtlasMapRenderer_OpenGL::getVisibleBBox31() const
{
    bool ok;
    const auto internalState = getRequiredInternalState(ok);
    if (!ok)
        return AreaI::largest();

    AreaI mainArea = internalState.globalFrustum2D31.getBBox31();
    AreaI extraArea = internalState.extraFrustum2D31.getBBox31();
    return extraArea.isEmpty() ? mainArea : mainArea.enlargeToInclude(extraArea);
}

OsmAnd::AreaI OsmAnd::AtlasMapRenderer_OpenGL::getVisibleBBox31(const MapRendererInternalState& internalState_) const
{
    const auto internalState = static_cast<const InternalState*>(&internalState_);

    AreaI mainArea = internalState->globalFrustum2D31.getBBox31();
    AreaI extraArea = internalState->extraFrustum2D31.getBBox31();
    return extraArea.isEmpty() ? mainArea : mainArea.enlargeToInclude(extraArea);
}

OsmAnd::AreaI OsmAnd::AtlasMapRenderer_OpenGL::getVisibleBBoxShifted() const
{
    bool ok;
    const auto internalState = getRequiredInternalState(ok);
    if (!ok)
        return AreaI::largest();

    return internalState.globalFrustum2D31.getBBoxShifted();
}

OsmAnd::AreaI OsmAnd::AtlasMapRenderer_OpenGL::getVisibleBBoxShifted(
    const MapRendererInternalState& internalState_) const
{
    const auto internalState = static_cast<const InternalState*>(&internalState_);
    return internalState->globalFrustum2D31.getBBoxShifted();
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isPositionVisible(const PointI64& position) const
{
    bool ok;
    const auto internalState = getRequiredInternalState(ok);
    if (!ok)
        return false;

    return static_cast<const Frustum2DI64*>(&internalState.globalFrustum2D31)->test(position);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isPositionVisible(const PointI& position31) const
{
    bool ok;
    const auto internalState = getRequiredInternalState(ok);
    if (!ok)
        return false;

    if (internalState.globalFrustum2D31.test(position31))
        return true;
    else
        return internalState.extraFrustum2D31.test(position31);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isPathVisible(const QVector<PointI>& path31) const
{
    bool ok;
    const auto internalState = getRequiredInternalState(ok);
    if (!ok)
        return false;

    if (internalState.globalFrustum2D31.test(path31))
        return true;
    else
        return internalState.extraFrustum2D31.test(path31);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isAreaVisible(const AreaI& area31) const
{
    bool ok;
    const auto internalState = getRequiredInternalState(ok);
    if (!ok)
        return false;

    if (internalState.globalFrustum2D31.test(area31))
        return true;
    else
        return internalState.extraFrustum2D31.test(area31);
}

bool OsmAnd::AtlasMapRenderer_OpenGL::isTileVisible(const int tileX, const int tileY, const int zoom) const
{
    bool ok;
    const auto internalState = getRequiredInternalState(ok);
    if (!ok)
        return false;

    const auto& tilesOfZoom = internalState.uniqueTiles.constFind(static_cast<ZoomLevel>(zoom));
    if (tilesOfZoom != internalState.uniqueTiles.cend())
        return tilesOfZoom->contains(TileId::fromXY(tileX, tileY));

    return false;
}

double OsmAnd::AtlasMapRenderer_OpenGL::getZoomOffset(
    const ZoomLevel zoomLevel, const double visualZoom, const double distanceFactor) const
{
    const auto totalFactor =
        distanceFactor * static_cast<double>(1u << static_cast<int>(zoomLevel)) * visualZoom;
    if (totalFactor < 1.0 || totalFactor > static_cast<double>(1u << static_cast<int>(MaxZoomLevel)))
        return std::numeric_limits<double>::quiet_NaN();
    const auto newZoomLevel = std::floor(std::log2(totalFactor));
    const auto zoomLevelFactor = static_cast<double>(1u << static_cast<int>(newZoomLevel));
    const auto totalZoom = newZoomLevel + std::max(totalFactor / zoomLevelFactor, 1.0) - 1.0;
    const auto currentZoomFloatPart = visualZoom >= 1.0
        ? visualZoom - 1.0
        : (visualZoom - 1.0) * 2.0;
    return totalZoom - static_cast<double>(zoomLevel) - currentZoomFloatPart;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::obtainScreenPointFromPosition(
    const PointI& position31, PointI& outScreenPoint, bool checkOffScreen) const
{
    bool ok;
    MapRendererState state;
    const auto internalState = getRequiredInternalState(ok, &state);
    if (!ok)
        return false;

    if (!checkOffScreen && !internalState.globalFrustum2D31.test(position31))
        return false;

    glm::vec3 positionInWorld;
    if (state.flatEarth)
    {
        const auto offsetFromTarget = Utilities::convert31toFloat(Utilities::shortestLongitudeVector(
            position31 - state.target31), state.zoomLevel) * AtlasMapRenderer::TileSize3D;
        positionInWorld = glm::vec3(offsetFromTarget.x, 0.0f, offsetFromTarget.y);
    }
    else
        positionInWorld = Utilities::sphericalWorldCoordinates(
            position31, internalState.mGlobeRotationPrecise, internalState.globeRadius, 0.0);

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
    bool ok;
    MapRendererState state;
    const auto internalState = getRequiredInternalState(ok, &state);
    if (!ok)
        return false;

    if (!checkOffScreen && !internalState.globalFrustum2D31.test(position31))
        return false;

    const auto height = getHeightOfLocation(state, position31);
    glm::vec3 positionInWorld;
    if (state.flatEarth)
    {
        const auto offsetFromTarget = Utilities::convert31toFloat(Utilities::shortestLongitudeVector(
            position31 - state.target31), state.zoomLevel) * AtlasMapRenderer::TileSize3D;
        positionInWorld = glm::vec3(offsetFromTarget.x, height, offsetFromTarget.y);
    }
    else
        positionInWorld = Utilities::sphericalWorldCoordinates(
            position31, internalState.mGlobeRotationPrecise, internalState.globeRadius, height);


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
    bool ok;
    const auto internalState = getRequiredInternalState(ok);

    return ok ? internalState.distanceFromCameraToGroundInMeters : 0.0f;
}

int OsmAnd::AtlasMapRenderer_OpenGL::getTileZoomOffset() const
{
    return _internalState.zoomLevelOffset;
}

double OsmAnd::AtlasMapRenderer_OpenGL::getTileSizeInMeters() const
{
    const auto state = getState();

    PointF targetInTileOffsetN;
    const auto targetTileId = Utilities::getTileId(state.target31, state.zoomLevel, &targetInTileOffsetN);
    const auto targetY = static_cast<double>(targetTileId.y) + targetInTileOffsetN.y;
    const auto metersPerTile = Utilities::getMetersPerTileUnit(state.zoomLevel, targetY, 1);

    return metersPerTile;
}

double OsmAnd::AtlasMapRenderer_OpenGL::getPixelsToMetersScaleFactor() const
{
    if (const auto& configuration = std::dynamic_pointer_cast<const AtlasMapRendererConfiguration>(getConfiguration()))
    {
        const auto state = getState();
        const auto tileSizeOnScreenInPixels =
            configuration->referenceTileSizeOnScreenInPixels * state.visualZoom * (1.0f + state.visualZoomShift);
        PointF targetInTileOffsetN;
        const auto targetTileId = Utilities::getTileId(state.target31, state.zoomLevel, &targetInTileOffsetN);
        const auto targetY = static_cast<double>(targetTileId.y) + targetInTileOffsetN.y;
        return Utilities::getMetersPerTileUnit(state.zoomLevel, targetY, tileSizeOnScreenInPixels);
    }
    return 0.0;
}

double OsmAnd::AtlasMapRenderer_OpenGL::getPixelsToMetersScaleFactor(
    const MapRendererState& state, const MapRendererInternalState& internalState_) const
{
    const auto internalState = static_cast<const InternalState*>(&internalState_);
    const auto tileSizeOnScreenInPixels =
        internalState->referenceTileSizeOnScreenInPixels * internalState->tileOnScreenScaleFactor;
    const auto targetY = static_cast<double>(internalState->targetTileId.y) + internalState->targetInTileOffsetN.y;
    const auto metersPerPixel = Utilities::getMetersPerTileUnit(state.zoomLevel, targetY, tileSizeOnScreenInPixels);
    return metersPerPixel;
}

double OsmAnd::AtlasMapRenderer_OpenGL::getMaxViewportScale() const
{
    const auto gpuAPI = getGPUAPI();
    const double scale = std::min((double)gpuAPI->_viewportDimensions[0] / (double)_internalState.glmViewport[2],
          (double)gpuAPI->_viewportDimensions[1] / (double)_internalState.glmViewport[3]);
    
    return scale;
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

OsmAnd::AtlasMapRendererMap3DObjectsStage* OsmAnd::AtlasMapRenderer_OpenGL::createMap3DObjectsStage()
{
    return new AtlasMapRendererMap3DObjectsStage_OpenGL(this);
}
