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

#include <cstdint>
#include <memory>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>

#include <QList>
#include <QHash>
#include <QSet>
#include <QThreadPool>
#include <QReadWriteLock>
#include <QWaitCondition>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <MapTypes.h>
#include <Concurrent.h>
#include <IMapRenderer.h>
#include <RenderAPI.h>
#include <IMapTileProvider.h>
#include <TilesCollection.h>

namespace OsmAnd {

    class IMapProvider;
    class MapRendererTiledResources;
    class MapSymbol;

    class MapRenderer : public IMapRenderer
    {
    public:
        STRONG_ENUM_EX(ResourceType, int32_t)
        {
            Unknown = -1,

            // Elevation data
            ElevationData,

            // Raster map
            RasterMap,

            // Symbols
            Symbols,

            __LAST
        };
        enum {
            ResourceTypesCount = static_cast<int>(ResourceType::__LAST)
        };

        // Possible state chains:
        // Unknown => Requesting => Requested => ProcessingRequest => ...
        // ... => Unavailable.
        // ... => Ready => Uploading => Uploaded => Unloading => Unloaded.
        STRONG_ENUM(ResourceState)
        {
            // Resource is not in any determined state (resource entry did not exist)
            Unknown = 0,

            // Resource is being requested
            Requesting,

            // Resource was requested and should arrive soon
            Requested,

            // Resource request is being processed
            ProcessingRequest,

            // Resource is not available at all
            Unavailable,

            // Resource data is in main memory, but not yet uploaded to GPU
            Ready,

            // Resource data is being uploaded to GPU
            Uploading,

            // Resource data is already in GPU
            Uploaded,

            // Resource data is being removed from GPU
            Unloading,

            // Resource is unloaded, next state is "Dead"
            Unloaded,

            // JustBeforeDeath state is installed just before resource is deallocated completely
            JustBeforeDeath
        };

        class TiledResourceEntry : public TilesCollectionEntryWithState<TiledResourceEntry, ResourceState, ResourceState::Unknown>
        {
        private:
        protected:
            TiledResourceEntry(MapRenderer* owner, const ResourceType type, const TilesCollection<TiledResourceEntry>& collection, const TileId tileId, const ZoomLevel zoom);

            MapRenderer* const _owner;
            Concurrent::Task* _requestTask;

            virtual bool obtainData(bool& dataAvailable) = 0;
            virtual bool uploadToGPU() = 0;
            virtual void unloadFromGPU() = 0;
        public:
            virtual ~TiledResourceEntry();

            const ResourceType type;

        friend class OsmAnd::MapRenderer;
        };

        class TiledResources : public TilesCollection<TiledResourceEntry>
        {
        private:
        protected:
            void verifyNoUploadedTilesPresent();
        public:
            TiledResources(const ResourceType& type);
            virtual ~TiledResources();

            const MapRenderer::ResourceType type;

            virtual void removeAllEntries();

        friend class OsmAnd::MapRenderer;
        };

        class MapTileResourceEntry : public TiledResourceEntry
        {
        private:
        protected:
            std::shared_ptr<const MapTile> _sourceData;
            std::shared_ptr<RenderAPI::ResourceInGPU> _resourceInGPU;

            virtual bool obtainData(bool& dataAvailable);
            virtual bool uploadToGPU();
            virtual void unloadFromGPU();
        public:
            MapTileResourceEntry(MapRenderer* owner, const ResourceType type, const TilesCollection<TiledResourceEntry>& collection, const TileId tileId, const ZoomLevel zoom);
            virtual ~MapTileResourceEntry();

            const std::shared_ptr<const MapTile>& sourceData;
            const std::shared_ptr<RenderAPI::ResourceInGPU>& resourceInGPU;

        friend class OsmAnd::MapRenderer;
        };

        class SymbolsResourceEntry : public TiledResourceEntry
        {
        private:
        protected:
            QList< std::shared_ptr<const MapSymbol> > _sourceData;
            QList< std::shared_ptr<RenderAPI::ResourceInGPU> > _resourcesInGPU;

            virtual bool obtainData(bool& dataAvailable);
            virtual bool uploadToGPU();
            virtual void unloadFromGPU();
        public:
            SymbolsResourceEntry(MapRenderer* owner, const TilesCollection<TiledResourceEntry>& collection, const TileId tileId, const ZoomLevel zoom);
            virtual ~SymbolsResourceEntry();

            const QList< std::shared_ptr<const MapSymbol> >& sourceData;
            const QList< std::shared_ptr<RenderAPI::ResourceInGPU> >& resourcesInGPU;

        friend class OsmAnd::MapRenderer;
        };

        typedef std::array< QList< std::shared_ptr<TiledResources> >, ResourceTypesCount > ResourcesStorage;
    private:
        const Concurrent::TaskHost::Bridge _taskHostBridge;

        class TileRequestTask : public Concurrent::HostedTask
        {
            Q_DISABLE_COPY(TileRequestTask);
        private:
        protected:
        public:
            TileRequestTask(
                const std::shared_ptr<TiledResourceEntry>& requestedEntry,
                const Concurrent::TaskHost::Bridge& bridge, ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod = nullptr, PostExecuteSignature postExecuteMethod = nullptr);
            virtual ~TileRequestTask();

            const std::shared_ptr<TiledResourceEntry> requestedEntry;
        };

        // Configuration-related:
        mutable QReadWriteLock _configurationLock;
        MapRendererConfiguration _currentConfiguration;
        volatile uint32_t _currentConfigurationInvalidatedMask;
        void invalidateCurrentConfiguration(const uint32_t changesMask);
        bool updateCurrentConfiguration();

        // State-related:
        mutable QMutex _requestedStateMutex;
        mutable QReadWriteLock _internalStateLock;
        MapRendererState _currentState;
        volatile uint32_t _requestedStateUpdatedMask;
        enum class StateChange : uint32_t
        {
            RasterLayers_Providers = 1 << 0,
            RasterLayers_Opacity = 1 << 1,
            ElevationData_Provider = 1 << 2,
            ElevationData_ScaleFactor = 1 << 3,
            Symbols_Providers = 1 << 4,
            WindowSize = 1 << 5,
            Viewport = 1 << 6,
            FieldOfView = 1 << 7,
            SkyColor = 1 << 8,
            FogParameters = 1 << 9,
            Azimuth = 1 << 10,
            ElevationAngle = 1 << 11,
            Target = 1 << 12,
            Zoom = 1 << 13,
        };
        bool revalidateState();
        void notifyRequestedStateWasUpdated(const StateChange change);

        // Resources-related:
        QThreadPool _resourcesRequestWorkersPool;
        struct ProvidersAndSourcesBinding
        {
            QHash< std::shared_ptr<IMapProvider>, std::shared_ptr<TiledResources> > providersToResources;
            QHash< std::shared_ptr<TiledResources>, std::shared_ptr<IMapProvider> > resourcesToProviders;
        };
        std::array< ProvidersAndSourcesBinding, ResourceTypesCount > _providersAndResourcesBindings;
        void updateProvidersAndResourcesBindings(const uint32_t updatedMask);
        ResourcesStorage _resources;
        void requestNeededResources();
        void uploadResources();
        void cleanupJunkResources();
        void releaseResourcesFrom(const std::shared_ptr<TiledResources>& collection);
        void requestResourcesUpload();
        bool isDataSourceAvailableFor(const std::shared_ptr<TiledResources>& collection);
        bool obtainProviderFor(TiledResources* const resourcesRef, std::shared_ptr<IMapProvider>& provider);
        uint32_t _invalidatedResourcesTypesMask;
        void invalidateResourcesOfType(const ResourceType type);
        void validateResources();

        // General:
        QSet<TileId> _uniqueTiles;
        std::unique_ptr<RenderAPI> _renderAPI;
        void invalidateFrame();
        Qt::HANDLE _renderThreadId;
        Qt::HANDLE _workerThreadId;
        std::unique_ptr<Concurrent::Thread> _backgroundWorker;
        QWaitCondition _backgroundWorkerWakeup;
        void backgroundWorkerProcedure();
        
    protected:
        MapRenderer();

        // Configuration-related:
        const MapRendererConfiguration& currentConfiguration;
        enum ConfigurationChange
        {
            ColorDepthForcing = 1 << 0,
            AtlasTexturesUsage = 1 << 1,
            ElevationDataResolution = 1 << 2,
            TexturesFilteringMode = 1 << 3,
            PaletteTexturesUsage = 1 << 4,
        };
        virtual void validateConfigurationChange(const ConfigurationChange& change) = 0;

        // State-related:
        const MapRendererState& currentState;
        struct InternalState
        {
            InternalState();
            virtual ~InternalState();

            TileId targetTileId;
            PointF targetInTileOffsetN;
            QList<TileId> visibleTiles;
        };
        virtual const InternalState* getInternalStateRef() const = 0;
        virtual InternalState* getInternalStateRef() = 0;
        virtual bool updateInternalState(InternalState* internalState, const MapRendererState& state);

        // Resources-related:
        const std::array< ProvidersAndSourcesBinding, ResourceTypesCount >& providersAndResourcesBindings;
        const ResourcesStorage& resources;
        virtual void validateResourcesOfType(const ResourceType type);

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        virtual std::shared_ptr<const MapTile> prepareTileForUploadingToGPU(const std::shared_ptr<const MapTile>& tile);
        virtual uint32_t getTilesPerAtlasTextureLimit(const ResourceType resourceType, const std::shared_ptr<const MapTile>& tile) = 0;

        std::shared_ptr<RenderAPI::ResourceInGPU> _processingTileStub;
        std::shared_ptr<RenderAPI::ResourceInGPU> _unavailableTileStub;
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        // General:
        const std::unique_ptr<RenderAPI>& renderAPI;
        virtual RenderAPI* allocateRenderAPI() = 0;

        // Customization points:
        virtual bool preInitializeRendering();
        virtual bool doInitializeRendering();
        virtual bool postInitializeRendering();

        virtual bool prePrepareFrame();
        virtual bool doPrepareFrame();
        virtual bool postPrepareFrame();

        virtual bool preRenderFrame();
        virtual bool doRenderFrame() = 0;
        virtual bool postRenderFrame();

        virtual bool preProcessRendering();
        virtual bool doProcessRendering();
        virtual bool postProcessRendering();

        virtual bool preReleaseRendering();
        virtual bool doReleaseRendering();
        virtual bool postReleaseRendering();
    public:
        virtual ~MapRenderer();

        virtual bool setup(const MapRendererSetupOptions& setupOptions);

        // Configuration-related:
        virtual void setConfiguration(const MapRendererConfiguration& configuration, bool forcedUpdate = false);

        virtual bool initializeRendering();
        virtual bool prepareFrame();
        virtual bool renderFrame();
        virtual bool processRendering();
        virtual bool releaseRendering();

        virtual unsigned int getVisibleTilesCount() const;

        virtual void setRasterLayerProvider(const RasterMapLayerId layerId, const std::shared_ptr<IMapBitmapTileProvider>& tileProvider, bool forcedUpdate = false);
        virtual void resetRasterLayerProvider(const RasterMapLayerId layerId, bool forcedUpdate = false);
        virtual void setRasterLayerOpacity(const RasterMapLayerId layerId, const float opacity, bool forcedUpdate = false);
        virtual void setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& tileProvider, bool forcedUpdate = false);
        virtual void resetElevationDataProvider(bool forcedUpdate = false);
        virtual void setElevationDataScaleFactor(const float factor, bool forcedUpdate = false);
        virtual void addSymbolProvider(const std::shared_ptr<IMapSymbolProvider>& provider, bool forcedUpdate = false);
        virtual void removeSymbolProvider(const std::shared_ptr<IMapSymbolProvider>& provider, bool forcedUpdate = false);
        virtual void removeAllSymbolProviders(bool forcedUpdate = false);
        virtual void setWindowSize(const PointI& windowSize, bool forcedUpdate = false);
        virtual void setViewport(const AreaI& viewport, bool forcedUpdate = false);
        virtual void setFieldOfView(const float fieldOfView, bool forcedUpdate = false);
        virtual void setDistanceToFog(const float fogDistance, bool forcedUpdate = false);
        virtual void setFogOriginFactor(const float factor, bool forcedUpdate = false);
        virtual void setFogHeightOriginFactor(const float factor, bool forcedUpdate = false);
        virtual void setFogDensity(const float fogDensity, bool forcedUpdate = false);
        virtual void setFogColor(const FColorRGB& color, bool forcedUpdate = false);
        virtual void setSkyColor(const FColorRGB& color, bool forcedUpdate = false);
        virtual void setAzimuth(const float azimuth, bool forcedUpdate = false);
        virtual void setElevationAngle(const float elevationAngle, bool forcedUpdate = false);
        virtual void setTarget(const PointI& target31, bool forcedUpdate = false);
        virtual void setZoom(const float zoom, bool forcedUpdate = false);
    };

}

#endif // __MAP_RENDERER_H_