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
        GLuint _tilePatchVBO;
        GLuint _tilePatchIBO;
        GLuint _tilePatchVAO;
        void createTilePatch();
        void releaseTilePatch();

        // Multiple variations of RasterStage program.
        // Variations are generated according to number of active raster tile providers.
        struct {
            GLuint id;

            struct {
                // Input data
                struct {
                    GLint vertexPosition;
                    GLint vertexTexCoords;
                    GLint vertexElevation;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLint mProjectionView;
                    GLint mapScale;
                    GLint targetInTilePosN;
                    GLint distanceFromCameraToTarget;
                    GLint cameraElevationAngleN;
                    GLint groundCameraPosition;
                    GLint scaleToRetainProjectedSize;

                    // Per-tile data
                    GLint tileCoordsOffset;
                    GLint elevationData_k;
                    GLint elevationData_sampler;
                    GLint elevationData_upperMetersPerUnit;
                    GLint elevationData_lowerMetersPerUnit;

                    // Per-tile-per-layer data
                    struct
                    {
                        GLint tileSizeN;
                        GLint tilePaddingN;
                        GLint slotsPerSide;
                        GLint slotIndex;
                    } elevationTileLayer, rasterTileLayers[RasterMapLayersCount];
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Per-tile-per-layer data
                    struct
                    {
                        GLint k;
                        GLint sampler;
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