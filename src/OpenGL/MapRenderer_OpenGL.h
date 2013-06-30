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
#ifndef __MAP_RENDERER_OPENGL_H_
#define __MAP_RENDERER_OPENGL_H_

#include <stdint.h>
#include <memory>
#include <array>

#include <QMap>
#include <QMultiMap>
#include <QSet>

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
#include <OpenGL_Base/MapRenderer_OpenGL_Base.h>

namespace OsmAnd {

    class OSMAND_CORE_API MapRenderer_OpenGL : public virtual MapRenderer_BaseOpenGL
    {
    public:
    private:
    protected:
        enum {
            // This helps a bit to avoid bleeding on far camera distances, but does not eliminate it completely.
            BitmapAtlasTilePadding = 15,
        };

        virtual GLenum validateResult();
        virtual GLuint compileShader(GLenum shaderType, const char* source);
        virtual GLuint linkProgram(GLuint shadersCount, GLuint *shaders);

        enum VariableType
        {
            In,
            Uniform
        };
        QMap< GLuint, QMultiMap< VariableType, GLint > > _programVariables;
        void findVariableLocation(GLuint program, GLint& location, const QString& name, const VariableType& type);

        virtual void uploadTileToTexture(TileLayerId layerId, const TileId& tileId, uint32_t zoom, const std::shared_ptr<IMapTileProvider::Tile>& tile, uint64_t& atlasPoolId, void*& textureRef, int& atlasSlotIndex, size_t& usedMemory);
        virtual void releaseTexture(void* textureRef);

        GLuint _textureSampler_Bitmap_NoAtlas;
        GLuint _textureSampler_Bitmap_Atlas;
        GLuint _textureSampler_ElevationData_NoAtlas;
        GLuint _textureSampler_ElevationData_Atlas;

        MapRenderer_OpenGL();
    public:
        virtual ~MapRenderer_OpenGL();

        virtual void initializeRendering();
        virtual void releaseRendering();
    };

}

#endif // __MAP_RENDERER_OPENGL_H_