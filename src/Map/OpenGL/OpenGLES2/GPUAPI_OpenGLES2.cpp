#include "GPUAPI_OpenGLES2.h"

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
#   define GL_RGBA8_EXT                                                 0x8058
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
OsmAnd::GPUAPI_OpenGLES2::P_glTexStorage2DEXT_PROC OsmAnd::GPUAPI_OpenGLES2::glTexStorage2DEXT = nullptr;

PFNGLBINDVERTEXARRAYOESPROC OsmAnd::GPUAPI_OpenGLES2::glBindVertexArrayOES = nullptr;
PFNGLDELETEVERTEXARRAYSOESPROC OsmAnd::GPUAPI_OpenGLES2::glDeleteVertexArraysOES = nullptr;
PFNGLGENVERTEXARRAYSOESPROC OsmAnd::GPUAPI_OpenGLES2::glGenVertexArraysOES = nullptr;

OsmAnd::GPUAPI_OpenGLES2::PFNGLPOPGROUPMARKEREXTPROC OsmAnd::GPUAPI_OpenGLES2::glPopGroupMarkerEXT = nullptr;
OsmAnd::GPUAPI_OpenGLES2::PFNGLPUSHGROUPMARKEREXTPROC OsmAnd::GPUAPI_OpenGLES2::glPushGroupMarkerEXT = nullptr;
#endif //!OSMAND_TARGET_OS_ios

OsmAnd::GPUAPI_OpenGLES2::GPUAPI_OpenGLES2()
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
    , supportedVertexShaderPrecisionFormats(_supportedVertexShaderPrecisionFormats)
    , supportedFragmentShaderPrecisionFormats(_supportedFragmentShaderPrecisionFormats)
{
}

OsmAnd::GPUAPI_OpenGLES2::~GPUAPI_OpenGLES2()
{
}

GLenum OsmAnd::GPUAPI_OpenGLES2::validateResult(const char* const function, const char* const file, const int line)
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

bool OsmAnd::GPUAPI_OpenGLES2::isShaderPrecisionFormatSupported(GLenum shaderType, GLenum precisionType)
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

bool OsmAnd::GPUAPI_OpenGLES2::initialize()
{
    bool ok;

    ok = GPUAPI_OpenGL::initialize();
    if (!ok)
        return false;

    const auto glVersionString = glGetString(GL_VERSION); // Format: "OpenGL<space>ES<space><version number><space><vendor-specific information>"
    GL_CHECK_RESULT;
    QRegExp glVersionRegExp(QLatin1String("(\\d+).(\\d+)"));
    glVersionRegExp.indexIn(QString(QLatin1String(reinterpret_cast<const char*>(glVersionString))));
    _glVersion = glVersionRegExp.cap(1).toUInt() * 10 + glVersionRegExp.cap(2).toUInt();
    LogPrintf(LogSeverityLevel::Info, "OpenGLES version %d [%s]", _glVersion, glVersionString);
    if (_glVersion < 20)
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
    _extensions = extensionsString.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 extensions: %s", qPrintable(extensions.join(' ')));

    GLboolean shaderCompilerPresent = GL_FALSE;
    glGetBooleanv(GL_SHADER_COMPILER, &shaderCompilerPresent);
    if (shaderCompilerPresent == GL_FALSE)
    {
        LogPrintf(LogSeverityLevel::Error, "OpenGLES2 shader compiler not supported");
        return false;
    }

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&_maxTextureSize));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture size %dx%d", _maxTextureSize, _maxTextureSize);

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, reinterpret_cast<GLint*>(&_maxTextureUnitsInFragmentShader));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture units in fragment shader %d", _maxTextureUnitsInFragmentShader);

    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, reinterpret_cast<GLint*>(&_maxTextureUnitsInVertexShader));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture units in vertex shader %d", _maxTextureUnitsInVertexShader);
    _isSupported_vertexShaderTextureLookup = (_maxTextureUnitsInVertexShader >= 1);

    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, reinterpret_cast<GLint*>(&_maxTextureUnitsCombined));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture units combined %d", _maxTextureUnitsCombined);

    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &_maxVertexUniformVectors);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal 4-component parameters in vertex shader %d", _maxVertexUniformVectors);

    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &_maxFragmentUniformVectors);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal 4-component parameters in fragment shader %d", _maxFragmentUniformVectors);

    glGetIntegerv(GL_MAX_VARYING_FLOATS, &_maxVaryingFloats);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal varying floats %d", _maxVaryingFloats);

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &_maxVertexAttribs);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal vertex attributes %d", _maxVertexAttribs);

    _isSupported_OES_vertex_array_object = extensions.contains("GL_OES_vertex_array_object");
    _isSupported_vertex_array_object = _isSupported_OES_vertex_array_object;

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
    _isSupported_texturesNPOT = extensions.contains("GL_OES_texture_npot");
    _isSupported_EXT_debug_marker = extensions.contains("GL_EXT_debug_marker");

    if (isShaderPrecisionFormatSupported(GL_VERTEX_SHADER, GL_LOW_FLOAT))
        _supportedVertexShaderPrecisionFormats.insert(GL_LOW_FLOAT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES2 vertex shader does not support 'GL_LOW_FLOAT' precision format");

    if (isShaderPrecisionFormatSupported(GL_VERTEX_SHADER, GL_MEDIUM_FLOAT))
        _supportedVertexShaderPrecisionFormats.insert(GL_MEDIUM_FLOAT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES2 vertex shader does not support 'GL_MEDIUM_FLOAT' precision format");

    if (isShaderPrecisionFormatSupported(GL_VERTEX_SHADER, GL_HIGH_FLOAT))
        _supportedVertexShaderPrecisionFormats.insert(GL_HIGH_FLOAT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES2 vertex shader does not support 'GL_HIGH_FLOAT' precision format");

    if (isShaderPrecisionFormatSupported(GL_VERTEX_SHADER, GL_LOW_INT))
        _supportedVertexShaderPrecisionFormats.insert(GL_LOW_INT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES2 vertex shader does not support 'GL_LOW_INT' precision format");

    if (isShaderPrecisionFormatSupported(GL_VERTEX_SHADER, GL_MEDIUM_INT))
        _supportedVertexShaderPrecisionFormats.insert(GL_MEDIUM_INT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES2 vertex shader does not support 'GL_MEDIUM_INT' precision format");

    if (isShaderPrecisionFormatSupported(GL_VERTEX_SHADER, GL_HIGH_INT))
        _supportedVertexShaderPrecisionFormats.insert(GL_HIGH_INT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES2 vertex shader does not support 'GL_HIGH_INT' precision format");

    if (isShaderPrecisionFormatSupported(GL_FRAGMENT_SHADER, GL_LOW_FLOAT))
        _supportedFragmentShaderPrecisionFormats.insert(GL_LOW_FLOAT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES2 fragment shader does not support 'GL_LOW_FLOAT' precision format");

    if (isShaderPrecisionFormatSupported(GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT))
        _supportedFragmentShaderPrecisionFormats.insert(GL_MEDIUM_FLOAT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES2 fragment shader does not support 'GL_MEDIUM_FLOAT' precision format");

    if (isShaderPrecisionFormatSupported(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT))
        _supportedFragmentShaderPrecisionFormats.insert(GL_HIGH_FLOAT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES2 fragment shader does not support 'GL_HIGH_FLOAT' precision format");

    if (isShaderPrecisionFormatSupported(GL_FRAGMENT_SHADER, GL_LOW_INT))
        _supportedFragmentShaderPrecisionFormats.insert(GL_LOW_INT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES2 fragment shader does not support 'GL_LOW_INT' precision format");

    if (isShaderPrecisionFormatSupported(GL_FRAGMENT_SHADER, GL_MEDIUM_INT))
        _supportedFragmentShaderPrecisionFormats.insert(GL_MEDIUM_INT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES2 fragment shader does not support 'GL_MEDIUM_INT' precision format");

    if (isShaderPrecisionFormatSupported(GL_FRAGMENT_SHADER, GL_HIGH_INT))
        _supportedFragmentShaderPrecisionFormats.insert(GL_HIGH_INT);
    else
        LogPrintf(LogSeverityLevel::Info, "OpenGLES2 fragment shader does not support 'GL_HIGH_INT' precision format");

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

    _isSupported_texture_float = _isSupported_OES_texture_float;
    _isSupported_texture_half_float = _isSupported_OES_texture_half_float;
    _isSupported_textureLod = _isSupported_EXT_shader_texture_lod;
    _isSupported_texture_rg = _isSupported_EXT_texture_rg;
    _isSupported_texture_storage = _isSupported_EXT_texture_storage;

    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::GPUAPI_OpenGLES2::release(const bool contextLost)
{
    bool ok;

    ok = GPUAPI_OpenGL::release(contextLost);
    if (!ok)
        return false;

    return true;
}

OsmAnd::GPUAPI_OpenGLES2::TextureFormat OsmAnd::GPUAPI_OpenGLES2::getTextureFormat(
    const SkColorType colorType) const
{
    if (isSupported_texture_storage)
    {
        GLenum internalFormat = GL_INVALID_ENUM;
        switch (colorType)
        {
            case SkColorType::kRGBA_8888_SkColorType:
                if (isSupported_ARM_rgba8 || isSupported_OES_rgb8_rgba8)
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

OsmAnd::GPUAPI_OpenGLES2::TextureFormat OsmAnd::GPUAPI_OpenGLES2::getTextureFormat_float() const
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
        format = isSupported_texture_rg ? GL_R32F_EXT : GL_LUMINANCE32F_EXT;
        type = GL_FLOAT;
    }
    else if (isSupported_texture_half_float)
    {
        format = isSupported_texture_rg ? GL_R16F_EXT : GL_LUMINANCE16F_EXT;
        type = GL_HALF_FLOAT_OES;
    }
    else
    {
        format = GL_LUMINANCE;
        type = GL_UNSIGNED_BYTE;
    }

    return TextureFormat::Make(type, format);
}

bool OsmAnd::GPUAPI_OpenGLES2::isValidTextureFormat(const TextureFormat textureFormat) const
{
    return
        textureFormat.format != GL_INVALID_ENUM &&
        textureFormat.type != GL_INVALID_ENUM;
}

size_t OsmAnd::GPUAPI_OpenGLES2::getTextureFormatPixelSize(const TextureFormat textureFormat) const
{
    GLenum format = static_cast<GLenum>(textureFormat.format);
    GLenum type = static_cast<GLenum>(textureFormat.type);
    if ((format == GL_R32F_EXT || format == GL_LUMINANCE32F_EXT) && type == GL_FLOAT)
        return 4;
    else if ((format == GL_R16F_EXT || format == GL_LUMINANCE16F_EXT) && type == GL_HALF_FLOAT_OES)
        return 2;

    return GPUAPI_OpenGL::getTextureFormatPixelSize(textureFormat);
}

GLenum OsmAnd::GPUAPI_OpenGLES2::getBaseInteralTextureFormat(const TextureFormat textureFormat) const
{
    switch (textureFormat.format)
    {
        case GL_R32F_EXT:
        case GL_R16F_EXT:
            return GL_RED_EXT;

        case GL_LUMINANCE32F_EXT:
        case GL_LUMINANCE16F_EXT:
            return GL_LUMINANCE;
    }

    return static_cast<GLenum>(textureFormat.format);
}

OsmAnd::GPUAPI_OpenGLES2::SourceFormat OsmAnd::GPUAPI_OpenGLES2::getSourceFormat_float() const
{
    GLenum format = GL_INVALID_ENUM;
    GLenum type = GL_INVALID_ENUM;

    if (isSupported_EXT_texture_rg)
        format = GL_RED_EXT;
    else
        format = GL_LUMINANCE;

    // NOTE: Regardless of float support, source is float
    type = GL_FLOAT;

    return SourceFormat::Make(type, format);
}

bool OsmAnd::GPUAPI_OpenGLES2::isValidSourceFormat(const SourceFormat sourceFormat) const
{
    return
        sourceFormat.format != GL_INVALID_ENUM &&
        sourceFormat.type != GL_INVALID_ENUM;
}

void OsmAnd::GPUAPI_OpenGLES2::allocateTexture2D(
    GLenum target,
    GLsizei levels,
    GLsizei width,
    GLsizei height,
    const TextureFormat textureFormat)
{
    if (isSupported_EXT_texture_storage)
    {
        GL_CHECK_PRESENT(glTexStorage2DEXT);

        glTexStorage2DEXT(target, levels, static_cast<GLenum>(textureFormat.format), width, height);
        GL_CHECK_RESULT;
        return;
    }

    // Fallback to dumb allocation
    GPUAPI_OpenGL::allocateTexture2D(target, levels, width, height, textureFormat);
}

void OsmAnd::GPUAPI_OpenGLES2::uploadDataToTexture2D(
    GLenum target, GLint level,
    GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
    const GLvoid *data, GLsizei dataRowLengthInElements, GLsizei elementSize,
    const SourceFormat sourceFormat)
{
    GL_CHECK_PRESENT(glTexSubImage2D);

    // Try to use glPixelStorei to unpack
    if (isSupported_EXT_unpack_subimage)
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

void OsmAnd::GPUAPI_OpenGLES2::setMipMapLevelsLimit(GLenum target, const uint32_t mipmapLevelsCount)
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

void OsmAnd::GPUAPI_OpenGLES2::glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays)
{
    assert(isSupported_vertex_array_object);

    GL_CHECK_PRESENT(glGenVertexArraysOES);

    glGenVertexArraysOES(n, arrays);
}

void OsmAnd::GPUAPI_OpenGLES2::glBindVertexArray_wrapper(GLuint array)
{
    assert(isSupported_vertex_array_object);

    GL_CHECK_PRESENT(glBindVertexArrayOES);

    glBindVertexArrayOES(array);
}

void OsmAnd::GPUAPI_OpenGLES2::glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays)
{
    assert(isSupported_vertex_array_object);

    GL_CHECK_PRESENT(glDeleteVertexArraysOES);

    glDeleteVertexArraysOES(n, arrays);
}

void OsmAnd::GPUAPI_OpenGLES2::preprocessShader(QString& code)
{
    const auto& shaderHeader = QString::fromLatin1(
        // Declare version of GLSL used
        "#version 100                                                                                                       ""\n"
        "                                                                                                                   ""\n");

    const auto& shaderAdditionalPrologue = QString::fromLatin1(
        // General definitions
        "#define INPUT attribute                                                                                            ""\n"
        "#define PARAM_OUTPUT varying                                                                                       ""\n"
        "#define PARAM_INPUT varying                                                                                        ""\n"
        "                                                                                                                   ""\n"
        // Features definitions
        "#define VERTEX_TEXTURE_FETCH_SUPPORTED %VertexTextureFetchSupported%                                               ""\n"
        "#define TEXTURE_LOD_SUPPORTED %TextureLodSupported%                                                                ""\n"
        "#define SAMPLE_TEXTURE_2D texture2D                                                                                ""\n"
        "#define SAMPLE_TEXTURE_2D_LOD texture2DLodEXT                                                                      ""\n"
        "                                                                                                                   ""\n");

    auto shaderAdditionalProloguePreprocessed = shaderAdditionalPrologue;
    shaderAdditionalProloguePreprocessed.replace("%VertexTextureFetchSupported%",
        QString::number(isSupported_vertexShaderTextureLookup ? 1 : 0));
    shaderAdditionalProloguePreprocessed.replace("%TextureLodSupported%",
        QString::number(isSupported_textureLod ? 1 : 0));

    code.prepend(shaderAdditionalProloguePreprocessed);
    code.prepend(shaderHeader);
}

void OsmAnd::GPUAPI_OpenGLES2::preprocessVertexShader(QString& code)
{
    const auto& vertexShaderAdditionalPrologue = QString::fromLatin1(
        // Set default precisions (this should be the default, but never trust mobile drivers)
        "precision highp float;                                                                                             ""\n"
        "precision highp int;                                                                                               ""\n"
        "                                                                                                                   ""\n");

    code.prepend(vertexShaderAdditionalPrologue);
    preprocessShader(code);
}

void OsmAnd::GPUAPI_OpenGLES2::preprocessFragmentShader(QString& code)
{
    const auto& fragmentShaderAdditionalPrologue = QString::fromLatin1(
        // Make some extensions required
        "#ifdef GL_EXT_shader_texture_lod                                                                                   ""\n"
        "#extension GL_EXT_shader_texture_lod : require                                                                     ""\n"
        "#endif // GL_EXT_shader_texture_lod                                                                                ""\n"
        "                                                                                                                   ""\n"
        // Fragment shader output declaration
        "#define FRAGMENT_COLOR_OUTPUT gl_FragColor                                                                         ""\n"
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
        "precision highp float;                                                                                             ""\n"
        "precision highp int;                                                                                               ""\n"
        "#endif                                                                                                             ""\n"
        "                                                                                                                   ""\n");

    auto fragmentShaderAdditionalProloguePreprocessed = fragmentShaderAdditionalPrologue;
    fragmentShaderAdditionalProloguePreprocessed.replace("%HighPrecisionSupported%", QString::number(
        supportedFragmentShaderPrecisionFormats.contains(GL_HIGH_FLOAT) && supportedFragmentShaderPrecisionFormats.contains(GL_HIGH_INT) ? 1 : 0));

    code.prepend(fragmentShaderAdditionalProloguePreprocessed);
    preprocessShader(code);
}

void OsmAnd::GPUAPI_OpenGLES2::optimizeVertexShader(QString& code)
{
}

void OsmAnd::GPUAPI_OpenGLES2::optimizeFragmentShader(QString& code)
{
}

void OsmAnd::GPUAPI_OpenGLES2::setTextureBlockSampler(const GLenum textureBlock, const SamplerType samplerType)
{
    // In OpenGLES 2.0 there is no settings of texture-block, so these settings are per-texture
    _textureBlocksSamplers[textureBlock] = samplerType;
}

void OsmAnd::GPUAPI_OpenGLES2::applyTextureBlockToTexture(const GLenum texture, const GLenum textureBlock)
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
}

void OsmAnd::GPUAPI_OpenGLES2::glPushGroupMarkerEXT_wrapper(GLsizei length, const GLchar* marker)
{
    GL_CHECK_PRESENT(glPushGroupMarkerEXT);
    glPushGroupMarkerEXT(length, marker);
}

void OsmAnd::GPUAPI_OpenGLES2::glPopGroupMarkerEXT_wrapper()
{
    GL_CHECK_PRESENT(glPopGroupMarkerEXT);
    glPopGroupMarkerEXT();
}

void OsmAnd::GPUAPI_OpenGLES2::glClearDepth_wrapper(const float depth)
{
    GL_CHECK_PRESENT(glClearDepthf);

    glClearDepthf(depth);
}
