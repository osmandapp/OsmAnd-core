#include "MapRendererTiledSymbolsResource.h"

#include "IMapDataProvider.h"
#include "IMapTiledSymbolsProvider.h"
#include "MapRendererResourcesManager.h"
#include "MapRendererBaseResourcesCollection.h"
#include "MapRendererTiledSymbolsResourcesCollection.h"
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
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);
    QList< std::shared_ptr<GroupResources> > referencedSharedGroupsResources;
    QList< proper::shared_future< std::shared_ptr<GroupResources> > > futureReferencedSharedGroupsResources;
    QSet< uint64_t > loadedSharedGroups;
    std::shared_ptr<const MapTiledSymbols> tile;
    const auto requestSucceeded = provider->obtainSymbols(tileId, zoom, tile,
        [this, provider, &sharedGroupsResources, &referencedSharedGroupsResources, &futureReferencedSharedGroupsResources, &loadedSharedGroups, tileBBox31]
        (const IMapSymbolProvider*, const std::shared_ptr<const Model::ObjectWithId>& object, const bool shareable) -> bool
        {
            if (!shareable)
                return true;

            // Check if this shared symbol is already available, or mark it as pending
            std::shared_ptr<GroupResources> sharedGroupResources;
            proper::shared_future< std::shared_ptr<GroupResources> > futureSharedGroupResources;
            if (sharedGroupsResources.obtainReferenceOrFutureReferenceOrMakePromise(object->id, sharedGroupResources, futureSharedGroupResources))
            {
                if (static_cast<bool>(sharedGroupResources))
                    referencedSharedGroupsResources.push_back(qMove(sharedGroupResources));
                else
                    futureReferencedSharedGroupsResources.push_back(qMove(futureSharedGroupResources));
                return false;
            }

            // Or load this shared group
            loadedSharedGroups.insert(object->id);
            return true;
        });
    if (!requestSucceeded)
        return false;

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
    for (auto& futureGroup : futureReferencedSharedGroupsResources)
    {
        auto groupResources = futureGroup.get();

        _referencedSharedGroupsResources.push_back(qMove(groupResources));
    }

    // Release source data:
    if (const auto retainedSource = std::dynamic_pointer_cast<const IRetainableResource>(_sourceData))
    {
        // If map tile implements 'Retained' interface, it must be kept, but 
        std::const_pointer_cast<IRetainableResource>(retainedSource)->releaseNonRetainedData();
    }
    else
    {
        // or simply release entire tile
        _sourceData.reset();
    }

    return true;
}

bool OsmAnd::MapRendererTiledSymbolsResource::uploadToGPU()
{
    typedef std::pair< std::shared_ptr<const MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > SymbolResourceEntry;
    typedef std::pair< std::shared_ptr<const MapSymbol>, proper::shared_future< std::shared_ptr<const GPUAPI::ResourceInGPU> > > FutureSymbolResourceEntry;

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

        // Basically, this check means "continue if shared resources where uploaded"
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

    // All resources have been uploaded to GPU successfully by this point

    // Unique
    for (const auto& entry : rangeOf(constOf(uniqueUploaded)))
    {
        const auto& groupResources = entry.key();
        auto& symbol = entry.value().first;
        auto& resource = entry.value().second;

        // Unload source data from symbol
        std::const_pointer_cast<MapSymbol>(symbol)->releaseNonRetainedData();

        // Publish symbol to global map
        owner->addMapSymbol(symbol, resource);

        // Move reference
        groupResources->resourcesInGPU.insert(qMove(symbol), qMove(resource));
    }

    // Shared
    for (const auto& entry : rangeOf(constOf(sharedUploaded)))
    {
        const auto& groupResources = entry.key();
        auto& symbol = entry.value().first;
        auto& resource = entry.value().second;

        // Unload source data from symbol
        std::const_pointer_cast<MapSymbol>(symbol)->releaseNonRetainedData();

        // Publish symbol to global map
        owner->addMapSymbol(symbol, resource);

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
        for (const auto& entryResourceInGPU : rangeOf(groupResources->resourcesInGPU))
        {
            const auto& symbol = entryResourceInGPU.key();
            auto& resourceInGPU = entryResourceInGPU.value();

            // Remove symbol from global map
            owner->removeMapSymbol(symbol);

            // Unload symbol from GPU
            assert(resourceInGPU.use_count() == 1);
            resourceInGPU.reset();
        }
    }
    _uniqueGroupsResources.clear();

    // Shared
    auto& sharedGroupsResources = collection->_sharedGroupsResources[zoom];
    for (auto& groupResources : _referencedSharedGroupsResources)
    {
        const auto shareableById = std::dynamic_pointer_cast<const MapSymbolsGroupShareableById>(groupResources->group);

        bool wasRemoved = false;
        auto groupResources_ = groupResources;
        sharedGroupsResources.releaseReference(shareableById->id, groupResources_, true, &wasRemoved);

        // Skip removing symbols from global map in case this was not the last reference
        // to shared group resources
        if (!wasRemoved)
            continue;

        for (const auto& entryResourceInGPU : rangeOf(groupResources->resourcesInGPU))
        {
            const auto& symbol = entryResourceInGPU.key();
            auto& resourceInGPU = entryResourceInGPU.value();

            // Remove symbol from global map
            owner->removeMapSymbol(symbol);

            // Unload symbol from GPU
            assert(resourceInGPU.use_count() == 1);
            resourceInGPU.reset();
        }
    }
    _referencedSharedGroupsResources.clear();
}

bool OsmAnd::MapRendererTiledSymbolsResource::checkIsSafeToUnlink()
{
    return _referencedSharedGroupsResources.isEmpty();
}

void OsmAnd::MapRendererTiledSymbolsResource::detach()
{
    const auto link_ = link.lock();
    const auto collection = static_cast<MapRendererTiledSymbolsResourcesCollection*>(&link_->collection);

    _uniqueGroupsResources.clear();

    auto& sharedGroupsResources = collection->_sharedGroupsResources[zoom];
    for (auto& groupResources : _referencedSharedGroupsResources)
    {
        const auto shareableById = std::dynamic_pointer_cast<const MapSymbolsGroupShareableById>(groupResources->group);

        bool wasRemoved = false;
        auto groupResources_ = groupResources;
        sharedGroupsResources.releaseReference(shareableById->id, groupResources_, true, &wasRemoved);

        // In case this was the last reference to shared group resources, check if any resources need to be deleted
        if (wasRemoved)
        {
            for (const auto& entryResourceInGPU : rangeOf(groupResources->resourcesInGPU))
            {
                const auto& symbol = entryResourceInGPU.key();
                auto& resourceInGPU = entryResourceInGPU.value();

                // Remove symbol from global map
                owner->removeMapSymbol(symbol);

                // Unload symbol from GPU thread (using dispatcher)
                assert(resourceInGPU.use_count() == 1);
                auto resourceInGPU_ = qMove(resourceInGPU);
                owner->renderer->getGpuThreadDispatcher().invokeAsync(
                    [resourceInGPU_]() mutable
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
}

OsmAnd::MapRendererTiledSymbolsResource::GroupResources::GroupResources(const std::shared_ptr<const MapSymbolsGroup>& group_)
    : group(group_)
{
}

OsmAnd::MapRendererTiledSymbolsResource::GroupResources::~GroupResources()
{
}
