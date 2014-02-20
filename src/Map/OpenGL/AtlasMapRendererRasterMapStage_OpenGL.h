#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_RASTER_MAP_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_RASTER_MAP_STAGE_OPENGL_H_

#include "stdlib_common.h"

#include <glm/glm.hpp>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererStage_OpenGL.h"
#include "GPUAPI_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRendererRasterMapStage_OpenGL : public AtlasMapRendererStage_OpenGL
    {
    private:
    protected:
        GLsizei _tilePatchIndicesCount;
        GLname _tilePatchVBO;
        GLname _tilePatchIBO;
        GLname _tilePatchVAO;
        void createTilePatch();
        void releaseTilePatch();

        // Multiple variations of RasterStage program.
        // Variations are generated according to number of active raster tile providers.
        struct TileProgram {
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
                    GLlocation elevationData_k;
                    GLlocation elevationData_sampler;
                    GLlocation elevationData_upperMetersPerUnit;
                    GLlocation elevationData_lowerMetersPerUnit;

                    // Per-tile-per-layer data
                    struct
                    {
                        GLlocation tileSizeN;
                        GLlocation tilePaddingN;
                        GLlocation slotsPerSide;
                        GLlocation slotIndex;
                    } elevationTileLayer, rasterTileLayers[RasterMapLayersCount];
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Per-tile-per-layer data
                    struct
                    {
                        GLlocation k;
                        GLlocation sampler;
                    } rasterTileLayers[RasterMapLayersCount];
                } param;
            } fs;
        } _tileProgramVariations[RasterMapLayersCount];
    public:
        AtlasMapRendererRasterMapStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererRasterMapStage_OpenGL();

        virtual void initialize();
        virtual void render();
        virtual void release();
        void recreateTile();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_RASTER_MAP_STAGE_OPENGL_H_)