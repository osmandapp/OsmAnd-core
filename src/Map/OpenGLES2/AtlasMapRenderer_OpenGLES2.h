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
#ifndef __ATLAS_MAP_RENDERER_OPENGLES2_H_
#define __ATLAS_MAP_RENDERER_OPENGLES2_H_

#include <stdint.h>
#include <memory>

#include <QMap>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OpenGL_Common/AtlasMapRenderer_OpenGL_Common.h>
#include <OpenGLES2/RenderAPI_OpenGLES2.h>

namespace OsmAnd {

    class OSMAND_CORE_API AtlasMapRenderer_OpenGLES2 : public AtlasMapRenderer_OpenGL_Common
    {
    protected:
        GLuint _tilePatchVAO;
        GLuint _tilePatchVBO;
        GLuint _tilePatchIBO;
        
        struct {
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
                    GLint mProjectionView;
                    GLint targetInTilePosN;
                    GLint targetTile;
                    GLint mView;
                    GLint cameraElevationAngle;
                    GLint mipmapK;

                    // Per-tile data
                    GLint tile;
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
                    } perTileLayer[MapTileLayerIdsCount];
                } param;
            } vs;

            struct {
                GLuint id;

                // Parameters
                struct {
                    // Common data
                    GLint fogColor;
                    GLint fogDistance;
                    GLint fogDensity;
                    GLint fogOriginFactor;
                    GLint scaleToRetainProjectedSize;

                    // Per-tile-per-layer data
                    struct
                    {
                        GLint k;
                        GLint sampler;
                    } perTileLayer[MapTileLayerIdsCount - MapTileLayerId::RasterMap];
                } param;
            } fs;
        } _mapStage;

        virtual void allocateTilePatch(MapTileVertex* vertices, GLsizei verticesCount, GLushort* indices, GLsizei indicesCount);
        virtual void releaseTilePatch();

        void initializeMapStage();
        void renderMapStage();
        void releaseMapStage();

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
                    GLint halfSize;
                } param;
            } vs;

            struct {
                GLuint id;

                // Parameters
                struct {
                    // Common data
                    GLint skyColor;
                    GLint fogColor;
                    GLint fogDensity;
                    GLint fogHeightOriginFactor;
                } param;
            } fs;
        } _skyStage;
        
        void initializeSkyStage();
        void renderSkyStage();
        void releaseSkyStage();

        virtual bool doInitializeRendering();
        virtual bool doRenderFrame();
        virtual bool doReleaseRendering();

        virtual RenderAPI* allocateRenderAPI();
    public:
        AtlasMapRenderer_OpenGLES2();
        virtual ~AtlasMapRenderer_OpenGLES2();
    };

}

#endif // __ATLAS_MAP_RENDERER_OPENGLES_H_
