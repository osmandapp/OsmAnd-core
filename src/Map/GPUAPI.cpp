#include "GPUAPI.h"

#include <cassert>

#include "Logging.h"

OsmAnd::GPUAPI::GPUAPI()
    : _isAttachedToRenderTarget(false)
{
}

OsmAnd::GPUAPI::~GPUAPI()
{
    const int resourcesRemaining =
#if OSMAND_DEBUG
        _allocatedResources.size();
#else
        _allocatedResourcesCounter.load();
#endif
    if (resourcesRemaining > 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "By the time of GPU destruction, it still contained %d allocated resources that will probably leak",
            resourcesRemaining);
    }
    assert(resourcesRemaining == 0);
}

bool OsmAnd::GPUAPI::isAttachedToRenderTarget()
{
    return _isAttachedToRenderTarget;
}

bool OsmAnd::GPUAPI::initialize()
{
    return true;
}

int OsmAnd::GPUAPI::checkElementVisibility(int queryIndex, float pointSize)
{
    return 0;
}

bool OsmAnd::GPUAPI::elementIsVisible(int queryIndex)
{
    return true;
}

bool OsmAnd::GPUAPI::attachToRenderTarget()
{
    _isAttachedToRenderTarget = true;
    return true;
}

bool OsmAnd::GPUAPI::detachFromRenderTarget(bool gpuContextLost)
{
    if (!isAttachedToRenderTarget())
    {
        return false;
    }

    _isAttachedToRenderTarget = false;

    return true;
}

bool OsmAnd::GPUAPI::release(bool gpuContextLost)
{
    bool ok;

    if (isAttachedToRenderTarget())
    {
        ok = detachFromRenderTarget(gpuContextLost);
        if (!ok)
            return false;
    }

    return true;
}

std::shared_ptr<OsmAnd::GPUAPI::AtlasTexturesPool> OsmAnd::GPUAPI::obtainAtlasTexturesPool(const AtlasTypeId& atlasTypeId)
{
    auto itPool = _atlasTexturesPools.constFind(atlasTypeId);
    if (itPool == _atlasTexturesPools.cend())
    {
        std::shared_ptr<AtlasTexturesPool> pool(new AtlasTexturesPool(this, atlasTypeId));
        itPool = _atlasTexturesPools.insert(atlasTypeId, pool);
    }

    return *itPool;
}

std::shared_ptr<OsmAnd::GPUAPI::SlotOnAtlasTextureInGPU> OsmAnd::GPUAPI::allocateTileInAltasTexture(
    const unsigned int tileSize,
    const AlphaChannelType alphaChannelType,
    const int64_t dateTimeFirst,
    const int64_t dateTimeLast,
    const int64_t dateTimePrevious,
    const int64_t dateTimeNext,
    const std::shared_ptr<AtlasTexturesPool>& pool,
    AtlasTexturesPool::AtlasTextureAllocator atlasTextureAllocator)
{
    return pool->allocateTile(tileSize, alphaChannelType,
        dateTimeFirst, dateTimeLast, dateTimePrevious, dateTimeNext, atlasTextureAllocator);
}

OsmAnd::AlphaChannelType OsmAnd::GPUAPI::getGpuResourceAlphaChannelType(const std::shared_ptr<const ResourceInGPU> gpuResource)
{
    if (gpuResource->type == ResourceInGPU::Type::SlotOnAtlasTexture)
        return std::static_pointer_cast<const SlotOnAtlasTextureInGPU>(gpuResource)->alphaChannelType;
    else //if (gpuResource->type == ResourceInGPU::Type::Texture)
        return std::static_pointer_cast<const TextureInGPU>(gpuResource)->alphaChannelType;
}

float OsmAnd::GPUAPI::getGpuResourceTexelSize(const std::shared_ptr<const ResourceInGPU> gpuResource)
{
    auto tileSize = gpuResource->type == ResourceInGPU::Type::SlotOnAtlasTexture
        ? std::static_pointer_cast<const SlotOnAtlasTextureInGPU>(gpuResource)->tileSize
        : std::static_pointer_cast<const TextureInGPU>(gpuResource)->width;
    return 1.0f / static_cast<float>(tileSize);
}

OsmAnd::GPUAPI::ResourceInGPU::ResourceInGPU(const Type type_, GPUAPI* api_, const RefInGPU& refInGPU_,
    const int64_t dateTimeFirst_ /*= 0*/, const int64_t dateTimeLast_ /*= 0*/,
    const int64_t dateTimePrevious_ /*= 0*/, const int64_t dateTimeNext_ /*= 0*/)
    : _refInGPU(refInGPU_)
    , api(api_)
    , type(type_)
    , refInGPU(_refInGPU)
    , dateTimeFirst(dateTimeFirst_)
    , dateTimeLast(dateTimeLast_)
    , dateTimePrevious(dateTimePrevious_)
    , dateTimeNext(dateTimeNext_)
{
    // Add this object to allocated resources list
    {
#if OSMAND_DEBUG
        QMutexLocker scopedLocker(&api->_allocatedResourcesMutex);
        api->_allocatedResources.push_back(this);
#else
        api->_allocatedResourcesCounter.ref();
#endif
    }
}

OsmAnd::GPUAPI::ResourceInGPU::~ResourceInGPU()
{
    // If we have reference to
    if (_refInGPU)
        api->releaseResourceInGPU(type, _refInGPU);

    // Remove this object from allocated resources list
    {
#if OSMAND_DEBUG
        QMutexLocker scopedLocker(&api->_allocatedResourcesMutex);
        api->_allocatedResources.removeOne(this);
#else
        api->_allocatedResourcesCounter.deref();
#endif
    }
}

void OsmAnd::GPUAPI::ResourceInGPU::lostRefInGPU() const
{
    _refInGPU = nullptr;
}

OsmAnd::GPUAPI::MetaResourceInGPU::MetaResourceInGPU(const Type type_, GPUAPI* api_)
    : ResourceInGPU(type_, api_, nullptr)
{
}

OsmAnd::GPUAPI::MetaResourceInGPU::~MetaResourceInGPU()
{
}

OsmAnd::GPUAPI::TextureInGPU::TextureInGPU(
    GPUAPI* api_,
    const RefInGPU& refInGPU_,
    const unsigned int width_,
    const unsigned int height_,
    const unsigned int mipmapLevels_,
    const AlphaChannelType alphaChannelType_,
    const int64_t dateTimeFirst_ /*= 0*/,
    const int64_t dateTimeLast_ /*= 0*/,
    const int64_t dateTimePrevious_ /*= 0*/,
    const int64_t dateTimeNext_ /*= 0*/)
    : ResourceInGPU(Type::Texture, api_, refInGPU_, dateTimeFirst_, dateTimeLast_, dateTimePrevious_, dateTimeNext_)
    , width(width_)
    , height(height_)
    , mipmapLevels(mipmapLevels_)
    , alphaChannelType(alphaChannelType_)
    , uTexelSizeN(1.0f / static_cast<float>(width))
    , vTexelSizeN(1.0f / static_cast<float>(height))
    , uHalfTexelSizeN(0.5f / static_cast<float>(width))
    , vHalfTexelSizeN(0.5f / static_cast<float>(height))
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

OsmAnd::GPUAPI::ElementArrayBufferInGPU::ElementArrayBufferInGPU(GPUAPI* api_, const RefInGPU& refInGPU_, const unsigned int itemsCount_)
    : ResourceInGPU(Type::ElementArrayBuffer, api_, refInGPU_)
    , itemsCount(itemsCount_)
{
}

OsmAnd::GPUAPI::ElementArrayBufferInGPU::~ElementArrayBufferInGPU()
{
}

OsmAnd::GPUAPI::AtlasTextureInGPU::AtlasTextureInGPU(
    GPUAPI* api_,
    const RefInGPU& refInGPU_,
    const unsigned int textureSize_,
    const unsigned int mipmapLevels_,
    const std::shared_ptr<AtlasTexturesPool>& pool_)
    : TextureInGPU(api_, refInGPU_, textureSize_, textureSize_, mipmapLevels_, AlphaChannelType::Invalid)
    , tileSize(pool_->typeId.tileSize)
    , padding(pool_->typeId.tilePadding)
    , slotsPerSide(textureSize_ / (tileSize + 2 * padding))
    , tileSizeN(static_cast<float>(tileSize + 2 * padding) / static_cast<float>(textureSize_))
    , tilePaddingN(static_cast<float>(padding) / static_cast<float>(textureSize_))
    , pool(pool_)
{
}

OsmAnd::GPUAPI::AtlasTextureInGPU::~AtlasTextureInGPU()
{
    const int tilesRemaining =
#if OSMAND_DEBUG
        _tiles.size();
#else
        _tilesCounter.load();
#endif
    if (tilesRemaining > 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "By the time of atlas texture destruction, it still contained %d allocated slots",
            tilesRemaining);
    }
    assert(tilesRemaining == 0);

    // Clear all references to this atlas
    {
        QMutexLocker scopedLocker(&pool->_freedSlotsMutex);

        pool->_freedSlots.remove(this);
    }
    {
        QMutexLocker scopedLocker(&pool->_unusedSlotsMutex);

        if (pool->_lastNonFullAtlasTexture == this)
        {
            pool->_lastNonFullAtlasTexture = nullptr;
            pool->_lastNonFullAtlasTextureWeak.reset();
        }
    }
}

OsmAnd::GPUAPI::SlotOnAtlasTextureInGPU::SlotOnAtlasTextureInGPU(
    const std::shared_ptr<AtlasTextureInGPU>& atlas_,
    const unsigned int slotIndex_,
    const unsigned int tileSize_,
    const AlphaChannelType alphaChannelType_,
    const int64_t dateTimeFirst_ /*= 0*/,
    const int64_t dateTimeLast_ /*= 0*/,
    const int64_t dateTimePrevious_ /*= 0*/,
    const int64_t dateTimeNext_ /*= 0*/)
    : ResourceInGPU(Type::SlotOnAtlasTexture, atlas_->api, atlas_->refInGPU,
        dateTimeFirst_, dateTimeLast_, dateTimePrevious_, dateTimeNext_)
    , atlasTexture(atlas_)
    , slotIndex(slotIndex_)
    , tileSize(tileSize_)
    , alphaChannelType(alphaChannelType_)
{
    // Add reference of this tile to atlas texture
    {
#if OSMAND_DEBUG
        QMutexLocker scopedLocker(&atlasTexture->_tilesMutex);
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
#if OSMAND_DEBUG
        QMutexLocker scopedLocker(&atlasTexture->_tilesMutex);
        atlasTexture->_tiles.remove(this);
#else
        atlasTexture->_tilesCounter.deref();
#endif
    }

    // Publish slot that was occupied by this tile as freed
    {
        QMutexLocker scopedLocker(&atlasTexture->pool->_freedSlotsMutex);

        atlasTexture->pool->_freedSlots.insert(atlasTexture.get(), qMove(AtlasTexturesPool::FreedSlotsEntry(atlasTexture, slotIndex)));
    }

    // Clear reference to GPU resource to avoid removal in base class
    _refInGPU = nullptr;
}

OsmAnd::GPUAPI::AtlasTexturesPool::AtlasTexturesPool(GPUAPI* api_, const AtlasTypeId& typeId_)
    : _lastNonFullAtlasTexture(nullptr)
    , _firstUnusedSlotIndex(0)
    , api(api_)
    , typeId(typeId_)
{
}

OsmAnd::GPUAPI::AtlasTexturesPool::~AtlasTexturesPool()
{
}

std::shared_ptr<OsmAnd::GPUAPI::SlotOnAtlasTextureInGPU> OsmAnd::GPUAPI::AtlasTexturesPool::allocateTile(
    const unsigned int tileSize,
    const AlphaChannelType alphaChannelType,
    const int64_t dateTimeFirst,
    const int64_t dateTimeLast,
    const int64_t dateTimePrevious,
    const int64_t dateTimeNext,
    AtlasTextureAllocator atlasTextureAllocator)
{
    // First look for freed slots
    {
        QMutexLocker scopedLocker(&_freedSlotsMutex);

        while (!_freedSlots.isEmpty())
        {
            const auto& itFreedSlotEntry = _freedSlots.begin();

            // Mark slot as occupied
            const auto freedSlotEntry = itFreedSlotEntry.value();
            _freedSlots.erase(itFreedSlotEntry);

            // Return allocated slot
            const auto& atlasTexture = std::get<0>(freedSlotEntry);
            const auto& slotIndex = std::get<1>(freedSlotEntry);
            return std::shared_ptr<SlotOnAtlasTextureInGPU>(new SlotOnAtlasTextureInGPU(
                atlasTexture.lock(),
                slotIndex,
                tileSize,
                alphaChannelType,
                dateTimeFirst,
                dateTimeLast,
                dateTimePrevious,
                dateTimeNext));
        }
    }

    {
        QMutexLocker scopedLocker(&_unusedSlotsMutex);

        std::shared_ptr<AtlasTextureInGPU> atlasTexture;

        // If we've never allocated any atlases yet or next unused slot is beyond allocated spaced - allocate new atlas texture then
        if (!_lastNonFullAtlasTexture || _firstUnusedSlotIndex == _lastNonFullAtlasTexture->slotsPerSide*_lastNonFullAtlasTexture->slotsPerSide)
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
        return std::shared_ptr<SlotOnAtlasTextureInGPU>(new SlotOnAtlasTextureInGPU(
            atlasTexture,
            _firstUnusedSlotIndex++,
            tileSize,
            alphaChannelType,
            dateTimeFirst,
            dateTimeLast,
            dateTimePrevious,
            dateTimeNext));
    }
}

OsmAnd::GPUAPI::MeshInGPU::MeshInGPU(
    GPUAPI* api_,
    const std::shared_ptr<ArrayBufferInGPU>& vertexBuffer_,
    const std::shared_ptr<ElementArrayBufferInGPU>& indexBuffer_,
    const std::shared_ptr<std::vector<std::pair<TileId, int32_t>>>& partSizes_,
    const ZoomLevel zoomLevel_,
    const bool isDenseObject_,
    const PointI* position31_/* = nullptr*/)
    : MetaResourceInGPU(Type::Mesh, api_)
    , vertexBuffer(vertexBuffer_)
    , indexBuffer(indexBuffer_)
    , partSizes(partSizes_)
    , zoomLevel(zoomLevel_)
    , isDenseObject(isDenseObject_)
    , position31(position31_)
{
}

OsmAnd::GPUAPI::MeshInGPU::~MeshInGPU()
{
    if (position31 != nullptr)
    {
        delete position31;
        position31 = nullptr;
    }
}

void OsmAnd::GPUAPI::MeshInGPU::lostRefInGPU() const
{
    if (vertexBuffer)
        vertexBuffer->lostRefInGPU();
    if (indexBuffer)
        indexBuffer->lostRefInGPU();
    _refInGPU = nullptr;
}
