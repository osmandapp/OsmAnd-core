#include "RenderAPI.h"

#include <assert.h>

#include "Logging.h"

OsmAnd::RenderAPI::RenderAPI()
    : tilesPerAtlasTextureLimit(1)
{
}

OsmAnd::RenderAPI::~RenderAPI()
{
    const int resourcesRemaining = 
#if defined(DEBUG) || defined(_DEBUG)
        _allocatedResources.size();
#else
        _allocatedResourcesCounter.load();
#endif
    if(resourcesRemaining > 0)
        LogPrintf(LogSeverityLevel::Error, "By the time of RenderAPI destruction, it still contained %d allocated resources that will probably leak", resourcesRemaining);
    assert(resourcesRemaining == 0);
}

bool OsmAnd::RenderAPI::initialize()
{
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
    // Add this object to allocated resources list
    {
#if defined(DEBUG) || defined(_DEBUG)
        QMutexLocker scopedLock(&api->_allocatedResourcesMutex);
        api->_allocatedResources.push_back(this);
#else
        api->_allocatedResourcesCounter.fetchAndAddRelaxed(1);
#endif
    }
}

OsmAnd::RenderAPI::ResourceInGPU::~ResourceInGPU()
{
    // If we have reference to 
    if(_refInGPU)
        api->releaseResourceInGPU(type, _refInGPU);

    // Remove this object from allocated resources list
    {
#if defined(DEBUG) || defined(_DEBUG)
        QMutexLocker scopedLock(&api->_allocatedResourcesMutex);
        api->_allocatedResources.removeOne(this);
#else
        api->_allocatedResourcesCounter.fetchAndAddRelaxed(-1);
#endif
    }
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
    const int tilesRemaining = 
#if defined(DEBUG) || defined(_DEBUG)
        _tiles.size();
#else
        _tilesCounter.load();
#endif
    if(tilesRemaining > 0)
        LogPrintf(LogSeverityLevel::Error, "By the time of atlas texture destruction, it still contained %d allocated tiles", tilesRemaining);
    assert(tilesRemaining == 0);

    // Clear all references to this atlas
    {
        QMutexLocker scopedLock(&pool->_freedSlotsMutex);

        pool->_freedSlots.remove(this);
    }
    {
        QMutexLocker scopedLock(&pool->_unusedSlotsMutex);

        if(pool->_lastNonFullAtlasTexture == this)
        {
            pool->_lastNonFullAtlasTexture = nullptr;
            pool->_lastNonFullAtlasTextureWeak.reset();
        }
    }
}

OsmAnd::RenderAPI::TileOnAtlasTextureInGPU::TileOnAtlasTextureInGPU( const std::shared_ptr<AtlasTextureInGPU>& atlas_, const uint32_t& slotIndex_ )
    : ResourceInGPU(Type::TileOnAtlasTexture, atlas_->api, atlas_->refInGPU)
    , atlasTexture(atlas_)
    , slotIndex(slotIndex_)
{
    // Add reference of this tile to atlas texture
    {
#if defined(DEBUG) || defined(_DEBUG)
        QMutexLocker scopedLock(&atlasTexture->_tilesMutex);
        atlasTexture->_tiles.insert(this);
#else
        atlasTexture->_tilesCounter.fetchAndAddRelaxed(1);
#endif
    }
}

OsmAnd::RenderAPI::TileOnAtlasTextureInGPU::~TileOnAtlasTextureInGPU()
{
    // Remove reference of this tile to atlas texture
    {
#if defined(DEBUG) || defined(_DEBUG)
        QMutexLocker scopedLock(&atlasTexture->_tilesMutex);
        atlasTexture->_tiles.remove(this);
#else
        atlasTexture->_tilesCounter.fetchAndAddRelaxed(-1);
#endif
    }

    // Publish slot that was occupied by this tile as freed
    {
        QMutexLocker scopedLock(&atlasTexture->pool->_freedSlotsMutex);

        atlasTexture->pool->_freedSlots.insert(atlasTexture.get(), std::tuple< std::weak_ptr<AtlasTextureInGPU>, uint32_t >(atlasTexture, slotIndex));
    }

    // Clear reference to GPU resource to avoid removal in base class
    _refInGPU = nullptr;
}

OsmAnd::RenderAPI::AtlasTexturesPool::AtlasTexturesPool( RenderAPI* api_, const AtlasTypeId& typeId_ )
    : _lastNonFullAtlasTexture(nullptr)
    , _firstUnusedSlotIndex(0)
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

        while(!_freedSlots.isEmpty())
        {
            const auto& itFreedSlotEntry = _freedSlots.begin();

            // Mark slot as occupied
            const auto freedSlotEntry = itFreedSlotEntry.value();
            _freedSlots.erase(itFreedSlotEntry);

            // Return allocated slot
            const auto& atlasTexture = std::get<0>(freedSlotEntry);
            const auto& slotIndex = std::get<1>(freedSlotEntry);
            return std::shared_ptr<TileOnAtlasTextureInGPU>(new TileOnAtlasTextureInGPU(atlasTexture.lock(), slotIndex));
        }
    }
    
    {
        QMutexLocker scopedLock(&_unusedSlotsMutex);

        std::shared_ptr<AtlasTextureInGPU> atlasTexture;
        
        // If we've never allocated any atlases yet or next unused slot is beyond allocated spaced - allocate new atlas texture then
        if(!_lastNonFullAtlasTexture || _firstUnusedSlotIndex == _lastNonFullAtlasTexture->slotsPerSide*_lastNonFullAtlasTexture->slotsPerSide)
        {
            atlasTexture.reset(atlasTextureAllocator());

            _lastNonFullAtlasTexture = atlasTexture.get();
            _lastNonFullAtlasTextureWeak = atlasTexture;
            _firstUnusedSlotIndex = 0;
        }
        else
        {
            atlasTexture = _lastNonFullAtlasTextureWeak.lock();
        }

        // Or let's just continue using current atlas texture
        return std::shared_ptr<TileOnAtlasTextureInGPU>(new TileOnAtlasTextureInGPU(atlasTexture, _firstUnusedSlotIndex++));
    }

    return nullptr;
}
