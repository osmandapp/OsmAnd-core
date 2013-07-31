#include "AtlasMapRenderer.h"

OsmAnd::AtlasMapRenderer::AtlasMapRenderer()
{
}

OsmAnd::AtlasMapRenderer::~AtlasMapRenderer()
{
}

uint32_t OsmAnd::AtlasMapRenderer::getTilesPerAtlasTextureLimit( const MapTileLayerId& layerId, const std::shared_ptr<IMapTileProvider::Tile>& tile )
{
    return currentConfiguration.textureAtlasesAllowed ? OptimalTilesPerAtlasTextureSide : 1;
}
