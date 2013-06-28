#include "BaseAtlasMapRenderer.h"

OsmAnd::BaseAtlasMapRenderer::BaseAtlasMapRenderer()
{
}

OsmAnd::BaseAtlasMapRenderer::~BaseAtlasMapRenderer()
{
}

uint32_t OsmAnd::BaseAtlasMapRenderer::getMaxHeightmapResolutionPerTile()
{
    //TODO: Obviously, not the best solution
    return TileElevationNodesPerSide * 2 + 1;
}
