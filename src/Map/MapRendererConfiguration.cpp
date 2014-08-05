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

void OsmAnd::MapRendererConfiguration::copyTo(MapRendererConfiguration& other) const
{
    other.texturesFilteringQuality = texturesFilteringQuality;
    other.limitTextureColorDepthBy16bits = limitTextureColorDepthBy16bits;
    other.heixelsPerTileSide = heixelsPerTileSide;
    other.paletteTexturesAllowed = paletteTexturesAllowed;
}
