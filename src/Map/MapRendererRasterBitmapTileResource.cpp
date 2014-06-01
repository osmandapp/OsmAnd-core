#include "MapRendererRasterBitmapTileResource.h"

#include "IMapRasterBitmapTileProvider.h"
#include "MapRendererResourcesManager.h"

OsmAnd::MapRendererRasterBitmapTileResource::MapRendererRasterBitmapTileResource(
    MapRendererResourcesManager* owner_,
    const TiledEntriesCollection<MapRendererBaseTiledResource>& collection_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : MapRendererBaseTiledResource(owner_, MapRendererResourceType::RasterBitmapTile, collection_, tileId_, zoom_)
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
        ok = owner->obtainProviderFor(static_cast<MapRendererBaseResourcesCollection*>(static_cast<MapRendererTiledResourcesCollection*>(&link_->collection)), provider_);
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

    return true;
}

bool OsmAnd::MapRendererRasterBitmapTileResource::uploadToGPU()
{
    bool ok = owner->uploadTileToGPU(_sourceData, _resourceInGPU);
    if (!ok)
        return false;

    _sourceData = std::static_pointer_cast<RasterBitmapTile>(_sourceData->createNoContentInstance());

    return true;
}

void OsmAnd::MapRendererRasterBitmapTileResource::unloadFromGPU()
{
    assert(_resourceInGPU.use_count() == 1);
    _resourceInGPU.reset();
}
