#include "AtlasMapRenderer.h"

OsmAnd::AtlasMapRenderer::AtlasMapRenderer()
{
}

OsmAnd::AtlasMapRenderer::~AtlasMapRenderer()
{
}

uint32_t OsmAnd::AtlasMapRenderer::getTilesPerAtlasTextureLimit( const TiledResourceType& resourceType, const std::shared_ptr<MapTile>& tile )
{
    return currentConfiguration.altasTexturesAllowed ? OptimalTilesPerAtlasTextureSide : 1;
}
