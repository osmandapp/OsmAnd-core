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
#ifndef _OSMAND_CORE_RENDER_API_H_
#define _OSMAND_CORE_RENDER_API_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
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

    class RenderAPI
    {
    public:
        typedef void* RefInGPU;

        class ResourceInGPU
        {
        public:
            enum class Type
            {
                Texture,
                SlotOnAtlasTexture,
                ArrayBuffer,
            };
        private:
        protected:
            ResourceInGPU(const Type type, RenderAPI* api, const RefInGPU& refInGPU);

            RefInGPU _refInGPU;
        public:
            virtual ~ResourceInGPU();

            RenderAPI* const api;
            const Type type;
            const RefInGPU& refInGPU;
        };

        class TextureInGPU : public ResourceInGPU
        {
        private:
        protected:
        public:
            TextureInGPU(RenderAPI* api, const RefInGPU& refInGPU, const unsigned int textureSize, const unsigned int mipmapLevels);
            virtual ~TextureInGPU();

            const unsigned int textureSize;
            const unsigned int mipmapLevels;
            const float texelSizeN;
            const float halfTexelSizeN;
        };

        class ArrayBufferInGPU : public ResourceInGPU
        {
        private:
        protected:
        public:
            ArrayBufferInGPU(RenderAPI* api, const RefInGPU& refInGPU, const unsigned int itemsCount);
            virtual ~ArrayBufferInGPU();

            const unsigned int itemsCount;
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

        class SlotOnAtlasTextureInGPU;
        class AtlasTextureInGPU;
        class AtlasTexturesPool
        {
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
            AtlasTexturesPool(RenderAPI* api, const AtlasTypeId& typeId);

            std::shared_ptr<SlotOnAtlasTextureInGPU> allocateTile(AtlasTextureAllocatorSignature atlasTextureAllocator);
        public:
            virtual ~AtlasTexturesPool();

            RenderAPI* const api;
            const AtlasTypeId typeId;

        friend OsmAnd::RenderAPI;
        };

        class AtlasTextureInGPU : public TextureInGPU
        {
        private:
        protected:
#if defined(DEBUG) || defined(_DEBUG)
            mutable QMutex _tilesMutex;
            QSet< SlotOnAtlasTextureInGPU* > _tiles;
#else
            QAtomicInt _tilesCounter;
#endif
        public:
            AtlasTextureInGPU(RenderAPI* api, const RefInGPU& refInGPU, const unsigned int textureSize, const unsigned int mipmapLevels, const std::shared_ptr<AtlasTexturesPool>& pool);
            virtual ~AtlasTextureInGPU();

            const unsigned int tileSize;
            const unsigned int padding;
            const unsigned int slotsPerSide;
            const float tileSizeN;
            const float tilePaddingN;

            const std::shared_ptr<AtlasTexturesPool> pool;

        friend OsmAnd::RenderAPI::SlotOnAtlasTextureInGPU;
        };

        class SlotOnAtlasTextureInGPU : public ResourceInGPU
        {
        private:
        protected:
        public:
            SlotOnAtlasTextureInGPU(const std::shared_ptr<AtlasTextureInGPU>& atlas, const uint32_t slotIndex);
            virtual ~SlotOnAtlasTextureInGPU();

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
        RenderAPI();

        std::shared_ptr<AtlasTexturesPool> obtainAtlasTexturesPool(const AtlasTypeId& atlasTypeId);
        std::shared_ptr<SlotOnAtlasTextureInGPU> allocateTile(const std::shared_ptr<AtlasTexturesPool>& pool, AtlasTexturesPool::AtlasTextureAllocatorSignature atlasTextureAllocator );

        virtual bool releaseResourceInGPU(const ResourceInGPU::Type type, const RefInGPU& refInGPU) = 0;

        bool _isSupported_8bitPaletteRGBA8;
    public:
        virtual ~RenderAPI();

        const bool& isSupported_8bitPaletteRGBA8;

        virtual bool initialize() = 0;
        virtual bool release() = 0;

        virtual bool uploadTileToGPU(const std::shared_ptr< const MapTile >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU) = 0;
        virtual bool uploadSymbolToGPU(const std::shared_ptr< const MapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU) = 0;

    friend OsmAnd::RenderAPI::ResourceInGPU;
    };
}

#endif // _OSMAND_CORE_RENDER_API_H_
