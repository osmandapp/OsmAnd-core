#include "MapRendererConfiguration.h"

OsmAnd::MapRendererConfiguration::MapRendererConfiguration()
    : texturesFilteringQuality(TextureFilteringQuality::Good)
    , limitTextureColorDepthBy16bits(false)
    , heixelsPerTileSide(32)
    , paletteTexturesAllowed(false)
{
}

OsmAnd::MapRendererConfiguration::~MapRendererConfiguration()
{
}
