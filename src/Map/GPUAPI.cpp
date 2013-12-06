#include "GPUAPI.h"

#include <cassert>

#include "Logging.h"

OsmAnd::GPUAPI::GPUAPI()
    : _isSupported_8bitPaletteRGBA8(false)
    , isSupported_8bitPaletteRGBA8(_isSupported_8bitPaletteRGBA8)
{
}

OsmAnd::GPUAPI::~GPUAPI()
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

bool OsmAnd::GPUAPI::initialize()
{
    return true;
}

bool OsmAnd::GPUAPI::release()
{
    return true;
}

std::shared_ptr<OsmAnd::GPUAPI::AtlasTexturesPool> OsmAnd::GPUAPI::obtainAtlasTexturesPool( const AtlasTypeId& atlasTypeId )
{
    auto itPool = _atlasTexturesPools.constFind(atlasTypeId);
    if(itPool == _atlasTexturesPools.cend())
    {
        std::shared_ptr<AtlasTexturesPool> pool(new AtlasTexturesPool(this, atlasTypeId));
        itPool = _atlasTexturesPools.insert(atlasTypeId, pool);
    }

    return *itPool;
}

std::shared_ptr<OsmAnd::GPUAPI::SlotOnAtlasTextureInGPU> OsmAnd::GPUAPI::allocateTile( const std::shared_ptr<AtlasTexturesPool>& pool, AtlasTexturesPool::AtlasTextureAllocatorSignature atlasTextureAllocator )
{
    return pool->allocateTile(atlasTextureAllocator);
}

OsmAnd::GPUAPI::ResourceInGPU::ResourceInGPU( const Type type_, GPUAPI* api_, const RefInGPU& refInGPU_ )
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
        api->_allocatedResourcesCounter.ref();
#endif
    }
}

OsmAnd::GPUAPI::ResourceInGPU::~ResourceInGPU()
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
        api->_allocatedResourcesCounter.deref();
#endif
    }
}

OsmAnd::GPUAPI::TextureInGPU::TextureInGPU(GPUAPI* api_, const RefInGPU& refInGPU_, const unsigned int textureSize_, const unsigned int mipmapLevels_)
    : ResourceInGPU(Type::Texture, api_, refInGPU_)
    , textureSize(textureSize_)
    , mipmapLevels(mipmapLevels_)
    , texelSizeN(1.0f / static_cast<float>(textureSize_))
    , halfTexelSizeN(0.5f / static_cast<float>(textureSize_))
{
}

OsmAnd::GPUAPI::TextureInGPU::~TextureInGPU()
{
}

OsmAnd::GPUAPI::ArrayBufferInGPU::ArrayBufferInGPU(GPUAPI* api_, const RefInGPU& refInGPU_, const unsigned int itemsCount_)
    : ResourceInGPU(Type::ArrayBuffer, api_, refInGPU_)
    , itemsCount(itemsCount_)
{
}

OsmAnd::GPUAPI::ArrayBufferInGPU::~ArrayBufferInGPU()
{
}

OsmAnd::GPUAPI::AtlasTextureInGPU::AtlasTextureInGPU(GPUAPI* api_, const RefInGPU& refInGPU_, const unsigned int textureSize_, const unsigned int mipmapLevels_, const std::shared_ptr<AtlasTexturesPool>& pool_)
    : TextureInGPU(api_, refInGPU_, textureSize_, mipmapLevels_)
    , tileSize(pool_->typeId.tileSize)
    , padding(pool_->typeId.tilePadding)
    , slotsPerSide(textureSize_ / (tileSize + 2*padding))
    , tileSizeN(static_cast<float>(tileSize + 2*padding) / static_cast<float>(textureSize_))
    , tilePaddingN(static_cast<float>(padding) / static_cast<float>(textureSize_))
    , pool(pool_)
{
}

OsmAnd::GPUAPI::AtlasTextureInGPU::~AtlasTextureInGPU()
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

OsmAnd::GPUAPI::SlotOnAtlasTextureInGPU::SlotOnAtlasTextureInGPU(const std::shared_ptr<AtlasTextureInGPU>& atlas_, const unsigned int slotIndex_)
    : ResourceInGPU(Type::SlotOnAtlasTexture, atlas_->api, atlas_->refInGPU)
    , atlasTexture(atlas_)
    , slotIndex(slotIndex_)
{
    // Add reference of this tile to atlas texture
    {
#if defined(DEBUG) || defined(_DEBUG)
        QMutexLocker scopedLock(&atlasTexture->_tilesMutex);
        atlasTexture->_tiles.insert(this);
#else
        atlasTexture->_tilesCounter.ref();
#endif
    }
}

OsmAnd::GPUAPI::SlotOnAtlasTextureInGPU::~SlotOnAtlasTextureInGPU()
{
    // Remove reference of this tile to atlas texture
    {
#if defined(DEBUG) || defined(_DEBUG)
        QMutexLocker scopedLock(&atlasTexture->_tilesMutex);
        atlasTexture->_tiles.remove(this);
#else
        atlasTexture->_tilesCounter.deref();
#endif
    }

    // Publish slot that was occupied by this tile as freed
    {
        QMutexLocker scopedLock(&atlasTexture->pool->_freedSlotsMutex);

        atlasTexture->pool->_freedSlots.insert(atlasTexture.get(), qMove(AtlasTexturesPool::FreedSlotsEntry(atlasTexture, slotIndex)));
    }

    // Clear reference to GPU resource to avoid removal in base class
    _refInGPU = nullptr;
}

OsmAnd::GPUAPI::AtlasTexturesPool::AtlasTexturesPool( GPUAPI* api_, const AtlasTypeId& typeId_ )
    : _lastNonFullAtlasTexture(nullptr)
    , _firstUnusedSlotIndex(0)
    , api(api_)
    , typeId(typeId_)
{
}

OsmAnd::GPUAPI::AtlasTexturesPool::~AtlasTexturesPool()
{
}

std::shared_ptr<OsmAnd::GPUAPI::SlotOnAtlasTextureInGPU> OsmAnd::GPUAPI::AtlasTexturesPool::allocateTile( AtlasTextureAllocatorSignature atlasTextureAllocator )
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
            return std::shared_ptr<SlotOnAtlasTextureInGPU>(new SlotOnAtlasTextureInGPU(atlasTexture.lock(), slotIndex));
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
        return std::shared_ptr<SlotOnAtlasTextureInGPU>(new SlotOnAtlasTextureInGPU(atlasTexture, _firstUnusedSlotIndex++));
    }

    return nullptr;
}
