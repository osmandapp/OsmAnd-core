#include "MapRendererElevationDataTileResource.h"

#include "IMapElevationDataProvider.h"
#include "MapRendererResourcesManager.h"

OsmAnd::MapRendererElevationDataTileResource::MapRendererElevationDataTileResource(
    MapRendererResourcesManager* owner_,
    const TiledEntriesCollection<MapRendererBaseTiledResource>& collection_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : MapRendererBaseTiledResource(owner_, MapRendererResourceType::ElevationDataTile, collection_, tileId_, zoom_)
    , resourceInGPU(_resourceInGPU)
{
}

OsmAnd::MapRendererElevationDataTileResource::~MapRendererElevationDataTileResource()
{
    safeUnlink();
}

bool OsmAnd::MapRendererElevationDataTileResource::obtainData(bool& dataAvailable, const IQueryController* queryController)
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
    _sourceData = std::static_pointer_cast<ElevationDataTile>(tile);
    dataAvailable = static_cast<bool>(tile);

    return true;
}

bool OsmAnd::MapRendererElevationDataTileResource::uploadToGPU()
{
    bool ok = resourcesManager->uploadTileToGPU(_sourceData, _resourceInGPU);
    if (!ok)
        return false;

    // Since content was uploaded to GPU, it's safe to release it
    _sourceData->releaseConsumableContent();

    return true;
}

void OsmAnd::MapRendererElevationDataTileResource::unloadFromGPU()
{
    _resourceInGPU.reset();
}

void OsmAnd::MapRendererElevationDataTileResource::releaseData()
{
    _sourceData.reset();
}
