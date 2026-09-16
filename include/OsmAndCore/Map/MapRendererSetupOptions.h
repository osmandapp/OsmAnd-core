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
        OSMAND_CALLABLE(GpuWorkerThreadPrologue, void, const IMapRenderer* mapRenderer);
        OSMAND_CALLABLE(GpuWorkerThreadEpilogue, void, const IMapRenderer* mapRenderer);
        OSMAND_CALLABLE(FrameUpdateRequestCallback, void, const IMapRenderer* mapRenderer);

        MapRendererSetupOptions();
        ~MapRendererSetupOptions();

        // Background GPU worker is used for uploading/unloading resources from GPU in background
        bool gpuWorkerThreadEnabled;
#if !defined(SWIG)
        inline MapRendererSetupOptions& setGpuWorkerThreadEnabled(const bool newGpuWorkerThreadEnabled)
        {
            gpuWorkerThreadEnabled = newGpuWorkerThreadEnabled;

            return *this;
        }
#endif // !defined(SWIG)

        GpuWorkerThreadPrologue gpuWorkerThreadPrologue;
#if !defined(SWIG)
        inline MapRendererSetupOptions& setGpuWorkerThreadPrologue(const GpuWorkerThreadPrologue newGpuWorkerThreadPrologue)
        {
            gpuWorkerThreadPrologue = newGpuWorkerThreadPrologue;

            return *this;
        }
#endif // !defined(SWIG)

        GpuWorkerThreadEpilogue gpuWorkerThreadEpilogue;
#if !defined(SWIG)
        inline MapRendererSetupOptions& setGpuWorkerThreadEpilogue(const GpuWorkerThreadEpilogue newGpuWorkerThreadEpilogue)
        {
            gpuWorkerThreadEpilogue = newGpuWorkerThreadEpilogue;

            return *this;
        }
#endif // !defined(SWIG)

        // This callback is called when frame needs update
        FrameUpdateRequestCallback frameUpdateRequestCallback;
#if !defined(SWIG)
        inline MapRendererSetupOptions& setFrameUpdateRequestCallback(const FrameUpdateRequestCallback newFrameUpdateRequestCallback)
        {
            frameUpdateRequestCallback = newFrameUpdateRequestCallback;

            return *this;
        }
#endif // !defined(SWIG)

        // Limit maximal number of raster map layers drawn in batch. 0 means "maximal as limited by platform"
        unsigned int maxNumberOfRasterMapLayersInBatch;
#if !defined(SWIG)
        inline MapRendererSetupOptions& setMaxNumberOfRasterMapLayersInBatch(
            const unsigned int newMaxNumberOfRasterMapLayersInBatch)
        {
            maxNumberOfRasterMapLayersInBatch = newMaxNumberOfRasterMapLayersInBatch;

            return *this;
        }
#endif // !defined(SWIG)

        // Display density factor
        float displayDensityFactor;
#if !defined(SWIG)
        inline MapRendererSetupOptions& setDisplayDensityFactor(
            const float newDisplayDensityFactor)
        {
            displayDensityFactor = newDisplayDensityFactor;

            return *this;
        }
#endif // !defined(SWIG)

        bool elevationVisualizationEnabled;
#if !defined(SWIG)
        inline MapRendererSetupOptions& setElevationVisualizationEnabled(
            const float newElevationVisualizationEnabled)
        {
            elevationVisualizationEnabled = newElevationVisualizationEnabled;

            return *this;
        }
#endif // !defined(SWIG)

        QString pathToOpenGLShadersCache;
#if !defined(SWIG)
        inline MapRendererSetupOptions& setPathToOpenGLShadersCache(
            const QString& newPathToOpenGLShadersCache)
        {
            pathToOpenGLShadersCache = newPathToOpenGLShadersCache;

            return *this;
        }
#endif // !defined(SWIG)

        // Teardown runs on the platform's main thread, and the waits below have no deadline of their
        // own, so a stuck worker can outlive the time the OS gives the process to exit. A positive
        // value (milliseconds) makes those waits give up and abandon the workers instead. Zero, the
        // default, keeps waiting indefinitely.
        int maxTeardownWaitTime;
#if !defined(SWIG)
        inline MapRendererSetupOptions& setMaxTeardownWaitTime(const int newMaxTeardownWaitTime)
        {
            maxTeardownWaitTime = newMaxTeardownWaitTime;

            return *this;
        }
#endif // !defined(SWIG)

        bool isValid() const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_SETUP_OPTIONS_H_)
