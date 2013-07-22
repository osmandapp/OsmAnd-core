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
#include <type_traits>

#include <QMap>
#include <QMultiMap>

#if defined(WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
#   include <GL/glew.h>
#endif

#if defined(OSMAND_TARGET_OS_darwin)
#   include <OpenGL/gl.h>
#elif defined(OSMAND_TARGET_OS_ios)
#   include <OpenGLES/ES2/gl.h>
#   include <OpenGLES/ES2/glext.h>
#elif defined(OSMAND_TARGET_OS_android)
#   include <EGL/egl.h>
#   include <GLES2/gl2.h>
#   include <GLES2/gl2ext.h>
#else
#   include <GL/gl.h>
#endif

#include <glm/glm.hpp>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <IMapRenderer.h>

#if defined(_DEBUG) || defined(DEBUG)
#   define GL_CHECK_RESULT validateResult()
#   define GL_GET_RESULT validateResult()
#   define GL_CHECK_PRESENT(x)                                                                     \
        {                                                                                          \
            static bool __checked_presence_of_##x = std::is_function<decltype(x)>::value;          \
            if(!__checked_presence_of_##x)                                                         \
            {                                                                                      \
                assert(x);                                                                         \
                __checked_presence_of_##x = true;                                                  \
            }                                                                                      \
        }
#else
#   define GL_CHECK_RESULT
#   define GL_GET_RESULT glGetError()
#   define GL_CHECK_PRESENT(x)
#endif

#if defined(OSMAND_OPENGLES2_RENDERER_SUPPORTED)
#   ifndef GL_UNPACK_ROW_LENGTH
#       define GL_UNPACK_ROW_LENGTH              0x0CF2
#   endif // !GL_UNPACK_ROW_LENGTH
#   ifndef GL_UNPACK_SKIP_ROWS
#       define GL_UNPACK_SKIP_ROWS               0x0CF3
#   endif // !GL_UNPACK_SKIP_ROWS
#   ifndef GL_UNPACK_SKIP_PIXELS
#       define GL_UNPACK_SKIP_PIXELS             0x0CF4
#   endif // !GL_UNPACK_SKIP_PIXELS
#endif // OSMAND_OPENGLES2_RENDERER_SUPPORTED

namespace OsmAnd {

    class OSMAND_CORE_API MapRenderer_BaseOpenGL : public virtual IMapRenderer
    {
    public:
    private:
    protected:
        enum {
            BaseBitmapAtlasTilePadding = 2,
            MipmapLodLevelsMax = 4,
        };

        uint32_t _maxTextureSize;
        
        virtual GLenum validateResult() = 0;
        virtual GLuint compileShader(GLenum shaderType, const char* source);
        virtual GLuint linkProgram(GLuint shadersCount, GLuint *shaders);

        virtual uint32_t getTextureFormatId(GLenum sourceFormat, GLenum sourcePixelDataType);
        virtual void wrapper_glTexStorage2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, GLenum sourceFormat, GLenum sourcePixelDataType) = 0;
        virtual void wrapperEx_glTexSubImage2D(GLenum target, GLint level,
            GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
            GLenum format, GLenum type,
            const GLvoid *pixels, GLsizei rowLengthInPixels = 0);

        virtual CachedTile* uploadTileToGPU(TileLayerId layerId, const TileId& tileId, uint32_t zoom, const std::shared_ptr<IMapTileProvider::Tile>& tile);
        virtual void releaseTileInGPU(CachedTile* tile);

        enum VariableType
        {
            In,
            Uniform
        };
        QMap< GLuint, QMultiMap< VariableType, GLint > > _programVariables;
        virtual void findVariableLocation(GLuint program, GLint& location, const QString& name, const VariableType& type);

        MapRenderer_BaseOpenGL();
    public:
        virtual ~MapRenderer_BaseOpenGL();
    };

}

#endif // __MAP_RENDERER_OPENGL_BASE_H_
