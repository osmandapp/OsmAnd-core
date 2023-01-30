#include "MapRendererTiledSymbolsResource.h"

#include "MapRenderer.h"
#include "MapRendererResourcesManager.h"
#include "IMapDataProvider.h"
#include "IMapTiledSymbolsProvider.h"
#include "RasterMapSymbol.h"
#include "MapRendererResourcesManager.h"
#include "MapRendererBaseResourcesCollection.h"
#include "MapRendererTiledSymbolsResourcesCollection.h"
#include "QKeyValueIterator.h"
#include "Utilities.h"

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
    return false;
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
        [provider, &sharedGroupsResources, &referencedSharedGroupsResources, &futureReferencedSharedGroupsResources, &loadedSharedGroups]
        (const IMapTiledSymbolsProvider*, const std::shared_ptr<const MapSymbolsGroup>& symbolsGroup) -> bool
        {
            // If map symbols group is not shareable, just accept it
            MapSymbolsGroup::SharingKey sharingKey;
            const auto isSharableById = symbolsGroup->obtainSharingKey(sharingKey);
            if (!isSharableById)
                return true;

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
    const auto requestSucceeded = provider->obtainTiledSymbols(request, tile);
    if (queryController && queryController->isAborted())
        return false;
    if (!requestSucceeded)
        return false;

    // Store data
    _sourceData = tile;
    dataAvailable = static_cast<bool>(tile);

    // Process data
    if (!dataAvailable)
        return true;

    // Convert data
    for (const auto& symbolsGroup : constOf(_sourceData->symbolsGroups))
    {
        if (queryController && queryController->isAborted())
            break;

        for (const auto& mapSymbol : constOf(symbolsGroup->symbols))
        {
            if (queryController && queryController->isAborted())
                break;

            const auto rasterMapSymbol = std::dynamic_pointer_cast<RasterMapSymbol>(mapSymbol);
            if (!rasterMapSymbol)
                continue;

            rasterMapSymbol->image = resourcesManager->adjustImageToConfiguration(
                rasterMapSymbol->image,
                AlphaChannelPresence::Present);
        }
    }
    if (queryController && queryController->isAborted())
        return false;

    // Move referenced shared groups
    _referencedSharedGroupsResources = referencedSharedGroupsResources;

    // tile->symbolsGroups contains groups that derived from unique symbols, or loaded shared groups
    for (const auto& symbolsGroup : constOf(_sourceData->symbolsGroups))
    {
        if (queryController && queryController->isAborted())
            break;

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
            auto groupResources = futureGroup.get();
            _referencedSharedGroupsResources.push_back(qMove(groupResources));
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
}

bool OsmAnd::MapRendererTiledSymbolsResource::uploadToGPU()
{
    typedef std::pair< std::shared_ptr<MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > SymbolResourceEntry;

    bool ok;
    bool anyUploadFailed = false;

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

    return true;
}

void OsmAnd::MapRendererTiledSymbolsResource::unloadFromGPU(bool gpuContextLost)
{
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
