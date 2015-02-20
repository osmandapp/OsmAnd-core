#include "MapRendererSetupOptions.h"

OsmAnd::MapRendererSetupOptions::MapRendererSetupOptions()
    : gpuWorkerThreadEnabled(false)
    , gpuWorkerThreadPrologue(nullptr)
    , gpuWorkerThreadEpilogue(nullptr)
    , frameUpdateRequestCallback(nullptr)
    , maxNumberOfRasterMapLayersInBatch(0)
    , displayDensityFactor(1.0f)
{
}

OsmAnd::MapRendererSetupOptions::~MapRendererSetupOptions()
{
}
