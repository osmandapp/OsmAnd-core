/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
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
#ifndef __ATLAS_MAP_RENDERER_OPENGL_COMMON_H_
#define __ATLAS_MAP_RENDERER_OPENGL_COMMON_H_

#include <cstdint>
#include <memory>

#include <glm/glm.hpp>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <AtlasMapRenderer.h>
#include <OpenGL_Common/RenderAPI_OpenGL_Common.h>

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
            glm::mat4 mProjection;
            glm::mat4 mProjectionInv;
            glm::mat4 mView;
            glm::mat4 mDistance;
            glm::mat4 mElevation;
            glm::mat4 mAzimuth;
            glm::mat4 mViewInv;
            glm::mat4 mDistanceInv;
            glm::mat4 mElevationInv;
            glm::mat4 mAzimuthInv;
            glm::vec2 groundCameraPosition;
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
            PointF skyplaneHalfSize;
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
            GLfloat position[2];
            GLfloat uv[2];
        };
#pragma pack(pop)

        void allocateTilePatch(MapTileVertex* vertices, GLsizei verticesCount, GLushort* indices, GLsizei indicesCount);
        void releaseTilePatch();

        struct {
            GLuint tilePatchVBO;
            GLuint tilePatchIBO;

            // Multiple variations of RasterStage program.
            // Variations are generated according to number of active raster tile providers.
            struct {
                GLuint program;

                GLuint tilePatchVAO;

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
                    GLint halfSize;
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
            /*GLuint skyplaneVAO;
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
                    GLint halfSize;
                } param;
            } vs;

            struct {
                GLuint id;

                // Parameters
                struct {
                    // Common data
                    GLint skyColor;
                } param;
            } fs;*/
        } _symbolsStage;
        void initializeSymbolsStage();
        void renderSymbolsStage();
        void releaseSymbolsStage();

        virtual bool doInitializeRendering();
        virtual bool doRenderFrame();
        virtual bool doReleaseRendering();

        virtual void validateConfigurationChange(const ConfigurationChange& change);

        GLsizei _tilePatchIndicesCount;
        virtual void createTilePatch();
        
        virtual void validateResourcesOfType(const ResourceType type);

        virtual bool updateInternalState(MapRenderer::InternalState* internalState, const MapRendererState& state);

        virtual bool postInitializeRendering();
        virtual bool preReleaseRendering();

        RenderAPI_OpenGL_Common* getRenderAPI() const;

        float getReferenceTileSizeOnScreen(const MapRendererState& state);
    public:
        virtual ~AtlasMapRenderer_OpenGL_Common();

        virtual float getReferenceTileSizeOnScreen();
        virtual float getScaledTileSizeOnScreen();
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI& location31);
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI64& location);
    };

}

#endif // __ATLAS_MAP_RENDERER_OPENGL_COMMON_H_
