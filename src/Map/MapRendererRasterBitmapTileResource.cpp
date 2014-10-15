#include "MapRendererRasterBitmapTileResource.h"

#include "IMapRasterBitmapTileProvider.h"
#include "MapRendererResourcesManager.h"

OsmAnd::MapRendererRasterBitmapTileResource::MapRendererRasterBitmapTileResource(
    MapRendererResourcesManager* owner_,
    const TiledEntriesCollection<MapRendererBaseTiledResource>& collection_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : MapRendererBaseTiledResource(owner_, MapRendererResourceType::MapLayer, collection_, tileId_, zoom_)
    , resourceInGPU(_resourceInGPU)
{
}

OsmAnd::MapRendererRasterBitmapTileResource::~MapRendererRasterBitmapTileResource()
{
    safeUnlink();
}

bool OsmAnd::MapRendererRasterBitmapTileResource::obtainData(bool& dataAvailable, const IQueryController* queryController)
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
    std::shared_ptr<MapTiledData> tile;
    const auto requestSucceeded = provider->obtainData(tileId, zoom, tile, queryController);
    if (!requestSucceeded)
        return false;

    // Store data
    _sourceData = std::static_pointer_cast<RasterBitmapTile>(tile);
    dataAvailable = static_cast<bool>(tile);

    // Convert data if such is present
    if (dataAvailable)
    {
        _sourceData->bitmap = resourcesManager->adjustBitmapToConfiguration(
            _sourceData->bitmap,
            _sourceData->alphaChannelData);
    }

    return true;
}

bool OsmAnd::MapRendererRasterBitmapTileResource::uploadToGPU()
{
    bool ok = resourcesManager->uploadTileToGPU(_sourceData, _resourceInGPU);
    if (!ok)
        return false;

    // Since content was uploaded to GPU, it's safe to release it
    _sourceData->releaseConsumableContent();

    return true;
}

void OsmAnd::MapRendererRasterBitmapTileResource::unloadFromGPU()
{
    _resourceInGPU.reset();
}

void OsmAnd::MapRendererRasterBitmapTileResource::releaseData()
{
    _sourceData.reset();
}
