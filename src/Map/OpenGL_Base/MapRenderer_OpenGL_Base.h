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
#ifndef __MAP_RENDERER_OPENGL_BASE_H_
#define __MAP_RENDERER_OPENGL_BASE_H_

#include <stdint.h>
#include <memory>

#include <QMap>
#include <QMultiMap>

#if defined(WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif
#include <GL/gl.h>

#include <glm/glm.hpp>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <IMapRenderer.h>

#if !defined(NDEBUG)
#   define GL_CHECK_RESULT validateResult()
#   define GL_GET_RESULT validateResult()
#else
#   define GL_CHECK_RESULT
#   define GL_GET_RESULT glGetError()
#endif

namespace OsmAnd {

    class OSMAND_CORE_API MapRenderer_BaseOpenGL : public virtual IMapRenderer
    {
    public:
    private:
    protected:
        uint32_t _maxTextureSize;
        
        virtual GLenum validateResult() = 0;
        virtual GLuint compileShader(GLenum shaderType, const char* source) = 0;
        virtual GLuint linkProgram(GLuint shadersCount, GLuint *shaders) = 0;

        MapRenderer_BaseOpenGL();
    public:
        virtual ~MapRenderer_BaseOpenGL();
    };

}

#endif // __MAP_RENDERER_OPENGL_BASE_H_