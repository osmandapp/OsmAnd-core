#include "MapRendererStage.h"

#include "MapRenderer.h"

OsmAnd::MapRendererStage::MapRendererStage(MapRenderer* const renderer_)
    : renderer(renderer_)
    , gpuAPI(renderer->gpuAPI)
    , currentState(renderer->currentState)
    , currentConfiguration(renderer->currentConfiguration)
    , setupOptions(renderer->setupOptions)
{
}

OsmAnd::MapRendererStage::~MapRendererStage()
{
}

const OsmAnd::MapRendererResources& OsmAnd::MapRendererStage::getResources()
{
    return renderer->getResources();
}
