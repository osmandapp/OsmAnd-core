#ifndef _OSMAND_CORE_GPU_API__OPENGL_COMMON_H_
#define _OSMAND_CORE_GPU_API__OPENGL_COMMON_H_

#include "stdlib_common.h"
#include <type_traits>

#include "QtExtensions.h"
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

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "GPUAPI.h"

#if OSMAND_DEBUG
#   define GL_CHECK_RESULT \
        static_cast<GPUAPI_OpenGL_Common*>(this->gpuAPI.get())->validateResult()
#   define GL_GET_RESULT \
        static_cast<GPUAPI_OpenGL_Common*>(this->gpuAPI.get())->validateResult()
#   define GL_CHECK_PRESENT(x)                                                                     \
        {                                                                                          \
            static bool __checked_presence_of_##x = std::is_function<decltype(x)>::value;          \
            if(!__checked_presence_of_##x)                                                         \
            {                                                                                      \
                assert(x != nullptr);                                                              \
                __checked_presence_of_##x = true;                                                  \
            }                                                                                      \
        }
#   define GL_PUSH_GROUP_MARKER(title) \
        static_cast<GPUAPI_OpenGL_Common*>(this->gpuAPI.get())->pushDebugGroupMarker((title))
#   define GL_POP_GROUP_MARKER \
        static_cast<GPUAPI_OpenGL_Common*>(this->gpuAPI.get())->popDebugGroupMarker()
#else
#   define GL_CHECK_RESULT
#   define GL_GET_RESULT glGetError()
#   define GL_CHECK_PRESENT(x)
#   define GL_PUSH_GROUP_MARKER(title)
#   define GL_POP_GROUP_MARKER
#endif

namespace OsmAnd
{
    enum class GLShaderVariableType
    {
        In,
        Uniform
    };

    class GPUAPI_OpenGL_Common : public GPUAPI
    {
        Q_DISABLE_COPY(GPUAPI_OpenGL_Common);
    private:
        bool uploadTileAsTextureToGPU(const std::shared_ptr< const MapTile >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU);
        bool uploadTileAsArrayBufferToGPU(const std::shared_ptr< const MapTile >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU);

        bool uploadSymbolAsTextureToGPU(const std::shared_ptr< const MapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU);
    protected:
        GLint _maxTextureSize;
        QHash< GLuint, QMultiMap< GLShaderVariableType, GLint > > _programVariables;
        QStringList _extensions;
        QVector<GLint> _compressedFormats;

        bool _isSupported_vertexShaderTextureLookup;
        bool _isSupported_textureLod;
        bool _isSupported_texturesNPOT;
        bool _isSupported_EXT_debug_marker;
        
        virtual bool releaseResourceInGPU(const ResourceInGPU::Type type, const RefInGPU& refInGPU);

        virtual void glPushGroupMarkerEXT_wrapper(GLsizei length, const GLchar* marker) = 0;
        virtual void glPopGroupMarkerEXT_wrapper() = 0;
    public:
        GPUAPI_OpenGL_Common();
        virtual ~GPUAPI_OpenGL_Common();

        const QStringList& extensions;
        const QVector<GLint>& compressedFormats;

        const GLint& maxTextureSize;
        const bool& isSupported_vertexShaderTextureLookup;
        const bool& isSupported_textureLod;
        const bool& isSupported_texturesNPOT;
        const bool& isSupported_EXT_debug_marker;
        
        virtual GLenum validateResult() = 0;
        virtual GLuint compileShader(GLenum shaderType, const char* source);
        virtual GLuint linkProgram(GLuint shadersCount, GLuint *shaders);

        virtual TextureFormat getTextureFormat(const std::shared_ptr< const MapTile >& tile) = 0;
        virtual TextureFormat getTextureFormat(const std::shared_ptr< const MapSymbol >& symbol) = 0;
        virtual SourceFormat getSourceFormat(const std::shared_ptr< const MapTile >& tile) = 0;
        virtual SourceFormat getSourceFormat(const std::shared_ptr< const MapSymbol >& symbol) = 0;
        virtual void allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const TextureFormat format) = 0;
        virtual void uploadDataToTexture2D(GLenum target, GLint level,
            GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
            const GLvoid *data, GLsizei dataRowLengthInElements, GLsizei elementSize,
            const SourceFormat sourceFormat) = 0;
        virtual void setMipMapLevelsLimit(GLenum target, const uint32_t mipmapLevelsCount) = 0;

        virtual void glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays) = 0;
        virtual void glBindVertexArray_wrapper(GLuint array) = 0;
        virtual void glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays) = 0;

        virtual void preprocessVertexShader(QString& code) = 0;
        virtual void preprocessFragmentShader(QString& code) = 0;
        virtual void optimizeVertexShader(QString& code) = 0;
        virtual void optimizeFragmentShader(QString& code) = 0;

        enum class SamplerType : int32_t
        {
            Invalid = -1,

            ElevationDataTile,
            BitmapTile_Bilinear,
            BitmapTile_BilinearMipmap,
            BitmapTile_TrilinearMipmap,
            Symbol,

            __LAST
        };
        enum {
            SamplerTypesCount = static_cast<int>(SamplerType::__LAST),
        };
        virtual void setTextureBlockSampler(const GLenum textureBlock, const SamplerType samplerType) = 0;
        virtual void applyTextureBlockToTexture(const GLenum texture, const GLenum textureBlock) = 0;

        virtual void clearVariablesLookup();
        virtual void findVariableLocation(GLuint program, GLint& location, const QString& name, const GLShaderVariableType& type);

        virtual bool uploadTileToGPU(const std::shared_ptr< const MapTile >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU);
        virtual bool uploadSymbolToGPU(const std::shared_ptr< const MapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU);

        virtual void waitUntilUploadIsComplete();

        virtual void pushDebugGroupMarker(const QString& title);
        virtual void popDebugGroupMarker();
    };
}

#endif // !defined(_OSMAND_CORE_GPU_API__OPENGL_COMMON_H_)
