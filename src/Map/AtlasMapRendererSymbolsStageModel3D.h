#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_MODEL_3D_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_MODEL_3D_H_

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "AtlasMapRendererSymbolsStage.h"
#include "Model3DMapSymbol.h"

namespace OsmAnd
{
    class AtlasMapRendererSymbolsStageModel3D
    {
    public:
        typedef AtlasMapRendererSymbolsStage::RenderableModel3DSymbol RenderableModel3DSymbol;
        typedef AtlasMapRendererSymbolsStage::ScreenQuadTree ScreenQuadTree;

    private:
        AtlasMapRenderer* getRenderer() const;

        glm::mat4 calculateModelMatrix(
            const Model3D::BBox& bbox,
            const glm::vec3& positionInWorld,
            const float direction,
            const float scale,
            bool& outElevated) const;
        QVector<float> getHeightOfPointsOnSegment(
            const glm::vec2& startInWorld,
            const glm::vec2& endInWorld,
            const float startHeight,
            const float endHeight) const;
        float getPointInWorldHeight(const glm::vec2& pointInWorld) const;
        PointI getLocation31FromPointInWorld(const glm::vec2& pointInWorld) const;
        
    protected:
    public:
        AtlasMapRendererSymbolsStageModel3D(AtlasMapRendererSymbolsStage* const symbolsStage);
        virtual ~AtlasMapRendererSymbolsStageModel3D();

        AtlasMapRendererSymbolsStage* const symbolsStage;

        const std::unique_ptr<GPUAPI>& gpuAPI;
        const MapRendererState& currentState;

        virtual bool initialize() = 0;
        virtual bool render(
            const std::shared_ptr<const RenderableModel3DSymbol>& renderable,
            AlphaChannelType& currentAlphaChannelType) = 0 ;
        virtual bool release(const bool gpuContextLost) = 0;

        void obtainRenderables(
            const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
            const std::shared_ptr<const Model3DMapSymbol>& model3DMapSymbol,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            QList< std::shared_ptr<AtlasMapRendererSymbolsStage::RenderableSymbol> >& outRenderableSymbols,
            const bool allowFastCheckByFrustum = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);
        bool plotSymbol(
            const std::shared_ptr<RenderableModel3DSymbol>& renderable,
            ScreenQuadTree& intersections,
            const bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_MODEL_3D_H_)