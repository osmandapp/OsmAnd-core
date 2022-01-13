#ifndef _OSMAND_CORE_I_MAP_RENDERER_H_
#define _OSMAND_CORE_I_MAP_RENDERER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Observable.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/MapRendererTypes.h>
#include <OsmAndCore/Map/MapRendererState.h>
#include <OsmAndCore/Map/MapRendererConfiguration.h>
#include <OsmAndCore/Map/MapRendererSetupOptions.h>
#include <OsmAndCore/Map/MapRendererDebugSettings.h>
#include <OsmAndCore/Map/MapSymbol.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>

namespace OsmAnd
{
    class IMapLayerProvider;
    class IMapElevationDataProvider;
    class IMapTiledSymbolsProvider;
    class IMapKeyedSymbolsProvider;
    class MapSymbol;
    
    struct MapRendererInternalState;
    
    namespace IMapRenderer_Metrics
    {
        struct Metric_update;
        struct Metric_prepareFrame;
        struct Metric_renderFrame;
    }

    class OSMAND_CORE_API IMapRenderer
    {
    public:
        struct MapSymbolInformation Q_DECL_FINAL
        {
            std::shared_ptr<const MapSymbol> mapSymbol;
            std::shared_ptr<const MapSymbolsGroup::AdditionalSymbolInstanceParameters> instanceParameters;
        };

    private:
    protected:
        IMapRenderer();
        
        virtual AreaI getVisibleBBox31(MapRendererInternalState* _internalState) const = 0;
        virtual double getCurrentPixelsToMetersScaleFactor(const ZoomLevel zoomLevel, MapRendererInternalState* _internalState) const = 0;
    public:
        virtual ~IMapRenderer();

        virtual MapRendererSetupOptions getSetupOptions() const = 0;
        virtual bool setup(const MapRendererSetupOptions& setupOptions) = 0;

        virtual std::shared_ptr<MapRendererConfiguration> getConfiguration() const = 0;
        virtual void setConfiguration(const std::shared_ptr<const MapRendererConfiguration>& configuration, bool forcedUpdate = false) = 0;

        virtual bool isRenderingInitialized() const = 0;
        virtual bool initializeRendering() = 0;
        virtual bool update(IMapRenderer_Metrics::Metric_update* const metric = nullptr) = 0;
        virtual bool prepareFrame(IMapRenderer_Metrics::Metric_prepareFrame* const metric = nullptr) = 0;
        virtual bool renderFrame(IMapRenderer_Metrics::Metric_renderFrame* const metric = nullptr) = 0;
        virtual bool releaseRendering(const bool gpuContextLost = false) = 0;

        virtual bool isIdle() const = 0;
        virtual QString getNotIdleReason() const = 0;

        virtual bool isGpuWorkerPaused() const = 0;
        virtual bool suspendGpuWorker() = 0;
        virtual bool resumeGpuWorker() = 0;

        OSMAND_OBSERVER_CALLABLE(FramePreparedObserver,
            IMapRenderer* mapRenderer);
        const ObservableAs<IMapRenderer::FramePreparedObserver> framePreparedObservable;

        virtual void reloadEverything() = 0;

        virtual MapRendererState getState() const = 0;
        virtual MapState getMapState() const = 0;
        virtual bool isFrameInvalidated() const = 0;
        virtual void forcedFrameInvalidate() = 0;
        virtual void forcedGpuProcessingCycle() = 0;

        virtual unsigned int getSymbolsCount() const = 0;
        virtual QList<MapSymbolInformation> getSymbolsAt(const PointI& screenPoint) const = 0;
        virtual QList<MapSymbolInformation> getSymbolsIn(const AreaI& screenPoint, const bool strict = false) const = 0;
        virtual bool isSymbolsUpdateSuspended(int* const pOutSuspendsCounter = nullptr) const = 0;
        virtual bool suspendSymbolsUpdate() = 0;
        virtual bool resumeSymbolsUpdate() = 0;

        virtual bool setMapLayerProvider(const int layerIndex, const std::shared_ptr<IMapLayerProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool resetMapLayerProvider(const int layerIndex, bool forcedUpdate = false) = 0;
        virtual bool setMapLayerConfiguration(const int layerIndex, const MapLayerConfiguration& configuration, bool forcedUpdate = false) = 0;

        virtual bool setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool resetElevationDataProvider(bool forcedUpdate = false) = 0;
        virtual bool setElevationConfiguration(const ElevationConfiguration& configuration, bool forcedUpdate = false) = 0;

        virtual bool addSymbolsProvider(const std::shared_ptr<IMapTiledSymbolsProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool addSymbolsProvider(const std::shared_ptr<IMapKeyedSymbolsProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool removeSymbolsProvider(const std::shared_ptr<IMapTiledSymbolsProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool removeSymbolsProvider(const std::shared_ptr<IMapKeyedSymbolsProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool removeAllSymbolsProviders(bool forcedUpdate = false) = 0;

        virtual bool setWindowSize(const PointI& windowSize, bool forcedUpdate = false) = 0;
        virtual bool setViewport(const AreaI& viewport, bool forcedUpdate = false) = 0;
        virtual bool setFieldOfView(const float fieldOfView, bool forcedUpdate = false) = 0;
        virtual bool setFogConfiguration(const FogConfiguration& configuration, bool forcedUpdate = false) = 0;
        virtual bool setSkyColor(const FColorRGB& color, bool forcedUpdate = false) = 0;
        virtual bool setAzimuth(const float azimuth, bool forcedUpdate = false) = 0;
        virtual bool setElevationAngle(const float elevationAngle, bool forcedUpdate = false) = 0;
        virtual bool setTarget(const PointI& target31, bool forcedUpdate = false) = 0;
        virtual bool setZoom(const float zoom, bool forcedUpdate = false) = 0;
        virtual bool setZoom(const ZoomLevel zoomLevel, const float visualZoom, bool forcedUpdate = false) = 0;
        virtual bool setZoomLevel(const ZoomLevel zoomLevel, bool forcedUpdate = false) = 0;
        virtual bool setVisualZoom(const float visualZoom, bool forcedUpdate = false) = 0;
        virtual bool setVisualZoomShift(const float visualZoomShift, bool forcedUpdate = false) = 0;

        virtual bool setStubsStyle(const MapStubStyle style, bool forcedUpdate = false) = 0;

        virtual std::shared_ptr<MapRendererDebugSettings> getDebugSettings() const = 0;
        virtual void setDebugSettings(const std::shared_ptr<const MapRendererDebugSettings>& debugSettings) = 0;

        virtual ZoomLevel getMinZoomLevel() const = 0;
        virtual ZoomLevel getMaxZoomLevel() const = 0;

        virtual ZoomLevel getMinimalZoomLevelsRangeLowerBound() const = 0;
        virtual ZoomLevel getMinimalZoomLevelsRangeUpperBound() const = 0;
        virtual ZoomLevel getMaximalZoomLevelsRangeLowerBound() const = 0;
        virtual ZoomLevel getMaximalZoomLevelsRangeUpperBound() const = 0;

        OSMAND_OBSERVER_CALLABLE(StateChangeObserver,
            const IMapRenderer* const mapRenderer,
            const MapRendererStateChange thisChange,
            SWIG_OMIT(const) MapRendererStateChanges allChanges);
        const ObservableAs<IMapRenderer::StateChangeObserver> stateChangeObservable;

        //NOTE: screen points origin from top-left
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI& location31) const = 0;
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI64& location) const = 0;
        
        virtual AreaI getVisibleBBox31() const = 0;
        virtual bool isPositionVisible(const PointI64& position) const = 0;
        virtual bool isPositionVisible(const PointI& position31) const = 0;
        virtual bool obtainScreenPointFromPosition(const PointI64& position, PointI& outScreenPoint) const = 0;
        virtual bool obtainScreenPointFromPosition(const PointI& position31, PointI& outScreenPoint, bool checkOffScreen = false) const = 0;

        virtual double getCurrentTileSizeInMeters() const = 0;
        virtual double getCurrentPixelsToMetersScaleFactor() const = 0;

        virtual int getMaxMissingDataZoomShift() const = 0;
        virtual int getMaxMissingDataUnderZoomShift() const = 0;
        virtual int getHeixelsPerTileSide() const = 0;
        virtual int getElevationDataTileSize() const = 0;

        virtual void setResourceWorkerThreadsLimit(const unsigned int limit) = 0;
        virtual void resetResourceWorkerThreadsLimit() = 0;
        virtual unsigned int getActiveResourceRequestsCount() const = 0;
        virtual void dumpResourcesInfo() const = 0;
    };

    enum class MapRendererClass
    {
        AtlasMapRenderer_OpenGL2plus,
        AtlasMapRenderer_OpenGLES2,
    };
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL createMapRenderer(const MapRendererClass mapRendererClass);
}

#endif // !defined(_OSMAND_CORE_I_MAP_RENDERER_H_)
