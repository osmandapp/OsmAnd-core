#include "MapRendererConfiguration.h"

OsmAnd::MapRendererConfiguration::MapRendererConfiguration()
    : texturesFilteringQuality(TextureFilteringQuality::Good)
    , limitTextureColorDepthBy16bits(false)
{
}

OsmAnd::MapRendererConfiguration::~MapRendererConfiguration()
{
}

void OsmAnd::MapRendererConfiguration::copyTo(MapRendererConfiguration& other) const
{
    other.texturesFilteringQuality = texturesFilteringQuality;
    other.limitTextureColorDepthBy16bits = limitTextureColorDepthBy16bits;
}

std::shared_ptr<OsmAnd::MapRendererConfiguration> OsmAnd::MapRendererConfiguration::createCopy() const
{
    std::shared_ptr<MapRendererConfiguration> copy(new MapRendererConfiguration());
    copyTo(*copy);
    return copy;
}
