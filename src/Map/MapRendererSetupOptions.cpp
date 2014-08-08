#include "MapRendererSetupOptions.h"

OsmAnd::MapRendererSetupOptions::MapRendererSetupOptions()
    : gpuWorkerThreadEnabled(false)
    , gpuWorkerThreadPrologue(nullptr)
    , gpuWorkerThreadEpilogue(nullptr)
    , frameUpdateRequestCallback(nullptr)
{
}

OsmAnd::MapRendererSetupOptions::~MapRendererSetupOptions()
{
}
