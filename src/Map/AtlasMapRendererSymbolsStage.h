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
    class OnPathRasterMapSymbol;
    class IOnSurfaceMapSymbol;
    class IBillboardMapSymbol;

    class AtlasMapRendererSymbolsStage : public AtlasMapRendererStage
    {
    public:
        struct RenderableSymbol;
        typedef QuadTree< std::shared_ptr<const RenderableSymbol>, AreaI::CoordType > IntersectionsQuadTree;

        struct RenderableSymbol
        {
            virtual ~RenderableSymbol();

            std::shared_ptr<const MapSymbol> mapSymbol;
            std::shared_ptr<const MapSymbolsGroup::AdditionalSymbolInstanceParameters> genericInstanceParameters;

            std::shared_ptr<const GPUAPI::ResourceInGPU> gpuResource;
            double distanceToCamera;
            IntersectionsQuadTree::BBox intersectionBBox;
        };

        struct RenderableBillboardSymbol : RenderableSymbol
        {
            virtual ~RenderableBillboardSymbol();

            std::shared_ptr<const MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters> instanceParameters;

            PointI offsetFromTarget31;
            PointF offsetFromTarget;
            glm::vec3 positionInWorld;
        };

        struct RenderableOnSurfaceSymbol : RenderableSymbol
        {
            virtual ~RenderableOnSurfaceSymbol();

            std::shared_ptr<const MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters> instanceParameters;

            PointI offsetFromTarget31;
            PointF offsetFromTarget;
            glm::vec3 positionInWorld;

            float direction;
        };

        struct RenderableOnPathSymbol : RenderableSymbol
        {
            virtual ~RenderableOnPathSymbol();

            std::shared_ptr<const MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters> instanceParameters;

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
    private:
        void obtainRenderableSymbols(
            QList< std::shared_ptr<const RenderableSymbol> >& outRenderableSymbols,
            IntersectionsQuadTree& outIntersections) const;

        mutable QReadWriteLock _lastPreparedIntersectionsLock;
        IntersectionsQuadTree _lastPreparedIntersections;

        void obtainRenderablesFromSymbol(
            const std::shared_ptr<const MapSymbol>& mapSymbol,
            const std::shared_ptr<const MapSymbolsGroup::AdditionalSymbolInstanceParameters>& instanceParameters,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols) const;

        bool plotSymbol(
            const std::shared_ptr<RenderableSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;

        // Billboard symbols:
        void obtainRenderablesFromBillboardSymbol(
            const std::shared_ptr<const IBillboardMapSymbol>& billboardMapSymbol,
            const std::shared_ptr<const MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters>& instanceParameters,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols) const;
        bool plotBillboardSymbol(
            const std::shared_ptr<RenderableBillboardSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;
        bool plotBillboardRasterSymbol(
            const std::shared_ptr<RenderableBillboardSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;
        bool plotBillboardVectorSymbol(
            const std::shared_ptr<RenderableBillboardSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;

        // On-surface symbols:
        void obtainRenderablesFromOnSurfaceSymbol(
            const std::shared_ptr<const IOnSurfaceMapSymbol>& onSurfaceMapSymbol,
            const std::shared_ptr<const MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters>& instanceParameters,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols) const;
        bool plotOnSurfaceSymbol(
            const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;
        bool plotOnSurfaceRasterSymbol(
            const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;
        bool plotOnSurfaceVectorSymbol(
            const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;

        // On-path symbols:
        void obtainRenderablesFromOnPathSymbol(
            const std::shared_ptr<const OnPathRasterMapSymbol>& onPathMapSymbol,
            const std::shared_ptr<const MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters>& instanceParameters,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols) const;
        bool plotOnPathSymbol(
            const std::shared_ptr<RenderableOnPathSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;

        // Intersection-related:
        bool applyIntersectionWithOtherSymbolsFiltering(
            const std::shared_ptr<const RenderableSymbol>& renderable,
            const IntersectionsQuadTree& intersections) const;
        bool applyMinDistanceToSameContentFromOtherSymbolFiltering(
            const std::shared_ptr<const RenderableSymbol>& renderable,
            const IntersectionsQuadTree& intersections) const;
        bool addToIntersections(
            const std::shared_ptr<const RenderableSymbol>& renderable,
            IntersectionsQuadTree& intersections) const;

        // Utilities:
        QVector<glm::vec2> convertPoints31ToWorld(
            const QVector<PointI>& points31) const;
        QVector<glm::vec2> convertPoints31ToWorld(
            const QVector<PointI>& points31,
            const unsigned int startIndex,
            const unsigned int endIndex) const;

        QVector<glm::vec2> projectFromWorldToScreen(
            const QVector<glm::vec2>& pointsInWorld) const;
        QVector<glm::vec2> projectFromWorldToScreen(
            const QVector<glm::vec2>& pointsInWorld,
            const unsigned int startIndex,
            const unsigned int endIndex) const;

        static std::shared_ptr<const GPUAPI::ResourceInGPU> captureGpuResource(
            const MapRenderer::MapSymbolReferenceOrigins& resources,
            const std::shared_ptr<const MapSymbol>& mapSymbol);

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

        double computeDistanceBetweenCameraToPath(
            const QVector<glm::vec2>& pathInWorld,
            const unsigned int startPathPointIndex,
            const glm::vec2& exactStartPointInWorld,
            const unsigned int endPathPointIndex,
            const glm::vec2& exactEndPointInWorld) const;

        QVector<RenderableOnPathSymbol::GlyphPlacement> computePlacementOfGlyphsOnPath(
            const bool is2D,
            const QVector<glm::vec2>& path,
            const QVector<float>& pathSegmentsLengths,
            const unsigned int startPathPointIndex,
            const float offsetFromStartPathPoint,
            const unsigned int endPathPointIndex,
            const glm::vec2& directionOnScreen,
            const QVector<float>& glyphsWidths) const;

        OOBBF calculateOnPath2dOOBB(const std::shared_ptr<RenderableOnPathSymbol>& renderable) const;

        OOBBF calculateOnPath3dOOBB(const std::shared_ptr<RenderableOnPathSymbol>& renderable) const;

        // Debug-related:
        void addPathDebugLine(
            const QVector<PointI>& path31,
            const ColorARGB color) const;

        void addIntersectionDebugBox(
            const std::shared_ptr<const RenderableSymbol>& renderable,
            const ColorARGB color,
            const bool drawBorder = true) const;

        void addIntersectionDebugBox(
            const IntersectionsQuadTree::BBox intersectionBBox,
            const ColorARGB color,
            const bool drawBorder = true) const;
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
