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
#include <QMutex>
#include <QThread>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <TileZoomCache.h>

namespace OsmAnd {

    class IMapTileProvider;
    class IMapElevationDataProvider;

    class OSMAND_CORE_API IMapRenderer
    {
    public:
        enum TextureDepth {
            _16bits,
            _32bits,
        };
        typedef std::function<void ()> RedrawRequestCallback;

        struct OSMAND_CORE_API Configuration
        {
            Configuration();

            std::shared_ptr<IMapTileProvider> tileProvider;
            std::shared_ptr<IMapElevationDataProvider> elevationDataProvider;
            PointI windowSize;
            float displayDensityFactor;
            AreaI viewport;
            float fieldOfView;
            float fogDistance;
            float azimuth;
            float elevationAngle;
            PointI target31;
            float requestedZoom;
            int zoomBase;
            float zoomFraction;
            TextureDepth preferredTextureDepth;
        };

    private:
        void tileReadyCallback(const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tile);
        bool _viewIsDirty;
        bool _tilesCacheInvalidated;
        bool _elevationDataCacheInvalidated;

        bool _isRenderingInitialized;

        Qt::HANDLE _renderThreadId;
    protected:
        IMapRenderer();

        virtual void invalidateConfiguration();
        QMutex _pendingToActiveConfigMutex;
        bool _configInvalidated;
        Configuration _pendingConfig;
        Configuration _activeConfig;

        QSet<TileId> _visibleTiles;
        PointF _targetInTile;
        
        QMutex _tilesCacheMutex;
        struct OSMAND_CORE_API CachedTile : TileZoomCache::Tile
        {
            CachedTile(const uint32_t& zoom, const TileId& id, const size_t& usedMemory);
            virtual ~CachedTile();
        };
        TileZoomCache _tilesCache;
        virtual void purgeTilesCache();
        void processPendingToCacheTiles();
        void obtainMissingTiles();
        virtual void cacheTile(const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap);

        virtual void uploadTileToTexture(const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap) = 0;
        
        void updateElevationDataCache();

        QMutex _tilesPendingToCacheMutex;
        struct OSMAND_CORE_API TilePendingToCache
        {
            TileId tileId;
            uint32_t zoom;
            std::shared_ptr<SkBitmap> tileBitmap;
        };
        QQueue< TilePendingToCache > _tilesPendingToCacheQueue;
        std::array< QSet< TileId >, 32 > _tilesPendingToCache;

        const bool& viewIsDirty;
        virtual void requestRedraw();
        
        const bool& tilesCacheInvalidated;
        virtual void invalidateTileCache();

        const bool& elevationDataCacheInvalidated;
        virtual void invalidateElevationDataCache();

        virtual void updateConfiguration();

        const Qt::HANDLE& renderThreadId;
    public:
        virtual ~IMapRenderer();

        RedrawRequestCallback redrawRequestCallback;
                
        const Configuration& configuration;
        const QSet<TileId>& visibleTiles;
        
        virtual int getCachedTilesCount() const;

        virtual void setTileProvider(const std::shared_ptr<IMapTileProvider>& tileProvider);
        virtual void setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& elevationDataProvider);
        virtual void setPreferredTextureDepth(TextureDepth depth);
        virtual void setWindowSize(const PointI& windowSize);
        virtual void setDisplayDensityFactor(const float& factor);
        virtual void setViewport(const AreaI& viewport);
        virtual void setFieldOfView(const float& fieldOfView);
        virtual void setDistanceToFog(const float& fogDistance);
        virtual void setAzimuth(const float& azimuth);
        virtual void setElevationAngle(const float& elevationAngle);
        virtual void setTarget(const PointI& target31);
        virtual void setZoom(const float& zoom);

        const bool& isRenderingInitialized;
        virtual void initializeRendering();
        virtual void performRendering();
        virtual void releaseRendering();
    };

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL createAtlasMapRenderer_OpenGL();
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
}

#endif // __I_MAP_RENDERER_H_