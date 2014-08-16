#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QReadWriteLock>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <glm/glm.hpp>
#include "restore_internal_warnings.h"

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

        typedef QuadTree< std::shared_ptr<const RenderableSymbol>, AreaI::CoordType > IntersectionsQuadTree;

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
        static QVector<float> computePathSegmentsLengths(const QVector<glm::vec2>& path);
        static bool computePointIndexAndOffsetFromOriginAndOffset(
            const QVector<float>& pathSegmentsLengths,
            const unsigned int originPathPointIndex,
            const float nOffsetFromOriginPathPoint,
            const float offsetToPoint,
            unsigned int& outPathPointIndex,
            float& outOffsetFromPathPoint);
        static glm::vec2 computeExactPointFromOriginAndOffset(
            const QVector<glm::vec2>& path,
            const QVector<float>& pathSegmentsLengths,
            const unsigned int originPathPointIndex,
            const float offsetFromOriginPathPoint);
        static glm::vec2 computeExactPointFromOriginAndNormalizedOffset(
            const QVector<glm::vec2>& path,
            const QVector<float>& pathSegmentsLengths,
            const unsigned int originPathPointIndex,
            const float nOffsetFromOriginPathPoint);
        static bool pathRenderableAs2D(
            const QVector<glm::vec2>& pathOnScreen,
            const unsigned int startPathPointIndex,
            const glm::vec2& exactStartPointOnScreen,
            const unsigned int endPathPointIndex,
            const glm::vec2& exactEndPointOnScreen);
        static bool segmentValidFor2D(const glm::vec2& vSegment);
        static glm::vec2 computePathDirection(
            const QVector<glm::vec2>& path,
            const unsigned int startPathPointIndex,
            const glm::vec2& exactStartPoint,
            const unsigned int endPathPointIndex,
            const glm::vec2& exactEndPoint);
        glm::vec2 computeSubpathDirectionInWorld(
            const QVector<glm::vec2>& pathInWorld,
            const float offsetFromStart,
            const float offsetToEnd,
            const unsigned int startPointIndex,
            const unsigned int endPointIndex,
            glm::vec2* outExactStartPoint = nullptr,
            glm::vec2* outExactEndPoint = nullptr) const;
        glm::vec2 computePathDirectionOnScreen(
            const QVector<glm::vec2>& pathOnScreen,
            const glm::vec2& exactStartPointInWorld,
            const glm::vec2& exactEndPointInWorld,
            glm::vec2* outExactStartPointOnScreen = nullptr,
            glm::vec2* outExactEndPointOnScreen = nullptr) const;
        QVector<RenderableOnPathSymbol::GlyphPlacement> computePlacementOfGlyphsOnPath(
            const bool is2D,
            const QVector<glm::vec2>& pathInWorld,
            const unsigned int startPointIndex,
            const unsigned int endPointIndex,
            const glm::vec2& exactStartPointInWorld,
            const glm::vec2& exactEndPointInWorld,
            const QVector<glm::vec2>& subpathOnScreen,
            const glm::vec2& exactStartPointOnScreen,
            const glm::vec2& exactEndPointOnScreen,
            const glm::vec2& directionOnScreen,
            const QVector<float>& glyphsWidths) const;
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
            const std::shared_ptr<const RenderableSymbol>& renderable,
            const IntersectionsQuadTree& intersections) const;
        bool applyIntersectionWithOtherSymbolsFiltering(
            const OOBBF oobb,
            const std::shared_ptr<const RenderableSymbol>& renderable,
            const IntersectionsQuadTree& intersections) const;
        bool applyMinDistanceToSameContentFromOtherSymbolFiltering(
            const AreaI boundsInWindow,
            const std::shared_ptr<const RenderableSymbol>& renderable,
            const IntersectionsQuadTree& intersections) const;
        bool applyMinDistanceToSameContentFromOtherSymbolFiltering(
            const OOBBF oobb,
            const std::shared_ptr<const RenderableSymbol>& renderable,
            const IntersectionsQuadTree& intersections) const;
        OOBBF calculateOnPath2dOOBB(const std::shared_ptr<RenderableOnPathSymbol>& renderable) const;
        OOBBF calculateOnPath3dOOBB(const std::shared_ptr<RenderableOnPathSymbol>& renderable) const;
        bool plotRenderable(
            const AreaI boundsInWindow,
            const std::shared_ptr<const RenderableSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;
        bool plotRenderable(
            const OOBBF oobb,
            const std::shared_ptr<const RenderableSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;
        static std::shared_ptr<const GPUAPI::ResourceInGPU> captureGpuResource(
            const MapRenderer::MapSymbolreferenceOrigins& resources,
            const std::shared_ptr<const MapSymbol>& mapSymbol);

        void obtainRenderableSymbols(
            QList< std::shared_ptr<const RenderableSymbol> >& outRenderableSymbols,
            IntersectionsQuadTree& outIntersections) const;

        mutable QReadWriteLock _lastPreparedIntersectionsLock;
        IntersectionsQuadTree _lastPreparedIntersections;
    protected:
        QList< std::shared_ptr<const RenderableSymbol> > renderableSymbols;

        void prepare();
    public:
        AtlasMapRendererSymbolsStage(AtlasMapRenderer* const renderer);
        virtual ~AtlasMapRendererSymbolsStage();

        void queryLastPreparedSymbolsAt(
            const PointI screenPoint,
            QList< std::shared_ptr<const MapSymbol> >& outMapSymbols) const;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_H_)
