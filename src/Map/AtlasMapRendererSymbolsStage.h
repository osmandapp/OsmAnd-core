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

            MapRenderer::MapSymbolReferenceOrigins* referenceOrigins;
            std::shared_ptr<const GPUAPI::ResourceInGPU> gpuResource;
            ScreenQuadTree::BBox visibleBBox;
            ScreenQuadTree::BBox intersectionBBox;
            float opacityFactor;
            int queryIndex;
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
                    , angleXZ(0.0f)
                    , angleY(angle_)
                    , rotationX(1.0f)
                    , rotationZ(0.0f)
                    , depth(depth_)
                    , vNormal(vNormal_)
                {
                }

                glm::vec2 anchorPoint;
                float elevation;
                float width;
                float angleXZ;
                float angleY;
                float rotationX;
                float rotationZ;
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
            bool forceUpdate = false);
        bool obtainRenderableSymbols(
            const MapRenderer::PublishedMapSymbolsByOrder& mapSymbolsByOrder,
            QList< std::shared_ptr<const RenderableSymbol> >& outRenderableSymbols,
            ScreenQuadTree& outIntersections,
            MapRenderer::PublishedMapSymbolsByOrder* pOutAcceptedMapSymbolsByOrder,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric);
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
            QVector<float> pathSegmentsLengthsOnRelief;
            QVector<float> pathSegmentsLengthsInWorld;
            QVector<float> pathDistancesInWorld;
            QVector<float> pathAnglesInWorld;
            QVector<glm::vec2> pathOnScreen;
            QVector<float> pathSegmentsLengthsOnScreen;
            QVector<float> pathDistancesOnScreen;
            QVector<float> pathAnglesOnScreen;
        };
        typedef QHash< std::shared_ptr< const QVector<PointI> >, ComputedPathData > ComputedPathsDataCache;

        void obtainRenderablesFromSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
            const std::shared_ptr<const MapSymbol>& mapSymbol,
            const std::shared_ptr<const MapSymbolsGroup::AdditionalSymbolInstanceParameters>& instanceParameters,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            ComputedPathsDataCache& computedPathsDataCache,
            QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols,
            ScreenQuadTree& intersections,
            bool allowFastCheckByFrustum = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);

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
            ScreenQuadTree& intersections,
            bool allowFastCheckByFrustum = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);
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
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);
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
            ScreenQuadTree& intersections,
            bool allowFastCheckByFrustum = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);
        bool plotOnPathSymbol(
            const std::shared_ptr<RenderableOnPathSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr) const;

        // Terrain-related:
        virtual void clearTerrainVisibilityFiltering() = 0;
        virtual int startTerrainVisibilityFiltering(
            const PointF& pointOnScreen,
            const glm::vec3& firstPointInWorld,
            const glm::vec3& secondPointInWorld,
            const glm::vec3& thirdPointInWorld,
            const glm::vec3& fourthPointInWorld) = 0;
        virtual bool applyTerrainVisibilityFiltering(const int queryIndex,
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
            const QVector<glm::vec2>& pointsInWorld,
            QVector<float>& worldLengthsOnRelief,
            QVector<float>& worldLengths,
            QVector<float>& screenLengths,
            QVector<float>& worldDistances,
            QVector<float>& screenDistances,
            QVector<float>& worldAngles,
            QVector<float>& screenAngles) const;

        static std::shared_ptr<const GPUAPI::ResourceInGPU> captureGpuResource(
            const MapRenderer::MapSymbolReferenceOrigins& resources,
            const std::shared_ptr<const MapSymbol>& mapSymbol);

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

        static glm::vec2 computeCorrespondingPoint(
            const float sourcePointOffset,
            const float sourceDistance,
            const float sourceAngle,
            const glm::vec2& destinationStartPoint,
            const glm::vec2& destinationEndPoint,
            const float destinationLength,
            const float destinationDistance,
            const float destinationAngle);

        static float findOffsetInSegmentForDistance(
            const float distance,
            const QVector<float>& pathSegmentsLengths,
            const unsigned int startPathPointIndex,
            const float offsetFromStartPathPoint,
            const unsigned int endPathPointIndex,
            unsigned int& segmentIndex);

        QVector<unsigned int> computePathForGlyphsPlacement(
            const bool is2D,
            const QVector<glm::vec2>& pathOnScreen,
            const QVector<float>& pathSegmentsLengthsOnScreen,
            const QVector<glm::vec2>& pathInWorld,
            const QVector<float>& pathSegmentsLengthsInWorld,
            const unsigned int startPathPointIndex,
            const float offsetFromStartPathPoint,
            const unsigned int endPathPointIndex,
            const glm::vec2& directionOnScreen,
            const QVector<float>& glyphsWidths,
            QVector<float>& pathOffsets,
            float& symmetricOffset) const;

        glm::vec2 computePathDirection(const QVector<glm::vec2>& path) const;

        double computeDistanceFromCameraToPath(const QVector<glm::vec2>& pathInWorld) const;

        QVector<glm::vec2> convertPathOnScreenToWorld(
            const float screenToWorldFactor,
            const QVector<unsigned int>& pointIndices,
            const QVector<float>& pointOffsets,
            const QVector<float>& pathDistancesOnScreen,
            const QVector<float>& pathAnglesOnScreen,
            const QVector<glm::vec2>& pathInWorld,
            const QVector<float>& pathSegmentsLengthsInWorld,
            const QVector<float>& pathDistancesInWorld,
            const QVector<float>& pathAnglesInWorld) const;

        QVector<glm::vec2> getPathInWorldToWorld(
            const QVector<unsigned int>& pointIndices,
            const QVector<float>& pointOffsets,
            const QVector<glm::vec2>& pathInWorld,
            const QVector<float>& pathSegmentsLengthsInWorld) const;

        bool computePlacementOfGlyphsOnPath(
            const QVector<glm::vec2>& path,
            const QVector<float>& pathSegmentsLengthsOnScreen,
            const QVector<float>& pathDistancesOnScreen,
            const QVector<float>& pathAnglesOnScreen,
            const QVector<glm::vec2>& pathInWorld,
            const QVector<float>& pathSegmentsLengthsOnRelief,
            const QVector<float>& pathSegmentsLengthsInWorld,
            const QVector<float>& pathDistancesInWorld,
            const QVector<float>& pathAnglesInWorld,
            const bool is2D,
            const glm::vec2& directionInWorld,
            const glm::vec2& directionOnScreen,
            const QVector<float>& glyphsWidths,
            const float glyphHeight,
            QVector<RenderableOnPathSymbol::GlyphPlacement>& outGlyphsPlacement,
            QVector<glm::vec3>& outRotatedElevatedBBoxInWorld) const;

        bool elevateGlyphAnchorPointIn2D(
            const glm::vec2& anchorPoint,
            glm::vec3& outElevatedAnchorPoint,
            glm::vec3& outElevatedAnchorInWorld) const;

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
        bool withTerrainFilter();
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
