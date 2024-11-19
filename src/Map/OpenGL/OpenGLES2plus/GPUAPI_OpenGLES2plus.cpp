#include "GPUAPI_OpenGLES2plus.h"

#include <cassert>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QRegExp>
#include <QStringList>
#include "restore_internal_warnings.h"

#include "MapCommonTypes.h"
#include "IRasterMapLayerProvider.h"
#include "MapSymbol.h"
#include "RasterMapSymbol.h"
#include "Logging.h"

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

#ifndef GL_LUMINANCE8_EXT
#   define GL_LUMINANCE8_EXT                                            0x8040
#endif //!GL_LUMINANCE8_EXT

#ifndef GL_RGB565_OES
#   if defined(GL_RGB565)
#       define GL_RGB565_OES GL_RGB565
#   else
#       define GL_RGB565_OES                                            0x8D62
#   endif
#endif //!GL_RGB565_OES

#ifndef GL_RGBA8_EXT
#   if defined(GL_RGBA8)
#       define GL_RGBA8_EXT GL_RGBA8
#   else
#       define GL_RGBA8_EXT                                             0x8058
#   endif
#endif //!GL_RGBA8_EXT

#ifndef GL_R8_EXT
#   define GL_R8_EXT                                                    0x8229
#endif //!GL_R8_EXT

#ifndef GL_R32F_EXT
#   define GL_R32F_EXT                                                  0x822E
#endif //!GL_R32F_EXT

#ifndef GL_RED_EXT
#   define GL_RED_EXT                                                   0x1903
#endif //!GL_RED_EXT

#ifndef GL_UNPACK_ROW_LENGTH
#    define GL_UNPACK_ROW_LENGTH                                        0x0CF2
#endif // !GL_UNPACK_ROW_LENGTH

#ifndef GL_UNPACK_SKIP_ROWS
#    define GL_UNPACK_SKIP_ROWS                                         0x0CF3
#endif // !GL_UNPACK_SKIP_ROWS

#ifndef GL_UNPACK_SKIP_PIXELS
#   define GL_UNPACK_SKIP_PIXELS                                        0x0CF4
#endif // !GL_UNPACK_SKIP_PIXELS

#ifndef GL_PALETTE8_RGBA8_OES
#   define GL_PALETTE8_RGBA8_OES                                        0x8B96
#endif // !GL_PALETTE8_RGBA8_OES

#if !defined(OSMAND_TARGET_OS_ios)
OsmAnd::GPUAPI_OpenGLES2plus::P_glTexStorage2DEXT_PROC OsmAnd::GPUAPI_OpenGLES2plus::glTexStorage2DEXT = nullptr;

PFNGLBINDVERTEXARRAYOESPROC OsmAnd::GPUAPI_OpenGLES2plus::glBindVertexArrayOES = nullptr;
PFNGLDELETEVERTEXARRAYSOESPROC OsmAnd::GPUAPI_OpenGLES2plus::glDeleteVertexArraysOES = nullptr;
PFNGLGENVERTEXARRAYSOESPROC OsmAnd::GPUAPI_OpenGLES2plus::glGenVertexArraysOES = nullptr;

OsmAnd::GPUAPI_OpenGLES2plus::PFNGLPOPGROUPMARKEREXTPROC OsmAnd::GPUAPI_OpenGLES2plus::glPopGroupMarkerEXT = nullptr;
OsmAnd::GPUAPI_OpenGLES2plus::PFNGLPUSHGROUPMARKEREXTPROC OsmAnd::GPUAPI_OpenGLES2plus::glPushGroupMarkerEXT = nullptr;
OsmAnd::GPUAPI_OpenGLES2plus::PFNGLLABELOBJECTEXTPROC OsmAnd::GPUAPI_OpenGLES2plus::glLabelObjectEXT = nullptr;
#endif //!OSMAND_TARGET_OS_ios

OsmAnd::GPUAPI_OpenGLES2plus::GPUAPI_OpenGLES2plus()
    : _isSupported_EXT_unpack_subimage(false)
    , _isSupported_EXT_texture_storage(false)
    , _isSupported_APPLE_texture_max_level(false)
    , _isSupported_OES_vertex_array_object(false)
    , _isSupported_OES_rgb8_rgba8(false)
    , _isSupported_ARM_rgba8(false)
    , _isSupported_EXT_texture(false)
    , _isSupported_OES_texture_float(false)
    , _isSupported_OES_texture_half_float(false)
    , _isSupported_EXT_texture_rg(false)
    , _isSupported_EXT_shader_texture_lod(false)
    , _isSupported_EXT_debug_marker(false)
    , _isSupported_EXT_debug_label(false)
    , _isSupported_APPLE_sync(false)
    , _isSupported_samplerObjects(false)
    , _isSupported_OES_get_program_binary(false)
    , isSupported_EXT_unpack_subimage(_isSupported_EXT_unpack_subimage)
    , isSupported_EXT_texture_storage(_isSupported_EXT_texture_storage)
    , isSupported_APPLE_texture_max_level(_isSupported_APPLE_texture_max_level)
    , isSupported_OES_vertex_array_object(_isSupported_OES_vertex_array_object)
    , isSupported_OES_rgb8_rgba8(_isSupported_OES_rgb8_rgba8)
    , isSupported_ARM_rgba8(_isSupported_ARM_rgba8)
    , isSupported_EXT_texture(_isSupported_EXT_texture)
    , isSupported_OES_texture_float(_isSupported_OES_texture_float)
    , isSupported_OES_texture_half_float(_isSupported_OES_texture_half_float)
    , isSupported_EXT_texture_rg(_isSupported_EXT_texture_rg)
    , isSupported_EXT_shader_texture_lod(_isSupported_EXT_shader_texture_lod)
    , isSupported_EXT_debug_marker(_isSupported_EXT_debug_marker)
    , isSupported_EXT_debug_label(_isSupported_EXT_debug_label)
    , isSupported_APPLE_sync(_isSupported_APPLE_sync)
    , isSupported_samplerObjects(_isSupported_samplerObjects)
    , isSupported_OES_get_program_binary(_isSupported_OES_get_program_binary)
    , supportedVertexShaderPrecisionFormats(_supportedVertexShaderPrecisionFormats)
    , supportedFragmentShaderPrecisionFormats(_supportedFragmentShaderPrecisionFormats)
{
}

OsmAnd::GPUAPI_OpenGLES2plus::~GPUAPI_OpenGLES2plus()
{
}

GLenum OsmAnd::GPUAPI_OpenGLES2plus::validateResult(const char* const function, const char* const file, const int line)
{
    auto result = glGetError();
    if (result == GL_NO_ERROR)
        return result;

    const char* errorString = nullptr;
    switch (result)
    {
        case GL_NO_ERROR:
            errorString = "no error";
            break;
        case GL_INVALID_ENUM:
            errorString = "invalid enumerant";
            break;
        case GL_INVALID_VALUE:
            errorString = "invalid value";
            break;
        case GL_INVALID_OPERATION:
            errorString = "invalid operation";
            break;
        case GL_OUT_OF_MEMORY:
            errorString = "out of memory";
            break;
        default:
            errorString = "(unknown)";
            break;
    }
    LogPrintf(LogSeverityLevel::Error,
        "OpenGL error 0x%08x (%s) in %s at %s:%d",
        result,
        errorString,
        function,
        file,
        line);

    return result;
}

bool OsmAnd::GPUAPI_OpenGLES2plus::isShaderPrecisionFormatSupported(GLenum shaderType, GLenum precisionType)
{
    GL_CHECK_PRESENT(glGetShaderPrecisionFormat);

    GLint range[] = { 0, 0 };
    GLint precision = 0;
    glGetShaderPrecisionFormat(shaderType, precisionType, range, &precision);
    GL_CHECK_RESULT;

    if (range[0] == 0 && range[1] == 0)
        return false;

    return true;
}

bool OsmAnd::GPUAPI_OpenGLES2plus::initialize()
{
    GL_CHECK_PRESENT(glGenFramebuffers);

    bool ok;

    ok = GPUAPI_OpenGL::initialize();
    if (!ok)
        return false;

    const auto glVersionString = glGetString(GL_VERSION); // Format: "OpenGL<space>ES<space><version number><space><vendor-specific information>"
    GL_CHECK_RESULT;
    QRegExp glVersionRegExp(QLatin1String("(\\d+).(\\d+)"));
    glVersionRegExp.indexIn(QString(QLatin1String(reinterpret_cast<const char*>(glVersionString))));
    _glVersion = glVersionRegExp.cap(1).toUInt() * 10 + glVersionRegExp.cap(2).toUInt();
    LogPrintf(LogSeverityLevel::Info, "OpenGLES version %d [%s]", glVersion, glVersionString);
    if (glVersion < 20)
    {
        LogPrintf(LogSeverityLevel::Info, "This OpenGLES version is not supported");
        return false;
    }

    const auto glslVersionString = glGetString(GL_SHADING_LANGUAGE_VERSION); // Format: "OpenGL<space>ES<space>GLSL<space>ES<space><version number><space><vendor-specific information>"
    GL_CHECK_RESULT;
    QRegExp glslVersionRegExp(QLatin1String("(\\d+).(\\d+)"));
    glslVersionRegExp.indexIn(QString(QLatin1String(reinterpret_cast<const char*>(glslVersionString))));
    _glslVersion = glslVersionRegExp.cap(1).toUInt() * 100;
    const auto minorVersion = glslVersionRegExp.cap(2).toUInt();
    if (minorVersion < 10)
        _glslVersion += minorVersion * 10;
    else
        _glslVersion += minorVersion;
    LogPrintf(LogSeverityLevel::Info, "GLSL version %d [%s]", _glslVersion, glslVersionString);

    const auto& extensionsString = QString::fromLatin1(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
    GL_CHECK_RESULT;
    _extensions = extensionsString.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    LogPrintf(LogSeverityLevel::Info, "OpenGLES extensions: %s", qPrintable(extensions.join(' ')));

    GLboolean shaderCompilerPresent = GL_FALSE;
    glGetBooleanv(GL_SHADER_COMPILER, &shaderCompilerPresent);
    if (shaderCompilerPresent == GL_FALSE)
    {
        LogPrintf(LogSeverityLevel::Error, "OpenGLES shader compiler not supported");
        return false;
    }

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&_maxTextureSize));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES maximal texture size %dx%d", _maxTextureSize, _maxTextureSize);

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, reinterpret_cast<GLint*>(&_maxTextureUnitsInFragmentShader));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES maximal texture units in fragment shader %d", _maxTextureUnitsInFragmentShader);

    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, reinterpret_cast<GLint*>(&_maxTextureUnitsCombined));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES maximal texture units combined %d", _maxTextureUnitsCombined);

    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &_maxVertexUniformVectors);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES maximal 4-component parameters in vertex shader %d", _maxVertexUniformVectors);

    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &_maxFragmentUniformVectors);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES maximal 4-component parameters in fragment shader %d", _maxFragmentUniformVectors);

    glGetIntegerv(GL_MAX_VARYING_VECTORS, &_maxVaryingVectors);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal varying vectors %d", _maxVaryingVectors);
    _maxVaryingFloats = _maxVaryingVectors * 4;

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &_maxVertexAttribs);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES maximal vertex attributes %d", _maxVertexAttribs);

    _isSupported_OES_vertex_array_object = extensions.contains("GL_OES_vertex_array_object");
    _isSupported_vertex_array_object = (glVersion >= 30) || _isSupported_OES_vertex_array_object;

    _isSupported_OES_rgb8_rgba8 = extensions.contains("GL_OES_rgb8_rgba8");
    _isSupported_ARM_rgba8 = extensions.contains("GL_ARM_rgba8");
    _isSupported_EXT_texture = extensions.contains("GL_EXT_texture");
    _isSupported_OES_texture_float = extensions.contains("GL_OES_texture_float");
    _isSupported_OES_texture_half_float = extensions.contains("GL_OES_texture_half_float");
    _isSupported_EXT_shader_texture_lod = extensions.contains("GL_EXT_shader_texture_lod");
    _isSupported_EXT_texture_rg = extensions.contains("GL_EXT_texture_rg");
    _isSupported_EXT_unpack_subimage = extensions.contains("GL_EXT_unpack_subimage");
    _isSupported_EXT_texture_storage = extensions.contains("GL_EXT_texture_storage");
    _isSupported_APPLE_texture_max_level = extensions.contains("GL_APPLE_texture_max_level");
    _isSupported_texturesNPOT = (glVersion >= 30) || extensions.contains("GL_OES_texture_npot");
    _isSupported_EXT_debug_marker = extensions.contains("GL_EXT_debug_marker");
    _isSupported_debug_marker = _isSupported_EXT_debug_marker;
    _isSupported_EXT_debug_label = extensions.contains("GL_EXT_debug_label");
    _isSupported_debug_label = _isSupported_EXT_debug_label;
    _isSupported_APPLE_sync = extensions.contains("GL_APPLE_sync");
    _isSupported_sync = (glVersion >= 30 || _isSupported_APPLE_sync);
    _isSupported_OES_get_program_binary = extensions.contains("GL_OES_get_program_binary");

    if (isShaderPrecisionFormatSupported(GL_VERTEX_SHADER, GL_LOW_FLOAT))
        _supportedVertexShaderPrecisionFormats.insert(GL_LOW_FLOAT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES vertex shader does not support 'GL_LOW_FLOAT' precision format");

    if (isShaderPrecisionFormatSupported(GL_VERTEX_SHADER, GL_MEDIUM_FLOAT))
        _supportedVertexShaderPrecisionFormats.insert(GL_MEDIUM_FLOAT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES vertex shader does not support 'GL_MEDIUM_FLOAT' precision format");

    if (isShaderPrecisionFormatSupported(GL_VERTEX_SHADER, GL_HIGH_FLOAT))
        _supportedVertexShaderPrecisionFormats.insert(GL_HIGH_FLOAT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES vertex shader does not support 'GL_HIGH_FLOAT' precision format");

    if (isShaderPrecisionFormatSupported(GL_VERTEX_SHADER, GL_LOW_INT))
        _supportedVertexShaderPrecisionFormats.insert(GL_LOW_INT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES vertex shader does not support 'GL_LOW_INT' precision format");

    if (isShaderPrecisionFormatSupported(GL_VERTEX_SHADER, GL_MEDIUM_INT))
        _supportedVertexShaderPrecisionFormats.insert(GL_MEDIUM_INT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES vertex shader does not support 'GL_MEDIUM_INT' precision format");

    if (isShaderPrecisionFormatSupported(GL_VERTEX_SHADER, GL_HIGH_INT))
        _supportedVertexShaderPrecisionFormats.insert(GL_HIGH_INT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES vertex shader does not support 'GL_HIGH_INT' precision format");

    if (isShaderPrecisionFormatSupported(GL_FRAGMENT_SHADER, GL_LOW_FLOAT))
        _supportedFragmentShaderPrecisionFormats.insert(GL_LOW_FLOAT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES fragment shader does not support 'GL_LOW_FLOAT' precision format");

    if (isShaderPrecisionFormatSupported(GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT))
        _supportedFragmentShaderPrecisionFormats.insert(GL_MEDIUM_FLOAT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES fragment shader does not support 'GL_MEDIUM_FLOAT' precision format");

    if (isShaderPrecisionFormatSupported(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT))
        _supportedFragmentShaderPrecisionFormats.insert(GL_HIGH_FLOAT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES fragment shader does not support 'GL_HIGH_FLOAT' precision format");

    if (isShaderPrecisionFormatSupported(GL_FRAGMENT_SHADER, GL_LOW_INT))
        _supportedFragmentShaderPrecisionFormats.insert(GL_LOW_INT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES fragment shader does not support 'GL_LOW_INT' precision format");

    if (isShaderPrecisionFormatSupported(GL_FRAGMENT_SHADER, GL_MEDIUM_INT))
        _supportedFragmentShaderPrecisionFormats.insert(GL_MEDIUM_INT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES fragment shader does not support 'GL_MEDIUM_INT' precision format");

    if (isShaderPrecisionFormatSupported(GL_FRAGMENT_SHADER, GL_HIGH_INT))
        _supportedFragmentShaderPrecisionFormats.insert(GL_HIGH_INT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES fragment shader does not support 'GL_HIGH_INT' precision format");

#if !defined(OSMAND_TARGET_OS_ios)
    if (_isSupported_EXT_debug_marker && !glPopGroupMarkerEXT)
    {
        glPopGroupMarkerEXT = reinterpret_cast<PFNGLPOPGROUPMARKEREXTPROC>(eglGetProcAddress("glPopGroupMarkerEXT"));
        if (glPopGroupMarkerEXT == nullptr)
        {
            LogPrintf(LogSeverityLevel::Warning, "Seems like buggy driver. EXT_debug_marker is present, but glPopGroupMarkerEXT() was not found. This extension will be disabled");
            _isSupported_EXT_debug_marker = false;
        }
    }
    if (_isSupported_EXT_debug_marker && !glPushGroupMarkerEXT)
    {
        glPushGroupMarkerEXT = reinterpret_cast<PFNGLPUSHGROUPMARKEREXTPROC>(eglGetProcAddress("glPushGroupMarkerEXT"));
        if (glPushGroupMarkerEXT == nullptr)
        {
            LogPrintf(LogSeverityLevel::Warning, "Seems like buggy driver. EXT_debug_marker is present, but glPushGroupMarkerEXT() was not found. This extension will be disabled");
            _isSupported_EXT_debug_marker = false;
        }
    }
    if (_isSupported_EXT_debug_label && !glLabelObjectEXT)
    {
        glLabelObjectEXT = reinterpret_cast<PFNGLLABELOBJECTEXTPROC>(eglGetProcAddress("glLabelObjectEXT"));
        if (glLabelObjectEXT == nullptr)
        {
            LogPrintf(LogSeverityLevel::Warning, "Seems like buggy driver. EXT_debug_label is present, but glLabelObjectEXT() was not found. This extension will be disabled");
            _isSupported_EXT_debug_label = false;
        }
    }
    if (_isSupported_EXT_texture_storage && !glTexStorage2DEXT)
    {
        glTexStorage2DEXT = reinterpret_cast<P_glTexStorage2DEXT_PROC>(eglGetProcAddress("glTexStorage2DEXT"));
        if (glTexStorage2DEXT == nullptr)
        {
            LogPrintf(LogSeverityLevel::Warning, "Seems like buggy driver. EXT_texture_storage is present, but glTexStorage2DEXT() was not found. This extension will be disabled");
            _isSupported_EXT_texture_storage = false;
        }
    }
    if (_isSupported_OES_vertex_array_object && !glBindVertexArrayOES)
    {
        glBindVertexArrayOES = reinterpret_cast<PFNGLBINDVERTEXARRAYOESPROC>(eglGetProcAddress("glBindVertexArrayOES"));
        if (glBindVertexArrayOES == nullptr)
        {
            LogPrintf(LogSeverityLevel::Warning, "Seems like buggy driver. OES_vertex_array_object is present, but glBindVertexArrayOES() was not found. This extension will be disabled");
            _isSupported_OES_vertex_array_object = false;
        }
    }
    if (_isSupported_OES_vertex_array_object && !glDeleteVertexArraysOES)
    {
        glDeleteVertexArraysOES = reinterpret_cast<PFNGLDELETEVERTEXARRAYSOESPROC>(eglGetProcAddress("glDeleteVertexArraysOES"));
        if (glDeleteVertexArraysOES == nullptr)
        {
            LogPrintf(LogSeverityLevel::Warning, "Seems like buggy driver. OES_vertex_array_object is present, but glDeleteVertexArraysOES() was not found. This extension will be disabled");
            _isSupported_OES_vertex_array_object = false;
        }
    }
    if (_isSupported_OES_vertex_array_object && !glGenVertexArraysOES)
    {
        glGenVertexArraysOES = reinterpret_cast<PFNGLGENVERTEXARRAYSOESPROC>(eglGetProcAddress("glGenVertexArraysOES"));
        if (glGenVertexArraysOES == nullptr)
        {
            LogPrintf(LogSeverityLevel::Warning, "Seems like buggy driver. OES_vertex_array_object is present, but glGenVertexArraysOES() was not found. This extension will be disabled");
            _isSupported_OES_vertex_array_object = false;
        }
    }
#endif // !OSMAND_TARGET_OS_ios

    GLint compressedFormatsLength = 0;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &compressedFormatsLength);
    GL_CHECK_RESULT;
    _compressedFormats.resize(compressedFormatsLength);
    if (compressedFormatsLength > 0)
    {
        glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, _compressedFormats.data());
        GL_CHECK_RESULT;
    }

    _isSupported_samplerObjects = (glVersion >= 30);
    _isSupported_texture_float = (glVersion >= 30) || _isSupported_OES_texture_float;
    _isSupported_texture_half_float = (glVersion >= 30) || _isSupported_OES_texture_half_float;
    _isSupported_textureLod = (glslVersion >= 130) || _isSupported_EXT_shader_texture_lod;
    _isSupported_texture_rg = (glVersion >= 30) || _isSupported_EXT_texture_rg;
    _isSupported_texture_storage = (glVersion >= 30) || _isSupported_EXT_texture_storage;
    _isSupported_integerOperations = (glslVersion >= 130);

    if (glVersion >= 30 || _isSupported_OES_get_program_binary)
    {
        GLint numProgramBinaryFormats = 0;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numProgramBinaryFormats);
        GL_CHECK_RESULT;
        _isSupported_program_binary = (numProgramBinaryFormats > 0);
    }
    else
        _isSupported_program_binary = false;

    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    GL_CHECK_RESULT;

    if (isSupported_samplerObjects)
    {
        GL_CHECK_PRESENT(glGenSamplers);
        GL_CHECK_PRESENT(glSamplerParameteri);
        GL_CHECK_PRESENT(glSamplerParameterf);

        // Allocate samplers
        glGenSamplers(SamplerTypesCount, _textureSamplers.data());
        GL_CHECK_RESULT;
        GLuint sampler;

        // ElevationDataTile sampler
        sampler = _textureSamplers[static_cast<int>(SamplerType::ElevationDataTile)];
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GL_CHECK_RESULT;

        // BitmapTile_Bilinear sampler
        sampler = _textureSamplers[static_cast<int>(SamplerType::BitmapTile_Bilinear)];
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;

        // BitmapTile_BilinearMipmap sampler
        sampler = _textureSamplers[static_cast<int>(SamplerType::BitmapTile_BilinearMipmap)];
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;

        // BitmapTile_TrilinearMipmap sampler
        sampler = _textureSamplers[static_cast<int>(SamplerType::BitmapTile_TrilinearMipmap)];
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;

        // Symbol sampler
        sampler = _textureSamplers[static_cast<int>(SamplerType::Symbol)];
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;

        // Depth buffer sampler
        sampler = _textureSamplers[static_cast<int>(SamplerType::DepthBuffer)];
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GL_CHECK_RESULT;
    }

    return true;
}

int OsmAnd::GPUAPI_OpenGLES2plus::checkElementVisibility(int queryIndex, float pointSize)
{
    const auto prevSize = _pointVisibilityCheckQueries.size();
    if (prevSize <= queryIndex)
    {
        const auto nextSize = queryIndex + 10;

        LogPrintf(LogSeverityLevel::Info, "OpenGLES number of queries used will be increased to %d", nextSize);

        if (nextSize > 128)
            return -1;
        _pointVisibilityCheckQueries.resize(nextSize);
        glGenQueries(nextSize - prevSize, _pointVisibilityCheckQueries.data() + prevSize);
        if (glGetError() != GL_NO_ERROR)
            return -1;
    }

    glBeginQuery(GL_ANY_SAMPLES_PASSED, _pointVisibilityCheckQueries[queryIndex]);
    if (glGetError() != GL_NO_ERROR)
        return -1;

    glDrawArrays(GL_POINTS, 0, 4);
    GL_CHECK_RESULT;

    glEndQuery(GL_ANY_SAMPLES_PASSED);
    if (glGetError() != GL_NO_ERROR)
        return -1;

    return queryIndex;
}

bool OsmAnd::GPUAPI_OpenGLES2plus::elementIsVisible(int queryIndex)
{
    assert(_pointVisibilityCheckQueries.size() > queryIndex);

    GLuint queryResult = GL_FALSE;
    
    const auto start = std::chrono::high_resolution_clock::now();

    glGetQueryObjectuiv(_pointVisibilityCheckQueries[queryIndex], GL_QUERY_RESULT, &queryResult);

    const int64_t period = (std::chrono::high_resolution_clock::now() - start).count() / 1000;
    waitTimeInMicroseconds.fetchAndAddOrdered(static_cast<int>(std::min(period, static_cast<int64_t>(INT32_MAX))));

    GL_CHECK_RESULT;

    return queryResult == GL_FALSE;
}

bool OsmAnd::GPUAPI_OpenGLES2plus::release(bool gpuContextLost)
{
    if (!gpuContextLost && _pointVisibilityCheckQueries.size() > 0)
    {
        GL_CHECK_PRESENT(glDeleteQueries);
        glDeleteQueries(_pointVisibilityCheckQueries.size(), _pointVisibilityCheckQueries.data());
        GL_CHECK_RESULT;
    }
    _pointVisibilityCheckQueries.resize(0);

    if (isSupported_samplerObjects)
    {
        GL_CHECK_PRESENT(glDeleteSamplers);
        if (_textureSamplers[0] != 0)
        {
            if (!gpuContextLost)
            {
                glDeleteSamplers(SamplerTypesCount, _textureSamplers.data());
                GL_CHECK_RESULT;
            }
            _textureSamplers.fill(0);
        }
    }

    bool ok = GPUAPI_OpenGL::release(gpuContextLost);
    if (!ok)
        return false;

    return true;
}

OsmAnd::GPUAPI_OpenGLES2plus::TextureFormat OsmAnd::GPUAPI_OpenGLES2plus::getTextureFormat(SkColorType colorType) const
{
    if (isSupported_texture_storage)
    {
        GLenum internalFormat = GL_INVALID_ENUM;
        switch (colorType)
        {
            case SkColorType::kRGBA_8888_SkColorType:
                if (glVersion >= 30)
                    internalFormat = GL_RGBA8;
                else if (isSupported_ARM_rgba8 || isSupported_OES_rgb8_rgba8)
                    internalFormat = GL_RGBA8_OES;
                else if (isSupported_EXT_texture)
                    internalFormat = GL_RGBA8_EXT;
                break;

            case SkColorType::kARGB_4444_SkColorType:
                internalFormat = GL_RGBA4;
                break;

            case SkColorType::kRGB_565_SkColorType:
                internalFormat = GL_RGB565;
                break;

            default:
                assert(false);
        }
        return TextureFormat::Make(GL_INVALID_ENUM, internalFormat);
    }

    return GPUAPI_OpenGL::getTextureFormat(colorType);
}

OsmAnd::GPUAPI_OpenGLES2plus::TextureFormat OsmAnd::GPUAPI_OpenGLES2plus::getTextureFormat_float() const
{
    if (isSupported_texture_storage)
    {
        GLenum internalFormat = isSupported_texture_rg ? GL_R8_EXT : GL_LUMINANCE8_EXT;
        if (isSupported_texture_float)
            internalFormat = isSupported_texture_rg ? GL_R32F_EXT : GL_LUMINANCE32F_EXT;
        else if (isSupported_texture_half_float)
            internalFormat = isSupported_texture_rg ? GL_R16F_EXT : GL_LUMINANCE16F_EXT;
        return TextureFormat::Make(GL_INVALID_ENUM, internalFormat);
    }

    GLenum format = GL_INVALID_ENUM;
    GLenum type = GL_INVALID_ENUM;

    if (isSupported_texture_float)
    {
        format = isSupported_texture_rg ? GL_RED_EXT : GL_LUMINANCE;
        type = GL_FLOAT;
    }
    else if (isSupported_texture_half_float)
    {
        format = isSupported_texture_rg ? GL_RED_EXT : GL_LUMINANCE;
        type = GL_HALF_FLOAT_OES;
    }
    else
    {
        format = GL_LUMINANCE;
        type = GL_UNSIGNED_BYTE;
    }

    return TextureFormat::Make(type, format);
}

size_t OsmAnd::GPUAPI_OpenGLES2plus::getTextureFormatPixelSize(TextureFormat textureFormat) const
{
    auto format = static_cast<GLenum>(textureFormat.format);
    auto type = static_cast<GLenum>(textureFormat.type);
    if ((format == GL_RED_EXT || format == GL_LUMINANCE) && type == GL_FLOAT)
        return 4;
    else if ((format == GL_RED_EXT || format == GL_LUMINANCE) && type == GL_HALF_FLOAT_OES)
        return 2;

    return GPUAPI_OpenGL::getTextureFormatPixelSize(textureFormat);
}

OsmAnd::GPUAPI_OpenGLES2plus::SourceFormat OsmAnd::GPUAPI_OpenGLES2plus::getSourceFormat_float() const
{
    GLenum format = GL_INVALID_ENUM;
    GLenum type = GL_INVALID_ENUM;

    if (isSupported_texture_rg)
        format = GL_RED_EXT;
    else
        format = GL_LUMINANCE;

    // NOTE: Regardless of float support, source is float
    type = GL_FLOAT;

    return SourceFormat::Make(type, format);
}

void OsmAnd::GPUAPI_OpenGLES2plus::allocateTexture2D(
    GLenum target,
    GLsizei levels,
    GLsizei width,
    GLsizei height,
    TextureFormat textureFormat)
{
    if (glVersion >= 30)
    {
        GL_CHECK_PRESENT(glTexStorage2D);

        glTexStorage2D(target, levels, static_cast<GLenum>(textureFormat.format), width, height);
        GL_CHECK_RESULT;
        return;
    }
    else if (isSupported_EXT_texture_storage)
    {
        GL_CHECK_PRESENT(glTexStorage2DEXT);

        glTexStorage2DEXT(target, levels, static_cast<GLenum>(textureFormat.format), width, height);
        GL_CHECK_RESULT;
        return;
    }

    // Fallback to dumb allocation
    GPUAPI_OpenGL::allocateTexture2D(target, levels, width, height, textureFormat);
}

void OsmAnd::GPUAPI_OpenGLES2plus::uploadDataToTexture2D(
    GLenum target, GLint level,
    GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
    const GLvoid* data, GLsizei dataRowLengthInElements, GLsizei elementSize,
    SourceFormat sourceFormat)
{
    GL_CHECK_PRESENT(glTexSubImage2D);

    // Try to use glPixelStorei to unpack
    if (glVersion >= 30 || isSupported_EXT_unpack_subimage)
    {
        GL_CHECK_PRESENT(glPixelStorei);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, dataRowLengthInElements);
        GL_CHECK_RESULT;

        glTexSubImage2D(target, level,
            xoffset, yoffset, width, height,
            static_cast<GLenum>(sourceFormat.format),
            static_cast<GLenum>(sourceFormat.type),
            data);
        GL_CHECK_RESULT;

        return;
    }

    // Otherwise fallback to manual unpacking

    // In case our row length is 0 or equals to image width (has no extra stride, just load as-is)
    if (dataRowLengthInElements == 0 || dataRowLengthInElements == width)
    {
        // Upload data
        glTexSubImage2D(target, level,
            xoffset, yoffset, width, height,
            static_cast<GLenum>(sourceFormat.format),
            static_cast<GLenum>(sourceFormat.type),
            data);
        GL_CHECK_RESULT;
        return;
    }

    // Otherwise we need to or load row by row
    auto pRow = reinterpret_cast<const uint8_t*>(data);
    for (auto rowIdx = 0; rowIdx < height; rowIdx++)
    {
        glTexSubImage2D(target, level,
            xoffset, yoffset + rowIdx, width, 1,
            static_cast<GLenum>(sourceFormat.format),
            static_cast<GLenum>(sourceFormat.type),
            pRow);
        GL_CHECK_RESULT;

        pRow += dataRowLengthInElements*elementSize;
    }
}

void OsmAnd::GPUAPI_OpenGLES2plus::setMipMapLevelsLimit(GLenum target, const uint32_t mipmapLevelsCount)
{
#if defined(OSMAND_TARGET_OS_ios)
    // Limit MipMap max level if possible
    if (isSupported_APPLE_texture_max_level)
    {
        GL_CHECK_PRESENT(glTexParameteri);

        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL_APPLE, mipmapLevelsCount);
        GL_CHECK_RESULT;
    }
#endif //OSMAND_TARGET_OS_ios
}

void OsmAnd::GPUAPI_OpenGLES2plus::glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays)
{
    assert(isSupported_vertex_array_object);

    if (glVersion >= 30)
    {
        GL_CHECK_PRESENT(glGenVertexArrays);

        glGenVertexArrays(n, arrays);
    }
    else if (_isSupported_OES_vertex_array_object)
    {
        GL_CHECK_PRESENT(glGenVertexArraysOES);

        glGenVertexArraysOES(n, arrays);
    }
}

void OsmAnd::GPUAPI_OpenGLES2plus::glBindVertexArray_wrapper(GLuint array)
{
    assert(isSupported_vertex_array_object);

    if (glVersion >= 30)
    {
        GL_CHECK_PRESENT(glBindVertexArray);

        glBindVertexArray(array);
    }
    else if (_isSupported_OES_vertex_array_object)
    {
        GL_CHECK_PRESENT(glBindVertexArrayOES);

        glBindVertexArrayOES(array);
    }
}

void OsmAnd::GPUAPI_OpenGLES2plus::glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays)
{
    assert(isSupported_vertex_array_object);

    if (glVersion >= 30)
    {
        GL_CHECK_PRESENT(glDeleteVertexArrays);

        glDeleteVertexArrays(n, arrays);
    }
    else if (_isSupported_OES_vertex_array_object)
    {
        GL_CHECK_PRESENT(glDeleteVertexArraysOES);

        glDeleteVertexArraysOES(n, arrays);
    }
}

GLsync OsmAnd::GPUAPI_OpenGLES2plus::glFenceSync_wrapper(GLenum condition, GLbitfield flags)
{
    if (glVersion >= 30)
    {
        GL_CHECK_PRESENT(glFenceSync);

        return glFenceSync(condition, flags);
    }
#if defined(OSMAND_TARGET_OS_ios)
    else if (isSupported_APPLE_sync)
    {
        GL_CHECK_PRESENT(glFenceSyncAPPLE);

        return glFenceSyncAPPLE(condition, flags);
    }
#endif // defined(OSMAND_TARGET_OS_ios)
    else
    {
        assert(false);
    }
}

void OsmAnd::GPUAPI_OpenGLES2plus::glDeleteSync_wrapper(GLsync sync)
{
    if (glVersion >= 30)
    {
        GL_CHECK_PRESENT(glDeleteSync);

        glDeleteSync(sync);
    }
#if defined(OSMAND_TARGET_OS_ios)
    else if (isSupported_APPLE_sync)
    {
        GL_CHECK_PRESENT(glDeleteSyncAPPLE);

        glDeleteSyncAPPLE(sync);
    }
#endif // defined(OSMAND_TARGET_OS_ios)
    else
    {
        assert(false);
    }
}

GLenum OsmAnd::GPUAPI_OpenGLES2plus::glClientWaitSync_wrapper(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    if (glVersion >= 30)
    {
        GL_CHECK_PRESENT(glClientWaitSync);

        return glClientWaitSync(sync, flags, timeout);
    }
#if defined(OSMAND_TARGET_OS_ios)
    else if (isSupported_APPLE_sync)
    {
        GL_CHECK_PRESENT(glClientWaitSyncAPPLE);

        return glClientWaitSyncAPPLE(sync, flags, timeout);
    }
#endif // defined(OSMAND_TARGET_OS_ios)
    else
    {
        assert(false);
    }
}

void OsmAnd::GPUAPI_OpenGLES2plus::preprocessShader(QString& code)
{
    QString shaderHeader;
    if (glslVersion >= 300)
    {
        shaderHeader = QStringLiteral(
            // Declare version of GLSL used
            "#version 300 es                                                                                                    ""\n"
            "                                                                                                                   ""\n"
            // General definitions
            "#define INPUT in                                                                                                   ""\n"
            "#define PARAM_OUTPUT out                                                                                           ""\n"
            "#define PARAM_INPUT in                                                                                             ""\n"
            "                                                                                                                   ""\n"
            // Features definitions
            "#define TEXTURE_LOD_SUPPORTED %TextureLodSupported%                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D texture                                                                                  ""\n"
            "#define SAMPLE_TEXTURE_2D_LOD textureLod                                                                           ""\n"
            "#define INTEGER_OPERATIONS_SUPPORTED %IntegerOperationsSupported%                                                  ""\n"
            "                                                                                                                   ""\n");
    }
    else if (glslVersion >= 100)
    {
        shaderHeader = QStringLiteral(
            // Declare version of GLSL used
            "#version 100                                                                                                       ""\n"
            "                                                                                                                   ""\n"
            // General definitions
            "#define INPUT attribute                                                                                            ""\n"
            "#define PARAM_OUTPUT varying                                                                                       ""\n"
            "#define PARAM_INPUT varying                                                                                        ""\n"
            "                                                                                                                   ""\n"
            // Features definitions
            "#define TEXTURE_LOD_SUPPORTED %TextureLodSupported%                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D texture2D                                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D_LOD texture2DLodEXT                                                                      ""\n"
            "#define INTEGER_OPERATIONS_SUPPORTED %IntegerOperationsSupported%                                                  ""\n"
            "                                                                                                                   ""\n");
    }
    else
    {
        assert(false);
    }

    auto shaderHeaderPreprocessed = shaderHeader;
    shaderHeaderPreprocessed.replace("%TextureLodSupported%", QString::number(isSupported_textureLod ? 1 : 0));
    shaderHeaderPreprocessed.replace("%IntegerOperationsSupported%", QString::number(isSupported_integerOperations ? 1 : 0));

    code.prepend(shaderHeaderPreprocessed);
}

void OsmAnd::GPUAPI_OpenGLES2plus::preprocessVertexShader(QString& code)
{
    QString commonHeader;
    preprocessShader(commonHeader);

    QString shaderHeader;
    if (glslVersion >= 300)
    {
        shaderHeader = QStringLiteral(
            // Set default precisions (this should be the default, but never trust mobile drivers)
            "#ifndef GL_FRAGMENT_PRECISION_HIGH                                                                                 ""\n"
            "#define GL_FRAGMENT_PRECISION_HIGH 1                                                                               ""\n"
            "#endif                                                                                                             ""\n"
            "precision highp float;                                                                                             ""\n"
            "precision highp int;                                                                                               ""\n"
            "                                                                                                                   ""\n");
    }
    else if (glslVersion >= 100)
    {
        shaderHeader = QStringLiteral(
            // It have been reported that GL_FRAGMENT_PRECISION_HIGH may not be defined in some cases
            "#ifndef GL_FRAGMENT_PRECISION_HIGH                                                                                 ""\n"
            "#if %HighPrecisionSupported%                                                                                       ""\n"
            "#define GL_FRAGMENT_PRECISION_HIGH 1                                                                               ""\n"
            "#endif                                                                                                             ""\n"
            "#endif                                                                                                             ""\n"
            "                                                                                                                   ""\n"
            // Set default precisions (this should be the default, but never trust mobile drivers)
            "#ifdef GL_FRAGMENT_PRECISION_HIGH                                                                                  ""\n"
            "precision highp float;                                                                                             ""\n"
            "precision highp int;                                                                                               ""\n"
            "#else                                                                                                              ""\n"
            "precision mediump float;                                                                                           ""\n"
            "precision mediump int;                                                                                             ""\n"
            "#endif                                                                                                             ""\n"
            "                                                                                                                   ""\n");
    }
    else
    {
        assert(false);
    }

    code.prepend(shaderHeader);
    code.prepend(commonHeader);
}

void OsmAnd::GPUAPI_OpenGLES2plus::preprocessFragmentShader(
    QString& code, const QString& fragmentTypePrefix /*= QString()*/, const QString& fragmentTypePrecision /*= QString()*/)
{
    QString commonHeader;
    preprocessShader(commonHeader);

    QString shaderHeader;
    if (glslVersion >= 300)
    {
        shaderHeader = QStringLiteral(
            // Set default precisions (this should be the default, but never trust mobile drivers)
            "#ifndef GL_FRAGMENT_PRECISION_HIGH                                                                                 ""\n"
            "#define GL_FRAGMENT_PRECISION_HIGH 1                                                                               ""\n"
            "#endif                                                                                                             ""\n"
            "precision highp float;                                                                                             ""\n"
            "precision highp int;                                                                                               ""\n"
            "                                                                                                                   ""\n"
            // Fragment shader output declaration
            "#define FRAGMENT_COLOR_OUTPUT out_FragColor                                                                        ""\n"
            "out %FragmentTypePrecision% %FragmentTypePrefix%vec4 out_FragColor;                                                ""\n"
            "                                                                                                                   ""\n");
    }
    else if (glslVersion >= 100)
    {
        assert(fragmentTypePrefix.isEmpty());
        assert(fragmentTypePrecision.isEmpty());
        shaderHeader = QStringLiteral(
            // Make some extensions required
            "#ifdef GL_EXT_shader_texture_lod                                                                                   ""\n"
            "#extension GL_EXT_shader_texture_lod : require                                                                     ""\n"
            "#endif // GL_EXT_shader_texture_lod                                                                                ""\n"
            "                                                                                                                   ""\n"
            // It have been reported that GL_FRAGMENT_PRECISION_HIGH may not be defined in some cases
            "#ifndef GL_FRAGMENT_PRECISION_HIGH                                                                                 ""\n"
            "#if %HighPrecisionSupported%                                                                                       ""\n"
            "#define GL_FRAGMENT_PRECISION_HIGH 1                                                                               ""\n"
            "#endif                                                                                                             ""\n"
            "#endif                                                                                                             ""\n"
            "                                                                                                                   ""\n"
            // Set default precisions (this should be the default, but never trust mobile drivers)
            "#ifdef GL_FRAGMENT_PRECISION_HIGH                                                                                  ""\n"
            "precision highp float;                                                                                             ""\n"
            "precision highp int;                                                                                               ""\n"
            "#else                                                                                                              ""\n"
            "precision mediump float;                                                                                           ""\n"
            "precision mediump int;                                                                                             ""\n"
            "#endif                                                                                                             ""\n"
            "                                                                                                                   ""\n"
            // Fragment shader output declaration
            "#define FRAGMENT_COLOR_OUTPUT gl_FragColor                                                                         ""\n");
    }
    else
    {
        assert(false);
    }

    auto shaderHeaderPreprocessed = shaderHeader;
    shaderHeaderPreprocessed.replace("%HighPrecisionSupported%", QString::number(
        supportedFragmentShaderPrecisionFormats.contains(GL_HIGH_FLOAT) && supportedFragmentShaderPrecisionFormats.contains(GL_HIGH_INT) ? 1 : 0));
    shaderHeaderPreprocessed.replace("%FragmentTypePrefix%", fragmentTypePrefix);
    shaderHeaderPreprocessed.replace("%FragmentTypePrecision%", fragmentTypePrecision);

    code.prepend(shaderHeaderPreprocessed);
    code.prepend(commonHeader);
}

void OsmAnd::GPUAPI_OpenGLES2plus::optimizeVertexShader(QString& code)
{
}

void OsmAnd::GPUAPI_OpenGLES2plus::optimizeFragmentShader(QString& code)
{
}

void OsmAnd::GPUAPI_OpenGLES2plus::setTextureBlockSampler(GLenum textureBlock, SamplerType samplerType)
{
    if (isSupported_samplerObjects)
    {
        GL_CHECK_PRESENT(glBindSampler);

        glBindSampler(textureBlock - GL_TEXTURE0, _textureSamplers[static_cast<int>(samplerType)]);
        GL_CHECK_RESULT;
    }
    else
    {
        // In case sampler objects are not supported, use settings per-texture
        _textureBlocksSamplers[textureBlock] = samplerType;
    }
}

void OsmAnd::GPUAPI_OpenGLES2plus::applyTextureBlockToTexture(GLenum texture, GLenum textureBlock)
{
    if (isSupported_samplerObjects)
    {
        // In case sampler objects are supported, nothing to do here
    }
    else
    {
        GL_CHECK_PRESENT(glTexParameteri);

        const auto samplerType = _textureBlocksSamplers[textureBlock];
        if (samplerType == SamplerType::ElevationDataTile)
        {
            glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            GL_CHECK_RESULT;
        }
        else if (samplerType == SamplerType::BitmapTile_Bilinear)
        {
            glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GL_CHECK_RESULT;
        }
        else if (samplerType == SamplerType::BitmapTile_BilinearMipmap)
        {
            glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GL_CHECK_RESULT;
        }
        else if (samplerType == SamplerType::BitmapTile_TrilinearMipmap)
        {
            glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GL_CHECK_RESULT;
        }
        else if (samplerType == SamplerType::Symbol)
        {
            glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GL_CHECK_RESULT;
        }
        else if (samplerType == SamplerType::DepthBuffer)
        {
            glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            GL_CHECK_RESULT;
        }
    }
}

void OsmAnd::GPUAPI_OpenGLES2plus::pushDebugGroupMarker(const QString& title)
{
    if (isSupported_EXT_debug_marker)
    {
        GL_CHECK_PRESENT(glPushGroupMarkerEXT);
        glPushGroupMarkerEXT(0, qPrintable(title));
    }
}

void OsmAnd::GPUAPI_OpenGLES2plus::popDebugGroupMarker()
{
    if (isSupported_EXT_debug_marker)
    {
        GL_CHECK_PRESENT(glPopGroupMarkerEXT);
        glPopGroupMarkerEXT();
    }
}

void OsmAnd::GPUAPI_OpenGLES2plus::setObjectLabel(ObjectType type_, GLuint name, const QString& label)
{
    if (isSupported_EXT_debug_label)
    {
        GLenum type;
        switch(type_)
        {
            case ObjectType::Buffer:
                type = GL_BUFFER_OBJECT_EXT;
                break;
            case ObjectType::Shader:
                type = GL_SHADER_OBJECT_EXT;
                break;
            case ObjectType::Program:
                type = GL_PROGRAM_OBJECT_EXT;
                break;
            case ObjectType::VertexArray:
                type = GL_VERTEX_ARRAY_OBJECT_EXT;
                break;
            case ObjectType::Query:
                type = GL_QUERY_OBJECT_EXT;
                break;
            case ObjectType::ProgramPipeline:
                type = GL_PROGRAM_PIPELINE_OBJECT_EXT;
                break;
            case ObjectType::TransformFeedback:
                type = GL_TRANSFORM_FEEDBACK;
                break;
            case ObjectType::Sampler:
                type = GL_SAMPLER;
                break;
            case ObjectType::Texture:
                type = GL_TEXTURE;
                break;
            case ObjectType::Renderbuffer:
                type = GL_RENDERBUFFER;
                break;
            case ObjectType::Framebuffer:
                type = GL_FRAMEBUFFER;
                break;
            default:
                assert(false);
                return;
        }

        GL_CHECK_PRESENT(glLabelObjectEXT);
        glLabelObjectEXT(type, name, 0, qPrintable(label));
        GL_CHECK_RESULT;
    }
}
