#include "MapRendererKeyedSymbolsResource.h"

OsmAnd::MapRendererKeyedSymbolsResource::MapRendererKeyedSymbolsResource(MapRendererResourcesManager* owner, const KeyedEntriesCollection<Key, MapRendererBaseKeyedResource>& collection, const Key key)
    : MapRendererBaseKeyedResource(owner, MapRendererResourceType::Symbols, collection, key)
{
}

OsmAnd::MapRendererKeyedSymbolsResource::~MapRendererKeyedSymbolsResource()
{
    safeUnlink();
}

bool OsmAnd::MapRendererKeyedSymbolsResource::obtainData(bool& dataAvailable, const IQueryController* queryController)
{
    // Obtain collection link and maintain it
    const auto link_ = link.lock();
    if (!link_)
        return false;
    const auto collection = static_cast<MapRendererKeyedResourcesCollection*>(&link_->collection);

    // Get source
    std::shared_ptr<IMapDataProvider> provider_;
    bool ok = owner->obtainProviderFor(static_cast<MapRendererBaseResourcesCollection*>(collection), provider_);
    if (!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapKeyedSymbolsProvider>(provider_);

    // Obtain source data from provider
    std::shared_ptr<const MapSymbolsGroup> sourceData;
    const auto requestSucceeded = provider->obtainSymbolsGroup(key, sourceData);
    if (!requestSucceeded)
        return false;

    // Store data
    _sourceData = sourceData;
    dataAvailable = static_cast<bool>(_sourceData);

    // Process data
    if (!dataAvailable)
        return true;

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

bool OsmAnd::MapRendererKeyedSymbolsResource::uploadToGPU()
{
    bool ok;
    bool anyUploadFailed = false;

    const auto link_ = link.lock();
    const auto collection = static_cast<MapRendererKeyedResourcesCollection*>(&link_->collection);

    QHash< std::shared_ptr<const MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > uploaded;
    for (const auto& symbol : constOf(_sourceData->symbols))
    {
        // Prepare data and upload to GPU
        assert(static_cast<bool>(symbol->bitmap));
        std::shared_ptr<const GPUAPI::ResourceInGPU> resourceInGPU;
        ok = owner->uploadSymbolToGPU(symbol, resourceInGPU);

        // If upload have failed, stop
        if (!ok)
        {
            LogPrintf(LogSeverityLevel::Error, "Failed to upload keyed symbol (size %dx%d)",
                symbol->bitmap->width(), symbol->bitmap->height());

            anyUploadFailed = true;
            break;
        }

        // Mark this symbol as uploaded
        uploaded.insert(symbol, qMove(resourceInGPU));
    }

    // If at least one symbol failed to upload, consider entire tile as failed to upload,
    // and unload its partial GPU resources
    if (anyUploadFailed)
    {
        uploaded.clear();

        return false;
    }

    // All resources have been uploaded to GPU successfully by this point

    for (const auto& entry : rangeOf(constOf(uploaded)))
    {
        auto& symbol = entry.key();
        auto& resource = entry.value();

        // Unload source data from symbol
        std::const_pointer_cast<MapSymbol>(symbol)->releaseNonRetainedData();

        // Publish symbol to global map
        owner->addMapSymbol(symbol, resource);

        // Move reference
        _resourcesInGPU.insert(qMove(symbol), qMove(resource));
    }

    return true;
}

void OsmAnd::MapRendererKeyedSymbolsResource::unloadFromGPU()
{
    return;
}

bool OsmAnd::MapRendererKeyedSymbolsResource::checkIsSafeToUnlink()
{
    return false;
}

void OsmAnd::MapRendererKeyedSymbolsResource::detach()
{
    return;
}
