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
#include <QHash>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#if defined(OSMAND_TARGET_OS_windows)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
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

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "SmartPOD.h"
#include "GPUAPI.h"
#include "Logging.h"

#if !defined(OSMAND_GPU_DEBUG)
#   define OSMAND_GPU_DEBUG OSMAND_DEBUG
#endif // !defined(OSMAND_GPU_DEBUG)

#if OSMAND_GPU_DEBUG
#   define GL_CHECK_RESULT \
        static_cast<GPUAPI_OpenGL*>(this->gpuAPI.get())->validateResult(__FUNCTION__, __FILE__, __LINE__)
#   define GL_GET_RESULT \
        static_cast<GPUAPI_OpenGL*>(this->gpuAPI.get())->validateResult(__FUNCTION__, __FILE__, __LINE__)
#   define GL_GET_AND_CHECK_RESULT \
        static_cast<GPUAPI_OpenGL*>(this->gpuAPI.get())->validateResult(__FUNCTION__, __FILE__, __LINE__)
#   define GL_CHECK_PRESENT(x) \
        const static OsmAnd::GPUAPI_OpenGL::glPresenseChecker<decltype(x)> glPresenseChecker_##x(&x, #x)
#   define GL_PUSH_GROUP_MARKER(title) \
        static_cast<GPUAPI_OpenGL*>(this->gpuAPI.get())->pushDebugGroupMarker((title))
#   define GL_POP_GROUP_MARKER \
        static_cast<GPUAPI_OpenGL*>(this->gpuAPI.get())->popDebugGroupMarker()
#else
#   define GL_CHECK_RESULT
#   define GL_GET_RESULT glGetError()
#   define GL_GET_AND_CHECK_RESULT glGetError()
#   define GL_CHECK_PRESENT(x)
#   define GL_PUSH_GROUP_MARKER(title)
#   define GL_POP_GROUP_MARKER
#endif

namespace OsmAnd
{
    class RasterMapSymbol;
    class VectorMapSymbol;

    enum class GlslVariableType
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

        inline bool isValid() const
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

        union TextureFormat
        {
            GPUAPI::TextureFormat value;
            struct
            {
                uint16_t type;
                uint16_t format;
            };

            static TextureFormat Make(GLenum type, GLenum format);
        };

        union SourceFormat
        {
            GPUAPI::SourceFormat value;
            struct
            {
                uint16_t type;
                uint16_t format;
            };

            static SourceFormat Make(GLenum type, GLenum format);
        };

        template <typename T, typename Enable = void>
        struct glPresenseChecker
        {
            Q_DISABLE_COPY_AND_MOVE(glPresenseChecker);

            glPresenseChecker(
                    T* const unknownStuff,
                    const char* const functionName)
            {
                LogPrintf(LogSeverityLevel::Error, "'%s' is not function or function pointer!", functionName);
                LogFlush();
                std::terminate();
            }

            static_assert(std::true_type::value, "What are you doing?");
        };

        template <typename T>
        struct glPresenseChecker<T, typename std::enable_if< std::is_pointer<T>::value >::type> Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(glPresenseChecker);

            glPresenseChecker(
                T* const pointerToFunctionPointer,
                const char* const functionName)
            {
                if ((*pointerToFunctionPointer) != nullptr)
                    return;

                LogPrintf(LogSeverityLevel::Error, "Function '%s()' was not loaded!", functionName);
                LogFlush();
                std::terminate();
            }
        };

        template <typename T>
        struct glPresenseChecker<T, typename std::enable_if< std::is_function<T>::value >::type> Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(glPresenseChecker);

            glPresenseChecker(
                T* const function,
                const char* const functionName)
            {
                // Nothing to do here, function is always present
                Q_UNUSED(function);
                Q_UNUSED(functionName);
            }
        };

        struct GlslProgramVariable
        {
            GlslVariableType type;
            QString name;
            GLenum dataType;
            GLsizei size;
        };

        class ProgramVariablesLookupContext
        {
            Q_DISABLE_COPY_AND_MOVE(ProgramVariablesLookupContext);
        private:
            QMap< GlslVariableType, QMap<QString, GLint> > _variablesByName;
            QMap< GlslVariableType, QMap<GLint, QString> > _variablesByLocation;
        protected:
            ProgramVariablesLookupContext(
                GPUAPI_OpenGL* const gpuAPI,
                const GLuint program,
                const QHash<QString, GlslProgramVariable>& variablesMap);
        public:
            virtual ~ProgramVariablesLookupContext();

            GPUAPI_OpenGL* const gpuAPI;
            const GLuint program;
            const QHash<QString, GlslProgramVariable> variablesMap;

            virtual bool lookupLocation(GLint& outLocation, const QString& name, const GlslVariableType type);
            bool lookupLocation(GLlocation& outLocation, const QString& name, const GlslVariableType type);
            bool lookupInLocation(GLlocation& outLocation, const QString& name);
            bool lookupUniformLocation(GLlocation& outLocation, const QString& name);

        friend class OsmAnd::GPUAPI_OpenGL;
        };
    private:
        bool uploadTiledDataAsTextureToGPU(const std::shared_ptr< const IMapTiledDataProvider::Data >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU);
        bool uploadTiledDataAsArrayBufferToGPU(const std::shared_ptr< const IMapTiledDataProvider::Data >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU);

        bool uploadSymbolAsTextureToGPU(const std::shared_ptr< const RasterMapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU);
        bool uploadSymbolAsMeshToGPU(const std::shared_ptr< const VectorMapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU);

        GLuint _vaoSimulationLastUnusedId;
        struct SimulatedVAO
        {
            GLname bindedArrayBuffer;
            GLname bindedElementArrayBuffer;

            struct VertexAttrib
            {
                GLname arrayBufferBinding;
                GLint arraySize;
                GLsizei arrayStride;
                GLenum arrayType;
                GLboolean arrayIsNormalized;
                GLvoid* arrayPointer;
            };
            QHash<GLuint, VertexAttrib> vertexAttribs;
        };
        QHash<GLname, SimulatedVAO> _vaoSimulationObjects;
        GLname _lastUsedSimulatedVAOObject;

        static QString decodeGlslVariableDataType(const GLenum dataType);
    protected:
        unsigned int _glVersion;
        unsigned int _glslVersion;
        QStringList _extensions;
        QVector<GLint> _compressedFormats;

        GLint _maxTextureSize;
        GLint _maxTextureUnitsInVertexShader;
        GLint _maxTextureUnitsInFragmentShader;
        GLint _maxTextureUnitsCombined;
        bool _isSupported_vertexShaderTextureLookup;
        bool _isSupported_textureLod;
        bool _isSupported_texturesNPOT;
        bool _isSupported_EXT_debug_marker;
        bool _isSupported_texture_storage;
        bool _isSupported_texture_float;
        bool _isSupported_texture_half_float;
        bool _isSupported_texture_rg;
        bool _isSupported_vertex_array_object;
        GLint _maxVertexUniformVectors;
        GLint _maxFragmentUniformVectors;
        GLint _maxVaryingFloats;
        GLint _maxVertexAttribs;
        
        virtual bool releaseResourceInGPU(const ResourceInGPU::Type type, const RefInGPU& refInGPU);

        virtual void glPushGroupMarkerEXT_wrapper(GLsizei length, const GLchar* marker) = 0;
        virtual void glPopGroupMarkerEXT_wrapper() = 0;

        virtual TextureFormat getTextureFormat(const SkColorType colorType) const;
        virtual TextureFormat getTextureFormat_float() const;
        virtual bool isValidTextureFormat(const TextureFormat textureFormat) const = 0;
        virtual size_t getTextureFormatPixelSize(const TextureFormat textureFormat) const;
        virtual GLenum getBaseInteralTextureFormat(const TextureFormat textureFormat) const;

        virtual SourceFormat getSourceFormat(const SkColorType colorType) const;
        virtual SourceFormat getSourceFormat_float() const = 0;
        virtual bool isValidSourceFormat(const SourceFormat sourceFormat) const = 0;

        virtual void glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays) = 0;
        virtual void glBindVertexArray_wrapper(GLuint array) = 0;
        virtual void glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays) = 0;
    public:
        GPUAPI_OpenGL();
        virtual ~GPUAPI_OpenGL();

        const unsigned int& glVersion;
        const unsigned int& glslVersion;
        const QStringList& extensions;
        const QVector<GLint>& compressedFormats;

        const GLint& maxTextureSize;
        const GLint& maxTextureUnitsInVertexShader;
        const GLint& maxTextureUnitsInFragmentShader;
        const GLint& maxTextureUnitsCombined;
        const bool& isSupported_vertexShaderTextureLookup;
        const bool& isSupported_textureLod;
        const bool& isSupported_texturesNPOT;
        const bool& isSupported_EXT_debug_marker;
        const bool& isSupported_texture_storage;
        const bool& isSupported_texture_float;
        const bool& isSupported_texture_half_float;
        const bool& isSupported_texture_rg;
        const bool& isSupported_vertex_array_object;
        const GLint& maxVertexUniformVectors;
        const GLint& maxFragmentUniformVectors;
        const GLint& maxVaryingFloats;
        const GLint& maxVertexAttribs;
        
        virtual GLenum validateResult(const char* const function, const char* const file, const int line) = 0;

        virtual GLuint compileShader(GLenum shaderType, const char* source);
        virtual GLuint linkProgram(
            GLuint shadersCount,
            const GLuint* shaders,
            const bool autoReleaseShaders = true,
            QHash<QString, GlslProgramVariable>* outVariablesMap = nullptr);

        virtual TextureFormat getTextureFormat(const std::shared_ptr< const IMapTiledDataProvider::Data >& tile);
        virtual TextureFormat getTextureFormat(const std::shared_ptr< const RasterMapSymbol >& symbol);
        virtual SourceFormat getSourceFormat(const std::shared_ptr< const IMapTiledDataProvider::Data >& tile);
        virtual SourceFormat getSourceFormat(const std::shared_ptr< const RasterMapSymbol >& symbol);
        virtual void allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const TextureFormat format);
        virtual void uploadDataToTexture2D(GLenum target, GLint level,
            GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
            const GLvoid *data, GLsizei dataRowLengthInElements, GLsizei elementSize,
            const SourceFormat sourceFormat) = 0;
        virtual void setMipMapLevelsLimit(GLenum target, const uint32_t mipmapLevelsCount) = 0;

        GLname allocateUninitializedVAO();
        void initializeVAO(const GLname vao);
        void useVAO(const GLname vao);
        void unuseVAO();
        void releaseVAO(const GLname vao, const bool gpuContextLost = false);

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

        virtual std::shared_ptr<ProgramVariablesLookupContext> obtainVariablesLookupContext(
            const GLuint& program,
            const QHash<QString, GlslProgramVariable>& variablesMap);
        virtual bool findVariableLocation(const GLuint& program, GLint& location, const QString& name, const GlslVariableType& type);

        virtual bool uploadTiledDataToGPU(const std::shared_ptr< const IMapTiledDataProvider::Data >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU);
        virtual bool uploadSymbolToGPU(const std::shared_ptr< const MapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU);

        virtual void waitUntilUploadIsComplete();

        virtual void pushDebugGroupMarker(const QString& title);
        virtual void popDebugGroupMarker();

        virtual void glClearDepth_wrapper(const float depth) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_GPU_API_OPENGL_H_)
