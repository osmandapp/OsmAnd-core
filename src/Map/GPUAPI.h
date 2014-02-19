#ifndef _OSMAND_CORE_GPU_API_H_
#define _OSMAND_CORE_GPU_API_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QHash>
#include <QMultiMap>
#include <QReadWriteLock>
#include <QMutex>
#include <QSet>
#include <QAtomicInt>

#include "OsmAndCore.h"
#include "Common.h"
#include "CommonTypes.h"
#include "MapTypes.h"

class SkBitmap;

namespace OsmAnd
{
    class MapTile;
    class MapSymbol;

    class GPUAPI
    {
        Q_DISABLE_COPY(GPUAPI);
    public:
        typedef void* RefInGPU;
        typedef uint32_t TextureFormat;
        union SourceFormat
        {
            uint32_t id;
            struct {
                uint16_t format;
                uint16_t type;
            };
        };

        class ResourceInGPU
        {
            Q_DISABLE_COPY(ResourceInGPU);
        public:
            enum class Type
            {
                Texture,
                SlotOnAtlasTexture,
                ArrayBuffer,
            };
        private:
        protected:
            ResourceInGPU(const Type type, GPUAPI* api, const RefInGPU& refInGPU);

            RefInGPU _refInGPU;
        public:
            virtual ~ResourceInGPU();

            GPUAPI* const api;
            const Type type;
            const RefInGPU& refInGPU;
        };

        class TextureInGPU : public ResourceInGPU
        {
            Q_DISABLE_COPY(TextureInGPU);
        private:
        protected:
        public:
            TextureInGPU(GPUAPI* api, const RefInGPU& refInGPU, const unsigned int width, const unsigned int height, const unsigned int mipmapLevels);
            virtual ~TextureInGPU();

            const unsigned int width;
            const unsigned int height;
            const unsigned int mipmapLevels;
            const float uTexelSizeN;
            const float vTexelSizeN;
            const float uHalfTexelSizeN;
            const float vHalfTexelSizeN;
        };

        class ArrayBufferInGPU : public ResourceInGPU
        {
            Q_DISABLE_COPY(ArrayBufferInGPU);
        private:
        protected:
        public:
            ArrayBufferInGPU(GPUAPI* api, const RefInGPU& refInGPU, const unsigned int itemsCount);
            virtual ~ArrayBufferInGPU();

            const unsigned int itemsCount;
        };

        union AtlasTypeId
        {
            uint64_t id;
            struct
            {
                TextureFormat format;
                uint16_t tileSize;
                uint16_t tilePadding;
            };

            inline operator uint64_t() const
            {
                return id;
            }

            inline AtlasTypeId& operator=( const uint64_t& that )
            {
                id = that;
                return *this;
            }

            inline bool operator==( const AtlasTypeId& that )
            {
                return this->id == that.id;
            }

            inline bool operator!=( const AtlasTypeId& that )
            {
                return this->id != that.id;
            }

            inline bool operator==( const uint64_t& that )
            {
                return this->id == that;
            }

            inline bool operator!=( const uint64_t& that )
            {
                return this->id != that;
            }
        };
        static_assert(sizeof(AtlasTypeId) == 8, "AtlasTypeId must be 8 bytes in size");

        class SlotOnAtlasTextureInGPU;
        class AtlasTextureInGPU;
        class AtlasTexturesPool
        {
            Q_DISABLE_COPY(AtlasTexturesPool);
        public:
            typedef std::function< AtlasTextureInGPU*() > AtlasTextureAllocatorSignature;
            typedef std::tuple< std::weak_ptr<AtlasTextureInGPU>, unsigned int > FreedSlotsEntry;
        private:
            mutable QMutex _freedSlotsMutex;
            QMultiHash< AtlasTextureInGPU*, FreedSlotsEntry> _freedSlots;

            mutable QMutex _unusedSlotsMutex;
            AtlasTextureInGPU* _lastNonFullAtlasTexture;
            std::weak_ptr<AtlasTextureInGPU> _lastNonFullAtlasTextureWeak;
            uint32_t _firstUnusedSlotIndex;
        protected:
            AtlasTexturesPool(GPUAPI* api, const AtlasTypeId& typeId);

            std::shared_ptr<SlotOnAtlasTextureInGPU> allocateTile(AtlasTextureAllocatorSignature atlasTextureAllocator);
        public:
            virtual ~AtlasTexturesPool();

            GPUAPI* const api;
            const AtlasTypeId typeId;

        friend OsmAnd::GPUAPI;
        };

        class AtlasTextureInGPU : public TextureInGPU
        {
            Q_DISABLE_COPY(AtlasTextureInGPU);
        private:
        protected:
#if OSMAND_DEBUG
            mutable QMutex _tilesMutex;
            QSet< SlotOnAtlasTextureInGPU* > _tiles;
#else
            QAtomicInt _tilesCounter;
#endif
        public:
            AtlasTextureInGPU(GPUAPI* api, const RefInGPU& refInGPU, const unsigned int textureSize, const unsigned int mipmapLevels, const std::shared_ptr<AtlasTexturesPool>& pool);
            virtual ~AtlasTextureInGPU();

            const unsigned int tileSize;
            const unsigned int padding;
            const unsigned int slotsPerSide;
            const float tileSizeN;
            const float tilePaddingN;

            const std::shared_ptr<AtlasTexturesPool> pool;

        friend OsmAnd::GPUAPI::SlotOnAtlasTextureInGPU;
        };

        class SlotOnAtlasTextureInGPU : public ResourceInGPU
        {
            Q_DISABLE_COPY(SlotOnAtlasTextureInGPU);
        private:
        protected:
        public:
            SlotOnAtlasTextureInGPU(const std::shared_ptr<AtlasTextureInGPU>& atlas, const uint32_t slotIndex);
            virtual ~SlotOnAtlasTextureInGPU();

            const std::shared_ptr<AtlasTextureInGPU> atlasTexture;
            const uint32_t slotIndex;
        };

    private:
#if OSMAND_DEBUG
        mutable QMutex _allocatedResourcesMutex;
        QList< ResourceInGPU* > _allocatedResources;
#else
        QAtomicInt _allocatedResourcesCounter;
#endif

        QHash< AtlasTypeId, std::shared_ptr<AtlasTexturesPool> > _atlasTexturesPools;
    protected:
        GPUAPI();

        std::shared_ptr<AtlasTexturesPool> obtainAtlasTexturesPool(const AtlasTypeId& atlasTypeId);
        std::shared_ptr<SlotOnAtlasTextureInGPU> allocateTile(const std::shared_ptr<AtlasTexturesPool>& pool, AtlasTexturesPool::AtlasTextureAllocatorSignature atlasTextureAllocator );

        virtual bool releaseResourceInGPU(const ResourceInGPU::Type type, const RefInGPU& refInGPU) = 0;

        bool _isSupported_8bitPaletteRGBA8;
    public:
        virtual ~GPUAPI();

        const bool& isSupported_8bitPaletteRGBA8;

        virtual bool initialize() = 0;
        virtual bool release() = 0;

        virtual bool uploadTileToGPU(const std::shared_ptr< const MapTile >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU) = 0;
        virtual bool uploadSymbolToGPU(const std::shared_ptr< const MapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU) = 0;

        virtual void waitUntilUploadIsComplete() = 0;

    friend OsmAnd::GPUAPI::ResourceInGPU;
    };
}

#endif // !defined(_OSMAND_CORE_GPU_API_H_)
