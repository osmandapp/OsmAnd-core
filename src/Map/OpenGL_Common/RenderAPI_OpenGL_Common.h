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
#ifndef __RENDER_API__OPENGL_BASE_H_
#define __RENDER_API__OPENGL_BASE_H_

#include <cstdint>
#include <memory>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>
#include <QMap>
#include <QMultiMap>
#include <QList>
#include <QStringList>
#include <QString>
#include <QVector>

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
#include <CommonTypes.h>
#include <RenderAPI.h>

#if defined(_DEBUG) || defined(DEBUG)
#   define GL_CHECK_RESULT \
        static_cast<RenderAPI_OpenGL_Common*>(this->renderAPI.get())->validateResult()
#   define GL_GET_RESULT \
        static_cast<RenderAPI_OpenGL_Common*>(this->renderAPI.get())->validateResult()
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

namespace OsmAnd {

    enum class GLShaderVariableType
    {
        In,
        Uniform
    };

    class RenderAPI_OpenGL_Common : public RenderAPI
    {
    private:
        bool uploadTileAsTextureToGPU(const std::shared_ptr< const MapTile >& tile, const uint32_t tilesPerAtlasTextureLimit, std::shared_ptr< ResourceInGPU >& resourceInGPU);
        bool uploadTileAsArrayBufferToGPU(const std::shared_ptr< const MapTile >& tile, std::shared_ptr< ResourceInGPU >& resourceInGPU);
    protected:
        GLint _maxTextureSize;
        QMap< GLuint, QMultiMap< GLShaderVariableType, GLint > > _programVariables;
        QStringList _extensions;
        QVector<GLint> _compressedFormats;

        bool _isSupported_vertexShaderTextureLookup;
        bool _isSupported_textureLod;
        
        virtual bool releaseResourceInGPU(const ResourceInGPU::Type& type, const RefInGPU& refInGPU);
    public:
        RenderAPI_OpenGL_Common();
        virtual ~RenderAPI_OpenGL_Common();

        enum {
            BaseBitmapAtlasTilePadding = 2,
            MipmapLodLevelsMax = 4,
        };

        const QStringList& extensions;
        const QVector<GLint>& compressedFormats;

        const GLint& maxTextureSize;
        const bool& isSupported_vertexShaderTextureLookup;
        const bool& isSupported_textureLod;
        
        virtual GLenum validateResult() = 0;
        virtual GLuint compileShader(GLenum shaderType, const char* source);
        virtual GLuint linkProgram(GLuint shadersCount, GLuint *shaders);

        virtual uint32_t getTileTextureFormat(const std::shared_ptr< const MapTile >& tile) = 0;
        virtual void allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const std::shared_ptr< const MapTile >& forTile) = 0;
        virtual void uploadDataToTexture2D(GLenum target, GLint level,
            GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
            const GLvoid *data, GLsizei dataRowLengthInElements,
            const std::shared_ptr< const MapTile >& fromTile) = 0;
        virtual void setMipMapLevelsLimit(GLenum target, const uint32_t mipmapLevelsCount) = 0;

        virtual void glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays) = 0;
        virtual void glBindVertexArray_wrapper(GLuint array) = 0;
        virtual void glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays) = 0;

        virtual void preprocessVertexShader(QString& code) = 0;
        virtual void preprocessFragmentShader(QString& code) = 0;
        virtual void optimizeVertexShader(QString& code) = 0;
        virtual void optimizeFragmentShader(QString& code) = 0;

        enum SamplerType : int32_t
        {
            Invalid = -1,

            ElevationDataTile,
            BitmapTile_Bilinear,
            BitmapTile_BilinearMipmap,
            BitmapTile_TrilinearMipmap,

            __LAST
        };
        enum {
            SamplerTypesCount = SamplerType::__LAST,
        };
        virtual void setSampler(GLenum texture, const SamplerType samplerType) = 0;

        virtual void clearVariablesLookup();
        virtual void findVariableLocation(GLuint program, GLint& location, const QString& name, const GLShaderVariableType& type);

        virtual bool uploadTileToGPU(const std::shared_ptr< const MapTile >& tile, const uint32_t tilesPerAtlasTextureLimit, std::shared_ptr< ResourceInGPU >& resourceInGPU);
    };

}

#endif // __RENDER_API__OPENGL_BASE_H_
