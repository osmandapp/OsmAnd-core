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

namespace OsmAnd
{
    class IMapLayerProvider;
    class IMapElevationDataProvider;
    class IMapTiledSymbolsProvider;
    class IMapKeyedSymbolsProvider;
    class MapSymbol;
    namespace IMapRenderer_Metrics
    {
        struct Metric_update;
        struct Metric_prepareFrame;
        struct Metric_renderFrame;
    }

    class OSMAND_CORE_API IMapRenderer
    {
    private:
    protected:
        IMapRenderer();
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
        virtual bool releaseRendering() = 0;

        virtual bool isIdle() const = 0;
        virtual QString getNotIdleReason() const = 0;

        virtual bool pauseGpuWorkerThread() = 0;
        virtual bool resumeGpuWorkerThread() = 0;

        OSMAND_OBSERVER_CALLABLE(FramePreparedObserver,
            IMapRenderer* mapRenderer);
        const ObservableAs<IMapRenderer::FramePreparedObserver> framePreparedObservable;

        virtual void reloadEverything() = 0;

        virtual MapRendererState getState() const = 0;
        virtual bool isFrameInvalidated() const = 0;
        virtual void forcedFrameInvalidate() = 0;
        virtual void forcedGpuProcessingCycle() = 0;

        virtual unsigned int getSymbolsCount() const = 0;
        virtual QList< std::shared_ptr<const MapSymbol> > getSymbolsAt(const PointI& screenPoint) const = 0;
        virtual bool isSymbolsUpdateSuspended(int* const pOutSuspendsCounter = nullptr) const = 0;
        virtual bool suspendSymbolsUpdate() = 0;
        virtual bool resumeSymbolsUpdate() = 0;

        virtual bool setMapLayerProvider(const unsigned int layerIndex, const std::shared_ptr<IMapLayerProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool resetMapLayerProvider(const unsigned int layerIndex, bool forcedUpdate = false) = 0;
        virtual bool setMapLayerConfiguration(const unsigned int layerIndex, const MapLayerConfiguration& configuration, bool forcedUpdate = false) = 0;

        virtual bool setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool resetElevationDataProvider(bool forcedUpdate = false) = 0;
        virtual bool setElevationDataConfiguration(const ElevationDataConfiguration& configuration, bool forcedUpdate = false) = 0;

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

        virtual std::shared_ptr<MapRendererDebugSettings> getDebugSettings() const = 0;
        virtual void setDebugSettings(const std::shared_ptr<const MapRendererDebugSettings>& debugSettings) = 0;

        virtual float getMinZoom() const = 0;
        virtual float getMaxZoom() const = 0;

        enum class ZoomRecommendationStrategy
        {
            NarrowestRange,
            WidestRange
        };
        virtual float getRecommendedMinZoom(const ZoomRecommendationStrategy strategy = ZoomRecommendationStrategy::NarrowestRange) const = 0;
        virtual float getRecommendedMaxZoom(const ZoomRecommendationStrategy strategy = ZoomRecommendationStrategy::NarrowestRange) const = 0;

        OSMAND_OBSERVER_CALLABLE(StateChangeObserver,
            const IMapRenderer* const mapRenderer,
            const MapRendererStateChange thisChange,
            SWIG_OMIT(const) MapRendererStateChanges allChanges);
        const ObservableAs<IMapRenderer::StateChangeObserver> stateChangeObservable;

        //NOTE: screen points origin from top-left
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI& location31) const = 0;
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI64& location) const = 0;

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
