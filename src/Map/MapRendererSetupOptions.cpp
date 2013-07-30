#include "MapRendererSetupOptions.h"

OsmAnd::MapRendererSetupOptions::MapRendererSetupOptions()
{
    backgroundWorker.enabled = false;
    backgroundWorker.prologue = nullptr;
    backgroundWorker.epilogue = nullptr;

    frameRequestCallback = nullptr;

    displayDensityFactor = 1.0f;
}

OsmAnd::MapRendererSetupOptions::~MapRendererSetupOptions()
{
}
