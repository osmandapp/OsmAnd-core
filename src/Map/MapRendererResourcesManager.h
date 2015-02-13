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
#include "Concurrent.h"
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
        Q_DISABLE_COPY_AND_MOVE(MapRendererResourcesManager);

    public:
        typedef std::array< QList< std::shared_ptr<MapRendererBaseResourcesCollection> >, MapRendererResourceTypesCount > ResourcesStorage;

    private:
        // Resource-requests related:
        const Concurrent::TaskHost::Bridge _taskHostBridge;
        QThreadPool _resourcesRequestWorkersPool;
        mutable QAtomicInt _resourcesRequestTasksCounter;
        class ResourceRequestTask : public Concurrent::HostedTask
        {
            Q_DISABLE_COPY_AND_MOVE(ResourceRequestTask);
        private:
        protected:
        public:
            ResourceRequestTask(
                const std::shared_ptr<MapRendererBaseResource>& requestedResource,
                const Concurrent::TaskHost::Bridge& bridge,
                ExecuteSignature executeMethod,
                PreExecuteSignature preExecuteMethod = nullptr,
                PostExecuteSignature postExecuteMethod = nullptr);
            virtual ~ResourceRequestTask();

            const MapRendererResourcesManager* const manager;
            const std::shared_ptr<MapRendererBaseResource> requestedResource;
        };

        // Each provider has a binded resource collection, and these are bindings:
        struct Binding
        {
            QHash< std::shared_ptr<IMapDataProvider>, std::shared_ptr<MapRendererBaseResourcesCollection> > providersToCollections;
            QHash< std::shared_ptr<MapRendererBaseResourcesCollection>, std::shared_ptr<IMapDataProvider> > collectionsToProviders;
        };
        std::array< Binding, MapRendererResourceTypesCount > _bindings;
        bool obtainProviderFor(MapRendererBaseResourcesCollection* const resourcesRef, std::shared_ptr<IMapDataProvider>& provider) const;
        bool isDataSourceAvailableFor(const std::shared_ptr<MapRendererBaseResourcesCollection>& collection) const;

        // Resources storages:
        mutable QReadWriteLock _resourcesStoragesLock;
        QList< std::shared_ptr<MapRendererBaseResourcesCollection> > _pendingRemovalResourcesCollections;
        ResourcesStorage _storageByType;
        QList< std::shared_ptr<MapRendererBaseResourcesCollection> > safeGetAllResourcesCollections() const;

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
        QSet<TileId> _activeTiles;
        ZoomLevel _activeZoom;
        bool updatesPresent() const;
        bool checkForUpdatesAndApply() const;
        void updateResources(const QSet<TileId>& tiles, const ZoomLevel zoom);
        void requestNeededResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom);
        void requestNeededTiledResources(const std::shared_ptr<MapRendererTiledResourcesCollection>& resourcesCollection, const QSet<TileId>& activeTiles, const ZoomLevel activeZoom);
        void requestNeededKeyedResources(const std::shared_ptr<MapRendererKeyedResourcesCollection>& resourcesCollection);
        void requestNeededResource(const std::shared_ptr<MapRendererBaseResource>& resource);
        void cleanupJunkResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom);
        bool cleanupJunkResource(const std::shared_ptr<MapRendererBaseResource>& resource, bool& needsResourcesUploadOrUnload);
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
        bool initializeDefaultResources();
        bool releaseDefaultResources();
        std::shared_ptr<const GPUAPI::ResourceInGPU> _processingTileStub;
        std::shared_ptr<const GPUAPI::ResourceInGPU> _unavailableTileStub;
    protected:
        MapRendererResourcesManager(MapRenderer* const owner);

        // Resources management:
        bool uploadTiledDataToGPU(const std::shared_ptr<const IMapTiledDataProvider::Data>& mapTile, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU);
        bool uploadSymbolToGPU(const std::shared_ptr<const MapSymbol>& mapSymbol, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU);
        std::shared_ptr<const SkBitmap> adjustBitmapToConfiguration(
            const std::shared_ptr<const SkBitmap>& input,
            const AlphaChannelPresence alphaChannelPresence) const;
        void releaseGpuUploadableDataFrom(const std::shared_ptr<MapSymbol>& mapSymbol);

        void updateBindings(const MapRendererState& state, const MapRendererStateChanges updatedMask);
        void updateActiveZone(const QSet<TileId>& tiles, const ZoomLevel zoom);
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
        const std::shared_ptr<const GPUAPI::ResourceInGPU>& processingTileStub;
        const std::shared_ptr<const GPUAPI::ResourceInGPU>& unavailableTileStub;

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
