#include "RenderAPI.h"

#include <assert.h>

#include "Logging.h"

OsmAnd::RenderAPI::RenderAPI()
    : _optimalTilesPerAtlasSqrt(0)
    , optimalTilesPerAtlasSqrt(_optimalTilesPerAtlasSqrt)
{
}

OsmAnd::RenderAPI::~RenderAPI()
{
    if(!_allocatedResources.isEmpty())
        LogPrintf(LogSeverityLevel::Error, "By the time of RenderAPI destruction, it still contained %d allocated resources that will probably leak", _allocatedResources.size());
    assert(_allocatedResources.isEmpty());
}

bool OsmAnd::RenderAPI::initialize( const uint32_t& optimalTilesPerAtlasSqrt_ )
{
    _optimalTilesPerAtlasSqrt = optimalTilesPerAtlasSqrt_;

    return true;
}

bool OsmAnd::RenderAPI::release()
{
    return true;
}

std::shared_ptr<OsmAnd::RenderAPI::AtlasTexturesPool> OsmAnd::RenderAPI::obtainAtlasTexturesPool( const AtlasTypeId& atlasTypeId )
{
    auto itPool = _atlasTexturesPools.find(atlasTypeId);
    if(itPool == _atlasTexturesPools.end())
    {
        std::shared_ptr<AtlasTexturesPool> pool(new AtlasTexturesPool(this, atlasTypeId));
        itPool = _atlasTexturesPools.insert(atlasTypeId, pool);
    }

    return *itPool;
}

std::shared_ptr<OsmAnd::RenderAPI::TileOnAtlasTextureInGPU> OsmAnd::RenderAPI::allocateTile( const std::shared_ptr<AtlasTexturesPool>& pool, AtlasTexturesPool::AtlasTextureAllocatorSignature atlasTextureAllocator )
{
    return pool->allocateTile(atlasTextureAllocator);
}

OsmAnd::RenderAPI::ResourceInGPU::ResourceInGPU( const Type& type_, RenderAPI* api_, const RefInGPU& refInGPU_ )
    : _refInGPU(refInGPU_)
    , api(api_)
    , type(type_)
    , refInGPU(_refInGPU)
{
}

OsmAnd::RenderAPI::ResourceInGPU::~ResourceInGPU()
{
    // If we have reference to 
    if(_refInGPU)
        api->releaseResourceInGPU(type, _refInGPU);
}

OsmAnd::RenderAPI::TextureInGPU::TextureInGPU( RenderAPI* api_, const RefInGPU& refInGPU_, const uint32_t& textureSize_, const uint32_t& mipmapLevels_ )
    : ResourceInGPU(Type::Texture, api_, refInGPU_)
    , textureSize(textureSize_)
    , mipmapLevels(mipmapLevels_)
    , texelSizeN(1.0f / static_cast<float>(textureSize_))
    , halfTexelSizeN(0.5f / static_cast<float>(textureSize_))
{
}

OsmAnd::RenderAPI::TextureInGPU::~TextureInGPU()
{
}

OsmAnd::RenderAPI::AtlasTextureInGPU::AtlasTextureInGPU( RenderAPI* api_, const RefInGPU& refInGPU_, const uint32_t& textureSize_, const uint32_t& mipmapLevels_, const std::shared_ptr<AtlasTexturesPool>& pool_ )
    : TextureInGPU(api_, refInGPU_, textureSize_, mipmapLevels_)
    , tiles(_tiles)
    , tileSize(pool_->typeId.tileSize)
    , padding(pool_->typeId.tilePadding)
    , slotsPerSide(textureSize_ / (tileSize + 2*padding))
    , tileSizeN(static_cast<float>(tileSize + 2*padding) / static_cast<float>(textureSize_))
    , tilePaddingN(static_cast<float>(padding) / static_cast<float>(textureSize_))
    , pool(pool_)
{
}

OsmAnd::RenderAPI::AtlasTextureInGPU::~AtlasTextureInGPU()
{
    if(!_tiles.isEmpty())
        LogPrintf(LogSeverityLevel::Error, "By the time of atlas texture destruction, it still contained %d allocated tiles", _tiles.size());
    assert(_tiles.isEmpty());

    // Clear all references to this atlas
    {
        QMutexLocker scopedLock(&pool->_freedSlotsMutex);

        pool->_freedSlots.remove(this);
    }
    {
        QMutexLocker scopedLock(&pool->_unusedSlotsMutex);

        if(pool->_lastNonFullAtlasTexture.get() == this)
            pool->_lastNonFullAtlasTexture.reset();
    }
}

OsmAnd::RenderAPI::TileOnAtlasTextureInGPU::TileOnAtlasTextureInGPU( AtlasTextureInGPU* const atlas_, const uint32_t& slotIndex_ )
    : ResourceInGPU(Type::TileOnAtlasTexture, atlas_->api, atlas_->refInGPU)
    , atlasTexture(atlas_)
    , slotIndex(slotIndex_)
{
    // Add reference of this tile to atlas texture
    {
        QMutexLocker scopedLock(&atlasTexture->_tilesMutex);
        atlasTexture->_tiles.insert(this);
    }
}

OsmAnd::RenderAPI::TileOnAtlasTextureInGPU::~TileOnAtlasTextureInGPU()
{
    // Remove reference of this tile to atlas texture
    {
        QMutexLocker scopedLock(&atlasTexture->_tilesMutex);
        atlasTexture->_tiles.remove(this);
    }

    // Publish slot that was occupied by this tile as freed
    {
        QMutexLocker scopedLock(&atlasTexture->pool->_freedSlotsMutex);

        atlasTexture->pool->_freedSlots.insert(atlasTexture, slotIndex);
    }

    // Clear reference to GPU resource to avoid removal in base class
    _refInGPU = nullptr;
}

OsmAnd::RenderAPI::AtlasTexturesPool::AtlasTexturesPool( RenderAPI* api_, const AtlasTypeId& typeId_ )
    : _firstUnusedSlotIndex(0)
    , api(api_)
    , typeId(typeId_)
{
}

OsmAnd::RenderAPI::AtlasTexturesPool::~AtlasTexturesPool()
{
}

std::shared_ptr<OsmAnd::RenderAPI::TileOnAtlasTextureInGPU> OsmAnd::RenderAPI::AtlasTexturesPool::allocateTile( AtlasTextureAllocatorSignature atlasTextureAllocator )
{
    // First look for freed slots
    {
        QMutexLocker scopedLock(&_freedSlotsMutex);

        if(!_freedSlots.isEmpty())
        {
            const auto& itFreedSlotEntry = _freedSlots.begin();

            // Mark slot as occupied
            const auto atlasTexture = itFreedSlotEntry.key();
            const auto slotIndex = itFreedSlotEntry.value();
            _freedSlots.remove(atlasTexture, slotIndex);

            // Return allocated slot
            return std::shared_ptr<TileOnAtlasTextureInGPU>(new TileOnAtlasTextureInGPU(atlasTexture, slotIndex));
        }
    }
    
    {
        QMutexLocker scopedLock(&_unusedSlotsMutex);

        // If we've never allocated any atlases yet or next unused slot is beyond allocated spaced - allocate new atlas texture then
        if(!_lastNonFullAtlasTexture || _firstUnusedSlotIndex == _lastNonFullAtlasTexture->slotsPerSide * _lastNonFullAtlasTexture->slotsPerSide)
        {
            std::shared_ptr<AtlasTextureInGPU> atlasTexture(atlasTextureAllocator());

            api->_allocatedResources.push_back(atlasTexture);

            _lastNonFullAtlasTexture = atlasTexture;
            _firstUnusedSlotIndex = 0;
        }

        // Or let's just continue using current atlas texture
        return std::shared_ptr<TileOnAtlasTextureInGPU>(new TileOnAtlasTextureInGPU(_lastNonFullAtlasTexture.get(), _firstUnusedSlotIndex++));
    }
}
