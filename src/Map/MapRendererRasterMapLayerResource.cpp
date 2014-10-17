#include "MapRendererRasterMapLayerResource.h"

#include "IRasterMapLayerProvider.h"
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

bool OsmAnd::MapRendererRasterMapLayerResource::obtainData(bool& dataAvailable, const IQueryController* queryController)
{
    bool ok = false;

    // Get source of tile
    std::shared_ptr<IMapDataProvider> provider_;
    if (const auto link_ = link.lock())
        ok = resourcesManager->obtainProviderFor(static_cast<MapRendererBaseResourcesCollection*>(static_cast<MapRendererTiledResourcesCollection*>(&link_->collection)), provider_);
    if (!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapTiledDataProvider>(provider_);

    // Obtain tile from provider
    std::shared_ptr<IMapTiledDataProvider::Data> tiledData;
    const auto requestSucceeded = provider->obtainData(tileId, zoom, tiledData, queryController);
    if (!requestSucceeded)
        return false;
    dataAvailable = static_cast<bool>(tiledData);
    
    // Store data
    if (dataAvailable)
        _sourceData = std::static_pointer_cast<IRasterMapLayerProvider::Data>(tiledData);

    // Convert data if such is present
    if (_sourceData)
    {
        _sourceData->bitmap = resourcesManager->adjustBitmapToConfiguration(
            _sourceData->bitmap,
            _sourceData->alphaChannelData);
    }

    return true;
}

bool OsmAnd::MapRendererRasterMapLayerResource::uploadToGPU()
{
    bool ok = resourcesManager->uploadTiledDataToGPU(_sourceData, _resourceInGPU);
    if (!ok)
        return false;

    // Since content was uploaded to GPU, it's safe to release it keeping retainable data
    _retainableCacheMetadata = _sourceData->retainableCacheMetadata;
    _sourceData.reset();

    return true;
}

void OsmAnd::MapRendererRasterMapLayerResource::unloadFromGPU()
{
    _resourceInGPU.reset();
}

void OsmAnd::MapRendererRasterMapLayerResource::releaseData()
{
    _retainableCacheMetadata.reset();
    _sourceData.reset();
}
