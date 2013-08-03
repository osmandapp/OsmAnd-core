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
#ifndef __RENDER_API__OPENGLES2_H_
#define __RENDER_API__OPENGLES2_H_

#include <stdint.h>
#include <memory>
#include <array>

#include <glm/glm.hpp>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OpenGL_Common/RenderAPI_OpenGL_Common.h>

namespace OsmAnd {

    class OSMAND_CORE_API RenderAPI_OpenGLES2 : public RenderAPI_OpenGL_Common
    {
    public:
#if !defined(OSMAND_TARGET_OS_ios)
        typedef void (GL_APIENTRYP P_glTexStorage2DEXT_PROC)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
        static P_glTexStorage2DEXT_PROC glTexStorage2DEXT;
#endif // !OSMAND_TARGET_OS_ios
    private:
        QList<QString> _glesExtensions;

        bool _isSupported_EXT_unpack_subimage;
        bool _isSupported_EXT_texture_storage;
        bool _isSupported_APPLE_texture_max_level;
        bool _isSupported_OES_vertex_array_object;
        bool _isSupported_OES_rgb8_rgba8;
        bool _isSupported_OES_texture_float;
        bool _isSupported_EXT_texture_rg;
        bool _isSupported_EXT_shader_texture_lod;

        void preprocessShader(QString& code);
    protected:
    public:
        RenderAPI_OpenGLES2();
        virtual ~RenderAPI_OpenGLES2();

        virtual bool initialize();
        virtual bool release();

        const QList<QString>& glesExtensions;

        const bool& isSupported_EXT_unpack_subimage;
        const bool& isSupported_EXT_texture_storage;
        const bool& isSupported_APPLE_texture_max_level;
        const bool& isSupported_OES_vertex_array_object;
        const bool& isSupported_OES_rgb8_rgba8;
        const bool& isSupported_OES_texture_float;
        const bool& isSupported_EXT_texture_rg;
        const bool& isSupported_EXT_shader_texture_lod;

        virtual GLenum validateResult();

        virtual uint32_t getTileTextureFormat(const std::shared_ptr< IMapTileProvider::Tile >& tile);
        virtual void allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const std::shared_ptr< IMapTileProvider::Tile >& forTile);
        virtual void uploadDataToTexture2D(GLenum target, GLint level,
            GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
            const GLvoid *data, GLsizei dataRowLengthInElements,
            const std::shared_ptr< IMapTileProvider::Tile >& fromTile);
        virtual void setMipMapLevelsLimit(GLenum target, const uint32_t& mipmapLevelsCount);

        virtual void glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays);
        virtual void glBindVertexArray_wrapper(GLuint array);
        virtual void glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays);

        virtual void preprocessVertexShader(QString& code);
        virtual void preprocessFragmentShader(QString& code);
        virtual void optimizeVertexShader(QString& code);
        virtual void optimizeFragmentShader(QString& code);

        virtual void setSampler(GLenum texture, const SamplerType& samplerType);
    };

}

#endif // __RENDER_API__OPENGLES2_H_
