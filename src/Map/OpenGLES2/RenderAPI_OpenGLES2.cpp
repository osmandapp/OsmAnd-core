#include "RenderAPI_OpenGLES2.h"

#include <assert.h>

#include <QRegExp>
#include <QStringList>

#include <SkBitmap.h>

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

#if !defined(OSMAND_TARGET_OS_ios)
OsmAnd::RenderAPI_OpenGLES2::P_glTexStorage2DEXT_PROC OsmAnd::RenderAPI_OpenGLES2::glTexStorage2DEXT = nullptr;
#endif //!OSMAND_TARGET_OS_ios

OsmAnd::RenderAPI_OpenGLES2::RenderAPI_OpenGLES2()
    : glesExtensions(_glesExtensions)
    , isSupported_EXT_unpack_subimage(_isSupported_EXT_unpack_subimage)
    , isSupported_EXT_texture_storage(_isSupported_EXT_texture_storage)
    , isSupported_APPLE_texture_max_level(_isSupported_APPLE_texture_max_level)
    , isSupported_OES_vertex_array_object(_isSupported_OES_vertex_array_object)
    , isSupported_OES_rgb8_rgba8(_isSupported_OES_rgb8_rgba8)
    , isSupported_OES_texture_float(_isSupported_OES_texture_float)
    , isSupported_EXT_texture_rg(_isSupported_EXT_texture_rg)
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
    LogPrintf(LogSeverityLevel::Error, "OpenGLES2 error 0x%08x : %s\n", result, errorString);

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
    LogPrintf(LogSeverityLevel::Info, "Using OpenGLES2 version %s\n", glVersionString);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&_maxTextureSize));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture size %dx%d\n", _maxTextureSize, _maxTextureSize);

    GLint maxTextureUnitsInFragmentShader;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnitsInFragmentShader);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture units in fragment shader %d\n", maxTextureUnitsInFragmentShader);
    assert(maxTextureUnitsInFragmentShader >= (MapTileLayerIdsCount - MapTileLayerId::RasterMap));

    GLint maxTextureUnitsInVertexShader;
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxTextureUnitsInVertexShader);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture units in vertex shader %d\n", maxTextureUnitsInVertexShader);
    _isSupported_vertexShaderTextureLookup = maxTextureUnitsInVertexShader >= MapTileLayerId::RasterMap;

    GLint maxFragmentUniformVectors;
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &maxFragmentUniformVectors);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal 4-component parameters in fragment shader %d\n", maxFragmentUniformVectors);

    GLint maxVertexUniformVectors;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxVertexUniformVectors);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal 4-component parameters in vertex shader %d\n", maxVertexUniformVectors);

    const auto& glesExtensionsString = QString::fromLatin1(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 extensions: %s\n", qPrintable(glesExtensionsString));
    _glesExtensions = glesExtensionsString.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if(!_glesExtensions.contains("GL_OES_vertex_array_object"))
    {
        LogPrintf(LogSeverityLevel::Error, "This device does not support required 'GL_OES_vertex_array_object' extension");
        return false;
    }
    _isSupported_OES_vertex_array_object = true;
    if(!_glesExtensions.contains("GL_OES_rgb8_rgba8"))
    {
        LogPrintf(LogSeverityLevel::Error, "This device does not support required 'GL_OES_rgb8_rgba8' extension");
        return false;
    }
    _isSupported_OES_rgb8_rgba8 = true;
    if(!_glesExtensions.contains("GL_OES_texture_float"))
    {
        LogPrintf(LogSeverityLevel::Error, "This device does not support required 'GL_OES_texture_float' extension");
        return false;
    }
    _isSupported_OES_texture_float = true;
    _isSupported_EXT_texture_rg = _glesExtensions.contains("GL_EXT_texture_rg");
    _isSupported_EXT_unpack_subimage = _glesExtensions.contains("GL_EXT_unpack_subimage");
    _isSupported_EXT_texture_storage = _glesExtensions.contains("GL_EXT_texture_storage");
    _isSupported_APPLE_texture_max_level = _glesExtensions.contains("GL_APPLE_texture_max_level");
    _isSupported_EXT_shader_texture_lod = _glesExtensions.contains("GL_EXT_shader_texture_lod");
#if !defined(OSMAND_TARGET_OS_ios)
    if(_isSupported_EXT_texture_storage && !glTexStorage2DEXT)
    {
        glTexStorage2DEXT = static_cast<P_glTexStorage2DEXT_PROC>(eglGetProcAddress(glTexStorage2DEXT));
        assert(glTexStorage2DEXT);
    }
#endif // !OSMAND_TARGET_OS_ios

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    GL_CHECK_RESULT;

    glClearDepthf(1.0f);
    GL_CHECK_RESULT;

    glEnable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;

    glDepthFunc(GL_LEQUAL);
    GL_CHECK_RESULT;

    glEnable(GL_BLEND);
    GL_CHECK_RESULT;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
    else if(format == GL_RGBA && type == GL_UNSIGNED_BYTE)
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

void OsmAnd::RenderAPI_OpenGLES2::preprocessVertexShader( QString& code )
{
    const QString vertexShader = QString::fromLatin1(
        // Declare version of GLSL used
        "#version 100                                                                                                       ""\n"
        "                                                                                                                   ""\n"
        // General definitions
        "#define INPUT attribute                                                                                            ""\n"
        "#define OUTPUT varying                                                                                             ""\n"
        "                                                                                                                   ""\n"
        "                                                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Set default precisions
        "precision highp float;                                                                                             ""\n"
        "precision highp int;                                                                                               ""\n"
        "precision highp sampler2D;                                                                                         ""\n"
        );

    "#ifdef GL_EXT_shader_texture_lod                                                                                   ""\n"
        "    #extension GL_EXT_shader_texture_lod : enable                                                                  ""\n"
        "#endif                                                                                                             ""\n"
}

void OsmAnd::RenderAPI_OpenGLES2::preprocessFragmentShader( QString& code )
{

}
