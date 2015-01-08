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
#include <QHash>
#include <QVector>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
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
        // Raster layers support:
        unsigned int _maxNumberOfRasterMapLayersInBatch;
        GLsizei _rasterTileIndicesCount;
        GLname _rasterTileVBO;
        GLname _rasterTileIBO;
        QHash<unsigned int, GLname> _rasterTileVAOs;
        void initializeRasterTile();
        void releaseRasterTile();
        struct RasterLayerTileProgram
        {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation vertexTexCoords;
                    GLlocation vertexElevation;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mProjectionView;
                    GLlocation mapScale;
                    GLlocation targetInTilePosN;
                    GLlocation distanceFromCameraToTarget;
                    GLlocation cameraElevationAngleN;
                    GLlocation groundCameraPosition;
                    GLlocation scaleToRetainProjectedSize;

                    // Per-tile data
                    GLlocation tileCoordsOffset;
                    GLlocation elevationData_scaleFactor;
                    GLlocation elevationData_sampler;
                    GLlocation elevationData_upperMetersPerUnit;
                    GLlocation elevationData_lowerMetersPerUnit;

                    // Per-tile-per-layer data
                    struct VsPerTilePerLayerParameters
                    {
                        GLlocation tileSizeN;
                        GLlocation tilePaddingN;
                        GLlocation slotsPerSide;
                        GLlocation slotIndex;
                    };
                    VsPerTilePerLayerParameters elevationDataLayer;
                    QVector<VsPerTilePerLayerParameters> rasterTileLayers;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Per-tile-per-layer data
                    struct FsPerTilePerLayerParameters
                    {
                        GLlocation opacity;
                        GLlocation sampler;
                    };
                    QVector<FsPerTilePerLayerParameters> rasterTileLayers;
                } param;
            } fs;
        };
        QMap<unsigned int, RasterLayerTileProgram> _rasterLayerTilePrograms;
        bool initializeRasterLayers();
        bool initializeRasterLayersProgram(
            const unsigned int numberOfLayersInBatch,
            RasterLayerTileProgram& outRasterLayerTileProgram);
        bool canRasterMapLayerBeBatched(
            const QVector<int>& batchedLayerIndices,
            const int layerIndex);
        bool renderRasterLayersBatch(
            const bool allowStubsDrawing,
            const QVector<int>& batchedLayerIndices,
            int& lastUsedProgram);
        bool activateRasterLayersProgram(
            const unsigned int numberOfLayersInBatch,
            const bool elevationProviderPresent,
            const int elevationDataSamplerIndex,
            int& lastUsedProgram);
        std::shared_ptr<const GPUAPI::ResourceInGPU> captureElevationDataResource(
            const TileId normalizedTileId);
        unsigned int captureRasterLayersResources(
            const TileId normalizedTileId,
            const bool allowStubsDrawing,
            const QVector<int>& batchedLayerIndices,
            QVector< std::shared_ptr<const GPUAPI::ResourceInGPU> >& outResourcesInGPU);
        bool releaseRasterLayers();
    public:
        AtlasMapRendererMapLayersStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererMapLayersStage_OpenGL();

        virtual bool initialize();
        virtual bool render(IMapRenderer_Metrics::Metric_renderFrame* const metric);
        virtual bool release();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_MAP_LAYERS_STAGE_OPENGL_H_)
