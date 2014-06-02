#include "MapRendererTiledSymbolsResource.h"

#include "MapRenderer.h"
#include "MapRendererResourcesManager.h"
#include "IMapDataProvider.h"
#include "IMapTiledSymbolsProvider.h"
#include "MapSymbolsGroupShareableById.h"
#include "MapRendererResourcesManager.h"
#include "MapRendererBaseResourcesCollection.h"
#include "MapRendererTiledSymbolsResourcesCollection.h"
#include "QKeyValueIterator.h"
#include "Utilities.h"

OsmAnd::MapRendererTiledSymbolsResource::MapRendererTiledSymbolsResource(
    MapRendererResourcesManager* owner,
    const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
    const TileId tileId,
    const ZoomLevel zoom)
    : MapRendererBaseTiledResource(owner, MapRendererResourceType::Symbols, collection, tileId, zoom)
{
}

OsmAnd::MapRendererTiledSymbolsResource::~MapRendererTiledSymbolsResource()
{
    safeUnlink();
}

bool OsmAnd::MapRendererTiledSymbolsResource::obtainData(bool& dataAvailable, const IQueryController* queryController)
{
    // Obtain collection link and maintain it
    const auto link_ = link.lock();
    if (!link_)
        return false;
    const auto collection = static_cast<MapRendererTiledSymbolsResourcesCollection*>(&link_->collection);

    // Get source of tile
    std::shared_ptr<IMapDataProvider> provider_;
    bool ok = owner->obtainProviderFor(static_cast<MapRendererBaseResourcesCollection*>(collection), provider_);
    if (!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapTiledSymbolsProvider>(provider_);

    auto& sharedGroupsResources = collection->_sharedGroupsResources[zoom];

    // Obtain tile from provider
    QList< std::shared_ptr<GroupResources> > referencedSharedGroupsResources;
    QList< proper::shared_future< std::shared_ptr<GroupResources> > > futureReferencedSharedGroupsResources;
    QSet< uint64_t > loadedSharedGroups;
    std::shared_ptr<MapTiledData> tile_;
    const auto requestSucceeded = provider->obtainData(tileId, zoom, tile_,
        [this, provider, &sharedGroupsResources, &referencedSharedGroupsResources, &futureReferencedSharedGroupsResources, &loadedSharedGroups]
        (const IMapTiledSymbolsProvider*, const std::shared_ptr<const MapSymbolsGroup>& symbolsGroup_) -> bool
        {
            const auto symbolsGroup = std::dynamic_pointer_cast<const MapSymbolsGroupShareableById>(symbolsGroup_);

            // If not shareable, just accept it
            if (!symbolsGroup)
                return true;

            // Check if this shared symbol is already available, or mark it as pending
            std::shared_ptr<GroupResources> sharedGroupResources;
            proper::shared_future< std::shared_ptr<GroupResources> > futureSharedGroupResources;
            if (sharedGroupsResources.obtainReferenceOrFutureReferenceOrMakePromise(symbolsGroup->id, sharedGroupResources, futureSharedGroupResources))
            {
                if (static_cast<bool>(sharedGroupResources))
                    referencedSharedGroupsResources.push_back(qMove(sharedGroupResources));
                else
                    futureReferencedSharedGroupsResources.push_back(qMove(futureSharedGroupResources));
                return false;
            }

            // Or load this shared group
            loadedSharedGroups.insert(symbolsGroup->id);
            return true;
        });
    if (!requestSucceeded)
        return false;
    const auto tile = std::static_pointer_cast<TiledMapSymbolsData>(tile_);

    // Store data
    _sourceData = tile;
    dataAvailable = static_cast<bool>(tile);

    // Process data
    if (!dataAvailable)
        return true;

    // Move referenced shared groups
    _referencedSharedGroupsResources = referencedSharedGroupsResources;

    // tile->symbolsGroups contains groups that derived from unique symbols, or loaded shared groups
    for (const auto& group : constOf(tile->symbolsGroups))
    {
        std::shared_ptr<GroupResources> groupResources(new GroupResources(group));

        if (const auto shareableById = std::dynamic_pointer_cast<const MapSymbolsGroupShareableById>(group))
        {
            // Check if this group is loaded as shared
            if (!loadedSharedGroups.contains(shareableById->id))
            {
                _uniqueGroupsResources.push_back(qMove(groupResources));
                continue;
            }

            // Otherwise insert it as shared group
            sharedGroupsResources.fulfilPromiseAndReference(shareableById->id, groupResources);
            _referencedSharedGroupsResources.push_back(qMove(groupResources));
        }
        else
        {
            _uniqueGroupsResources.push_back(groupResources);
        }
    }

    // Wait for future referenced shared groups
    for (auto& futureGroup : constOf(futureReferencedSharedGroupsResources))
    {
        auto groupResources = futureGroup.get();

        _referencedSharedGroupsResources.push_back(qMove(groupResources));
    }

    // Register all obtained symbols
    for (const auto& groupResources : constOf(_uniqueGroupsResources))
    {
        for (const auto& symbol : constOf(groupResources->group->symbols))
            owner->registerMapSymbol(symbol, shared_from_this());
    }
    for (const auto& groupResources : constOf(_uniqueGroupsResources))
    {
        for (const auto& symbol : constOf(groupResources->group->symbols))
            owner->registerMapSymbol(symbol, shared_from_this());
    }

    // Since there's a copy of references to map symbols groups and symbols themselves,
    // it's safe to consume all the data here
    _sourceData->releaseConsumableContent();

    return true;
}

bool OsmAnd::MapRendererTiledSymbolsResource::uploadToGPU()
{
    typedef std::pair< std::shared_ptr<MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > SymbolResourceEntry;
    typedef std::pair< std::shared_ptr<MapSymbol>, proper::shared_future< std::shared_ptr<const GPUAPI::ResourceInGPU> > > FutureSymbolResourceEntry;

    bool ok;
    bool anyUploadFailed = false;

    const auto link_ = link.lock();
    const auto collection = static_cast<MapRendererTiledSymbolsResourcesCollection*>(&link_->collection);

    // Unique
    QMultiHash< std::shared_ptr<GroupResources>, SymbolResourceEntry > uniqueUploaded;
    for (const auto& groupResources : constOf(_uniqueGroupsResources))
    {
        for (const auto& symbol : constOf(groupResources->group->symbols))
        {
            // Prepare data and upload to GPU
            assert(static_cast<bool>(symbol->bitmap));
            std::shared_ptr<const GPUAPI::ResourceInGPU> resourceInGPU;
            ok = owner->uploadSymbolToGPU(symbol, resourceInGPU);

            // If upload have failed, stop
            if (!ok)
            {
                LogPrintf(LogSeverityLevel::Error, "Failed to upload unique symbol (size %dx%d) in %dx%d@%d tile",
                    symbol->bitmap->width(), symbol->bitmap->height(),
                    tileId.x, tileId.y, zoom);

                anyUploadFailed = true;
                break;
            }

            // Mark this symbol as uploaded
            uniqueUploaded.insertMulti(groupResources, qMove(SymbolResourceEntry(symbol, resourceInGPU)));
        }
        if (anyUploadFailed)
            break;
    }

    // Shared
    QMultiHash< std::shared_ptr<GroupResources>, SymbolResourceEntry > sharedUploaded;
    for (const auto& groupResources : constOf(_referencedSharedGroupsResources))
    {
        if (groupResources->group->symbols.isEmpty())
            continue;

        // Basically, this check means "continue if shared symbols were uploaded by other resource"
        // This check needs no special handling, since all GPU resources work is done from same thread.
        if (!groupResources->resourcesInGPU.isEmpty())
            continue;

        for (const auto& symbol : constOf(groupResources->group->symbols))
        {
            // Prepare data and upload to GPU
            assert(static_cast<bool>(symbol->bitmap));
            std::shared_ptr<const GPUAPI::ResourceInGPU> resourceInGPU;
            ok = owner->uploadSymbolToGPU(symbol, resourceInGPU);

            // If upload have failed, stop
            if (!ok)
            {
                LogPrintf(LogSeverityLevel::Error, "Failed to upload unique symbol (size %dx%d) in %dx%d@%d tile",
                    symbol->bitmap->width(), symbol->bitmap->height(),
                    tileId.x, tileId.y, zoom);

                anyUploadFailed = true;
                break;
            }

            // Mark this symbol as uploaded
            sharedUploaded.insertMulti(groupResources, qMove(SymbolResourceEntry(symbol, resourceInGPU)));
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

        // Unload bitmap from symbol, since it's uploaded already
        symbol->bitmap.reset();

        // Move reference
        groupResources->resourcesInGPU.insert(qMove(symbol), qMove(resource));
    }

    // Shared
    for (const auto& entry : rangeOf(constOf(sharedUploaded)))
    {
        const auto& groupResources = entry.key();
        auto symbol = entry.value().first;
        auto& resource = entry.value().second;

        // Unload bitmap from symbol, since it's uploaded already
        symbol->bitmap.reset();

        // Move reference
        groupResources->resourcesInGPU.insert(qMove(symbol), qMove(resource));
    }

    return true;
}

void OsmAnd::MapRendererTiledSymbolsResource::unloadFromGPU()
{
    const auto link_ = link.lock();
    const auto collection = static_cast<MapRendererTiledSymbolsResourcesCollection*>(&link_->collection);

    // Unique
    for (const auto& groupResources : constOf(_uniqueGroupsResources))
    {
        // Unload map symbol resources from GPU
#if OSMAND_DEBUG
        for (const auto& entryResourceInGPU : rangeOf(groupResources->resourcesInGPU))
        {
            const auto& symbol = entryResourceInGPU.key();
            auto& resourceInGPU = entryResourceInGPU.value();

            assert(resourceInGPU.use_count() == 1);
            resourceInGPU.reset();
        }
#else
        groupResources->resourcesInGPU.clear();
#endif
    }

    // Shared
    auto& sharedGroupsResources = collection->_sharedGroupsResources[zoom];
    for (auto& groupResources : _referencedSharedGroupsResources)
    {
        const auto shareableById = std::dynamic_pointer_cast<const MapSymbolsGroupShareableById>(groupResources->group);

        auto groupResources_ = groupResources;
        sharedGroupsResources.releaseReference(shareableById->id, groupResources_, true);

        // Release reference to GPU resources
        // If any reference to resource is last, GPU resource is unloaded
        groupResources->resourcesInGPU.clear();
    }
}

void OsmAnd::MapRendererTiledSymbolsResource::releaseData()
{
    const auto link_ = link.lock();
    const auto collection = static_cast<MapRendererTiledSymbolsResourcesCollection*>(&link_->collection);

    // Unregister all obtained unique symbols
    for (const auto& groupResources : constOf(_uniqueGroupsResources))
    {
        for (const auto& symbol : constOf(groupResources->group->symbols))
            owner->unregisterMapSymbol(symbol, shared_from_this());
    }
    _uniqueGroupsResources.clear();

    auto& sharedGroupsResources = collection->_sharedGroupsResources[zoom];
    for (auto& groupResources : _referencedSharedGroupsResources)
    {
        const auto shareableById = std::dynamic_pointer_cast<const MapSymbolsGroupShareableById>(groupResources->group);

        bool wasRemoved = false;
        auto groupResources_ = groupResources;
        sharedGroupsResources.releaseReference(shareableById->id, groupResources_, true, &wasRemoved);

        // Unregister symbols
        for (const auto& symbol : constOf(groupResources->group->symbols))
            owner->unregisterMapSymbol(symbol, shared_from_this());

        for (const auto& entryResourceInGPU : rangeOf(groupResources->resourcesInGPU))
        {
            const auto& symbol = entryResourceInGPU.key();
            auto& resourceInGPU = entryResourceInGPU.value();

            // In case this was the last reference to shared group resources, check if any resources need to be deleted
            if (wasRemoved)
            {
                // Unload symbol from GPU thread (using dispatcher)
                assert(resourceInGPU.use_count() == 1);
                auto resourceInGPU_ = qMove(resourceInGPU);
                owner->renderer->getGpuThreadDispatcher().invokeAsync(
                    [resourceInGPU_]
                    () mutable
                    {
                        assert(resourceInGPU_.use_count() == 1);
                        resourceInGPU_.reset();
                    });
#ifndef Q_COMPILER_RVALUE_REFS
                resourceInGPU.reset();
#endif //!Q_COMPILER_RVALUE_REFS
            }
        }
    }
    _referencedSharedGroupsResources.clear();

    _sourceData.reset();
}

std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU> OsmAnd::MapRendererTiledSymbolsResource::getGpuResourceFor(const std::shared_ptr<const MapSymbol>& mapSymbol)
{
    return nullptr;
}

OsmAnd::MapRendererTiledSymbolsResource::GroupResources::GroupResources(const std::shared_ptr<MapSymbolsGroup>& group_)
    : group(group_)
{
}

OsmAnd::MapRendererTiledSymbolsResource::GroupResources::~GroupResources()
{
}
