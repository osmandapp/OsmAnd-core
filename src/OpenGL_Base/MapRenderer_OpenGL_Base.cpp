#include "MapRenderer_OpenGL_Base.h"

#include <assert.h>
#include <QThread>

OsmAnd::MapRenderer_BaseOpenGL::MapRenderer_BaseOpenGL()
    : _maxTextureSize(0)
    , _atlasSizeOnTexture(0)
    , _lastUnfinishedAtlas(0)
    , _renderThreadId(nullptr)
{
}

OsmAnd::MapRenderer_BaseOpenGL::~MapRenderer_BaseOpenGL()
{
}

void OsmAnd::MapRenderer_BaseOpenGL::initializeRendering()
{
    assert(!_isRenderingInitialized);

    _renderThreadId = QThread::currentThreadId();
    // Get maximal texture size if not yet determined
    if(_maxTextureSize == 0)
    {
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&_maxTextureSize));
        GL_CHECK_RESULT;
    }
    
    _isRenderingInitialized = true;
}

void OsmAnd::MapRenderer_BaseOpenGL::performRendering()
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

void OsmAnd::MapRenderer_BaseOpenGL::releaseRendering()
{
    assert(_isRenderingInitialized);
    assert(_renderThreadId == QThread::currentThreadId());

    purgeTilesCache();

    _isRenderingInitialized = false;
}

int OsmAnd::MapRenderer_BaseOpenGL::getCachedTilesCount()
{
    return _texturesRefCounts.size();
}

void OsmAnd::MapRenderer_BaseOpenGL::purgeTilesCache()
{
    IMapRenderer::purgeTilesCache();

    assert(_freeAtlasSlots.isEmpty());
    _lastUnfinishedAtlas = 0;
}

void OsmAnd::MapRenderer_BaseOpenGL::cacheTile( const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap )
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

void OsmAnd::MapRenderer_BaseOpenGL::validateResult()
{
    assert(false);
}

OsmAnd::MapRenderer_BaseOpenGL::CachedTile_OpenGL::CachedTile_OpenGL( MapRenderer_BaseOpenGL* owner_, const uint32_t& zoom, const TileId& id, const size_t& usedMemory, uint32_t textureId_, uint32_t atlasSlotIndex_ )
    : IMapRenderer::CachedTile(zoom, id, usedMemory)
    , owner(owner_)
    , textureId(textureId_)
    , atlasSlotIndex(atlasSlotIndex_)
{
}

OsmAnd::MapRenderer_BaseOpenGL::CachedTile_OpenGL::~CachedTile_OpenGL()
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
