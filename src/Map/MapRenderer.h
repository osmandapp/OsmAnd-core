#ifndef _OSMAND_CORE_MAP_RENDERER_H_
#define _OSMAND_CORE_MAP_RENDERER_H_

#include "stdlib_common.h"
#include <map>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QAtomicInt>
#include <QHash>
#include <QMap>
#include <QReadWriteLock>
#include <QSet>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapCommonTypes.h"
#include "MapRendererTypes_private.h"
#include "Thread.h"
#include "Dispatcher.h"
#include "IMapRenderer.h"
#include "GPUAPI.h"
#include "IMapTiledDataProvider.h"
#include "TiledEntriesCollection.h"
#include "MapRendererInternalState.h"
#include "MapRendererResourcesManager.h"
#include "MapSymbolsGroup.h"

namespace OsmAnd
{
    class MapRendererTiledResources;
    class MapSymbol;
    class MapRendererStage;
    struct MapRendererInternalState;

    class MapRenderer : public IMapRenderer
    {
        Q_DISABLE_COPY_AND_MOVE(MapRenderer);

    public:
        typedef QSet< std::shared_ptr<MapRendererBaseResource> > MapSymbolReferenceOrigins;
        typedef QHash< std::shared_ptr<const MapSymbol>, MapSymbolReferenceOrigins > PublishedMapSymbols;
        typedef std::map< std::shared_ptr<const MapSymbolsGroup>, PublishedMapSymbols, MapSymbolsGroup::Comparator > PublishedMapSymbolsByGroup;
        typedef QMap< int, PublishedMapSymbolsByGroup > PublishedMapSymbolsByOrder;

        enum {
            MaxMissingDataZoomShift = 5,
            MaxMissingDataUnderZoomShift = 2,
            HeixelsPerTileSide = (1 << MapRenderer::MaxMissingDataZoomShift) + 1,
            ElevationDataTileSize = HeixelsPerTileSide + 2,
        };

    private:
        // General:
        bool _isRenderingInitialized;
        MapRendererSetupOptions _setupOptions;

        // Configuration-related:
        mutable QReadWriteLock _configurationLock;
        std::shared_ptr<MapRendererConfiguration> _currentConfiguration;
        std::shared_ptr<const MapRendererConfiguration> _currentConfigurationAsConst;
        std::shared_ptr<MapRendererConfiguration> _requestedConfiguration;
        QAtomicInt _currentConfigurationInvalidatedMask;
        bool updateCurrentConfiguration(const unsigned int currentConfigurationInvalidatedMask);

        // State-related:
        mutable QAtomicInt _frameInvalidatesCounter;
        int _frameInvalidatesToBeProcessed;
        mutable QMutex _requestedStateMutex;
        MapRendererState _requestedState;
        MapRendererState _currentState;
        QAtomicInt _requestedStateUpdatedMask;
        void notifyRequestedStateWasUpdated(const MapRendererStateChange change);

        // Resources-related:
        std::unique_ptr<MapRendererResourcesManager> _resources;
        QAtomicInt _resourcesGpuSyncRequestsCounter;

        // Symbols-related:
        mutable QReadWriteLock _publishedMapSymbolsByOrderLock;
        PublishedMapSymbolsByOrder _publishedMapSymbolsByOrder;
        QHash< std::shared_ptr<const MapSymbolsGroup>, SmartPOD<unsigned int, 0> > _publishedMapSymbolsGroups;
        QAtomicInt _publishedMapSymbolsCount;
        void doPublishMapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& symbolGroup,
            const std::shared_ptr<const MapSymbol>& symbol,
            const std::shared_ptr<MapRendererBaseResource>& resource);
        void doUnpublishMapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& symbolGroup,
            const std::shared_ptr<const MapSymbol>& symbol,
            const std::shared_ptr<MapRendererBaseResource>& resource);
        bool validatePublishedMapSymbolsIntegrity();
        QAtomicInt _suspendSymbolsUpdateCounter;
        int _symbolsUpdateInterval;

        // GPU worker related:
        Qt::HANDLE _gpuWorkerThreadId;
        std::unique_ptr<Concurrent::Thread> _gpuWorkerThread;
        volatile bool _gpuWorkerThreadIsAlive;
        QMutex _gpuWorkerThreadWakeupMutex;
        QWaitCondition _gpuWorkerThreadWakeup;
        volatile bool _gpuWorkerIsSuspended;
        void gpuWorkerThreadProcedure();
        void processGpuWorker();

        // General:
        void invalidateFrame();
        Qt::HANDLE _renderThreadId;
        Concurrent::Dispatcher _renderThreadDispatcher;
        Concurrent::Dispatcher _gpuThreadDispatcher;

        // Debug-related:
        mutable QReadWriteLock _debugSettingsLock;
        QAtomicInt _currentDebugSettingsInvalidatedCounter;
        std::shared_ptr<MapRendererDebugSettings> _currentDebugSettings;
        std::shared_ptr<const MapRendererDebugSettings> _currentDebugSettingsAsConst;
        std::shared_ptr<MapRendererDebugSettings> _requestedDebugSettings;
        bool updateCurrentDebugSettings();

        virtual AreaI getVisibleBBox31(const MapRendererInternalState& internalState) const = 0;
        virtual double getPixelsToMetersScaleFactor(const MapRendererState& state, const MapRendererInternalState& internalState) const = 0;
    protected:
        MapRenderer(
            GPUAPI* const gpuAPI,
            const std::unique_ptr<const MapRendererConfiguration>& baseConfiguration,
            const std::unique_ptr<const MapRendererDebugSettings>& baseDebugSettings);

        // General:
        const std::unique_ptr<GPUAPI> gpuAPI;
        const MapRendererSetupOptions& setupOptions;
        bool hasGpuWorkerThread() const;
        bool isInGpuWorkerThread() const;
        bool isInRenderThread() const;

        // Configuration-related:
        const std::shared_ptr<const MapRendererConfiguration>& currentConfiguration;
        enum class ConfigurationChange
        {
            ColorDepthForcing,
            TexturesFilteringMode,

            __LAST
        };
        enum {
            RegisteredConfigurationChangesCount = static_cast<unsigned int>(ConfigurationChange::__LAST)
        };
        virtual uint32_t getConfigurationChangeMask(
            const std::shared_ptr<const MapRendererConfiguration>& current,
            const std::shared_ptr<const MapRendererConfiguration>& updated) const;
        void invalidateCurrentConfiguration(const uint32_t changesMask);
        virtual void validateConfigurationChange(const ConfigurationChange& change);

        // State-related:
        const MapRendererState& currentState;
        mutable QReadWriteLock _internalStateLock;
        virtual const MapRendererInternalState* getInternalStateRef() const = 0;
        virtual MapRendererInternalState* getInternalStateRef() = 0;
        virtual const MapRendererInternalState& getInternalState() const = 0;
        virtual MapRendererInternalState& getInternalState() = 0;
        virtual bool updateInternalState(
            MapRendererInternalState& outInternalState,
            const MapRendererState& state,
            const MapRendererConfiguration& configuration) const;

        // Resources-related:
        const MapRendererResourcesManager& getResources() const;
        MapRendererResourcesManager& getResources();
        virtual void onValidateResourcesOfType(const MapRendererResourceType type);
        void requestResourcesUploadOrUnload();
        bool adjustImageToConfiguration(
            const sk_sp<const SkImage>& input,
            sk_sp<SkImage>& output,
            const AlphaChannelPresence alphaChannelPresence = AlphaChannelPresence::Unknown) const;
        sk_sp<const SkImage> adjustImageToConfiguration(
            const sk_sp<const SkImage>& input,
            const AlphaChannelPresence alphaChannelPresence = AlphaChannelPresence::Unknown) const;

        // Symbols-related:
        QReadWriteLock& publishedMapSymbolsByOrderLock;
        const PublishedMapSymbolsByOrder& publishedMapSymbolsByOrder;
        void publishMapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& symbolGroup,
            const std::shared_ptr<const MapSymbol>& symbol,
            const std::shared_ptr<MapRendererBaseResource>& resource);
        void unpublishMapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& symbolGroup,
            const std::shared_ptr<const MapSymbol>& symbol,
            const std::shared_ptr<MapRendererBaseResource>& resource);
        void batchPublishMapSymbols(
            const QList< PublishOrUnpublishMapSymbol >& mapSymbolsToPublish);
        void batchUnpublishMapSymbols(
            const QList< PublishOrUnpublishMapSymbol >& mapSymbolsToUnublish);

        // General:

        // Debug-related:
        const std::shared_ptr<const MapRendererDebugSettings>& currentDebugSettings;

        // Customization points:
        virtual bool preInitializeRendering();
        virtual bool doInitializeRendering();
        virtual bool postInitializeRendering();

        virtual bool preUpdate(IMapRenderer_Metrics::Metric_update* metric);
        virtual bool doUpdate(IMapRenderer_Metrics::Metric_update* metric);
        virtual bool postUpdate(IMapRenderer_Metrics::Metric_update* metric);

        virtual bool prePrepareFrame();
        virtual bool doPrepareFrame();
        virtual bool postPrepareFrame();

        virtual bool preRenderFrame(IMapRenderer_Metrics::Metric_renderFrame* metric);
        virtual bool doRenderFrame(IMapRenderer_Metrics::Metric_renderFrame* metric) = 0;
        virtual bool postRenderFrame(IMapRenderer_Metrics::Metric_renderFrame* metric);

        virtual bool preReleaseRendering(bool gpuContextLost);
        virtual bool doReleaseRendering(bool gpuContextLost);
        virtual bool postReleaseRendering(bool gpuContextLost);

        virtual bool handleStateChange(const MapRendererState& state, MapRendererStateChanges mask);
    public:
        virtual ~MapRenderer();

        virtual MapRendererSetupOptions getSetupOptions() const Q_DECL_OVERRIDE;
        virtual bool setup(const MapRendererSetupOptions& setupOptions) Q_DECL_OVERRIDE;

        // General:
        virtual bool isRenderingInitialized() const Q_DECL_OVERRIDE;

        // Configuration-related:
        virtual std::shared_ptr<MapRendererConfiguration> getConfiguration() const Q_DECL_OVERRIDE;
        virtual void setConfiguration(
            const std::shared_ptr<const MapRendererConfiguration>& configuration,
            bool forcedUpdate = false) Q_DECL_OVERRIDE;

        bool initializeRendering(bool renderTargetAvailable) override;
        bool update(IMapRenderer_Metrics::Metric_update* metric = nullptr) final;
        bool prepareFrame(IMapRenderer_Metrics::Metric_prepareFrame* metric = nullptr) final;
        bool renderFrame(IMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) final;
        bool releaseRendering(bool gpuContextLost) override;

        bool attachToRenderTarget() override;
        bool isAttachedToRenderTarget() override;
        bool detachFromRenderTarget() override;

        virtual bool isIdle() const Q_DECL_OVERRIDE;
        virtual QString getNotIdleReason() const Q_DECL_OVERRIDE;

        virtual bool isGpuWorkerPaused() const Q_DECL_OVERRIDE;
        virtual bool suspendGpuWorker() Q_DECL_OVERRIDE;
        virtual bool resumeGpuWorker() Q_DECL_OVERRIDE;

        virtual void reloadEverything() Q_DECL_OVERRIDE;

        virtual MapRendererState getState() const Q_DECL_OVERRIDE;
        virtual MapState getMapState() const Q_DECL_OVERRIDE;
        virtual bool isFrameInvalidated() const Q_DECL_OVERRIDE;
        virtual void forcedFrameInvalidate() Q_DECL_OVERRIDE;
        virtual void forcedGpuProcessingCycle() Q_DECL_OVERRIDE;

        Concurrent::Dispatcher& getRenderThreadDispatcher();
        Concurrent::Dispatcher& getGpuThreadDispatcher();

        virtual bool setMapLayerProvider(const int layerIndex, const std::shared_ptr<IMapLayerProvider>& provider, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool resetMapLayerProvider(const int layerIndex, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setMapLayerConfiguration(const int layerIndex, const MapLayerConfiguration& configuration, bool forcedUpdate = false) Q_DECL_OVERRIDE;

        virtual bool setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& provider, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool resetElevationDataProvider(bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setElevationConfiguration(const ElevationConfiguration& configuration, bool forcedUpdate = false) Q_DECL_OVERRIDE;

        virtual bool addSymbolsProvider(const std::shared_ptr<IMapTiledSymbolsProvider>& provider, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool addSymbolsProvider(const std::shared_ptr<IMapKeyedSymbolsProvider>& provider, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool hasSymbolsProvider(const std::shared_ptr<IMapTiledSymbolsProvider>& provider) Q_DECL_OVERRIDE;
        virtual bool hasSymbolsProvider(const std::shared_ptr<IMapKeyedSymbolsProvider>& provider) Q_DECL_OVERRIDE;
        virtual bool removeSymbolsProvider(const std::shared_ptr<IMapTiledSymbolsProvider>& provider, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool removeSymbolsProvider(const std::shared_ptr<IMapKeyedSymbolsProvider>& provider, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool removeAllSymbolsProviders(bool forcedUpdate = false) Q_DECL_OVERRIDE;

        virtual bool setWindowSize(const PointI& windowSize, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setViewport(const AreaI& viewport, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setFieldOfView(const float fieldOfView, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setFogConfiguration(const FogConfiguration& configuration, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setSkyColor(const FColorRGB& color, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setAzimuth(const float azimuth, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setElevationAngle(const float elevationAngle, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setTarget(const PointI& target31, bool forcedUpdate = false, bool disableUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setZoom(const float zoom, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setZoom(const ZoomLevel zoomLevel, const float visualZoom, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setZoomLevel(const ZoomLevel zoomLevel, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setVisualZoom(const float visualZoom, bool forcedUpdate = false) Q_DECL_OVERRIDE;
        virtual bool setVisualZoomShift(const float visualZoomShift, bool forcedUpdate = false) Q_DECL_OVERRIDE;

        virtual bool setStubsStyle(const MapStubStyle style, bool forcedUpdate = false) Q_DECL_OVERRIDE;

        virtual bool setBackgroundColor(const FColorRGB& color, bool forcedUpdate = false) Q_DECL_OVERRIDE;

        virtual ZoomLevel getMinZoomLevel() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoomLevel() const Q_DECL_OVERRIDE;

        virtual ZoomLevel getMinimalZoomLevelsRangeLowerBound() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMinimalZoomLevelsRangeUpperBound() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaximalZoomLevelsRangeLowerBound() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaximalZoomLevelsRangeUpperBound() const Q_DECL_OVERRIDE;

        virtual int getMaxMissingDataZoomShift() const Q_DECL_OVERRIDE;
        virtual int getMaxMissingDataUnderZoomShift() const Q_DECL_OVERRIDE;
        virtual int getHeixelsPerTileSide() const Q_DECL_OVERRIDE;
        virtual int getElevationDataTileSize() const Q_DECL_OVERRIDE;

        // Symbols-related:
        virtual unsigned int getSymbolsCount() const Q_DECL_OVERRIDE;
        virtual bool isSymbolsUpdateSuspended(int* const pOutSuspendsCounter = nullptr) const Q_DECL_OVERRIDE;
        virtual bool suspendSymbolsUpdate() Q_DECL_OVERRIDE;
        virtual bool resumeSymbolsUpdate() Q_DECL_OVERRIDE;
        virtual int getSymbolsUpdateInterval() Q_DECL_OVERRIDE;
        virtual void setSymbolsUpdateInterval(int interval) Q_DECL_OVERRIDE;

        // Debug-related:
        virtual std::shared_ptr<MapRendererDebugSettings> getDebugSettings() const Q_DECL_OVERRIDE;
        virtual void setDebugSettings(const std::shared_ptr<const MapRendererDebugSettings>& debugSettings) Q_DECL_OVERRIDE;
        virtual void setResourceWorkerThreadsLimit(const unsigned int limit) Q_DECL_OVERRIDE;
        virtual void resetResourceWorkerThreadsLimit() Q_DECL_OVERRIDE;
        virtual unsigned int getActiveResourceRequestsCount() const Q_DECL_OVERRIDE;
        virtual void dumpResourcesInfo() const Q_DECL_OVERRIDE;

    friend struct OsmAnd::MapRendererInternalState;
    friend class OsmAnd::MapRendererStage;
    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_MAP_RENDERER_H_)
