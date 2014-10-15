#include "MapRendererSetupOptions.h"

OsmAnd::MapRendererSetupOptions::MapRendererSetupOptions()
    : gpuWorkerThreadEnabled(false)
    , gpuWorkerThreadPrologue(nullptr)
    , gpuWorkerThreadEpilogue(nullptr)
    , frameUpdateRequestCallback(nullptr)
    , maxNumberOfRasterMapLayersInBatch(0)
{
}

OsmAnd::MapRendererSetupOptions::~MapRendererSetupOptions()
{
}
