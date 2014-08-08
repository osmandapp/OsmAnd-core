#include "AtlasMapRendererStage.h"

#include "AtlasMapRenderer.h"

OsmAnd::AtlasMapRendererStage::AtlasMapRendererStage(AtlasMapRenderer* const renderer_)
    : MapRendererStage(renderer_)
    , AtlasMapRendererStageHelper(this)
{
}

OsmAnd::AtlasMapRendererStage::~AtlasMapRendererStage()
{
}

const OsmAnd::AtlasMapRendererConfiguration& OsmAnd::AtlasMapRendererStage::getCurrentConfiguration() const
{
    return static_cast<const OsmAnd::AtlasMapRendererConfiguration&>(*currentConfiguration);
}
