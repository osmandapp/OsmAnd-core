#include "AtlasMapRenderer_OpenGL_Base.h"

#include <assert.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <QtGlobal>
#include <QtCore/qmath.h>
#include <QThread>

#include "IMapRenderer.h"
#include "IMapTileProvider.h"
#include "IMapBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "OsmAndLogging.h"
#include "OsmAndUtilities.h"
#include "OpenGL_Base/Utilities_OpenGL_Base.h"

OsmAnd::AtlasMapRenderer_BaseOpenGL::AtlasMapRenderer_BaseOpenGL()
    : _zNear(0.1f)
{
}

OsmAnd::AtlasMapRenderer_BaseOpenGL::~AtlasMapRenderer_BaseOpenGL()
{
}

void OsmAnd::AtlasMapRenderer_BaseOpenGL::validateTileLayerCache( const TileLayerId& layer )
{
    IMapRenderer::validateTileLayerCache(layer);

    if(layer == ElevationData)
    {
        // Recreate tile patch since elevation data influences density of tile patch
        releaseTilePatch();
        createTilePatch();
    }
}

void OsmAnd::AtlasMapRenderer_BaseOpenGL::updateConfiguration()
{
    BaseAtlasMapRenderer::updateConfiguration();

    computeProjectionAndViewMatrices();
    computeVisibleTileset();
    computeSkyplaneSize();
}

void OsmAnd::AtlasMapRenderer_BaseOpenGL::computeProjectionAndViewMatrices()
{
    // Prepare values for projection matrix
    _aspectRatio = static_cast<float>(_activeConfig.viewport.width());
    auto viewportHeight = _activeConfig.viewport.height();
    if(viewportHeight > 0)
        _aspectRatio /= static_cast<float>(viewportHeight);
    _fovInRadians = qDegreesToRadians(_activeConfig.fieldOfView);
    _projectionPlaneHalfHeight = _zNear * _fovInRadians;
    _projectionPlaneHalfWidth = _projectionPlaneHalfHeight * _aspectRatio;
    
    // Setup projection with fake Z-far plane
    _mProjection = glm::frustum(-_projectionPlaneHalfWidth, _projectionPlaneHalfWidth, -_projectionPlaneHalfHeight, _projectionPlaneHalfHeight, _zNear, 1000.0f);

    // Calculate limits of camera distance to target and actual distance
    const auto& rasterMapProvider = _activeConfig.tileProviders[RasterMap];
    auto tileProvider = static_cast<IMapBitmapTileProvider*>(rasterMapProvider.get());
    const float screenTile = tileProvider->getTileSize() * (_activeConfig.displayDensityFactor / tileProvider->getTileDensity());
    _nearDistanceFromCameraToTarget = Utilities_BaseOpenGL::calculateCameraDistance(_mProjection, _activeConfig.viewport, TileSide3D / 2.0f, screenTile / 2.0f, 1.5f);
    _baseDistanceFromCameraToTarget = Utilities_BaseOpenGL::calculateCameraDistance(_mProjection, _activeConfig.viewport, TileSide3D / 2.0f, screenTile / 2.0f, 1.0f);
    _farDistanceFromCameraToTarget = Utilities_BaseOpenGL::calculateCameraDistance(_mProjection, _activeConfig.viewport, TileSide3D / 2.0f, screenTile / 2.0f, 0.75f);

    // zoomFraction == [ 0.0 ... 0.5] scales tile [1.0x ... 1.5x]
    // zoomFraction == [-0.5 ...-0.0] scales tile [.75x ... 1.0x]
    if(_activeConfig.zoomFraction >= 0.0f)
        _distanceFromCameraToTarget = _baseDistanceFromCameraToTarget - (_baseDistanceFromCameraToTarget - _nearDistanceFromCameraToTarget) * (2.0f * _activeConfig.zoomFraction);
    else
        _distanceFromCameraToTarget = _baseDistanceFromCameraToTarget - (_farDistanceFromCameraToTarget - _baseDistanceFromCameraToTarget) * (2.0f * _activeConfig.zoomFraction);
    _tileScaleFactor = ((_activeConfig.zoomFraction >= 0.0f) ? (1.0f + _activeConfig.zoomFraction) : (1.0f + 0.5f * _activeConfig.zoomFraction));
    _scaleToRetainProjectedSize = _distanceFromCameraToTarget / _baseDistanceFromCameraToTarget;
    
    // Recalculate projection with obtained value
    _zSkyplane = _activeConfig.fogDistance + _distanceFromCameraToTarget;
    _zFar = glm::length(glm::vec3(_projectionPlaneHalfWidth * (_zSkyplane / _zNear), _projectionPlaneHalfHeight * (_zSkyplane / _zNear), _zSkyplane));
    _mProjection = glm::frustum(-_projectionPlaneHalfWidth, _projectionPlaneHalfWidth, -_projectionPlaneHalfHeight, _projectionPlaneHalfHeight, _zNear, _zFar);

    // Setup camera
    _mDistance = glm::translate(0.0f, 0.0f, -_distanceFromCameraToTarget);
    _mElevation = glm::rotate(_activeConfig.elevationAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    _mAzimuth = glm::rotate(_activeConfig.azimuth, glm::vec3(0.0f, 1.0f, 0.0f));
    _mView = _mDistance * _mElevation * _mAzimuth;

    // Get inverse camera
    _mDistanceInv = glm::translate(0.0f, 0.0f, _distanceFromCameraToTarget);
    _mElevationInv = glm::rotate(-_activeConfig.elevationAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    _mAzimuthInv = glm::rotate(-_activeConfig.azimuth, glm::vec3(0.0f, 1.0f, 0.0f));
    _mViewInv = _mAzimuthInv * _mElevationInv * _mDistanceInv;
}

void OsmAnd::AtlasMapRenderer_BaseOpenGL::computeVisibleTileset()
{
    // Get target tile
    _targetTile.x = _activeConfig.target31.x >> (31 - _activeConfig.zoomBase);
    _targetTile.y = _activeConfig.target31.y >> (31 - _activeConfig.zoomBase);

    // Compute in-tile offset
    if(_activeConfig.zoomBase == 31)
    {
        _targetInTilePosN.x = 0;
        _targetInTilePosN.y = 0;
    }
    else
    {
        PointI tileOrigin31;
        tileOrigin31.x = _targetTile.x << (31 - _activeConfig.zoomBase);
        tileOrigin31.y = _targetTile.y << (31 - _activeConfig.zoomBase);
        auto tileWidth31 = (1u << (31 - _activeConfig.zoomBase)) - 1;
        _targetInTilePosN.x = static_cast<double>(_activeConfig.target31.x - tileOrigin31.x) / tileWidth31;
        _targetInTilePosN.y = static_cast<double>(_activeConfig.target31.y - tileOrigin31.y) / tileWidth31;
    }
    
    // 4 points of frustum near clipping box in camera coordinate space
    const glm::vec4 nTL_c(-_projectionPlaneHalfWidth, +_projectionPlaneHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nTR_c(+_projectionPlaneHalfWidth, +_projectionPlaneHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nBL_c(-_projectionPlaneHalfWidth, -_projectionPlaneHalfHeight, -_zNear, 1.0f);
    const glm::vec4 nBR_c(+_projectionPlaneHalfWidth, -_projectionPlaneHalfHeight, -_zNear, 1.0f);

    // 4 points of frustum far clipping box in camera coordinate space
    const auto zFar = _zSkyplane + 0.5f * TileSide3D;
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
    if(intersectionPointsCounter < 4 && Utilities_BaseOpenGL::lineSegmentIntersectPlane(planeN, planeO, glm::vec3(nBL_g.x, nBL_g.y, nBL_g.z), glm::vec3(fBL_g.x, fBL_g.y, fBL_g.z), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = glm::vec2(intersectionPoint.x, intersectionPoint.z);
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_BaseOpenGL::lineSegmentIntersectPlane(planeN, planeO, glm::vec3(nBR_g.x, nBR_g.y, nBR_g.z), glm::vec3(fBR_g.x, fBR_g.y, fBR_g.z), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = glm::vec2(intersectionPoint.x, intersectionPoint.z);
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_BaseOpenGL::lineSegmentIntersectPlane(planeN, planeO, glm::vec3(nTR_g.x, nTR_g.y, nTR_g.z), glm::vec3(fTR_g.x, fTR_g.y, fTR_g.z), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = glm::vec2(intersectionPoint.x, intersectionPoint.z);
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_BaseOpenGL::lineSegmentIntersectPlane(planeN, planeO, glm::vec3(nTL_g.x, nTL_g.y, nTL_g.z), glm::vec3(fTL_g.x, fTL_g.y, fTL_g.z), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = glm::vec2(intersectionPoint.x, intersectionPoint.z);
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_BaseOpenGL::lineSegmentIntersectPlane(planeN, planeO, glm::vec3(fTR_g.x, fTR_g.y, fTR_g.z), glm::vec3(fBR_g.x, fBR_g.y, fBR_g.z), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = glm::vec2(intersectionPoint.x, intersectionPoint.z);
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_BaseOpenGL::lineSegmentIntersectPlane(planeN, planeO, glm::vec3(fTL_g.x, fTL_g.y, fTL_g.z), glm::vec3(fBL_g.x, fBL_g.y, fBL_g.z), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = glm::vec2(intersectionPoint.x, intersectionPoint.z);
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_BaseOpenGL::lineSegmentIntersectPlane(planeN, planeO, glm::vec3(nTR_g.x, nTR_g.y, nTR_g.z), glm::vec3(nBR_g.x, nBR_g.y, nBR_g.z), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = glm::vec2(intersectionPoint.x, intersectionPoint.z);
        intersectionPointsCounter++;
    }
    if(intersectionPointsCounter < 4 && Utilities_BaseOpenGL::lineSegmentIntersectPlane(planeN, planeO, glm::vec3(nTL_g.x, nTL_g.y, nTL_g.z), glm::vec3(nBL_g.x, nBL_g.y, nBL_g.z), intersectionPoint))
    {
        intersectionPoints[intersectionPointsCounter] = glm::vec2(intersectionPoint.x, intersectionPoint.z);
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
        PointF(ip[0].x + _targetInTilePosN.x, ip[0].y + _targetInTilePosN.y),
        PointF(ip[1].x + _targetInTilePosN.x, ip[1].y + _targetInTilePosN.y),
        PointF(ip[2].x + _targetInTilePosN.x, ip[2].y + _targetInTilePosN.y),
        PointF(ip[3].x + _targetInTilePosN.x, ip[3].y + _targetInTilePosN.y),
    };

    //NOTE: so far scanline does not work exactly as expected, so temporary switch to old implementation
    {
        _visibleTiles.clear();
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
                tileId.x = x + _targetTile.x;
                tileId.y = y + _targetTile.y;

                _visibleTiles.insert(tileId);
            }
        }
    }
    /*
    // Find visible tiles using scanline fill
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

void OsmAnd::AtlasMapRenderer_BaseOpenGL::computeSkyplaneSize()
{
    float zSkyplaneK = _zSkyplane / _zNear;
    _skyplaneHalfSize.x = zSkyplaneK * _projectionPlaneHalfWidth;
    _skyplaneHalfSize.y = zSkyplaneK * _projectionPlaneHalfHeight;
}

void OsmAnd::AtlasMapRenderer_BaseOpenGL::initializeRendering()
{
    createTilePatch();
}

void OsmAnd::AtlasMapRenderer_BaseOpenGL::releaseRendering()
{
    releaseTilePatch();
}

void OsmAnd::AtlasMapRenderer_BaseOpenGL::createTilePatch()
{
    MapTileVertex* pVertices = nullptr;
    uint32_t verticesCount = 0;
    GLushort* pIndices = nullptr;
    uint32_t indicesCount = 0;
    
    if(!_activeConfig.tileProviders[ElevationData])
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
        // Complex tile patch, consisting of (TileElevationNodesPerSide*TileElevationNodesPerSide) number of
        // height clusters.
        const GLfloat clusterSize = static_cast<GLfloat>(TileSide3D) / static_cast<float>(_activeConfig.heightmapPatchesPerSide);
        const auto verticesPerLine = _activeConfig.heightmapPatchesPerSide + 1;
        const auto outerVertices = verticesPerLine * verticesPerLine;
        verticesCount = outerVertices;
        verticesCount += _activeConfig.heightmapPatchesPerSide * _activeConfig.heightmapPatchesPerSide; // Inner
        pVertices = new MapTileVertex[verticesCount];
        indicesCount = (_activeConfig.heightmapPatchesPerSide * _activeConfig.heightmapPatchesPerSide) * 4 * 3;
        pIndices = new GLushort[indicesCount];

        MapTileVertex* pV = pVertices;

        // Form outer vertices
        for(auto row = 0u; row < verticesPerLine; row++)
        {
            for(auto col = 0u; col < verticesPerLine; col++, pV++)
            {
                pV->position[0] = static_cast<float>(col) * clusterSize;
                pV->position[1] = static_cast<float>(row) * clusterSize;

                pV->uv[0] = static_cast<float>(col) / static_cast<float>(_activeConfig.heightmapPatchesPerSide);
                pV->uv[1] = static_cast<float>(row) / static_cast<float>(_activeConfig.heightmapPatchesPerSide);
            }
        }

        // Form inner vertices
        for(auto row = 0u; row < _activeConfig.heightmapPatchesPerSide; row++)
        {
            for(auto col = 0u; col < _activeConfig.heightmapPatchesPerSide; col++, pV++)
            {
                pV->position[0] = (static_cast<float>(col) + 0.5f) * clusterSize;
                pV->position[1] = (static_cast<float>(row) + 0.5f) * clusterSize;

                pV->uv[0] = (static_cast<float>(col) + 0.5f) / static_cast<float>(_activeConfig.heightmapPatchesPerSide);
                pV->uv[1] = (static_cast<float>(row) + 0.5f) / static_cast<float>(_activeConfig.heightmapPatchesPerSide);
            }
        }

        // Form indices
        GLushort* pI = pIndices;
        for(auto row = 0u; row < _activeConfig.heightmapPatchesPerSide; row++)
        {
            for(auto col = 0u; col < _activeConfig.heightmapPatchesPerSide; col++, pV++)
            {
                // p0 - center point
                // p1 - top left
                // p2 - bottom left
                // p3 - bottom right
                // p4 - top right
                const auto p0 = outerVertices + (row * _activeConfig.heightmapPatchesPerSide + col);
                const auto p1 = (row + 0) * verticesPerLine + col + 0;
                const auto p2 = (row + 1) * verticesPerLine + col + 0;
                const auto p3 = (row + 1) * verticesPerLine + col + 1;
                const auto p4 = (row + 0) * verticesPerLine + col + 1;

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

                // Triangle 2
                pI[0] = p0;
                pI[1] = p3;
                pI[2] = p4;
                pI += 3;

                // Triangle 3
                pI[0] = p0;
                pI[1] = p4;
                pI[2] = p1;
                pI += 3;
            }
        }
    }
    
    allocateTilePatch(pVertices, verticesCount, pIndices, indicesCount);

    if(_activeConfig.tileProviders[ElevationData])
    {
        delete[] pVertices;
        delete[] pIndices;
    }
}
