#ifndef _OSMAND_CORE_MAP_RENDERER_SETUP_OPTIONS_H_
#define _OSMAND_CORE_MAP_RENDERER_SETUP_OPTIONS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    struct OSMAND_CORE_API MapRendererSetupOptions
    {
        MapRendererSetupOptions();
        ~MapRendererSetupOptions();

        // Background GPU worker is used for uploading/unloading resources from GPU in background
        typedef std::function<void ()> GpuWorkerThreadPrologue;
        typedef std::function<void ()> GpuWorkerThreadEpilogue;
        bool gpuWorkerThreadEnabled;
        GpuWorkerThreadPrologue gpuWorkerThreadPrologue;
        GpuWorkerThreadEpilogue gpuWorkerThreadEpilogue;

        // This callback is called when frame needs update
        typedef std::function<void()> FrameUpdateRequestCallback;
        FrameUpdateRequestCallback frameUpdateRequestCallback;

        float displayDensityFactor;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_SETUP_OPTIONS_H_)
