#include "MapRendererElevationDataResource.h"

#include "IMapElevationDataProvider.h"
#include "MapRendererResourcesManager.h"

OsmAnd::MapRendererElevationDataResource::MapRendererElevationDataResource(
    MapRendererResourcesManager* owner_,
    const TiledEntriesCollection<MapRendererBaseTiledResource>& collection_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : MapRendererBaseTiledResource(owner_, MapRendererResourceType::ElevationData, collection_, tileId_, zoom_)
    , sourceData(_sourceData)
{
}

OsmAnd::MapRendererElevationDataResource::~MapRendererElevationDataResource()
{
    safeUnlink();
}

bool OsmAnd::MapRendererElevationDataResource::supportsObtainDataAsync() const
{
    bool ok = false;

    std::shared_ptr<IMapDataProvider> provider;
    if (const auto link_ = link.lock())
    {
        ok = resourcesManager->obtainProviderFor(
            static_cast<MapRendererBaseResourcesCollection*>(static_cast<MapRendererTiledResourcesCollection*>(&link_->collection)),
            provider);
    }
    if (!ok)
        return false;

    return provider->supportsNaturalObtainDataAsync();
}

bool OsmAnd::MapRendererElevationDataResource::obtainData(
    bool& dataAvailable,
    const std::shared_ptr<const IQueryController>& queryController)
{
    bool ok = false;

    // Get source of tile
    std::shared_ptr<IMapDataProvider> provider_;
    if (const auto link_ = link.lock())
    {
        ok = resourcesManager->obtainProviderFor(
            static_cast<MapRendererBaseResourcesCollection*>(static_cast<MapRendererTiledResourcesCollection*>(&link_->collection)),
            provider_);
    }
    if (!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapTiledDataProvider>(provider_);

    // Obtain tile from provider
    std::shared_ptr<IMapTiledDataProvider::Data> tile;
    IMapElevationDataProvider::Request request;
    request.tileId = tileId;
    request.zoom = zoom;
    request.queryController = queryController;
    const auto requestSucceeded = provider->obtainTiledData(request, tile);
    if (!requestSucceeded)
        return false;
    dataAvailable = static_cast<bool>(tile);

    // Store data
    if (dataAvailable)
        _sourceData = std::static_pointer_cast<IMapElevationDataProvider::Data>(tile);

    return true;
}

void OsmAnd::MapRendererElevationDataResource::obtainDataAsync(
    const ObtainDataAsyncCallback callback,
    const std::shared_ptr<const IQueryController>& queryController,
    const bool cacheOnly /*=false*/)
{
    bool ok = false;

    // Get source of tile
    std::shared_ptr<IMapDataProvider> provider_;
    if (const auto link_ = link.lock())
    {
        ok = resourcesManager->obtainProviderFor(
            static_cast<MapRendererBaseResourcesCollection*>(static_cast<MapRendererTiledResourcesCollection*>(&link_->collection)),
            provider_);
    }
    if (!ok)
    {
        callback(false, false);
        return;
    }
    const auto provider = std::static_pointer_cast<IMapTiledDataProvider>(provider_);

    IMapElevationDataProvider::Request request;
    request.tileId = tileId;
    request.zoom = zoom;
    request.queryController = queryController;
    provider->obtainDataAsync(request,
        [this, callback]
        (const IMapDataProvider* const provider,
            const bool requestSucceeded,
            const std::shared_ptr<IMapDataProvider::Data>& data,
            const std::shared_ptr<Metric>& metric)
        {
            const auto dataAvailable = static_cast<bool>(data);

            // Store data
            if (dataAvailable)
                _sourceData = std::static_pointer_cast<IMapElevationDataProvider::Data>(data);

            callback(requestSucceeded, dataAvailable);
        });
}

bool OsmAnd::MapRendererElevationDataResource::uploadToGPU()
{
    bool ok = resourcesManager->uploadTiledDataToGPU(_sourceData, _resourceInGPU);
    if (!ok)
        return false;

    // NOTE: Don't release source data since it's needed for some on-CPU computations

    return true;
}

void OsmAnd::MapRendererElevationDataResource::captureResourceInGPU(
    std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU>& resourceInGPU) const
{
    if (resourceInGPULock.testAndSetAcquire(0, 1))
    {
        resourceInGPU = _resourceInGPU;
        resourceInGPULock.storeRelease(0);
    }
}

void OsmAnd::MapRendererElevationDataResource::unloadFromGPU()
{
    REPEAT_UNTIL(resourceInGPULock.testAndSetAcquire(0, 1))
    {
        _resourceInGPU.reset();
        resourceInGPULock.storeRelease(0);
    }
}

void OsmAnd::MapRendererElevationDataResource::lostDataInGPU()
{
    if (_resourceInGPU)
        _resourceInGPU->lostRefInGPU();
    _resourceInGPU.reset();
}

void OsmAnd::MapRendererElevationDataResource::releaseData()
{
    _sourceData.reset();
}
