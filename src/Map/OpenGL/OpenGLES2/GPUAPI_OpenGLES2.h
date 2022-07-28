#ifndef _OSMAND_CORE_GPU_API_OPENGLES2_H_
#define _OSMAND_CORE_GPU_API_OPENGLES2_H_

#include "stdlib_common.h"
#include <array>

#include "QtExtensions.h"
#include <QString>
#include <QHash>
#include <QSet>

#include <glm/glm.hpp>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "OpenGL/GPUAPI_OpenGL.h"

namespace OsmAnd
{
    class GPUAPI_OpenGLES2 : public GPUAPI_OpenGL
    {
    public:
#if !defined(OSMAND_TARGET_OS_ios)
        typedef void (GL_APIENTRYP P_glTexStorage2DEXT_PROC)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
        static P_glTexStorage2DEXT_PROC glTexStorage2DEXT;

        static PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES;
        static PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOES;
        static PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOES;

        typedef void (GL_APIENTRYP PFNGLPOPGROUPMARKEREXTPROC)(void);
        typedef void (GL_APIENTRYP PFNGLPUSHGROUPMARKEREXTPROC)(GLsizei length, const GLchar* marker);
        static PFNGLPOPGROUPMARKEREXTPROC glPopGroupMarkerEXT;
        static PFNGLPUSHGROUPMARKEREXTPROC glPushGroupMarkerEXT;
#endif // !OSMAND_TARGET_OS_ios
    private:
        bool _isSupported_EXT_unpack_subimage;
        bool _isSupported_EXT_texture_storage;
        bool _isSupported_APPLE_texture_max_level;
        bool _isSupported_OES_vertex_array_object;
        bool _isSupported_OES_rgb8_rgba8;
        bool _isSupported_ARM_rgba8;
        bool _isSupported_EXT_texture;
        bool _isSupported_OES_texture_float;
        bool _isSupported_OES_texture_half_float;
        bool _isSupported_EXT_texture_rg;
        bool _isSupported_EXT_shader_texture_lod;
        QSet<GLenum> _supportedVertexShaderPrecisionFormats;
        QSet<GLenum> _supportedFragmentShaderPrecisionFormats;

        void preprocessShader(QString& code);

        QHash<GLenum, SamplerType> _textureBlocksSamplers;

        bool isShaderPrecisionFormatSupported(GLenum shaderType, GLenum precisionType);
    protected:
        virtual TextureFormat getTextureFormat(const SkColorType colorType) const Q_DECL_OVERRIDE;
        virtual TextureFormat getTextureFormat_float() const Q_DECL_OVERRIDE;
        virtual bool isValidTextureFormat(const TextureFormat textureFormat) const Q_DECL_OVERRIDE;
        virtual size_t getTextureFormatPixelSize(const TextureFormat textureFormat) const Q_DECL_OVERRIDE;

        virtual SourceFormat getSourceFormat_float() const Q_DECL_OVERRIDE;
        virtual bool isValidSourceFormat(const SourceFormat sourceFormat) const Q_DECL_OVERRIDE;

        virtual void glPushGroupMarkerEXT_wrapper(GLsizei length, const GLchar* marker) Q_DECL_OVERRIDE;
        virtual void glPopGroupMarkerEXT_wrapper() Q_DECL_OVERRIDE;

        virtual void glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays) Q_DECL_OVERRIDE;
        virtual void glBindVertexArray_wrapper(GLuint array) Q_DECL_OVERRIDE;
        virtual void glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays) Q_DECL_OVERRIDE;
    public:
        GPUAPI_OpenGLES2();
        virtual ~GPUAPI_OpenGLES2();

        virtual bool initialize();
        virtual bool release(const bool contextLost);

        const bool& isSupported_EXT_unpack_subimage;
        const bool& isSupported_EXT_texture_storage;
        const bool& isSupported_APPLE_texture_max_level;
        const bool& isSupported_OES_vertex_array_object;
        const bool& isSupported_OES_rgb8_rgba8;
        const bool& isSupported_ARM_rgba8;
        const bool& isSupported_EXT_texture;
        const bool& isSupported_OES_texture_float;
        const bool& isSupported_OES_texture_half_float;
        const bool& isSupported_EXT_texture_rg;
        const bool& isSupported_EXT_shader_texture_lod;

        const QSet<GLenum>& supportedVertexShaderPrecisionFormats;
        const QSet<GLenum>& supportedFragmentShaderPrecisionFormats;

        virtual GLenum validateResult(const char* const function, const char* const file, const int line);

        virtual void allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const TextureFormat format);
        virtual void uploadDataToTexture2D(GLenum target, GLint level,
            GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
            const GLvoid *data, GLsizei dataRowLengthInElements, GLsizei elementSize,
            const SourceFormat sourceFormat);
        virtual void setMipMapLevelsLimit(GLenum target, const uint32_t mipmapLevelsCount);

        virtual void preprocessVertexShader(QString& code);
        virtual void preprocessFragmentShader(QString& code);
        virtual void optimizeVertexShader(QString& code);
        virtual void optimizeFragmentShader(QString& code);

        virtual void setTextureBlockSampler(const GLenum textureBlock, const SamplerType samplerType);
        virtual void applyTextureBlockToTexture(const GLenum texture, const GLenum textureBlock);

        virtual void glClearDepth_wrapper(const float depth);
    };
}

#endif // !defined(_OSMAND_CORE_GPU_API_OPENGLES2_H_)
