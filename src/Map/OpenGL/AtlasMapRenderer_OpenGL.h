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
        std::vector<std::byte> _terrainDepthBuffer;
        PointI _terrainDepthBufferSize;

        void updateFrustum(InternalState* internalState, const MapRendererState& state) const;
        void computeTileset(const TileId targetTileId, const PointF targetInTileOffsetN,
            const PointF* points, QSet<TileId>* visibleTiles) const;
        void computeVisibleTileset(InternalState* internalState, const MapRendererState& state,
            const float visibleDistance, const double elevationCosine, const bool sortTiles) const;
        void computeUniqueTileset(InternalState* internalState,
            const ZoomLevel zoomLevel, const TileId targetTileId, const bool sortTiles) const;
        double getRealDistanceToHorizon(const InternalState& internalState, const MapRendererState& state,
            const PointD& groundPosition, const double inglobeAngle, const double referenceDistance) const;
        bool getPositionFromScreenPoint(const InternalState& internalState, const MapRendererState& state,
            const PointI& screenPoint, PointD& position, const float height = 0.0f, float* distance = nullptr) const;
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
        bool doInitializeRendering() override;
        bool doRenderFrame(IMapRenderer_Metrics::Metric_renderFrame* metric) override;
        bool doReleaseRendering(bool gpuContextLost) override;
        bool handleStateChange(const MapRendererState& state, MapRendererStateChanges mask) override;

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
        float getLocationHeightInMeters(const MapRendererState& state, const PointI& location31) const override;
        float getHeightOfLocation(const MapRendererState& state, const PointI& location31) const override;
        bool getProjectedLocation(const MapRendererInternalState& internalState, const MapRendererState& state,
            const PointI& location31, const float height, PointI& outLocation31) const override;
        bool getWorldPointFromScreenPoint(
            const MapRendererInternalState& internalState,
            const MapRendererState& state,
            const PointI& screenPoint,
            PointF& outWorldPoint) const override;
        float getWorldElevationOfLocation(const MapRendererState& state,
            const float elevationInMeters, const PointI& location31) const override;
        float getElevationOfLocationInMeters(const MapRendererState& state,
            const float elevation, const ZoomLevel zoom, const PointI& location31) const override;
    public:
        AtlasMapRenderer_OpenGL(GPUAPI_OpenGL* gpuAPI);
        virtual ~AtlasMapRenderer_OpenGL();

        const std::vector<std::byte>& terrainDepthBuffer;
        const PointI& terrainDepthBufferSize;

        float getTileSizeOnScreenInPixels() const override;

        bool getLocationFromScreenPoint(const PointI& screenPoint, PointI& location31) const override;
        bool getLocationFromScreenPoint(const PointI& screenPoint, PointI64& location) const override;
        bool getLocationFromElevatedPoint(const PointI& screenPoint, PointI& location31,
            float* heightInMeters = nullptr) const override;
        float getHeightAndLocationFromElevatedPoint(const PointI& screenPoint, PointI& location31) const override;
        bool getZoomAndRotationAfterPinch(
            const PointI& firstLocation31, const float firstHeight, const PointI& firstPoint,
            const PointI& secondLocation31, const float secondHeight, const PointI& secondPoint,
            PointD& zoomAndRotate) const override;

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
        
        double getTileSizeInMeters() const override;
        double getPixelsToMetersScaleFactor() const override;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_H_)
