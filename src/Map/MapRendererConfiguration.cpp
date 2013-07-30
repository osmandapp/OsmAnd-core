#include "MapRendererConfiguration.h"

OsmAnd::MapRendererConfiguration::MapRendererConfiguration()
{
    textureAtlasesAllowed = false;
    force16bitTextureBitmapColorDepth = false;
    heightmapPatchesPerSide = 24;
}

OsmAnd::MapRendererConfiguration::~MapRendererConfiguration()
{
}
