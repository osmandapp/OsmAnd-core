#ifndef _OSMAND_CORE_GPU_API_OPENGL_H_
#define _OSMAND_CORE_GPU_API_OPENGL_H_

#include "stdlib_common.h"
#include <type_traits>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMap>
#include <QMultiMap>
#include <QList>
#include <QStringList>
#include <QString>
#include <QVector>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#if defined(WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#if defined(OSMAND_OPENGL2PLUS_RENDERER_SUPPORTED)
#   include <GL/glew.h>
#endif
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#if defined(OSMAND_TARGET_OS_macosx)
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
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <glm/glm.hpp>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "SmartPOD.h"
#include "GPUAPI.h"

#if OSMAND_DEBUG
#   define GL_CHECK_RESULT \
        static_cast<GPUAPI_OpenGL*>(this->gpuAPI.get())->validateResult()
#   define GL_GET_RESULT \
        static_cast<GPUAPI_OpenGL*>(this->gpuAPI.get())->validateResult()
#   define GL_CHECK_PRESENT(funcName)                                                                           \
        {                                                                                                       \
            static bool __checked_presence_of_##funcName = std::is_function<decltype(funcName)>::value;         \
            if (!__checked_presence_of_##funcName)                                                              \
            {                                                                                                   \
                if (funcName == nullptr)                                                                        \
                {                                                                                               \
                    LogPrintf(LogSeverityLevel::Error, "Missing '" #funcName "()'!");                           \
                    LogFlush();                                                                                 \
                    std::terminate();                                                                           \
                }                                                                                               \
                __checked_presence_of_##funcName = true;                                                        \
            }                                                                                                   \
        }
#   define GL_PUSH_GROUP_MARKER(title) \
        static_cast<GPUAPI_OpenGL*>(this->gpuAPI.get())->pushDebugGroupMarker((title))
#   define GL_POP_GROUP_MARKER \
        static_cast<GPUAPI_OpenGL*>(this->gpuAPI.get())->popDebugGroupMarker()
#else
#   define GL_CHECK_RESULT
#   define GL_GET_RESULT glGetError()
#   define GL_CHECK_PRESENT(x)
#   define GL_PUSH_GROUP_MARKER(title)
#   define GL_POP_GROUP_MARKER
#endif

namespace OsmAnd
{
    class RasterMapSymbol;
    class VectorMapSymbol;

    enum class GLShaderVariableType
    {
        In,
        Uniform
    };

    template<typename T, T DEFAULT_VALUE>
    struct GLref : public SmartPOD<T, DEFAULT_VALUE>
    {
        typedef SmartPOD<T, DEFAULT_VALUE> Base;

        virtual ~GLref()
        {
        }

        inline GLref& operator=(const T& that)
        {
            Base::operator=(that);
            return *this;
        }

        inline operator bool() const
        {
            return this->value != DEFAULT_VALUE;
        }
    };

    typedef GLref<GLuint, 0> GLname;
    typedef GLref<GLint, -1> GLlocation;

    class GPUAPI_OpenGL : public GPUAPI
    {
        Q_DISABLE_COPY_AND_MOVE(GPUAPI_OpenGL);
    public:
        class ProgramVariablesLookupContext
        {
            Q_DISABLE_COPY_AND_MOVE(ProgramVariablesLookupContext);
        private:
            GPUAPI_OpenGL* const gpuAPI;
            const GLuint program;

            QMap< GLShaderVariableType, QMap<QString, GLint> > _variablesByName;
            QMap< GLShaderVariableType, QMap<GLint, QString> > _variablesByLocation;
        protected:
            ProgramVariablesLookupContext(GPUAPI_OpenGL* gpuAPI, GLuint program);
        public:
            virtual ~ProgramVariablesLookupContext();

            virtual void lookupLocation(GLint& location, const QString& name, const GLShaderVariableType& type);
            void lookupLocation(GLlocation& location, const QString& name, const GLShaderVariableType& type);

        friend class OsmAnd::GPUAPI_OpenGL;
        };
    private:
        bool uploadTileAsTextureToGPU(const std::shared_ptr< const MapTiledData >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU);
        bool uploadTileAsArrayBufferToGPU(const std::shared_ptr< const MapTiledData >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU);

        bool uploadSymbolAsTextureToGPU(const std::shared_ptr< const RasterMapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU);
        bool uploadSymbolAsMeshToGPU(const std::shared_ptr< const VectorMapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU);
    protected:
        unsigned int _glVersion;
        unsigned int _glslVersion;
        QStringList _extensions;
        QVector<GLint> _compressedFormats;

        GLint _maxTextureSize;
        bool _isSupported_vertexShaderTextureLookup;
        bool _isSupported_textureLod;
        bool _isSupported_texturesNPOT;
        bool _isSupported_EXT_debug_marker;
        bool _isSupported_texture_storage;
        bool _isSupported_texture_float;
        bool _isSupported_texture_rg;
        GLint _maxVertexUniformVectors;
        GLint _maxFragmentUniformVectors;
        
        virtual bool releaseResourceInGPU(const ResourceInGPU::Type type, const RefInGPU& refInGPU);

        virtual void glPushGroupMarkerEXT_wrapper(GLsizei length, const GLchar* marker) = 0;
        virtual void glPopGroupMarkerEXT_wrapper() = 0;

        virtual TextureFormat getTextureFormat(const SkBitmap::Config skBitmapConfig) const;
        virtual TextureFormat getTextureSizedFormat(const SkBitmap::Config skBitmapConfig) const = 0;
        virtual TextureFormat getTextureSizedFormat_float() const = 0;

        virtual SourceFormat getSourceFormat(const SkBitmap::Config skBitmapConfig) const;
        virtual SourceFormat getSourceFormat_float() const = 0;
    public:
        GPUAPI_OpenGL();
        virtual ~GPUAPI_OpenGL();

        const unsigned int& glVersion;
        const unsigned int& glslVersion;
        const QStringList& extensions;
        const QVector<GLint>& compressedFormats;

        const GLint& maxTextureSize;
        const bool& isSupported_vertexShaderTextureLookup;
        const bool& isSupported_textureLod;
        const bool& isSupported_texturesNPOT;
        const bool& isSupported_EXT_debug_marker;
        const bool& isSupported_texture_storage;
        const bool& isSupported_texture_float;
        const bool& isSupported_texture_rg;
        const GLint& maxVertexUniformVectors;
        const GLint& maxFragmentUniformVectors;
        
        virtual GLenum validateResult() = 0;

        virtual GLuint compileShader(GLenum shaderType, const char* source);
        virtual GLuint linkProgram(GLuint shadersCount, const GLuint* shaders, const bool autoReleaseShaders = true);

        virtual TextureFormat getTextureFormat(const std::shared_ptr< const MapTiledData >& tile);
        virtual TextureFormat getTextureFormat(const std::shared_ptr< const RasterMapSymbol >& symbol);
        virtual SourceFormat getSourceFormat(const std::shared_ptr< const MapTiledData >& tile);
        virtual SourceFormat getSourceFormat(const std::shared_ptr< const RasterMapSymbol >& symbol);
        virtual void allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const TextureFormat format);
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

        enum class SamplerType
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

        virtual std::shared_ptr<ProgramVariablesLookupContext> obtainVariablesLookupContext(const GLuint& program);
        virtual void findVariableLocation(const GLuint& program, GLint& location, const QString& name, const GLShaderVariableType& type);

        virtual bool uploadTileToGPU(const std::shared_ptr< const MapTiledData >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU);
        virtual bool uploadSymbolToGPU(const std::shared_ptr< const MapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU);

        virtual void waitUntilUploadIsComplete();

        virtual void pushDebugGroupMarker(const QString& title);
        virtual void popDebugGroupMarker();

        virtual void glClearDepth_wrapper(const float depth) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_GPU_API_OPENGL_H_)
