#ifndef _OSMAND_CORE_GPU_API_OPENGL3_H_
#define _OSMAND_CORE_GPU_API_OPENGL3_H_

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

        std::array< GLuint, SamplerTypesCount > _textureSamplers;
        QHash<GLenum, SamplerType> _textureBlocksSamplers;

        // Groups emulation for gDEBugger
        QStringList _gdebuggerGroupsStack;
        QMutex _gdebuggerGroupsStackMutex;

        virtual void glPushGroupMarkerEXT_wrapper(GLsizei length, const GLchar* marker);
        virtual void glPopGroupMarkerEXT_wrapper();
    protected:
        virtual TextureFormat getTextureSizedFormat(const SkBitmap::Config skBitmapConfig) const;
        virtual TextureFormat getTextureSizedFormat_float() const;

        virtual SourceFormat getSourceFormat_float() const;
    public:
        GPUAPI_OpenGL2plus();
        virtual ~GPUAPI_OpenGL2plus();

        virtual bool initialize();
        virtual bool release();

        const bool& isSupported_GREMEDY_string_marker;
        const bool& isSupported_ARB_sampler_objects;
        const bool& isSupported_samplerObjects;
        const bool& isSupported_ARB_vertex_array_object;
        const bool& isSupported_APPLE_vertex_array_object;
        const bool& isSupported_ARB_texture_storage;
        const bool& isSupported_ARB_texture_float;
        const bool& isSupported_ATI_texture_float;
        const bool& isSupported_ARB_texture_rg;

        virtual GLenum validateResult();

        virtual void allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const TextureFormat format);
        virtual void uploadDataToTexture2D(GLenum target, GLint level,
            GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
            const GLvoid *data, GLsizei dataRowLengthInElements, GLsizei elementSize,
            const SourceFormat sourceFormat);
        virtual void setMipMapLevelsLimit(GLenum target, const uint32_t mipmapLevelsCount);

        virtual void glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays);
        virtual void glBindVertexArray_wrapper(GLuint array);
        virtual void glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays);

        virtual void preprocessVertexShader(QString& code);
        virtual void preprocessFragmentShader(QString& code);
        virtual void optimizeVertexShader(QString& code);
        virtual void optimizeFragmentShader(QString& code);

        virtual void setTextureBlockSampler(const GLenum textureBlock, const SamplerType samplerType);
        virtual void applyTextureBlockToTexture(const GLenum texture, const GLenum textureBlock);

        virtual void pushDebugGroupMarker(const QString& title);
        virtual void popDebugGroupMarker();
    };
}

#endif // !defined(_OSMAND_CORE_GPU_API_OPENGL3_H_)
