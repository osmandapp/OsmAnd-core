#include "RenderAPI_OpenGLES2.h"

#include <assert.h>

#include <QRegExp>
#include <QStringList>

#include <SkBitmap.h>

#include <GLSLOptimizer.h>

#include "IMapBitmapTileProvider.h"
#include "Logging.h"

#undef GL_CHECK_RESULT
#undef GL_GET_RESULT
#if defined(_DEBUG) || defined(DEBUG)
#   define GL_CHECK_RESULT validateResult()
#   define GL_GET_RESULT validateResult()
#else
#   define GL_CHECK_RESULT
#   define GL_GET_RESULT glGetError()
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
OsmAnd::RenderAPI_OpenGLES2::P_glTexStorage2DEXT_PROC OsmAnd::RenderAPI_OpenGLES2::glTexStorage2DEXT = nullptr;
#endif //!OSMAND_TARGET_OS_ios

OsmAnd::RenderAPI_OpenGLES2::RenderAPI_OpenGLES2()
    : isSupported_EXT_unpack_subimage(_isSupported_EXT_unpack_subimage)
    , isSupported_EXT_texture_storage(_isSupported_EXT_texture_storage)
    , isSupported_APPLE_texture_max_level(_isSupported_APPLE_texture_max_level)
    , isSupported_OES_vertex_array_object(_isSupported_OES_vertex_array_object)
    , isSupported_OES_rgb8_rgba8(_isSupported_OES_rgb8_rgba8)
    , isSupported_OES_texture_float(_isSupported_OES_texture_float)
    , isSupported_EXT_texture_rg(_isSupported_EXT_texture_rg)
    , isSupported_EXT_shader_texture_lod(_isSupported_EXT_shader_texture_lod)
{
}

OsmAnd::RenderAPI_OpenGLES2::~RenderAPI_OpenGLES2()
{
}

GLenum OsmAnd::RenderAPI_OpenGLES2::validateResult()
{
    auto result = glGetError();
    if(result == GL_NO_ERROR)
        return result;

    const char* errorString = nullptr;
    switch(result)
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
    LogPrintf(LogSeverityLevel::Error, "OpenGLES2 error 0x%08x : %s", result, errorString);

    return result;
}

bool OsmAnd::RenderAPI_OpenGLES2::initialize()
{
    bool ok;

    ok = RenderAPI_OpenGL_Common::initialize();
    if(!ok)
        return false;

    const auto glVersionString = glGetString(GL_VERSION);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "Using OpenGLES2 version %s", glVersionString);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&_maxTextureSize));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture size %dx%d", _maxTextureSize, _maxTextureSize);

    GLint maxTextureUnitsInFragmentShader;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnitsInFragmentShader);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture units in fragment shader %d", maxTextureUnitsInFragmentShader);
    assert(maxTextureUnitsInFragmentShader >= (MapTileLayerIdsCount - MapTileLayerId::RasterMap));

    GLint maxTextureUnitsInVertexShader;
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxTextureUnitsInVertexShader);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture units in vertex shader %d", maxTextureUnitsInVertexShader);
    _isSupported_vertexShaderTextureLookup = maxTextureUnitsInVertexShader >= MapTileLayerId::RasterMap;

    GLint maxFragmentUniformVectors;
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &maxFragmentUniformVectors);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal 4-component parameters in fragment shader %d", maxFragmentUniformVectors);

    GLint maxVertexUniformVectors;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxVertexUniformVectors);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal 4-component parameters in vertex shader %d", maxVertexUniformVectors);

    const auto& extensionsString = QString::fromLatin1(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 extensions: %s", qPrintable(extensionsString));
    _extensions = extensionsString.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if(!extensions.contains("GL_OES_vertex_array_object"))
    {
        LogPrintf(LogSeverityLevel::Error, "This device does not support required 'GL_OES_vertex_array_object' extension");
        return false;
    }
    _isSupported_OES_vertex_array_object = true;
    if(!extensions.contains("GL_OES_rgb8_rgba8"))
    {
        LogPrintf(LogSeverityLevel::Error, "This device does not support required 'GL_OES_rgb8_rgba8' extension");
        return false;
    }
    _isSupported_OES_rgb8_rgba8 = true;
    if(!extensions.contains("GL_OES_texture_float"))
    {
        LogPrintf(LogSeverityLevel::Error, "This device does not support required 'GL_OES_texture_float' extension");
        return false;
    }
    _isSupported_OES_texture_float = true;
    if(!extensions.contains("GL_EXT_shader_texture_lod"))
    {
        LogPrintf(LogSeverityLevel::Error, "This device does not support required 'GL_EXT_shader_texture_lod' extension");
        return false;
    }
    _isSupported_EXT_shader_texture_lod = true;
    _isSupported_EXT_texture_rg = extensions.contains("GL_EXT_texture_rg");
    _isSupported_EXT_unpack_subimage = extensions.contains("GL_EXT_unpack_subimage");
    _isSupported_EXT_texture_storage = extensions.contains("GL_EXT_texture_storage");
    _isSupported_APPLE_texture_max_level = extensions.contains("GL_APPLE_texture_max_level");
#if !defined(OSMAND_TARGET_OS_ios)
    if(_isSupported_EXT_texture_storage && !glTexStorage2DEXT)
    {
        glTexStorage2DEXT = static_cast<P_glTexStorage2DEXT_PROC>(eglGetProcAddress(glTexStorage2DEXT));
        assert(glTexStorage2DEXT);
    }
#endif // !OSMAND_TARGET_OS_ios

    GLint compressedFormatsLength = 0;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &compressedFormatsLength);
    GL_CHECK_RESULT;
    _compressedFormats.resize(compressedFormatsLength);
    if(compressedFormatsLength > 0)
    {
        glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, _compressedFormats.data());
        GL_CHECK_RESULT;
    }
    _isSupported_8bitPaletteRGBA8 = extensions.contains("GL_OES_compressed_paletted_texture") || compressedFormats.contains(GL_PALETTE8_RGBA8_OES);
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 8-bit palette RGBA8 textures: %s", isSupported_8bitPaletteRGBA8 ? "supported" : "not supported");

    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::RenderAPI_OpenGLES2::release()
{
    bool ok;

    ok = RenderAPI_OpenGL_Common::release();
    if(!ok)
        return false;

    return true;
}

uint32_t OsmAnd::RenderAPI_OpenGLES2::getTileTextureFormat( const std::shared_ptr< IMapTileProvider::Tile >& tile )
{
    // If current device supports glTexStorage2D, lets use sized format
    if(isSupported_EXT_texture_storage)
    {
        GLenum textureFormat = GL_INVALID_ENUM;

        if(tile->type == IMapTileProvider::Bitmap)
        {
            auto bitmapTile = static_cast<IMapBitmapTileProvider::Tile*>(tile.get());

            switch (bitmapTile->bitmap->getConfig())
            {
            case SkBitmap::Config::kARGB_8888_Config:
                textureFormat = GL_RGBA8_OES;
                break;
            case SkBitmap::Config::kARGB_4444_Config:
                textureFormat = GL_RGBA4;
                break;
            case SkBitmap::Config::kRGB_565_Config:
                textureFormat = GL_RGB565;
                break;
            }
        }
        else if(tile->type == IMapTileProvider::ElevationData)
        {
            if(isSupported_vertexShaderTextureLookup)
            {
                if(isSupported_OES_texture_float && isSupported_EXT_texture_rg)
                    textureFormat = GL_R32F_EXT;
                else if(isSupported_EXT_texture_rg)
                    textureFormat = GL_R8_EXT;
                else
                    textureFormat = GL_LUMINANCE8_EXT;
            }
        }

        assert(textureFormat != GL_INVALID_ENUM);

        return static_cast<uint32_t>(textureFormat);
    }

    // But if glTexStorage2D is not supported, we need to fallback to pixel type and format specification
    GLenum format = GL_INVALID_ENUM;
    GLenum type = GL_INVALID_ENUM;
    if(tile->type == IMapTileProvider::Bitmap)
    {
        auto bitmapTile = static_cast<IMapBitmapTileProvider::Tile*>(tile.get());

        switch (bitmapTile->bitmap->getConfig())
        {
        case SkBitmap::Config::kARGB_8888_Config:
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            break;
        case SkBitmap::Config::kARGB_4444_Config:
            format = GL_RGBA;
            type = GL_UNSIGNED_SHORT_4_4_4_4;
            break;
        case SkBitmap::Config::kRGB_565_Config:
            format = GL_RGB;
            type = GL_UNSIGNED_SHORT_5_6_5;
            break;
        }
    }
    else if(tile->type == IMapTileProvider::ElevationData)
    {
        if(isSupported_vertexShaderTextureLookup)
        {
            format = GL_LUMINANCE;
            type = GL_UNSIGNED_BYTE;
        }
    }

    assert(format != GL_INVALID_ENUM);
    assert(format != GL_INVALID_ENUM);
    assert((format >> 16) == 0);
    assert((type >> 16) == 0);

    return (static_cast<uint32_t>(format) << 16) | type;
}

void OsmAnd::RenderAPI_OpenGLES2::allocateTexture2D( GLenum target, GLsizei levels, GLsizei width, GLsizei height, const std::shared_ptr< IMapTileProvider::Tile >& tile )
{
    const auto encodedFormat = getTileTextureFormat(tile);

    // Use glTexStorage2D if possible
    if(isSupported_EXT_texture_storage)
    {
        GL_CHECK_PRESENT(glTexStorage2DEXT);

        glTexStorage2DEXT(target, levels, static_cast<GLenum>(encodedFormat), width, height);
        GL_CHECK_RESULT;
        return;
    }

    // Fallback to dump allocation
    GLenum format = static_cast<GLenum>(encodedFormat >> 16);
    GLenum type = static_cast<GLenum>(encodedFormat & 0xFFFF);
    GLsizei pixelSizeInBytes = 0;
    if(format == GL_RGBA && type == GL_UNSIGNED_BYTE)
        pixelSizeInBytes = 4;
    else if(format == GL_RGBA && type == GL_UNSIGNED_SHORT_4_4_4_4)
        pixelSizeInBytes = 2;
    else if(format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5)
        pixelSizeInBytes = 2;
    else if(format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE)
        pixelSizeInBytes = 1;
    
    uint8_t* dummyBuffer = new uint8_t[width * height * pixelSizeInBytes];
    
    glTexImage2D(target, 0, format, width, height, 0, format, type, dummyBuffer);
    GL_CHECK_RESULT;

    delete[] dummyBuffer;
}

void OsmAnd::RenderAPI_OpenGLES2::uploadDataToTexture2D(
    GLenum target, GLint level,
    GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
    const GLvoid *data, GLsizei dataRowLengthInElements,
    const std::shared_ptr< IMapTileProvider::Tile >& tile )
{
    GL_CHECK_PRESENT(glTexSubImage2D);

    // Try to use glPixelStorei to unpack
    if(isSupported_EXT_unpack_subimage)
    {
        GL_CHECK_PRESENT(glPixelStorei);

        GLenum sourceFormat = GL_INVALID_ENUM;
        GLenum sourceFormatType = GL_INVALID_ENUM;
        if(tile->type == IMapTileProvider::Bitmap)
        {
            auto bitmapTile = static_cast<IMapBitmapTileProvider::Tile*>(tile.get());

            switch (bitmapTile->bitmap->getConfig())
            {
            case SkBitmap::Config::kARGB_8888_Config:
                sourceFormat = GL_RGBA;
                sourceFormatType = GL_UNSIGNED_BYTE;
                break;
            case SkBitmap::Config::kARGB_4444_Config:
                sourceFormat = GL_RGBA;
                sourceFormatType = GL_UNSIGNED_SHORT_4_4_4_4;
                break;
            case SkBitmap::Config::kRGB_565_Config:
                sourceFormat = GL_RGB;
                sourceFormatType = GL_UNSIGNED_SHORT_5_6_5;
                break;
            }
        }
        else if(tile->type == IMapTileProvider::ElevationData)
        {
            if(isSupported_EXT_texture_rg)
                sourceFormat = GL_RED_EXT;
            else
                sourceFormat = GL_LUMINANCE;
            sourceFormatType = GL_FLOAT;
        }

        glPixelStorei(GL_UNPACK_ROW_LENGTH, dataRowLengthInElements);
        GL_CHECK_RESULT;

        glTexSubImage2D(target, level,
            xoffset, yoffset, width, height,
            sourceFormat,
            sourceFormatType,
            data);
        GL_CHECK_RESULT;

        return;
    }

    // Otherwise fallback to manual unpacking
    const auto encodedFormat = getTileTextureFormat(tile);
    GLenum format = GL_INVALID_ENUM;
    GLenum type = GL_INVALID_ENUM;
    if(tile->type == IMapTileProvider::Bitmap)
    {
        auto bitmapTile = static_cast<IMapBitmapTileProvider::Tile*>(tile.get());

        switch (bitmapTile->bitmap->getConfig())
        {
        case SkBitmap::Config::kARGB_8888_Config:
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            break;
        case SkBitmap::Config::kARGB_4444_Config:
            format = GL_RGBA;
            type = GL_UNSIGNED_SHORT_4_4_4_4;
            break;
        case SkBitmap::Config::kRGB_565_Config:
            format = GL_RGB;
            type = GL_UNSIGNED_SHORT_5_6_5;
            break;
        }
    }
    else if(tile->type == IMapTileProvider::ElevationData)
    {
        if(isSupported_vertexShaderTextureLookup)
        {
            format = isSupported_EXT_texture_rg ? GL_RED_EXT : GL_LUMINANCE;
            type = GL_UNSIGNED_BYTE;
        }
    }
    
    // In case our row length is 0 or equals to image width (has no extra stride, just load as-is)
    if(dataRowLengthInElements == 0 || dataRowLengthInElements == width)
    {
        // Upload data
        glTexSubImage2D(target, level,
            xoffset, yoffset, width, height,
            format, type,
            data);
        GL_CHECK_RESULT;
        return;
    }

    // Otherwise we need to or load row by row
    GLsizei pixelSizeInBytes = 0;
    if(tile->type == IMapTileProvider::Bitmap)
    {
        auto bitmapTile = static_cast<IMapBitmapTileProvider::Tile*>(tile.get());

        switch (bitmapTile->bitmap->getConfig())
        {
        case SkBitmap::Config::kARGB_8888_Config:
            pixelSizeInBytes = 4;
            break;
        case SkBitmap::Config::kARGB_4444_Config:
        case SkBitmap::Config::kRGB_565_Config:
            pixelSizeInBytes = 2;
            break;
        }
    }
    else if(tile->type == IMapTileProvider::ElevationData)
    {
        pixelSizeInBytes = 1;
    }
    assert(pixelSizeInBytes != 0);
    auto pRow = reinterpret_cast<const uint8_t*>(data);
    for(auto rowIdx = 0; rowIdx < height; rowIdx++)
    {
        glTexSubImage2D(target, level,
            xoffset, yoffset + rowIdx, width, 1,
            format, type,
            pRow);
        GL_CHECK_RESULT;

        pRow += dataRowLengthInElements * pixelSizeInBytes;
    }
}

void OsmAnd::RenderAPI_OpenGLES2::setMipMapLevelsLimit( GLenum target, const uint32_t& mipmapLevelsCount )
{
#if defined(OSMAND_TARGET_OS_ios)
    // Limit MipMap max level if possible
    if(isSupported_APPLE_texture_max_level)
    {
        GL_CHECK_PRESENT(glTexParameteri);

        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL_APPLE, mipmapLevelsCount);
        GL_CHECK_RESULT;
    }
#endif //OSMAND_TARGET_OS_ios
}

void OsmAnd::RenderAPI_OpenGLES2::glGenVertexArrays_wrapper( GLsizei n, GLuint* arrays )
{
    GL_CHECK_PRESENT(glGenVertexArraysOES);

    glGenVertexArraysOES(n, arrays);
}

void OsmAnd::RenderAPI_OpenGLES2::glBindVertexArray_wrapper( GLuint array )
{
    GL_CHECK_PRESENT(glBindVertexArrayOES);

    glBindVertexArrayOES(array);
}

void OsmAnd::RenderAPI_OpenGLES2::glDeleteVertexArrays_wrapper( GLsizei n, const GLuint* arrays )
{
    GL_CHECK_PRESENT(glDeleteVertexArraysOES);

    glDeleteVertexArraysOES(n, arrays);
}

void OsmAnd::RenderAPI_OpenGLES2::preprocessShader( QString& code, const QString& extraHeader /*= QString()*/ )
{
    const auto& shaderHeader = QString::fromLatin1(
        // Declare version of GLSL used
        "#version 100                                                                                                       ""\n"
        "                                                                                                                   ""\n");

    const auto& shaderSource = QString::fromLatin1(
        // General definitions
        "#define INPUT attribute                                                                                            ""\n"
        "#define PARAM_OUTPUT varying                                                                                       ""\n"
        "#define PARAM_INPUT varying                                                                                        ""\n"
        "                                                                                                                   ""\n"
        // Set default precisions
        "precision highp float;                                                                                             ""\n"
        "precision highp int;                                                                                               ""\n"
        "precision highp sampler2D;                                                                                         ""\n"
        "                                                                                                                   ""\n"
        // Features definitions
        "#define VERTEX_TEXTURE_FETCH_SUPPORTED 0                                                                           ""\n"
        "#define SAMPLE_TEXTURE_2D texture2D                                                                                ""\n"
        "#define SAMPLE_TEXTURE_2D_LOD texture2DLodEXT                                                                      ""\n"
        "                                                                                                                   ""\n");

    code.prepend(shaderSource);
    code.prepend(extraHeader);
    code.prepend(shaderHeader);
}

void OsmAnd::RenderAPI_OpenGLES2::preprocessVertexShader( QString& code )
{
    preprocessShader(code);
}

void OsmAnd::RenderAPI_OpenGLES2::preprocessFragmentShader( QString& code )
{
    const auto& shaderSource = QString::fromLatin1(
        // Make some extensions required
        "#extension GL_EXT_shader_texture_lod : require                                                                     ""\n"
        "                                                                                                                   ""\n"
        // Fragment shader output declaration
        "#define FRAGMENT_COLOR_OUTPUT gl_FragColor                                                                         ""\n"
        "                                                                                                                   ""\n");

    preprocessShader(code, shaderSource);
}

void OsmAnd::RenderAPI_OpenGLES2::setSampler( GLenum texture, const SamplerType& samplerType )
{
    GL_CHECK_PRESENT(glTexParameteri);

    if(samplerType == SamplerType::ElevationDataTile)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GL_CHECK_RESULT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GL_CHECK_RESULT;
    }
    else if(samplerType == SamplerType::BitmapTile_Bilinear)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;
    }
    else if(samplerType == SamplerType::BitmapTile_BilinearMipmap)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        GL_CHECK_RESULT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;
    }
    else if(samplerType == SamplerType::BitmapTile_TrilinearMipmap)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        GL_CHECK_RESULT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;
    }
}

void OsmAnd::RenderAPI_OpenGLES2::optimizeVertexShader( QString& code )
{
    /*
    auto context = glslopt_initialize(true);

    auto optimizedShader = glslopt_optimize(context, kGlslOptShaderVertex, qPrintable(code), 0);
    if(!glslopt_get_status(optimizedShader))
    {
        LogPrintf(LogSeverityLevel::Error, "%s", glslopt_get_log(optimizedShader));
        assert(false);
    }
    code = QString::fromLocal8Bit(glslopt_get_output(optimizedShader));
    glslopt_shader_delete(optimizedShader);

    glslopt_cleanup(context);
    */
}

void OsmAnd::RenderAPI_OpenGLES2::optimizeFragmentShader( QString& code )
{
    /*
    auto context = glslopt_initialize(true);

    auto optimizedShader = glslopt_optimize(context, kGlslOptShaderFragment, qPrintable(code), 0);
    if(!glslopt_get_status(optimizedShader))
    {
        LogPrintf(LogSeverityLevel::Error, "%s", glslopt_get_log(optimizedShader));
        assert(false);
    }
    code = QString::fromLocal8Bit(glslopt_get_output(optimizedShader));
    glslopt_shader_delete(optimizedShader);

    glslopt_cleanup(context);
    */
}
