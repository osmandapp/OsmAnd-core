#include "MapRendererStage.h"

#include "MapRenderer.h"

OsmAnd::MapRendererStage::MapRendererStage(MapRenderer* const renderer_)
    : renderer(renderer_)
    , gpuAPI(renderer->gpuAPI)
    , currentState(renderer->currentState)
    , currentConfiguration(renderer->currentConfiguration)
    , setupOptions(renderer->setupOptions)
    , debugSettings(renderer->currentDebugSettings)
{
}

OsmAnd::MapRendererStage::~MapRendererStage()
{
}

const OsmAnd::MapRendererResourcesManager& OsmAnd::MapRendererStage::getResources()
{
    return renderer->getResources();
}
