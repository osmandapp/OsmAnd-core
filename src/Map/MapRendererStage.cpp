#include "MapRendererStage.h"

#include "MapRenderer.h"

OsmAnd::MapRendererStage::MapRendererStage(MapRenderer* const renderer_)
    : renderer(renderer_)
    , gpuAPI(renderer->gpuAPI)
    , setupOptions(renderer->setupOptions)
    , currentConfiguration(renderer->currentConfiguration)
    , currentState(renderer->currentState)
    , internalState(renderer->getInternalState())
    , debugSettings(renderer->currentDebugSettings)
    , publishedMapSymbolsByOrderLock(renderer->publishedMapSymbolsByOrderLock)
    , publishedMapSymbolsByOrder(renderer->publishedMapSymbolsByOrder)
{
}

OsmAnd::MapRendererStage::~MapRendererStage()
{
}

const OsmAnd::MapRendererResourcesManager& OsmAnd::MapRendererStage::getResources() const
{
    return renderer->getResources();
}
