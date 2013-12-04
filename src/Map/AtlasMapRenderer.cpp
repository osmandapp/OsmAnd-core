#include "AtlasMapRenderer.h"

OsmAnd::AtlasMapRenderer::AtlasMapRenderer()
{
}

OsmAnd::AtlasMapRenderer::~AtlasMapRenderer()
{
}

uint32_t OsmAnd::AtlasMapRenderer::getTilesPerAtlasTextureLimit(const MapRendererResources::ResourceType resourceType, const std::shared_ptr<const MapTile>& tile)
{
    return currentConfiguration.altasTexturesAllowed ? OptimalTilesPerAtlasTextureSide : 1;
}
