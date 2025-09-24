#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP_LAYERS_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP_LAYERS_STAGE_OPENGL_H_

#include "stdlib_common.h"
#include "ignore_warnings_on_external_includes.h"
#include <array>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <glm/glm.hpp>
#include "restore_internal_warnings.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMap>
#include <QHash>
#include <QVector>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "Ref.h"
#include "AtlasMapRendererMapLayersStage.h"
#include "AtlasMapRendererStageHelper_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRendererMapLayersStage_OpenGL
        : public AtlasMapRendererMapLayersStage
        , private AtlasMapRendererStageHelper_OpenGL
    {
    private:
    protected:
        enum class BatchedLayerType
        {
            Raster,
            Other
        };

        struct ElevationResource Q_DECL_FINAL
        {
            ElevationResource(
                const std::shared_ptr<const GPUAPI::ResourceInGPU>& resourceInGPU,
                const TileId tileIdN,
                const ZoomLevel zoomLevel_,
                const double tileSize_,
                const PointF texCoordsOffset = PointF(0.0f, 0.0f),
                const PointF texCoordsScale = PointF(1.0f, 1.0f));

            std::shared_ptr<const GPUAPI::ResourceInGPU> resourceInGPU;
            TileId tileIdN;
            ZoomLevel zoomLevel;
            double tileSize;
            PointF texCoordsOffset;
            PointF texCoordsScale;

        private:
            Q_DISABLE_COPY(ElevationResource);
        };

        struct BatchedLayerResource Q_DECL_FINAL
        {
            BatchedLayerResource(
                const std::shared_ptr<const GPUAPI::ResourceInGPU>& resourceInGPU,
                const int zoomShift = 0,
                const PointF texCoordsOffset = PointF(0.0f, 0.0f),
                const PointF texCoordsScale = PointF(1.0f, 1.0f));

            std::shared_ptr<const GPUAPI::ResourceInGPU> resourceInGPU;
            int zoomShift;
            PointF texCoordsOffset;
            PointF texCoordsScale;

            bool canBeBatchedWith(const BatchedLayerResource& that) const;

        private:
            Q_DISABLE_COPY(BatchedLayerResource);
        };
        struct BatchedLayer Q_DECL_FINAL
        {
            BatchedLayer(const BatchedLayerType type, const int layerIndex);

            const BatchedLayerType type;
            const int layerIndex;
            QList< Ref<BatchedLayerResource> > resourcesInGPU;

        private:
            Q_DISABLE_COPY(BatchedLayer);
        };
        struct PerTileBatchedLayers Q_DECL_FINAL
        {
            PerTileBatchedLayers(const TileId tileId, const bool containsOriginLayer);

            const TileId tileId;
            const bool containsOriginLayer;
            bool lastBatch;
            bool hasDynamics;
            QList< Ref<BatchedLayer> > layers;
            std::shared_ptr<QList<Ref<ElevationResource>>> elevationResourcesInGPU;

            bool operator<(const PerTileBatchedLayers& that) const;

        private:
            Q_DISABLE_COPY(PerTileBatchedLayers);
        };
        QList< Ref<PerTileBatchedLayers> > batchLayersByTiles(
            const QVector<TileId>& tiles, const QSet<TileId>& visibleTilesSet, ZoomLevel zoomLevel);

        static const float _particleSize;
        static const float _particleSpeedFactor;
        static const ZoomLevel _particleConstantSpeedMinZoom;

        // My location parameters:
        glm::vec3 myLocation;
        float myLocationRadius;
        float headingDirection;
        float headingRadius;

        // Raster layers support:
        unsigned int _maxNumberOfRasterMapLayersInBatch;
        GLsizei _rasterTileIndicesCount;
        GLname _rasterTileVBO;
        GLname _rasterTileIBO;
        QHash<unsigned int, QHash<int, GLname>> _rasterTileVAOs;
        void initializeRasterTile();
        void releaseRasterTile(bool gpuContextLost);

        enum RenderingFeatures : int
        {
            None = 0,
            Grids,
            Dynamics,

            All
        };

        struct RasterLayerTileProgram
        {
            GLname id;
            QByteArray binaryCache;
            GLenum cacheFormat;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mPerspectiveProjectionView;
                    GLlocation resultScale;
                    GLlocation targetInTilePosN;
                    GLlocation tileSize;
                    GLlocation distanceFromCameraToTarget;
                    GLlocation cameraElevationAngleN;
                    GLlocation groundCameraPosition;
                    GLlocation scaleToRetainProjectedSize;
                    GLlocation elevation_configuration;
                    GLlocation elevation_hillshadeConfiguration;
                    GLlocation elevation_colorMapKeys;
                    GLlocation elevation_colorMapValues;
                    GLlocation primaryGridAxisX;
                    GLlocation secondaryGridAxisX;
                    GLlocation primaryGridAxisY;
                    GLlocation secondaryGridAxisY;

                    // Per-tile data
                    GLlocation tileCoords31;
                    GLlocation primaryGridTileTop;
                    GLlocation primaryGridTileBot;
                    GLlocation secondaryGridTileTop;
                    GLlocation secondaryGridTileBot;
                    GLlocation tileCoordsOffset;
                    GLlocation elevation_scale;
                    GLlocation elevation_dataSampler;

                    // Per-tile-per-layer data
                    struct VsPerTilePerLayerParameters
                    {
                        GLlocation texCoordsOffsetAndScale;
                    };
                    QVector<VsPerTilePerLayerParameters> rasterTileLayers;
                    VsPerTilePerLayerParameters elevationLayer;
                    GLlocation elevationLayerTexelSize;
                    GLlocation elevationLayerDataPlace;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data
                    GLlocation lastBatch;
                    GLlocation blendingEnabled;
                    GLlocation backgroundColor;
                    GLlocation myLocationColor;
                    GLlocation myLocation;
                    GLlocation myDirection;
                    GLlocation gridParameters;
                    GLlocation primaryGridColor;
                    GLlocation secondaryGridColor;
                    GLlocation worldCameraPosition;
                    GLlocation mistConfiguration;
                    GLlocation mistColor;
                    // Per-tile-per-layer data
                    struct FsPerTilePerLayerParameters
                    {
                        GLlocation sampler;
                        GLlocation isPremultipliedAlpha;
                        GLlocation opacityFactor;
                        GLlocation texCoordsOffsetAndScale;
                        GLlocation transitionPhase;
                        GLlocation texelSize;
                    };
                    QVector<FsPerTilePerLayerParameters> rasterTileLayers;
                } param;
            } fs;
        };
        QMap<unsigned int, QVector<RasterLayerTileProgram>> _rasterLayerTilePrograms;
        bool initializeRasterLayers();
        bool initializeRasterLayersProgram(
            const unsigned int numberOfLayersInBatch,
            const RenderingFeatures programFeatures,
            QVector<RasterLayerTileProgram>& outRasterLayerTilePrograms);
        bool renderRasterLayersBatch(
            const Ref<PerTileBatchedLayers>& batch,
            AlphaChannelType& currentAlphaChannelType,
            GLname& lastUsedProgram,
            bool& haveElevation,
            const bool withElevation,
            const bool blendingEnabled,
            const ZoomLevel zoomLevel);
        void configureElevationData(
            const std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU>& elevationDataResource,
            const RasterLayerTileProgram& program,
            const TileId tileIdN,
            const ZoomLevel zoomLevel,
            const PointF& texCoordsOffsetN,
            const PointF& texCoordsScaleN,
            const double tileSize,
            const int elevationDataSamplerIndex);
        void cancelElevation(
            const RasterLayerTileProgram& program,
            const int elevationDataSamplerIndex);
        PointD getGridFractions(const double tile, const double nextTile);
        PointD getFloatShift(const double first1, const double second1, const double first2, const double second2);
        bool activateRasterLayersProgram(
            const unsigned int numberOfLayersInBatch,
            const bool batchWithDynamics,
            const int elevationDataSamplerIndex,
            GLname& lastUsedProgram,
            const ZoomLevel zoomLevel);
        std::shared_ptr<const GPUAPI::ResourceInGPU> captureLayerResource(
            const std::shared_ptr<const IMapRendererResourcesCollection>& resourcesCollection,
            const TileId normalizedTileId,
            const ZoomLevel zoomLevel,
            MapRendererResourceState* const outState = nullptr);
        bool releaseRasterLayers(bool gpuContextLost);
    public:
        AtlasMapRendererMapLayersStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererMapLayersStage_OpenGL();

        virtual bool initialize();
        virtual bool render(IMapRenderer_Metrics::Metric_renderFrame* const metric);
        virtual bool release(bool gpuContextLost);
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_MAP_LAYERS_STAGE_OPENGL_H_)
