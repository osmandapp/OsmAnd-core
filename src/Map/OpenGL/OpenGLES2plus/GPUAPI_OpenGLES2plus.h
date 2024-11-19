#ifndef _OSMAND_CORE_GPU_API_OPENGLES2_PLUS_H_
#define _OSMAND_CORE_GPU_API_OPENGLES2_PLUS_H_

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
    class GPUAPI_OpenGLES2plus : public GPUAPI_OpenGL
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
        typedef void (GL_APIENTRYP PFNGLLABELOBJECTEXTPROC)(GLenum type, GLuint object, GLsizei length, const GLchar *label);
        static PFNGLPOPGROUPMARKEREXTPROC glPopGroupMarkerEXT;
        static PFNGLPUSHGROUPMARKEREXTPROC glPushGroupMarkerEXT;
        static PFNGLLABELOBJECTEXTPROC glLabelObjectEXT;
#endif // !OSMAND_TARGET_OS_ios
    private:
        QVector<GLuint> _pointVisibilityCheckQueries;
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
        bool _isSupported_EXT_debug_marker;
        bool _isSupported_EXT_debug_label;
        bool _isSupported_APPLE_sync;
        bool _isSupported_samplerObjects;
        bool _isSupported_OES_get_program_binary;
        QSet<GLenum> _supportedVertexShaderPrecisionFormats;
        QSet<GLenum> _supportedFragmentShaderPrecisionFormats;

        void preprocessShader(QString& code);

        std::array<GLuint, SamplerTypesCount> _textureSamplers;
        QHash<GLenum, SamplerType> _textureBlocksSamplers;

        bool isShaderPrecisionFormatSupported(GLenum shaderType, GLenum precisionType);
    protected:
        TextureFormat getTextureFormat(const SkColorType colorType) const override;
        TextureFormat getTextureFormat_float() const override;
        size_t getTextureFormatPixelSize(const TextureFormat textureFormat) const override;

        virtual SourceFormat getSourceFormat_float() const Q_DECL_OVERRIDE;

        void glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays) override;
        void glBindVertexArray_wrapper(GLuint array) override;
        void glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays) override;

        GLsync glFenceSync_wrapper(GLenum condition, GLbitfield flags) override;
        void glDeleteSync_wrapper(GLsync sync) override;
        GLenum glClientWaitSync_wrapper(GLsync sync, GLbitfield flags, GLuint64 timeout) override;
    public:
        GPUAPI_OpenGLES2plus();
        virtual ~GPUAPI_OpenGLES2plus();

        bool initialize() override;
        int checkElementVisibility(int queryIndex, float pointSize) override;
        bool elementIsVisible(int queryIndex) override;
        bool release(bool gpuContextLost) override;

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
        const bool& isSupported_EXT_debug_marker;
        const bool& isSupported_EXT_debug_label;
        const bool& isSupported_APPLE_sync;
        const bool& isSupported_samplerObjects;
        const bool& isSupported_OES_get_program_binary;

        const QSet<GLenum>& supportedVertexShaderPrecisionFormats;
        const QSet<GLenum>& supportedFragmentShaderPrecisionFormats;

        GLenum validateResult(const char* function, const char* file, int line) override;

        void allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, TextureFormat format) override;
        void uploadDataToTexture2D(GLenum target, GLint level,
            GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
            const GLvoid* data, GLsizei dataRowLengthInElements, GLsizei elementSize,
            SourceFormat sourceFormat) override;
        void setMipMapLevelsLimit(GLenum target, uint32_t mipmapLevelsCount) override;

        void preprocessVertexShader(QString& code) override;
        void preprocessFragmentShader(QString& code, const QString& fragmentTypePrefix = QString(), const QString& fragmentTypePrecision = QString()) override;
        void optimizeVertexShader(QString& code) override;
        void optimizeFragmentShader(QString& code) override;

        void setTextureBlockSampler(GLenum textureBlock, SamplerType samplerType) override;
        void applyTextureBlockToTexture(GLenum texture, GLenum textureBlock) override;

        void pushDebugGroupMarker(const QString& title) override;
        void popDebugGroupMarker() override;

        void setObjectLabel(ObjectType type, GLuint name, const QString& label) override;
    };
}

#endif // !defined(_OSMAND_CORE_GPU_API_OPENGLES2_H_)
