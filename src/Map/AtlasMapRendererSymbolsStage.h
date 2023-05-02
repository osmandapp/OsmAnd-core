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
#include "SkPath.h"

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
        typedef QuadTree< std::shared_ptr<const RenderableSymbol>, AreaI::CoordType > ScreenQuadTree;

        struct RenderableSymbol
        {
            virtual ~RenderableSymbol();

            std::shared_ptr<const MapSymbolsGroup> mapSymbolGroup;
            std::shared_ptr<const MapSymbol> mapSymbol;
            std::shared_ptr<const MapSymbolsGroup::AdditionalSymbolInstanceParameters> genericInstanceParameters;

            std::shared_ptr<const GPUAPI::ResourceInGPU> gpuResource;
            ScreenQuadTree::BBox visibleBBox;
            ScreenQuadTree::BBox intersectionBBox;
            float opacityFactor;
        };

        struct RenderableBillboardSymbol : RenderableSymbol
        {
            ~RenderableBillboardSymbol() override;

            std::shared_ptr<const MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters> instanceParameters;

            glm::vec3 positionInWorld;
            float elevationInMeters;
            TileId tileId;
            PointF offsetInTileN;
        };

        struct RenderableOnSurfaceSymbol : RenderableSymbol
        {
            ~RenderableOnSurfaceSymbol() override;

            std::shared_ptr<const MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters> instanceParameters;

            float elevationInMeters;
            TileId tileId;
            PointF offsetInTileN;
            PointI offsetFromTarget31;
            PointF offsetFromTarget;
            glm::vec3 positionInWorld;
            double distanceToCamera;

            float direction;
        };

        struct RenderableOnPathSymbol : RenderableSymbol
        {
            ~RenderableOnPathSymbol() override;

            std::shared_ptr<const MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters> instanceParameters;

            bool is2D;
            glm::vec2 directionInWorld;
            glm::vec2 directionOnScreen;
            double distanceToCamera;

            struct GlyphPlacement
            {
                inline GlyphPlacement()
                    : width(qSNaN())
                    , angleY(qSNaN())
                {
                }

                inline GlyphPlacement(
                    const glm::vec2& anchorPoint_,
                    const float width_,
                    const float angle_,
                    const float depth_,
                    const glm::vec2& vNormal_)
                    : anchorPoint(anchorPoint_)
                    , width(width_)
                    , angleY(angle_)
                    , depth(depth_)
                    , vNormal(vNormal_)
                {
                }

                glm::vec2 anchorPoint;
                float elevation;
                float width;
                float angleX;
                float angleY;
                float angleZ;
                float depth;
                glm::vec2 vNormal;
            };
            QVector< GlyphPlacement > glyphsPlacement;
            QVector<glm::vec3> rotatedElevatedBBoxInWorld;
        };
    private:
        bool obtainRenderableSymbols(
            QList< std::shared_ptr<const RenderableSymbol> >& outRenderableSymbols,
            ScreenQuadTree& outIntersections,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric,
            bool forceUpdate = false) const;
        bool obtainRenderableSymbols(
            const MapRenderer::PublishedMapSymbolsByOrder& mapSymbolsByOrder,
            QList< std::shared_ptr<const RenderableSymbol> >& outRenderableSymbols,
            ScreenQuadTree& outIntersections,
            MapRenderer::PublishedMapSymbolsByOrder* pOutAcceptedMapSymbolsByOrder,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric) const;
        mutable MapRenderer::PublishedMapSymbolsByOrder _lastAcceptedMapSymbolsByOrder;
        std::chrono::high_resolution_clock::time_point _lastResumeSymbolsUpdateTime;

        mutable QReadWriteLock _lastPreparedIntersectionsLock;
        ScreenQuadTree _lastPreparedIntersections;

        mutable QReadWriteLock _lastVisibleSymbolsLock;
        ScreenQuadTree _lastVisibleSymbols;

        // Path calculations cache
        struct ComputedPathData
        {
            QVector<glm::vec2> pathInWorld;
            QVector<float> pathSegmentsLengthsInWorld;
            QVector<glm::vec2> pathOnScreen;
            QVector<float> pathSegmentsLengthsOnScreen;
        };
        typedef QHash< std::shared_ptr< const QVector<PointI> >, ComputedPathData > ComputedPathsDataCache;

        void obtainRenderablesFromSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
            const std::shared_ptr<const MapSymbol>& mapSymbol,
            const std::shared_ptr<const MapSymbolsGroup::AdditionalSymbolInstanceParameters>& instanceParameters,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            ComputedPathsDataCache& computedPathsDataCache,
            QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols,
            bool allowFastCheckByFrustum = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;

        bool plotSymbol(
            const std::shared_ptr<RenderableSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;

        // Billboard symbols:
        void obtainRenderablesFromBillboardSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
            const std::shared_ptr<const IBillboardMapSymbol>& billboardMapSymbol,
            const std::shared_ptr<const MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters>& instanceParameters,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols,
            bool allowFastCheckByFrustum = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;
        bool plotBillboardSymbol(
            const std::shared_ptr<RenderableBillboardSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;
        bool plotBillboardRasterSymbol(
            const std::shared_ptr<RenderableBillboardSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;
        bool plotBillboardVectorSymbol(
            const std::shared_ptr<RenderableBillboardSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;

        // On-surface symbols:
        void obtainRenderablesFromOnSurfaceSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
            const std::shared_ptr<const IOnSurfaceMapSymbol>& onSurfaceMapSymbol,
            const std::shared_ptr<const MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters>& instanceParameters,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols,
            bool allowFastCheckByFrustum = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;
        bool plotOnSurfaceSymbol(
            const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;
        bool plotOnSurfaceRasterSymbol(
            const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;
        bool plotOnSurfaceVectorSymbol(
            const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;

        // On-path symbols:
        void obtainRenderablesFromOnPathSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
            const std::shared_ptr<const OnPathRasterMapSymbol>& onPathMapSymbol,
            const std::shared_ptr<const MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters>& instanceParameters,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            ComputedPathsDataCache& computedPathsDataCache,
            QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols,
            bool allowFastCheckByFrustum = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;
        bool plotOnPathSymbol(
            const std::shared_ptr<RenderableOnPathSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;

        // Terrain-related:
        virtual bool applyTerrainVisibilityFiltering(
            const glm::vec3& positionOnScreen,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric) const = 0;

        // Intersection-related:
        bool applyOnScreenVisibilityFiltering(
            const ScreenQuadTree::BBox& visibleBBox,
            const ScreenQuadTree& intersections, AtlasMapRenderer_Metrics::Metric_renderFrame* metric) const;
        bool applyIntersectionWithOtherSymbolsFiltering(
            const std::shared_ptr<const RenderableSymbol>& renderable,
            const ScreenQuadTree& intersections,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric) const;
        bool applyMinDistanceToSameContentFromOtherSymbolFiltering(
            const std::shared_ptr<const RenderableSymbol>& renderable,
            const ScreenQuadTree& intersections,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric) const;
        bool addToIntersections(
            const std::shared_ptr<const RenderableSymbol>& renderable,
            ScreenQuadTree& intersections,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric) const;

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

        SkPath computePathForGlyphsPlacement(
            const bool is2D,
            const QVector<glm::vec2>& pathOnScreen,
            const QVector<float>& pathSegmentsLengthsOnScreen,
            const QVector<glm::vec2>& pathInWorld,
            const QVector<float>& pathSegmentsLengthsInWorld,
            const unsigned int startPathPointIndex,
            const float offsetFromStartPathPoint,
            const unsigned int endPathPointIndex,
            const glm::vec2& directionOnScreen,
            const QVector<float>& glyphsWidths) const;

        glm::vec2 computePathDirection(const SkPath& path) const;

        double computeDistanceFromCameraToPath(const SkPath& pathInWorld) const;

        SkPath convertPathOnScreenToWorld(const SkPath& pathOnScreen, bool& outOk) const;

        SkPath projectPathInWorldToScreen(const SkPath& pathInWorld) const;

        bool computePlacementOfGlyphsOnPath(
            const SkPath& path,
            const bool is2D,
            const glm::vec2& directionInWorld,
            const glm::vec2& directionOnScreen,
            const QVector<float>& glyphsWidths,
            const float glyphHeight,
            QVector<RenderableOnPathSymbol::GlyphPlacement>& outGlyphsPlacement,
            QVector<glm::vec3>& outRotatedElevatedBBoxInWorld) const;

        bool elevateGlyphAnchorPointIn2D(const glm::vec2& anchorPoint, glm::vec3& outElevatedAnchorPoint) const;

        bool elevateGlyphAnchorPointsIn3D(
            QVector<RenderableOnPathSymbol::GlyphPlacement>& glyphsPlacement,
            QVector<glm::vec3>& outRotatedElevatedBBoxInWorld,
            const float glyphHeight,
            const glm::vec2& directionInWorld) const;

        OOBBF calculateOnPath2dOOBB(const std::shared_ptr<RenderableOnPathSymbol>& renderable) const;

        OOBBF calculateOnPath3dOOBB(const std::shared_ptr<RenderableOnPathSymbol>& renderable) const;

        QVector<PointF> calculateOnPath3DRotatedBBox(
            const QVector<RenderableOnPathSymbol::GlyphPlacement>& glyphsPlacement,
            const float glyphHeight,
            const glm::vec2& directionInWorld) const;

        float getSubsectionOpacityFactor(const std::shared_ptr<const MapSymbol>& mapSymbol) const;

        // Debug-related:
        void addPathDebugLine(
            const QVector<PointI>& path31,
            const ColorARGB color) const;

        void addIntersectionDebugBox(
            const std::shared_ptr<const RenderableSymbol>& renderable,
            const ColorARGB color,
            const bool drawBorder = true) const;

        void addIntersectionDebugBox(
            const ScreenQuadTree::BBox intersectionBBox,
            const ColorARGB color,
            const bool drawBorder = true) const;

        static void convertRenderableSymbolsToMapSymbolInformation(
            const QList< std::shared_ptr<const RenderableSymbol> >& input,
            QList<IMapRenderer::MapSymbolInformation>& output);
    protected:
        QList< std::shared_ptr<const RenderableSymbol> > renderableSymbols;

        void prepare(AtlasMapRenderer_Metrics::Metric_renderFrame* metric);
    public:
        AtlasMapRendererSymbolsStage(AtlasMapRenderer* const renderer);
        virtual ~AtlasMapRendererSymbolsStage();

        void queryLastPreparedSymbolsAt(
            const PointI screenPoint,
            QList<IMapRenderer::MapSymbolInformation>& outMapSymbols) const;
        void queryLastPreparedSymbolsIn(
            const AreaI& screenArea,
            QList<IMapRenderer::MapSymbolInformation>& outMapSymbols,
            const bool strict = false) const;
        void queryLastVisibleSymbolsAt(
            const PointI& screenPoint,
            QList<IMapRenderer::MapSymbolInformation>& outMapSymbols) const;
        void queryLastVisibleSymbolsIn(
            const AreaI& screenArea,
            QList<IMapRenderer::MapSymbolInformation>& outMapSymbols,
            const bool strict = false) const;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_H_)
