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
    protected:
        MapRenderer(
            GPUAPI* const gpuAPI,
            const std::unique_ptr<const MapRendererConfiguration>& baseConfiguration,
            const std::unique_ptr<const MapRendererDebugSettings>& baseDebugSettings);

        // General:
        virtual bool isRenderingInitialized() const;
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
            ElevationVisualization,

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

        virtual bool preUpdate(IMapRenderer_Metrics::Metric_update* const metric);
        virtual bool doUpdate(IMapRenderer_Metrics::Metric_update* const metric);
        virtual bool postUpdate(IMapRenderer_Metrics::Metric_update* const metric);

        virtual bool prePrepareFrame();
        virtual bool doPrepareFrame();
        virtual bool postPrepareFrame();

        virtual bool preRenderFrame(IMapRenderer_Metrics::Metric_renderFrame* const metric);
        virtual bool doRenderFrame(IMapRenderer_Metrics::Metric_renderFrame* const metric) = 0;
        virtual bool postRenderFrame(IMapRenderer_Metrics::Metric_renderFrame* const metric);

        virtual bool preReleaseRendering(const bool gpuContextLost);
        virtual bool doReleaseRendering(const bool gpuContextLost);
        virtual bool postReleaseRendering(const bool gpuContextLost);
    public:
        virtual ~MapRenderer();

        virtual MapRendererSetupOptions getSetupOptions() const;
        virtual bool setup(const MapRendererSetupOptions& setupOptions);

        // General:

        // Configuration-related:
        virtual std::shared_ptr<MapRendererConfiguration> getConfiguration() const;
        virtual void setConfiguration(const std::shared_ptr<const MapRendererConfiguration>& configuration, bool forcedUpdate = false);

        virtual bool initializeRendering();
        virtual bool update(IMapRenderer_Metrics::Metric_update* const metric = nullptr) Q_DECL_FINAL;
        virtual bool prepareFrame(IMapRenderer_Metrics::Metric_prepareFrame* const metric = nullptr) Q_DECL_FINAL;
        virtual bool renderFrame(IMapRenderer_Metrics::Metric_renderFrame* const metric = nullptr) Q_DECL_FINAL;
        virtual bool releaseRendering(const bool gpuContextLost = false);

        virtual bool isIdle() const;
        virtual QString getNotIdleReason() const;

        virtual bool isGpuWorkerPaused() const;
        virtual bool suspendGpuWorker();
        virtual bool resumeGpuWorker();

        virtual void reloadEverything();

        virtual MapRendererState getState() const;
        virtual MapState getMapState() const;
        virtual bool isFrameInvalidated() const;
        virtual void forcedFrameInvalidate();
        virtual void forcedGpuProcessingCycle();

        Concurrent::Dispatcher& getRenderThreadDispatcher();
        Concurrent::Dispatcher& getGpuThreadDispatcher();

        virtual bool setMapLayerProvider(const int layerIndex, const std::shared_ptr<IMapLayerProvider>& provider, bool forcedUpdate = false);
        virtual bool resetMapLayerProvider(const int layerIndex, bool forcedUpdate = false);
        virtual bool setMapLayerConfiguration(const int layerIndex, const MapLayerConfiguration& configuration, bool forcedUpdate = false);

        virtual bool setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& provider, bool forcedUpdate = false);
        virtual bool resetElevationDataProvider(bool forcedUpdate = false);
        virtual bool setElevationConfiguration(const ElevationConfiguration& configuration, bool forcedUpdate = false);

        virtual bool addSymbolsProvider(const std::shared_ptr<IMapTiledSymbolsProvider>& provider, bool forcedUpdate = false);
        virtual bool addSymbolsProvider(const std::shared_ptr<IMapKeyedSymbolsProvider>& provider, bool forcedUpdate = false);
        virtual bool removeSymbolsProvider(const std::shared_ptr<IMapTiledSymbolsProvider>& provider, bool forcedUpdate = false);
        virtual bool removeSymbolsProvider(const std::shared_ptr<IMapKeyedSymbolsProvider>& provider, bool forcedUpdate = false);
        virtual bool removeAllSymbolsProviders(bool forcedUpdate = false);

        virtual bool setWindowSize(const PointI& windowSize, bool forcedUpdate = false);
        virtual bool setViewport(const AreaI& viewport, bool forcedUpdate = false);
        virtual bool setFieldOfView(const float fieldOfView, bool forcedUpdate = false);
        virtual bool setFogConfiguration(const FogConfiguration& configuration, bool forcedUpdate = false);
        virtual bool setSkyColor(const FColorRGB& color, bool forcedUpdate = false);
        virtual bool setAzimuth(const float azimuth, bool forcedUpdate = false);
        virtual bool setElevationAngle(const float elevationAngle, bool forcedUpdate = false);
        virtual bool setTarget(const PointI& target31, bool forcedUpdate = false);
        virtual bool setZoom(const float zoom, bool forcedUpdate = false);
        virtual bool setZoom(const ZoomLevel zoomLevel, const float visualZoom, bool forcedUpdate = false);
        virtual bool setZoomLevel(const ZoomLevel zoomLevel, bool forcedUpdate = false);
        virtual bool setVisualZoom(const float visualZoom, bool forcedUpdate = false);
        virtual bool setVisualZoomShift(const float visualZoomShift, bool forcedUpdate = false);

        virtual bool setStubsStyle(const MapStubStyle style, bool forcedUpdate = false);

        virtual ZoomLevel getMinZoomLevel() const;
        virtual ZoomLevel getMaxZoomLevel() const;

        virtual ZoomLevel getMinimalZoomLevelsRangeLowerBound() const;
        virtual ZoomLevel getMinimalZoomLevelsRangeUpperBound() const;
        virtual ZoomLevel getMaximalZoomLevelsRangeLowerBound() const;
        virtual ZoomLevel getMaximalZoomLevelsRangeUpperBound() const;

        virtual int getMaxMissingDataZoomShift() const Q_DECL_OVERRIDE;
        virtual int getMaxMissingDataUnderZoomShift() const Q_DECL_OVERRIDE;
        virtual int getHeixelsPerTileSide() const Q_DECL_OVERRIDE;
        virtual int getElevationDataTileSize() const Q_DECL_OVERRIDE;

        // Symbols-related:
        virtual unsigned int getSymbolsCount() const;
        virtual bool isSymbolsUpdateSuspended(int* const pOutSuspendsCounter = nullptr) const;
        virtual bool suspendSymbolsUpdate();
        virtual bool resumeSymbolsUpdate();

        // Debug-related:
        virtual std::shared_ptr<MapRendererDebugSettings> getDebugSettings() const;
        virtual void setDebugSettings(const std::shared_ptr<const MapRendererDebugSettings>& debugSettings);
        virtual void setResourceWorkerThreadsLimit(const unsigned int limit);
        virtual void resetResourceWorkerThreadsLimit();
        virtual unsigned int getActiveResourceRequestsCount() const;
        virtual void dumpResourcesInfo() const;

    friend struct OsmAnd::MapRendererInternalState;
    friend class OsmAnd::MapRendererStage;
    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_MAP_RENDERER_H_)
