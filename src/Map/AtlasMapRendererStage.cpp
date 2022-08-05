#include "AtlasMapRendererStage.h"

#include "AtlasMapRenderer.h"

OsmAnd::AtlasMapRendererStage::AtlasMapRendererStage(AtlasMapRenderer* const renderer_)
    : MapRendererStage(renderer_)
    , AtlasMapRendererStageHelper(this)
{
}

OsmAnd::AtlasMapRendererStage::~AtlasMapRendererStage() = default;

std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU> OsmAnd::AtlasMapRendererStage::captureElevationDataResource(
    TileId normalizedTileId, ZoomLevel zoomLevel, std::shared_ptr<const IMapElevationDataProvider::Data>* pOutSource /*= nullptr*/) const
{
    if (!currentState.elevationDataProvider)
        return nullptr;

    const auto& resourcesCollection_ = getResources().getCollectionSnapshot(MapRendererResourceType::ElevationData, currentState.elevationDataProvider);
    const auto& resourcesCollection = std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection_);

    // Obtain tile entry by normalized tile coordinates, since tile may repeat several times
    std::shared_ptr<MapRendererBaseTiledResource> resource_;
    if (resourcesCollection->obtainResource(normalizedTileId, zoomLevel, resource_))
    {
        const auto resource = std::static_pointer_cast<MapRendererElevationDataResource>(resource_);

        // Check state and obtain GPU resource
        if (resource->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
        {
            // Capture GPU resource
            auto gpuResource = resource->resourceInGPU;
            if (pOutSource) {
                *pOutSource = resource->sourceData;
            }

            resource->setState(MapRendererResourceState::Uploaded);

            return gpuResource;
        }
    }

    return nullptr;
}

const OsmAnd::AtlasMapRendererConfiguration& OsmAnd::AtlasMapRendererStage::getCurrentConfiguration() const
{
    return static_cast<const OsmAnd::AtlasMapRendererConfiguration&>(*currentConfiguration); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
}
