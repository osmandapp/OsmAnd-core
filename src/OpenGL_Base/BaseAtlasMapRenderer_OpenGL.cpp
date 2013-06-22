#include "BaseAtlasMapRenderer_OpenGL.h"

#include <assert.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <QtGlobal>
#include <QtCore/qmath.h>
#include <QThread>

#include "IMapRenderer.h"
#include "IMapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "OsmAndLogging.h"

OsmAnd::BaseAtlasMapRenderer_OpenGL::BaseAtlasMapRenderer_OpenGL()
    : _maxTextureSize(0)
    , _atlasSizeOnTexture(0)
    , _lastUnfinishedAtlas(0)
    , _renderThreadId(nullptr)
{
}

OsmAnd::BaseAtlasMapRenderer_OpenGL::~BaseAtlasMapRenderer_OpenGL()
{
}

float OsmAnd::BaseAtlasMapRenderer_OpenGL::calculateCameraDistance( const glm::mat4& P, const AreaI& viewport, const float& Ax, const float& Sx, const float& k )
{
    const float w = viewport.width();
    const float x = viewport.left;

    const float fw = (Sx*k) / (0.5f * viewport.width());

    float d = (P[3][0] + Ax*P[0][0] - fw*(P[3][3] + Ax*P[0][3]))/(P[2][0] - fw*P[2][3]);

    return d;
}

bool OsmAnd::BaseAtlasMapRenderer_OpenGL::rayIntersectPlane( const glm::vec3& planeN, float planeO, const glm::vec3& rayD, const glm::vec3& rayO, float& distance )
{
    auto alpha = glm::dot(planeN, rayD);

    if(!qFuzzyCompare(alpha, 0.0f))
    {
        distance = (-glm::dot(planeN, rayO) + planeO) / alpha;

        return (distance >= 0.0f);
    }

    return false;
}

void OsmAnd::BaseAtlasMapRenderer_OpenGL::updateConfiguration()
{
    if(elevationDataCacheInvalidated)
    {
        // Recreate tile patch since elevation data influences density of tile patch
        releaseTilePatch();
        createTilePatch();
    }

    IMapRenderer::updateConfiguration();

    computeMatrices();
    computeVisibleTileset();
}

void OsmAnd::BaseAtlasMapRenderer_OpenGL::computeMatrices()
{
    // Setup projection with fake Z-far plane
    GLfloat aspectRatio = static_cast<GLfloat>(_activeConfig.viewport.width());
    auto viewportHeight = _activeConfig.viewport.height();
    if(viewportHeight > 0)
        aspectRatio /= static_cast<GLfloat>(viewportHeight);
    _mProjection = glm::perspective(_activeConfig.fieldOfView, aspectRatio, 0.1f, 1000.0f);

    // Calculate limits of camera distance to target and actual distance
    const float screenTile = _activeConfig.tileProvider->getTileSize() * (_activeConfig.displayDensityFactor / _activeConfig.tileProvider->getTileDensity());
    const float nearD = calculateCameraDistance(_mProjection, _activeConfig.viewport, TileDimension3D / 2.0f, screenTile / 2.0f, 1.5f);
    const float baseD = calculateCameraDistance(_mProjection, _activeConfig.viewport, TileDimension3D / 2.0f, screenTile / 2.0f, 1.0f);
    const float farD = calculateCameraDistance(_mProjection, _activeConfig.viewport, TileDimension3D / 2.0f, screenTile / 2.0f, 0.75f);

    // zoomFraction == [ 0.0 ... 0.5] scales tile [1.0x ... 1.5x]
    // zoomFraction == [-0.5 ...-0.0] scales tile [.75x ... 1.0x]
    if(_activeConfig.zoomFraction >= 0.0f)
        _distanceFromCameraToTarget = baseD - (baseD - nearD) * (2.0f * _activeConfig.zoomFraction);
    else
        _distanceFromCameraToTarget = baseD - (farD - baseD) * (2.0f * _activeConfig.zoomFraction);

    // Recalculate projection with obtained value
    _mProjection = glm::perspective(_activeConfig.fieldOfView, aspectRatio, 0.1f, _activeConfig.fogDistance * 1.2f + _distanceFromCameraToTarget);

    // Setup camera
    const auto& c0 = glm::translate(0.0f, 0.0f, -_distanceFromCameraToTarget);
    const auto& c1 = glm::rotate(c0, _activeConfig.elevationAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    _mView = glm::rotate(c1, _activeConfig.azimuth, glm::vec3(0.0f, 1.0f, 0.0f));
}

void OsmAnd::BaseAtlasMapRenderer_OpenGL::computeVisibleTileset()
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
    auto clip = _activeConfig.fogDistance * 1.2f + _distanceFromCameraToTarget;
    float tlD, trD, blD, brD;
    bool intersects;
    intersects = rayIntersectPlane(planeN, 0.0f, tlRayD, tln, tlD);
    if(!intersects)
        tlD = clip * 1.5f;
    auto tlP = tln + tlRayD * tlD;
    intersects = rayIntersectPlane(planeN, 0.0f, trRayD, trn, trD);
    if(!intersects)
        trD = clip * 1.5f;
    auto trP = trn + trRayD * trD;
    intersects = rayIntersectPlane(planeN, 0.0f, blRayD, bln, blD);
    if(!intersects)
        blD = clip * 1.5f;
    auto blP = bln + blRayD * blD;
    intersects = rayIntersectPlane(planeN, 0.0f, brRayD, brn, brD);
    if(!intersects)
        brD = clip * 1.5f;
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
    tlP /= TileDimension3D;
    trP /= TileDimension3D;
    blP /= TileDimension3D;
    brP /= TileDimension3D;

    // Obtain visible tile indices in current zoom
    //TODO: it's not optimal, since dumb AABB takes a lot of unneeded tiles.
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
            if(tileId.x < 0 || tileId.x > (1u << _activeConfig.zoomBase) - 1)
                continue;
            tileId.y = centerZ.y + y;
            if(tileId.y < 0 || tileId.y > (1u << _activeConfig.zoomBase) - 1)
                continue;

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
    _targetInTile.x = static_cast<double>(_activeConfig.target31.x - tileXo31) / div;
    _targetInTile.y = static_cast<double>(_activeConfig.target31.y - tileYo31) / div;
}

void OsmAnd::BaseAtlasMapRenderer_OpenGL::cacheTile( const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap )
{
    assert(!_tilesCache.contains(zoom, tileId));

    if(_renderThreadId != QThread::currentThreadId())
    {
        QMutexLocker scopeLock(&_tilesPendingToCacheMutex);

        assert(!_tilesPendingToCache[zoom].contains(tileId));

        TilePendingToCache pendingTile;
        pendingTile.tileId = tileId;
        pendingTile.zoom = zoom;
        pendingTile.tileBitmap = tileBitmap;
        _tilesPendingToCacheQueue.enqueue(pendingTile);
        _tilesPendingToCache[zoom].insert(tileId);

        return;
    }

    uploadTileToTexture(tileId, zoom, tileBitmap);
}

int OsmAnd::BaseAtlasMapRenderer_OpenGL::getCachedTilesCount()
{
    return _texturesRefCounts.size();
}

void OsmAnd::BaseAtlasMapRenderer_OpenGL::initializeRendering()
{
    assert(!_isRenderingInitialized);

    _renderThreadId = QThread::currentThreadId();
    createTilePatch();
 
    _isRenderingInitialized = true;
}

void OsmAnd::BaseAtlasMapRenderer_OpenGL::performRendering()
{
    assert(_isRenderingInitialized);
    assert(_renderThreadId == QThread::currentThreadId());

    if(_configInvalidated)
        updateConfiguration();

    if(!viewIsDirty)
        return;

    {
        QMutexLocker scopeLock(&_tilesPendingToCacheMutex);

        while(!_tilesPendingToCacheQueue.isEmpty())
        {
            const auto& pendingTile = _tilesPendingToCacheQueue.dequeue();
            _tilesPendingToCache[pendingTile.zoom].remove(pendingTile.tileId);

            // This tile may already have been loaded earlier (since it's already on disk!)
            {
                QMutexLocker scopeLock(&_tilesCacheMutex);
                if(_tilesCache.contains(pendingTile.zoom, pendingTile.tileId))
                    continue;
            }

            uploadTileToTexture(pendingTile.tileId, pendingTile.zoom, pendingTile.tileBitmap);
        }
    }
    updateTilesCache();

    updateElevationDataCache();
}

void OsmAnd::BaseAtlasMapRenderer_OpenGL::releaseRendering()
{
    assert(_isRenderingInitialized);
    assert(_renderThreadId == QThread::currentThreadId());

    purgeTilesCache();
    releaseTilePatch();

    _isRenderingInitialized = false;
}

void OsmAnd::BaseAtlasMapRenderer_OpenGL::createTilePatch()
{
    //NOTE: this can be optimized using tessellation shader and GL_PATCH
    Vertex* pVertices = nullptr;
    uint32_t verticesCount = 0;
    GLushort* pIndices = nullptr;
    uint32_t indicesCount = 0;
    
    if(!_activeConfig.elevationDataProvider)
    {
        // Simple tile patch, that consists of 4 vertices

        // Vertex data
        const GLfloat tsz = static_cast<GLfloat>(TileDimension3D);
        Vertex vertices[4] =
        {
            // CCW
            { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
            { {0.0f, 0.0f,  tsz}, {0.0f, 1.0f} },
            { { tsz, 0.0f,  tsz}, {1.0f, 1.0f} },
            { { tsz, 0.0f, 0.0f}, {1.0f, 0.0f} }
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
        //TODO: complex tile patch
        assert(false);
    }
    
    allocateTilePatch(pVertices, verticesCount, pIndices, indicesCount);

    if(_activeConfig.elevationDataProvider)
    {
        delete[] pVertices;
        delete[] pIndices;
    }
}

void OsmAnd::BaseAtlasMapRenderer_OpenGL::purgeTilesCache()
{
    BaseAtlasMapRenderer::purgeTilesCache();

    assert(_freeAtlasSlots.isEmpty());
    _lastUnfinishedAtlas = 0;
}

OsmAnd::BaseAtlasMapRenderer_OpenGL::CachedTile_OpenGL::CachedTile_OpenGL( BaseAtlasMapRenderer_OpenGL* owner_, const uint32_t& zoom, const TileId& id, const size_t& usedMemory, uint32_t textureId_, uint32_t atlasSlotIndex_ )
    : IMapRenderer::CachedTile(zoom, id, usedMemory)
    , owner(owner_)
    , textureId(textureId_)
    , atlasSlotIndex(atlasSlotIndex_)
{
}

OsmAnd::BaseAtlasMapRenderer_OpenGL::CachedTile_OpenGL::~CachedTile_OpenGL()
{
    assert(owner->_renderThreadId == QThread::currentThreadId());

    const auto& itRefCnt = owner->_texturesRefCounts.find(textureId);
    auto& refCnt = *itRefCnt;
    refCnt -= 1;

    if(owner->_maxTextureSize != 0 && refCnt > 0)
    {
        // A free atlas slot
        owner->_freeAtlasSlots.insert(textureId, atlasSlotIndex);
    }

    if(refCnt == 0)
    {
        owner->_freeAtlasSlots.remove(textureId);
        owner->releaseTexture(textureId);
        owner->_texturesRefCounts.remove(textureId);
    }
}
