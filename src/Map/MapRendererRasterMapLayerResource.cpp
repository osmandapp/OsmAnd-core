#include "MapRendererRasterMapLayerResource.h"

#include "IRasterMapLayerProvider.h"
#include "MapRenderer.h"
#include "MapRendererResourcesManager.h"

OsmAnd::MapRendererRasterMapLayerResource::MapRendererRasterMapLayerResource(
    MapRendererResourcesManager* owner_,
    const TiledEntriesCollection<MapRendererBaseTiledResource>& collection_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : MapRendererBaseTiledResource(owner_, MapRendererResourceType::MapLayer, collection_, tileId_, zoom_)
    , resourceInGPU(_resourceInGPU)
{
}

OsmAnd::MapRendererRasterMapLayerResource::~MapRendererRasterMapLayerResource()
{
    safeUnlink();
}

bool OsmAnd::MapRendererRasterMapLayerResource::supportsObtainDataAsync() const
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

bool OsmAnd::MapRendererRasterMapLayerResource::obtainData(
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
    std::shared_ptr<IMapTiledDataProvider::Data> tiledData;
    IRasterMapLayerProvider::Request request;
    request.tileId = tileId;
    request.zoom = zoom;
    request.queryController = queryController;
    const auto requestSucceeded = provider->obtainTiledData(request, tiledData);
    if (!requestSucceeded)
        return false;
    dataAvailable = static_cast<bool>(tiledData);
    
    // Store data
    if (dataAvailable)
        _sourceData = std::static_pointer_cast<IRasterMapLayerProvider::Data>(tiledData);

    // Convert data if such is present
    if (_sourceData)
    {
        for (auto image : _sourceData->images)
        {
            image = resourcesManager->adjustImageToConfiguration(
                image, _sourceData->alphaChannelPresence);
        }
    }

    return true;
}

void OsmAnd::MapRendererRasterMapLayerResource::obtainDataAsync(
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

    const auto self = std::static_pointer_cast<MapRendererRasterMapLayerResource>(shared_from_this());
    const auto weakSelf = std::weak_ptr<MapRendererRasterMapLayerResource>(self);
    const auto weakManager =
        std::weak_ptr<MapRendererResourcesManager>(resourcesManager->renderer->getResourcesSharedPtr());

    IRasterMapLayerProvider::Request request;
    request.tileId = tileId;
    request.zoom = zoom;
    request.cacheOnly = cacheOnly;
    request.queryController = queryController;
    provider->obtainDataAsync(request,
        [weakSelf, weakManager, callback]
        (const IMapDataProvider* const provider,
            const bool requestSucceeded,
            const std::shared_ptr<IMapDataProvider::Data>& data,
            const std::shared_ptr<Metric>& metric)
        {
            const auto dataAvailable = static_cast<bool>(data);

            // Store data
            if (dataAvailable)
            {
                const auto self = weakSelf.lock();
                if (!self)
                {
                    callback(false, false);
                    return;
                }
                if (const auto resourcesManager = weakManager.lock())
                {
                    self->_sourceData = std::static_pointer_cast<IRasterMapLayerProvider::Data>(data);

                    // Convert data if such is present
                    if (self->_sourceData)
                    {
                        for (auto image : self->_sourceData->images)
                        {
                            image = resourcesManager->adjustImageToConfiguration(
                                image, self->_sourceData->alphaChannelPresence);
                        }
                    }
                }
                else
                    return;
            }
            callback(requestSucceeded, dataAvailable);
        });
}

bool OsmAnd::MapRendererRasterMapLayerResource::uploadToGPU()
{
    auto sourceData = _sourceData;
    if (!sourceData)
        return true;

    bool ok = resourcesManager->uploadTiledDataToGPU(sourceData, _resourceInGPU, shared_from_this());
    if (!ok)
        return false;

    // Since content was uploaded to GPU, it's safe to release it keeping retainable data
    _retainableCacheMetadata = sourceData->retainableCacheMetadata;
    // Remove data in case it contains constant (single) raster image only
    if (sourceData->images.size() <= 1)
        sourceData.reset();

    return true;
}

void OsmAnd::MapRendererRasterMapLayerResource::unloadFromGPU()
{
    _resourceInGPU.reset();
}

void OsmAnd::MapRendererRasterMapLayerResource::lostDataInGPU()
{
    if (_resourceInGPU)
        _resourceInGPU->lostRefInGPU();
    _resourceInGPU.reset();
}

void OsmAnd::MapRendererRasterMapLayerResource::releaseData()
{
    _retainableCacheMetadata.reset();
    _sourceData.reset();
}
