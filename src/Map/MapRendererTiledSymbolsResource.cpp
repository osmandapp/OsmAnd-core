#include "MapRendererTiledSymbolsResource.h"

#include "MapRenderer.h"
#include "MapRendererResourcesManager.h"
#include "IMapDataProvider.h"
#include "IMapTiledSymbolsProvider.h"
#include "ObfMapObjectsProvider.h"
#include "RasterMapSymbol.h"
#include "MapRendererResourcesManager.h"
#include "MapRendererBaseResourcesCollection.h"
#include "MapRendererTiledSymbolsResourcesCollection.h"
#include "QKeyValueIterator.h"
#include "QRunnableFunctor.h"
#include "Utilities.h"

//#define OSMAND_PERFORMANCE_METRICS 1
#if !defined(OSMAND_PERFORMANCE_METRICS)
#   define OSMAND_PERFORMANCE_METRICS 0
#endif // !defined(OSMAND_PERFORMANCE_METRICS)

//#define OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE 1
#ifndef OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
#   define OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE 0
#endif // !defined(OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE)

//#define OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES 1
#ifndef OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES
#   define OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES 0
#endif // !defined(OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES)

//#define OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_REQUESTS 1
#ifndef OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_REQUESTS
#   define OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_REQUESTS 0
#endif // !defined(OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_REQUESTS)

OsmAnd::MapRendererTiledSymbolsResource::MapRendererTiledSymbolsResource(
    MapRendererResourcesManager* owner_,
    const TiledEntriesCollection<MapRendererBaseTiledResource>& collection_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : MapRendererBaseTiledResource(owner_, MapRendererResourceType::Symbols, collection_, tileId_, zoom_)
{
}

OsmAnd::MapRendererTiledSymbolsResource::~MapRendererTiledSymbolsResource()
{
    safeUnlink();
}

bool OsmAnd::MapRendererTiledSymbolsResource::supportsObtainDataAsync() const
{
    bool ok = false;

    std::shared_ptr<IMapDataProvider> provider;
    if (const auto link_ = link.lock())
    {
        const auto collection = static_cast<MapRendererTiledSymbolsResourcesCollection*>(&link_->collection);
        ok = resourcesManager->obtainProviderFor(
            static_cast<MapRendererBaseResourcesCollection*>(collection),
            provider);
    }
    if (!ok)
        return false;

    return provider->supportsNaturalObtainDataAsync();
}

bool OsmAnd::MapRendererTiledSymbolsResource::obtainData(
    bool& dataAvailable,
    const std::shared_ptr<const IQueryController>& queryController)
{
    // Obtain collection link and maintain it
    const auto link_ = link.lock();
    if (!link_)
        return false;
    const auto collection = static_cast<MapRendererTiledSymbolsResourcesCollection*>(&link_->collection);

    // Get source of tile
    std::shared_ptr<IMapDataProvider> provider_;
    bool ok = resourcesManager->obtainProviderFor(static_cast<MapRendererBaseResourcesCollection*>(collection), provider_);
    if (!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapTiledSymbolsProvider>(provider_);

    auto& sharedGroupsResources = collection->_sharedGroupsResources[zoom];

    QSet<MapSymbolsGroup::SharingKey> uniqueSymbolsGroupsKeys;
    bool checkUniqueKeys = zoom <= ObfMapObjectsProvider::AddDuplicatedMapObjectsMaxZoom;
    // Obtain tile from provider
    QList< std::shared_ptr<SharedGroupResources> > referencedSharedGroupsResources;
    QList< proper::shared_future< std::shared_ptr<SharedGroupResources> > > futureReferencedSharedGroupsResources;
    QSet< uint64_t > loadedSharedGroups;
    std::shared_ptr<IMapTiledSymbolsProvider::Data> tile;
    IMapTiledSymbolsProvider::Request request;
    request.queryController = queryController;
    request.tileId = tileId;
    request.zoom = zoom;
    const auto& mapState = resourcesManager->renderer->getMapState();
    request.mapState = mapState;
    request.filterCallback =
        [provider, &sharedGroupsResources, &referencedSharedGroupsResources, &futureReferencedSharedGroupsResources, &loadedSharedGroups, &uniqueSymbolsGroupsKeys, checkUniqueKeys]
        (const IMapTiledSymbolsProvider*, const std::shared_ptr<const MapSymbolsGroup>& symbolsGroup) -> bool
        {
            // If map symbols group is not shareable, just accept it
            MapSymbolsGroup::SharingKey sharingKey;
            const auto isSharableById = symbolsGroup->obtainSharingKey(sharingKey);
            if (!isSharableById)
                return true;

            if (checkUniqueKeys)
            {
                if (uniqueSymbolsGroupsKeys.contains(sharingKey))
                    return false;
                uniqueSymbolsGroupsKeys.insert(sharingKey);
            }

            // Check if this shared symbol is already available, or mark it as pending
            std::shared_ptr<SharedGroupResources> sharedGroupResources;
            proper::shared_future< std::shared_ptr<SharedGroupResources> > futureSharedGroupResources;
            if (sharedGroupsResources.obtainReferenceOrFutureReferenceOrMakePromise(sharingKey, sharedGroupResources, futureSharedGroupResources))
            {
                if (static_cast<bool>(sharedGroupResources))
                {
                    referencedSharedGroupsResources.push_back(qMove(sharedGroupResources));

#if OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
                    LogPrintf(LogSeverityLevel::Debug,
                        "Shared GroupResources(%p) for %s referenced from %p (%dx%d@%d)",
                        sharedGroupResources.get(),
                        qPrintable(symbolsGroup->getDebugTitle()),
                        this,
                        tileId.x, tileId.y, zoom);
#endif // OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
                }
                else
                    futureReferencedSharedGroupsResources.push_back(qMove(futureSharedGroupResources));
                return false;
            }

            // Or load this shared group
            loadedSharedGroups.insert(sharingKey);
            return true;
        };
    request.combineTilesData = isMetaTiled;

    const auto requestSucceeded = provider->obtainTiledSymbols(request, tile);
    if (!requestSucceeded)
    {
        for (const auto& sharedGroup : constOf(loadedSharedGroups))
        {
            sharedGroupsResources.breakPromise(sharedGroup);
        }
        return false;
    }

    // Store data
    _sourceData = tile;
    dataAvailable = static_cast<bool>(tile);

    // Process data
    if (!dataAvailable)
        return true;

    // Convert data
    for (const auto& symbolsGroup : constOf(_sourceData->symbolsGroups))
    {
        for (const auto& mapSymbol : constOf(symbolsGroup->symbols))
        {
            const auto rasterMapSymbol = std::dynamic_pointer_cast<RasterMapSymbol>(mapSymbol);
            if (!rasterMapSymbol)
                continue;

            rasterMapSymbol->image = resourcesManager->adjustImageToConfiguration(
                rasterMapSymbol->image,
                AlphaChannelPresence::Present);
        }
    }
    // Move referenced shared groups
    _referencedSharedGroupsResources = referencedSharedGroupsResources;

    // tile->symbolsGroups contains groups that derived from unique symbols, or loaded shared groups
    for (const auto& symbolsGroup : constOf(_sourceData->symbolsGroups))
    {
        MapSymbolsGroup::SharingKey sharingKey;
        if (symbolsGroup->obtainSharingKey(sharingKey))
        {
            // Check if this map symbols group is loaded as shared (even if it's sharable)
            if (!loadedSharedGroups.contains(sharingKey))
            {
                // Create GroupResources instance and add it to unique group resources
                const std::shared_ptr<GroupResources> groupResources(new GroupResources(symbolsGroup));
                _uniqueGroupsResources.push_back(qMove(groupResources));
                continue;
            }

            // Otherwise insert it as shared group
            const std::shared_ptr<SharedGroupResources> groupResources(new SharedGroupResources(symbolsGroup));
            sharedGroupsResources.fulfilPromiseAndReference(sharingKey, groupResources);
            _referencedSharedGroupsResources.push_back(qMove(groupResources));

#if OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
            LogPrintf(LogSeverityLevel::Debug,
                "Shared GroupResources(%p) for %s allocated and referenced from %p (%dx%d@%d): %" PRIu64 " ref(s)",
                groupResources.get(),
                qPrintable(symbolsGroup->getDebugTitle()),
                this,
                tileId.x, tileId.y, zoom,
                sharedGroupsResources.getReferencesCount(symbolsGroupWithId->getId()));
#endif // OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
        }
        else
        {
            // Create GroupResources instance and add it to unique group resources
            const std::shared_ptr<GroupResources> groupResources(new GroupResources(symbolsGroup));
            _uniqueGroupsResources.push_back(qMove(groupResources));
        }
    }
    if (queryController && queryController->isAborted())
        return false;

    int failCounter = 0;
    // Wait for future referenced shared groups
    for (auto& futureGroup : futureReferencedSharedGroupsResources)
    {
        if (queryController && queryController->isAborted())
            break;

        auto timeout = proper::chrono::system_clock::now() + proper::chrono::seconds(3);
        if (proper::future_status::ready == futureGroup.wait_until(timeout))
        {
            try
            {
                auto groupResources = futureGroup.get();
                _referencedSharedGroupsResources.push_back(qMove(groupResources));
            }
            catch(const std::exception& e)
            {
                continue;
            }
        }
        else
        {
            failCounter++;
            LogPrintf(LogSeverityLevel::Error, "Get groupResources timeout exceed = %d", failCounter);
        }
        if (failCounter >= 3)
        {
            LogPrintf(LogSeverityLevel::Error, "Get groupResources timeout exceed");
            break;
        }

#if OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
        const auto symbolsGroupWithId = std::static_pointer_cast<const IMapSymbolsGroupWithUniqueId>(groupResources->group);
        LogPrintf(LogSeverityLevel::Debug,
            "Shared GroupResources(%p) for %s referenced from %p (%dx%d@%d): %" PRIu64 " ref(s)",
            groupResources.get(),
            qPrintable(symbolsGroup->getDebugTitle()),
            this,
            tileId.x, tileId.y, zoom,
            sharedGroupsResources.getReferencesCount(symbolsGroupWithId->getId()));
#endif // OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
    }
    if (queryController && queryController->isAborted())
        return false;

    // Get symbol subsection
    const auto symbolSubsection = resourcesManager->renderer->getSymbolsProviderSubsection(provider);

    // Register all obtained symbols
    const auto& self = shared_from_this();
    QList< PublishOrUnpublishMapSymbol > mapSymbolsToPublish;
    for (const auto& groupResources : constOf(_uniqueGroupsResources))
    {
        if (queryController && queryController->isAborted())
            break;

        const auto& symbolsGroup = groupResources->group;
        auto& publishedMapSymbols = _publishedMapSymbolsByGroup[symbolsGroup];
        mapSymbolsToPublish.reserve(mapSymbolsToPublish.size() + symbolsGroup->symbols.size());
        for (const auto& mapSymbol : constOf(symbolsGroup->symbols))
        {
            // Set symbol subsection
            mapSymbol->subsection = symbolSubsection;

            PublishOrUnpublishMapSymbol mapSymbolToPublish = {
                symbolsGroup,
                std::static_pointer_cast<const MapSymbol>(mapSymbol),
                self };
            mapSymbolsToPublish.push_back(mapSymbolToPublish);
            publishedMapSymbols.push_back(mapSymbol);
        }
    }
    for (const auto& groupResources : constOf(_referencedSharedGroupsResources))
    {
        if (queryController && queryController->isAborted())
            break;

        const auto& symbolsGroup = groupResources->group;
        auto& publishedMapSymbols = _publishedMapSymbolsByGroup[symbolsGroup];
        mapSymbolsToPublish.reserve(mapSymbolsToPublish.size() + symbolsGroup->symbols.size());
        for (const auto& mapSymbol : constOf(symbolsGroup->symbols))
        {
            // Set symbol subsection
            mapSymbol->subsection = symbolSubsection;

            PublishOrUnpublishMapSymbol mapSymbolToPublish = {
                symbolsGroup,
                std::static_pointer_cast<const MapSymbol>(mapSymbol),
                self };
            mapSymbolsToPublish.push_back(mapSymbolToPublish);
            publishedMapSymbols.push_back(mapSymbol);
        }
    }
    if (queryController && queryController->isAborted())
        return false;
    resourcesManager->batchPublishMapSymbols(mapSymbolsToPublish);

    // Since there's a copy of references to map symbols groups and symbols themselves,
    // it's safe to consume all the data here
    _retainableCacheMetadata = _sourceData->retainableCacheMetadata;
    _sourceData.reset();

    return true;
}

void OsmAnd::MapRendererTiledSymbolsResource::obtainDataAsync(
    const ObtainDataAsyncCallback callback,
    const std::shared_ptr<const IQueryController>& queryController,
    const bool cacheOnly /*=false*/)
{
    // Obtain collection link and maintain it
    const auto link_ = link.lock();
    if (!link_)
    {
        callback(false, false);
        return;
    }

    const auto self = std::static_pointer_cast<MapRendererTiledSymbolsResource>(shared_from_this());
    const auto weakSelf = std::weak_ptr<MapRendererTiledSymbolsResource>(self);
    const auto weakController = std::weak_ptr<const IQueryController>(queryController);
    const auto weakManager =
        std::weak_ptr<MapRendererResourcesManager>(resourcesManager->renderer->getResourcesSharedPtr());

    const QRunnableFunctor::Callback task =
        [weakSelf, weakController, weakManager, callback]
        (const QRunnableFunctor* const runnable)
        {
            const auto self = weakSelf.lock();
            const auto queryController = weakController.lock();
            if (!self || !queryController)
            {
                callback(false, false);
                return;
            }
            const auto link = self->link.lock();
            if (!link)
            {
                callback(false, false);
                return;
            }

            const auto collection = static_cast<MapRendererTiledSymbolsResourcesCollection*>(&link->collection);

            std::shared_ptr<IMapDataProvider> provider_;
            if (const auto resourcesManager = weakManager.lock())
            {
                // Get source of tile
                bool ok = resourcesManager->obtainProviderFor(
                    static_cast<MapRendererBaseResourcesCollection*>(collection), provider_);
                if (!ok)
                {
                    callback(false, false);
                    return;
                }
            }
            else
                return;
            const auto provider = std::static_pointer_cast<IMapTiledSymbolsProvider>(provider_);
            const auto tileId = self->tileId;
            const auto zoom = self->zoom;
            auto& sharedGroupsResources = collection->_sharedGroupsResources[zoom];

            // Obtain tile from provider
            QSet<MapSymbolsGroup::SharingKey> uniqueSymbolsGroupsKeys;
            bool checkUniqueKeys = zoom <= ObfMapObjectsProvider::AddDuplicatedMapObjectsMaxZoom;
            QList< std::shared_ptr<SharedGroupResources> > referencedSharedGroupsResources;
            QList< proper::shared_future< std::shared_ptr<SharedGroupResources> > > futureReferencedSharedGroupsResources;
            QSet< uint64_t > loadedSharedGroups;
            std::shared_ptr<IMapTiledSymbolsProvider::Data> tile;
            IMapTiledSymbolsProvider::Request request;
            request.queryController = queryController;
            request.tileId = tileId;
            request.zoom = zoom;
            if (const auto resourcesManager = weakManager.lock())
            {
                const auto& mapState = resourcesManager->renderer->getMapState();
                request.mapState = mapState;
            }
            else
                return;
            request.filterCallback =
                [provider, &sharedGroupsResources, &referencedSharedGroupsResources, &futureReferencedSharedGroupsResources, &loadedSharedGroups, &uniqueSymbolsGroupsKeys, checkUniqueKeys]
                (const IMapTiledSymbolsProvider*, const std::shared_ptr<const MapSymbolsGroup>& symbolsGroup) -> bool
                {
                    // If map symbols group is not shareable, just accept it
                    MapSymbolsGroup::SharingKey sharingKey;
                    const auto isSharableById = symbolsGroup->obtainSharingKey(sharingKey);
                    if (!isSharableById)
                        return true;

                    if (checkUniqueKeys)
                    {
                        if (uniqueSymbolsGroupsKeys.contains(sharingKey))
                            return false;
                        uniqueSymbolsGroupsKeys.insert(sharingKey);
                    }

                    // Check if this shared symbol is already available, or mark it as pending
                    std::shared_ptr<SharedGroupResources> sharedGroupResources;
                    proper::shared_future< std::shared_ptr<SharedGroupResources> > futureSharedGroupResources;
                    if (sharedGroupsResources.obtainReferenceOrFutureReferenceOrMakePromise(sharingKey, sharedGroupResources, futureSharedGroupResources))
                    {
                        if (static_cast<bool>(sharedGroupResources))
                        {
                            referencedSharedGroupsResources.push_back(qMove(sharedGroupResources));

#if OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
                            LogPrintf(LogSeverityLevel::Debug,
                                "Shared GroupResources(%p) for %s referenced from %p (%dx%d@%d)",
                                sharedGroupResources.get(),
                                qPrintable(symbolsGroup->getDebugTitle()),
                                this,
                                tileId.x, tileId.y, zoom);
#endif // OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
                        }
                        else
                            futureReferencedSharedGroupsResources.push_back(qMove(futureSharedGroupResources));
                        return false;
                    }

                    // Or load this shared group
                    loadedSharedGroups.insert(sharingKey);
                    return true;
                };
            request.combineTilesData = self->isMetaTiled;

            const auto requestSucceeded = provider->obtainTiledSymbols(request, tile);
            if (!requestSucceeded)
            {
                for (const auto& sharedGroup : constOf(loadedSharedGroups))
                {
                    sharedGroupsResources.breakPromise(sharedGroup);
                }
                callback(false, false);
                return;
            }

            // Store data
            self->_sourceData = tile;
            const bool dataAvailable = static_cast<bool>(tile);

            // Process data
            if (!dataAvailable)
            {
                for (const auto& sharedGroup : constOf(loadedSharedGroups))
                {
                    sharedGroupsResources.breakPromise(sharedGroup);
                }
                callback(true, false);
                return;
            }

            if (const auto resourcesManager = weakManager.lock())
            {
                // Convert data
                for (const auto& symbolsGroup : constOf(self->_sourceData->symbolsGroups))
                {
                    for (const auto& mapSymbol : constOf(symbolsGroup->symbols))
                    {
                        const auto rasterMapSymbol = std::dynamic_pointer_cast<RasterMapSymbol>(mapSymbol);
                        if (!rasterMapSymbol)
                            continue;

                        rasterMapSymbol->image = resourcesManager->adjustImageToConfiguration(
                            rasterMapSymbol->image,
                            AlphaChannelPresence::Present);
                    }
                }
            }
            else
                return;

            // Move referenced shared groups
            self->_referencedSharedGroupsResources = referencedSharedGroupsResources;

            // tile->symbolsGroups contains groups that derived from unique symbols, or loaded shared groups
            for (const auto& symbolsGroup : constOf(self->_sourceData->symbolsGroups))
            {
                MapSymbolsGroup::SharingKey sharingKey;
                if (symbolsGroup->obtainSharingKey(sharingKey))
                {
                    // Check if this map symbols group is loaded as shared (even if it's sharable)
                    if (!loadedSharedGroups.contains(sharingKey))
                    {
                        // Create GroupResources instance and add it to unique group resources
                        const std::shared_ptr<GroupResources> groupResources(new GroupResources(symbolsGroup));
                        self->_uniqueGroupsResources.push_back(qMove(groupResources));
                        continue;
                    }

                    // Otherwise insert it as shared group
                    const std::shared_ptr<SharedGroupResources> groupResources(new SharedGroupResources(symbolsGroup));
                    sharedGroupsResources.fulfilPromiseAndReference(sharingKey, groupResources);
                    self->_referencedSharedGroupsResources.push_back(qMove(groupResources));

#if OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
                    LogPrintf(LogSeverityLevel::Debug,
                        "Shared GroupResources(%p) for %s allocated and referenced from %p (%dx%d@%d): %" PRIu64 " ref(s)",
                        groupResources.get(),
                        qPrintable(symbolsGroup->getDebugTitle()),
                        this,
                        tileId.x, tileId.y, zoom,
                        sharedGroupsResources.getReferencesCount(symbolsGroupWithId->getId()));
#endif // OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
                }
                else
                {
                    // Create GroupResources instance and add it to unique group resources
                    const std::shared_ptr<GroupResources> groupResources(new GroupResources(symbolsGroup));
                    self->_uniqueGroupsResources.push_back(qMove(groupResources));
                }
            }
            if (queryController && queryController->isAborted())
            {
                callback(false, false);
                return;
            }

            int failCounter = 0;
            // Wait for future referenced shared groups
            for (auto& futureGroup : futureReferencedSharedGroupsResources)
            {
                if (queryController && queryController->isAborted())
                    break;

                auto timeout = proper::chrono::system_clock::now() + proper::chrono::seconds(3);
                if (proper::future_status::ready == futureGroup.wait_until(timeout))
                {
                    try
                    {
                        auto groupResources = futureGroup.get();
                        self->_referencedSharedGroupsResources.push_back(qMove(groupResources));
                    }
                    catch(const std::exception& e)
                    {
                        continue;
                    }
                }
                else
                {
                    failCounter++;
                    LogPrintf(LogSeverityLevel::Error, "Get groupResources timeout exceed = %d", failCounter);
                }
                if (failCounter >= 3)
                {
                    LogPrintf(LogSeverityLevel::Error, "Get groupResources timeout exceed");
                    break;
                }

#if OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
                const auto symbolsGroupWithId = std::static_pointer_cast<const IMapSymbolsGroupWithUniqueId>(groupResources->group);
                LogPrintf(LogSeverityLevel::Debug,
                    "Shared GroupResources(%p) for %s referenced from %p (%dx%d@%d): %" PRIu64 " ref(s)",
                    groupResources.get(),
                    qPrintable(symbolsGroup->getDebugTitle()),
                    this,
                    tileId.x, tileId.y, zoom,
                    sharedGroupsResources.getReferencesCount(symbolsGroupWithId->getId()));
#endif // OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
            }
            if (queryController && queryController->isAborted())
            {
                callback(false, false);
                return;
            }

            int symbolSubsection;
            if (const auto resourcesManager = weakManager.lock())
            {
                // Get symbol subsection
                symbolSubsection = resourcesManager->renderer->getSymbolsProviderSubsection(provider);
            }
            else
                return;

            // Register all obtained symbols
            QList< PublishOrUnpublishMapSymbol > mapSymbolsToPublish;
            for (const auto& groupResources : constOf(self->_uniqueGroupsResources))
            {
                if (queryController && queryController->isAborted())
                    break;

                const auto& symbolsGroup = groupResources->group;
                auto& publishedMapSymbols = self->_publishedMapSymbolsByGroup[symbolsGroup];
                mapSymbolsToPublish.reserve(mapSymbolsToPublish.size() + symbolsGroup->symbols.size());
                for (const auto& mapSymbol : constOf(symbolsGroup->symbols))
                {
                    // Set symbol subsection
                    mapSymbol->subsection = symbolSubsection;

                    PublishOrUnpublishMapSymbol mapSymbolToPublish = {
                        symbolsGroup,
                        std::static_pointer_cast<const MapSymbol>(mapSymbol),
                        self };
                    mapSymbolsToPublish.push_back(mapSymbolToPublish);
                    publishedMapSymbols.push_back(mapSymbol);
                }
            }
            for (const auto& groupResources : constOf(self->_referencedSharedGroupsResources))
            {
                if (queryController && queryController->isAborted())
                    break;

                const auto& symbolsGroup = groupResources->group;
                auto& publishedMapSymbols = self->_publishedMapSymbolsByGroup[symbolsGroup];
                mapSymbolsToPublish.reserve(mapSymbolsToPublish.size() + symbolsGroup->symbols.size());
                for (const auto& mapSymbol : constOf(symbolsGroup->symbols))
                {
                    // Set symbol subsection
                    mapSymbol->subsection = symbolSubsection;

                    PublishOrUnpublishMapSymbol mapSymbolToPublish = {
                        symbolsGroup,
                        std::static_pointer_cast<const MapSymbol>(mapSymbol),
                        self };
                    mapSymbolsToPublish.push_back(mapSymbolToPublish);
                    publishedMapSymbols.push_back(mapSymbol);
                }
            }
            if (queryController && queryController->isAborted())
            {
                callback(false, false);
                return;
            }

            if (const auto resourcesManager = weakManager.lock())
            {
                resourcesManager->batchPublishMapSymbols(mapSymbolsToPublish);
            }
            else
                return;

            // Since there's a copy of references to map symbols groups and symbols themselves,
            // it's safe to consume all the data here
            self->_retainableCacheMetadata = self->_sourceData->retainableCacheMetadata;
            self->_sourceData.reset();

            callback(requestSucceeded, dataAvailable);
        };

    const auto taskRunnable = new QRunnableFunctor(task);
    taskRunnable->setAutoDelete(true);
    QThreadPool::globalInstance()->start(taskRunnable);
}

bool OsmAnd::MapRendererTiledSymbolsResource::uploadToGPU()
{
    const Stopwatch timer(
#if OSMAND_PERFORMANCE_METRICS
        true
#else
        false
#endif // OSMAND_PERFORMANCE_METRICS
        );
    
    typedef std::pair< std::shared_ptr<MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > SymbolResourceEntry;

    bool ok;
    bool anyUploadFailed = false;
    int uploaded = 0;
    
    const auto link_ = link.lock();
    if (!link_)
        return false;
    const auto collection = static_cast<MapRendererTiledSymbolsResourcesCollection*>(&link_->collection);

    // Unique
    QMultiHash< std::shared_ptr<GroupResources>, SymbolResourceEntry > uniqueUploaded;
    for (const auto& groupResources : constOf(_uniqueGroupsResources))
    {
        for (const auto& symbol : constOf(groupResources->group->symbols))
        {
            // Prepare data and upload to GPU
            std::shared_ptr<const GPUAPI::ResourceInGPU> resourceInGPU;
            ok = resourcesManager->uploadSymbolToGPU(symbol, resourceInGPU);

            uploaded++;
            
            // If upload have failed, stop
            if (!ok)
            {
                LogPrintf(LogSeverityLevel::Error, "Failed to upload unique symbol in %dx%d@%d tile",
                    tileId.x, tileId.y, zoom);

                anyUploadFailed = true;
                break;
            }

            // Mark this symbol as uploaded
            uniqueUploaded.insert(groupResources, qMove(SymbolResourceEntry(symbol, resourceInGPU)));
        }
        if (anyUploadFailed)
            break;
    }

    // Shared
    QMultiHash< std::shared_ptr<GroupResources>, SymbolResourceEntry > sharedUploaded;
    QMultiHash< std::shared_ptr<GroupResources>, SymbolResourceEntry > sharedReferenced;
    for (const auto& groupResources : constOf(_referencedSharedGroupsResources))
    {
        if (groupResources->group->symbols.isEmpty())
            continue;

        // This check means "continue if shared symbols were uploaded by other resource"
        // This check needs no special handling, since all GPU resources work is done from same thread.
        if (!groupResources->resourcesInGPU.isEmpty())
        {
            for (const auto& entryResourceInGPU : rangeOf(constOf(groupResources->resourcesInGPU)))
            {
                const auto& symbol = entryResourceInGPU.key();
                auto& resourceInGPU = entryResourceInGPU.value();

                sharedReferenced.insert(groupResources, qMove(SymbolResourceEntry(symbol, resourceInGPU)));
            }
            continue;
        }

        for (const auto& symbol : constOf(groupResources->group->symbols))
        {
            // Prepare data and upload to GPU
            std::shared_ptr<const GPUAPI::ResourceInGPU> resourceInGPU;
            ok = resourcesManager->uploadSymbolToGPU(symbol, resourceInGPU);
            
            uploaded++;

            // If upload have failed, stop
            if (!ok)
            {
                LogPrintf(LogSeverityLevel::Error, "Failed to upload unique symbol in %dx%d@%d tile",
                    tileId.x, tileId.y, zoom);

                anyUploadFailed = true;
                break;
            }

            // Mark this symbol as uploaded
            sharedUploaded.insert(groupResources, qMove(SymbolResourceEntry(symbol, resourceInGPU)));
        }
        if (anyUploadFailed)
            break;
    }

    // If at least one symbol failed to upload, consider entire tile as failed to upload,
    // and unload its partial GPU resources
    if (anyUploadFailed)
    {
        uniqueUploaded.clear();
        sharedUploaded.clear();

        return false;
    }

    // All resources have been uploaded to GPU successfully by this point,
    // so it's safe to walk across symbols and remove bitmaps:

    // Unique
    for (const auto& entry : rangeOf(constOf(uniqueUploaded)))
    {
        const auto& groupResources = entry.key();
        const auto& symbol = entry.value().first;
        auto& resource = entry.value().second;

        // Unload GPU data from symbol, since it's uploaded already
        resourcesManager->releaseGpuUploadableDataFrom(symbol);

        // Add GPU resource reference
        _symbolToResourceInGpuLUT.insert(symbol, resource);
#if OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES
        LogPrintf(LogSeverityLevel::Debug,
            "Inserted GPU resource %p for map symbol %p in %p (uploadToGPU-uniqueUploaded)",
            resource.get(),
            symbol.get(),
            this);
#endif // OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES
        groupResources->resourcesInGPU.insert(qMove(symbol), qMove(resource));
    }

    // Shared (uploaded)
    for (const auto& entry : rangeOf(constOf(sharedUploaded)))
    {
        const auto& groupResources = entry.key();
        auto symbol = entry.value().first;
        auto& resource = entry.value().second;

        // Unload GPU data from symbol, since it's uploaded already
        resourcesManager->releaseGpuUploadableDataFrom(symbol);

        // Add GPU resource reference
        _symbolToResourceInGpuLUT.insert(symbol, resource);
#if OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES
        LogPrintf(LogSeverityLevel::Debug,
            "Inserted GPU resource %p for map symbol %p in %p (uploadToGPU-sharedUploaded)",
            resource.get(),
            symbol.get(),
            this);
#endif // OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES
        groupResources->resourcesInGPU.insert(qMove(symbol), qMove(resource));
    }

    // Shared (referenced)
    for (const auto& entry : rangeOf(constOf(sharedReferenced)))
    {
        const auto& groupResources = entry.key();
        auto symbol = entry.value().first;
        auto& resource = entry.value().second;

        // Add GPU resource reference
        _symbolToResourceInGpuLUT.insert(symbol, resource);
#if OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES
        LogPrintf(LogSeverityLevel::Debug,
            "Inserted GPU resource %p for map symbol %p in %p (uploadToGPU-sharedReferenced)",
            resource.get(),
            symbol.get(),
            this);
#endif // OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES
    }

#if OSMAND_PERFORMANCE_METRICS
#if OSMAND_PERFORMANCE_METRICS <= 1
    auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    LogPrintf(LogSeverityLevel::Info, ">>>>> %ld: uploadToGPU %d in %fs", millis, uploaded, timer.elapsed());
#endif // OSMAND_PERFORMANCE_METRICS <= 1
#endif // OSMAND_PERFORMANCE_METRICS

    return true;
}

void OsmAnd::MapRendererTiledSymbolsResource::unloadFromGPU(bool gpuContextLost)
{
    const Stopwatch timer(
#if OSMAND_PERFORMANCE_METRICS
        true
#else
        false
#endif // OSMAND_PERFORMANCE_METRICS
        );
    
    int count = 0;
    
    const auto link_ = link.lock();
    if (!link_)
        return;
    const auto collection = static_cast<MapRendererTiledSymbolsResourcesCollection*>(&link_->collection);

    // Remove quick references
#if OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES
    for (const auto& symbolToResourceInGpuEntry : rangeOf(constOf(_symbolToResourceInGpuLUT)))
    {
        LogPrintf(LogSeverityLevel::Debug,
            "Removed GPU resource %p for map symbol %p in %p (unloadFromGPU)",
            symbolToResourceInGpuEntry.value().get(),
            symbolToResourceInGpuEntry.key().get(),
            this);
    }
#endif // OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES
    if (gpuContextLost)
    {
        for (auto& resourceInGPU : constOf(_symbolToResourceInGpuLUT))
            resourceInGPU->lostRefInGPU();
    }
    _symbolToResourceInGpuLUT.clear();

    // Unique
    for (const auto& groupResources : constOf(_uniqueGroupsResources))
    {
        if (gpuContextLost)
        {
            for (auto& resourceInGPU : constOf(groupResources->resourcesInGPU))
                resourceInGPU->lostRefInGPU();
        }

        // For unique group resources it's safe to clear 'MapSymbol->ResourceInGPU' map
        groupResources->resourcesInGPU.clear();
    }
    _uniqueGroupsResources.clear();

    // Shared
    auto& sharedGroupsResources = collection->_sharedGroupsResources[zoom];
    for (auto& groupResources : _referencedSharedGroupsResources)
    {
        MapSymbolsGroup::SharingKey sharingKey;
        groupResources->group->obtainSharingKey(sharingKey);
        count++;
        bool wasRemoved = false;
        uintmax_t* pRefsRemaining = nullptr;
#if OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
        uintmax_t refsRemaining = 0;
        pRefsRemaining = &refsRemaining;
#endif // OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE

        // Release reference first, since GroupResources will be released after GPU resource will be dereferenced,
        // other tile may catch empty non-loadable GroupResources.
        auto groupResources_ = groupResources;
        sharedGroupsResources.releaseReference(sharingKey, groupResources_, true, &wasRemoved, pRefsRemaining);

#if OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
        LogPrintf(LogSeverityLevel::Debug,
            "Shared GroupResources(%p) dereferenced for %s in %p (%dx%d@%d): %" PRIu64 " ref(s) remain, %s",
            groupResources.get(),
            qPrintable(symbolsGroup->getDebugTitle()),
            this,
            tileId.x, tileId.y, zoom,
            static_cast<uint64_t>(refsRemaining),
            wasRemoved ? "removed" : "not removed");
#endif // OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE

        // In case final reference to shared group resources was removed, it's safe to clear resources,
        // since no-one else will check for this map being empty. Otherwise, these resources are still needed
        // somewhere
        if (wasRemoved)
        {
            if (gpuContextLost)
            {
                for (auto& resourceInGPU : constOf(groupResources->resourcesInGPU))
                    resourceInGPU->lostRefInGPU();
            }

            groupResources->resourcesInGPU.clear();
        }
    }
    _referencedSharedGroupsResources.clear();
    
#if OSMAND_PERFORMANCE_METRICS
#if OSMAND_PERFORMANCE_METRICS <= 1
    auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    LogPrintf(LogSeverityLevel::Info, ">>>>> %ld: unloadFromGPU %d in %fs", millis, count, timer.elapsed());
#endif // OSMAND_PERFORMANCE_METRICS <= 1
#endif // OSMAND_PERFORMANCE_METRICS
}

void OsmAnd::MapRendererTiledSymbolsResource::unloadFromGPU()
{
    unloadFromGPU(false);
}

void OsmAnd::MapRendererTiledSymbolsResource::lostDataInGPU()
{
    unloadFromGPU(true);
}

void OsmAnd::MapRendererTiledSymbolsResource::releaseData()
{
    const auto link_ = link.lock();
    if (!link_)
        return;
    const auto collection = static_cast<MapRendererTiledSymbolsResourcesCollection*>(&link_->collection);
    const auto& self = shared_from_this();

    // Unregister all registered map symbols
    QList< PublishOrUnpublishMapSymbol > mapSymbolsToUnpublish;
    for (const auto& publishedMapSymbolsEntry : rangeOf(constOf(_publishedMapSymbolsByGroup)))
    {
        const auto& symbolsGroup = publishedMapSymbolsEntry.key();
        const auto& mapSymbols = publishedMapSymbolsEntry.value();
        mapSymbolsToUnpublish.reserve(mapSymbolsToUnpublish.size() + mapSymbols.size());
        for (const auto& mapSymbol : constOf(mapSymbols))
        {
            PublishOrUnpublishMapSymbol mapSymbolToUnpublish = {
                symbolsGroup,
                mapSymbol,
                self };
            mapSymbolsToUnpublish.push_back(mapSymbolToUnpublish);
        }
    }
    resourcesManager->batchUnpublishMapSymbols(mapSymbolsToUnpublish);
    _publishedMapSymbolsByGroup.clear();

    // Remove quick references (if any left)
#if OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES
    for (const auto& symbolToResourceInGpuEntry : rangeOf(constOf(_symbolToResourceInGpuLUT)))
    {
        LogPrintf(LogSeverityLevel::Debug,
            "Removed GPU resource %p for map symbol %p in %p (releaseData)",
            symbolToResourceInGpuEntry.value().get(),
            symbolToResourceInGpuEntry.key().get(),
            this);
    }
#endif // OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_CHANGES
    _symbolToResourceInGpuLUT.clear();

    // Unique
    for (const auto& groupResources : constOf(_uniqueGroupsResources))
    {
        // In case GPU resources was not released before, perform this operation here
        // Otherwise, groupResources->resourcesInGPU array is empty for unique group resources
        for (const auto& entryResourceInGPU : rangeOf(groupResources->resourcesInGPU))
        {
            const auto& symbol = entryResourceInGPU.key();
            auto& resourceInGPU = entryResourceInGPU.value();

            // Unload symbol from GPU thread (using dispatcher)
            auto resourceInGPU_ = qMove(resourceInGPU);
#ifndef Q_COMPILER_RVALUE_REFS
            resourceInGPU.reset();
#endif //!Q_COMPILER_RVALUE_REFS
            resourcesManager->renderer->getGpuThreadDispatcher().invokeAsync(
                [resourceInGPU_]
                () mutable
                {
                    resourceInGPU_.reset();
                });
        }
    }
    _uniqueGroupsResources.clear();

    // Shared
    auto& sharedGroupsResources = collection->_sharedGroupsResources[zoom];
    for (auto& groupResources : _referencedSharedGroupsResources)
    {
        MapSymbolsGroup::SharingKey sharingKey;
        groupResources->group->obtainSharingKey(sharingKey);

        // Otherwise, perform dereferencing
        uintmax_t* pRefsRemaining = nullptr;
#if OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
        uintmax_t refsRemaining = 0;
        pRefsRemaining = &refsRemaining;
#endif // OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
        bool wasRemoved = false;
        auto groupResources_ = groupResources;
        sharedGroupsResources.releaseReference(sharingKey, groupResources_, true, &wasRemoved, pRefsRemaining);

#if OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
        LogPrintf(LogSeverityLevel::Debug,
            "Shared GroupResources(%p) dereferenced for %s in %p (%dx%d@%d): %" PRIu64 " ref(s) remain, %s",
            groupResources.get(),
            qPrintable(symbolsGroup->getDebugTitle()),
            this,
            tileId.x, tileId.y, zoom,
            static_cast<uint64_t>(refsRemaining),
            wasRemoved ? "removed" : "not removed");
#endif // OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE

        // GPU resources of a shared group resources should be unloaded only in case this was the last
        // reference to shared group resources
        if (!wasRemoved)
            continue;

        // If this was the last reference to those shared group resources, it's safe to unload
        // all GPU resources assigned to it. And clear the entire mapping, since it's not needed
        // by anyone anywhere
        for (const auto& entryResourceInGPU : rangeOf(groupResources->resourcesInGPU))
        {
            const auto& symbol = entryResourceInGPU.key();
            auto& resourceInGPU = entryResourceInGPU.value();

            // Unload symbol from GPU thread (using dispatcher)
            auto resourceInGPU_ = qMove(resourceInGPU);
#ifndef Q_COMPILER_RVALUE_REFS
            resourceInGPU.reset();
#endif //!Q_COMPILER_RVALUE_REFS
            resourcesManager->renderer->getGpuThreadDispatcher().invokeAsync(
                [resourceInGPU_]
                () mutable
                {
                    resourceInGPU_.reset();
                });
        }
        groupResources->resourcesInGPU.clear();
    }
    _referencedSharedGroupsResources.clear();

    _retainableCacheMetadata.reset();
    _sourceData.reset();
}

std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU> OsmAnd::MapRendererTiledSymbolsResource::getGpuResourceFor(
    const std::shared_ptr<const MapSymbol>& mapSymbol) const
{
    QReadLocker scopedLocker(&_symbolToResourceInGpuLUTLock);

    const auto citResourceInGPU = _symbolToResourceInGpuLUT.constFind(mapSymbol);
    if (citResourceInGPU == _symbolToResourceInGpuLUT.cend())
    {
#if OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_REQUESTS
        LogPrintf(LogSeverityLevel::Debug,
            "GPU resource not found for map symbol %p in %p",
            mapSymbol.get(),
            this);
#endif // OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_REQUESTS
        return nullptr;
    }
#if OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_REQUESTS
    LogPrintf(LogSeverityLevel::Debug,
        "Found GPU resource %p for map symbol %p in %p",
        citResourceInGPU->get(),
        mapSymbol.get(),
        this);
#endif // OSMAND_LOG_MAP_SYMBOLS_TO_GPU_RESOURCES_MAP_REQUESTS
    return *citResourceInGPU;
}

OsmAnd::MapRendererTiledSymbolsResource::GroupResources::GroupResources(const std::shared_ptr<MapSymbolsGroup>& group_)
    : group(group_)
{
}

OsmAnd::MapRendererTiledSymbolsResource::GroupResources::~GroupResources()
{
#if OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
    LogPrintf(LogSeverityLevel::Debug, "Shared GroupResources(%p) destroyed", this);
#endif // OSMAND_LOG_SHARED_MAP_SYMBOLS_GROUPS_LIFECYCLE
}

OsmAnd::MapRendererTiledSymbolsResource::SharedGroupResources::SharedGroupResources(
    const std::shared_ptr<MapSymbolsGroup>& group)
    : GroupResources(group)
{
}

OsmAnd::MapRendererTiledSymbolsResource::SharedGroupResources::~SharedGroupResources()
{
}
