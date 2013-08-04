#include "MapRendererConfiguration.h"

OsmAnd::MapRendererConfiguration::MapRendererConfiguration()
    : texturesFilteringQuality(TextureFilteringQuality::Good)
    , altasTexturesAllowed(false)
    , limitTextureColorDepthBy16bits(false)
    , heixelsPerTileSide(24)
    , paletteTexturesAllowed(false)
{
}

OsmAnd::MapRendererConfiguration::~MapRendererConfiguration()
{
}
