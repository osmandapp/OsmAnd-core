#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP_LAYERS_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP_LAYERS_STAGE_OPENGL_H_

#include "stdlib_common.h"
#include <array>

#include <glm/glm.hpp>

#include "QtExtensions.h"

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
        GLsizei _rasterTileIndicesCount;
        GLname _rasterTileVBO;
        GLname _rasterTileIBO;
        QList<GLname> _rasterTileVAOs;
        void initializeRasterTile();
        void releaseRasterTile();

        //struct RasterLayerTileProgram {
        //    GLname id;

        //    struct {
        //        // Input data
        //        struct {
        //            GLlocation vertexPosition;
        //            GLlocation vertexTexCoords;
        //            GLlocation vertexElevation;
        //        } in;

        //        // Parameters
        //        struct {
        //            // Common data
        //            GLlocation mProjectionView;
        //            GLlocation mapScale;
        //            GLlocation targetInTilePosN;
        //            GLlocation distanceFromCameraToTarget;
        //            GLlocation cameraElevationAngleN;
        //            GLlocation groundCameraPosition;
        //            GLlocation scaleToRetainProjectedSize;

        //            // Per-tile data
        //            GLlocation tileCoordsOffset;
        //            GLlocation elevationData_k;
        //            GLlocation elevationData_sampler;
        //            GLlocation elevationData_upperMetersPerUnit;
        //            GLlocation elevationData_lowerMetersPerUnit;

        //            // Per-tile-per-layer data
        //            struct PerTilePerLayerParameters
        //            {
        //                GLlocation tileSizeN;
        //                GLlocation tilePaddingN;
        //                GLlocation slotsPerSide;
        //                GLlocation slotIndex;
        //            };
        //        } param;
        //    } vs;

        //    struct {
        //        // Parameters
        //        struct {
        //            // Per-tile-per-layer data
        //            struct
        //            {
        //                GLlocation k;
        //                GLlocation sampler;
        //            } rasterTileLayers[RasterMapLayersCount];
        //        } param;
        //    } fs;
        //} _tileProgramVariations[RasterMapLayersCount];
        bool initializeRasterLayers();
        bool releaseRasterLayers();
    public:
        AtlasMapRendererMapLayersStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererMapLayersStage_OpenGL();

        virtual bool initialize();
        virtual bool render(IMapRenderer_Metrics::Metric_renderFrame* const metric);
        virtual bool release();

        virtual void updateRasterTile();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_MAP_LAYERS_STAGE_OPENGL_H_)
