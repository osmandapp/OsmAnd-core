#include "MapRenderer_OpenGL_Base.h"

#include <assert.h>
#include <QThread>

OsmAnd::MapRenderer_BaseOpenGL::MapRenderer_BaseOpenGL()
    : _maxTextureSize(0)
    , _atlasSizeOnTexture(0)
    , _lastUnfinishedAtlas(0)
{
}

OsmAnd::MapRenderer_BaseOpenGL::~MapRenderer_BaseOpenGL()
{
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
    assert(owner->renderThreadId == QThread::currentThreadId());

    const auto& itRefCnt = owner->_texturesRefCounts.find(textureId);
    auto& refCnt = *itRefCnt;
    refCnt -= 1;

    if(owner->_maxTextureSize != 0 && owner->_activeConfig.textureAtlasesAllowed && refCnt > 0)
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
