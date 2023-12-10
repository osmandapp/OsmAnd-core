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
    public:
        virtual ~IMapRenderer();

        virtual MapRendererSetupOptions getSetupOptions() const = 0;
        virtual bool setup(const MapRendererSetupOptions& setupOptions) = 0;

        virtual std::shared_ptr<MapRendererConfiguration> getConfiguration() const = 0;
        virtual void setConfiguration(const std::shared_ptr<const MapRendererConfiguration>& configuration, bool forcedUpdate = false) = 0;

        virtual bool isRenderingInitialized() const = 0;
        virtual bool initializeRendering(bool renderTargetAvailable) = 0;
        virtual bool update(IMapRenderer_Metrics::Metric_update* const metric = nullptr) = 0;
        virtual bool prepareFrame(IMapRenderer_Metrics::Metric_prepareFrame* const metric = nullptr) = 0;
        virtual bool renderFrame(IMapRenderer_Metrics::Metric_renderFrame* const metric = nullptr) = 0;
        virtual bool releaseRendering(bool gpuContextLost = false) = 0;

        virtual bool attachToRenderTarget() = 0;
        virtual bool isAttachedToRenderTarget() = 0;
        virtual bool detachFromRenderTarget() = 0;

        virtual bool isIdle() const = 0;
        virtual QString getNotIdleReason() const = 0;

        virtual bool isGpuWorkerPaused() const = 0;
        virtual bool suspendGpuWorker() = 0;
        virtual bool resumeGpuWorker() = 0;

        OSMAND_OBSERVER_CALLABLE(FramePrepared,
            IMapRenderer* mapRenderer);
        const ObservableAs<IMapRenderer::FramePrepared> framePreparedObservable;
        
        OSMAND_OBSERVER_CALLABLE(TargetChanged,
            IMapRenderer* mapRenderer);
        const ObservableAs<IMapRenderer::TargetChanged> targetChangedObservable;

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
        virtual int getSymbolsUpdateInterval() = 0;
        virtual void setSymbolsUpdateInterval(int interval) = 0;
        virtual void shouldUpdateSymbols() = 0;
        virtual bool needUpdatedSymbols() = 0;
        virtual void dontNeedUpdatedSymbols() = 0;
        virtual void setSymbolsUpdated() = 0;
        virtual bool freshSymbols() = 0;
        virtual void clearSymbolsUpdated() = 0;

        virtual bool setMapLayerProvider(const int layerIndex, const std::shared_ptr<IMapLayerProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool resetMapLayerProvider(const int layerIndex, bool forcedUpdate = false) = 0;
        virtual bool setMapLayerConfiguration(const int layerIndex, const MapLayerConfiguration& configuration, bool forcedUpdate = false) = 0;

        virtual bool setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool resetElevationDataProvider(bool forcedUpdate = false) = 0;
        virtual bool setElevationConfiguration(const ElevationConfiguration& configuration, bool forcedUpdate = false) = 0;

        virtual bool addSymbolsProvider(const std::shared_ptr<IMapTiledSymbolsProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool addSymbolsProvider(const int subsectionIndex, const std::shared_ptr<IMapTiledSymbolsProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool addSymbolsProvider(const std::shared_ptr<IMapKeyedSymbolsProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool addSymbolsProvider(const int subsectionIndex, const std::shared_ptr<IMapKeyedSymbolsProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool hasSymbolsProvider(const std::shared_ptr<IMapTiledSymbolsProvider>& provider) = 0;
        virtual bool hasSymbolsProvider(const std::shared_ptr<IMapKeyedSymbolsProvider>& provider) = 0;
        virtual int getSymbolsProviderSubsection(const std::shared_ptr<IMapTiledSymbolsProvider>& provider) = 0;
        virtual int getSymbolsProviderSubsection(const std::shared_ptr<IMapKeyedSymbolsProvider>& provider) = 0;
        virtual bool removeSymbolsProvider(const std::shared_ptr<IMapTiledSymbolsProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool removeSymbolsProvider(const std::shared_ptr<IMapKeyedSymbolsProvider>& provider, bool forcedUpdate = false) = 0;
        virtual bool removeAllSymbolsProviders(bool forcedUpdate = false) = 0;
        virtual bool setSymbolSubsectionConfiguration(const int subsectionIndex, const SymbolSubsectionConfiguration& configuration, bool forcedUpdate = false) = 0;

        virtual bool setWindowSize(const PointI& windowSize, bool forcedUpdate = false) = 0;
        virtual bool setViewport(const AreaI& viewport, bool forcedUpdate = false) = 0;
        virtual bool setFieldOfView(const float fieldOfView, bool forcedUpdate = false) = 0;
        virtual bool setVisibleDistance(const float visibleDistance, bool forcedUpdate = false) = 0;
        virtual bool setDetailedDistance(const float detailedDistance, bool forcedUpdate = false) = 0;
        virtual bool setSkyColor(const FColorRGB& color, bool forcedUpdate = false) = 0;
        virtual bool setAzimuth(const float azimuth, bool forcedUpdate = false) = 0;
        virtual bool setElevationAngle(const float elevationAngle, bool forcedUpdate = false) = 0;
        virtual bool setTarget(const PointI& target31, bool forcedUpdate = false, bool disableUpdate = false) = 0;
        virtual bool setMapTarget(const PointI& screenPoint, const PointI& location31,
            bool forcedUpdate = false, bool disableUpdate = false) = 0;
        virtual bool resetMapTarget() = 0;
        virtual bool resetMapTargetPixelCoordinates(const PointI& screenPoint) = 0;
        virtual bool setMapTargetPixelCoordinates(const PointI& screenPoint,
            bool forcedUpdate = false, bool disableUpdate = false) = 0;
        virtual bool setMapTargetLocation(const PointI& location31,
            bool forcedUpdate = false, bool disableUpdate = false) = 0;
        virtual bool setSecondaryTarget(const PointI& screenPoint, const PointI& location31,
            bool forcedUpdate = false, bool disableUpdate = false) = 0;
        virtual bool setSecondaryTargetPixelCoordinates(const PointI& screenPoint,
            bool forcedUpdate = false, bool disableUpdate = false) = 0;
        virtual bool setSecondaryTargetLocation(const PointI& location31,
            bool forcedUpdate = false, bool disableUpdate = false) = 0;
        virtual int getAimingActions() = 0;
        virtual bool setAimingActions(const int actionBits, bool forcedUpdate = false) = 0;
        virtual bool setFlatZoom(const float zoom, bool forcedUpdate = false) = 0;
        virtual bool setFlatZoom(const ZoomLevel zoomLevel, const float visualZoom, bool forcedUpdate = false) = 0;
        virtual bool setFlatZoomLevel(const ZoomLevel zoomLevel, bool forcedUpdate = false) = 0;
        virtual bool setFlatVisualZoom(const float visualZoom, bool forcedUpdate = false) = 0;
        virtual bool setZoom(const float zoom, bool forcedUpdate = false) = 0;
        virtual bool setZoom(const ZoomLevel zoomLevel, const float visualZoom, bool forcedUpdate = false) = 0;
        virtual bool setZoomLevel(const ZoomLevel zoomLevel, bool forcedUpdate = false) = 0;
        virtual bool setVisualZoom(const float visualZoom, bool forcedUpdate = false) = 0;
        virtual bool setVisualZoomShift(const float visualZoomShift, bool forcedUpdate = false) = 0;
        virtual bool restoreFlatZoom(const float heightInMeters, bool forcedUpdate = false) = 0;

        virtual bool setStubsStyle(const MapStubStyle style, bool forcedUpdate = false) = 0;

        virtual bool setBackgroundColor(const FColorRGB& color, bool forcedUpdate = false) = 0;
        virtual bool setFogColor(const FColorRGB& color, bool forcedUpdate = false) = 0;
        virtual bool setSymbolsOpacity(const float opacityFactor, bool forcedUpdate = false) = 0;
        virtual float getSymbolsOpacity() const = 0;
        virtual bool getMapTargetLocation(PointI& location31) const = 0;
        virtual bool getSecondaryTargetLocation(PointI& location31) const = 0;
        virtual float getMapTargetHeightInMeters() const = 0;

        virtual std::shared_ptr<MapRendererDebugSettings> getDebugSettings() const = 0;
        virtual void setDebugSettings(const std::shared_ptr<const MapRendererDebugSettings>& debugSettings) = 0;

        virtual void useJSON() = 0;
        virtual bool withJSON() const = 0;
        virtual void setJSON(const QJsonDocument* jsonDocument) = 0;
        virtual QByteArray getJSON() const = 0;

        virtual ZoomLevel getMinZoomLevel() const = 0;
        virtual ZoomLevel getMaxZoomLevel() const = 0;
        virtual bool setMinZoomLevel(const ZoomLevel zoomLevel, bool forcedUpdate = false) = 0;
        virtual bool setMaxZoomLevel(const ZoomLevel zoomLevel, bool forcedUpdate = false) = 0;

        virtual ZoomLevel getMinimalZoomLevelsRangeLowerBound() const = 0;
        virtual ZoomLevel getMinimalZoomLevelsRangeUpperBound() const = 0;
        virtual ZoomLevel getMaximalZoomLevelsRangeLowerBound() const = 0;
        virtual ZoomLevel getMaximalZoomLevelsRangeUpperBound() const = 0;

        OSMAND_OBSERVER_CALLABLE(StateChanged,
            const IMapRenderer* mapRenderer,
            MapRendererStateChange thisChange,
            SWIG_OMIT(const) MapRendererStateChanges allChanges);
        const ObservableAs<IMapRenderer::StateChanged> stateChangeObservable;

        //NOTE: screen points origin from top-left
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI& location31) const = 0;
        virtual bool getLocationFromScreenPoint(const PointD& screenPoint, PointI& location31) const = 0;
        virtual bool getLocationFromScreenPoint(const PointD& screenPoint, PointI64& location) const = 0;
        virtual bool getLocationFromElevatedPoint(const PointI& screenPoint, PointI& location31,
            float* heightInMeters = nullptr) const = 0;
        virtual float getHeightAndLocationFromElevatedPoint(const PointI& screenPoint, PointI& location31) const = 0;
        virtual float getSurfaceZoomAfterPinch(
            const PointI& firstLocation31, const float firstHeightInMeters, const PointI& firstScreenPoint,
            const PointI& secondLocation31, const float secondHeightInMeters, const PointI& secondScreenPoint) = 0;
        virtual float getSurfaceZoomAfterPinchWithParams(
            const PointI& fixedLocation31, float surfaceZoom, float rotation,
            const PointI& firstLocation31, const float firstHeightInMeters, const PointI& firstScreenPoint,
            const PointI& secondLocation31, const float secondHeightInMeters, const PointI& secondScreenPoint) = 0;
        virtual bool getZoomAndRotationAfterPinch(
            const PointI& firstLocation31, const float firstHeightInMeters, const PointI& firstPoint,
            const PointI& secondLocation31, const float secondHeightInMeters, const PointI& secondPoint,
            PointD& zoomAndRotate) const = 0;
        virtual bool getTiltAndRotationAfterMove(
            const PointI& firstLocation31, const float firstHeightInMeters, const PointI& firstPoint,
            const PointI& secondLocation31, const float secondHeightInMeters, const PointI& secondPoint,
            PointD& tiltAndRotate) const = 0;

        virtual float getLocationHeightInMeters(const PointI& location31) const = 0;

        virtual AreaI getVisibleBBox31() const = 0;
        virtual AreaI getVisibleBBoxShifted() const = 0;
        virtual bool isPositionVisible(const PointI64& position) const = 0;
        virtual bool isPositionVisible(const PointI& position31) const = 0;
        virtual bool isPathVisible(const QVector<PointI>& path31) const = 0;
        virtual bool isAreaVisible(const AreaI& area31) const = 0;
        virtual bool isTileVisible(const int tileX, const int tileY, const int zoom) const = 0;
        virtual bool obtainScreenPointFromPosition(const PointI64& position, PointI& outScreenPoint) const = 0;
        virtual bool obtainScreenPointFromPosition(const PointI& position31, PointI& outScreenPoint, bool checkOffScreen = false) const = 0;
        virtual bool obtainElevatedPointFromPosition(const PointI& position31, PointI& outScreenPoint, bool checkOffScreen = false) const = 0;
        virtual bool getNewTargetByScreenPoint(const PointI& screenPoint, const PointI& location31,
            PointI& target31, const float height = 0.0f) const = 0;
        virtual float getHeightOfLocation(const PointI& location31) const = 0;
        virtual float getMapTargetDistance(const PointI& location31, bool checkOffScreen = false) const = 0;

        virtual float getCameraHeightInMeters() const = 0;

        virtual double getTileSizeInMeters() const = 0;
        virtual double getPixelsToMetersScaleFactor() const = 0;
        virtual float getTileSizeOnScreenInPixels() const = 0;

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
        AtlasMapRenderer_OpenGLES2plus,
    };
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL createMapRenderer(const MapRendererClass mapRendererClass);
}

#endif // !defined(_OSMAND_CORE_I_MAP_RENDERER_H_)
