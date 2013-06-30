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
        GLuint _vertexShader;
        GLuint _fragmentShader;
        GLuint _programObject;
        
        // Input data
        GLint _vertexShader_in_vertexPosition;
        GLint _vertexShader_in_vertexTexCoords;

        // Parameters: common data
        GLint _vertexShader_param_mProjection;
        GLint _vertexShader_param_mView;
        GLint _vertexShader_param_centerOffset;
        GLint _vertexShader_param_targetTile;
        GLint _fragmentShader_param_distanceFromCameraToTarget;
        GLint _fragmentShader_param_cameraElevationAngle;
        GLint _fragmentShader_param_fogColor;
        GLint _fragmentShader_param_fogDistance;
        GLint _fragmentShader_param_fogDensity;
        
        // Parameters: per-tile data
        GLint _vertexShader_param_tile;
        GLint _vertexShader_param_elevationData_k;
        GLint _vertexShader_param_elevationData_sampler;

        // Parameters: per-tile-per-layer data
        struct
        {
            GLint tileSizeN;
            GLint tilePaddingN;
            GLint slotsPerSide;
            GLint slotIndex;
        } _vertexShader_param_perTileLayer[IMapRenderer::TileLayerId::IdsCount];
        struct
        {
            GLint k;
            GLint sampler;
        } _fragmentShader_param_perTileLayer[IMapRenderer::TileLayerId::IdsCount - IMapRenderer::TileLayerId::RasterMap];

        virtual void allocateTilePatch(Vertex* vertices, size_t verticesCount, GLushort* indices, size_t indicesCount);
        virtual void releaseTilePatch();
    public:
        AtlasMapRenderer_OpenGL();
        virtual ~AtlasMapRenderer_OpenGL();

        virtual void initializeRendering();
        virtual void performRendering();
        virtual void releaseRendering();
    };

}

#endif // __ATLAS_MAP_RENDERER_OPENGL_H_