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
#ifndef __I_MAP_RENDERER_H_
#define __I_MAP_RENDERER_H_

#include <stdint.h>
#include <memory>
#include <functional>
#include <array>

#include <QQueue>
#include <QSet>
#include <QMap>
#include <QMultiMap>
#include <QMutex>
#include <QThread>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <TileZoomCache.h>
#include <IMapTileProvider.h>

namespace OsmAnd {

    class OSMAND_CORE_API IMapRenderer
    {
    public:
        enum TileLayerId : int
        {
            Invalid = -1,

            ElevationData,
            RasterMap,
            MapOverlay0,
            MapOverlay1,
            MapOverlay2,
            MapOverlay3,

            IdsCount,
        };

        typedef std::function<void ()> RedrawRequestCallback;

        struct OSMAND_CORE_API Configuration
        {
            Configuration();

            std::array< std::shared_ptr<IMapTileProvider>, TileLayerId::IdsCount > tileProviders;
            PointI windowSize;
            float displayDensityFactor;
            AreaI viewport;
            float fieldOfView;
            float skyColor[3];
            float fogColor[3];
            float fogDistance;
            float fogOriginFactor;
            float fogDensity;
            float azimuth;
            float elevationAngle;
            PointI target31;
            float requestedZoom;
            int zoomBase;
            float zoomFraction;

            bool force16bitColorDepthLimit;
            bool textureAtlasesAllowed;
            uint32_t heightmapPatchesPerSide;
        };

    private:
        void handleProvidedTile(const TileLayerId& layerId, const TileId& tileId, uint32_t zoom, const std::shared_ptr<IMapTileProvider::Tile>& tile, bool success);

        QMutex _tileLayersCacheInvalidatedMaskMutex;
        volatile uint32_t _tileLayersCacheInvalidatedMask;

        volatile bool _isRenderingInitialized;

        Qt::HANDLE _renderThreadId;

        void purgeTileLayerCache(const TileLayerId& layerId);
        void cacheTile(TileLayerId layerId, const TileId& tileId, uint32_t zoom, const std::shared_ptr<IMapTileProvider::Tile>& tile);

        void validateConfiguration();
    protected:
        IMapRenderer();

        enum {
            MaxTilesPerTextureAtlasSide = 16,
        };

        virtual void invalidateConfiguration();
        volatile bool _configInvalidated;
        QMutex _pendingConfigModificationMutex;
        Configuration _pendingConfig;
        Configuration _activeConfig;

        QSet<TileId> _visibleTiles;
        PointI _targetTile;
        PointF _targetInTilePosN;
        
        struct OSMAND_CORE_API PendingToCacheTile
        {
            PendingToCacheTile(IMapRenderer* const renderer, TileLayerId layerId, const uint32_t& zoom, const TileId& tileId, const std::shared_ptr<IMapTileProvider::Tile>& tile);

            IMapRenderer* const renderer;
            const TileLayerId layerId;

            const TileId tileId;
            const uint32_t zoom;
            const std::shared_ptr<IMapTileProvider::Tile> tile;
        };
        struct OSMAND_CORE_API CachedTile : TileZoomCache::Tile
        {
            CachedTile(IMapRenderer* const renderer, TileLayerId layerId, const uint32_t& zoom, const TileId& id, const uint64_t& atlasPoolId, void* textureRef, int atlasSlotIndex, const size_t& usedMemory);
            virtual ~CachedTile();

            IMapRenderer* const renderer;
            const TileLayerId layerId;

            const uint64_t atlasPoolId;

            void* const textureRef;
            const int atlasSlotIndex;
        };
        struct OSMAND_CORE_API AtlasTexturePool
        {
            AtlasTexturePool();

            uint32_t _textureSize;
            uint32_t _padding;
            void* _lastNonFullTextureRef;
            int _firstFreeSlotIndex;
            QMultiMap< void*, int > _freedSlots;

            float _tileSizeN;
            float _tilePaddingN;
            uint32_t _slotsPerSide;
            uint32_t _mipmapLevels;
        };
        struct OSMAND_CORE_API TileLayer
        {
            TileLayer();

            QMutex _cacheModificationMutex;
            TileZoomCache _cache;

            QMap< uint64_t, AtlasTexturePool > _atlasTexturePools;
            QMap< void*, uint32_t > _textureRefCount;

            QMutex _pendingToCacheMutex;
            QQueue< PendingToCacheTile > _pendingToCacheQueue;
            std::array< QSet< TileId >, 32 > _pendingToCache;

            QMutex _requestedTilesMutex;
            std::array< QSet< TileId >, 32 > _requestedTiles;

            void purgeCache();
            bool uploadPending();
        };
        std::array< TileLayer, TileLayerId::IdsCount > _tileLayers;
        void requestCacheMissTiles();
        
        virtual void uploadTileToTexture(TileLayerId layerId, const TileId& tileId, uint32_t zoom, const std::shared_ptr<IMapTileProvider::Tile>& tile, uint64_t& atlasPoolId, void*& textureRef, int& atlasSlotIndex, size_t& usedMemory) = 0;
        virtual void releaseTexture(void* textureRef) = 0;

        virtual void requestRedraw();
        
        const volatile uint32_t& tilesCacheInvalidatedMask;
        virtual void invalidateTileLayerCache(const TileLayerId& layerId);
        void invalidateTileLayersCache();
        virtual void validateTileLayerCache(const TileLayerId& layerId);

        virtual void updateConfiguration();

        const Qt::HANDLE& renderThreadId;
    public:
        virtual ~IMapRenderer();

        RedrawRequestCallback redrawRequestCallback;
                
        const Configuration& configuration;
        const QSet<TileId>& visibleTiles;
        
        virtual void setTileProvider(const TileLayerId& layerId, const std::shared_ptr<IMapTileProvider>& tileProvider, bool forcedUpdate = false);
        virtual void set16bitColorDepthLimit(const bool& forced, bool forcedUpdate = false);
        virtual void setWindowSize(const PointI& windowSize, bool forcedUpdate = false);
        virtual void setDisplayDensityFactor(const float& factor, bool forcedUpdate = false);
        virtual void setViewport(const AreaI& viewport, bool forcedUpdate = false);
        virtual void setFieldOfView(const float& fieldOfView, bool forcedUpdate = false);
        virtual void setDistanceToFog(const float& fogDistance, bool forcedUpdate = false);
        virtual void setFogOriginFactor(const float& factor, bool forcedUpdate = false);
        virtual void setFogDensity(const float& fogDensity, bool forcedUpdate = false);
        virtual void setFogColor(const float& r, const float& g, const float& b, bool forcedUpdate = false);
        virtual void setSkyColor(const float& r, const float& g, const float& b, bool forcedUpdate = false);
        virtual void setAzimuth(const float& azimuth, bool forcedUpdate = false);
        virtual void setElevationAngle(const float& elevationAngle, bool forcedUpdate = false);
        virtual void setTarget(const PointI& target31, bool forcedUpdate = false);
        virtual void setZoom(const float& zoom, bool forcedUpdate = false);
        virtual void setTextureAtlasesUsagePermit(const bool& allow, bool forcedUpdate = false);
        virtual void setHeightmapPatchesPerSide(const uint32_t& patchesCount, bool forcedUpdate = false);

        const volatile bool& isRenderingInitialized;
        virtual void initializeRendering();
        virtual void performRendering();
        virtual void releaseRendering();
    };

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL createAtlasMapRenderer_OpenGL();
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
}

#endif // __I_MAP_RENDERER_H_