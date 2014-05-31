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
#include "TiledEntriesCollection.h"
#include "KeyedEntriesCollection.h"
#include "SharedResourcesContainer.h"
#include "Concurrent.h"
#include "IQueryController.h"

namespace OsmAnd
{
    class MapRenderer;
    class IMapDataProvider;
    class MapTiledSymbols;
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

        // Base resource
        class BaseResource
        {
        private:
            bool _isJunk;
        protected:
            BaseResource(MapRendererResources* owner, const ResourceType type);

            Concurrent::Task* _requestTask;

            void markAsJunk();

            virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController = nullptr) = 0;
            virtual bool uploadToGPU() = 0;
            virtual void unloadFromGPU() = 0;

            virtual void removeSelfFromCollection() = 0;
        public:
            virtual ~BaseResource();

            MapRendererResources* const owner;
            const ResourceType type;

            const bool& isJunk;

            virtual ResourceState getState() const = 0;
            virtual void setState(const ResourceState newState) = 0;
            virtual bool setStateIf(const ResourceState testState, const ResourceState newState) = 0;

        friend class OsmAnd::MapRendererResources;
        };

        // Base class for all tiled resources
        class BaseTiledResource
            : public BaseResource
            , public TiledEntriesCollectionEntryWithState<BaseTiledResource, ResourceState, ResourceState::Unknown>
        {
            typedef TiledEntriesCollectionEntryWithState<BaseTiledResource, ResourceState, ResourceState::Unknown> BaseTilesCollectionEntryWithState;

        private:
        protected:
            BaseTiledResource(
                MapRendererResources* owner,
                const ResourceType type,
                const TiledEntriesCollection<BaseTiledResource>& collection,
                const TileId tileId,
                const ZoomLevel zoom);

            virtual void removeSelfFromCollection();
        public:
            virtual ~BaseTiledResource();

            virtual ResourceState getState() const;
            virtual void setState(const ResourceState newState);
            virtual bool setStateIf(const ResourceState testState, const ResourceState newState);

        friend class OsmAnd::MapRendererResources;
        };

        // Base class for all keyed resources
        class BaseKeyedResource
            : public BaseResource
            , public KeyedEntriesCollectionEntryWithState<const void*, BaseKeyedResource, ResourceState, ResourceState::Unknown>
        {
            typedef const void* Key;
            typedef KeyedEntriesCollectionEntryWithState<Key, BaseKeyedResource, ResourceState, ResourceState::Unknown> BaseKeyedEntriesCollectionEntryWithState;

        private:
        protected:
            BaseKeyedResource(
                MapRendererResources* owner,
                const ResourceType type,
                const KeyedEntriesCollection<Key, BaseKeyedResource>& collection,
                const Key key);

            virtual void removeSelfFromCollection();
        public:
            virtual ~BaseKeyedResource();

            virtual ResourceState getState() const;
            virtual void setState(const ResourceState newState);
            virtual bool setStateIf(const ResourceState testState, const ResourceState newState);

        friend class OsmAnd::MapRendererResources;
        };

        // Base Collection of resources
        class BaseResourcesCollection
        {
        public:
            typedef std::function<void (const std::shared_ptr<BaseResource>& resource, bool& cancel)> ResourceActionCallback;
            typedef std::function<bool (const std::shared_ptr<BaseResource>& resource, bool& cancel)> ResourceFilterCallback;

        private:
        protected:
            BaseResourcesCollection(const ResourceType& type);
        public:
            virtual ~BaseResourcesCollection();

            const MapRendererResources::ResourceType type;

            virtual int getResourcesCount() const = 0;
            virtual void forEachResourceExecute(const ResourceActionCallback action) = 0;
            virtual void obtainResources(QList< std::shared_ptr<BaseResource> >* outList, const ResourceFilterCallback filter) = 0;
            virtual void removeResources(const ResourceFilterCallback filter) = 0;

        friend class OsmAnd::MapRendererResources;
        };
        typedef std::array< QList< std::shared_ptr<BaseResourcesCollection> >, ResourceTypesCount > ResourcesStorage;

        // Tiled resources collection
        class TiledResourcesCollection
            : public BaseResourcesCollection
            , public TiledEntriesCollection<BaseTiledResource>
        {
        private:
        protected:
            TiledResourcesCollection(const ResourceType& type);

            void verifyNoUploadedResourcesPresent() const;
            virtual void removeAllEntries();
        public:
            virtual ~TiledResourcesCollection();

            virtual int getResourcesCount() const;
            virtual void forEachResourceExecute(const ResourceActionCallback action);
            virtual void obtainResources(QList< std::shared_ptr<BaseResource> >* outList, const ResourceFilterCallback filter);
            virtual void removeResources(const ResourceFilterCallback filter);

        friend class OsmAnd::MapRendererResources;
        };

        // Map tile:
        class MapTileResource : public BaseTiledResource
        {
        private:
        protected:
            MapTileResource(MapRendererResources* owner, const ResourceType type, const TiledEntriesCollection<BaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom);

            std::shared_ptr<const MapTiledData> _sourceData;
            std::shared_ptr<const GPUAPI::ResourceInGPU> _resourceInGPU;

            virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController);
            virtual bool uploadToGPU();
            virtual void unloadFromGPU();
        public:
            virtual ~MapTileResource();

            const std::shared_ptr<const GPUAPI::ResourceInGPU>& resourceInGPU;

        friend class OsmAnd::MapRendererResources;
        };

        // Keyed resources collection:
        class KeyedResourcesCollection
            : public BaseResourcesCollection
            , public KeyedEntriesCollection<const void*, BaseKeyedResource>
        {
        private:
        protected:
            KeyedResourcesCollection(const ResourceType& type);

            void verifyNoUploadedResourcesPresent() const;
            virtual void removeAllEntries();
        public:
            virtual ~KeyedResourcesCollection();

            virtual int getResourcesCount() const;
            virtual void forEachResourceExecute(const ResourceActionCallback action);
            virtual void obtainResources(QList< std::shared_ptr<BaseResource> >* outList, const ResourceFilterCallback filter);
            virtual void removeResources(const ResourceFilterCallback filter);

        friend class OsmAnd::MapRendererResources;
        };

        // Tiled symbols:
        class SymbolsTiledResourcesCollection;
        class SymbolsTiledResource : public BaseTiledResource
        {
        private:
        protected:
            SymbolsTiledResource(MapRendererResources* owner, const TiledEntriesCollection<BaseTiledResource>& collection, const TileId tileId, const ZoomLevel zoom);

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

            std::shared_ptr<const MapTiledSymbols> _sourceData;
            QList< std::shared_ptr<GroupResources> > _uniqueGroupsResources;
            QList< std::shared_ptr<GroupResources> > _referencedSharedGroupsResources;

            virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController);
            virtual bool uploadToGPU();
            virtual void unloadFromGPU();

            virtual bool checkIsSafeToUnlink();
            virtual void detach();
        public:
            virtual ~SymbolsTiledResource();

        friend class OsmAnd::MapRendererResources;
        friend class OsmAnd::MapRendererResources::SymbolsTiledResourcesCollection;
        };
        class SymbolsTiledResourcesCollection : public TiledResourcesCollection
        {
        private:
        protected:
            SymbolsTiledResourcesCollection();

            std::array< SharedResourcesContainer<uint64_t, SymbolsTiledResource::GroupResources>, ZoomLevelsCount > _sharedGroupsResources;
        public:
            virtual ~SymbolsTiledResourcesCollection();

        friend class OsmAnd::MapRendererResources;
        friend class OsmAnd::MapRendererResources::SymbolsTiledResource;
        };

        // Keyed symbols:
        class SymbolsKeyedResource : public BaseKeyedResource
        {
        private:
        protected:
            SymbolsKeyedResource(MapRendererResources* owner, const KeyedEntriesCollection<Key, BaseKeyedResource>& collection, const Key key);

            std::shared_ptr<const MapSymbolsGroup> _sourceData;
            QHash< std::shared_ptr<const MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > _resourcesInGPU;

            virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController);
            virtual bool uploadToGPU();
            virtual void unloadFromGPU();

            virtual bool checkIsSafeToUnlink();
            virtual void detach();
        public:
            virtual ~SymbolsKeyedResource();

        friend class OsmAnd::MapRendererResources;
        };
        
        // Symbols:
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
                const std::shared_ptr<BaseResource>& requestedResource,
                const Concurrent::TaskHost::Bridge& bridge,
                ExecuteSignature executeMethod,
                PreExecuteSignature preExecuteMethod = nullptr,
                PostExecuteSignature postExecuteMethod = nullptr);
            virtual ~ResourceRequestTask();

            const std::shared_ptr<BaseResource> requestedResource;
        };

        // Each provider has a binded resource collection, and these are bindings:
        struct Binding
        {
            QHash< std::shared_ptr<IMapDataProvider>, std::shared_ptr<BaseResourcesCollection> > providersToCollections;
            QHash< std::shared_ptr<BaseResourcesCollection>, std::shared_ptr<IMapDataProvider> > collectionsToProviders;
        };
        std::array< Binding, ResourceTypesCount > _bindings;
        bool obtainProviderFor(BaseResourcesCollection* const resourcesRef, std::shared_ptr<IMapDataProvider>& provider) const;
        bool isDataSourceAvailableFor(const std::shared_ptr<BaseResourcesCollection>& collection) const;

        // Resources storages:
        ResourcesStorage _storageByType;

        // Symbols:
        mutable QMutex _mapSymbolsByOrderMutex;
        MapSymbolsByOrder _mapSymbolsByOrder;
        unsigned int _mapSymbolsCount;
        void addMapSymbol(const std::shared_ptr<const MapSymbol>& symbol, const std::shared_ptr<const GPUAPI::ResourceInGPU>& gpuResource);
        void removeMapSymbol(const std::shared_ptr<const MapSymbol>& symbol);

        void notifyNewResourceAvailableForDrawing();

        // Invalidated resources:
        mutable QReadWriteLock _invalidatedResourcesTypesMaskLock;
        volatile uint32_t _invalidatedResourcesTypesMask;
        void invalidateAllResources();
        void invalidateResourcesOfType(const ResourceType type);
        bool validateResources();
        bool validateResourcesOfType(const ResourceType type);

        // Resources management:
        QSet<TileId> _activeTiles;
        ZoomLevel _activeZoom;
        void updateResources(const QSet<TileId>& tiles, const ZoomLevel zoom);
        void requestNeededResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom);
        void requestNeededTiledResources(const std::shared_ptr<TiledResourcesCollection>& resourcesCollection, const QSet<TileId>& activeTiles, const ZoomLevel activeZoom);
        void requestNeededKeyedResources(const std::shared_ptr<KeyedResourcesCollection>& resourcesCollection);
        void requestNeededResource(const std::shared_ptr<BaseResource>& resource);
        void cleanupJunkResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom);
        unsigned int unloadResources();
        unsigned int uploadResources(const unsigned int limit = 0u, bool* const outMoreThanLimitAvailable = nullptr);
        void releaseResourcesFrom(const std::shared_ptr<BaseResourcesCollection>& collection);
        void requestResourcesUploadOrUnload();
        void releaseAllResources();

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
        bool uploadTileToGPU(const std::shared_ptr<const MapTiledData>& mapTile, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU);
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

        std::shared_ptr<const BaseResourcesCollection> getCollection(const ResourceType type, const std::shared_ptr<IMapDataProvider>& ofProvider) const;

        QMutex& getSymbolsMapMutex() const;
        const MapSymbolsByOrder& getMapSymbolsByOrder() const;
        unsigned int getMapSymbolsCount() const;

        void dumpResourcesInfo() const;

    friend class OsmAnd::MapRenderer;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_RESOURCES_H_)
