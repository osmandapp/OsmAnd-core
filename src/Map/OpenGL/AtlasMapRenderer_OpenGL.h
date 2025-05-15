#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_H_

#include "stdlib_common.h"

#include <glm/glm.hpp>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRenderer.h"
#include "AtlasMapRendererInternalState_OpenGL.h"
#include "AtlasMapRendererSkyStage_OpenGL.h"
#include "AtlasMapRendererMapLayersStage_OpenGL.h"
#include "AtlasMapRendererSymbolsStage_OpenGL.h"
#include "AtlasMapRendererDebugStage_OpenGL.h"
#include "OpenGL/GPUAPI_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRenderer_OpenGL : public AtlasMapRenderer
    {
        Q_DISABLE_COPY_AND_MOVE(AtlasMapRenderer_OpenGL);
    public:
        // Short type aliases:
        typedef AtlasMapRendererInternalState_OpenGL InternalState;
    private:
    protected:
        const static float _zNear;
        const static double _radius;
        const static double _minimumAngleForAdvancedHorizon;
        const static double _distancePerAngleFactor;
        const static double _maximumAbsoluteLatitudeForRealHorizon;
        const static double _minimumSkyHeightInKilometers;
        const static double _maximumHeightFromSeaLevelInMeters;
        const static double _maximumDepthFromSeaLevelInMeters;
        const static double _detailDistanceFactor;
        const static float _invalidElevationValue;
        const static float _minimumVisualZoom;
        const static float _maximumVisualZoom;
        const static double _minimumElevationAngle;

        void updateFrustum(InternalState* internalState, const MapRendererState& state) const;
        void computeTileset(const TileId targetTileId, const PointF targetInTileOffsetN,
            const PointF* points, QSet<TileId>* frustumTiles) const;
        void computeVisibleTileset(InternalState* internalState, const MapRendererState& state, const float highDetail,
            const float visibleDistance, const double elevationCosine, const bool sortTiles) const;
        void computeUniqueTileset(InternalState* internalState, const MapRendererState& state,
            const ZoomLevel zoomLevel, const TileId targetTileId, const bool sortTiles) const;
        bool isPointVisible(const InternalState& internalState, const glm::vec3& point, bool skipTop,
            bool skipLeft, bool skipBottom, bool skipRight, bool skipFront, bool skipBack, float tolerance = 0.0f) const;
        bool isPointInsideTileBox(const glm::vec3& point, const glm::vec3& minPoint, const glm::vec3& maxPoint,
            bool skipTop, bool skipLeft, bool skipBottom, bool skipRight, bool skipFront, bool skipBack) const;
        bool isRayOnTileBox(const glm::vec3& startPoint, const glm::vec3& endPoint,
            const glm::vec3& minPoint, const glm::vec3& maxPoint) const;
        bool isEdgeVisible(const InternalState& internalState,
            const glm::vec3& startPoint, const glm::vec3& endPoint) const;
        bool isTileVisible(const InternalState& internalState,
            const glm::vec3& minPoint, const glm::vec3& maxPoint) const;
        double getZoomOffset(const ZoomLevel zoomLevel, const double visualZoom, const double distanceFactor) const;
        void getElevationDataLimits(const MapRendererState& state,
            std::shared_ptr<const IMapElevationDataProvider::Data>& elevationData,
            const TileId& tileId, const ZoomLevel zoomLevel, float& minHeight, float& maxHeight) const;
        bool getHeightLimits(const MapRendererState& state,
            const TileId& tileId, const ZoomLevel zoomLevel, float& minHeight, float& maxHeight) const;
        double getRealDistanceToHorizon(const InternalState& internalState, const MapRendererState& state,
            const PointD& groundPosition, const double inglobeAngle, const double referenceDistance) const;
        bool getPositionFromScreenPoint(const InternalState& internalState, const MapRendererState& state,
            const PointI& screenPoint, PointD& position,
            const float height = 0.0f, float* distance = nullptr, float* sinAngle = nullptr) const;
        bool getPositionFromScreenPoint(const InternalState& internalState, const MapRendererState& state,
            const PointD& screenPoint, PointD& position,
            const float height = 0.0f, float* distance = nullptr, float* sinAngle = nullptr) const;
        bool getNearestLocationFromScreenPoint(const InternalState& internalState, const MapRendererState& state,
            const PointI& location31, const PointI& screenPoint,
            PointI64& fixedLocation, PointI64& currentLocation) const;
        std::shared_ptr<const GPUAPI::ResourceInGPU> captureElevationDataResource(const MapRendererState& state,
            TileId normalizedTileId, ZoomLevel zoomLevel,
            std::shared_ptr<const IMapElevationDataProvider::Data>* pOutSource = nullptr) const;
        OsmAnd::ZoomLevel getElevationData(const MapRendererState& state,
            TileId normalizedTileId, ZoomLevel zoomLevel, PointF& offsetInTileN, bool noUnderscaled,
            std::shared_ptr<const IMapElevationDataProvider::Data>* pOutSource = nullptr) const;

        // State-related:
        InternalState _internalState;
        const MapRendererInternalState* getInternalStateRef() const override;
        MapRendererInternalState* getInternalStateRef() override;
        const MapRendererInternalState& getInternalState() const override;
        MapRendererInternalState& getInternalState() override;
        bool updateInternalState(
            MapRendererInternalState& outInternalState, const MapRendererState& state,
            const MapRendererConfiguration& configuration,
            const bool skipTiles = false, const bool sortTiles = false) const override;

        // Resources:
        void onValidateResourcesOfType(MapRendererResourceType type) override;

        // Customization points:
        bool doInitializeRendering(bool reinitialize) override;
        bool doRenderFrame(IMapRenderer_Metrics::Metric_renderFrame* metric) override;
        bool doReleaseRendering(bool gpuContextLost) override;
        bool handleStateChange(const MapRendererState& state, MapRendererStateChanges mask) override;
        void flushRenderCommands() override;

        GPUAPI_OpenGL* getGPUAPI() const;

        // Stages:
        AtlasMapRendererSkyStage* createSkyStage() override;
        AtlasMapRendererMapLayersStage* createMapLayersStage() override;
        AtlasMapRendererSymbolsStage* createSymbolsStage() override;
        AtlasMapRendererDebugStage* createDebugStage() override;

        AreaI getVisibleBBox31(const MapRendererInternalState& internalState) const override;
        AreaI getVisibleBBoxShifted(const MapRendererInternalState& internalState) const override;
        double getPixelsToMetersScaleFactor(const MapRendererState& state, const MapRendererInternalState& internalState) const override;
        bool getNewTargetByScreenPoint(const MapRendererState& state,
            const PointI& screenPoint, const PointI& location31, PointI& target31, const float height = 0.0f) const override;
        bool getLocationFromElevatedPoint(const MapRendererState& state, const PointI& screenPoint, PointI& location31,
            float* heightInMeters = nullptr) const override;
        bool getExtraZoomAndTiltForRelief(const MapRendererState& state, PointF& zoomAndTilt) const override;
        bool getExtraZoomAndRotationForAiming(const MapRendererState& state,
            const PointI& firstLocation31, const float firstHeightInMeters, const PointI& firstPoint,
            const PointI& secondLocation31, const float secondHeightInMeters, const PointI& secondPoint,
            PointD& zoomAndRotate) const override;
        bool getTiltAndRotationForAiming(const MapRendererState& state,
            const PointI& firstLocation31, const float firstHeight, const PointI& firstPoint,
            const PointI& secondLocation31, const float secondHeight, const PointI& secondPoint,
            PointD& tiltAndRotate) const override;
        bool isLocationHeightAvailable(const MapRendererState& state, const PointI& location31) const override;
        float getLocationHeightInMeters(const MapRendererState& state, const PointI& location31) const override;
        float getHeightOfLocation(const MapRendererState& state, const PointI& location31) const override;
        bool getProjectedLocation(const MapRendererInternalState& internalState, const MapRendererState& state,
            const PointI& location31, const float height, PointI& outLocation31) const override;
        bool getLastProjectablePoint(const MapRendererInternalState& internalState,
            const glm::vec3& startPoint, const glm::vec3& endPoint, glm::vec3& visiblePoint) const override;
        bool getLastVisiblePoint(const MapRendererInternalState& internalState,
            const glm::vec3& startPoint, const glm::vec3& endPoint, glm::vec3& visiblePoint) const override;
        bool isPointProjectable(const MapRendererInternalState& internalState, const glm::vec3& point) const override;
        bool isPointVisible(const MapRendererInternalState& internalState, const glm::vec3& point, bool skipTop = false,
            bool skipLeft = false, bool skipBottom = false, bool skipRight = false, bool skipFront = false, bool skipBack = false, float tolerance = 0.0) const override;
        bool getWorldPointFromScreenPoint(
            const MapRendererInternalState& internalState,
            const MapRendererState& state,
            const PointI& screenPoint,
            PointF& outWorldPoint) const override;
        float getWorldElevationOfLocation(const MapRendererState& state,
            const float elevationInMeters, const PointI& location31) const override;
        float getElevationOfLocationInMeters(const MapRendererState& state,
            const float elevation, const ZoomLevel zoom, const PointI& location31) const override;
        double getDistanceFactor(const MapRendererState& state, const float tileSize,
            double& baseUnits, float& sinAngleToPlane) const override;
        OsmAnd::ZoomLevel getSurfaceZoom(const MapRendererState& state, float& surfaceVisualZoom) const override;
        OsmAnd::ZoomLevel getFlatZoom(const MapRendererState& state, const ZoomLevel surfaceZoomLevel,
            const float surfaceVisualZoom, const double& pointElevation, float& flatVisualZoom) const override;

    public:
        AtlasMapRenderer_OpenGL(GPUAPI_OpenGL* gpuAPI);
        virtual ~AtlasMapRenderer_OpenGL();

        float getTileSizeOnScreenInPixels() const override;

        bool getLocationFromScreenPoint(const PointI& screenPoint, PointI& location31) const override;
        bool getLocationFromScreenPoint(const PointD& screenPoint, PointI& location31) const override;
        bool getLocationFromScreenPoint(const PointD& screenPoint, PointI64& location) const override;
        bool getLocationFromElevatedPoint(const PointI& screenPoint, PointI& location31,
            float* heightInMeters = nullptr) const override;
        float getHeightAndLocationFromElevatedPoint(const PointI& screenPoint, PointI& location31) const override;
        bool getZoomAndRotationAfterPinch(
            const PointI& firstLocation31, const float firstHeightInMeters, const PointI& firstPoint,
            const PointI& secondLocation31, const float secondHeightInMeters, const PointI& secondPoint,
            PointD& zoomAndRotate) const override;
        bool getTiltAndRotationAfterMove(
            const PointI& firstLocation31, const float firstHeightInMeters, const PointI& firstPoint,
            const PointI& secondLocation31, const float secondHeightInMeters, const PointI& secondPoint,
            PointD& tiltAndRotate) const override;

        float getLocationHeightInMeters(const PointI& location31) const override;
        bool getNewTargetByScreenPoint(const PointI& screenPoint, const PointI& location31,
            PointI& target31, const float height = 0.0f) const override;
        float getHeightOfLocation(const PointI& location31) const override;
        float getMapTargetDistance(const PointI& location31, bool checkOffScreen = false) const override;

        AreaI getVisibleBBox31() const override;
        AreaI getVisibleBBoxShifted() const override;
        bool isPositionVisible(const PointI64& position) const override;
        bool isPositionVisible(const PointI& position31) const override;
        bool isPathVisible(const QVector<PointI>& path31) const override;
        bool isAreaVisible(const AreaI& area31) const override;
        bool isTileVisible(const int tileX, const int tileY, const int zoom) const override;
        bool obtainScreenPointFromPosition(const PointI64& position, PointI& outScreenPoint) const override;
        bool obtainScreenPointFromPosition(const PointI& position31, PointI& outScreenPoint, bool checkOffScreen = false) const override;
        bool obtainElevatedPointFromPosition(const PointI& position31, PointI& outScreenPoint, bool checkOffScreen = false) const override;

        float getCameraHeightInMeters() const override;
        int getTileZoomOffset() const override;
        
        double getTileSizeInMeters() const override;
        double getPixelsToMetersScaleFactor() const override;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_H_)
