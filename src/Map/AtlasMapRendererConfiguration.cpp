#include "AtlasMapRendererConfiguration.h"

#include "IAtlasMapRenderer.h"

OsmAnd::AtlasMapRendererConfiguration::AtlasMapRendererConfiguration()
    : referenceTileSizeOnScreenInPixels(IAtlasMapRenderer::DefaultReferenceTileSizeOnScreenInPixels)
{
}

OsmAnd::AtlasMapRendererConfiguration::~AtlasMapRendererConfiguration()
{
}

void OsmAnd::AtlasMapRendererConfiguration::copyTo(MapRendererConfiguration& other_) const
{
    if (const auto other = dynamic_cast<AtlasMapRendererConfiguration*>(&other_))
    {
        other->referenceTileSizeOnScreenInPixels = referenceTileSizeOnScreenInPixels;
    }

    MapRendererConfiguration::copyTo(other_);
}
