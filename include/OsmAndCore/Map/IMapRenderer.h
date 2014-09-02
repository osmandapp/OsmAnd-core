#ifndef _OSMAND_CORE_I_MAP_RENDERER_H_
#define _OSMAND_CORE_I_MAP_RENDERER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QSet>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Observable.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/MapRendererTypes.h>
#include <OsmAndCore/Map/MapRendererState.h>
#include <OsmAndCore/Map/MapRendererConfiguration.h>
#include <OsmAndCore/Map/MapRendererSetupOptions.h>
#include <OsmAndCore/Map/MapRendererDebugSettings.h>

namespace OsmAnd
{
    class IMapDataProvider;
    class MapSymbol;

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
        virtual bool update() = 0;
        virtual bool prepareFrame() = 0;
        virtual bool renderFrame() = 0;
        virtual bool releaseRendering() = 0;

        virtual bool isIdle() const = 0;

        virtual bool pauseGpuWorkerThread() = 0;
        virtual bool resumeGpuWorkerThread() = 0;

        OSMAND_CALLABLE(FramePreparedObserver, void, IMapRenderer* mapRenderer);
        const ObservableAs<IMapRenderer::FramePreparedObserver> framePreparedObservable;

        virtual void reloadEverything() = 0;

        virtual MapRendererState getState() const = 0;
        virtual bool isFrameInvalidated() const = 0;
        virtual void forcedFrameInvalidate() = 0;
        virtual void forcedGpuProcessingCycle() = 0;

        virtual unsigned int getSymbolsCount() const = 0;
        virtual QList< std::shared_ptr<const MapSymbol> > getSymbolsAt(const PointI& screenPoint) const = 0;

        virtual void setRasterLayerProvider(const RasterMapLayerId layerId, const std::shared_ptr<IMapRasterBitmapTileProvider>& tileProvider, bool forcedUpdate = false) = 0;
        virtual void resetRasterLayerProvider(const RasterMapLayerId layerId, bool forcedUpdate = false) = 0;
        virtual void setRasterLayerOpacity(const RasterMapLayerId layerId, const float opacity, bool forcedUpdate = false) = 0;
        virtual void setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& tileProvider, bool forcedUpdate = false) = 0;
        virtual void resetElevationDataProvider(bool forcedUpdate = false) = 0;
        virtual void setElevationDataScaleFactor(const float factor, bool forcedUpdate = false) = 0;
        virtual void addSymbolProvider(const std::shared_ptr<IMapDataProvider>& provider, bool forcedUpdate = false) = 0;
        virtual void removeSymbolProvider(const std::shared_ptr<IMapDataProvider>& provider, bool forcedUpdate = false) = 0;
        virtual void removeAllSymbolProviders(bool forcedUpdate = false) = 0;
        virtual void setWindowSize(const PointI& windowSize, bool forcedUpdate = false) = 0;
        virtual void setViewport(const AreaI& viewport, bool forcedUpdate = false) = 0;
        virtual void setFieldOfView(const float fieldOfView, bool forcedUpdate = false) = 0;
        virtual void setDistanceToFog(const float fogDistance, bool forcedUpdate = false) = 0;
        virtual void setFogOriginFactor(const float factor, bool forcedUpdate = false) = 0;
        virtual void setFogHeightOriginFactor(const float factor, bool forcedUpdate = false) = 0;
        virtual void setFogDensity(const float fogDensity, bool forcedUpdate = false) = 0;
        virtual void setFogColor(const FColorRGB& color, bool forcedUpdate = false) = 0;
        virtual void setSkyColor(const FColorRGB& color, bool forcedUpdate = false) = 0;
        virtual void setAzimuth(const float azimuth, bool forcedUpdate = false) = 0;
        virtual void setElevationAngle(const float elevationAngle, bool forcedUpdate = false) = 0;
        virtual void setTarget(const PointI& target31, bool forcedUpdate = false) = 0;
        virtual void setZoom(const float zoom, bool forcedUpdate = false) = 0;

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

        OSMAND_CALLABLE(StateChangeObserver,
            void,
            const IMapRenderer* const mapRenderer,
            const MapRendererStateChange thisChange,
            const MapRendererStateChanges allChanges);
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
