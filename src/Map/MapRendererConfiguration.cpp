#include "MapRendererConfiguration.h"

OsmAnd::MapRendererConfiguration::MapRendererConfiguration()
{
    displayDensityFactor = 1.0f;

    textureAtlasesAllowed = true;
    heightmapPatchesPerSide = 24;

    backgroundWorker.enabled = false;
    backgroundWorker.prologue = nullptr;
    backgroundWorker.epilogue = nullptr;

    frameRequestCallback = nullptr;
}

OsmAnd::MapRendererConfiguration::~MapRendererConfiguration()
{
}
