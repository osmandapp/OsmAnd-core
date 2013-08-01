#include "MapRendererConfiguration.h"

OsmAnd::MapRendererConfiguration::MapRendererConfiguration()
    : texturesFilteringQuality(TextureFilteringQuality::Best)
    , textureAtlasesAllowed(false)
    , limitTextureColorDepthBy16bits(false)
    , heightmapPatchesPerSide(24)
{
}

OsmAnd::MapRendererConfiguration::~MapRendererConfiguration()
{
}
