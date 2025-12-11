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
#include "MapCommonTypes.h"
#include "IMapTiledDataProvider.h"
#include "MapRendererBaseResource.h"
#include "MapRenderer3DObjects.h"

namespace OsmAnd
{
    class MapSymbol;

    class GPUAPI
    {
        Q_DISABLE_COPY_AND_MOVE(GPUAPI);
    public:
        typedef const void* RefInGPU;
        typedef uint32_t TextureFormat;
        typedef uint32_t SourceFormat;

        class ResourceInGPU
        {
            Q_DISABLE_COPY_AND_MOVE(ResourceInGPU);
        public:
            enum class Type
            {
                Texture,
                SlotOnAtlasTexture,
                ArrayBuffer,
                ElementArrayBuffer,
                Mesh
            };
        private:
        protected:
            ResourceInGPU(const Type type, GPUAPI* api, const RefInGPU& refInGPU,
                const int64_t dateTimeFirst = 0, const int64_t dateTimeLast = 0,
                const int64_t dateTimePrevious = 0, const int64_t dateTimeNext = 0);

            mutable RefInGPU _refInGPU;
        public:
            virtual ~ResourceInGPU();

            GPUAPI* const api;
            const Type type;
            const RefInGPU& refInGPU;
            const int64_t dateTimeFirst;
            const int64_t dateTimeLast;
            const int64_t dateTimePrevious;
            const int64_t dateTimeNext;

            virtual void lostRefInGPU() const;
        };

        class MetaResourceInGPU : public ResourceInGPU
        {
            Q_DISABLE_COPY_AND_MOVE(MetaResourceInGPU);
        private:
        protected:
            MetaResourceInGPU(const Type type, GPUAPI* api);
        public:
            virtual ~MetaResourceInGPU();
        };

        class TextureInGPU : public ResourceInGPU
        {
            Q_DISABLE_COPY_AND_MOVE(TextureInGPU);
        private:
        protected:
        public:
            TextureInGPU(
                GPUAPI* api,
                const RefInGPU& refInGPU,
                const unsigned int width,
                const unsigned int height,
                const unsigned int mipmapLevels,
                const AlphaChannelType alphaChannelType,
                const int64_t dateTimeFirst = 0,
                const int64_t dateTimeLast = 0,
                const int64_t dateTimePrevious = 0,
                const int64_t dateTimeNext = 0);
            virtual ~TextureInGPU();

            const unsigned int width;
            const unsigned int height;
            const unsigned int mipmapLevels;
            const AlphaChannelType alphaChannelType;
            const float uTexelSizeN;
            const float vTexelSizeN;
            const float uHalfTexelSizeN;
            const float vHalfTexelSizeN;
        };

        class ArrayBufferInGPU : public ResourceInGPU
        {
            Q_DISABLE_COPY_AND_MOVE(ArrayBufferInGPU);
        private:
        protected:
        public:
            ArrayBufferInGPU(GPUAPI* api, const RefInGPU& refInGPU, const unsigned int itemsCount);
            virtual ~ArrayBufferInGPU();

            const unsigned int itemsCount;
        };

        class ElementArrayBufferInGPU : public ResourceInGPU
        {
            Q_DISABLE_COPY_AND_MOVE(ElementArrayBufferInGPU);
        private:
        protected:
        public:
            ElementArrayBufferInGPU(GPUAPI* api, const RefInGPU& refInGPU, const unsigned int itemsCount);
            virtual ~ElementArrayBufferInGPU();

            const unsigned int itemsCount;
        };

        struct MapRenderer3DBuildingGPUData
        {
            struct PerformanceDebugInfo
            {
                float obtainDataTimeMilliseconds;
                float uploadToGpuTimeMilliseconds;
            } _performanceDebugInfo;

            QSet<uint64_t> buildingIDs;
            std::shared_ptr<const ArrayBufferInGPU> vertexBuffer;
            std::shared_ptr<const ElementArrayBufferInGPU> indexBuffer;
            ZoomLevel zoom;
            TileId tileId;
            int referenceCount;
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

            inline AtlasTypeId& operator=(const uint64_t& that)
            {
                id = that;
                return *this;
            }

            inline bool operator==(const AtlasTypeId& that)
            {
                return this->id == that.id;
            }

            inline bool operator!=(const AtlasTypeId& that)
            {
                return this->id != that.id;
            }

            inline bool operator==(const uint64_t& that)
            {
                return this->id == that;
            }

            inline bool operator!=(const uint64_t& that)
            {
                return this->id != that;
            }
        };
        static_assert(sizeof(AtlasTypeId) == 8, "AtlasTypeId must be 8 bytes in size");

        class SlotOnAtlasTextureInGPU;
        class AtlasTextureInGPU;
        class AtlasTexturesPool
        {
            Q_DISABLE_COPY_AND_MOVE(AtlasTexturesPool);
        public:
            typedef std::function< AtlasTextureInGPU*() > AtlasTextureAllocator;
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

            std::shared_ptr<SlotOnAtlasTextureInGPU> allocateTile(
                const unsigned int tileSize,
                const AlphaChannelType alphaChannelType,
                const int64_t dateTimeFirst,
                const int64_t dateTimeLast,
                const int64_t dateTimePrevious,
                const int64_t dateTimeNext,
                AtlasTextureAllocator atlasTextureAllocator);
        public:
            virtual ~AtlasTexturesPool();

            GPUAPI* const api;
            const AtlasTypeId typeId;

        friend OsmAnd::GPUAPI;
        };

        class AtlasTextureInGPU : public TextureInGPU
        {
            Q_DISABLE_COPY_AND_MOVE(AtlasTextureInGPU);
        private:
        protected:
#if OSMAND_DEBUG
            mutable QMutex _tilesMutex;
            QSet< SlotOnAtlasTextureInGPU* > _tiles;
#else
            QAtomicInt _tilesCounter;
#endif
        public:
            AtlasTextureInGPU(
                GPUAPI* api,
                const RefInGPU& refInGPU,
                const unsigned int textureSize,
                const unsigned int mipmapLevels,
                const std::shared_ptr<AtlasTexturesPool>& pool);
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
            Q_DISABLE_COPY_AND_MOVE(SlotOnAtlasTextureInGPU);
        private:
        protected:
        public:
            SlotOnAtlasTextureInGPU(
                const std::shared_ptr<AtlasTextureInGPU>& atlas,
                const unsigned int slotIndex,
                const unsigned int tileSize,
                const AlphaChannelType alphaChannelType,
                const int64_t dateTimeFirst = 0,
                const int64_t dateTimeLast = 0,
                const int64_t dateTimePrevious = 0,
                const int64_t dateTimeMext = 0);
            virtual ~SlotOnAtlasTextureInGPU();

            const std::shared_ptr<AtlasTextureInGPU> atlasTexture;
            const unsigned int slotIndex;
            const unsigned int tileSize;
            const AlphaChannelType alphaChannelType;
        };

        class MeshInGPU : public MetaResourceInGPU
        {
            Q_DISABLE_COPY_AND_MOVE(MeshInGPU);
        private:
        protected:
        public:
            MeshInGPU(
                GPUAPI* api,
                const std::shared_ptr<ArrayBufferInGPU>& vertexBuffer,
                const std::shared_ptr<ElementArrayBufferInGPU>& indexBuffer,
                const std::shared_ptr<std::vector<std::pair<TileId, int32_t>>>& partSizes,
                const ZoomLevel zoomLevel,
                const bool isDenseObject,
                const PointI* position31 = nullptr);

            virtual ~MeshInGPU();

            const std::shared_ptr<ArrayBufferInGPU> vertexBuffer;
            const std::shared_ptr<ElementArrayBufferInGPU> indexBuffer;
            const std::shared_ptr<std::vector<std::pair<TileId, int32_t>>> partSizes;
            const ZoomLevel zoomLevel;
            const PointI* position31;
            const bool isDenseObject;

            void lostRefInGPU() const override;
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
        std::shared_ptr<SlotOnAtlasTextureInGPU> allocateTileInAltasTexture(
            const unsigned int tileSize,
            const AlphaChannelType alphaChannelType,
            const int64_t dateTimeFirst,
            const int64_t dateTimeLast,
            const int64_t dateTimePrevious,
            const int64_t dateTimeNext,
            const std::shared_ptr<AtlasTexturesPool>& pool,
            AtlasTexturesPool::AtlasTextureAllocator atlasTextureAllocator);

        virtual bool releaseResourceInGPU(const ResourceInGPU::Type type, const RefInGPU& refInGPU) = 0;
    public:
        virtual ~GPUAPI();

        virtual bool initialize() = 0;
        virtual int checkElementVisibility(int queryIndex, float pointSize) = 0;
        virtual bool elementIsVisible(int queryIndex) = 0;
        virtual bool release(bool gpuContextLost) = 0;

        virtual bool uploadTiledDataToGPU(
            const std::shared_ptr< const IMapTiledDataProvider::Data >& tile,
            std::shared_ptr< const ResourceInGPU >& resourceInGPU,
            bool waitForGPU,
            volatile bool* gpuContextLost,
            int64_t dateTime = 0,
            const std::shared_ptr<MapRendererBaseResource>& resource = nullptr) = 0;
        virtual bool uploadSymbolToGPU(
            const std::shared_ptr< const MapSymbol >& symbol,
            std::shared_ptr< const ResourceInGPU >& resourceInGPU,
            bool waitForGPU,
            volatile bool* gpuContextLost) = 0;

        virtual bool uploadDataAsVertices(
            const void* data,
            const size_t dataSize,
            const unsigned int vertexCount,
            std::shared_ptr< const ArrayBufferInGPU >& outVertexBuffer,
            bool waitForGPU,
            volatile bool* gpuContextLost) = 0;

        virtual bool uploadDataAsIndices(
            const void* data,
            const size_t dataSize,
            const unsigned int indexCount,
            std::shared_ptr< const ElementArrayBufferInGPU >& outIndexBuffer,
            bool waitForGPU,
            volatile bool* gpuContextLost) = 0;

        virtual void waitUntilUploadIsComplete(volatile bool* gpuContextLost) = 0;

        virtual AlphaChannelType getGpuResourceAlphaChannelType(const std::shared_ptr<const ResourceInGPU> gpuResource);
        float getGpuResourceTexelSize(const std::shared_ptr<const ResourceInGPU> gpuResource);

        QAtomicInt waitTimeInMicroseconds;

    friend OsmAnd::GPUAPI::ResourceInGPU;
    };
}

#endif // !defined(_OSMAND_CORE_GPU_API_H_)
