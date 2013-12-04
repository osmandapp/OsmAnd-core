#include "MapRendererSetupOptions.h"

OsmAnd::MapRendererSetupOptions::MapRendererSetupOptions()
{
    gpuWorkerThread.enabled = false;
    gpuWorkerThread.prologue = nullptr;
    gpuWorkerThread.epilogue = nullptr;

    frameUpdateRequestCallback = nullptr;

    displayDensityFactor = 1.0f;
}

OsmAnd::MapRendererSetupOptions::~MapRendererSetupOptions()
{
}
