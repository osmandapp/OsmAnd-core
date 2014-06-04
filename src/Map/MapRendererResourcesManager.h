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
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "MapRendererBaseResource.h"
#include "MapRendererBaseTiledResource.h"
#include "MapRendererBaseKeyedResource.h"
#include "MapRendererBaseResourcesCollection.h"
#include "MapRendererTiledResourcesCollection.h"
#include "MapRendererKeyedResourcesCollection.h"
#include "MapRendererRasterBitmapTileResource.h"
#include "MapRendererElevationDataTileResource.h"
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
    class MapRenderer;
    class IMapDataProvider;
    class TiledMapSymbolsData;
    class MapSymbolsGroup;
    class MapSymbol;

    class MapRendererResourcesManager
    {
        Q_DISABLE_COPY(MapRendererResourcesManager);
    public:
        typedef std::array< QList< std::shared_ptr<MapRendererBaseResourcesCollection> >, MapRendererResourceTypesCount > ResourcesStorage;

        typedef QHash< std::shared_ptr<const MapSymbol>, QList< std::shared_ptr<MapRendererBaseResource> > > MapSymbolsByOrderRegisterLayer;
        typedef QMap<int, MapSymbolsByOrderRegisterLayer > MapSymbolsByOrderRegister;

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
                const std::shared_ptr<MapRendererBaseResource>& requestedResource,
                const Concurrent::TaskHost::Bridge& bridge,
                ExecuteSignature executeMethod,
                PreExecuteSignature preExecuteMethod = nullptr,
                PostExecuteSignature postExecuteMethod = nullptr);
            virtual ~ResourceRequestTask();

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
        ResourcesStorage _storageByType;

        // Symbols:
        mutable QMutex _mapSymbolsRegistersMutex;
        MapSymbolsByOrderRegister _mapSymbolsByOrderRegister;
        unsigned int _mapSymbolsInRegisterCount;
        void registerMapSymbol(const std::shared_ptr<const MapSymbol>& symbol, const std::shared_ptr<MapRendererBaseResource>& resource);
        void unregisterMapSymbol(const std::shared_ptr<const MapSymbol>& symbol, const std::shared_ptr<MapRendererBaseResource>& resource);

        void notifyNewResourceAvailableForDrawing();

        // Invalidated resources:
        mutable QReadWriteLock _invalidatedResourcesTypesMaskLock;
        volatile uint32_t _invalidatedResourcesTypesMask;
        void invalidateAllResources();
        void invalidateResourcesOfType(const MapRendererResourceType type);
        bool validateResources();
        bool validateResourcesOfType(const MapRendererResourceType type);

        // Resources management:
        QSet<TileId> _activeTiles;
        ZoomLevel _activeZoom;
        void updateResources(const QSet<TileId>& tiles, const ZoomLevel zoom);
        void requestNeededResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom);
        void requestNeededTiledResources(const std::shared_ptr<MapRendererTiledResourcesCollection>& resourcesCollection, const QSet<TileId>& activeTiles, const ZoomLevel activeZoom);
        void requestNeededKeyedResources(const std::shared_ptr<MapRendererKeyedResourcesCollection>& resourcesCollection);
        void requestNeededResource(const std::shared_ptr<MapRendererBaseResource>& resource);
        void cleanupJunkResources(const QSet<TileId>& activeTiles, const ZoomLevel activeZoom);
        unsigned int unloadResources();
        unsigned int uploadResources(const unsigned int limit = 0u, bool* const outMoreThanLimitAvailable = nullptr);
        void releaseResourcesFrom(const std::shared_ptr<MapRendererBaseResourcesCollection>& collection);
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
        MapRendererResourcesManager(MapRenderer* const owner);

        // Resources management:
        bool uploadTileToGPU(const std::shared_ptr<const MapTiledData>& mapTile, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU);
        bool uploadSymbolToGPU(const std::shared_ptr<const MapSymbol>& mapSymbol, std::shared_ptr<const GPUAPI::ResourceInGPU>& outResourceInGPU);
        std::shared_ptr<const SkBitmap> adjustBitmapToConfiguration(
            const std::shared_ptr<const SkBitmap>& input,
            const AlphaChannelData alphaChannelData) const;

        void updateBindings(const MapRendererState& state, const uint32_t updatedMask);
        void updateActiveZone(const QSet<TileId>& tiles, const ZoomLevel zoom);
        void syncResourcesInGPU(
            const unsigned int limitUploads = 0u,
            bool* const outMoreUploadsThanLimitAvailable = nullptr,
            unsigned int* const outResourcesUploaded = nullptr,
            unsigned int* const outResourcesUnloaded = nullptr);
    public:
        ~MapRendererResourcesManager();

        MapRenderer* const renderer;

        // Default resources
        const std::shared_ptr<const GPUAPI::ResourceInGPU>& processingTileStub;
        const std::shared_ptr<const GPUAPI::ResourceInGPU>& unavailableTileStub;

        std::shared_ptr<const MapRendererBaseResourcesCollection> getCollection(const MapRendererResourceType type, const std::shared_ptr<IMapDataProvider>& ofProvider) const;
        QMutex& getMapSymbolsRegistersMutex() const;
        const MapSymbolsByOrderRegister& getMapSymbolsByOrderRegister() const;
        unsigned int getMapSymbolsInRegisterCount() const;

        void dumpResourcesInfo() const;

    friend class OsmAnd::MapRenderer;
    friend class OsmAnd::MapRendererBaseResource;
    friend class OsmAnd::MapRendererBaseTiledResource;
    friend class OsmAnd::MapRendererBaseKeyedResource;
    friend class OsmAnd::MapRendererBaseResourcesCollection;
    friend class OsmAnd::MapRendererTiledResourcesCollection;
    friend class OsmAnd::MapRendererKeyedResourcesCollection;
    friend class OsmAnd::MapRendererRasterBitmapTileResource;
    friend class OsmAnd::MapRendererElevationDataTileResource;
    friend class OsmAnd::MapRendererTiledSymbolsResourcesCollection;
    friend class OsmAnd::MapRendererTiledSymbolsResource;
    friend class OsmAnd::MapRendererKeyedSymbolsResource;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_RESOURCES_H_)
