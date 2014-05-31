#include "MapRendererElevationDataTileResource.h"

#include "IMapElevationDataProvider.h"
#include "MapRendererResourcesManager.h"

OsmAnd::MapRendererElevationDataTileResource::MapRendererElevationDataTileResource(
    MapRendererResourcesManager* owner,
    const MapRendererResourceType type,
    const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
    const TileId tileId,
    const ZoomLevel zoom)
    : MapRendererBaseTiledResource(owner, type, collection, tileId, zoom)
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
        ok = owner->obtainProviderFor(&link_->collection, provider_);
    if (!ok)
        return false;
    const auto provider = std::static_pointer_cast<IMapTiledDataProvider>(provider_);

    // Obtain tile from provider
    std::shared_ptr<const MapTiledData> tile;
    const auto requestSucceeded = provider->obtainData(tileId, zoom, tile, queryController);
    if (!requestSucceeded)
        return false;

    // Store data
    _sourceData = tile;
    dataAvailable = static_cast<bool>(tile);

    return true;
}

bool OsmAnd::MapRendererElevationDataTileResource::uploadToGPU()
{
    bool ok = owner->uploadTileToGPU(_sourceData, _resourceInGPU);
    if (!ok)
        return false;

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

void OsmAnd::MapRendererElevationDataTileResource::unloadFromGPU()
{
    assert(_resourceInGPU.use_count() == 1);
    _resourceInGPU.reset();
}
