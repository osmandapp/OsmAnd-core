/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_COMMON_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_COMMON_H_

#include <OsmAndCore/stdlib_common.h>

#include <glm/glm.hpp>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <AtlasMapRenderer.h>
#include <OpenGL_Common/GPUAPI_OpenGL_Common.h>

namespace OsmAnd {

    class AtlasMapRenderer_OpenGL_Common : public AtlasMapRenderer
    {
    public:
    private:
    protected:
        AtlasMapRenderer_OpenGL_Common();

        enum {
            DefaultReferenceTileSizeOnScreen = 256,
        };

        const static float _zNear;

        struct InternalState : public AtlasMapRenderer::InternalState
        {
            glm::mat4 mOrthographicProjection;
            glm::mat4 mPerspectiveProjection;
            glm::mat4 mCameraView;
            glm::mat4 mDistance;
            glm::mat4 mElevation;
            glm::mat4 mAzimuth;
            glm::mat4 mPerspectiveProjectionInv;
            glm::mat4 mCameraViewInv;
            glm::mat4 mDistanceInv;
            glm::mat4 mElevationInv;
            glm::mat4 mAzimuthInv;
            glm::vec2 groundCameraPosition;
            glm::vec4 worldCameraPosition;
            float zSkyplane;
            float zFar;
            float projectionPlaneHalfHeight;
            float projectionPlaneHalfWidth;
            float aspectRatio;
            float fovInRadians;
            float nearDistanceFromCameraToTarget;
            float baseDistanceFromCameraToTarget;
            float farDistanceFromCameraToTarget;
            float distanceFromCameraToTarget;
            float groundDistanceFromCameraToTarget;
            float tileScaleFactor;
            float scaleToRetainProjectedSize;
            PointF skyplaneSize;
            float correctedFogDistance;
        };
        InternalState _internalState;
        virtual const AtlasMapRenderer::InternalState* getInternalStateRef() const;
        virtual AtlasMapRenderer::InternalState* getInternalStateRef();

        void computeVisibleTileset(InternalState* internalState, const MapRendererState& state);

#pragma pack(push)
#pragma pack(1)
        struct MapTileVertex 
        {
            GLfloat positionXZ[2];
            GLfloat textureUV[2];
        };
#pragma pack(pop)

        void allocateTilePatch(MapTileVertex* vertices, GLsizei verticesCount, GLushort* indices, GLsizei indicesCount);
        void releaseTilePatch();

        struct {
            GLuint tilePatchVBO;
            GLuint tilePatchIBO;
            GLuint tilePatchVAO;

            // Multiple variations of RasterStage program.
            // Variations are generated according to number of active raster tile providers.
            struct {
                GLuint program;

                struct {
                    GLuint id;

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
                    GLuint id;

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
            } variations[RasterMapLayersCount];
        } _rasterMapStage;
        void initializeRasterMapStage();
        void renderRasterMapStage();
        void releaseRasterMapStage();

        struct {
            GLuint skyplaneVAO;
            GLuint skyplaneVBO;
            GLuint skyplaneIBO;

            GLuint program;

            struct {
                GLuint id;

                // Input data
                struct {
                    GLint vertexPosition;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLint mProjectionViewModel;
                    GLint planeSize;
                } param;
            } vs;

            struct {
                GLuint id;

                // Parameters
                struct {
                    // Common data
                    GLint skyColor;
                } param;
            } fs;
        } _skyStage;
        void initializeSkyStage();
        void renderSkyStage();
        void releaseSkyStage();

        struct {
            GLuint symbolVAO;
            GLuint symbolVBO;
            GLuint symbolIBO;

            GLuint program;

            struct {
                GLuint id;

                // Input data
                struct {
                    GLint vertexPosition;
                    GLint vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLint mPerspectiveProjectionView;
                    GLint mOrthographicProjection;
                    GLint viewport;

                    // Per-symbol data
                    GLint symbolOffsetFromTarget;
                    GLint symbolSize;
                    GLint distanceFromCamera;
                    GLint onScreenOffset;
                } param;
            } vs;

            struct {
                GLuint id;

                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLint sampler;
                } param;
            } fs;
        } _symbolsStage;
        void initializeSymbolsStage();
        void renderSymbolsStage();
        void releaseSymbolsStage();

#if OSMAND_DEBUG
        struct {
            GLuint vao;
            GLuint vbo;
            GLuint ibo;

            GLuint program;

            struct {
                GLuint id;

                // Input data
                struct {
                    GLint vertexPosition;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLint mProjectionViewModel;
                    GLint rect;
                } param;
            } vs;

            struct {
                GLuint id;

                // Parameters
                struct {
                    // Common data
                    GLint color;
                } param;
            } fs;
        } _debugStage_Rects2D;
        void initializeDebugStage();
        void renderDebugStage();
        void releaseDebugStage();

        typedef std::pair<AreaI, uint32_t> DebugRect2D;
        QList<DebugRect2D> _debugRects2D;
        void clearDebugPrimitives();
        void addDebugRect2D(const AreaI& rect, uint32_t argbColor);
#endif

        virtual bool doInitializeRendering();
        virtual bool doRenderFrame();
        virtual bool doReleaseRendering();

        GLsizei _tilePatchIndicesCount;
        virtual void createTilePatch();
        
        virtual void onValidateResourcesOfType(const MapRendererResources::ResourceType type);

        virtual bool updateInternalState(MapRenderer::InternalState* internalState, const MapRendererState& state);

        virtual bool postInitializeRendering();
        virtual bool preReleaseRendering();

        GPUAPI_OpenGL_Common* getGPUAPI() const;

        float getReferenceTileSizeOnScreen(const MapRendererState& state);
    public:
        virtual ~AtlasMapRenderer_OpenGL_Common();

        virtual float getReferenceTileSizeOnScreen();
        virtual float getScaledTileSizeOnScreen();
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI& location31);
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI64& location);
    };

}

#endif // _OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_COMMON_H_
