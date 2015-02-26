#include "MapRendererKeyedSymbolsResource.h"

#include "IMapDataProvider.h"
#include "IMapKeyedDataProvider.h"
#include "IMapKeyedSymbolsProvider.h"
#include "MapSymbol.h"
#include "RasterMapSymbol.h"
#include "MapSymbolsGroup.h"
#include "IUpdatableMapSymbolsGroup.h"
#include "MapRendererKeyedResourcesCollection.h"
#include "MapRendererResourcesManager.h"
#include "QKeyValueIterator.h"

OsmAnd::MapRendererKeyedSymbolsResource::MapRendererKeyedSymbolsResource(
    MapRendererResourcesManager* owner_,
    const IMapDataProvider::SourceType sourceType_,
    const KeyedEntriesCollection<Key, MapRendererBaseKeyedResource>& collection_,
    const Key key_)
    : MapRendererBaseKeyedResource(owner_, MapRendererResourceType::Symbols, sourceType_, collection_, key_)
{
}

OsmAnd::MapRendererKeyedSymbolsResource::~MapRendererKeyedSymbolsResource()
{
    safeUnlink();
}

bool OsmAnd::MapRendererKeyedSymbolsResource::updatesPresent()
{
    bool updatesPresent = MapRendererBaseKeyedResource::updatesPresent();

    if (const auto updatableMapSymbolGroup = std::dynamic_pointer_cast<IUpdatableMapSymbolsGroup>(_mapSymbolsGroup))
    {
        if (updatableMapSymbolGroup->updatesPresent())
            updatesPresent = true;
    }

    return updatesPresent;
}

bool OsmAnd::MapRendererKeyedSymbolsResource::checkForUpdatesAndApply()
{
    bool updatesApplied = MapRendererBaseKeyedResource::checkForUpdatesAndApply();

    if (const auto updatableMapSymbolGroup = std::dynamic_pointer_cast<IUpdatableMapSymbolsGroup>(_mapSymbolsGroup))
    {
        if (updatableMapSymbolGroup->update())
            updatesApplied = true;
    }

    return updatesApplied;
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
    bool ok = resourcesManager->obtainProviderFor(static_cast<MapRendererBaseResourcesCollection*>(collection), provider_);
    if (!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapKeyedDataProvider>(provider_);

    // Obtain source data from provider
    std::shared_ptr<IMapKeyedDataProvider::Data> keyedData;
    const auto requestSucceeded = provider->obtainData(key, keyedData);
    if (!requestSucceeded)
        return false;

    // Store data
    dataAvailable = static_cast<bool>(keyedData);
    if (dataAvailable)
        _sourceData = std::static_pointer_cast<IMapKeyedSymbolsProvider::Data>(keyedData);

    // Process data
    if (!dataAvailable)
        return true;

    // Convert data
    for (const auto& mapSymbol : constOf(_sourceData->symbolsGroup->symbols))
    {
        const auto rasterMapSymbol = std::dynamic_pointer_cast<RasterMapSymbol>(mapSymbol);
        if (!rasterMapSymbol)
            continue;

        rasterMapSymbol->bitmap = resourcesManager->adjustBitmapToConfiguration(
            rasterMapSymbol->bitmap,
            AlphaChannelPresence::Present);
    }

    // Register all obtained symbols
    _mapSymbolsGroup = _sourceData->symbolsGroup;
    const auto self = shared_from_this();
    QList< PublishOrUnpublishMapSymbol > mapSymbolsToPublish;
    mapSymbolsToPublish.reserve(_mapSymbolsGroup->symbols.size());
    for (const auto& symbol : constOf(_mapSymbolsGroup->symbols))
    {
        PublishOrUnpublishMapSymbol mapSymbolToPublish = {
            _mapSymbolsGroup,
            std::static_pointer_cast<const MapSymbol>(symbol),
            self };
        mapSymbolsToPublish.push_back(mapSymbolToPublish);
    }
    resourcesManager->batchPublishMapSymbols(mapSymbolsToPublish);

    return true;
}

bool OsmAnd::MapRendererKeyedSymbolsResource::uploadToGPU()
{
    bool ok;
    bool anyUploadFailed = false;

    const auto link_ = link.lock();
    const auto collection = static_cast<MapRendererKeyedResourcesCollection*>(&link_->collection);

    QHash< std::shared_ptr<MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > uploaded;
    for (const auto& symbol : constOf(_sourceData->symbolsGroup->symbols))
    {
        // Prepare data and upload to GPU
        std::shared_ptr<const GPUAPI::ResourceInGPU> resourceInGPU;
        ok = resourcesManager->uploadSymbolToGPU(symbol, resourceInGPU);

        // If upload have failed, stop
        if (!ok)
        {
            LogPrintf(LogSeverityLevel::Error, "Failed to upload keyed symbol");

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
    _retainableCacheMetadata = _sourceData->retainableCacheMetadata;
    _sourceData.reset();

    for (const auto& entry : rangeOf(constOf(uploaded)))
    {
        const auto& symbol = entry.key();
        auto& resource = entry.value();

        // Unload GPU data from symbol, since it's uploaded already
        resourcesManager->releaseGpuUploadableDataFrom(symbol);

        // Move reference
        _resourcesInGPU.insert(symbol, qMove(resource));
    }

    return true;
}

void OsmAnd::MapRendererKeyedSymbolsResource::unloadFromGPU()
{
    _resourcesInGPU.clear();
}

void OsmAnd::MapRendererKeyedSymbolsResource::lostDataInGPU()
{
    for (auto& resourceInGPU : constOf(_resourcesInGPU))
        resourceInGPU->lostRefInGPU();
    _resourcesInGPU.clear();
}

void OsmAnd::MapRendererKeyedSymbolsResource::releaseData()
{
    // Unregister all obtained symbols
    const auto self = shared_from_this();
    QList< PublishOrUnpublishMapSymbol > mapSymbolsToUnpublish;
    mapSymbolsToUnpublish.reserve(_mapSymbolsGroup->symbols.size());
    for (const auto& symbol : constOf(_mapSymbolsGroup->symbols))
    {
        PublishOrUnpublishMapSymbol mapSymbolToUnpublish = {
            _mapSymbolsGroup,
            std::static_pointer_cast<const MapSymbol>(symbol),
            self };
        mapSymbolsToUnpublish.push_back(mapSymbolToUnpublish);
    }
    resourcesManager->batchUnpublishMapSymbols(mapSymbolsToUnpublish);

    _mapSymbolsGroup.reset();

    _retainableCacheMetadata.reset();
    _sourceData.reset();
}

std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU> OsmAnd::MapRendererKeyedSymbolsResource::getGpuResourceFor(
    const std::shared_ptr<const MapSymbol>& mapSymbol) const
{
    const auto citResourceInGPU = _resourcesInGPU.constFind(mapSymbol);
    if (citResourceInGPU == _resourcesInGPU.cend())
        return nullptr;
    return *citResourceInGPU;
}
