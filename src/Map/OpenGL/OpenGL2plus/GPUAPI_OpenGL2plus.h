#ifndef _OSMAND_CORE_GPU_API_OPENGL2_PLUS_H_
#define _OSMAND_CORE_GPU_API_OPENGL2_PLUS_H_

#include "stdlib_common.h"
#include <array>

#include "QtExtensions.h"
#include <QString>
#include <QStringList>
#include <QMutex>

#include <glm/glm.hpp>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "OpenGL/GPUAPI_OpenGL.h"

namespace OsmAnd
{
    class GPUAPI_OpenGL2plus : public GPUAPI_OpenGL
    {
        Q_DISABLE_COPY_AND_MOVE(GPUAPI_OpenGL2plus);
    private:
        void preprocessShader(QString& code);

        bool _isSupported_GREMEDY_string_marker;
        bool _isSupported_ARB_sampler_objects;
        bool _isSupported_samplerObjects;
        bool _isSupported_ARB_vertex_array_object;
        bool _isSupported_APPLE_vertex_array_object;
        bool _isSupported_ARB_texture_storage;
        bool _isSupported_ARB_texture_float;
        bool _isSupported_ATI_texture_float;
        bool _isSupported_ARB_texture_rg;
        bool _isSupported_EXT_gpu_shader4;
        bool _isSupported_EXT_debug_marker;
        bool _isSupported_EXT_debug_label;
        bool _isSupported_ARB_sync;

        GLenum _framebufferDepthDataFormat;
        GLenum _framebufferDepthDataType;

        std::array< GLuint, SamplerTypesCount > _textureSamplers;
        QHash<GLenum, SamplerType> _textureBlocksSamplers;

        // Groups emulation for gDEBugger
        QStringList _gdebuggerGroupsStack;
        QMutex _gdebuggerGroupsStackMutex;
    protected:
        TextureFormat getTextureFormat(const SkColorType colorType) const override;
        TextureFormat getTextureFormat_float() const override;
        size_t getTextureFormatPixelSize(const TextureFormat textureFormat) const override;
        GLenum getBaseInternalTextureFormat(const TextureFormat textureFormat) const override;

        SourceFormat getSourceFormat_float() const override;

        void glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays) override;
        void glBindVertexArray_wrapper(GLuint array) override;
        void glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays) override;

        GLsync glFenceSync_wrapper(GLenum condition, GLbitfield flags) override;
        void glDeleteSync_wrapper(GLsync sync) override;
        GLenum glClientWaitSync_wrapper(GLsync sync, GLbitfield flags, GLuint64 timeout) override;
    public:
        GPUAPI_OpenGL2plus();
        virtual ~GPUAPI_OpenGL2plus();

        bool initialize() override;
        bool attachToRenderTarget() override;
        bool detachFromRenderTarget(bool gpuContextLost) override;
        bool release(bool gpuContextLost) override;

        const bool& isSupported_GREMEDY_string_marker;
        const bool& isSupported_ARB_sampler_objects;
        const bool& isSupported_samplerObjects;
        const bool& isSupported_ARB_vertex_array_object;
        const bool& isSupported_APPLE_vertex_array_object;
        const bool& isSupported_ARB_texture_storage;
        const bool& isSupported_ARB_texture_float;
        const bool& isSupported_ATI_texture_float;
        const bool& isSupported_ARB_texture_rg;
        const bool& isSupported_EXT_gpu_shader4;
        const bool& isSupported_EXT_debug_marker;
        const bool& isSupported_EXT_debug_label;
        const bool& isSupported_ARB_sync;

        const GLenum& framebufferDepthDataFormat;
        const GLenum& framebufferDepthDataType;

        GLenum validateResult(const char* function, const char* file, int line) override;

        void allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const TextureFormat format) override;
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

        void glClearDepth_wrapper(float depth) override;

        void readFramebufferDepth(GLint x, GLint y, GLsizei width, GLsizei height, std::vector<std::byte>& outData) override;
        bool pickFramebufferDepthValue(const std::vector<std::byte>& data, GLint x, GLint y, GLsizei width, GLsizei height, GLfloat& outValue) override;

        void enableOffscreenRendering(const GLsizei bufferWidth, const GLsizei bufferHeight);
        void disableOffscreenRendering();
    };
}

#endif // !defined(_OSMAND_CORE_GPU_API_OPENGL2_PLUS_H_)
