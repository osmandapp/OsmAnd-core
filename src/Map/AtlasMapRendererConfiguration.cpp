#include "AtlasMapRendererConfiguration.h"

OsmAnd::AtlasMapRendererConfiguration::AtlasMapRendererConfiguration()
    : referenceTileSizeOnScreenInPixels(DefaultReferenceTileSizeOnScreenInPixels)
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

std::shared_ptr<OsmAnd::MapRendererConfiguration> OsmAnd::AtlasMapRendererConfiguration::createCopy() const
{
    std::shared_ptr<MapRendererConfiguration> copy(new AtlasMapRendererConfiguration());
    copyTo(*copy);
    return copy;
}
