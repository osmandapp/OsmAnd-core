#include "MapRendererConfiguration.h"

OsmAnd::MapRendererConfiguration::MapRendererConfiguration()
    : texturesFilteringQuality(TextureFilteringQuality::Good)
    , altasTexturesAllowed(false)
    , limitTextureColorDepthBy16bits(false)
    , heightmapPatchesPerSide(24)
    , paletteTexturesAllowed(true)
{
}

OsmAnd::MapRendererConfiguration::~MapRendererConfiguration()
{
}
