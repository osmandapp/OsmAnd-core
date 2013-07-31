#include "MapRendererConfiguration.h"

OsmAnd::MapRendererConfiguration::MapRendererConfiguration()
    : texturesFiltering(TextureFiltering::Trilinear)
    , textureAtlasesAllowed(false)
    , limitTextureColorDepthBy16bits(false)
    , heightmapPatchesPerSide(24)
{
}

OsmAnd::MapRendererConfiguration::~MapRendererConfiguration()
{
}
