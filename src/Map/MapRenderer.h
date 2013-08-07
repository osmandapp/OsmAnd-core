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
#ifndef __MAP_RENDERER_H_
#define __MAP_RENDERER_H_

#include <stdint.h>
#include <memory>
#include <functional>
#include <array>

#include <QReadWriteLock>
#include <QWaitCondition>
#include <QSet>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <MapTypes.h>
#include <Concurrent.h>
#include <IMapRenderer.h>
#include <IMapTileProvider.h>

namespace OsmAnd {

    class RenderAPI;
    class MapRendererTiledResources;
    class OSMAND_CORE_API MapRenderer : public IMapRenderer
    {
    public:
        enum TiledResourceType : int32_t
        {
            InvalidId = -1,

            // Elevation data
            ElevationData,

            // Raster layers (MapRasterLayersCount)
            RasterBaseLayer,
            __RasterLayer_LAST = RasterBaseLayer + (RasterMapLayersCount-1),

            __LAST
        };
        enum {
            TiledResourceTypesCount = static_cast<unsigned>(TiledResourceType::__LAST)
        };
    private:
        QReadWriteLock _configurationLock;
        MapRendererConfiguration _currentConfiguration;
        volatile uint32_t _currentConfigurationInvalidatedMask;

        QReadWriteLock _stateLock;
        MapRendererState _currentState;
        volatile bool _currentStateInvalidated;

        volatile uint32_t _invalidatedRasterLayerResourcesMask;
        QReadWriteLock _invalidatedRasterLayerResourcesMaskLock;
        
        volatile bool _invalidatedElevationDataResources;

        std::array< std::unique_ptr<MapRendererTiledResources>, TiledResourceTypesCount > _tiledResources;
        void uploadTiledResources();
        void releaseTiledResources(const std::unique_ptr<MapRendererTiledResources>& collection);
        IMapTileProvider* getTileProviderFor(const TiledResourceType& resourceType) const;

        QSet<TileId> _uniqueTiles;

        std::unique_ptr<RenderAPI> _renderAPI;
    protected:
        MapRenderer();

        const MapRendererConfiguration& currentConfiguration;
        enum ConfigurationChange {
            ColorDepthForcing = 1 << 0,
            AtlasTexturesUsage = 1 << 1,
            ElevationDataResolution = 1 << 2,
            TexturesFilteringMode = 1 << 3,
            PaletteTexturesUsage = 1 << 4,
        };
        void invalidateCurrentConfiguration(const uint32_t& changesMask);
        virtual void validateConfigurationChange(const ConfigurationChange& change) = 0;
        bool updateCurrentConfiguration();

        virtual bool preInitializeRendering();
        virtual bool doInitializeRendering();
        virtual bool postInitializeRendering();

        virtual bool preProcessRendering();
        virtual bool doProcessRendering();
        virtual bool postProcessRendering();

        virtual bool preRenderFrame();
        virtual bool doRenderFrame() = 0;
        virtual bool postRenderFrame();

        virtual bool prePostprocessRendering();
        virtual bool doPostprocessRendering();
        virtual bool postPostprocessRendering();

        virtual bool preReleaseRendering();
        virtual bool doReleaseRendering();
        virtual bool postReleaseRendering();

        const MapRendererState& currentState;
        void invalidateCurrentState();
        virtual bool updateCurrentState();

        TileId _targetTileId;
        PointF _targetInTileOffsetN;

        void invalidateRasterLayerResources(const RasterMapLayerId& layerId);
        virtual void validateRasterLayerResources(const RasterMapLayerId& layerId);

        void invalidateElevationDataResources();
        virtual void validateElevationDataResources();

        void invalidateFrame();

        void requestUploadDataToGPU();

        const std::array< std::unique_ptr<MapRendererTiledResources>, TiledResourceTypesCount >& tiledResources;
        void requestMissingTiledResources();
        virtual std::shared_ptr<IMapTileProvider::Tile> prepareTileForUploadingToGPU(const std::shared_ptr<IMapTileProvider::Tile>& tile);
        virtual uint32_t getTilesPerAtlasTextureLimit(const TiledResourceType& resourceType, const std::shared_ptr<IMapTileProvider::Tile>& tile) = 0;
        void processRequestedTile(const TiledResourceType& resourceType, const TileId& tileId, const ZoomLevel& zoom, const std::shared_ptr<IMapTileProvider::Tile>& tile, bool success);

        Qt::HANDLE _renderThreadId;
        Qt::HANDLE _workerThreadId;

        std::unique_ptr<Concurrent::Thread> _backgroundWorker;
        QWaitCondition _backgroundWorkerWakeup;
        void backgroundWorkerProcedure();

        const std::unique_ptr<RenderAPI>& renderAPI;
        virtual RenderAPI* allocateRenderAPI() = 0;
    public:
        virtual ~MapRenderer();

        virtual bool setup(const MapRendererSetupOptions& setupOptions);

        virtual void setConfiguration(const MapRendererConfiguration& configuration, bool forcedUpdate = false);

        virtual bool initializeRendering();
        virtual bool processRendering();
        virtual bool renderFrame();
        virtual bool postprocessRendering();
        virtual bool releaseRendering();

        virtual void setRasterLayerProvider(const RasterMapLayerId& layerId, const std::shared_ptr<IMapBitmapTileProvider>& tileProvider, bool forcedUpdate = false);
        virtual void setRasterLayerOpacity(const RasterMapLayerId& layerId, const float& opacity, bool forcedUpdate = false);
        virtual void setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& tileProvider, bool forcedUpdate = false);
        virtual void setElevationDataScaleFactor(const float& factor, bool forcedUpdate = false);
        virtual void setWindowSize(const PointI& windowSize, bool forcedUpdate = false);
        virtual void setViewport(const AreaI& viewport, bool forcedUpdate = false);
        virtual void setFieldOfView(const float& fieldOfView, bool forcedUpdate = false);
        virtual void setDistanceToFog(const float& fogDistance, bool forcedUpdate = false);
        virtual void setFogOriginFactor(const float& factor, bool forcedUpdate = false);
        virtual void setFogHeightOriginFactor(const float& factor, bool forcedUpdate = false);
        virtual void setFogDensity(const float& fogDensity, bool forcedUpdate = false);
        virtual void setFogColor(const FColorRGB& color, bool forcedUpdate = false);
        virtual void setSkyColor(const FColorRGB& color, bool forcedUpdate = false);
        virtual void setAzimuth(const float& azimuth, bool forcedUpdate = false);
        virtual void setElevationAngle(const float& elevationAngle, bool forcedUpdate = false);
        virtual void setTarget(const PointI& target31, bool forcedUpdate = false);
        virtual void setZoom(const float& zoom, bool forcedUpdate = false);
    };

}

#endif // __MAP_RENDERER_H_