#include "AtlasMapRenderer_OpenGL_Base.h"

#include <assert.h>

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
#include "OpenGL_Base/Utilities_OpenGL_Base.h"

OsmAnd::AtlasMapRenderer_BaseOpenGL::AtlasMapRenderer_BaseOpenGL()
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
    computeFog();
    computeVisibleTileset();
    compuleSkyplaneSize();
}

void OsmAnd::AtlasMapRenderer_BaseOpenGL::computeProjectionAndViewMatrices()
{
    // Setup projection with fake Z-far plane
    GLfloat aspectRatio = static_cast<GLfloat>(_activeConfig.viewport.width());
    auto viewportHeight = _activeConfig.viewport.height();
    if(viewportHeight > 0)
        aspectRatio /= static_cast<GLfloat>(viewportHeight);
    _mProjection = glm::perspective(_activeConfig.fieldOfView, aspectRatio, 0.1f, 1000.0f);

    // Calculate limits of camera distance to target and actual distance
    const auto& rasterMapProvider = _activeConfig.tileProviders[RasterMap];
    auto tileProvider = static_cast<IMapBitmapTileProvider*>(rasterMapProvider.get());
    const float screenTile = tileProvider->getTileSize() * (_activeConfig.displayDensityFactor / tileProvider->getTileDensity());
    const float nearD = Utilities_BaseOpenGL::calculateCameraDistance(_mProjection, _activeConfig.viewport, TileSide3D / 2.0f, screenTile / 2.0f, 1.5f);
    const float baseD = Utilities_BaseOpenGL::calculateCameraDistance(_mProjection, _activeConfig.viewport, TileSide3D / 2.0f, screenTile / 2.0f, 1.0f);
    const float farD = Utilities_BaseOpenGL::calculateCameraDistance(_mProjection, _activeConfig.viewport, TileSide3D / 2.0f, screenTile / 2.0f, 0.75f);

    // zoomFraction == [ 0.0 ... 0.5] scales tile [1.0x ... 1.5x]
    // zoomFraction == [-0.5 ...-0.0] scales tile [.75x ... 1.0x]
    if(_activeConfig.zoomFraction >= 0.0f)
        _distanceFromCameraToTarget = baseD - (baseD - nearD) * (2.0f * _activeConfig.zoomFraction);
    else
        _distanceFromCameraToTarget = baseD - (farD - baseD) * (2.0f * _activeConfig.zoomFraction);
    _tileScaleFactor = ((_activeConfig.zoomFraction >= 0.0f) ? (1.0f + _activeConfig.zoomFraction) : (1.0f + 0.5f * _activeConfig.zoomFraction));
    
    // Recalculate projection with obtained value
    _mProjection = glm::perspective(_activeConfig.fieldOfView, aspectRatio, 0.1f, _activeConfig.fogDistance * 1.5f + _distanceFromCameraToTarget);

    // Setup camera
    const auto& c0 = glm::translate(0.0f, 0.0f, -_distanceFromCameraToTarget);
    const auto& c1 = glm::rotate(c0, _activeConfig.elevationAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    _mView = glm::rotate(c1, _activeConfig.azimuth, glm::vec3(0.0f, 1.0f, 0.0f));
}

void OsmAnd::AtlasMapRenderer_BaseOpenGL::computeVisibleTileset()
{
    glm::vec4 glViewport(
        _activeConfig.viewport.left,
        _activeConfig.windowSize.y - _activeConfig.viewport.bottom,
        _activeConfig.viewport.width(),
        _activeConfig.viewport.height());

    // Top-left far
    const auto& tlf = glm::unProject(
        glm::vec3(_activeConfig.viewport.left, _activeConfig.windowSize.y - _activeConfig.viewport.bottom + _activeConfig.viewport.height(), 1.0),
        _mView, _mProjection, glViewport);

    // Top-right far
    const auto& trf = glm::unProject(
        glm::vec3(_activeConfig.viewport.right, _activeConfig.windowSize.y - _activeConfig.viewport.bottom + _activeConfig.viewport.height(), 1.0),
        _mView, _mProjection, glViewport);

    // Bottom-left far
    const auto& blf = glm::unProject(
        glm::vec3(_activeConfig.viewport.left, _activeConfig.windowSize.y - _activeConfig.viewport.bottom, 1.0),
        _mView, _mProjection, glViewport);

    // Bottom-right far
    const auto& brf = glm::unProject(
        glm::vec3(_activeConfig.viewport.right, _activeConfig.windowSize.y - _activeConfig.viewport.bottom, 1.0),
        _mView, _mProjection, glViewport);

    // Top-left near
    const auto& tln = glm::unProject(
        glm::vec3(_activeConfig.viewport.left, _activeConfig.windowSize.y - _activeConfig.viewport.bottom + _activeConfig.viewport.height(), 0.0),
        _mView, _mProjection, glViewport);

    // Top-right near
    const auto& trn = glm::unProject(
        glm::vec3(_activeConfig.viewport.right, _activeConfig.windowSize.y - _activeConfig.viewport.bottom + _activeConfig.viewport.height(), 0.0),
        _mView, _mProjection, glViewport);

    // Bottom-left near
    const auto& bln = glm::unProject(
        glm::vec3(_activeConfig.viewport.left, _activeConfig.windowSize.y - _activeConfig.viewport.bottom, 0.0),
        _mView, _mProjection, glViewport);

    // Bottom-right near
    const auto& brn = glm::unProject(
        glm::vec3(_activeConfig.viewport.right, _activeConfig.windowSize.y - _activeConfig.viewport.bottom, 0.0),
        _mView, _mProjection, glViewport);

    // Obtain 4 normalized ray directions for each side of viewport
    const auto& tlRayD = glm::normalize(tlf - tln);
    const auto& trRayD = glm::normalize(trf - trn);
    const auto& blRayD = glm::normalize(blf - bln);
    const auto& brRayD = glm::normalize(brf - brn);

    // Our plane normal is always Y-up and plane origin is 0
    glm::vec3 planeN(0.0f, 1.0f, 0.0f);

    // Intersect 4 rays with tile-plane
    auto clip = _activeConfig.fogDistance  + _distanceFromCameraToTarget;
    float tlD, trD, blD, brD;
    bool intersects;
    intersects = Utilities_BaseOpenGL::rayIntersectPlane(planeN, 0.0f, tlRayD, tln, tlD);
    if(!intersects)
        tlD = clip;
    auto tlP = tln + tlRayD * tlD;
    intersects = Utilities_BaseOpenGL::rayIntersectPlane(planeN, 0.0f, trRayD, trn, trD);
    if(!intersects)
        trD = clip;
    auto trP = trn + trRayD * trD;
    intersects = Utilities_BaseOpenGL::rayIntersectPlane(planeN, 0.0f, blRayD, bln, blD);
    if(!intersects)
        blD = clip;
    auto blP = bln + blRayD * blD;
    intersects = Utilities_BaseOpenGL::rayIntersectPlane(planeN, 0.0f, brRayD, brn, brD);
    if(!intersects)
        brD = clip;
    auto brP = brn + brRayD * brD;

    // Limit all I-points using clip distance
    auto tlLength = glm::length(tlP);
    if(tlLength > clip)
        tlP /= tlLength / clip;

    auto trLength = glm::length(trP);
    if(trLength > clip)
        trP /= trLength / clip;

    auto blLength = glm::length(blP);
    if(blLength > clip)
        blP /= blLength / clip;

    auto brLength = glm::length(brP);
    if(brLength > clip)
        brP /= brLength / clip;

    // Get tile indices
    tlP /= TileSide3D;
    trP /= TileSide3D;
    blP /= TileSide3D;
    brP /= TileSide3D;

    // Obtain visible tile indices in current zoom
    //TODO: it's not optimal, since dumb AABB takes a lot of unneeded tiles, up to 50% of selected
    _visibleTiles.clear();
    PointI p0, p1, p2, p3;
    p0.x = tlP[0] > 0.0f ? qCeil(tlP[0]) : qFloor(tlP[0]);
    p0.y = tlP[2] > 0.0f ? qCeil(tlP[2]) : qFloor(tlP[2]);
    p1.x = trP[0] > 0.0f ? qCeil(trP[0]) : qFloor(trP[0]);
    p1.y = trP[2] > 0.0f ? qCeil(trP[2]) : qFloor(trP[2]);
    p2.x = blP[0] > 0.0f ? qCeil(blP[0]) : qFloor(blP[0]);
    p2.y = blP[2] > 0.0f ? qCeil(blP[2]) : qFloor(blP[2]);
    p3.x = brP[0] > 0.0f ? qCeil(brP[0]) : qFloor(brP[0]);
    p3.y = brP[2] > 0.0f ? qCeil(brP[2]) : qFloor(brP[2]);
    auto yMax = qMax(qMax(p0.y, p1.y), qMax(p2.y, p3.y));
    auto yMin = qMin(qMin(p0.y, p1.y), qMin(p2.y, p3.y));
    auto xMax = qMax(qMax(p0.x, p1.x), qMax(p2.x, p3.x));
    auto xMin = qMin(qMin(p0.x, p1.x), qMin(p2.x, p3.x));
    PointI centerZ;
    centerZ.x = _activeConfig.target31.x >> (31 - _activeConfig.zoomBase);
    centerZ.y = _activeConfig.target31.y >> (31 - _activeConfig.zoomBase);
    for(int32_t y = yMin; y <= yMax; y++)
    {
        for(int32_t x = xMin; x <= xMax; x++)
        {
            TileId tileId;
            tileId.x = centerZ.x + x;
            tileId.y = centerZ.y + y;

            _visibleTiles.insert(tileId);
        }
    }

    // Compute in-tile offset
    auto zoomTileMask = ((1u << _activeConfig.zoomBase) - 1) << (31 - _activeConfig.zoomBase);
    auto tileXo31 = _activeConfig.target31.x & zoomTileMask;
    auto tileYo31 = _activeConfig.target31.y & zoomTileMask;
    auto div = 1u << (31 - _activeConfig.zoomBase);
    if(div > 1)
        div -= 1;
    _normalizedTargetInTileOffset.x = static_cast<double>(_activeConfig.target31.x - tileXo31) / div;
    _normalizedTargetInTileOffset.y = static_cast<double>(_activeConfig.target31.y - tileYo31) / div;
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

void OsmAnd::AtlasMapRenderer_BaseOpenGL::compuleSkyplaneSize()
{
    // Setup fake camera
    auto mView = glm::translate(0.0f, 0.0f, -_distanceFromCameraToTarget);

    glm::vec4 glViewport(
        _activeConfig.viewport.left,
        _activeConfig.windowSize.y - _activeConfig.viewport.bottom,
        _activeConfig.viewport.width(),
        _activeConfig.viewport.height());

    // Top-left far
    const auto& tlf = glm::unProject(
        glm::vec3(_activeConfig.viewport.left, _activeConfig.windowSize.y - _activeConfig.viewport.bottom + _activeConfig.viewport.height(), 1.0),
        mView, _mProjection, glViewport);

    _skyplaneHalfSize[0] = qAbs(tlf.x);
    _skyplaneHalfSize[1] = qAbs(tlf.y);
}

void OsmAnd::AtlasMapRenderer_BaseOpenGL::computeFog()
{
}
