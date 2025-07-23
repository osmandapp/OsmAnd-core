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
    class AtlasMapRendererSymbolsStageModel3D;

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
            glm::mat2 mRotate;
            PointI position31;
            float elevationInMeters;
            TileId tileId;
            PointF offsetInTileN;
            PointI offsetOnScreen;
        };

        struct RenderableModel3DSymbol : RenderableSymbol
        {
            ~RenderableModel3DSymbol() override;

            glm::mat4 mModel;
        };

        struct RenderableOnSurfaceSymbol : RenderableSymbol
        {
            ~RenderableOnSurfaceSymbol() override;

            std::shared_ptr<const MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters> instanceParameters;

            float elevationInMeters;
            float elevationFactor;
            TileId tileId;
            PointF offsetInTileN;
            PointI offsetFromTarget31;
            PointF offsetFromTarget;
            glm::vec3 positionInWorld;
            double distanceToCamera;
            PointI target31;

            float direction;
        };

        struct RenderableOnPathSymbol : RenderableSymbol
        {
            ~RenderableOnPathSymbol() override;

            std::shared_ptr<const MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters> instanceParameters;

            bool is2D;
            glm::vec2 directionInWorld;
            glm::vec2 directionOnScreen;
            float pixelSizeInWorld;
            double distanceToCamera;
            PointI target31;
            glm::vec2 pinPointOnScreen;
            glm::vec3 pinPointInWorld;

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
            bool forceUpdate,
            bool needUpdatedSymbols);
        bool obtainRenderableSymbols(
            const MapRenderer::PublishedMapSymbolsByOrder& mapSymbolsByOrder,
            const bool preRenderDenseSymbolsDepth,
            QList< std::shared_ptr<const RenderableSymbol> >& outRenderableSymbols,
            ScreenQuadTree& outIntersections,
            MapRenderer::PublishedMapSymbolsByOrder* pOutAcceptedMapSymbolsByOrder,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric);
        mutable MapRenderer::PublishedMapSymbolsByOrder _lastAcceptedMapSymbolsByOrder;
        std::chrono::high_resolution_clock::time_point _lastResumeSymbolsUpdateTime;
        bool _previouslyInvalidated;

        mutable QReadWriteLock _lastPreparedIntersectionsLock;
        ScreenQuadTree _lastPreparedIntersections;

        mutable QReadWriteLock _lastVisibleSymbolsLock;
        ScreenQuadTree _lastVisibleSymbols;

        mutable QList<PointI> _primaryGridMarksOnXAsis;
        mutable QList<PointI> _primaryGridMarksOnYAsis;
        mutable QList<PointI> _secondaryGridMarksOnXAsis;
        mutable QList<PointI> _secondaryGridMarksOnYAsis;

        // Path calculations cache
        struct ComputedPathData
        {
            QVector<glm::vec2> pathInWorld;
            QVector<glm::vec2> vectorsInWorld;
            QVector<float> pathSegmentsLengthsOnRelief;
            QVector<float> pathSegmentsLengthsInWorld;
            QVector<float> pathDistancesInWorld;
            QVector<float> pathAnglesInWorld;
            QVector<glm::vec2> pathOnScreen;
            QVector<glm::vec2> vectorsOnScreen;
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
            std::shared_ptr<RenderableSymbol>& outRenderableSymbol,
            ScreenQuadTree& intersections,
            bool allowFastCheckByFrustum = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);

        bool plotSymbol(
            const std::shared_ptr<RenderableSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);

        // Billboard symbols:
        void obtainRenderablesFromBillboardSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
            const std::shared_ptr<const IBillboardMapSymbol>& billboardMapSymbol,
            const std::shared_ptr<const MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters>& instanceParameters,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            std::shared_ptr<RenderableSymbol>& outRenderableSymbol,
            ScreenQuadTree& intersections,
            bool allowFastCheckByFrustum = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);
        bool plotBillboardSymbol(
            const std::shared_ptr<RenderableBillboardSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);
        bool plotBillboardRasterSymbol(
            const std::shared_ptr<RenderableBillboardSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);
        bool plotBillboardVectorSymbol(
            const std::shared_ptr<RenderableBillboardSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);

        // On-surface symbols:
        void obtainRenderablesFromOnSurfaceSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
            const std::shared_ptr<const IOnSurfaceMapSymbol>& onSurfaceMapSymbol,
            const std::shared_ptr<const MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters>& instanceParameters,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            std::shared_ptr<RenderableSymbol>& outRenderableSymbol,
            bool allowFastCheckByFrustum = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);
        bool plotOnSurfaceSymbol(
            const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);
        bool plotOnSurfaceRasterSymbol(
            const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);
        bool plotOnSurfaceVectorSymbol(
            const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);

        // On-path symbols:
        void obtainRenderablesFromOnPathSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
            const std::shared_ptr<const OnPathRasterMapSymbol>& onPathMapSymbol,
            const std::shared_ptr<const MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters>& instanceParameters,
            const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
            ComputedPathsDataCache& computedPathsDataCache,
            std::shared_ptr<RenderableSymbol>& outRenderableSymbol,
            ScreenQuadTree& intersections,
            bool allowFastCheckByFrustum = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);
        bool plotOnPathSymbol(
            const std::shared_ptr<RenderableOnPathSymbol>& renderable,
            ScreenQuadTree& intersections,
            bool applyFiltering = true,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric = nullptr);

        // Terrain-related:
        virtual bool preRender(QList< std::shared_ptr<const RenderableSymbol> >& preRenderableSymbols,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric) = 0;
        virtual void clearTerrainVisibilityFiltering() = 0;
        virtual int startTerrainVisibilityFiltering(
            const PointF& pointOnScreen,
            const glm::vec3& firstPointInWorld,
            const glm::vec3& secondPointInWorld,
            const glm::vec3& thirdPointInWorld,
            const glm::vec3& fourthPointInWorld) = 0;
        virtual bool applyTerrainVisibilityFiltering(const int queryIndex,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric) = 0;

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
        const static float _inclineThresholdOnPath2D;
        const static float _tiltThresholdOnPath3D;
        QVector<glm::vec2> convertPoints31ToWorld(
            const QVector<PointI>& points31) const;
        QVector<glm::vec2> convertPoints31ToWorld(
            const QVector<PointI>& points31,
            const unsigned int startIndex,
            const unsigned int endIndex) const;

        bool projectFromWorldToScreen(
            ComputedPathData& computedPathData,
            const QVector<PointI>* points31 = nullptr) const;

        float getWorldPixelSize(const glm::vec3& pinPointInWorld, glm::vec2& pinPointOnScreen) const;

        static std::shared_ptr<const GPUAPI::ResourceInGPU> captureGpuResource(
            const MapRenderer::MapSymbolReferenceOrigins& resources,
            const std::shared_ptr<const MapSymbol>& mapSymbol);

        static bool computePointIndexAndOffsetFromOriginAndOffset(
            const QVector<float>& pathSegmentsLengths,
            const unsigned int originPathPointIndex,
            const float distanceFromOriginPathPoint,
            const float offsetToPoint,
            unsigned int& outPathPointIndex,
            float& outOffsetFromPathPoint);

        static glm::vec2 computeExactPointFromOriginAndOffset(
            const QVector<glm::vec2>& path,
            const QVector<glm::vec2>& vectors,
            const QVector<float>& pathSegmentsLengths,
            const unsigned int originPathPointIndex,
            const float offsetFromOriginPathPoint);

        static bool pathRenderableAs2D(
            const QVector<glm::vec2>& pathOnScreen,
            const QVector<glm::vec2>& vectorsOnScreen,
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
            const glm::vec2& destinationVector,
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
            const float pathPixelSizeInWorld,
            const bool is2D,
            const ComputedPathData& computedPathData,
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
            const QVector<glm::vec2>& vectorsInWorld,
            const QVector<float>& pathSegmentsLengthsInWorld,
            const QVector<float>& pathDistancesInWorld,
            const QVector<float>& pathAnglesInWorld) const;

        QVector<glm::vec2> getPathInWorldToWorld(
            const QVector<unsigned int>& pointIndices,
            const QVector<float>& pointOffsets,
            const QVector<glm::vec2>& pathInWorld,
            const QVector<glm::vec2>& vectorsInWorld,
            const QVector<float>& pathSegmentsLengthsInWorld) const;

        bool computePlacementOfGlyphsOnPath(
            const float pathPixelSizeInWorld,
            const ComputedPathData& computedPathData,
            const bool is2D,
            const glm::vec2& directionInWorld,
            const glm::vec2& directionOnScreen,
            const QVector<float>& glyphsWidths,
            const float glyphHeight,
            bool checkVisibility,
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
            const float pathPixelSizeInWorld,
            const glm::vec2& directionInWorld,
            bool checkVisibility) const;

        OOBBF calculateOnPath2dOOBB(const std::shared_ptr<RenderableOnPathSymbol>& renderable) const;

        OOBBF calculateOnPath3dOOBB(const std::shared_ptr<RenderableOnPathSymbol>& renderable) const;

        QVector<PointF> calculateOnPath3DRotatedBBox(
            const QVector<RenderableOnPathSymbol::GlyphPlacement>& glyphsPlacement,
            const float glyphHeight,
            const float pathPixelSizeInWorld,
            const glm::vec2& directionInWorld) const;

        float getSubsectionOpacityFactor(const std::shared_ptr<const MapSymbol>& mapSymbol) const;

        PointI getApproximate31(
            const double coordinate, const double coord1, const double coord2, const PointI& pos1, const PointI& pos2,
            const bool isPrimary, const bool isAxisY, const double* pRefLon, int32_t& iteration) const;
        
        PointI convertWorldPosTo31(const glm::vec3& worldPos);
        glm::vec3 convert31PosToWorld(const PointI& position31);
        
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
        QList< std::shared_ptr<const RenderableSymbol> > denseSymbols;
        QList< std::shared_ptr<const RenderableSymbol> > renderableSymbols;

        void prepare(AtlasMapRenderer_Metrics::Metric_renderFrame* metric);

        std::shared_ptr<AtlasMapRendererSymbolsStageModel3D> _model3DSubstage;
        virtual void createSubstages() = 0;

        bool isLongPrepareStage = false;
    public:
        AtlasMapRendererSymbolsStage(AtlasMapRenderer* const renderer);
        virtual ~AtlasMapRendererSymbolsStage();

        bool release(bool gpuContextLost) override;

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

        virtual void drawDebugMetricSymbol(IMapRenderer_Metrics::Metric_renderFrame* metric_) = 0;

        friend class AtlasMapRendererSymbolsStageModel3D;

        struct SymbolsLongStageDebugHelper
        {
            QVector<ScreenQuadTree::BBox> debugSymbolsBBoxesRejectedByIntersection;
            QVector<ScreenQuadTree::BBox> debugSymbolsBBoxesRejectedByMinDistanceToSameContentFromOtherSymbol;
            QVector<ScreenQuadTree::BBox> debugSymbolsCheckBBoxesRejectedByMinDistanceToSameContentFromOtherSymbol;
            QVector<ScreenQuadTree::BBox> debugSymbolsBBoxesRejectedByPresentationMode;
            QVector<QVector<PointI>> debugTooShortOnPathSymbolsRenderablesPaths;

            double metersPerPixel;
            AreaI visibleBBoxShifted;
            ZoomLevel mapZoomLevel;
            ZoomLevel surfaceZoomLevel;
            float mapVisualZoom;
            float surfaceVisualZoom;
            float mapVisualZoomShift;
            bool hasElevationDataProvider;
            PointI target31;
        } mutable symbolsLongStageDebugHelper;

        bool isMapStateChanged(const MapState& mapState) const;
        void applyMapState(const MapState& mapState);
        void clearSymbolsLongStageDebugHelper();
        void drawSymbolsLongStageDebugHelperBboxes();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_H_)
