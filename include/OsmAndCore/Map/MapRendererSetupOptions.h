#ifndef _OSMAND_CORE_MAP_RENDERER_SETUP_OPTIONS_H_
#define _OSMAND_CORE_MAP_RENDERER_SETUP_OPTIONS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Callable.h>

namespace OsmAnd
{
    class IMapRenderer;

    struct OSMAND_CORE_API MapRendererSetupOptions Q_DECL_FINAL
    {
        MapRendererSetupOptions();
        ~MapRendererSetupOptions();

        // Background GPU worker is used for uploading/unloading resources from GPU in background
        OSMAND_CALLABLE(GpuWorkerThreadPrologue, void, const IMapRenderer* const mapRenderer);
        OSMAND_CALLABLE(GpuWorkerThreadEpilogue, void, const IMapRenderer* const mapRenderer);
        bool gpuWorkerThreadEnabled;
        GpuWorkerThreadPrologue gpuWorkerThreadPrologue;
        GpuWorkerThreadEpilogue gpuWorkerThreadEpilogue;

        // This callback is called when frame needs update
        OSMAND_CALLABLE(FrameUpdateRequestCallback, void, const IMapRenderer* const mapRenderer);
        FrameUpdateRequestCallback frameUpdateRequestCallback;

        float displayDensityFactor;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_SETUP_OPTIONS_H_)
