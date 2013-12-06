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
#ifndef _OSMAND_CORE_MAP_RENDERER_RESOURCES_H_
#define _OSMAND_CORE_MAP_RENDERER_RESOURCES_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QHash>
#include <QSet>
#include <QThreadPool>
#include <QReadWriteLock>
#include <QWaitCondition>

#include "OsmAndCore.h"
#include "MapRendererState.h"
#include "GPUAPI.h"
#include "TilesCollection.h"
#include "Concurrent.h"

namespace OsmAnd
{
    class MapRenderer;
    class IMapProvider;
    class MapSymbolsTile;
    class MapSymbolsGroup;
    class MapSymbol;

    class MapRendererResources
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
        // ... => Ready => Uploading => Uploaded [=> IsBeingUsed] => UnloadPending => Unloading => Unloaded.
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

            // Resource data in GPU is being used
            IsBeingUsed,

            // Resource is no longer needed, and it needs to be unloaded from GPU
            UnloadPending,

            // Resource data is being removed from GPU
            Unloading,

            // Resource is unloaded, next state is "Dead"
            Unloaded,

            // JustBeforeDeath state is installed just before resource is deallocated completely
            JustBeforeDeath
        };

        // Generic interface that all resources must implement
        class IResource
        {
        protected:
            virtual bool obtainData(bool& dataAvailable) = 0;
            virtual bool uploadToGPU() = 0;
            virtual void unloadFromGPU() = 0;
        public:
            virtual ~IResource()
            {}

        friend class OsmAnd::MapRendererResources;
        };

        // Generic resource
        class GenericResource : public IResource
        {
        private:
        protected:
            GenericResource(MapRendererResources* owner, const ResourceType type);

            Concurrent::Task* _requestTask;
        public:
            virtual ~GenericResource();

            MapRendererResources* const owner;
            const ResourceType type;

        friend class OsmAnd::MapRendererResources;
        };

        // Base class for all tiled resources
        class BaseTiledResource
            : public GenericResource
            , public TilesCollectionEntryWithState<BaseTiledResource, ResourceState, ResourceState::Unknown>
        {
        private:
        protected:
            BaseTiledResource(MapRendererResources* owner, const ResourceType type, const TilesCollection<BaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom);
        public:
            virtual ~BaseTiledResource();

        friend class OsmAnd::MapRendererResources;
        };

        // Collection of tiled resources
        class TiledResourcesCollection : public TilesCollection<BaseTiledResource>
        {
        private:
        protected:
            TiledResourcesCollection(const ResourceType& type);

            void verifyNoUploadedTilesPresent() const;
            virtual void removeAllEntries();
        public:
            virtual ~TiledResourcesCollection();

            const MapRendererResources::ResourceType type;

        friend class OsmAnd::MapRendererResources;
        };

        typedef std::array< QList< std::shared_ptr<TiledResourcesCollection> >, ResourceTypesCount > TiledResourcesStorage;

        // Resource of map tile
        class MapTileResource : public BaseTiledResource
        {
        private:
        protected:
            MapTileResource(MapRendererResources* owner, const ResourceType type, const TilesCollection<BaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom);

            std::shared_ptr<const MapTile> _sourceData;
            std::shared_ptr<const GPUAPI::ResourceInGPU> _resourceInGPU;

            virtual bool obtainData(bool& dataAvailable);
            virtual bool uploadToGPU();
            virtual void unloadFromGPU();
        public:
            virtual ~MapTileResource();

            const std::shared_ptr<const GPUAPI::ResourceInGPU>& resourceInGPU;

        friend class OsmAnd::MapRendererResources;
        };

        // Symbols-related:
        class SymbolsTileResource;
        class SymbolsResourcesCollection : public TiledResourcesCollection
        {
        private:
        protected:
            SymbolsResourcesCollection();

            // Symbols groups cache
            struct CacheLevel
            {
                mutable QReadWriteLock _lock;
                QHash< uint64_t, std::weak_ptr< const MapSymbolsGroup > > _cache;
            };
            std::array<CacheLevel, ZoomLevelsCount> _cacheLevels;

            // GPU resources cache
            QMutex _gpuResourcesCacheMutex;
            QHash< std::shared_ptr<const MapSymbol>, std::weak_ptr<const GPUAPI::ResourceInGPU> > _gpuResourcesCache;
        public:
            virtual ~SymbolsResourcesCollection();

            friend class OsmAnd::MapRendererResources;
        friend class OsmAnd::MapRendererResources::SymbolsTileResource;
        };
        class SymbolsTileResource : public BaseTiledResource
        {
        private:
        protected:
            SymbolsTileResource(MapRendererResources* owner, const TilesCollection<BaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom);

            std::shared_ptr<const MapSymbolsTile> _sourceData;
            QList< std::shared_ptr<const MapSymbolsGroup> > _uniqueSymbolsGroups;
            QList< std::shared_ptr<const MapSymbolsGroup> > _sharedSymbolsGroups;
            QHash< std::shared_ptr<const MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > _uniqueResourcesInGPU;
            QHash< std::shared_ptr<const MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > _sharedResourcesInGPU;

            virtual void detach();

            virtual bool obtainData(bool& dataAvailable);
            virtual bool uploadToGPU();
            virtual void unloadFromGPU();
        public:
            virtual ~SymbolsTileResource();

        friend class OsmAnd::MapRendererResources;
        };

    private:
        // Resource-requests related:
        const Concurrent::TaskHost::Bridge _taskHostBridge;
        QThreadPool _resourcesRequestWorkersPool;
        class ResourceRequestTask : public Concurrent::HostedTask
        {
            Q_DISABLE_COPY(ResourceRequestTask);
        private:
        protected:
        public:
            ResourceRequestTask(
                const std::shared_ptr<IResource>& requestedResource,
                const Concurrent::TaskHost::Bridge& bridge, ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod = nullptr, PostExecuteSignature postExecuteMethod = nullptr);
            virtual ~ResourceRequestTask();

            const std::shared_ptr<IResource> requestedResource;
        };

        // Each provider has a binded resource collection, and these are bindings:
        struct Binding
        {
            QHash< std::shared_ptr<IMapProvider>, std::shared_ptr<TiledResourcesCollection> > providersToCollections;
            QHash< std::shared_ptr<TiledResourcesCollection>, std::shared_ptr<IMapProvider> > collectionsToProviders;
        };
        std::array< Binding, ResourceTypesCount > _bindings;
        bool obtainProviderFor(TiledResourcesCollection* const resourcesRef, std::shared_ptr<IMapProvider>& provider) const;
        bool isDataSourceAvailableFor(const std::shared_ptr<TiledResourcesCollection>& collection) const;

        // Resources storages:
        TiledResourcesStorage _storage;

        void notifyNewResourceAvailable();

        // Invalidated resources:
        uint32_t _invalidatedResourcesTypesMask;
        void invalidateResourcesOfType(const ResourceType type);
        void validateResources();
        void validateResourcesOfType(const ResourceType type);

        // Resources management:
        QSet<TileId> _activeTiles;
        ZoomLevel _activeZoom;
        void updateResources(const QSet<TileId>& tiles, const ZoomLevel zoom);
        void requestNeededResources(const QSet<TileId>& tiles, const ZoomLevel zoom);
        void cleanupJunkResources(const QSet<TileId>& tiles, const ZoomLevel zoom);
        unsigned int unloadResources();
        unsigned int uploadResources(const unsigned int limit = 0u, bool* const outMoreThanLimitAvailable = nullptr);
        void releaseResourcesFrom(const std::shared_ptr<TiledResourcesCollection>& collection);
        void requestResourcesUpload();

        // Worker thread
        volatile bool _workerThreadIsAlive;
        const std::unique_ptr<Concurrent::Thread> _workerThread;
        Qt::HANDLE _workerThreadId;
        QMutex _workerThreadWakeupMutex;
        QWaitCondition _workerThreadWakeup;
        void workerThreadProcedure();

        // Default resources
        bool initializeDefaultResources();
        bool releaseDefaultResources();
        std::shared_ptr<const GPUAPI::ResourceInGPU> _processingTileStub;
        std::shared_ptr<const GPUAPI::ResourceInGPU> _unavailableTileStub;
    protected:
        MapRendererResources(MapRenderer* const owner);

        // Resources management:
        bool uploadTileToGPU(const std::shared_ptr<const MapTile>& mapTile, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU);
        bool uploadSymbolToGPU(const std::shared_ptr<const MapSymbol>& mapSymbol, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU);

        void updateBindings(const MapRendererState& state, const uint32_t updatedMask);
        void updateActiveZone(const QSet<TileId>& tiles, const ZoomLevel zoom);
        void syncResourcesInGPU(
            const unsigned int limitUploads = 0u,
            bool* const outMoreUploadsThanLimitAvailable = nullptr,
            unsigned int* const outResourcesUploaded = nullptr,
            unsigned int* const outResourcesUnloaded = nullptr);
    public:
        ~MapRendererResources();

        MapRenderer* const renderer;

        // Default resources
        const std::shared_ptr<const GPUAPI::ResourceInGPU>& processingTileStub;
        const std::shared_ptr<const GPUAPI::ResourceInGPU>& unavailableTileStub;

        std::shared_ptr<const TiledResourcesCollection> getCollection(const ResourceType type, const std::shared_ptr<IMapProvider>& ofProvider) const;

    friend class OsmAnd::MapRenderer;
    };
}

#endif // _OSMAND_CORE_MAP_RENDERER_RESOURCES_H_
