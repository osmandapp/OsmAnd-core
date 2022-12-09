#include "GPUAPI_OpenGL2plus.h"

#include <cassert>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QStringList>
#include <QRegExp>
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

OsmAnd::GPUAPI_OpenGL2plus::GPUAPI_OpenGL2plus()
    : _isSupported_GREMEDY_string_marker(false)
    , _isSupported_ARB_sampler_objects(false)
    , _isSupported_samplerObjects(false)
    , _isSupported_ARB_vertex_array_object(false)
    , _isSupported_APPLE_vertex_array_object(false)
    , _isSupported_ARB_texture_storage(false)
    , _isSupported_ARB_texture_float(false)
    , _isSupported_ATI_texture_float(false)
    , _isSupported_ARB_texture_rg(false)
    , _isSupported_EXT_gpu_shader4(false)
    , _isSupported_EXT_debug_marker(false)
    , _isSupported_EXT_debug_label(false)
    , _isSupported_ARB_sync(false)
    , _framebufferDepthDataFormat(0)
    , _framebufferDepthDataType(0)
    , isSupported_GREMEDY_string_marker(_isSupported_GREMEDY_string_marker)
    , isSupported_ARB_sampler_objects(_isSupported_ARB_sampler_objects)
    , isSupported_samplerObjects(_isSupported_samplerObjects)
    , isSupported_ARB_vertex_array_object(_isSupported_ARB_vertex_array_object)
    , isSupported_APPLE_vertex_array_object(_isSupported_APPLE_vertex_array_object)
    , isSupported_ARB_texture_storage(_isSupported_ARB_texture_storage)
    , isSupported_ARB_texture_float(_isSupported_ARB_texture_float)
    , isSupported_ATI_texture_float(_isSupported_ATI_texture_float)
    , isSupported_ARB_texture_rg(_isSupported_ARB_texture_rg)
    , isSupported_EXT_gpu_shader4(_isSupported_EXT_gpu_shader4)
    , isSupported_EXT_debug_marker(_isSupported_EXT_debug_marker)
    , isSupported_EXT_debug_label(_isSupported_EXT_debug_label)
    , isSupported_ARB_sync(_isSupported_ARB_sync)
    , framebufferDepthDataFormat(_framebufferDepthDataFormat)
    , framebufferDepthDataType(_framebufferDepthDataType)
{
}

OsmAnd::GPUAPI_OpenGL2plus::~GPUAPI_OpenGL2plus()
{
}

GLenum OsmAnd::GPUAPI_OpenGL2plus::validateResult(const char* const function, const char* const file, const int line)
{
    GL_CHECK_PRESENT(glGetError);

    auto result = glGetError();
    if (result == GL_NO_ERROR)
        return result;

    LogPrintf(LogSeverityLevel::Error,
        "OpenGL error 0x%08x (%s) in %s at %s:%d",
        result,
        gluErrorString(result),
        function,
        file,
        line);

    return result;
}

bool OsmAnd::GPUAPI_OpenGL2plus::initialize()
{
    bool ok;

    ok = GPUAPI_OpenGL::initialize();
    if (!ok)
        return false;

    if (glewInit() != GLEW_NO_ERROR)
        return false;
    // Silence OpenGL error here, it's inside GLEW, so it's not ours
    (void)glGetError();

    GL_CHECK_PRESENT(glGetError);
    GL_CHECK_PRESENT(glGetString);
    GL_CHECK_PRESENT(glGetFloatv);
    GL_CHECK_PRESENT(glGetIntegerv);
    GL_CHECK_PRESENT(glHint);

    const auto glVersionString = glGetString(GL_VERSION);
    GL_CHECK_RESULT;
    QRegExp glVersionRegExp(QLatin1String("(\\d+).(\\d+)"));
    glVersionRegExp.indexIn(QString(QLatin1String(reinterpret_cast<const char*>(glVersionString))));
    _glVersion = glVersionRegExp.cap(1).toUInt() * 10 + glVersionRegExp.cap(2).toUInt();
    LogPrintf(LogSeverityLevel::Info, "OpenGL version %d [%s]", _glVersion, glVersionString);
    if (_glVersion < 20)
    {
        LogPrintf(LogSeverityLevel::Info, "This OpenGL version is not supported");
        return false;
    }
    //////////////////////////////////////////////////////////////////////////
    //NOTE: For testing, limit GL version to 2.0
    //_glVersion = 20;
    //////////////////////////////////////////////////////////////////////////

    const auto glslVersionString = glGetString(GL_SHADING_LANGUAGE_VERSION);
    GL_CHECK_RESULT;
    QRegExp glslVersionRegExp(QLatin1String("(\\d+).(\\d+)"));
    glslVersionRegExp.indexIn(QString(QLatin1String(reinterpret_cast<const char*>(glslVersionString))));
    _glslVersion = glslVersionRegExp.cap(1).toUInt() * 100 + glslVersionRegExp.cap(2).toUInt();
    LogPrintf(LogSeverityLevel::Info, "GLSL version %d [%s]", _glslVersion, glslVersionString);
    //////////////////////////////////////////////////////////////////////////
    //NOTE: For testing, limit GLSL version to 1.30, what corresponds to OpenGL 3.0
    //NOTE: For testing, limit GLSL version to 1.10, what corresponds to OpenGL 2.0
    //_glslVersion = 110;
    //////////////////////////////////////////////////////////////////////////

    if (glVersion >= 30)
    {
        GL_CHECK_PRESENT(glGetStringi);

        GLint numExtensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        GL_CHECK_RESULT;
        _extensions.clear();
        for (auto extensionIdx = 0; extensionIdx < numExtensions; extensionIdx++)
        {
            const auto& extension = QLatin1String(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, extensionIdx)));
            GL_CHECK_RESULT;

            _extensions.push_back(extension);
        }
    }
    else
    {
        const auto& extensionsString = QString::fromLatin1(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
        GL_CHECK_RESULT;
        _extensions = extensionsString.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    }
    LogPrintf(LogSeverityLevel::Info, "OpenGL extensions: %s", qPrintable(extensions.join(' ')));

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&_maxTextureSize));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture size %dx%d", _maxTextureSize, _maxTextureSize);

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, reinterpret_cast<GLint*>(&_maxTextureUnitsInFragmentShader));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture units in fragment shader %d", _maxTextureUnitsInFragmentShader);

    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, reinterpret_cast<GLint*>(&_maxTextureUnitsInVertexShader));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture units in vertex shader %d", _maxTextureUnitsInVertexShader);
    //////////////////////////////////////////////////////////////////////////
    //NOTE: for testing
    // _maxTextureUnitsInVertexShader = 0;
    //////////////////////////////////////////////////////////////////////////
    _isSupported_vertexShaderTextureLookup = (_maxTextureUnitsInVertexShader >= 1);

    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, reinterpret_cast<GLint*>(&_maxTextureUnitsCombined));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture units combined %d", _maxTextureUnitsCombined);

    if (extensions.contains(QLatin1String("GL_ARB_ES2_compatibility")))
    {
        // According to http://www.opengl.org/wiki/GLSL_Uniform ("Implementation limits") , this will give incorrect results for AMD/ATI
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &_maxVertexUniformVectors);
        GL_CHECK_RESULT;
        LogPrintf(LogSeverityLevel::Info, "OpenGL maximal 4-component parameters in vertex shader %d", _maxVertexUniformVectors);

        // According to http://www.opengl.org/wiki/GLSL_Uniform ("Implementation limits") , this will give incorrect results for AMD/ATI
        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &_maxFragmentUniformVectors);
        GL_CHECK_RESULT;
        LogPrintf(LogSeverityLevel::Info, "OpenGL maximal 4-component parameters in fragment shader %d", _maxFragmentUniformVectors);
    }

    GLint maxVertexUniformComponents;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniformComponents);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal parameters in vertex shader %d", maxVertexUniformComponents);
    _maxVertexUniformVectors = maxVertexUniformComponents / 4; // Workaround for AMD/ATI (see above)

    GLint maxFragmentUniformComponents;
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxFragmentUniformComponents);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal parameters in fragment shader %d", maxFragmentUniformComponents);
    _maxFragmentUniformVectors = maxFragmentUniformComponents / 4; // Workaround for AMD/ATI (see above)

    glGetIntegerv(GL_MAX_VARYING_FLOATS, &_maxVaryingFloats);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal varying floats %d", _maxVaryingFloats);

    if (glVersion >= 40 || extensions.contains(QLatin1String("GL_ARB_ES2_compatibility")))
    {
        glGetIntegerv(GL_MAX_VARYING_VECTORS, &_maxVaryingVectors);
        GL_CHECK_RESULT;
        LogPrintf(LogSeverityLevel::Info, "OpenGL maximal varying vectors %d", _maxVaryingVectors);
    }
    else if (glVersion >= 30)
    {
        GLint maxVaryingComponents;
        glGetIntegerv(GL_MAX_VARYING_COMPONENTS, &maxVaryingComponents);
        GL_CHECK_RESULT;
        LogPrintf(LogSeverityLevel::Info, "OpenGL maximal varying components %d", maxVaryingComponents);
        _maxVaryingVectors = maxVaryingComponents / 4;
    }
    else
    {
        _maxVaryingVectors = _maxVaryingFloats / 4;
    }

    if (glVersion >= 43)
    {
        GLint maxUniformLocations;
        glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &maxUniformLocations);
        GL_CHECK_RESULT;
        LogPrintf(LogSeverityLevel::Info, "OpenGL maximal defined parameters %d", maxUniformLocations);
    }

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &_maxVertexAttribs);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal vertex attributes %d", _maxVertexAttribs);

    // textureLod() is supported by GLSL 1.30+ specification (which is supported by OpenGL 3.0+), or if GL_ARB_shader_texture_lod is available
    _isSupported_textureLod = (glslVersion >= 130) || extensions.contains(QLatin1String("GL_ARB_shader_texture_lod"));
    _isSupported_texturesNPOT = (glVersion >= 20); // OpenGL 2.0+ fully supports NPOT textures
    _isSupported_EXT_debug_marker = extensions.contains("GL_EXT_debug_marker");
    _isSupported_GREMEDY_string_marker = extensions.contains("GL_GREMEDY_string_marker");
    _isSupported_debug_marker = _isSupported_EXT_debug_marker || _isSupported_GREMEDY_string_marker;
    _isSupported_EXT_debug_label = extensions.contains("GL_EXT_debug_label");
    _isSupported_debug_label = (glVersion >= 43 || _isSupported_EXT_debug_label);
    _isSupported_ARB_sync = extensions.contains("GL_ARB_sync");
    _isSupported_sync = (glVersion >= 32 || _isSupported_ARB_sync);
    // http://www.opengl.org/sdk/docs/man/html/glGenSamplers.xhtml are supported only if OpenGL 3.3+ or GL_ARB_sampler_objects is available
    _isSupported_ARB_sampler_objects = extensions.contains(QLatin1String("GL_ARB_sampler_objects"));
    _isSupported_samplerObjects = (glVersion >= 33) || _isSupported_ARB_sampler_objects;
    _isSupported_ARB_vertex_array_object = extensions.contains(QLatin1String("GL_ARB_vertex_array_object"));
    _isSupported_APPLE_vertex_array_object = extensions.contains(QLatin1String("GL_APPLE_vertex_array_object"));
    _isSupported_vertex_array_object = (glVersion >= 30 || isSupported_ARB_vertex_array_object || isSupported_APPLE_vertex_array_object);
    //////////////////////////////////////////////////////////////////////////
    //NOTE: for testing
    //_isSupported_vertex_array_object = false;
    //////////////////////////////////////////////////////////////////////////

    // glTexStorage2D is supported in OpenGL 4.2+ or if GL_ARB_texture_storage is available
    // https://www.opengl.org/sdk/docs/man/html/glTexStorage2D.xhtml
    _isSupported_ARB_texture_storage = extensions.contains(QLatin1String("GL_ARB_texture_storage"));
    _isSupported_texture_storage = (glVersion >= 42) || isSupported_ARB_texture_storage;

    _isSupported_ARB_texture_float = extensions.contains(QStringLiteral("GL_ARB_texture_float"));
    _isSupported_ATI_texture_float = extensions.contains(QStringLiteral("GL_ATI_texture_float"));
    _isSupported_texture_float = isSupported_ARB_texture_float || isSupported_ATI_texture_float;

    _isSupported_texture_rg = _isSupported_ARB_texture_rg = extensions.contains(QLatin1String("GL_ARB_texture_rg"));

    _isSupported_EXT_gpu_shader4 = extensions.contains(QStringLiteral("GL_EXT_gpu_shader4"));
    _isSupported_integerOperations = (glslVersion >= 130) || _isSupported_EXT_gpu_shader4;

    GLint compressedFormatsLength = 0;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &compressedFormatsLength);
    GL_CHECK_RESULT;
    _compressedFormats.resize(compressedFormatsLength);
    if (compressedFormatsLength > 0)
    {
        glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, _compressedFormats.data());
        GL_CHECK_RESULT;
    }

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

bool OsmAnd::GPUAPI_OpenGL2plus::attachToRenderTarget()
{
    if (isAttachedToRenderTarget())
        return false;

    glGetIntegerv(GL_DEPTH_BITS, &_framebufferDepthBits);
    GL_CHECK_RESULT;
    if (_framebufferDepthBits == 24)
        _framebufferDepthBytes = 4;
    else
        _framebufferDepthBytes = _framebufferDepthBits / 8;
    LogPrintf(LogSeverityLevel::Info, "OpenGL render target depth buffer: %d bits (%d bytes)", _framebufferDepthBits, _framebufferDepthBytes);

    GLint framebufferStencilBits;
    glGetIntegerv(GL_STENCIL_BITS, &framebufferStencilBits);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL render target stencil buffer: %d bits", framebufferStencilBits);

    if (_framebufferDepthBits == 32)
    {
        _framebufferDepthDataFormat = GL_DEPTH_COMPONENT;
        _framebufferDepthDataType = GL_UNSIGNED_INT;
        LogPrintf(LogSeverityLevel::Info, "OpenGL render target depth buffer: DEPTH_COMPONENT/UNSIGNED_INT");
    }
    else if (_framebufferDepthBits == 24)
    {
        if (framebufferStencilBits == 8)
        {
            _framebufferDepthDataFormat = GL_DEPTH_STENCIL;
            if (glVersion >= 30)
            {
                _framebufferDepthDataType = GL_UNSIGNED_INT_24_8;
                LogPrintf(LogSeverityLevel::Info, "OpenGL render target depth buffer: DEPTH_STENCIL/UNSIGNED_INT_24_8");
            }
            else
            {
                _framebufferDepthDataType = GL_UNSIGNED_INT;
                LogPrintf(LogSeverityLevel::Info, "OpenGL render target depth buffer: DEPTH_STENCIL/UNSIGNED_INT");
            }
        }
        else
        {
            _framebufferDepthDataFormat = GL_DEPTH_COMPONENT;
            _framebufferDepthDataType = GL_UNSIGNED_INT;
            LogPrintf(LogSeverityLevel::Info, "OpenGL render target depth buffer: DEPTH_COMPONENT/UNSIGNED_INT");
        }
    }
    else if (_framebufferDepthBits == 16)
    {
        _framebufferDepthDataFormat = GL_DEPTH_COMPONENT;
        _framebufferDepthDataType = GL_UNSIGNED_SHORT;
        LogPrintf(LogSeverityLevel::Info, "OpenGL render target depth buffer: DEPTH_COMPONENT/UNSIGNED_SHORT");
    }

    return GPUAPI_OpenGL::attachToRenderTarget();
}

bool OsmAnd::GPUAPI_OpenGL2plus::detachFromRenderTarget(bool gpuContextLost)
{
    if (!isAttachedToRenderTarget())
        return false;

    _framebufferDepthBits = 0;
    _framebufferDepthBytes = 0;
    _framebufferDepthDataFormat = 0;
    _framebufferDepthDataType = 0;

    return GPUAPI_OpenGL::detachFromRenderTarget(gpuContextLost);
}

bool OsmAnd::GPUAPI_OpenGL2plus::release(bool gpuContextLost)
{
    bool ok;

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

    ok = GPUAPI_OpenGL::release(gpuContextLost);
    if (!ok)
        return false;

    return true;
}

OsmAnd::GPUAPI_OpenGL2plus::TextureFormat OsmAnd::GPUAPI_OpenGL2plus::getTextureFormat(
    const SkColorType colorType) const
{
    if (isSupported_texture_storage)
    {
        GLenum internalFormat = GL_INVALID_ENUM;
        switch (colorType)
        {
            case SkColorType::kRGBA_8888_SkColorType:
                internalFormat = GL_RGBA8;
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

OsmAnd::GPUAPI_OpenGL2plus::TextureFormat OsmAnd::GPUAPI_OpenGL2plus::getTextureFormat_float() const
{
    if (isSupported_texture_storage)
    {
        GLenum internalFormat = GL_LUMINANCE8_EXT;
        if (isSupported_texture_float)
            internalFormat = isSupported_texture_rg ? GL_R32F : GL_LUMINANCE32F_ARB;
        else
            internalFormat = GL_LUMINANCE16;
        return TextureFormat::Make(GL_INVALID_ENUM, internalFormat);
    }

    GLenum format = GL_INVALID_ENUM;
    GLenum type = GL_INVALID_ENUM;

    if (isSupported_texture_float)
    {
        format = isSupported_texture_rg ? GL_R32F : GL_LUMINANCE32F_ARB;
        type = GL_FLOAT;
    }
    else
    {
        format = GL_LUMINANCE16;
        type = GL_UNSIGNED_SHORT;
    }

    return TextureFormat::Make(type, format);
}

size_t OsmAnd::GPUAPI_OpenGL2plus::getTextureFormatPixelSize(const TextureFormat textureFormat) const
{
    auto format = static_cast<GLenum>(textureFormat.format);
    auto type = static_cast<GLenum>(textureFormat.type);
    if ((format == GL_R32F || format == GL_LUMINANCE32F_ARB) && type == GL_FLOAT)
        return 4;
    else if (format == GL_LUMINANCE16 && type == GL_UNSIGNED_SHORT)
        return 2;

    return GPUAPI_OpenGL::getTextureFormatPixelSize(textureFormat);
}

GLenum OsmAnd::GPUAPI_OpenGL2plus::getBaseInternalTextureFormat(const TextureFormat textureFormat) const
{
    switch (textureFormat.format)
    {
        case GL_R32F:
            return GL_RED;

        case GL_LUMINANCE32F_ARB:
        case GL_LUMINANCE16:
            return GL_LUMINANCE;
    }

    return static_cast<GLenum>(textureFormat.format);
}

OsmAnd::GPUAPI_OpenGL2plus::SourceFormat OsmAnd::GPUAPI_OpenGL2plus::getSourceFormat_float() const
{
    GLenum format = GL_INVALID_ENUM;
    GLenum type = GL_INVALID_ENUM;

    if (isSupported_texture_rg)
        format = GL_RED;
    else
        format = GL_LUMINANCE;

    // NOTE: Regardless of float support, source is float
    type = GL_FLOAT;

    return SourceFormat::Make(type, format);
}

void OsmAnd::GPUAPI_OpenGL2plus::allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const TextureFormat textureFormat)
{
    if (isSupported_texture_storage)
    {
        GL_CHECK_PRESENT(glTexStorage2D);

        glTexStorage2D(target, levels, static_cast<GLenum>(textureFormat.format), width, height);
        GL_CHECK_RESULT;

        return;
    }

    GPUAPI_OpenGL::allocateTexture2D(target, levels, width, height, textureFormat);
}

void OsmAnd::GPUAPI_OpenGL2plus::uploadDataToTexture2D(
    GLenum target, GLint level,
    GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
    const GLvoid *data, GLsizei dataRowLengthInElements, GLsizei elementSize,
    const SourceFormat sourceFormat)
{
    GL_CHECK_PRESENT(glPixelStorei);
    GL_CHECK_PRESENT(glTexSubImage2D);

    // GL_UNPACK_ROW_LENGTH is supported from OpenGL 1.1+
    glPixelStorei(GL_UNPACK_ROW_LENGTH, dataRowLengthInElements);
    GL_CHECK_RESULT;

    glTexSubImage2D(target, level,
        xoffset, yoffset, width, height,
        static_cast<GLenum>(sourceFormat.format),
        static_cast<GLenum>(sourceFormat.type),
        data);
    GL_CHECK_RESULT;
}

void OsmAnd::GPUAPI_OpenGL2plus::setMipMapLevelsLimit(GLenum target, const uint32_t mipmapLevelsCount)
{
    GL_CHECK_PRESENT(glTexParameteri);

    // GL_TEXTURE_MAX_LEVEL supported from OpenGL 1.2+
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipmapLevelsCount);
    GL_CHECK_RESULT;
}

void OsmAnd::GPUAPI_OpenGL2plus::glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays)
{
    if (glVersion >= 30 || isSupported_ARB_vertex_array_object)
    {
        GL_CHECK_PRESENT(glGenVertexArrays);

        glGenVertexArrays(n, arrays);
    }
    else if (isSupported_APPLE_vertex_array_object)
    {
        GL_CHECK_PRESENT(glGenVertexArraysAPPLE);

        glGenVertexArraysAPPLE(n, arrays);
    }
}

void OsmAnd::GPUAPI_OpenGL2plus::glBindVertexArray_wrapper(GLuint array)
{
    if (glVersion >= 30 || isSupported_ARB_vertex_array_object)
    {
        GL_CHECK_PRESENT(glBindVertexArray);

        glBindVertexArray(array);
    }
    else if (isSupported_APPLE_vertex_array_object)
    {
        GL_CHECK_PRESENT(glBindVertexArrayAPPLE);

        glBindVertexArrayAPPLE(array);
    }
}

void OsmAnd::GPUAPI_OpenGL2plus::glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays)
{
    if (glVersion >= 30 || isSupported_ARB_vertex_array_object)
    {
        GL_CHECK_PRESENT(glDeleteVertexArrays);

        glDeleteVertexArrays(n, arrays);
    }
    else if (isSupported_APPLE_vertex_array_object)
    {
        GL_CHECK_PRESENT(glDeleteVertexArraysAPPLE);

        glDeleteVertexArraysAPPLE(n, arrays);
    }
}

GLsync OsmAnd::GPUAPI_OpenGL2plus::glFenceSync_wrapper(GLenum condition, GLbitfield flags)
{
    if (glVersion >= 32 || isSupported_ARB_sync)
    {
        GL_CHECK_PRESENT(glFenceSync);

        return glFenceSync(condition, flags);
    }
    else
    {
        assert(false);
    }
}

void OsmAnd::GPUAPI_OpenGL2plus::glDeleteSync_wrapper(GLsync sync)
{
    if (glVersion >= 32 || isSupported_ARB_sync)
    {
        GL_CHECK_PRESENT(glDeleteSync);

        glDeleteSync(sync);
    }
    else
    {
        assert(false);
    }
}

GLenum OsmAnd::GPUAPI_OpenGL2plus::glClientWaitSync_wrapper(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    if (glVersion >= 32 || isSupported_ARB_sync)
    {
        GL_CHECK_PRESENT(glClientWaitSync);

        return glClientWaitSync(sync, flags, timeout);
    }
    else
    {
        assert(false);
    }
}

void OsmAnd::GPUAPI_OpenGL2plus::preprocessShader(QString& code)
{
    QString shaderHeader;
    if (glslVersion >= 330)
    {
        shaderHeader = QStringLiteral(
            // Declare version of GLSL used
            "#version 330 core                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            // General definitions
            "#define INPUT in                                                                                                   ""\n"
            "#define PARAM_OUTPUT out                                                                                           ""\n"
            "#define PARAM_INPUT in                                                                                             ""\n"
            "                                                                                                                   ""\n"
            // Features definitions
            "#define VERTEX_TEXTURE_FETCH_SUPPORTED %VertexTextureFetchSupported%                                               ""\n"
            "#define TEXTURE_LOD_SUPPORTED %TextureLodSupported%                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D texture                                                                                  ""\n"
            "#define SAMPLE_TEXTURE_2D_LOD textureLod                                                                           ""\n"
            "#define INTEGER_OPERATIONS_SUPPORTED %IntegerOperationsSupported%                                                  ""\n"
            "                                                                                                                   ""\n");
    }
    else if (glslVersion >= 130)
    {
        shaderHeader = QStringLiteral(
            // Declare version of GLSL used
            "#version 130                                                                                                       ""\n"
            "                                                                                                                   ""\n"
            // General definitions
            "#define INPUT in                                                                                                   ""\n"
            "#define PARAM_OUTPUT out                                                                                           ""\n"
            "#define PARAM_INPUT in                                                                                             ""\n"
            "                                                                                                                   ""\n"
            // Features definitions
            "#define VERTEX_TEXTURE_FETCH_SUPPORTED %VertexTextureFetchSupported%                                               ""\n"
            "#define TEXTURE_LOD_SUPPORTED %TextureLodSupported%                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D texture2D                                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D_LOD texture2DLod                                                                         ""\n"
            "#define INTEGER_OPERATIONS_SUPPORTED %IntegerOperationsSupported%                                                  ""\n"
            "                                                                                                                   ""\n");
    }
    else if (glslVersion >= 110)
    {
        shaderHeader = QStringLiteral(
            // Declare version of GLSL used
            "#version 110                                                                                                       ""\n"
            "                                                                                                                   ""\n"
            // General definitions
            "#define INPUT attribute                                                                                            ""\n"
            "#define PARAM_OUTPUT varying                                                                                       ""\n"
            "#define PARAM_INPUT varying                                                                                        ""\n"
            "                                                                                                                   ""\n"
            // Precision specifying is not supported
            "#define highp                                                                                                      ""\n"
            "#define mediump                                                                                                    ""\n"
            "#define lowp                                                                                                       ""\n"
            "                                                                                                                   ""\n"
            // Features definitions
            "#define VERTEX_TEXTURE_FETCH_SUPPORTED %VertexTextureFetchSupported%                                               ""\n"
            "#define TEXTURE_LOD_SUPPORTED %TextureLodSupported%                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D texture2D                                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D_LOD texture2DLod                                                                         ""\n"
            "#define INTEGER_OPERATIONS_SUPPORTED %IntegerOperationsSupported%                                                  ""\n"
            "                                                                                                                   ""\n");
    }
    else
    {
        assert(false);
    }

    auto shaderHeaderPreprocessed = shaderHeader;
    shaderHeaderPreprocessed.replace("%VertexTextureFetchSupported%", QString::number(isSupported_vertexShaderTextureLookup ? 1 : 0));
    shaderHeaderPreprocessed.replace("%TextureLodSupported%", QString::number(isSupported_textureLod ? 1 : 0));
    shaderHeaderPreprocessed.replace("%IntegerOperationsSupported%", QString::number(isSupported_integerOperations ? 1 : 0));

    code.prepend(shaderHeaderPreprocessed);
}

void OsmAnd::GPUAPI_OpenGL2plus::preprocessVertexShader(QString& code)
{
    QString commonHeader;
    preprocessShader(commonHeader);

    QString shaderHeader;
    if (glslVersion >= 130)
    {
        // Nothing to do here
    }
    else if (glslVersion >= 110)
    {
        shaderHeader = QStringLiteral(
            "#ifdef GL_EXT_gpu_shader4                                                                                          ""\n"
            "#extension GL_EXT_gpu_shader4 : require                                                                            ""\n"
            "#endif // GL_EXT_gpu_shader4                                                                                       ""\n"
            "                                                                                                                   ""\n");
    }
    else
    {
        assert(false);
    }

    code.prepend(shaderHeader);
    code.prepend(commonHeader);
}

void OsmAnd::GPUAPI_OpenGL2plus::preprocessFragmentShader(
    QString& code, const QString& fragmentTypePrefix /*= QString()*/, const QString& fragmentTypePrecision /*= QString()*/)
{
    QString commonHeader;
    preprocessShader(commonHeader);

    QString shaderHeader;
    if (glslVersion >= 130)
    {
        shaderHeader = QStringLiteral(
            // Fragment shader output declaration
            "#define FRAGMENT_COLOR_OUTPUT out_FragColor                                                                        ""\n"
            "out %FragmentTypePrecision% %FragmentTypePrefix%vec4 out_FragColor;                                                ""\n"
            "                                                                                                                   ""\n");
    }
    else if (glslVersion >= 110)
    {
        assert(fragmentTypePrefix.isEmpty());
        assert(fragmentTypePrecision.isEmpty());
        shaderHeader = QStringLiteral(
            // Make some extensions required
            "#ifdef GL_ARB_shader_texture_lod                                                                                   ""\n"
            "#extension GL_ARB_shader_texture_lod : require                                                                     ""\n"
            "#endif // GL_ARB_shader_texture_lod                                                                                ""\n"
            "#ifdef GL_EXT_gpu_shader4                                                                                          ""\n"
            "#extension GL_EXT_gpu_shader4 : require                                                                            ""\n"
            "#endif // GL_EXT_gpu_shader4                                                                                       ""\n"
            "                                                                                                                   ""\n"
            // Fragment shader output declaration
            "#define FRAGMENT_COLOR_OUTPUT gl_FragColor                                                                         ""\n"
            "                                                                                                                   ""\n");
    }
    else
    {
        assert(false);
    }

    auto shaderHeaderPreprocessed = shaderHeader;
    shaderHeaderPreprocessed.replace("%FragmentTypePrefix%", fragmentTypePrefix);
    shaderHeaderPreprocessed.replace("%FragmentTypePrecision%", fragmentTypePrecision);

    code.prepend(shaderHeaderPreprocessed);
    code.prepend(commonHeader);
}

void OsmAnd::GPUAPI_OpenGL2plus::optimizeVertexShader(QString& code)
{
}

void OsmAnd::GPUAPI_OpenGL2plus::optimizeFragmentShader(QString& code)
{
}

void OsmAnd::GPUAPI_OpenGL2plus::setTextureBlockSampler(const GLenum textureBlock, const SamplerType samplerType)
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

void OsmAnd::GPUAPI_OpenGL2plus::applyTextureBlockToTexture(const GLenum texture, const GLenum textureBlock)
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

void OsmAnd::GPUAPI_OpenGL2plus::pushDebugGroupMarker(const QString& title)
{
    if (isSupported_GREMEDY_string_marker)
    {
        GL_CHECK_PRESENT(glStringMarkerGREMEDY);

        QString marker;
        {
            QMutexLocker scopedLocker(&_gdebuggerGroupsStackMutex);
            _gdebuggerGroupsStack.push_back(title);
            marker = _gdebuggerGroupsStack.join(QLatin1Char('/'));
        }
        marker = QLatin1String("Group begin '") + marker + QLatin1String("':");
        glStringMarkerGREMEDY(marker.length(), qPrintable(marker));
    }

    if (isSupported_EXT_debug_marker)
    {
        GL_CHECK_PRESENT(glPushGroupMarkerEXT);
        glPushGroupMarkerEXT(0, qPrintable(title));
    }
}

void OsmAnd::GPUAPI_OpenGL2plus::popDebugGroupMarker()
{
    if (isSupported_EXT_debug_marker)
    {
        GL_CHECK_PRESENT(glPopGroupMarkerEXT);
        glPopGroupMarkerEXT();
    }

    if (isSupported_GREMEDY_string_marker)
    {
        GL_CHECK_PRESENT(glStringMarkerGREMEDY);

        QString marker;
        {
            QMutexLocker scopedLocker(&_gdebuggerGroupsStackMutex);
            marker = _gdebuggerGroupsStack.join(QLatin1Char('/'));
            _gdebuggerGroupsStack.pop_back();
        }
        marker = QLatin1String("Group end '") + marker + QLatin1String("'.");
        glStringMarkerGREMEDY(marker.length(), qPrintable(marker));
    }
}

void OsmAnd::GPUAPI_OpenGL2plus::setObjectLabel(ObjectType type_, GLuint name, const QString& label)
{
    if (glVersion >= 43)
    {
        GLenum type;
        switch(type_)
        {
            case ObjectType::Buffer:
                type = GL_BUFFER;
                break;
            case ObjectType::Shader:
                type = GL_SHADER;
                break;
            case ObjectType::Program:
                type = GL_PROGRAM;
                break;
            case ObjectType::VertexArray:
                type = GL_VERTEX_ARRAY;
                break;
            case ObjectType::Query:
                type = GL_QUERY;
                break;
            case ObjectType::ProgramPipeline:
                type = GL_PROGRAM_PIPELINE;
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

        GL_CHECK_PRESENT(glObjectLabel);
        glObjectLabel(type, name, -1, qPrintable(label));
        GL_CHECK_RESULT;
    }
    else if (isSupported_EXT_debug_label)
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

void OsmAnd::GPUAPI_OpenGL2plus::glClearDepth_wrapper(float depth)
{
    GL_CHECK_PRESENT(glClearDepth);

    glClearDepth(depth);
}

void OsmAnd::GPUAPI_OpenGL2plus::readFramebufferDepth(GLint x, GLint y, GLsizei width, GLsizei height, std::vector<std::byte>& outData)
{
    GL_CHECK_PRESENT(glReadPixels);

    glReadPixels(x, y, width, height, _framebufferDepthDataFormat, _framebufferDepthDataType, outData.data());
    GL_CHECK_RESULT;
}

bool OsmAnd::GPUAPI_OpenGL2plus::pickFramebufferDepthValue(
    const std::vector<std::byte>& data, GLint x, GLint y, GLsizei width, GLsizei height, GLfloat& outValue)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return false;

    double value;
    const auto pValue = data.data() + (y * width + x) * _framebufferDepthBytes;
    if ((_framebufferDepthDataFormat == GL_DEPTH_COMPONENT || _framebufferDepthDataFormat == GL_DEPTH_STENCIL) && _framebufferDepthDataType == GL_UNSIGNED_INT)
    {
        value = static_cast<double>(*reinterpret_cast<const uint32_t*>(pValue)) / std::numeric_limits<uint32_t>::max();
    }
    else if (_framebufferDepthDataFormat == GL_DEPTH_STENCIL && _framebufferDepthDataType == GL_UNSIGNED_INT_24_8)
    {
        value = static_cast<double>(*reinterpret_cast<const uint32_t*>(pValue) >> 8) / (std::numeric_limits<uint32_t>::max() >> 8);
    }
    else if (_framebufferDepthDataFormat == GL_DEPTH_COMPONENT && _framebufferDepthDataType == GL_UNSIGNED_SHORT)
    {
        value = static_cast<double>(*reinterpret_cast<const uint16_t*>(pValue)) / std::numeric_limits<uint16_t>::max();
    }
    else
    {
        LogPrintf(LogSeverityLevel::Error,
              "Unsupported combination of framebuffer depth data format (0x%04x) and type (0x%04x)",
              _framebufferDepthDataFormat,
              _framebufferDepthDataType);
        assert(false);
        return false;
    }

    outValue = static_cast<float>(value);
    return true;
}
