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

#include <SkBitmap.h>

#include "IMapRenderer.h"
#include "IMapTiledDataProvider.h"
#include "IMapRasterBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "MapSymbol.h"
#include "MapObject.h"
#include "QuadTree.h"
#include "Logging.h"
#include "Utilities.h"
#include "OpenGL/Utilities_OpenGL.h"

const float OsmAnd::AtlasMapRenderer_OpenGL::_zNear = 0.1f;

OsmAnd::AtlasMapRenderer_OpenGL::AtlasMapRenderer_OpenGL(GPUAPI_OpenGL* const gpuAPI_)
    : AtlasMapRenderer(gpuAPI_)
    , _skyStage(this)
    , _rasterMapStage(this)
    , _symbolsStage(this)
#if OSMAND_DEBUG
    , _debugStage(this)
#endif
{
}

OsmAnd::AtlasMapRenderer_OpenGL::~AtlasMapRenderer_OpenGL()
{
}

bool OsmAnd::AtlasMapRenderer_OpenGL::doInitializeRendering()
{
    GL_CHECK_PRESENT(glClearColor);
    GL_CHECK_PRESENT(glClearDepthf);

    bool ok;

    ok = AtlasMapRenderer::doInitializeRendering();
    if (!ok)
        return false;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    GL_CHECK_RESULT;

    glClearDepthf(1.0f);
    GL_CHECK_RESULT;

    _skyStage.initialize();
    _rasterMapStage.initialize();
    _symbolsStage.initialize();
#if OSMAND_DEBUG
    _debugStage.initialize();
#endif

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::doRenderFrame()
{
    GL_PUSH_GROUP_MARKER(QLatin1String("OsmAndCore"));

    GL_CHECK_PRESENT(glViewport);
    GL_CHECK_PRESENT(glEnable);
    GL_CHECK_PRESENT(glDisable);
    GL_CHECK_PRESENT(glBlendFunc);
    GL_CHECK_PRESENT(glClear);

#if OSMAND_DEBUG
    _debugStage.clear();
#endif

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

    // Turn on depth testing for sky stage (since with disabled depth test, write to depth buffer is blocked),
    // but since sky is on top of everything, accept all fragments
    glEnable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;
    glDepthFunc(GL_ALWAYS);
    GL_CHECK_RESULT;

    // Render the sky
    _skyStage.render();

    // Change depth test function prior to raster map stage and further stages
    glDepthFunc(GL_LEQUAL);
    GL_CHECK_RESULT;

    // Raster map stage is rendered without blending, since it's done in fragment shader
    _rasterMapStage.render();

    // Turn on blending since now objects with transparency are going to be rendered
    glEnable(GL_BLEND);
    GL_CHECK_RESULT;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_RESULT;

    // Render map symbols without writing depth buffer, since symbols use own sorting and intersection checking
    //NOTE: Currently map symbols are incompatible with height-maps
    glDepthMask(GL_FALSE);
    GL_CHECK_RESULT;
    _symbolsStage.render();
    glDepthMask(GL_TRUE);
    GL_CHECK_RESULT;

    //TODO: render special fog object some day

#if OSMAND_DEBUG
    glDisable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;

    _debugStage.render();

    glEnable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;
#endif

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::doReleaseRendering()
{
    bool ok;

#if OSMAND_DEBUG
    _debugStage.release();
#endif
    _symbolsStage.release();
    _rasterMapStage.release();
    _skyStage.release();

    ok = AtlasMapRenderer::doReleaseRendering();
    if (!ok)
        return false;

    return true;
}

void OsmAnd::AtlasMapRenderer_OpenGL::onValidateResourcesOfType(const MapRendererResourcesManager::MapRendererResourceType type)
{
    AtlasMapRenderer::onValidateResourcesOfType(type);

    if (type == MapRendererResourcesManager::MapRendererResourceType::ElevationData)
    {
        // Recreate tile patch since elevation data influences density of tile patch
        _rasterMapStage.recreateTile();
    }
}

bool OsmAnd::AtlasMapRenderer_OpenGL::updateInternalState(MapRenderer::InternalState* internalState_, const MapRendererState& state)
{
    bool ok;
    ok = AtlasMapRenderer::updateInternalState(internalState_, state);
    if (!ok)
        return false;

    const auto internalState = static_cast<InternalState*>(internalState_);

    // Prepare values for projection matrix
    const auto viewportWidth = state.viewport.width();
    const auto viewportHeight = state.viewport.height();
    if (viewportWidth == 0 || viewportHeight == 0)
        return false;
    internalState->aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    internalState->fovInRadians = qDegreesToRadians(state.fieldOfView);
    internalState->projectionPlaneHalfHeight = _zNear * internalState->fovInRadians;
    internalState->projectionPlaneHalfWidth = internalState->projectionPlaneHalfHeight * internalState->aspectRatio;

    // Setup perspective projection with fake Z-far plane
    internalState->mPerspectiveProjection = glm::frustum(
        -internalState->projectionPlaneHalfWidth, internalState->projectionPlaneHalfWidth,
        -internalState->projectionPlaneHalfHeight, internalState->projectionPlaneHalfHeight,
        _zNear, 1000.0f);

    // Calculate limits of camera distance to target and actual distance
    internalState->screenTileSize = getReferenceTileSizeOnScreen(state);
    internalState->nearDistanceFromCameraToTarget = Utilities_OpenGL_Common::calculateCameraDistance(internalState->mPerspectiveProjection, state.viewport, TileSize3D / 2.0f, internalState->screenTileSize / 2.0f, 1.5f);
    internalState->baseDistanceFromCameraToTarget = Utilities_OpenGL_Common::calculateCameraDistance(internalState->mPerspectiveProjection, state.viewport, TileSize3D / 2.0f, internalState->screenTileSize / 2.0f, 1.0f);
    internalState->farDistanceFromCameraToTarget = Utilities_OpenGL_Common::calculateCameraDistance(internalState->mPerspectiveProjection, state.viewport, TileSize3D / 2.0f, internalState->screenTileSize / 2.0f, 0.75f);

    // zoomFraction == [ 0.0 ... 0.5] scales tile [1.0x ... 1.5x]
    // zoomFraction == [-0.5 ...-0.0] scales tile [.75x ... 1.0x]
    if (state.zoomFraction >= 0.0f)
        internalState->distanceFromCameraToTarget = internalState->baseDistanceFromCameraToTarget - (internalState->baseDistanceFromCameraToTarget - internalState->nearDistanceFromCameraToTarget) * (2.0f * state.zoomFraction);
    else
        internalState->distanceFromCameraToTarget = internalState->baseDistanceFromCameraToTarget - (internalState->farDistanceFromCameraToTarget - internalState->baseDistanceFromCameraToTarget) * (2.0f * state.zoomFraction);
    internalState->groundDistanceFromCameraToTarget = internalState->distanceFromCameraToTarget * qCos(qDegreesToRadians(state.elevationAngle));
    internalState->tileScaleFactor = ((state.zoomFraction >= 0.0f) ? (1.0f + state.zoomFraction) : (1.0f + 0.5f * state.zoomFraction));
    internalState->scaleToRetainProjectedSize = internalState->distanceFromCameraToTarget / internalState->baseDistanceFromCameraToTarget;

    // Recalculate perspective projection with obtained value
    internalState->zSkyplane = state.fogDistance * internalState->scaleToRetainProjectedSize + internalState->distanceFromCameraToTarget;
    internalState->zFar = glm::length(glm::vec3(
        internalState->projectionPlaneHalfWidth * (internalState->zSkyplane / _zNear),
        internalState->projectionPlaneHalfHeight * (internalState->zSkyplane / _zNear),
        internalState->zSkyplane));
    internalState->mPerspectiveProjection = glm::frustum(
        -internalState->projectionPlaneHalfWidth, internalState->projectionPlaneHalfWidth,
        -internalState->projectionPlaneHalfHeight, internalState->projectionPlaneHalfHeight,
        _zNear, internalState->zFar);
    internalState->mPerspectiveProjectionInv = glm::inverse(internalState->mPerspectiveProjection);

    // Calculate orthographic projection
    const auto viewportBottom = state.windowSize.y - state.viewport.bottom;
    internalState->mOrthographicProjection = glm::ortho(
        static_cast<float>(state.viewport.left), static_cast<float>(state.viewport.right),
        static_cast<float>(viewportBottom) /*bottom*/, static_cast<float>(viewportBottom + viewportHeight) /*top*/,
        _zNear, internalState->zFar);

    // Setup camera
    internalState->mDistance = glm::translate(glm::vec3(0.0f, 0.0f, -internalState->distanceFromCameraToTarget));
    internalState->mElevation = glm::rotate(state.elevationAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    internalState->mAzimuth = glm::rotate(state.azimuth, glm::vec3(0.0f, 1.0f, 0.0f));
    internalState->mCameraView = internalState->mDistance * internalState->mElevation * internalState->mAzimuth;

    // Get inverse camera
    internalState->mDistanceInv = glm::translate(glm::vec3(0.0f, 0.0f, internalState->distanceFromCameraToTarget));
    internalState->mElevationInv = glm::rotate(-state.elevationAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    internalState->mAzimuthInv = glm::rotate(-state.azimuth, glm::vec3(0.0f, 1.0f, 0.0f));
    internalState->mCameraViewInv = internalState->mAzimuthInv * internalState->mElevationInv * internalState->mDistanceInv;

    // Get camera positions
    internalState->groundCameraPosition = (internalState->mAzimuthInv * glm::vec4(0.0f, 0.0f, internalState->distanceFromCameraToTarget, 1.0f)).xz;
    internalState->worldCameraPosition = (_internalState.mCameraViewInv * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;

    // Correct fog distance
    internalState->correctedFogDistance = state.fogDistance * internalState->scaleToRetainProjectedSize + (internalState->distanceFromCameraToTarget - internalState->groundDistanceFromCameraToTarget);

    // Calculate skyplane size
    float zSkyplaneK = internalState->zSkyplane / _zNear;
    internalState->skyplaneSize.x = zSkyplaneK * internalState->projectionPlaneHalfWidth * 2.0f;
    internalState->skyplaneSize.y = zSkyplaneK * internalState->projectionPlaneHalfHeight * 2.0f;

    // Update frustum
    updateFrustum(internalState, state);

    // Compute visible tileset
    computeVisibleTileset(internalState, state);

    return true;
}

const OsmAnd::MapRenderer::InternalState* OsmAnd::AtlasMapRenderer_OpenGL::getInternalStateRef() const
{
    return &_internalState;
}

OsmAnd::MapRenderer::InternalState* OsmAnd::AtlasMapRenderer_OpenGL::getInternalStateRef()
{
    return &_internalState;
}

void OsmAnd::AtlasMapRenderer_OpenGL::updateFrustum(InternalState* internalState, const MapRendererState& state)
{
    // 4 points of frustum near clipping box in camera coordinate space
    const glm::vec4 nTL_c(-internalState->projectionPlaneHalfWidth, +internalState->projectionPlaneHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nTR_c(+internalState->projectionPlaneHalfWidth, +internalState->projectionPlaneHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nBL_c(-internalState->projectionPlaneHalfWidth, -internalState->projectionPlaneHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nBR_c(+internalState->projectionPlaneHalfWidth, -internalState->projectionPlaneHalfHeight, -_zNear, 1.0f);

    // 4 points of frustum far clipping box in camera coordinate space
    const auto zFar = internalState->zSkyplane;
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

    if (intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nBL_g.xyz, fBL_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz;
        intersectionPointsCounter++;
    }
    if (intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nBR_g.xyz, fBR_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz;
        intersectionPointsCounter++;
    }
    if (intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTR_g.xyz, fTR_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz;
        intersectionPointsCounter++;
    }
    if (intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTL_g.xyz, fTL_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz;
        intersectionPointsCounter++;
    }
    if (intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, fTR_g.xyz, fBR_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz;
        intersectionPointsCounter++;
    }
    if (intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, fTL_g.xyz, fBL_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz;
        intersectionPointsCounter++;
    }
    if (intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTL_g.xyz, nBL_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz;
        intersectionPointsCounter++;
    }
    if (intersectionPointsCounter < 4 && Utilities_OpenGL_Common::lineSegmentIntersectPlane(planeN, planeO, nTR_g.xyz, nBR_g.xyz, intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = intersectionPoint.xz;
        intersectionPointsCounter++;
    }
    assert(intersectionPointsCounter == 4);

    internalState->frustum2D.p0 = PointF(intersectionPoints[0].x, intersectionPoints[0].y);
    internalState->frustum2D.p1 = PointF(intersectionPoints[1].x, intersectionPoints[1].y);
    internalState->frustum2D.p2 = PointF(intersectionPoints[2].x, intersectionPoints[2].y);
    internalState->frustum2D.p3 = PointF(intersectionPoints[3].x, intersectionPoints[3].y);

    const auto tileSize31 = (1u << (ZoomLevel::MaxZoomLevel - state.zoomBase));
    internalState->frustum2D31.p0 = (internalState->frustum2D.p0 / TileSize3D) * static_cast<double>(tileSize31);
    internalState->frustum2D31.p1 = (internalState->frustum2D.p1 / TileSize3D) * static_cast<double>(tileSize31);
    internalState->frustum2D31.p2 = (internalState->frustum2D.p2 / TileSize3D) * static_cast<double>(tileSize31);
    internalState->frustum2D31.p3 = (internalState->frustum2D.p3 / TileSize3D) * static_cast<double>(tileSize31);
}

void OsmAnd::AtlasMapRenderer_OpenGL::computeVisibleTileset(InternalState* internalState, const MapRendererState& state)
{
    // Normalize 2D-frustum points to tiles
    PointF p[4];
    p[0] = internalState->frustum2D.p0 / TileSize3D;
    p[1] = internalState->frustum2D.p1 / TileSize3D;
    p[2] = internalState->frustum2D.p2 / TileSize3D;
    p[3] = internalState->frustum2D.p3 / TileSize3D;

    // "Round"-up tile indices
    // In-tile normalized position is added, since all tiles are going to be
    // translated in opposite direction during rendering
    p[0] += internalState->targetInTileOffsetN;
    p[1] += internalState->targetInTileOffsetN;
    p[2] += internalState->targetInTileOffsetN;
    p[3] += internalState->targetInTileOffsetN;

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
                tileId.x = x + internalState->targetTileId.x;
                tileId.y = y + internalState->targetTileId.y;

                visibleTiles.insert(tileId);
            }
        }

        internalState->visibleTiles = visibleTiles.toList();
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

bool OsmAnd::AtlasMapRenderer_OpenGL::postInitializeRendering()
{
    bool ok;

    ok = AtlasMapRenderer::postInitializeRendering();
    if (!ok)
        return false;

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::preReleaseRendering()
{
    bool ok;

    ok = AtlasMapRenderer::preReleaseRendering();
    if (!ok)
        return false;

    return true;
}

OsmAnd::GPUAPI_OpenGL* OsmAnd::AtlasMapRenderer_OpenGL::getGPUAPI() const
{
    return static_cast<OsmAnd::GPUAPI_OpenGL*>(gpuAPI.get());
}

float OsmAnd::AtlasMapRenderer_OpenGL::getReferenceTileSizeOnScreen( const MapRendererState& state )
{
    const auto& rasterMapProvider = state.rasterLayerProviders[static_cast<int>(RasterMapLayerId::BaseLayer)];
    if (!rasterMapProvider)
        return static_cast<float>(DefaultReferenceTileSizeOnScreen) * setupOptions.displayDensityFactor;

    auto tileProvider = std::static_pointer_cast<IMapRasterBitmapTileProvider>(rasterMapProvider);
    return tileProvider->getTileSize() * (setupOptions.displayDensityFactor / tileProvider->getTileDensityFactor());
}

float OsmAnd::AtlasMapRenderer_OpenGL::getReferenceTileSizeOnScreen()
{
    return getReferenceTileSizeOnScreen(state);
}

float OsmAnd::AtlasMapRenderer_OpenGL::getScaledTileSizeOnScreen()
{
    InternalState internalState;
    bool ok = updateInternalState(&internalState, state);

    return getReferenceTileSizeOnScreen(state) * internalState.tileScaleFactor;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getLocationFromScreenPoint( const PointI& screenPoint, PointI& location31 )
{
    PointI64 location;
    if (!getLocationFromScreenPoint(screenPoint, location))
        return false;
    location31 = Utilities::normalizeCoordinates(location, ZoomLevel31);

    return true;
}

bool OsmAnd::AtlasMapRenderer_OpenGL::getLocationFromScreenPoint( const PointI& screenPoint, PointI64& location )
{
    InternalState internalState;
    bool ok = updateInternalState(&internalState, state);
    if (!ok)
        return false;

    glm::vec4 viewport(
        state.viewport.left,
        state.windowSize.y - state.viewport.bottom,
        state.viewport.width(),
        state.viewport.height());
    const auto nearInWorld = glm::unProject(glm::vec3(screenPoint.x, state.windowSize.y - screenPoint.y, 0.0f), internalState.mCameraView, internalState.mPerspectiveProjection, viewport);
    const auto farInWorld = glm::unProject(glm::vec3(screenPoint.x, state.windowSize.y - screenPoint.y, 1.0f), internalState.mCameraView, internalState.mPerspectiveProjection, viewport);
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

    const auto zoomDiff = ZoomLevel::MaxZoomLevel - state.zoomBase;
    const auto tileSize31 = (1u << zoomDiff);
    x *= tileSize31;
    y *= tileSize31;

    location.x = static_cast<int64_t>(x) + (internalState.targetTileId.x << zoomDiff);
    location.y = static_cast<int64_t>(y) + (internalState.targetTileId.y << zoomDiff);
    
    return true;
}
