#include "GPUAPI_OpenGL.h"

#include <cassert>

#include "QtExtensions.h"
#include <QtMath>
#include <QRegularExpression>
#include <QRegExp>

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include <SkCanvas.h>
#include "restore_internal_warnings.h"

#include "IMapRenderer.h"
#include "IMapTiledDataProvider.h"
#include "IRasterMapLayerProvider.h"
#include "IMapElevationDataProvider.h"
#include "MapSymbol.h"
#include "RasterMapSymbol.h"
#include "VectorMapSymbol.h"
#include "Logging.h"
#include "Utilities.h"
#include "QKeyValueIterator.h"

#undef GL_CHECK_RESULT
#undef GL_GET_RESULT
#undef GL_GET_AND_CHECK_RESULT
#if OSMAND_GPU_DEBUG
#   define GL_CHECK_RESULT validateResult(__FUNCTION__, __FILE__, __LINE__)
#   define GL_GET_RESULT validateResult(__FUNCTION__, __FILE__, __LINE__)
#   define GL_GET_AND_CHECK_RESULT validateResult(__FUNCTION__, __FILE__, __LINE__)
#else
#   define GL_CHECK_RESULT
#   define GL_GET_RESULT glGetError()
#   define GL_GET_AND_CHECK_RESULT glGetError()
#endif

OsmAnd::GPUAPI_OpenGL::GPUAPI_OpenGL()
    : _vaoSimulationLastUnusedId(1)
    , _glVersion(0)
    , _glslVersion(0)
    , _maxTextureSize(0)
    , _maxTextureUnitsInVertexShader(0)
    , _maxTextureUnitsInFragmentShader(0)
    , _maxTextureUnitsCombined(0)
    , _isSupported_vertexShaderTextureLookup(false)
    , _isSupported_textureLod(false)
    , _isSupported_texturesNPOT(false)
    , _isSupported_texture_storage(false)
    , _isSupported_texture_float(false)
    , _isSupported_texture_rg(false)
    , _isSupported_vertex_array_object(false)
    , _maxVertexUniformVectors(-1)
    , _maxFragmentUniformVectors(-1)
    , _maxVertexAttribs(-1)
    , glVersion(_glVersion)
    , glslVersion(_glslVersion)
    , extensions(_extensions)
    , compressedFormats(_compressedFormats)
    , maxTextureSize(_maxTextureSize)
    , maxTextureUnitsInVertexShader(_maxTextureUnitsInVertexShader)
    , maxTextureUnitsInFragmentShader(_maxTextureUnitsInFragmentShader)
    , maxTextureUnitsCombined(_maxTextureUnitsCombined)
    , isSupported_vertexShaderTextureLookup(_isSupported_vertexShaderTextureLookup)
    , isSupported_textureLod(_isSupported_textureLod)
    , isSupported_texturesNPOT(_isSupported_texturesNPOT)
    , isSupported_EXT_debug_marker(_isSupported_EXT_debug_marker)
    , isSupported_texture_storage(_isSupported_texture_storage)
    , isSupported_texture_float(_isSupported_texture_float)
    , isSupported_texture_rg(_isSupported_texture_rg)
    , isSupported_vertex_array_object(_isSupported_vertex_array_object)
    , maxVertexUniformVectors(_maxVertexUniformVectors)
    , maxFragmentUniformVectors(_maxFragmentUniformVectors)
    , maxVertexAttribs(_maxVertexAttribs)
{
}

OsmAnd::GPUAPI_OpenGL::~GPUAPI_OpenGL()
{
}

GLuint OsmAnd::GPUAPI_OpenGL::compileShader(GLenum shaderType, const char* source)
{
    GL_CHECK_PRESENT(glCreateShader);
    GL_CHECK_PRESENT(glShaderSource);
    GL_CHECK_PRESENT(glCompileShader);
    GL_CHECK_PRESENT(glGetShaderiv);
    GL_CHECK_PRESENT(glGetShaderInfoLog);
    GL_CHECK_PRESENT(glDeleteShader);
    GL_CHECK_PRESENT(glDeleteProgram);

    GLuint shader;

    shader = glCreateShader(shaderType);
    GL_CHECK_RESULT;
    if (shader == 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to allocate GLSL shader");
        return shader;
    }

    const GLint sourceLen = static_cast<GLint>(strlen(source));
    glShaderSource(shader, 1, &source, &sourceLen);
    GL_CHECK_RESULT;

    glCompileShader(shader);
    const auto compilationResult = GL_GET_AND_CHECK_RESULT;

    // Check if compiled
    GLint didCompile = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &didCompile);
    GL_CHECK_RESULT;
    if (didCompile != GL_TRUE)
    {
        GLint logBufferSize = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logBufferSize);
        GL_CHECK_RESULT;

        //WORKAROUND: Some drivers report incorrect length of the log
        //  - Qualcomm Adreno https://developer.qualcomm.com/forum/qdevnet-forums/mobile-technologies/mobile-gaming-graphics-optimization-adreno/26795
        bool logBufferSizeWorkaround = false;
        if (logBufferSize <= 0)
        {
            logBufferSize = 4096;
            logBufferSizeWorkaround = true;
        }

        const auto logBuffer = new GLchar[logBufferSize];
        memset(logBuffer, 0, sizeof(GLchar) * logBufferSize);

        GLsizei actualLogLength = 0;
        glGetShaderInfoLog(shader, logBufferSize, &actualLogLength, logBuffer);
        GL_CHECK_RESULT;

        //WORKAROUND: Sometimes compilation fails without a reason, but at least report glGetError() value
        if (logBuffer[0] == '\0')
        {
            sprintf(logBuffer,
                "Driver haven't reported compilation failure reason, last result code was 0x%08x",
                compilationResult);
            logBufferSizeWorkaround = false;
        }

        if (logBufferSizeWorkaround)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to compile GLSL shader (driver is buggy so errors log may be incomplete):\n"
                "%s\n"
                "Source:\n"
                "-------SHADER BEGIN-------\n"
                "%s\n"
                "--------SHADER END--------",
                logBuffer,
                source);
        }
        else
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to compile GLSL shader:\n"
                "%s\n"
                "Source:\n"
                "-------SHADER BEGIN-------\n"
                "%s\n"
                "--------SHADER END--------",
                logBuffer,
                source);
        }

        delete[] logBuffer;

        glDeleteShader(shader);
        shader = 0;
        return shader;
    }

    return shader;
}

GLuint OsmAnd::GPUAPI_OpenGL::linkProgram(
    GLuint shadersCount,
    const GLuint* shaders,
    const bool autoReleaseShaders /*= true*/,
    QHash<QString, GlslProgramVariable>* outVariablesMap /*= nullptr*/)
{
    GL_CHECK_PRESENT(glCreateProgram);
    GL_CHECK_PRESENT(glAttachShader);
    GL_CHECK_PRESENT(glDetachShader);
    GL_CHECK_PRESENT(glLinkProgram);
    GL_CHECK_PRESENT(glGetProgramiv);
    GL_CHECK_PRESENT(glGetProgramInfoLog);
    GL_CHECK_PRESENT(glDeleteProgram);
    GL_CHECK_PRESENT(glGetActiveAttrib);
    GL_CHECK_PRESENT(glGetActiveUniform);

    GLuint program = 0;

    program = glCreateProgram();
    GL_CHECK_RESULT;
    if (program == 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to allocate GLSL program");
        return program;
    }

    for (auto shaderIdx = 0u; shaderIdx < shadersCount; shaderIdx++)
    {
        glAttachShader(program, shaders[shaderIdx]);
        GL_CHECK_RESULT;
    }

    glLinkProgram(program);
    const auto linkingResult = GL_GET_AND_CHECK_RESULT;

    for (auto shaderIdx = 0u; shaderIdx < shadersCount; shaderIdx++)
    {
        glDetachShader(program, shaders[shaderIdx]);
        GL_CHECK_RESULT;

        if (autoReleaseShaders)
        {
            glDeleteShader(shaders[shaderIdx]);
            GL_CHECK_RESULT;
        }
    }

    GLint linkSuccessful;
    glGetProgramiv(program, GL_LINK_STATUS, &linkSuccessful);
    GL_CHECK_RESULT;
    if (linkSuccessful == GL_FALSE)
    {
        GLint logBufferSize = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logBufferSize);
        GL_CHECK_RESULT;

        //WORKAROUND: Some drivers report incorrect length of the log
        //  - Qualcomm Adreno https://developer.qualcomm.com/forum/qdevnet-forums/mobile-technologies/mobile-gaming-graphics-optimization-adreno/26795
        bool logBufferSizeWorkaround = false;
        if (logBufferSize <= 0)
        {
            logBufferSize = 4096;
            logBufferSizeWorkaround = true;
        }

        const auto logBuffer = new GLchar[logBufferSize];
        memset(logBuffer, 0, sizeof(GLchar) * logBufferSize);

        GLsizei actualLogLength = 0;
        glGetProgramInfoLog(program, logBufferSize, &actualLogLength, logBuffer);
        GL_CHECK_RESULT;

        //WORKAROUND: Sometimes compilation fails without a reason, but at least report glGetError() value
        if (logBuffer[0] == '\0')
        {
            sprintf(logBuffer, "Driver haven't reported linking failure reason, last result code was 0x%08x", linkingResult);
            logBufferSizeWorkaround = false;
        }

        if (logBufferSizeWorkaround)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to link GLSL program %d (driver is buggy so errors log may be incomplete):\n%s",
                program,
                logBuffer);
        }
        else
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to link GLSL program %d:\n%s",
                program,
                logBuffer);
        }

        delete[] logBuffer;

        glDeleteProgram(program);
        GL_CHECK_RESULT;

        return 0;
    }

    if (outVariablesMap)
        outVariablesMap->clear();

    GLint attributesCount = 0;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attributesCount);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Verbose, "GLSL program %d has %d input variable(s)", program, attributesCount);

    GLint attributeNameMaxLength = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &attributeNameMaxLength);
    GL_CHECK_RESULT;

    const auto attributeName = new char[attributeNameMaxLength];
    for (int attributeIdx = 0; attributeIdx < attributesCount; attributeIdx++)
    {
        GLint attributeSize = 0;
        GLenum attributeType = GL_INVALID_ENUM;
        memset(attributeName, 0, attributeNameMaxLength);
        glGetActiveAttrib(program, attributeIdx, attributeNameMaxLength, NULL, &attributeSize, &attributeType, attributeName);
        GL_CHECK_RESULT;

#if OSMAND_GPU_DEBUG
        if (attributeSize > 1)
        {
            LogPrintf(LogSeverityLevel::Verbose,
                "\tInput %d: %20s %-20s <Size: %d>",
                attributeIdx,
                qPrintable(decodeGlslVariableDataType(attributeType)),
                attributeName,
                attributeSize);
        }
        else
        {
            LogPrintf(LogSeverityLevel::Verbose,
                "\tInput %d: %20s %-20s",
                attributeIdx,
                qPrintable(decodeGlslVariableDataType(attributeType)),
                attributeName);
        }
#endif // OSMAND_GPU_DEBUG

        if (outVariablesMap)
        {
            const QString name = QLatin1String(attributeName);
            outVariablesMap->insert(name, { GlslVariableType::In, name, attributeType, attributeSize });
        }
    }
    delete[] attributeName;

    GLint uniformsCount = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniformsCount);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Verbose, "GLSL program %d has %d parameter variable(s)", program, uniformsCount);

    GLint uniformNameMaxLength = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformNameMaxLength);
    GL_CHECK_RESULT;

    const auto uniformName = new char[uniformNameMaxLength];
    for (int uniformIdx = 0; uniformIdx < uniformsCount; uniformIdx++)
    {
        GLint uniformSize = 0;
        GLenum uniformType = GL_INVALID_ENUM;
        memset(uniformName, 0, uniformNameMaxLength);
        glGetActiveUniform(program, uniformIdx, uniformNameMaxLength, NULL, &uniformSize, &uniformType, uniformName);
        GL_CHECK_RESULT;

#if OSMAND_GPU_DEBUG
        if (uniformSize > 1)
        {
            LogPrintf(LogSeverityLevel::Verbose,
                "\tUniform %d: %20s %-20s <Size: %d>",
                uniformIdx,
                qPrintable(decodeGlslVariableDataType(uniformType)),
                uniformName,
                uniformSize);
        }
        else
        {
            LogPrintf(LogSeverityLevel::Verbose,
                "\tUniform %d: %20s %-20s",
                uniformIdx,
                qPrintable(decodeGlslVariableDataType(uniformType)),
                uniformName);
        }
#endif // OSMAND_GPU_DEBUG

        if (outVariablesMap)
        {
            const QString name = QLatin1String(attributeName);
            outVariablesMap->insert(name, { GlslVariableType::Uniform, name, uniformType, uniformSize });
        }
    }
    delete[] uniformName;

    return program;
}

QString OsmAnd::GPUAPI_OpenGL::decodeGlslVariableDataType(const GLenum dataType)
{
    struct GlslDataTypeEntry
    {
        GLenum dataType;
        QString name;
    };

    static const GlslDataTypeEntry glslDataTypes[] =
    {
        { GL_INVALID_ENUM, QLatin1String("invalid") },
#if defined(GL_FLOAT)
        { GL_FLOAT, QLatin1String("float") },
#endif // defined(GL_FLOAT)
#if defined(GL_FLOAT_VEC2)
        { GL_FLOAT_VEC2, QLatin1String("vec2") },
#endif // defined(GL_FLOAT_VEC2)
#if defined(GL_FLOAT_VEC3)
        { GL_FLOAT_VEC3, QLatin1String("vec3") },
#endif // defined(GL_FLOAT_VEC3)
#if defined(GL_FLOAT_VEC4)
        { GL_FLOAT_VEC4, QLatin1String("vec4") },
#endif // defined(GL_FLOAT_VEC4)
#if defined(GL_DOUBLE)
        { GL_DOUBLE, QLatin1String("double") },
#endif // defined(GL_DOUBLE)
#if defined(GL_DOUBLE_VEC2)
        { GL_DOUBLE_VEC2, QLatin1String("dvec2") },
#endif // defined(GL_DOUBLE_VEC2)
#if defined(GL_DOUBLE_VEC3)
        { GL_DOUBLE_VEC3, QLatin1String("dvec3") },
#endif // defined(GL_DOUBLE_VEC3)
#if defined(GL_DOUBLE_VEC4)
        { GL_DOUBLE_VEC4, QLatin1String("dvec4") },
#endif // defined(GL_DOUBLE_VEC4)
#if defined(GL_INT)
        { GL_INT, QLatin1String("int") },
#endif // defined(GL_INT)
#if defined(GL_INT_VEC2)
        { GL_INT_VEC2, QLatin1String("ivec2") },
#endif // defined(GL_INT_VEC2)
#if defined(GL_INT_VEC3)
        { GL_INT_VEC3, QLatin1String("ivec3") },
#endif // defined(GL_INT_VEC3)
#if defined(GL_INT_VEC4)
        { GL_INT_VEC4, QLatin1String("ivec4") },
#endif // defined(GL_INT_VEC4)
#if defined(GL_UNSIGNED_INT)
        { GL_UNSIGNED_INT, QLatin1String("unsigned int") },
#endif // defined(GL_UNSIGNED_INT)
#if defined(GL_UNSIGNED_INT_VEC2)
        { GL_UNSIGNED_INT_VEC2, QLatin1String("uvec2") },
#endif // defined(GL_UNSIGNED_INT_VEC2)
#if defined(GL_UNSIGNED_INT_VEC3)
        { GL_UNSIGNED_INT_VEC3, QLatin1String("uvec3") },
#endif // defined(GL_UNSIGNED_INT_VEC3)
#if defined(GL_UNSIGNED_INT_VEC4)
        { GL_UNSIGNED_INT_VEC4, QLatin1String("uvec4") },
#endif // defined(GL_UNSIGNED_INT_VEC4)
#if defined(GL_BOOL)
        { GL_BOOL, QLatin1String("bool") },
#endif // defined(GL_BOOL)
#if defined(GL_BOOL_VEC2)
        { GL_BOOL_VEC2, QLatin1String("bvec2") },
#endif // defined(GL_BOOL_VEC2)
#if defined(GL_BOOL_VEC3)
        { GL_BOOL_VEC3, QLatin1String("bvec3") },
#endif // defined(GL_BOOL_VEC3)
#if defined(GL_BOOL_VEC4)
        { GL_BOOL_VEC4, QLatin1String("bvec4") },
#endif // defined(GL_BOOL_VEC4)
#if defined(GL_FLOAT_MAT2)
        { GL_FLOAT_MAT2, QLatin1String("mat2") },
#endif // defined(GL_FLOAT_MAT2)
#if defined(GL_FLOAT_MAT3)
        { GL_FLOAT_MAT3, QLatin1String("mat3") },
#endif // defined(GL_FLOAT_MAT3)
#if defined(GL_FLOAT_MAT4)
        { GL_FLOAT_MAT4, QLatin1String("mat4") },
#endif // defined(GL_FLOAT_MAT4)
#if defined(GL_FLOAT_MAT2x3)
        { GL_FLOAT_MAT2x3, QLatin1String("mat2x3") },
#endif // defined(GL_FLOAT_MAT2x3)
#if defined(GL_FLOAT_MAT2x4)
        { GL_FLOAT_MAT2x4, QLatin1String("mat2x4") },
#endif // defined(GL_FLOAT_MAT2x4)
#if defined(GL_FLOAT_MAT3x2)
        { GL_FLOAT_MAT3x2, QLatin1String("mat3x2") },
#endif // defined(GL_FLOAT_MAT3x2)
#if defined(GL_FLOAT_MAT3x4)
        { GL_FLOAT_MAT3x4, QLatin1String("mat3x4") },
#endif // defined(GL_FLOAT_MAT3x4)
#if defined(GL_FLOAT_MAT4x2)
        { GL_FLOAT_MAT4x2, QLatin1String("mat4x2") },
#endif // defined(GL_FLOAT_MAT4x2)
#if defined(GL_FLOAT_MAT4x3)
        { GL_FLOAT_MAT4x3, QLatin1String("mat4x3") },
#endif // defined(GL_FLOAT_MAT4x3)
#if defined(GL_DOUBLE_MAT2)
        { GL_DOUBLE_MAT2, QLatin1String("dmat2") },
#endif // defined(GL_DOUBLE_MAT2)
#if defined(GL_DOUBLE_MAT3)
        { GL_DOUBLE_MAT3, QLatin1String("dmat3") },
#endif // defined(GL_DOUBLE_MAT3)
#if defined(GL_DOUBLE_MAT4)
        { GL_DOUBLE_MAT4, QLatin1String("dmat4") },
#endif // defined(GL_DOUBLE_MAT4)
#if defined(GL_DOUBLE_MAT2x3)
        { GL_DOUBLE_MAT2x3, QLatin1String("dmat2x3") },
#endif // defined(GL_DOUBLE_MAT2x3)
#if defined(GL_DOUBLE_MAT2x4)
        { GL_DOUBLE_MAT2x4, QLatin1String("dmat2x4") },
#endif // defined(GL_DOUBLE_MAT2x4)
#if defined(GL_DOUBLE_MAT3x2)
        { GL_DOUBLE_MAT3x2, QLatin1String("dmat3x2") },
#endif // defined(GL_DOUBLE_MAT3x2)
#if defined(GL_DOUBLE_MAT3x4)
        { GL_DOUBLE_MAT3x4, QLatin1String("dmat3x4") },
#endif // defined(GL_DOUBLE_MAT3x4)
#if defined(GL_DOUBLE_MAT4x2)
        { GL_DOUBLE_MAT4x2, QLatin1String("dmat4x2") },
#endif // defined(GL_DOUBLE_MAT4x2)
#if defined(GL_DOUBLE_MAT4x3)
        { GL_DOUBLE_MAT4x3, QLatin1String("dmat4x3") },
#endif // defined(GL_DOUBLE_MAT4x3)
#if defined(GL_SAMPLER_1D)
        { GL_SAMPLER_1D, QLatin1String("sampler1D") },
#endif // defined(GL_SAMPLER_1D)
#if defined(GL_SAMPLER_2D)
        { GL_SAMPLER_2D, QLatin1String("sampler2D") },
#endif // defined(GL_SAMPLER_2D)
#if defined(GL_SAMPLER_3D)
        { GL_SAMPLER_3D, QLatin1String("sampler3D") },
#endif // defined(GL_SAMPLER_3D)
#if defined(GL_SAMPLER_CUBE)
        { GL_SAMPLER_CUBE, QLatin1String("samplerCube") },
#endif // defined(GL_SAMPLER_CUBE)
#if defined(GL_SAMPLER_1D_SHADOW)
        { GL_SAMPLER_1D_SHADOW, QLatin1String("sampler1DShadow") },
#endif // defined(GL_SAMPLER_1D_SHADOW)
#if defined(GL_SAMPLER_2D_SHADOW)
        { GL_SAMPLER_2D_SHADOW, QLatin1String("sampler2DShadow") },
#endif // defined(GL_SAMPLER_2D_SHADOW)
#if defined(GL_SAMPLER_1D_ARRAY)
        { GL_SAMPLER_1D_ARRAY, QLatin1String("sampler1DArray") },
#endif // defined(GL_SAMPLER_1D_ARRAY)
#if defined(GL_SAMPLER_2D_ARRAY)
        { GL_SAMPLER_2D_ARRAY, QLatin1String("sampler2DArray") },
#endif // defined(GL_SAMPLER_2D_ARRAY)
#if defined(GL_SAMPLER_1D_ARRAY_SHADOW)
        { GL_SAMPLER_1D_ARRAY_SHADOW, QLatin1String("sampler1DArrayShadow") },
#endif // defined(GL_SAMPLER_1D_ARRAY_SHADOW)
#if defined(GL_SAMPLER_2D_ARRAY_SHADOW)
        { GL_SAMPLER_2D_ARRAY_SHADOW, QLatin1String("sampler2DArrayShadow") },
#endif // defined(GL_SAMPLER_2D_ARRAY_SHADOW)
#if defined(GL_SAMPLER_2D_MULTISAMPLE)
        { GL_SAMPLER_2D_MULTISAMPLE, QLatin1String("sampler2DMS") },
#endif // defined(GL_SAMPLER_2D_MULTISAMPLE)
#if defined(GL_SAMPLER_2D_MULTISAMPLE_ARRAY)
        { GL_SAMPLER_2D_MULTISAMPLE_ARRAY, QLatin1String("sampler2DMSArray") },
#endif // defined(GL_SAMPLER_2D_MULTISAMPLE_ARRAY)
#if defined(GL_SAMPLER_CUBE_SHADOW)
        { GL_SAMPLER_CUBE_SHADOW, QLatin1String("samplerCubeShadow") },
#endif // defined(GL_SAMPLER_CUBE_SHADOW)
#if defined(GL_SAMPLER_BUFFER)
        { GL_SAMPLER_BUFFER, QLatin1String("samplerBuffer") },
#endif // defined(GL_SAMPLER_BUFFER)
#if defined(GL_SAMPLER_2D_RECT)
        { GL_SAMPLER_2D_RECT, QLatin1String("sampler2DRect") },
#endif // defined(GL_SAMPLER_2D_RECT)
#if defined(GL_SAMPLER_2D_RECT_SHADOW)
        { GL_SAMPLER_2D_RECT_SHADOW, QLatin1String("sampler2DRectShadow") },
#endif // defined(GL_SAMPLER_2D_RECT_SHADOW)
#if defined(GL_INT_SAMPLER_1D)
        { GL_INT_SAMPLER_1D, QLatin1String("isampler1D") },
#endif // defined(GL_INT_SAMPLER_1D)
#if defined(GL_INT_SAMPLER_2D)
        { GL_INT_SAMPLER_2D, QLatin1String("isampler2D") },
#endif // defined(GL_INT_SAMPLER_2D)
#if defined(GL_INT_SAMPLER_3D)
        { GL_INT_SAMPLER_3D, QLatin1String("isampler3D") },
#endif // defined(GL_INT_SAMPLER_3D)
#if defined(GL_INT_SAMPLER_CUBE)
        { GL_INT_SAMPLER_CUBE, QLatin1String("isamplerCube") },
#endif // defined(GL_INT_SAMPLER_CUBE)
#if defined(GL_INT_SAMPLER_1D_ARRAY)
        { GL_INT_SAMPLER_1D_ARRAY, QLatin1String("isampler1DArray") },
#endif // defined(GL_INT_SAMPLER_1D_ARRAY)
#if defined(GL_INT_SAMPLER_2D_ARRAY)
        { GL_INT_SAMPLER_2D_ARRAY, QLatin1String("isampler2DArray") },
#endif // defined(GL_INT_SAMPLER_2D_ARRAY)
#if defined(GL_INT_SAMPLER_2D_MULTISAMPLE)
        { GL_INT_SAMPLER_2D_MULTISAMPLE, QLatin1String("isampler2DMS") },
#endif // defined(GL_INT_SAMPLER_2D_MULTISAMPLE)
#if defined(GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY)
        { GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, QLatin1String("isampler2DMSArray") },
#endif // defined(GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY)
#if defined(GL_INT_SAMPLER_BUFFER)
        { GL_INT_SAMPLER_BUFFER, QLatin1String("isamplerBuffer") },
#endif // defined(GL_INT_SAMPLER_BUFFER)
#if defined(GL_INT_SAMPLER_2D_RECT)
        { GL_INT_SAMPLER_2D_RECT, QLatin1String("isampler2DRect") },
#endif // defined(GL_INT_SAMPLER_2D_RECT)
#if defined(GL_UNSIGNED_INT_SAMPLER_1D)
        { GL_UNSIGNED_INT_SAMPLER_1D, QLatin1String("usampler1D") },
#endif // defined(GL_UNSIGNED_INT_SAMPLER_1D)
#if defined(GL_UNSIGNED_INT_SAMPLER_2D)
        { GL_UNSIGNED_INT_SAMPLER_2D, QLatin1String("usampler2D") },
#endif // defined(GL_UNSIGNED_INT_SAMPLER_2D)
#if defined(GL_UNSIGNED_INT_SAMPLER_3D)
        { GL_UNSIGNED_INT_SAMPLER_3D, QLatin1String("usampler3D") },
#endif // defined(GL_UNSIGNED_INT_SAMPLER_3D)
#if defined(GL_UNSIGNED_INT_SAMPLER_CUBE)
        { GL_UNSIGNED_INT_SAMPLER_CUBE, QLatin1String("usamplerCube") },
#endif // defined(GL_UNSIGNED_INT_SAMPLER_CUBE)
#if defined(GL_UNSIGNED_INT_SAMPLER_1D_ARRAY)
        { GL_UNSIGNED_INT_SAMPLER_1D_ARRAY, QLatin1String("usampler2DArray") },
#endif // defined(GL_UNSIGNED_INT_SAMPLER_1D_ARRAY)
#if defined(GL_UNSIGNED_INT_SAMPLER_2D_ARRAY)
        { GL_UNSIGNED_INT_SAMPLER_2D_ARRAY, QLatin1String("usampler2DArray") },
#endif // defined(GL_UNSIGNED_INT_SAMPLER_2D_ARRAY)
#if defined(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE)
        { GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE, QLatin1String("usampler2DMS") },
#endif // defined(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE)
#if defined(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY)
        { GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, QLatin1String("usampler2DMSArray") },
#endif // defined(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY)
#if defined(GL_UNSIGNED_INT_SAMPLER_BUFFER)
        { GL_UNSIGNED_INT_SAMPLER_BUFFER, QLatin1String("usamplerBuffer") },
#endif // defined(GL_UNSIGNED_INT_SAMPLER_BUFFER)
#if defined(GL_UNSIGNED_INT_SAMPLER_2D_RECT)
        { GL_UNSIGNED_INT_SAMPLER_2D_RECT, QLatin1String("usampler2DRect") },
#endif // defined(GL_UNSIGNED_INT_SAMPLER_2D_RECT)
#if defined(GL_IMAGE_1D)
        { GL_IMAGE_1D, QLatin1String("image1D") },
#endif // defined(GL_IMAGE_1D)
#if defined(GL_IMAGE_2D)
        { GL_IMAGE_2D, QLatin1String("image2D") },
#endif // defined(GL_IMAGE_2D)
#if defined(GL_IMAGE_3D)
        { GL_IMAGE_3D, QLatin1String("image3D") },
#endif // defined(GL_IMAGE_3D)
#if defined(GL_IMAGE_2D_RECT)
        { GL_IMAGE_2D_RECT, QLatin1String("image2DRect") },
#endif // defined(GL_IMAGE_2D_RECT)
#if defined(GL_IMAGE_CUBE)
        { GL_IMAGE_CUBE, QLatin1String("imageCube") },
#endif // defined(GL_IMAGE_CUBE)
#if defined(GL_IMAGE_BUFFER)
        { GL_IMAGE_BUFFER, QLatin1String("imageBuffer") },
#endif // defined(GL_IMAGE_BUFFER)
#if defined(GL_IMAGE_1D_ARRAY)
        { GL_IMAGE_1D_ARRAY, QLatin1String("image1DArray") },
#endif // defined(GL_IMAGE_1D_ARRAY)
#if defined(GL_IMAGE_2D_ARRAY)
        { GL_IMAGE_2D_ARRAY, QLatin1String("image2DArray") },
#endif // defined(GL_IMAGE_2D_ARRAY)
#if defined(GL_IMAGE_2D_MULTISAMPLE)
        { GL_IMAGE_2D_MULTISAMPLE, QLatin1String("image2DMS") },
#endif // defined(GL_IMAGE_2D_MULTISAMPLE)
#if defined(GL_IMAGE_2D_MULTISAMPLE_ARRAY)
        { GL_IMAGE_2D_MULTISAMPLE_ARRAY, QLatin1String("image2DMSArray") },
#endif // defined(GL_IMAGE_2D_MULTISAMPLE_ARRAY)
#if defined(GL_INT_IMAGE_1D)
        { GL_INT_IMAGE_1D, QLatin1String("iimage1D") },
#endif // defined(GL_INT_IMAGE_1D)
#if defined(GL_INT_IMAGE_2D)
        { GL_INT_IMAGE_2D, QLatin1String("iimage2D") },
#endif // defined(GL_INT_IMAGE_2D)
#if defined(GL_INT_IMAGE_3D)
        { GL_INT_IMAGE_3D, QLatin1String("iimage3D") },
#endif // defined(GL_INT_IMAGE_3D)
#if defined(GL_INT_IMAGE_2D_RECT)
        { GL_INT_IMAGE_2D_RECT, QLatin1String("iimage2DRect") },
#endif // defined(GL_INT_IMAGE_2D_RECT)
#if defined(GL_INT_IMAGE_CUBE)
        { GL_INT_IMAGE_CUBE, QLatin1String("iimageCube") },
#endif // defined(GL_INT_IMAGE_CUBE)
#if defined(GL_INT_IMAGE_BUFFER)
        { GL_INT_IMAGE_BUFFER, QLatin1String("iimageBuffer") },
#endif // defined(GL_INT_IMAGE_BUFFER)
#if defined(GL_INT_IMAGE_1D_ARRAY)
        { GL_INT_IMAGE_1D_ARRAY, QLatin1String("iimage1DArray") },
#endif // defined(GL_INT_IMAGE_1D_ARRAY)
#if defined(GL_INT_IMAGE_2D_ARRAY)
        { GL_INT_IMAGE_2D_ARRAY, QLatin1String("iimage2DArray") },
#endif // defined(GL_INT_IMAGE_2D_ARRAY)
#if defined(GL_INT_IMAGE_2D_MULTISAMPLE)
        { GL_INT_IMAGE_2D_MULTISAMPLE, QLatin1String("iimage2DMS") },
#endif // defined(GL_INT_IMAGE_2D_MULTISAMPLE)
#if defined(GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY)
        { GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY, QLatin1String("iimage2DMSArray") },
#endif // defined(GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY)
#if defined(GL_UNSIGNED_INT_IMAGE_1D)
        { GL_UNSIGNED_INT_IMAGE_1D, QLatin1String("uimage1D") },
#endif // defined(GL_UNSIGNED_INT_IMAGE_1D)
#if defined(GL_UNSIGNED_INT_IMAGE_2D)
        { GL_UNSIGNED_INT_IMAGE_2D, QLatin1String("uimage2D") },
#endif // defined(GL_UNSIGNED_INT_IMAGE_2D)
#if defined(GL_UNSIGNED_INT_IMAGE_3D)
        { GL_UNSIGNED_INT_IMAGE_3D, QLatin1String("uimage3D") },
#endif // defined(GL_UNSIGNED_INT_IMAGE_3D)
#if defined(GL_UNSIGNED_INT_IMAGE_2D_RECT)
        { GL_UNSIGNED_INT_IMAGE_2D_RECT, QLatin1String("uimage2DRect") },
#endif // defined(GL_UNSIGNED_INT_IMAGE_2D_RECT)
#if defined(GL_UNSIGNED_INT_IMAGE_CUBE)
        { GL_UNSIGNED_INT_IMAGE_CUBE, QLatin1String("uimageCube") },
#endif // defined(GL_UNSIGNED_INT_IMAGE_CUBE)
#if defined(GL_UNSIGNED_INT_IMAGE_BUFFER)
        { GL_UNSIGNED_INT_IMAGE_BUFFER, QLatin1String("uimageBuffer") },
#endif // defined(GL_UNSIGNED_INT_IMAGE_BUFFER)
#if defined(GL_UNSIGNED_INT_IMAGE_1D_ARRAY)
        { GL_UNSIGNED_INT_IMAGE_1D_ARRAY, QLatin1String("uimage1DArray") },
#endif // defined(GL_UNSIGNED_INT_IMAGE_1D_ARRAY)
#if defined(GL_UNSIGNED_INT_IMAGE_2D_ARRAY)
        { GL_UNSIGNED_INT_IMAGE_2D_ARRAY, QLatin1String("uimage2DArray") },
#endif // defined(GL_UNSIGNED_INT_IMAGE_2D_ARRAY)
#if defined(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE)
        { GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE, QLatin1String("uimage2DMS") },
#endif // defined(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE)
#if defined(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY)
        { GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY, QLatin1String("uimage2DMSArray") },
#endif // defined(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY)
#if defined(GL_UNSIGNED_INT_ATOMIC_COUNTER)
        { GL_UNSIGNED_INT_ATOMIC_COUNTER, QLatin1String("atomic_uint") },
#endif // defined(GL_UNSIGNED_INT_ATOMIC_COUNTER)
    };
    const auto glslDataTypesCount = sizeof(glslDataTypes) / sizeof(GlslDataTypeEntry);

    for (auto glslDataTypeIdx = 0u; glslDataTypeIdx < glslDataTypesCount; glslDataTypeIdx++)
    {
        const auto& glslDataType = glslDataTypes[glslDataTypeIdx];
        if (glslDataType.dataType != dataType)
            continue;

        return glslDataType.name;
    }

    return QString(QLatin1String("unknown-%d")).arg(dataType);
}

std::shared_ptr<OsmAnd::GPUAPI_OpenGL::ProgramVariablesLookupContext> OsmAnd::GPUAPI_OpenGL::obtainVariablesLookupContext(
    const GLuint& program,
    const QHash<QString, GlslProgramVariable>& variablesMap)
{
    return std::shared_ptr<ProgramVariablesLookupContext>(new ProgramVariablesLookupContext(this, program, variablesMap));
}

bool OsmAnd::GPUAPI_OpenGL::findVariableLocation(
    const GLuint& program,
    GLint& location,
    const QString& name,
    const GlslVariableType& type)
{
    GL_CHECK_PRESENT(glGetAttribLocation);
    GL_CHECK_PRESENT(glGetUniformLocation);

    GLint resolvedLocation = -1;
    if (type == GlslVariableType::In)
        resolvedLocation = glGetAttribLocation(program, qPrintable(name));
    else if (type == GlslVariableType::Uniform)
        resolvedLocation = glGetUniformLocation(program, qPrintable(name));
    GL_CHECK_RESULT;

    if (resolvedLocation == -1)
        return false;

    location = resolvedLocation;
    return true;
}

bool OsmAnd::GPUAPI_OpenGL::uploadTiledDataToGPU(
    const std::shared_ptr< const IMapTiledDataProvider::Data >& tile,
    std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    if (const auto rasterMapLayerData = std::dynamic_pointer_cast<const IRasterMapLayerProvider::Data>(tile))
    {
        return uploadTiledDataAsTextureToGPU(rasterMapLayerData, resourceInGPU);
    }
    else if (const auto elevationData = std::dynamic_pointer_cast<const IMapElevationDataProvider::Data>(tile))
    {
        if (isSupported_vertexShaderTextureLookup)
        {
            return uploadTiledDataAsTextureToGPU(tile, resourceInGPU);
        }
        else
        {
            return uploadTiledDataAsArrayBufferToGPU(tile, resourceInGPU);
        }
    }

    assert(false);
    return false;
}

bool OsmAnd::GPUAPI_OpenGL::uploadSymbolToGPU(
    const std::shared_ptr< const MapSymbol >& symbol,
    std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    if (const auto rasterMapSymbol = std::dynamic_pointer_cast<const RasterMapSymbol>(symbol))
    {
        return uploadSymbolAsTextureToGPU(rasterMapSymbol, resourceInGPU);
    }
    else if (const auto primitiveMapSymbol = std::dynamic_pointer_cast<const VectorMapSymbol>(symbol))
    {
        return uploadSymbolAsMeshToGPU(primitiveMapSymbol, resourceInGPU);
    }

    assert(false);
    return false;
}

bool OsmAnd::GPUAPI_OpenGL::releaseResourceInGPU(const ResourceInGPU::Type type, const RefInGPU& refInGPU)
{
    switch (type)
    {
        case ResourceInGPU::Type::Texture:
        {
            GL_CHECK_PRESENT(glDeleteTextures);
            GL_CHECK_PRESENT(glIsTexture);

            GLuint texture = static_cast<GLuint>(reinterpret_cast<intptr_t>(refInGPU));
#if OSMAND_DEBUG
            if (!glIsTexture(texture))
            {
                LogPrintf(LogSeverityLevel::Error,
                    "%d is not an OpenGL texture on thread %p",
                    texture,
                    QThread::currentThreadId());
                return false;
            }
            GL_CHECK_RESULT;
#endif
            glDeleteTextures(1, &texture);
            GL_CHECK_RESULT;

            return true;
        }
        case ResourceInGPU::Type::ElementArrayBuffer:
        case ResourceInGPU::Type::ArrayBuffer:
        {
            GL_CHECK_PRESENT(glDeleteBuffers);
            GL_CHECK_PRESENT(glIsBuffer);

            GLuint buffer = static_cast<GLuint>(reinterpret_cast<intptr_t>(refInGPU));
#if OSMAND_DEBUG
            if (!glIsBuffer(buffer))
            {
                LogPrintf(LogSeverityLevel::Error,
                    "%d is not an OpenGL buffer on thread %p",
                    buffer,
                    QThread::currentThreadId());
                return false;
            }
            GL_CHECK_RESULT;
#endif
            glDeleteBuffers(1, &buffer);
            GL_CHECK_RESULT;

            return true;
        }
    }

    return false;
}

bool OsmAnd::GPUAPI_OpenGL::uploadTiledDataAsTextureToGPU(
    const std::shared_ptr< const IMapTiledDataProvider::Data >& tile,
    std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    GL_CHECK_PRESENT(glGenTextures);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glGenerateMipmap);
    GL_CHECK_PRESENT(glTexParameteri);

    // Depending on tile type, determine texture properties:
    auto alphaChannelType = AlphaChannelType::Invalid;
    GLsizei sourcePixelByteSize = 0;
    bool mipmapGenerationSupported = false;
    bool tileUsesPalette = false;
    uint32_t tileSize = 0;
    size_t dataRowLength = 0;
    const void* tileData = nullptr;
    if (const auto rasterMapLayerData = std::dynamic_pointer_cast<const IRasterMapLayerProvider::Data>(tile))
    {
        switch (rasterMapLayerData->bitmap->alphaType())
        {
            case SkAlphaType::kPremul_SkAlphaType:
                alphaChannelType = AlphaChannelType::Premultiplied;
                break;
            case SkAlphaType::kUnpremul_SkAlphaType:
                alphaChannelType = AlphaChannelType::Straight;
                break;
            case SkAlphaType::kOpaque_SkAlphaType:
                alphaChannelType = AlphaChannelType::Opaque;
                break;
            default:
                assert(false);
                return false;
        }

        switch (rasterMapLayerData->bitmap->colorType())
        {
            case SkColorType::kRGBA_8888_SkColorType:
                sourcePixelByteSize = 4;
                break;
            case SkColorType::kARGB_4444_SkColorType:
            case SkColorType::kRGB_565_SkColorType:
                sourcePixelByteSize = 2;
                break;
            case SkColorType::kAlpha_8_SkColorType:
                sourcePixelByteSize = 1;
                tileUsesPalette = true;
                break;
            default:
                assert(false);
                return false;
        }
        tileSize = rasterMapLayerData->bitmap->width();
        dataRowLength = rasterMapLayerData->bitmap->rowBytes();
        tileData = rasterMapLayerData->bitmap->getPixels();

        // No need to generate mipmaps if textureLod is not supported
        mipmapGenerationSupported = isSupported_textureLod;
    }
    else if (const auto elevationData = std::dynamic_pointer_cast<const IMapElevationDataProvider::Data>(tile))
    {
        sourcePixelByteSize = 4;
        tileSize = elevationData->size;
        dataRowLength = elevationData->rowLength;
        tileData = elevationData->pRawData;
        mipmapGenerationSupported = false;
    }
    else
    {
        assert(false);
        return false;
    }
    const auto textureFormat = getTextureFormat(tile);
    const auto sourceFormat = getSourceFormat(tile);

    // Calculate texture size. Tiles are always stored in square textures.
    // Also, since atlas-texture support for tiles was deprecated, only 1 tile per texture is allowed.

    // If tile has NPOT size, then it needs to be rounded-up to nearest POT value
    const auto tileSizePOT = Utilities::getNextPowerOfTwo(tileSize);
    const auto textureSize = (tileSizePOT != tileSize && !isSupported_texturesNPOT) ? tileSizePOT : tileSize;
    const bool useAtlasTexture = (textureSize != tileSize);

    // Get number of mipmap levels
    auto mipmapLevels = 1u;
    if (mipmapGenerationSupported)
        mipmapLevels += qLn(textureSize) / M_LN2;

    // If tile size matches size of texture, upload is quite straightforward
    if (!useAtlasTexture)
    {
        // Create texture id
        GLuint texture;
        glGenTextures(1, &texture);
        GL_CHECK_RESULT;
        assert(texture != 0);

        // Activate texture
        glBindTexture(GL_TEXTURE_2D, texture);
        GL_CHECK_RESULT;

        if (!tileUsesPalette)
        {
            // Allocate square 2D texture
            allocateTexture2D(GL_TEXTURE_2D, mipmapLevels, textureSize, textureSize, textureFormat);

            // Upload data
            uploadDataToTexture2D(GL_TEXTURE_2D, 0,
                0, 0, (GLsizei)tileSize, (GLsizei)tileSize,
                tileData, dataRowLength / sourcePixelByteSize, sourcePixelByteSize,
                sourceFormat);

            // Set maximal mipmap level
            setMipMapLevelsLimit(GL_TEXTURE_2D, mipmapLevels - 1);

            // Generate mipmap levels
            if (mipmapLevels > 1)
            {
                glGenerateMipmap(GL_TEXTURE_2D);
                GL_CHECK_RESULT;
            }
        }
        else
        {
            assert(false);
            /*
            auto bitmapTile = static_cast<IMapBitmapTileProvider::Tile*>(tile.get());

            // Decompress to 32bit color texture
            SkBitmap decompressedTexture;
            bitmapTile->bitmap->deepCopyTo(&decompressedTexture, SkColorType::kRGBA_8888_SkColorType);

            // Generate mipmaps
            decompressedTexture.buildMipMap();
            auto generatedLevels = decompressedTexture.extractMipLevel(nullptr, SK_FixedMax, SK_FixedMax);
            if (mipmapLevels > generatedLevels)
            mipmapLevels = generatedLevels;

            LogPrintf(LogSeverityLevel::Info, "colors = %d", bitmapTile->bitmap->getColorTable()->count());

            // For each mipmap level starting from 0, copy data to packed array
            // and prepare merged palette
            for(auto mipmapLevel = 1; mipmapLevel < mipmapLevels; mipmapLevel++)
            {
            SkBitmap mipmapLevelData;

            auto test22 = decompressedTexture.extractMipLevel(&mipmapLevelData, SK_Fixed1<<mipmapLevel, SK_Fixed1<<mipmapLevel);

            SkBitmap recomressed;
            recomressed.setConfig(SkBitmap::kIndex_8_SkColorType, tileSize >> mipmapLevel, tileSize >> mipmapLevel);
            recomressed.allocPixels();

            LogPrintf(LogSeverityLevel::Info, "\tcolors = %d", recomressed.getColorTable()->count());
            }

            //TEST:
            auto datasize = 256*sizeof(uint32_t) + bitmapTile->bitmap->getSize();
            uint8_t* buf = new uint8_t[datasize];
            for(auto colorIdx = 0; colorIdx < 256; colorIdx++)
            {
            buf[colorIdx*4 + 0] = colorIdx;
            buf[colorIdx*4 + 1] = colorIdx;
            buf[colorIdx*4 + 2] = colorIdx;
            buf[colorIdx*4 + 3] = 0xFF;
            }
            for(auto pixelIdx = 0; pixelIdx < tileSize*tileSize; pixelIdx++)
            buf[256*4 + pixelIdx] = pixelIdx % 256;
            glCompressedTexImage2D(
            GL_TEXTURE_2D, 0, GL_PALETTE8_RGBA8_OES,
            tileSize, tileSize, 0,
            datasize, buf);
            GL_CHECK_RESULT;
            delete[] buf;
            */
            //TODO:
            // 1. convert to full RGBA8
            // 2. generate required amount of mipmap levels
            // 3. glCompressedTexImage2D to load all mipmap levels at once
        }

        // Deselect atlas as active texture
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_RESULT;

        // Create resource-in-GPU descriptor
        resourceInGPU.reset(new TextureInGPU(
            this,
            reinterpret_cast<RefInGPU>(texture),
            textureSize,
            textureSize,
            mipmapLevels,
            alphaChannelType));

        return true;
    }

    // Otherwise, create an atlas texture with 1 slot only:
    //NOTE: No support for palette atlas textures
    assert(!tileUsesPalette);

    // Find proper atlas textures pool by format of texture and full size of tile (including padding)
    AtlasTypeId atlasTypeId;
    atlasTypeId.format = textureFormat;
    atlasTypeId.tileSize = tileSize;
    atlasTypeId.tilePadding = 0;
    const auto atlasTexturesPool = obtainAtlasTexturesPool(atlasTypeId);
    if (!atlasTexturesPool)
        return false;

    // Get free slot from that pool
    const auto slotInGPU = allocateTileInAltasTexture(alphaChannelType, atlasTexturesPool,
        [this, textureSize, mipmapLevels, atlasTexturesPool, textureFormat]
        () -> AtlasTextureInGPU*
        {
            // Allocate texture id
            GLuint texture;
            glGenTextures(1, &texture);
            GL_CHECK_RESULT;
            assert(texture != 0);

            // Select this texture
            glBindTexture(GL_TEXTURE_2D, texture);
            GL_CHECK_RESULT;

            // Allocate space for this texture
            allocateTexture2D(GL_TEXTURE_2D, mipmapLevels, textureSize, textureSize, textureFormat);
            GL_CHECK_RESULT;

            // Set maximal mipmap level
            setMipMapLevelsLimit(GL_TEXTURE_2D, mipmapLevels - 1);

            // Deselect texture
            glBindTexture(GL_TEXTURE_2D, 0);
            GL_CHECK_RESULT;

            return new AtlasTextureInGPU(
                this,
                reinterpret_cast<RefInGPU>(texture),
                textureSize,
                mipmapLevels,
                atlasTexturesPool);
        });

    // Upload tile to allocated slot in atlas texture

    // Select atlas as active texture
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(slotInGPU->atlasTexture->refInGPU)));
    GL_CHECK_RESULT;

    // Upload data
    uploadDataToTexture2D(GL_TEXTURE_2D, 0,
        0, 0, (GLsizei)tileSize, (GLsizei)tileSize,
        tileData, dataRowLength / sourcePixelByteSize, sourcePixelByteSize,
        sourceFormat);
    GL_CHECK_RESULT;

    // Generate mipmap
    if (slotInGPU->atlasTexture->mipmapLevels > 1)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        GL_CHECK_RESULT;
    }

    // Deselect atlas as active texture
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_RESULT;

    resourceInGPU = slotInGPU;

    return true;
}

bool OsmAnd::GPUAPI_OpenGL::uploadTiledDataAsArrayBufferToGPU(
    const std::shared_ptr< const IMapTiledDataProvider::Data >& tile,
    std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);

    const auto elevationData = std::dynamic_pointer_cast<const IMapElevationDataProvider::Data>(tile);
    if (!elevationData)
    {
        assert(false);
        return false;
    }

    // Create array buffer
    GLuint buffer;
    glGenBuffers(1, &buffer);
    GL_CHECK_RESULT;

    // Bind it
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    GL_CHECK_RESULT;

    // Upload data
    const auto itemsCount = elevationData->size*elevationData->size;
    assert(elevationData->size*sizeof(float) == elevationData->rowLength);
    glBufferData(GL_ARRAY_BUFFER, itemsCount*sizeof(float), elevationData->pRawData, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    // Unbind it
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    auto arrayBufferInGPU = new ArrayBufferInGPU(this, reinterpret_cast<RefInGPU>(buffer), itemsCount);
    resourceInGPU.reset(static_cast<ResourceInGPU*>(arrayBufferInGPU));

    return true;
}

bool OsmAnd::GPUAPI_OpenGL::uploadSymbolAsTextureToGPU(
    const std::shared_ptr< const RasterMapSymbol >& symbol,
    std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    GL_CHECK_PRESENT(glGenTextures);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glGenerateMipmap);
    GL_CHECK_PRESENT(glTexParameteri);

    // Determine texture properties:
    auto alphaChannelType = AlphaChannelType::Invalid;
    GLsizei sourcePixelByteSize = 0;
    bool symbolUsesPalette = false;
    switch (symbol->bitmap->alphaType())
    {
        case SkAlphaType::kPremul_SkAlphaType:
            alphaChannelType = AlphaChannelType::Premultiplied;
            break;
        case SkAlphaType::kUnpremul_SkAlphaType:
            alphaChannelType = AlphaChannelType::Straight;
            break;
        case SkAlphaType::kOpaque_SkAlphaType:
            alphaChannelType = AlphaChannelType::Opaque;
            break;
        default:
            assert(false);
            return false;
    }
    switch (symbol->bitmap->colorType())
    {
        case SkColorType::kRGBA_8888_SkColorType:
            sourcePixelByteSize = 4;
            break;
        case SkColorType::kARGB_4444_SkColorType:
        case SkColorType::kRGB_565_SkColorType:
            sourcePixelByteSize = 2;
            break;
        case SkColorType::kAlpha_8_SkColorType:
            sourcePixelByteSize = 1;
            symbolUsesPalette = true;
            break;
        default:
            LogPrintf(LogSeverityLevel::Error,
                "Tried to upload symbol bitmap with unsupported color type %d to GPU",
                symbol->bitmap->colorType());
            assert(false);
            return false;
    }
    const auto textureFormat = getTextureFormat(symbol);

    // Symbols don't use mipmapping, so there is no difference between POT vs NPOT size of texture.
    // In OpenGLES 2.0 and OpenGL 2.0+, NPOT textures are supported in general.
    // OpenGLES 2.0 has some limitations without isSupported_texturesNPOT:
    //  - no mipmaps
    //  - only LINEAR or NEAREST minification filter.

    // Create texture id
    GLuint texture;
    glGenTextures(1, &texture);
    GL_CHECK_RESULT;
    assert(texture != 0);

    // Activate texture
    glBindTexture(GL_TEXTURE_2D, texture);
    GL_CHECK_RESULT;

    if (!symbolUsesPalette)
    {
        // Allocate square 2D texture
        allocateTexture2D(GL_TEXTURE_2D, 1, symbol->bitmap->width(), symbol->bitmap->height(), textureFormat);

        // Upload data
        uploadDataToTexture2D(GL_TEXTURE_2D, 0,
            0, 0, (GLsizei)symbol->bitmap->width(), (GLsizei)symbol->bitmap->height(),
            symbol->bitmap->getPixels(), symbol->bitmap->rowBytes() / sourcePixelByteSize, sourcePixelByteSize,
            getSourceFormat(symbol));

        // Set maximal mipmap level to 0
        setMipMapLevelsLimit(GL_TEXTURE_2D, 0);
    }
    else
    {
        //TODO: palettes are not yet supported
        assert(false);
    }

    // Deselect atlas as active texture
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_RESULT;

    // Create resource-in-GPU descriptor
    resourceInGPU.reset(new TextureInGPU(
        this,
        reinterpret_cast<RefInGPU>(texture),
        symbol->bitmap->width(),
        symbol->bitmap->height(),
        1,
        alphaChannelType));

    return true;
}

bool OsmAnd::GPUAPI_OpenGL::uploadSymbolAsMeshToGPU(
    const std::shared_ptr< const VectorMapSymbol >& symbol,
    std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);

    const auto s = symbol->getVerticesAndIndexes();
    
    // Primitive map symbol has to have vertices, so checks are worthless
    assert(s->vertices && s->verticesCount > 0);

    // Create vertex buffer
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    GL_CHECK_RESULT;

    // Bind it
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    GL_CHECK_RESULT;

    // Upload data
    glBufferData(GL_ARRAY_BUFFER, s->verticesCount*sizeof(VectorMapSymbol::Vertex), s->vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    // Unbind it
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    // Create ArrayBuffer resource
    const std::shared_ptr<ArrayBufferInGPU> vertexBufferResource(new ArrayBufferInGPU(
        this,
        reinterpret_cast<RefInGPU>(vertexBuffer),
        s->verticesCount));

    // Primitive map symbol may have no index buffer, so check if it needs to be created
    std::shared_ptr<ElementArrayBufferInGPU> indexBufferResource;
    if (s->indices != nullptr && s->indicesCount > 0)
    {
        // Create index buffer
        GLuint indexBuffer;
        glGenBuffers(1, &indexBuffer);
        GL_CHECK_RESULT;

        // Bind it
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        GL_CHECK_RESULT;

        // Upload data
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            s->indicesCount*sizeof(VectorMapSymbol::Index),
            s->indices,
            GL_STATIC_DRAW);
        GL_CHECK_RESULT;

        // Unbind it
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        GL_CHECK_RESULT;

        // Create ElementArrayBuffer resource
        indexBufferResource.reset(new ElementArrayBufferInGPU(
            this,
            reinterpret_cast<RefInGPU>(indexBuffer),
            s->indicesCount));
    }

    PointI* position31 = nullptr;
    if (s->position31 != nullptr)
        position31 = new PointI(s->position31->x, s->position31->y);

    // Create mesh resource
    resourceInGPU.reset(new MeshInGPU(this, vertexBufferResource, indexBufferResource, position31));

    return true;
}

void OsmAnd::GPUAPI_OpenGL::waitUntilUploadIsComplete()
{
    // glFinish won't return until all current instructions for GPU (in current context) are complete
    glFinish();
    GL_CHECK_RESULT;
}

void OsmAnd::GPUAPI_OpenGL::pushDebugGroupMarker(const QString& title)
{
    if (isSupported_EXT_debug_marker)
        glPushGroupMarkerEXT_wrapper(0, qPrintable(title));
}

void OsmAnd::GPUAPI_OpenGL::popDebugGroupMarker()
{
    if (isSupported_EXT_debug_marker)
        glPopGroupMarkerEXT_wrapper();
}

OsmAnd::GPUAPI_OpenGL::TextureFormat OsmAnd::GPUAPI_OpenGL::getTextureFormat(
    const std::shared_ptr< const IMapTiledDataProvider::Data >& tile)
{
    if (const auto rasterMapLayerData = std::dynamic_pointer_cast<const IRasterMapLayerProvider::Data>(tile))
    {
        return getTextureFormat(rasterMapLayerData->bitmap->colorType());
    }
    else if (const auto elevationData = std::dynamic_pointer_cast<const IMapElevationDataProvider::Data>(tile))
    {
        assert(isSupported_vertexShaderTextureLookup);

        if (isSupported_texture_storage)
        {
            const auto floatTextureSizedFormat = getTextureSizedFormat_float();
            if (isValidTextureSizedFormat(floatTextureSizedFormat))
                return floatTextureSizedFormat;
        }

        // The only supported format
        const GLenum format = GL_LUMINANCE;
        const GLenum type = GL_UNSIGNED_BYTE;

        assert((format >> 16) == 0);
        assert((type >> 16) == 0);
        return (static_cast<TextureFormat>(format) << 16) | type;
    }

    assert(false);
    return static_cast<TextureFormat>(GL_INVALID_ENUM);
}

OsmAnd::GPUAPI_OpenGL::TextureFormat OsmAnd::GPUAPI_OpenGL::getTextureFormat(
    const std::shared_ptr< const RasterMapSymbol >& symbol)
{
    return getTextureFormat(symbol->bitmap->colorType());
}

OsmAnd::GPUAPI_OpenGL::TextureFormat OsmAnd::GPUAPI_OpenGL::getTextureFormat(const SkColorType skBitmapConfig) const
{
    // If current device supports glTexStorage2D, lets use sized format
    if (isSupported_texture_storage)
    {
        const auto textureSizedFormat = getTextureSizedFormat(skBitmapConfig);
        if (isValidTextureSizedFormat(textureSizedFormat))
            return textureSizedFormat;
    }

    // But if glTexStorage2D is not supported, we need to fallback to pixel type and format specification
    GLenum format = GL_INVALID_ENUM;
    GLenum type = GL_INVALID_ENUM;
    switch (skBitmapConfig)
    {
        case SkColorType::kRGBA_8888_SkColorType:
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            break;

        case SkColorType::kARGB_4444_SkColorType:
            format = GL_RGBA;
            type = GL_UNSIGNED_SHORT_4_4_4_4;
            break;

        case SkColorType::kRGB_565_SkColorType:
            format = GL_RGB;
            type = GL_UNSIGNED_SHORT_5_6_5;
            break;

        default:
            assert(false);
            return static_cast<TextureFormat>(GL_INVALID_ENUM);
    }

    assert(format != GL_INVALID_ENUM);
    assert(type != GL_INVALID_ENUM);
    assert((format >> 16) == 0);
    assert((type >> 16) == 0);
    return (static_cast<TextureFormat>(format) << 16) | type;
}

OsmAnd::GPUAPI_OpenGL::SourceFormat OsmAnd::GPUAPI_OpenGL::getSourceFormat(
    const std::shared_ptr< const IMapTiledDataProvider::Data >& tile)
{
    if (const auto rasterMapLayerData = std::dynamic_pointer_cast<const IRasterMapLayerProvider::Data>(tile))
    {
        return getSourceFormat(rasterMapLayerData->bitmap->colorType());
    }
    else if (const auto elevationData = std::dynamic_pointer_cast<const IMapElevationDataProvider::Data>(tile))
    {
        return getSourceFormat_float();
    }

    SourceFormat sourceFormat;
    sourceFormat.format = GL_INVALID_ENUM;
    sourceFormat.type = GL_INVALID_ENUM;
    return sourceFormat;
}

OsmAnd::GPUAPI_OpenGL::SourceFormat OsmAnd::GPUAPI_OpenGL::getSourceFormat(
    const std::shared_ptr< const RasterMapSymbol >& symbol)
{
    return getSourceFormat(symbol->bitmap->colorType());
}

OsmAnd::GPUAPI_OpenGL::SourceFormat OsmAnd::GPUAPI_OpenGL::getSourceFormat(const SkColorType skBitmapConfig) const
{
    SourceFormat sourceFormat;
    sourceFormat.format = GL_INVALID_ENUM;
    sourceFormat.type = GL_INVALID_ENUM;

    switch (skBitmapConfig)
    {
        case SkColorType::kRGBA_8888_SkColorType:
            sourceFormat.format = GL_RGBA;
            sourceFormat.type = GL_UNSIGNED_BYTE;
            break;
        case SkColorType::kARGB_4444_SkColorType:
            sourceFormat.format = GL_RGBA;
            sourceFormat.type = GL_UNSIGNED_SHORT_4_4_4_4;
            break;
        case SkColorType::kRGB_565_SkColorType:
            sourceFormat.format = GL_RGB;
            sourceFormat.type = GL_UNSIGNED_SHORT_5_6_5;
            break;
    }

    return sourceFormat;
}

void OsmAnd::GPUAPI_OpenGL::allocateTexture2D(
    GLenum target,
    GLsizei levels,
    GLsizei width,
    GLsizei height,
    const TextureFormat encodedFormat)
{
    GLenum format = static_cast<GLenum>(encodedFormat >> 16);
    GLenum type = static_cast<GLenum>(encodedFormat & 0xFFFF);
    GLsizei pixelSizeInBytes = 0;
    if (format == GL_RGBA && type == GL_UNSIGNED_BYTE)
        pixelSizeInBytes = 4;
    else if (format == GL_RGBA && type == GL_UNSIGNED_SHORT_4_4_4_4)
        pixelSizeInBytes = 2;
    else if (format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5)
        pixelSizeInBytes = 2;
    else if (format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE)
        pixelSizeInBytes = 1;

    uint8_t* dummyBuffer = new uint8_t[width * height * pixelSizeInBytes];

    glTexImage2D(target, 0, format, width, height, 0, format, type, dummyBuffer);
    GL_CHECK_RESULT;

    delete[] dummyBuffer;
}

OsmAnd::GLname OsmAnd::GPUAPI_OpenGL::allocateUninitializedVAO()
{
    if (isSupported_vertex_array_object)
    {
        GLname vao;

        glGenVertexArrays_wrapper(1, &vao);
        GL_CHECK_RESULT;
        glBindVertexArray_wrapper(vao);
        GL_CHECK_RESULT;

        return vao;
    }

    // Otherwise simulate VAO
    GLname vao;

    // Check if out-of-free-ids
    if (_vaoSimulationLastUnusedId == std::numeric_limits<GLuint>::max())
    {
        assert(false);
        return vao;
    }
    *vao = (_vaoSimulationLastUnusedId++);

    return vao;
}

void OsmAnd::GPUAPI_OpenGL::initializeVAO(const GLname vao)
{
    if (isSupported_vertex_array_object)
    {
        glBindVertexArray_wrapper(0);
        GL_CHECK_RESULT;

        return;
    }

    // In case VAO simulation is used, capture all vertex attributes and binded buffers
    GL_CHECK_PRESENT(glGetVertexAttribiv);

    SimulatedVAO simulatedVAO;

    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&simulatedVAO.bindedArrayBuffer));
    GL_CHECK_RESULT;

    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&simulatedVAO.bindedElementArrayBuffer));
    GL_CHECK_RESULT;

    for (GLuint vertexAttribIndex = 0; vertexAttribIndex < maxVertexAttribs; vertexAttribIndex++)
    {
        // Check if vertex attribute is enabled
        GLint vertexAttribEnabled = 0;
        glGetVertexAttribiv(vertexAttribIndex, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &vertexAttribEnabled);
        GL_CHECK_RESULT;
        if (vertexAttribEnabled == GL_FALSE)
            continue;

        SimulatedVAO::VertexAttrib vertexAttrib;

        glGetVertexAttribiv(
            vertexAttribIndex,
            GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,
            reinterpret_cast<GLint*>(&vertexAttrib.arrayBufferBinding));
        GL_CHECK_RESULT;

        vertexAttrib.arraySize = 4;
        glGetVertexAttribiv(
            vertexAttribIndex,
            GL_VERTEX_ATTRIB_ARRAY_SIZE,
            reinterpret_cast<GLint*>(&vertexAttrib.arraySize));
        GL_CHECK_RESULT;

        vertexAttrib.arrayStride = 0;
        glGetVertexAttribiv(
            vertexAttribIndex,
            GL_VERTEX_ATTRIB_ARRAY_STRIDE,
            reinterpret_cast<GLint*>(&vertexAttrib.arrayStride));
        GL_CHECK_RESULT;

        vertexAttrib.arrayType = GL_FLOAT;
        glGetVertexAttribiv(
            vertexAttribIndex,
            GL_VERTEX_ATTRIB_ARRAY_TYPE,
            reinterpret_cast<GLint*>(&vertexAttrib.arrayType));
        GL_CHECK_RESULT;

        vertexAttrib.arrayIsNormalized = GL_FALSE;
        glGetVertexAttribiv(
            vertexAttribIndex,
            GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,
            reinterpret_cast<GLint*>(&vertexAttrib.arrayIsNormalized));
        GL_CHECK_RESULT;

        vertexAttrib.arrayPointer = nullptr;
        glGetVertexAttribPointerv(vertexAttribIndex, GL_VERTEX_ATTRIB_ARRAY_POINTER, &vertexAttrib.arrayPointer);
        GL_CHECK_RESULT;

        simulatedVAO.vertexAttribs.insert(vertexAttribIndex, qMove(vertexAttrib));
    }

    assert(!_vaoSimulationObjects.contains(vao));
    _vaoSimulationObjects.insert(vao, simulatedVAO);
}

void OsmAnd::GPUAPI_OpenGL::useVAO(const GLname vao)
{
    if (isSupported_vertex_array_object)
    {
        glBindVertexArray_wrapper(vao);
        GL_CHECK_RESULT;

        return;
    }

    // In case VAO simulation is used, apply all settings from specified simulated VAO
    GL_CHECK_PRESENT(glDisableVertexAttribArray);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    assert(_vaoSimulationObjects.contains(vao));
    const auto& simulatedVAO = constOf(_vaoSimulationObjects)[vao];

    glBindBuffer(GL_ARRAY_BUFFER, simulatedVAO.bindedArrayBuffer);
    GL_CHECK_RESULT;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, simulatedVAO.bindedElementArrayBuffer);
    GL_CHECK_RESULT;

    for (GLuint vertexAttribIndex = 0; vertexAttribIndex < maxVertexAttribs; vertexAttribIndex++)
    {
        const auto itVertexAttrib = simulatedVAO.vertexAttribs.constFind(vertexAttribIndex);
        if (itVertexAttrib == simulatedVAO.vertexAttribs.cend())
        {
            // Disable vertex attrib if no settings captured
            glDisableVertexAttribArray(vertexAttribIndex);
            GL_CHECK_RESULT;

            continue;
        }
        const auto& vertexAttrib = *itVertexAttrib;

        // Apply captured settings
        glEnableVertexAttribArray(vertexAttribIndex);
        GL_CHECK_RESULT;

        glVertexAttribPointer(vertexAttribIndex,
            vertexAttrib.arraySize,
            vertexAttrib.arrayType,
            vertexAttrib.arrayIsNormalized,
            vertexAttrib.arrayStride,
            vertexAttrib.arrayPointer);
        GL_CHECK_RESULT;
    }

    _lastUsedSimulatedVAOObject = vao;
}

void OsmAnd::GPUAPI_OpenGL::unuseVAO()
{
    if (isSupported_vertex_array_object)
    {
        glBindVertexArray_wrapper(0);
        GL_CHECK_RESULT;

        return;
    }

    // In case VAO simulation is used, reset all settings from specified simulated VAO
    GL_CHECK_PRESENT(glDisableVertexAttribArray);

    if (!_lastUsedSimulatedVAOObject.isValid())
        return;
    assert(_vaoSimulationObjects.contains(_lastUsedSimulatedVAOObject));
    const auto& simulatedVAO = constOf(_vaoSimulationObjects)[_lastUsedSimulatedVAOObject];

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    for (const auto itVertexAttribEntry : rangeOf(constOf(simulatedVAO.vertexAttribs)))
    {
        const auto vertexAttribIndex = itVertexAttribEntry.key();

        glDisableVertexAttribArray(vertexAttribIndex);
        GL_CHECK_RESULT;
    }

    _lastUsedSimulatedVAOObject.reset();
}

void OsmAnd::GPUAPI_OpenGL::releaseVAO(const GLname vao, const bool gpuContextLost /*= false*/)
{
    if (isSupported_vertex_array_object)
    {
        if (!gpuContextLost)
        {
            glDeleteVertexArrays_wrapper(1, &vao);
            GL_CHECK_RESULT;
        }

        return;
    }

    // In case VAO simulation is used, remove captured state from stored simulated VAO objects
    assert(_lastUsedSimulatedVAOObject != vao);
    assert(_vaoSimulationObjects.contains(vao));

    _vaoSimulationObjects.remove(vao);
}

OsmAnd::GPUAPI_OpenGL::ProgramVariablesLookupContext::ProgramVariablesLookupContext(
    GPUAPI_OpenGL* const gpuAPI_,
    const GLuint program_,
    const QHash<QString, GlslProgramVariable>& variablesMap_)
    : gpuAPI(gpuAPI_)
    , program(program_)
    , variablesMap(variablesMap_)
{
}

OsmAnd::GPUAPI_OpenGL::ProgramVariablesLookupContext::~ProgramVariablesLookupContext()
{
}

bool OsmAnd::GPUAPI_OpenGL::ProgramVariablesLookupContext::lookupLocation(
    GLint& outLocation,
    const QString& name,
    const GlslVariableType type)
{
    auto& variablesByName = _variablesByName[type];
    const auto itPreviousLocation = variablesByName.constFind(name);
    if (itPreviousLocation != variablesByName.constEnd())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Variable '%s' (%s) was already located in program %d at location %d",
            qPrintable(name),
            type == GlslVariableType::In ? "In" : "Uniform",
            program,
            *itPreviousLocation);
        return false;
    }

    if (!gpuAPI->findVariableLocation(program, outLocation, name, type))
    {
        // In case the variable was not found in the program, this does not necessarily mean that it's not there.
        //WORKAROUND: Some drivers have naming differences with specification:
        // - nVidia (some old drivers)
        // - PowerVR SGX on Android

        // So, before failing completely, perform smart "proper" name resolving
        if (!variablesMap.isEmpty())
        {
            // So try to find misspelled variable name by driver

            //WORKAROUND: 1. 'array_var[10]member_var' instead of 'array_var[10].member_var' - this happens e.g. on '1.8.GOOGLENEXUS.ED945322@2112805'
            {
                const QRegExp regExp(QLatin1String("^") + QRegExp::escape(name).replace("\\.", "\\.?") + QLatin1String("$"));
                for (const auto& variable : constOf(variablesMap))
                {
                    if (variable.type != type || !regExp.exactMatch(variable.name))
                        continue;

                    LogPrintf(LogSeverityLevel::Warning,
                        "Seems like buggy driver. Trying to use '%s' instead of '%s' as variable name (%s)",
                        qPrintable(variable.name),
                        qPrintable(name),
                        type == GlslVariableType::In ? "In" : "Uniform");

                    if (lookupLocation(outLocation, variable.name, type))
                        return true;
                }
            }

            //WORKAROUND: 2. 'struct[0].member[0]' instead of 'struct.member'
            {
                const QRegExp regExp(
                    QLatin1String("^") +
                    QRegExp::escape(name).replace("\\.", "(?:\\[0\\])?\\.") +
                    QLatin1String("(?:\\[0\\])?$"));
                for (const auto& variable : constOf(variablesMap))
                {
                    if (variable.type != type || !regExp.exactMatch(variable.name))
                        continue;

                    LogPrintf(LogSeverityLevel::Warning,
                        "Seems like buggy driver. Trying to use '%s' instead of '%s' as variable name (%s)",
                        qPrintable(variable.name),
                        qPrintable(name),
                        type == GlslVariableType::In ? "In" : "Uniform");

                    if (lookupLocation(outLocation, variable.name, type))
                        return true;
                }
            }
        }

        LogPrintf(LogSeverityLevel::Error,
            "Variable '%s' (%s) was not found in GLSL program %d",
            qPrintable(name),
            type == GlslVariableType::In ? "In" : "Uniform",
            program);
        return false;
    }

    auto& variablesByLocation = _variablesByLocation[type];
    const auto itOtherName = variablesByLocation.constFind(outLocation);
    if (itOtherName != variablesByLocation.constEnd())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Variable '%s' (%s) in program %d shares same location at other variable '%s'",
            qPrintable(name),
            type == GlslVariableType::In ? "In" : "Uniform",
            program,
            qPrintable(*itOtherName));
        return false;
    }

    return true;
}

bool OsmAnd::GPUAPI_OpenGL::ProgramVariablesLookupContext::lookupLocation(
    GLlocation& outLocation,
    const QString& name,
    const GlslVariableType type)
{
    return lookupLocation(*outLocation, name, type);
}

bool OsmAnd::GPUAPI_OpenGL::ProgramVariablesLookupContext::lookupInLocation(GLlocation& outLocation, const QString& name)
{
    return lookupLocation(outLocation, name, GlslVariableType::In);
}

bool OsmAnd::GPUAPI_OpenGL::ProgramVariablesLookupContext::lookupUniformLocation(GLlocation& outLocation, const QString& name)
{
    return lookupLocation(outLocation, name, GlslVariableType::Uniform);
}
