#include "MapRendererSetupOptions.h"

OsmAnd::MapRendererSetupOptions::MapRendererSetupOptions()
    : gpuWorkerThreadEnabled(false)
    , gpuWorkerThreadPrologue(nullptr)
    , gpuWorkerThreadEpilogue(nullptr)
    , frameUpdateRequestCallback(nullptr)
    , maxNumberOfRasterMapLayersInBatch(0)
    , displayDensityFactor(1.0f)
    , elevationVisualizationEnabled(true)
{
}

OsmAnd::MapRendererSetupOptions::~MapRendererSetupOptions()
{
}

bool OsmAnd::MapRendererSetupOptions::isValid() const
{
    return
        (displayDensityFactor > 0.0f);
}
