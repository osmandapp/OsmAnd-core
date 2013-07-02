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
#ifndef __ATLAS_MAP_RENDERER_OPENGL_H_
#define __ATLAS_MAP_RENDERER_OPENGL_H_

#include <stdint.h>
#include <memory>

#include <QMap>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <OpenGL_Base/AtlasMapRenderer_OpenGL_Base.h>
#include <OpenGL/MapRenderer_OpenGL.h>

namespace OsmAnd {

    class MapDataCache;

    class OSMAND_CORE_API AtlasMapRenderer_OpenGL
        : public AtlasMapRenderer_BaseOpenGL
        , public MapRenderer_OpenGL
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
                    GLint mView;
                    GLint centerOffset;
                    GLint targetTile;

                    // Per-tile data
                    GLint tile;
                    GLint elevationData_k;
                    GLint elevationData_sampler;

                    // Per-tile-per-layer data
                    struct
                    {
                        GLint tileSizeN;
                        GLint tilePaddingN;
                        GLint slotsPerSide;
                        GLint slotIndex;
                    } perTileLayer[IMapRenderer::TileLayerId::IdsCount];
                } param;
            } vs;

            struct {
                GLuint id;

                // Parameters
                struct {
                    // Common data
                    GLint distanceFromCameraToTarget;
                    GLint cameraElevationAngle;
                    GLint fogColor;
                    GLint fogDistance;
                    GLint fogDensity;
                    GLint fogOriginFactor;
                    GLint tileScaleFactor;

                    // Per-tile-per-layer data
                    struct
                    {
                        GLint k;
                        GLint sampler;
                    } perTileLayer[IMapRenderer::TileLayerId::IdsCount - IMapRenderer::TileLayerId::RasterMap];
                } param;
            } fs;
        } _mapStage;

        virtual void allocateTilePatch(MapTileVertex* vertices, size_t verticesCount, GLushort* indices, size_t indicesCount);
        virtual void releaseTilePatch();

        void initializeRendering_MapStage();
        void performRendering_MapStage();
        void releaseRendering_MapStage();

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
                    GLint fogOriginFactor;
                } param;
            } fs;
        } _skyStage;
        
        void initializeRendering_SkyStage();
        void performRendering_SkyStage();
        void releaseRendering_SkyStage();
    public:
        AtlasMapRenderer_OpenGL();
        virtual ~AtlasMapRenderer_OpenGL();

        virtual void initializeRendering();
        virtual void performRendering();
        virtual void releaseRendering();
    };

}

#endif // __ATLAS_MAP_RENDERER_OPENGL_H_