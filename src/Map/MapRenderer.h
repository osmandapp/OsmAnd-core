#ifndef _OSMAND_CORE_MAP_RENDERER_H_
#define _OSMAND_CORE_MAP_RENDERER_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QAtomicInt>
#include <QHash>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapTypes.h"
#include "Concurrent.h"
#include "IMapRenderer.h"
#include "GPUAPI.h"
#include "IMapTiledDataProvider.h"
#include "TiledEntriesCollection.h"
#include "MapRendererInternalState.h"
#include "MapRendererResourcesManager.h"

namespace OsmAnd
{
    class MapRendererTiledResources;
    class MapSymbol;
    class MapRendererStage;
    struct MapRendererInternalState;
    
    class MapRenderer : public IMapRenderer
    {
        Q_DISABLE_COPY(MapRenderer);

    private:
        // General:
        bool _isRenderingInitialized;
        MapRendererSetupOptions _setupOptions;

        // Configuration-related:
        mutable QReadWriteLock _configurationLock;
        MapRendererConfiguration _currentConfiguration;
        MapRendererConfiguration _requestedConfiguration;
        volatile uint32_t _currentConfigurationInvalidatedMask;
        void invalidateCurrentConfiguration(const uint32_t changesMask);
        bool updateCurrentConfiguration();

        // State-related:
        mutable QAtomicInt _frameInvalidatesCounter;
        int _frameInvalidatesToBeProcessed;
        mutable QMutex _requestedStateMutex;
        MapRendererState _requestedState;
        MapRendererState _currentState;
        volatile uint32_t _requestedStateUpdatedMask;
        bool revalidateState();
        void notifyRequestedStateWasUpdated(const MapRendererStateChange change);

        // Resources-related:
        std::unique_ptr<MapRendererResourcesManager> _resources;
        QAtomicInt _resourcesGpuSyncRequestsCounter;
        
        // GPU worker related:
        Qt::HANDLE _gpuWorkerThreadId;
        std::unique_ptr<Concurrent::Thread> _gpuWorkerThread;
        volatile bool _gpuWorkerIsAlive;
        QMutex _gpuWorkerThreadWakeupMutex;
        QWaitCondition _gpuWorkerThreadWakeup;
        volatile bool _gpuWorkerIsPaused;
        QMutex _gpuWorkerThreadPausedMutex;
        QWaitCondition _gpuWorkerThreadPaused;
        void gpuWorkerThreadProcedure();

        // General:
        void invalidateFrame();
        Qt::HANDLE _renderThreadId;
        Concurrent::Dispatcher _renderThreadDispatcher;
        Concurrent::Dispatcher _gpuThreadDispatcher;
    protected:
        MapRenderer(GPUAPI* const gpuAPI);

        // General:
        virtual bool isRenderingInitialized() const;
        const std::unique_ptr<GPUAPI> gpuAPI;
        const MapRendererSetupOptions& setupOptions;

        // Configuration-related:
        const MapRendererConfiguration& currentConfiguration;
        enum ConfigurationChange
        {
            ColorDepthForcing = 1 << 0,
            ElevationDataResolution = 1 << 1,
            TexturesFilteringMode = 1 << 2,
            PaletteTexturesUsage = 1 << 3,
        };
        virtual void validateConfigurationChange(const ConfigurationChange& change);

        // State-related:
        const MapRendererState& currentState;
        mutable QReadWriteLock _internalStateLock;
        virtual const MapRendererInternalState* getInternalStateRef() const = 0;
        virtual MapRendererInternalState* getInternalStateRef() = 0;
        virtual bool updateInternalState(MapRendererInternalState* internalState, const MapRendererState& state);

        // Resources-related:
        const MapRendererResourcesManager& getResources() const;
        MapRendererResourcesManager& getResources();
        virtual void onValidateResourcesOfType(const MapRendererResourceType type);
        void requestResourcesUploadOrUnload();
        bool adjustBitmapToConfiguration(
            const std::shared_ptr<const SkBitmap>& input,
            std::shared_ptr<const SkBitmap>& output,
            const AlphaChannelData alphaChannelData = AlphaChannelData::Undefined) const;
        std::shared_ptr<const SkBitmap> adjustBitmapToConfiguration(
            const std::shared_ptr<const SkBitmap>& input,
            const AlphaChannelData alphaChannelData = AlphaChannelData::Undefined) const;

        // General:

        // Customization points:
        virtual bool preInitializeRendering();
        virtual bool doInitializeRendering();
        virtual bool postInitializeRendering();

        virtual bool preUpdate();
        virtual bool doUpdate();
        virtual bool postUpdate();

        virtual bool prePrepareFrame();
        virtual bool doPrepareFrame();
        virtual bool postPrepareFrame();

        virtual bool preRenderFrame();
        virtual bool doRenderFrame() = 0;
        virtual bool postRenderFrame();

        virtual bool preReleaseRendering();
        virtual bool doReleaseRendering();
        virtual bool postReleaseRendering();
    public:
        virtual ~MapRenderer();

        virtual MapRendererSetupOptions getSetupOptions() const;
        virtual bool setup(const MapRendererSetupOptions& setupOptions);

        // General:

        // Configuration-related:
        virtual MapRendererConfiguration getConfiguration() const;
        virtual void setConfiguration(const MapRendererConfiguration& configuration, bool forcedUpdate = false);

        virtual bool initializeRendering();
        virtual bool update();
        virtual bool prepareFrame();
        virtual bool renderFrame();
        virtual bool releaseRendering();

        virtual bool pauseGpuWorkerThread();
        virtual bool resumeGpuWorkerThread();

        virtual void reloadEverything();

        virtual MapRendererState getState() const;
        virtual bool isFrameInvalidated() const;
        virtual void forcedFrameInvalidate();
        virtual void forcedGpuProcessingCycle();

        Concurrent::Dispatcher& getRenderThreadDispatcher();
        Concurrent::Dispatcher& getGpuThreadDispatcher();

        virtual unsigned int getSymbolsCount() const;

        virtual void setRasterLayerProvider(const RasterMapLayerId layerId, const std::shared_ptr<IMapRasterBitmapTileProvider>& tileProvider, bool forcedUpdate = false);
        virtual void resetRasterLayerProvider(const RasterMapLayerId layerId, bool forcedUpdate = false);
        virtual void setRasterLayerOpacity(const RasterMapLayerId layerId, const float opacity, bool forcedUpdate = false);
        virtual void setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& tileProvider, bool forcedUpdate = false);
        virtual void resetElevationDataProvider(bool forcedUpdate = false);
        virtual void setElevationDataScaleFactor(const float factor, bool forcedUpdate = false);
        virtual void addSymbolProvider(const std::shared_ptr<IMapDataProvider>& provider, bool forcedUpdate = false);
        virtual void removeSymbolProvider(const std::shared_ptr<IMapDataProvider>& provider, bool forcedUpdate = false);
        virtual void removeAllSymbolProviders(bool forcedUpdate = false);
        virtual void setWindowSize(const PointI& windowSize, bool forcedUpdate = false);
        virtual void setViewport(const AreaI& viewport, bool forcedUpdate = false);
        virtual void setFieldOfView(const float fieldOfView, bool forcedUpdate = false);
        virtual void setDistanceToFog(const float fogDistance, bool forcedUpdate = false);
        virtual void setFogOriginFactor(const float factor, bool forcedUpdate = false);
        virtual void setFogHeightOriginFactor(const float factor, bool forcedUpdate = false);
        virtual void setFogDensity(const float fogDensity, bool forcedUpdate = false);
        virtual void setFogColor(const FColorRGB& color, bool forcedUpdate = false);
        virtual void setSkyColor(const FColorRGB& color, bool forcedUpdate = false);
        virtual void setAzimuth(const float azimuth, bool forcedUpdate = false);
        virtual void setElevationAngle(const float elevationAngle, bool forcedUpdate = false);
        virtual void setTarget(const PointI& target31, bool forcedUpdate = false);
        virtual void setZoom(const float zoom, bool forcedUpdate = false);

        virtual float getMinZoom() const;
        virtual float getMaxZoom() const;

        virtual float getRecommendedMinZoom(const ZoomRecommendationStrategy strategy) const;
        virtual float getRecommendedMaxZoom(const ZoomRecommendationStrategy strategy) const;

        virtual void dumpResourcesInfo() const;

    friend struct OsmAnd::MapRendererInternalState;
    friend class OsmAnd::MapRendererStage;
    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_MAP_RENDERER_H_)
