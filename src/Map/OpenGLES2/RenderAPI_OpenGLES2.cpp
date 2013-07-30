#include "RenderAPI_OpenGLES2.h"

#include <assert.h>

#include <QRegExp>
#include <QStringList>

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

#ifndef GL_RGBA4_OES
#   if defined(GL_RGBA4)
#       define GL_RGBA4_OES GL_RGBA4
#   else
#       define GL_RGBA4_OES                                             0x8056
#   endif
#endif //!GL_RGBA4_OES

#ifndef GL_RGB5_A1_OES
#   if defined(GL_RGB5_A1)
#       define GL_RGB5_A1_OES GL_RGB5_A1
#   else
#       define GL_RGB5_A1_OES                                           0x8057
#   endif
#endif //!GL_RGB5_A1_OES

#ifndef GL_RGB565_OES
#   if defined(GL_RGB565)
#       define GL_RGB565_OES GL_RGB565
#   else
#       define GL_RGB565_OES                                            0x8D62
#   endif
#endif //!GL_RGB565_OES


/*
#if defined(OSMAND_OPENGLES2_RENDERER_SUPPORTED)
#   if !defined(GL_UNPACK_ROW_LENGTH)
#       define GL_UNPACK_ROW_LENGTH              0x0CF2
#   endif // !GL_UNPACK_ROW_LENGTH
#   if !defined(GL_UNPACK_SKIP_ROWS)
#       define GL_UNPACK_SKIP_ROWS               0x0CF3
#   endif // !GL_UNPACK_SKIP_ROWS
#   if !defined(GL_UNPACK_SKIP_PIXELS)
#       define GL_UNPACK_SKIP_PIXELS             0x0CF4
#   endif // !GL_UNPACK_SKIP_PIXELS
#   if !defined(GL_TEXTURE_MAX_LEVEL) && defined(OSMAND_TARGET_OS_ios)
#       define GL_TEXTURE_MAX_LEVEL GL_TEXTURE_MAX_LEVEL_APPLE
#   endif
#endif // OSMAND_OPENGLES2_RENDERER_SUPPORTED
*/


#if !defined(OSMAND_TARGET_OS_ios)
OsmAnd::RenderAPI_OpenGLES2::P_glTexStorage2DEXT_PROC OsmAnd::RenderAPI_OpenGLES2::glTexStorage2DEXT = nullptr;
#endif //!OSMAND_TARGET_OS_ios

OsmAnd::RenderAPI_OpenGLES2::RenderAPI_OpenGLES2()
    : glesExtensions(_glesExtensions)
    , isSupported_vertexShaderTextureLookup(_isSupported_vertexShaderTextureLookup)
    , isSupported_EXT_unpack_subimage(_isSupported_EXT_unpack_subimage)
    , isSupported_EXT_texture_storage(_isSupported_EXT_texture_storage)
    , isSupported_APPLE_texture_max_level(_isSupported_APPLE_texture_max_level)
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
    if(!_glesExtensions.contains("GL_OES_texture_float"))
    {
        LogPrintf(LogSeverityLevel::Error, "This device does not support required 'GL_OES_texture_float' extension");
        return false;
    }
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

            switch (bitmapTile->format)
            {
            case IMapBitmapTileProvider::RGBA_8888:
                textureFormat = force16bitBitmapColorDepth ? GL_RGB5_A1_OES : GL_RGBA8_OES;
                break;
            case IMapBitmapTileProvider::RGBA_4444:
                textureFormat = GL_RGBA4_OES;
                break;
            case IMapBitmapTileProvider::RGB_565:
                textureFormat = GL_RGB5_OES;
                break;
            }
        }
        else if(tile->type == IMapTileProvider::ElevationData)
        {
            textureFormat = GL_LUMINANCE8_EXT;
            /*
            //TODO:
            (if EXT_texture_rg is supported)
                R8_EXT                         0x8229 
                RG8_EXT                        0x822B

            (if EXT_texture_rg and OES_texture_float are supported)
                R32F_EXT                       0x822E
                RG32F_EXT                      0x8230

            (if EXT_texture_rg and OES_texture_half_float are supported)
                R16F_EXT                       0x822D
                RG16F_EXT                      0x822F
            */
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

        switch (bitmapTile->format)
        {
        case IMapBitmapTileProvider::RGBA_8888:
            //TODO: test if we still can convert textures into 16bit by force
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            break;
        case IMapBitmapTileProvider::RGBA_4444:
            format = GL_RGBA;
            type = GL_UNSIGNED_SHORT_4_4_4_4;
            break;
        case IMapBitmapTileProvider::RGB_565:
            format = GL_RGB;
            type = GL_UNSIGNED_SHORT_5_6_5;
            break;
        }
    }
    else if(tile->type == IMapTileProvider::ElevationData)
    {
        format = GL_LUMINANCE;
        type = GL_UNSIGNED_BYTE;
    }

    assert( (format >> 16) == 0 );
    assert( (type >> 16) == 0 );

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
    if(tile->type == IMapTileProvider::Bitmap)
    {
        auto bitmapTile = static_cast<IMapBitmapTileProvider::Tile*>(tile.get());

        switch (bitmapTile->format)
        {
        case IMapBitmapTileProvider::RGBA_8888:
            //TODO: test if we still can convert textures into 16bit by force
            pixelSizeInBytes = 4;
            break;
        case IMapBitmapTileProvider::RGBA_4444:
            pixelSizeInBytes = 2;
            break;
        case IMapBitmapTileProvider::RGB_565:
            pixelSizeInBytes = 2;
            break;
        }
    }
    else if(tile->type == IMapTileProvider::ElevationData)
    {
        pixelSizeInBytes = 1;
    }

    uint8_t* dummyBuffer = new uint8_t[width * height * pixelSizeInBytes];
    
    glTexImage2D(target, 0, sourceFormat, width, height, 0, sourceFormat, sourcePixelDataType, dummyBuffer);
    GL_CHECK_RESULT;

    delete[] dummyBuffer;
}

void OsmAnd::RenderAPI_OpenGLES2::uploadDataToTexture2D(
    GLenum target, GLint level,
    GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
    const GLvoid *data, GLsizei dataRowLengthInElements,
    const std::shared_ptr< IMapTileProvider::Tile >& fromTile )
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

            switch (bitmapTile->format)
            {
            case IMapBitmapTileProvider::RGBA_8888:
                sourceFormat = GL_RGBA;
                sourceFormatType = GL_UNSIGNED_BYTE;
                break;
            case IMapBitmapTileProvider::RGBA_4444:
                sourceFormat = GL_RGBA;
                sourceFormatType = GL_UNSIGNED_SHORT_4_4_4_4;
                break;
            case IMapBitmapTileProvider::RGB_565:
                sourceFormat = GL_RGB;
                sourceFormatType = GL_UNSIGNED_SHORT_5_6_5;
                break;
            }
        }
        else if(tile->type == IMapTileProvider::ElevationData)
        {
            sourceFormat = GL_RED;
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
    // In case our row length is 0 or equals to image width (has no extra pixels, just load as is)
    if(rowLengthInPixels == 0 || rowLengthInPixels == width)
    {
        // Upload data
        glTexSubImage2D(target, level,
            xoffset, yoffset, width, height,
            format, type,
            pixels);
        GL_CHECK_RESULT;
        return;
    }

    // Otherwise we need to or load row by row
    const auto encodedFormat = getTileTextureFormat(tile);
    GLenum format = static_cast<GLenum>(encodedFormat >> 16);
    GLenum type = static_cast<GLenum>(encodedFormat & 0xFFFF);
    GLsizei pixelSizeInBytes = 0;
    if(tile->type == IMapTileProvider::Bitmap)
    {
        auto bitmapTile = static_cast<IMapBitmapTileProvider::Tile*>(tile.get());

        switch (bitmapTile->format)
        {
        case IMapBitmapTileProvider::RGBA_8888:
            //TODO: test if we still can convert textures into 16bit by force
            pixelSizeInBytes = 4;
            break;
        case IMapBitmapTileProvider::RGBA_4444:
            pixelSizeInBytes = 2;
            break;
        case IMapBitmapTileProvider::RGB_565:
            pixelSizeInBytes = 2;
            break;
        }
    }
    else if(tile->type == IMapTileProvider::ElevationData)
    {
        pixelSizeInBytes = 1;
    }
    assert(pixelSizeInBytes != 0);
    auto pRow = reinterpret_cast<const uint8_t*>(pixels);
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
