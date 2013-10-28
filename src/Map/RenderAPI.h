/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __RENDER_API_H_
#define __RENDER_API_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <QHash>
#include <QMultiMap>
#include <QReadWriteLock>
#include <QMutex>
#include <QSet>
#include <QAtomicInt>

#include <OsmAndCore.h>
#include <Common.h>
#include <CommonTypes.h>
#include <MapTypes.h>
#include <IMapTileProvider.h>

namespace OsmAnd {

    class OSMAND_CORE_API RenderAPI
    {
    public:
        typedef void* RefInGPU;

        class OSMAND_CORE_API ResourceInGPU
        {
        public:
            enum Type
            {
                Texture,
                TileOnAtlasTexture,
                ArrayBuffer,
            };
        private:
        protected:
            ResourceInGPU(const Type& type, RenderAPI* api, const RefInGPU& refInGPU);

            RefInGPU _refInGPU;
        public:
            virtual ~ResourceInGPU();

            RenderAPI* const api;
            const Type type;
            const RefInGPU& refInGPU;
        };

        class OSMAND_CORE_API TextureInGPU : public ResourceInGPU
        {
        private:
        protected:
        public:
            TextureInGPU(RenderAPI* api, const RefInGPU& refInGPU, const uint32_t textureSize, const uint32_t mipmapLevels);
            virtual ~TextureInGPU();

            const uint32_t textureSize;
            const uint32_t mipmapLevels;
            const float texelSizeN;
            const float halfTexelSizeN;
        };

        class OSMAND_CORE_API ArrayBufferInGPU : public ResourceInGPU
        {
        private:
        protected:
        public:
            ArrayBufferInGPU(RenderAPI* api, const RefInGPU& refInGPU, const uint32_t itemsCount);
            virtual ~ArrayBufferInGPU();

            const uint32_t itemsCount;
        };

        union AtlasTypeId
        {
            uint64_t id;
            struct
            {
                uint32_t format;
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

        class TileOnAtlasTextureInGPU;
        class AtlasTextureInGPU;
        class OSMAND_CORE_API AtlasTexturesPool
        {
        public:
            typedef std::function< AtlasTextureInGPU*() > AtlasTextureAllocatorSignature;
        private:
            mutable QMutex _freedSlotsMutex;
            QMultiMap< AtlasTextureInGPU*, std::tuple< std::weak_ptr<AtlasTextureInGPU>, uint32_t > > _freedSlots;

            mutable QMutex _unusedSlotsMutex;
            AtlasTextureInGPU* _lastNonFullAtlasTexture;
            std::weak_ptr<AtlasTextureInGPU> _lastNonFullAtlasTextureWeak;
            uint32_t _firstUnusedSlotIndex;
        protected:
            AtlasTexturesPool(RenderAPI* api, const AtlasTypeId& typeId);

            std::shared_ptr<TileOnAtlasTextureInGPU> allocateTile(AtlasTextureAllocatorSignature atlasTextureAllocator);
        public:
            virtual ~AtlasTexturesPool();

            RenderAPI* const api;
            const AtlasTypeId typeId;

            friend OsmAnd::RenderAPI;
        };

        class OSMAND_CORE_API AtlasTextureInGPU : public TextureInGPU
        {
        private:
        protected:
#if defined(DEBUG) || defined(_DEBUG)
            mutable QMutex _tilesMutex;
            QSet< TileOnAtlasTextureInGPU* > _tiles;
#else
            QAtomicInt _tilesCounter;
#endif
        public:
            AtlasTextureInGPU(RenderAPI* api, const RefInGPU& refInGPU, const uint32_t textureSize, const uint32_t mipmapLevels, const std::shared_ptr<AtlasTexturesPool>& pool);
            virtual ~AtlasTextureInGPU();

            const uint16_t tileSize;
            const uint16_t padding;
            const uint32_t slotsPerSide;
            const float tileSizeN;
            const float tilePaddingN;

            const std::shared_ptr<AtlasTexturesPool> pool;

        friend OsmAnd::RenderAPI::TileOnAtlasTextureInGPU;
        };

        class OSMAND_CORE_API TileOnAtlasTextureInGPU : public ResourceInGPU
        {
        private:
        protected:
        public:
            TileOnAtlasTextureInGPU(const std::shared_ptr<AtlasTextureInGPU>& atlas, const uint32_t slotIndex);
            virtual ~TileOnAtlasTextureInGPU();

            const std::shared_ptr<AtlasTextureInGPU> atlasTexture;
            const uint32_t slotIndex;
        };
    
    private:
#if defined(DEBUG) || defined(_DEBUG)
        mutable QMutex _allocatedResourcesMutex;
        QList< ResourceInGPU* > _allocatedResources;
#else
        QAtomicInt _allocatedResourcesCounter;
#endif

        QHash< AtlasTypeId, std::shared_ptr<AtlasTexturesPool> > _atlasTexturesPools;
    protected:
        std::shared_ptr<AtlasTexturesPool> obtainAtlasTexturesPool(const AtlasTypeId& atlasTypeId);
        std::shared_ptr<TileOnAtlasTextureInGPU> allocateTile(const std::shared_ptr<AtlasTexturesPool>& pool, AtlasTexturesPool::AtlasTextureAllocatorSignature atlasTextureAllocator );

        virtual bool releaseResourceInGPU(const ResourceInGPU::Type& type, const RefInGPU& refInGPU) = 0;

        bool _isSupported_8bitPaletteRGBA8;
    public:
        RenderAPI();
        virtual ~RenderAPI();

        const bool& isSupported_8bitPaletteRGBA8;

        virtual bool initialize() = 0;
        virtual bool release() = 0;

        // tilesPerAtlasTextureLimit:
        //   0 - unlimited
        //   1 - don't use atlas textures
        //   N - allow up to N*N tiles per atlas texture
        virtual bool uploadTileToGPU(const std::shared_ptr< const MapTile >& tile, const uint32_t tilesPerAtlasTextureLimit, std::shared_ptr< ResourceInGPU >& resourceInGPU) = 0;

    friend OsmAnd::RenderAPI::ResourceInGPU;
    };
}

#endif // __RENDER_API_H_
