#include "MapRendererConfiguration.h"

OsmAnd::MapRendererConfiguration::MapRendererConfiguration()
    : texturesFilteringQuality(TextureFilteringQuality::Good)
    , textureAtlasesAllowed(false)
    , limitTextureColorDepthBy16bits(false)
    , heightmapPatchesPerSide(24)
{
}

OsmAnd::MapRendererConfiguration::~MapRendererConfiguration()
{
}
