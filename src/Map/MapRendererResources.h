#ifndef _OSMAND_CORE_MAP_RENDERER_RESOURCES_H_
#define _OSMAND_CORE_MAP_RENDERER_RESOURCES_H_

#include "stdlib_common.h"
#include <functional>
#include <array>

#include "QtExtensions.h"
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
#include "SharedResourcesContainer.h"
#include "Concurrent.h"
#include "IQueryController.h"

namespace OsmAnd
{
    class MapRenderer;
    class IMapProvider;
    class MapSymbolsTile;
    class MapSymbolsGroup;
    class MapSymbol;

    class MapRendererResources
    {
        Q_DISABLE_COPY(MapRendererResources);
    public:
        enum class ResourceType
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
        // ... => Unavailable => JustBeforeDeath
        // ... => Ready => Uploading => Uploaded [=> IsBeingUsed] => UnloadPending => Unloading => Unloaded => JustBeforeDeath
        // ... => RequestCanceledWhileBeingProcessed => JustBeforeDeath
        enum class ResourceState
        {
            // Resource is not in any determined state (resource entry did not exist)
            Unknown = 0,

            // Resource is being requested
            Requesting,

            // Resource was requested and should arrive soon
            Requested,

            // Resource request is being processed
            ProcessingRequest,

            // Request was canceled while being processed
            RequestCanceledWhileBeingProcessed,

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
            virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController = nullptr) = 0;
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
            bool _isJunk;
        protected:
            BaseTiledResource(MapRendererResources* owner, const ResourceType type, const TilesCollection<BaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom);

            void markAsJunk();
        public:
            virtual ~BaseTiledResource();

            const bool& isJunk;

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

            virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController);
            virtual bool uploadToGPU();
            virtual void unloadFromGPU();
        public:
            virtual ~MapTileResource();

            const std::shared_ptr<const GPUAPI::ResourceInGPU>& resourceInGPU;

        friend class OsmAnd::MapRendererResources;
        };

        // Symbols-related:
        class SymbolsResourcesCollection;
        class SymbolsTileResource : public BaseTiledResource
        {
        private:
        protected:
            SymbolsTileResource(MapRendererResources* owner, const TilesCollection<BaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom);

            class GroupResources
            {
                Q_DISABLE_COPY(GroupResources);
            private:
            protected:
            public:
                GroupResources(const std::shared_ptr<const MapSymbolsGroup>& group);
                ~GroupResources();

                const std::shared_ptr<const MapSymbolsGroup> group;
                QHash< std::shared_ptr<const MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > resourcesInGPU;
            };

            std::shared_ptr<const MapSymbolsTile> _sourceData;
            QList< std::shared_ptr<GroupResources> > _uniqueGroupsResources;
            QList< std::shared_ptr<GroupResources> > _referencedSharedGroupsResources;

            virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController);
            virtual bool uploadToGPU();
            virtual void unloadFromGPU();

            virtual bool checkIsSafeToUnlink();
            virtual void detach();
        public:
            virtual ~SymbolsTileResource();

        friend class OsmAnd::MapRendererResources;
        friend class OsmAnd::MapRendererResources::SymbolsResourcesCollection;
        };
        class SymbolsResourcesCollection : public TiledResourcesCollection
        {
        private:
        protected:
            SymbolsResourcesCollection();

            std::array< SharedResourcesContainer<uint64_t, SymbolsTileResource::GroupResources>, ZoomLevelsCount > _sharedGroupsResources;
        public:
            virtual ~SymbolsResourcesCollection();

        friend class OsmAnd::MapRendererResources;
        friend class OsmAnd::MapRendererResources::SymbolsTileResource;
        };
        
        typedef QHash< std::shared_ptr<const MapSymbol>, std::weak_ptr<const GPUAPI::ResourceInGPU> > MapSymbolsLayer;
        typedef QMap<int, MapSymbolsLayer > MapSymbolsByOrder;

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

        // Symbols:
        mutable QMutex _mapSymbolsByOrderMutex;
        MapSymbolsByOrder _mapSymbolsByOrder;
        unsigned int _mapSymbolsCount;
        void addMapSymbol(const std::shared_ptr<const MapSymbol>& symbol, const std::shared_ptr<const GPUAPI::ResourceInGPU>& gpuResource);
        void removeMapSymbol(const std::shared_ptr<const MapSymbol>& symbol);

        void notifyNewResourceAvailableForDrawing();

        // Invalidated resources:
        uint32_t _invalidatedResourcesTypesMask;
        void invalidateResourcesOfType(const ResourceType type);
        void validateResources();
        void validateResourcesOfType(const ResourceType type);

        // Resources management:
        QSet<TileId> _activeTiles;
        ZoomLevel _activeZoom;
        void updateResources(const QSet<TileId>& tiles, const ZoomLevel zoom);
        void requestNeededResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom);
        void cleanupJunkResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom);
        unsigned int unloadResources();
        unsigned int uploadResources(const unsigned int limit = 0u, bool* const outMoreThanLimitAvailable = nullptr);
        void releaseResourcesFrom(const std::shared_ptr<TiledResourcesCollection>& collection);
        void requestResourcesUploadOrUnload();

        // Worker thread
        volatile bool _workerThreadIsAlive;
        const std::unique_ptr<Concurrent::Thread> _workerThread;
        Qt::HANDLE _workerThreadId;
        mutable QMutex _workerThreadWakeupMutex;
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

        QMutex& getSymbolsMapMutex() const;
        const MapSymbolsByOrder& getMapSymbolsByOrder() const;
        unsigned int getMapSymbolsCount() const;

        void dumpResourcesInfo() const;

    friend class OsmAnd::MapRenderer;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_RESOURCES_H_)
