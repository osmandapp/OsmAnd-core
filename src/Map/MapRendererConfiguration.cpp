#include "MapRendererConfiguration.h"

OsmAnd::MapRendererConfiguration::MapRendererConfiguration()
{
    textureAtlasesAllowed = false;
    limitTextureColorDepthBy16bits = false;
    heightmapPatchesPerSide = 24;
}

OsmAnd::MapRendererConfiguration::~MapRendererConfiguration()
{
}
