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
#ifndef __ATLAS_MAP_RENDERER_OPENGL_BASE_H_
#define __ATLAS_MAP_RENDERER_OPENGL_BASE_H_

#include <stdint.h>
#include <memory>

#if defined(WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <glm/glm.hpp>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <BaseAtlasMapRenderer.h>
#include <OpenGL_Base/MapRenderer_OpenGL_Base.h>

namespace OsmAnd {

    class OSMAND_CORE_API AtlasMapRenderer_BaseOpenGL
        : public virtual BaseAtlasMapRenderer
        , public virtual MapRenderer_BaseOpenGL
    {
    public:
    private:
    protected:
        glm::mat4 _mProjection;
        glm::mat4 _mView;
        float _distanceFromCameraToTarget;
        
        void computeProjectionAndViewMatrices();
        void computeVisibleTileset();

#pragma pack(push)
#pragma pack(1)
        struct Vertex 
        {
            GLfloat position[3];
            GLfloat uv[2];
        };
#pragma pack(pop)

        virtual void createTilePatch();
        virtual void allocateTilePatch(Vertex* vertices, size_t verticesCount, GLushort* indices, size_t indicesCount) = 0;
        virtual void releaseTilePatch() = 0;
        
        virtual void updateConfiguration();

        AtlasMapRenderer_BaseOpenGL();
    public:
        virtual ~AtlasMapRenderer_BaseOpenGL();

        virtual void initializeRendering();
        virtual void releaseRendering();
    };

}

#endif // __ATLAS_MAP_RENDERER_OPENGL_BASE_H_