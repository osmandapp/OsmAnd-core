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
    class MapSymbol;
    class RasterMapSymbol;

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

            bool is2D;
            glm::vec2 directionInWorld;
            glm::vec2 directionOnScreen;

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
            const MapRenderer::PublishedMapSymbols& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const;

        void obtainRenderablesFromOnPathSymbols(
            const MapRenderer::PublishedMapSymbols& input,
            QList< std::shared_ptr<RenderableOnPathSymbol> >& output) const;

        QVector<glm::vec2> convertPoints31ToWorld(const QVector<PointI>& points31) const;
        QVector<glm::vec2> convertPoints31ToWorld(const QVector<PointI>& points31, unsigned int startIndex, unsigned int endIndex) const;
        QVector<glm::vec2> projectFromWorldToScreen(const QVector<glm::vec2>& pointsInWorld) const;
        QVector<glm::vec2> projectFromWorldToScreen(const QVector<glm::vec2>& pointsInWorld, unsigned int startIndex, unsigned int endIndex) const;
        bool isInclineAllowedFor2D(const glm::vec2& pointOnScreen0, const glm::vec2& pointOnScreen1) const;
        bool tryToFindSpaceOnPath(
            const unsigned int pathSize,
            const QVector<glm::vec2>& pathInWorld,
            const QVector<glm::vec2>& pathOnScreen,
            const float requestedLengthInPixels,
            const unsigned int startPointIndex,
            const bool alreadyOccupiedLengthIsIn2D,
            const float alreadyOccupiedLength,
            unsigned int& outEndPointIndex,
            bool& outLastOccupiedLengthIsIn2D,
            float& outLastOccupiedLength) const;
        glm::vec2 computeSubpathDirection(
            const QVector<glm::vec2>& path,
            const unsigned int startPointIndex,
            const unsigned int endPointIndex) const;
        /*QVector<RenderableOnPathSymbol::GlyphPlacement> computePlacementOfGlyphsOnPath(
            const bool is2D,
            const QVector<glm::vec2>& pathInWorld,
            const QVector<glm::vec2>& pathOnScreen,
            const unsigned int startPointIndex,
            const unsigned int endPointIndex,
            const float offsetFromStart) const;*/
        //void placeGlyphsOnPathSymbolSubpath(
        //    const std::shared_ptr<RenderableOnPathSymbol>& renderable,
        //    bool& outShouldInvert,
        //    glm::vec2& outDirectionOnScreenN) const;
        double computeDistanceBetweenCameraToPath(
            const QVector<glm::vec2>& pathInWorld,
            const unsigned int startPointIndex,
            const unsigned int endPointIndex) const;

        void sortRenderablesFromOnPathSymbols(
            const QList< std::shared_ptr<RenderableOnPathSymbol> >& entries,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const;

        void processBillboardSymbols(
            const MapRenderer::PublishedMapSymbols& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const;

        void obtainAndSortRenderablesFromBillboardSymbols(
            const MapRenderer::PublishedMapSymbols& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const;

        void processOnSurfaceSymbols(const MapRenderer::PublishedMapSymbols& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const;

        void obtainAndSortRenderablesFromOnSurfaceSymbols(
            const MapRenderer::PublishedMapSymbols& input,
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

        bool applyIntersectionWithOtherSymbolsFiltering(
            const AreaI boundsInWindow,
            const std::shared_ptr<const MapSymbol>& symbol,
            const IntersectionsQuadTree& intersections) const;
        bool applyIntersectionWithOtherSymbolsFiltering(
            const OOBBF oobb,
            const std::shared_ptr<const MapSymbol>& symbol,
            const IntersectionsQuadTree& intersections) const;
        bool applyMinDistanceToSameContentFromOtherSymbolFiltering(
            const AreaI boundsInWindow,
            const std::shared_ptr<const RasterMapSymbol>& symbol,
            const IntersectionsQuadTree& intersections) const;
        bool applyMinDistanceToSameContentFromOtherSymbolFiltering(
            const OOBBF oobb,
            const std::shared_ptr<const RasterMapSymbol>& symbol,
            const IntersectionsQuadTree& intersections) const;
        OOBBF calculateOnPath2dOOBB(const std::shared_ptr<RenderableOnPathSymbol>& renderable) const;
        OOBBF calculateOnPath3dOOBB(const std::shared_ptr<RenderableOnPathSymbol>& renderable) const;
        bool plotSymbol(
            const AreaI boundsInWindow,
            const std::shared_ptr<const MapSymbol>& symbol,
            IntersectionsQuadTree& intersections) const;
        bool plotSymbol(
            const OOBBF oobb,
            const std::shared_ptr<const MapSymbol>& symbol,
            IntersectionsQuadTree& intersections) const;
        static std::shared_ptr<const GPUAPI::ResourceInGPU> captureGpuResource(
            const MapRenderer::MapSymbolreferenceOrigins& resources,
            const std::shared_ptr<const MapSymbol>& mapSymbol);
    protected:
        void obtainRenderableSymbols(
            QList< std::shared_ptr<const RenderableSymbol> >& outRenderableSymbols) const;
    public:
        AtlasMapRendererSymbolsStage(AtlasMapRenderer* const renderer);
        virtual ~AtlasMapRendererSymbolsStage();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_H_)
