#ifndef _OSMAND_CORE_MAP_RENDERER_RESOURCES_H_
#define _OSMAND_CORE_MAP_RENDERER_RESOURCES_H_

#include "stdlib_common.h"
#include <functional>
#include <array>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QList>
#include <QHash>
#include <QSet>
#include <QReadWriteLock>
#include <QWaitCondition>
#include <QVector>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "MapRendererTypes_private.h"
#include "MapRendererState.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "MapRendererBaseResource.h"
#include "MapRendererBaseTiledResource.h"
#include "MapRendererBaseKeyedResource.h"
#include "MapRendererBaseResourcesCollection.h"
#include "MapRendererTiledResourcesCollection.h"
#include "MapRendererKeyedResourcesCollection.h"
#include "MapRendererRasterMapLayerResource.h"
#include "MapRendererElevationDataResource.h"
#include "MapRendererTiledSymbolsResourcesCollection.h"
#include "MapRendererTiledSymbolsResource.h"
#include "MapRendererKeyedSymbolsResource.h"
#include "GPUAPI.h"
#include "TiledEntriesCollection.h"
#include "KeyedEntriesCollection.h"
#include "SharedResourcesContainer.h"
#include "Thread.h"
#include "TaskHost.h"
#include "HostedTask.h"
#include "WorkerPool.h"
#include "IQueryController.h"

namespace OsmAnd
{
    class AtlasMapRenderer;
    class MapRenderer;
    class IMapDataProvider;
    class TiledMapSymbolsData;
    class MapSymbolsGroup;
    class MapSymbol;

    class MapRendererResourcesManager
    {
        Q_DISABLE_COPY_AND_MOVE(MapRendererResourcesManager)

    public:
        typedef std::array<
            QList< std::shared_ptr<MapRendererBaseResourcesCollection> >,
            MapRendererResourceTypesCount > ResourcesStorage;

    private:
        // Resource-requests related:
        const Concurrent::TaskHost::Bridge _taskHostBridge;
        Concurrent::WorkerPool _resourcesRequestWorkerPool;
        QAtomicInt _resourcesRequestTasksCounter;
        class ResourceRequestTask : public Concurrent::HostedTask
        {
            Q_DISABLE_COPY_AND_MOVE(ResourceRequestTask)
        private:
            void execute();
            void postExecute(const bool wasCancelled);

            static void executeWrapper(Task* const task);
            static void postExecuteWrapper(Task* const task, const bool wasCancelled);
        protected:
        public:
            ResourceRequestTask(
                const std::shared_ptr<MapRendererBaseResource>& requestedResource,
                const Concurrent::TaskHost::Bridge& bridge);
            virtual ~ResourceRequestTask();

            MapRendererResourcesManager* const manager;
            const std::shared_ptr<MapRendererBaseResource> requestedResource;

            int64_t calculatePriority(
                const TileId centerTileId,
                const QVector<TileId>& activeTiles,
                const ZoomLevel activeZoom) const;
        };
        void setResourceWorkerThreadsLimit(const unsigned int limit);
        void resetResourceWorkerThreadsLimit();

        // Each provider has a binded resource collection, and these are bindings:
        struct Binding
        {
            QHash< std::shared_ptr<IMapDataProvider>, std::shared_ptr<MapRendererBaseResourcesCollection> > providersToCollections;
            QHash< std::shared_ptr<MapRendererBaseResourcesCollection>, std::shared_ptr<IMapDataProvider> > collectionsToProviders;
        };
        std::array< Binding, MapRendererResourceTypesCount > _bindings;
        bool obtainProviderFor(
            MapRendererBaseResourcesCollection* const resourcesRef,
            std::shared_ptr<IMapDataProvider>& provider) const;
        bool noLockObtainProviderFor(
            MapRendererBaseResourcesCollection* const resourcesRef,
            std::shared_ptr<IMapDataProvider>& provider) const;
        bool isDataSourceAvailableFor(const std::shared_ptr<MapRendererBaseResourcesCollection>& collection) const;

        // Resources storages:
        mutable QReadWriteLock _resourcesStoragesLock;
        QList< std::shared_ptr<MapRendererBaseResourcesCollection> > _pendingRemovalResourcesCollections;
        ResourcesStorage _storageByType;
        QList< std::shared_ptr<MapRendererBaseResourcesCollection> > safeGetAllResourcesCollections() const;
        void safeGetAllResourcesCollections(
            QList< std::shared_ptr<MapRendererBaseResourcesCollection> >& outPendingRemovalResourcesCollections,
            QList< std::shared_ptr<MapRendererBaseResourcesCollection> >& outOtherResourcesCollections) const;

        // Symbols-related:
        void publishMapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& symbolGroup,
            const std::shared_ptr<const MapSymbol>& symbol,
            const std::shared_ptr<MapRendererBaseResource>& resource);
        void unpublishMapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& symbolGroup,
            const std::shared_ptr<const MapSymbol>& symbol,
            const std::shared_ptr<MapRendererBaseResource>& resource);
        void batchPublishMapSymbols(
            const QList< PublishOrUnpublishMapSymbol >& mapSymbolsToPublish);
        void batchUnpublishMapSymbols(
            const QList< PublishOrUnpublishMapSymbol >& mapSymbolsToUnublish);

        // Invalidated resources:
        QAtomicInt _invalidatedResourcesTypesMask;
        void invalidateAllResources();
        void invalidateResourcesOfType(const MapRendererResourceType type);
        bool validateResources();
        bool validateResourcesOfType(const MapRendererResourceType type);

        // Resources management:
        TileId _centerTileId;
        QVector<TileId> _activeTiles;
        ZoomLevel _activeZoom;
        QVector<QRunnable*> _requestedResourcesTasks;
        bool updatesPresent() const;
        bool checkForUpdatesAndApply() const;
        void updateResources(
            const TileId centerTileId,
            const QVector<TileId>& tiles,
            const ZoomLevel zoom);
        void requestNeededResources(
            const QList< std::shared_ptr<MapRendererBaseResourcesCollection> >& resourcesCollections,
            const TileId centerTileId,
            const QVector<TileId>& activeTiles,
            const ZoomLevel activeZoom);
        void requestNeededResources(
            const std::shared_ptr<MapRendererBaseResourcesCollection>& resourcesCollection,
            const QVector<TileId>& tiles,
            const ZoomLevel zoom);
        void requestNeededTiledResources(
            const std::shared_ptr<MapRendererTiledResourcesCollection>& resourcesCollection,
            const QVector<TileId>& tiles,
            const ZoomLevel zoom);
        void requestNeededKeyedResources(
            const std::shared_ptr<MapRendererKeyedResourcesCollection>& resourcesCollection);
        void requestNeededResource(
            const std::shared_ptr<MapRendererBaseResource>& resource);
        bool beginResourceRequestProcessing(const std::shared_ptr<MapRendererBaseResource>& resource);
        void endResourceRequestProcessing(
            const std::shared_ptr<MapRendererBaseResource>& resource,
            const bool requestSucceeded,
            const bool dataAvailable);
        void processResourceRequestCancellation(const std::shared_ptr<MapRendererBaseResource>& resource);
        void cleanupJunkResources(
            const QList< std::shared_ptr<MapRendererBaseResourcesCollection> >& pendingRemovalResourcesCollections,
            const QList< std::shared_ptr<MapRendererBaseResourcesCollection> >& resourcesCollections,
            const QVector<TileId>& activeTiles,
            const ZoomLevel activeZoom);
        bool cleanupJunkResource(
            const std::shared_ptr<MapRendererBaseResource>& resource,
            bool& needsResourcesUploadOrUnload);
        unsigned int unloadResources();
        void unloadResourcesFrom(
            const std::shared_ptr<MapRendererBaseResourcesCollection>& collection,
            unsigned int& totalUnloaded);
        unsigned int uploadResources(const unsigned int limit = 0u, bool* const outMoreThanLimitAvailable = nullptr);
        void uploadResourcesFrom(
            const std::shared_ptr<MapRendererBaseResourcesCollection>& collection,
            const unsigned int limit,
            unsigned int& totalUploaded,
            bool& moreThanLimitAvailable,
            bool& atLeastOneUploadFailed);
        void blockingReleaseResourcesFrom(
            const std::shared_ptr<MapRendererBaseResourcesCollection>& collection,
            const bool gpuContextLost);
        void requestResourcesUploadOrUnload();
        void notifyNewResourceAvailableForDrawing();
        void releaseAllResources(const bool gpuContextLost);

        // Worker thread:
        volatile bool _workerThreadIsAlive;
        const std::unique_ptr<Concurrent::Thread> _workerThread;
        Qt::HANDLE _workerThreadId;
        mutable QMutex _workerThreadWakeupMutex;
        QWaitCondition _workerThreadWakeup;
        void workerThreadProcedure();

        // Default resources:
        std::array<std::shared_ptr<const GPUAPI::ResourceInGPU>, MapStubStylesCount> _processingTileStubs;
        std::array<std::shared_ptr<const GPUAPI::ResourceInGPU>, MapStubStylesCount> _unavailableTileStubs;
        bool initializeDefaultResources();
        bool initializeTileStub(const QString& resourceName, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResource);
        bool releaseDefaultResources();
    protected:
        MapRendererResourcesManager(MapRenderer* const owner);

        // Resources management:
        bool uploadTiledDataToGPU(const std::shared_ptr<const IMapTiledDataProvider::Data>& mapTile, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU);
        bool uploadSymbolToGPU(const std::shared_ptr<const MapSymbol>& mapSymbol, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU);
        std::shared_ptr<const SkBitmap> adjustBitmapToConfiguration(
            const std::shared_ptr<const SkBitmap>& input,
            const AlphaChannelPresence alphaChannelPresence) const;
        void releaseGpuUploadableDataFrom(const std::shared_ptr<MapSymbol>& mapSymbol);

        bool updateBindings(const MapRendererState& state, const MapRendererStateChanges updatedMask);
        void updateElevationDataProviderBindings(const MapRendererState& state);
        void updateMapLayerProviderBindings(const MapRendererState& state);
        void updateSymbolProviderBindings(const MapRendererState& state);

        void updateActiveZone(const TileId centerTileId, const QVector<TileId>& tiles, const ZoomLevel zoom);
        void syncResourcesInGPU(
            const unsigned int limitUploads = 0u,
            bool* const outMoreUploadsThanLimitAvailable = nullptr,
            unsigned int* const outResourcesUploaded = nullptr,
            unsigned int* const outResourcesUnloaded = nullptr);

        std::shared_ptr<const MapRendererBaseResourcesCollection> getCollection(
            const MapRendererResourceType type,
            const std::shared_ptr<IMapDataProvider>& ofProvider) const;
        bool updateCollectionsSnapshots() const;
    public:
        ~MapRendererResourcesManager();

        MapRenderer* const renderer;

        // Default resources:
        const std::array<std::shared_ptr<const GPUAPI::ResourceInGPU>, MapStubStylesCount>& processingTileStubs;
        const std::array<std::shared_ptr<const GPUAPI::ResourceInGPU>, MapStubStylesCount>& unavailableTileStubs;

        // Resources management:
        bool collectionsSnapshotsInvalidated() const;
        std::shared_ptr<const IMapRendererResourcesCollection> getCollectionSnapshot(
            const MapRendererResourceType type,
            const std::shared_ptr<IMapDataProvider>& ofProvider) const;
        bool eachResourceIsUploadedOrUnavailable() const;
        bool allResourcesAreUploaded() const;
        void dumpResourcesInfo() const;

    friend class OsmAnd::MapRenderer;
    friend class OsmAnd::MapRendererBaseResource;
    friend class OsmAnd::MapRendererBaseTiledResource;
    friend class OsmAnd::MapRendererBaseKeyedResource;
    friend class OsmAnd::MapRendererBaseResourcesCollection;
    friend class OsmAnd::MapRendererTiledResourcesCollection;
    friend class OsmAnd::MapRendererKeyedResourcesCollection;
    friend class OsmAnd::MapRendererRasterMapLayerResource;
    friend class OsmAnd::MapRendererElevationDataResource;
    friend class OsmAnd::MapRendererTiledSymbolsResourcesCollection;
    friend class OsmAnd::MapRendererTiledSymbolsResource;
    friend class OsmAnd::MapRendererKeyedSymbolsResource;

    //TODO: MapRendererResourcesManager needs to be splitted into Altas and etc.
    friend class OsmAnd::AtlasMapRenderer;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_RESOURCES_H_)
