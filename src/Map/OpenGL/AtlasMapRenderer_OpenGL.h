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
        double _depthBufferRange;
        std::vector<std::byte> _terrainDepthBuffer;
        PointI _terrainDepthBufferSize;

        void updateFrustum(InternalState* internalState, const MapRendererState& state) const;
        void computeVisibleTileset(InternalState* internalState, const MapRendererState& state) const;

        // State-related:
        InternalState _internalState;
        const MapRendererInternalState* getInternalStateRef() const override;
        MapRendererInternalState* getInternalStateRef() override;
        const MapRendererInternalState& getInternalState() const override;
        MapRendererInternalState& getInternalState() override;
        bool updateInternalState(
            MapRendererInternalState& outInternalState, const MapRendererState& state, const MapRendererConfiguration& configuration) const override;

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
        double getPixelsToMetersScaleFactor(const MapRendererState& state, const MapRendererInternalState& internalState) const override;
    public:
        AtlasMapRenderer_OpenGL(GPUAPI_OpenGL* gpuAPI);
        virtual ~AtlasMapRenderer_OpenGL();

        const double depthBufferRange;
        const std::vector<std::byte>& terrainDepthBuffer;
        const PointI& terrainDepthBufferSize;

        float getTileSizeOnScreenInPixels() const override;

        bool getLocationFromScreenPoint(const PointI& screenPoint, PointI& location31) const override;
        bool getLocationFromScreenPoint(const PointI& screenPoint, PointI64& location) const override;

        AreaI getVisibleBBox31() const override;
        bool isPositionVisible(const PointI64& position) const override;
        bool isPositionVisible(const PointI& position31) const override;
        bool obtainScreenPointFromPosition(const PointI64& position, PointI& outScreenPoint) const override;
        bool obtainScreenPointFromPosition(const PointI& position31, PointI& outScreenPoint, bool checkOffScreen = false) const override;

        double getTileSizeInMeters() const override;
        double getPixelsToMetersScaleFactor() const override;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_H_)
