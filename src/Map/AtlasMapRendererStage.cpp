#include "AtlasMapRendererStage.h"

#include "AtlasMapRenderer.h"

OsmAnd::AtlasMapRendererStage::AtlasMapRendererStage(AtlasMapRenderer* const renderer_)
    : MapRendererStage(renderer_)
    , AtlasMapRendererStageHelper(this)
{
}

OsmAnd::AtlasMapRendererStage::~AtlasMapRendererStage() = default;

std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU> OsmAnd::AtlasMapRendererStage::captureElevationDataResource(
    TileId normalizedTileId,
    ZoomLevel zoomLevel,
    std::shared_ptr<const IMapElevationDataProvider::Data>* pOutSource /* = nullptr */,
    bool* isNotReady /* = nullptr */) const
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
        else if (isNotReady)
            *isNotReady = true;
    }

    return nullptr;
}

OsmAnd::ZoomLevel OsmAnd::AtlasMapRendererStage::getElevationData(TileId normalizedTileId, ZoomLevel zoomLevel,
    PointF& offsetInTileN, std::shared_ptr<const IMapElevationDataProvider::Data>* pOutSource /* = nullptr */,
    bool* isNotReady /* = nullptr */) const
{
    if (!currentState.elevationDataProvider)
        return ZoomLevel::InvalidZoomLevel;

    if (captureElevationDataResource(normalizedTileId, zoomLevel, pOutSource, isNotReady))
        return zoomLevel;

    const auto maxMissingDataZoomShift = currentState.elevationDataProvider->getMaxMissingDataZoomShift();
    const auto maxUnderZoomShift = currentState.elevationDataProvider->getMaxMissingDataUnderZoomShift();
    const auto minZoom = currentState.elevationDataProvider->getMinZoom();
    const auto maxZoom = currentState.elevationDataProvider->getMaxZoom();
    for (int absZoomShift = 1; absZoomShift <= maxMissingDataZoomShift; absZoomShift++)
    {
        // Look for underscaled first. Only full match is accepted.
        // Don't replace tiles of absent zoom levels by the unserscaled ones
        const auto underscaledZoom = static_cast<int>(zoomLevel) + absZoomShift;
        if (underscaledZoom >= minZoom && underscaledZoom <= maxZoom && absZoomShift <= maxUnderZoomShift &&
            zoomLevel >= minZoom)
        {
            auto underscaledTileIdN =
                TileId::fromXY(normalizedTileId.x << absZoomShift, normalizedTileId.y << absZoomShift);
            PointF offsetN;
            if (absZoomShift < 20)
                offsetN = offsetInTileN * static_cast<float>(1u << absZoomShift);
            else
            {
                const double tileSize = 1ull << absZoomShift;
                offsetN.x = static_cast<double>(offsetInTileN.x) * tileSize;
                offsetN.y = static_cast<double>(offsetInTileN.y) * tileSize;
            }
            const auto innerOffset =
                PointI(static_cast<int32_t>(std::floor(offsetN.x)), static_cast<int32_t>(std::floor(offsetN.y)));
            underscaledTileIdN.x += innerOffset.x;
            underscaledTileIdN.y += innerOffset.y;
            const auto underscaledZoomLevel = static_cast<ZoomLevel>(underscaledZoom);
            if (captureElevationDataResource(underscaledTileIdN, underscaledZoomLevel, pOutSource, isNotReady))
            {
                offsetInTileN.x = offsetN.x - static_cast<float>(innerOffset.x);
                offsetInTileN.y = offsetN.y - static_cast<float>(innerOffset.y);
                return underscaledZoomLevel;
            }
        }

        // If underscaled was not found, look for overscaled (surely, if such zoom level exists at all)
        const auto overscaledZoom = static_cast<int>(zoomLevel) - absZoomShift;
        if (overscaledZoom >= minZoom && overscaledZoom <= maxZoom)
        {
            PointF texCoordsOffset;
            PointF texCoordsScale;
            const auto overscaledTileIdN = Utilities::getTileIdOverscaledByZoomShift(
                normalizedTileId,
                absZoomShift,
                &texCoordsOffset,
                &texCoordsScale);
            const auto overscaledZoomLevel = static_cast<ZoomLevel>(overscaledZoom);
            if (captureElevationDataResource(overscaledTileIdN, overscaledZoomLevel, pOutSource, isNotReady))
            {
                offsetInTileN.x = texCoordsOffset.x + offsetInTileN.x * texCoordsScale.x;
                offsetInTileN.y = texCoordsOffset.y + offsetInTileN.y * texCoordsScale.y;
                return overscaledZoomLevel;
            }
        }
    }

    return ZoomLevel::InvalidZoomLevel;
}

const OsmAnd::AtlasMapRendererConfiguration& OsmAnd::AtlasMapRendererStage::getCurrentConfiguration() const
{
    return static_cast<const OsmAnd::AtlasMapRendererConfiguration&>(*currentConfiguration); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
}
