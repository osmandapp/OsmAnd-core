#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include <glm/glm.hpp>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "QuadTree.h"
#include "AtlasMapRendererStage.h"
#include "GPUAPI.h"

namespace OsmAnd
{
    class AtlasMapRendererSymbolsStage : public AtlasMapRendererStage
    {
    public:
        struct RenderableSymbol
        {
            virtual ~RenderableSymbol();

            std::shared_ptr<const MapSymbol> mapSymbol;
            std::shared_ptr<const GPUAPI::ResourceInGPU> gpuResource;
            double distanceToCamera;
        };

        struct RenderableBillboardSymbol : RenderableSymbol
        {
            virtual ~RenderableBillboardSymbol();

            PointI offsetFromTarget31;
            PointF offsetFromTarget;
            glm::vec3 positionInWorld;
        };

        struct RenderableOnPathSymbol : RenderableSymbol
        {
            virtual ~RenderableOnPathSymbol();

            int subpathStartIndex;
            int subpathEndIndex;
            QVector<glm::vec2> subpathPointsInWorld;
            float offset;
            float subpathLength;
            QVector<float> segmentLengths;
            glm::vec2 subpathDirectionOnScreen;
            bool is2D;

            struct GlyphPlacement
            {
                inline GlyphPlacement()
                    : width(qSNaN())
                    , angle(qSNaN())
                {
                }

                inline GlyphPlacement(
                    const glm::vec2& anchorPoint_,
                    const float width_,
                    const float angle_,
                    const glm::vec2& vNormal_)
                    : anchorPoint(anchorPoint_)
                    , width(width_)
                    , angle(angle_)
                    , vNormal(vNormal_)
                {
                }

                glm::vec2 anchorPoint;
                float width;
                float angle;
                glm::vec2 vNormal;
            };
            QVector< GlyphPlacement > glyphsPlacement;

            // 2D-only:
            QVector<glm::vec2> subpathPointsOnScreen;

            // 3D-only:
            glm::vec2 subpathDirectioInWorld;
        };

        struct RenderableOnSurfaceSymbol : RenderableSymbol
        {
            virtual ~RenderableOnSurfaceSymbol();

            PointI offsetFromTarget31;
            PointF offsetFromTarget;
            glm::vec3 positionInWorld;

            float direction;
        };

        typedef QuadTree< std::shared_ptr<const MapSymbol>, AreaI::CoordType > IntersectionsQuadTree;

    private:
        void processOnPathSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const;

        // Obtains visible portions of each OnPathSymbol
        void obtainRenderableEntriesForOnPathSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QList< std::shared_ptr<RenderableOnPathSymbol> >& output) const;

        // Calculates renderable OnPathSymbols to in world
        void calculateRenderableOnPathSymbolsInWorld(
            QList< std::shared_ptr<RenderableOnPathSymbol> >& entries) const;

        // Determines if each renderable OnPathSymbol is 2D-mode or 3D-mode
        void determineRenderableOnPathSymbolsMode(
            QList< std::shared_ptr<RenderableOnPathSymbol> >& entries) const;

        // Adjusts renderable OnPathSymbol bitmap placement on entire path
        void adjustPlacementOfGlyphsOnPath(
            QList< std::shared_ptr<RenderableOnPathSymbol> >& entries) const;

        void sortRenderableOnPathSymbols(
            const QList< std::shared_ptr<RenderableOnPathSymbol> >& entries,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const;

        void processBillboardSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const;

        void obtainAndSortBillboardSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const;

        void processOnSurfaceSymbols(const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const;

        void obtainAndSortOnSurfaceSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const;

        bool plotBillboardSymbol(
            const std::shared_ptr<RenderableBillboardSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;
        bool plotBillboardRasterSymbol(
            const std::shared_ptr<RenderableBillboardSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;
        bool plotBillboardVectorSymbol(
            const std::shared_ptr<RenderableBillboardSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;

        bool plotOnPathSymbol(
            const std::shared_ptr<RenderableOnPathSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;

        bool plotOnSurfaceSymbol(
            const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;
        bool plotOnSurfaceRasterSymbol(
            const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;
        bool plotOnSurfaceVectorSymbol(
            const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;
    protected:
        void obtainRenderableSymbols(
            QList< std::shared_ptr<const RenderableSymbol> >& outRenderableSymbols) const;
    public:
        AtlasMapRendererSymbolsStage(AtlasMapRenderer* const renderer);
        virtual ~AtlasMapRendererSymbolsStage();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_H_)
