#include "GPUAPI_OpenGL3.h"

#include <cassert>

#include "QtExtensions.h"
#include <QStringList>

#include <SkBitmap.h>

#include "MapRendererTypes.h"
#include "IMapRasterBitmapTileProvider.h"
#include "MapSymbolProvidersCommon.h"
#include "Logging.h"

#undef GL_CHECK_RESULT
#undef GL_GET_RESULT
#if OSMAND_DEBUG
#   define GL_CHECK_RESULT validateResult()
#   define GL_GET_RESULT validateResult()
#else
#   define GL_CHECK_RESULT
#   define GL_GET_RESULT glGetError()
#endif

OsmAnd::GPUAPI_OpenGL3::GPUAPI_OpenGL3()
    : isSupported_GREMEDY_string_marker(_isSupported_GREMEDY_string_marker)
{
}

OsmAnd::GPUAPI_OpenGL3::~GPUAPI_OpenGL3()
{
}

GLenum OsmAnd::GPUAPI_OpenGL3::validateResult()
{
    GL_CHECK_PRESENT(glGetError);

    auto result = glGetError();
    if (result == GL_NO_ERROR)
        return result;

    LogPrintf(LogSeverityLevel::Error, "OpenGL error 0x%08x : %s", result, gluErrorString(result));

    return result;
}

bool OsmAnd::GPUAPI_OpenGL3::initialize()
{
    bool ok;

    ok = GPUAPI_OpenGL::initialize();
    if (!ok)
        return false;

    glewExperimental = GL_TRUE;
    glewInit();
    // For now, silence OpenGL error here, it's inside GLEW, so it's not ours
    (void)glGetError();
    //GL_CHECK_RESULT;

    GL_CHECK_PRESENT(glGetError);
    GL_CHECK_PRESENT(glGetString);
    GL_CHECK_PRESENT(glGetStringi);
    GL_CHECK_PRESENT(glGetFloatv);
    GL_CHECK_PRESENT(glGetIntegerv);
    GL_CHECK_PRESENT(glGenSamplers);
    GL_CHECK_PRESENT(glSamplerParameteri);
    GL_CHECK_PRESENT(glSamplerParameterf);
    GL_CHECK_PRESENT(glHint);
    
    const auto glVersionString = glGetString(GL_VERSION);
    GL_CHECK_RESULT;
    GLint glVersion[2] = {0, 0};
    glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
    GL_CHECK_RESULT;
    glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "Using OpenGL version %d.%d [%s]", glVersion[0], glVersion[1], glVersionString);
    assert(glVersion[0] >= 3);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&_maxTextureSize));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture size %dx%d", _maxTextureSize, _maxTextureSize);

    GLint maxTextureUnitsInFragmentShader;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnitsInFragmentShader);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture units in fragment shader %d", maxTextureUnitsInFragmentShader);
    assert(maxTextureUnitsInFragmentShader >= RasterMapLayersCount);

    GLint maxTextureUnitsInVertexShader;
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxTextureUnitsInVertexShader);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture units in vertex shader %d", maxTextureUnitsInVertexShader);
    //////////////////////////////////////////////////////////////////////////
    //NOTE: for testing
    //maxTextureUnitsInVertexShader = 0;
    //////////////////////////////////////////////////////////////////////////
    _isSupported_vertexShaderTextureLookup = (maxTextureUnitsInVertexShader >= 1);

    GLint maxTextureUnitsCombined;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnitsCombined);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture units (combined) %d", maxTextureUnitsCombined);

    // According to http://www.opengl.org/wiki/GLSL_Uniform ("Implementation limits") , this will give incorrect results for AMD/ATI
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &_maxVertexUniformVectors);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal 4-component parameters in vertex shader %d", _maxVertexUniformVectors);

    GLint maxVertexUniformComponents;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniformComponents);
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal parameters in vertex shader %d", maxVertexUniformComponents);
    _maxVertexUniformVectors = maxVertexUniformComponents / 4; // Workaround for AMD/ATI (see above)

    // According to http://www.opengl.org/wiki/GLSL_Uniform ("Implementation limits") , this will give incorrect results for AMD/ATI
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &_maxFragmentUniformVectors);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal 4-component parameters in fragment shader %d", _maxFragmentUniformVectors);

    GLint maxFragmentUniformComponents;
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxFragmentUniformComponents);
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal parameters in fragment shader %d", maxFragmentUniformComponents);
    _maxFragmentUniformVectors = maxFragmentUniformComponents / 4; // Workaround for AMD/ATI (see above)

    GLint maxUniformLocations;
    glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &maxUniformLocations);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal defined parameters %d", maxUniformLocations);

    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    GL_CHECK_RESULT;
    _extensions.clear();
    for(auto extensionIdx = 0; extensionIdx < numExtensions; extensionIdx++)
    {
        const auto& extension = QLatin1String(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, extensionIdx)));
        GL_CHECK_RESULT;

        _extensions.push_back(extension);
    }
    LogPrintf(LogSeverityLevel::Info, "OpenGL extensions: %s", qPrintable(extensions.join(' ')));
    _isSupported_textureLod = true; // textureLod() is supported by GLSL 1.30+ specification (which is supported by OpenGL 3.0+)
    _isSupported_texturesNPOT = true; // OpenGL 2.0+ fully supports NPOT textures
    _isSupported_GREMEDY_string_marker = extensions.contains("GL_GREMEDY_string_marker");
    _isSupported_EXT_debug_marker = extensions.contains("GL_EXT_debug_marker");

    GLint compressedFormatsLength = 0;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &compressedFormatsLength);
    GL_CHECK_RESULT;
    _compressedFormats.resize(compressedFormatsLength);
    if (compressedFormatsLength > 0)
    {
        glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, _compressedFormats.data());
        GL_CHECK_RESULT;
    }
    _isSupported_8bitPaletteRGBA8 = extensions.contains("GL_OES_compressed_paletted_texture") || compressedFormats.contains(GL_PALETTE8_RGBA8_OES);
    LogPrintf(LogSeverityLevel::Info, "OpenGL 8-bit palette RGBA8 textures: %s", isSupported_8bitPaletteRGBA8 ? "supported" : "not supported");

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

    return true;
}

bool OsmAnd::GPUAPI_OpenGL3::release()
{
    bool ok;

    GL_CHECK_PRESENT(glDeleteSamplers);

    if (_textureSamplers[0] != 0)
    {
        glDeleteSamplers(1, _textureSamplers.data());
        GL_CHECK_RESULT;
        _textureSamplers.fill(0);
    }
    
    ok = GPUAPI_OpenGL::release();
    if (!ok)
        return false;

    return true;
}

OsmAnd::GPUAPI::TextureFormat OsmAnd::GPUAPI_OpenGL3::getTextureFormat(const std::shared_ptr< const MapTile >& tile)
{
    GLenum textureFormat = GL_INVALID_ENUM;

    if (tile->dataType == MapTileDataType::Bitmap)
    {
        const auto& bitmapTile = std::static_pointer_cast<const MapBitmapTile>(tile);

        switch (bitmapTile->bitmap->getConfig())
        {
        case SkBitmap::Config::kARGB_8888_Config:
            textureFormat = GL_RGBA8;
            break;
        case SkBitmap::Config::kARGB_4444_Config:
            textureFormat = GL_RGBA4;
            break;
        case SkBitmap::Config::kRGB_565_Config:
            textureFormat = GL_RGB565;
            break;
        }
    }
    else if (tile->dataType == MapTileDataType::ElevationData)
    {
        textureFormat = GL_R32F;
    }

    assert(textureFormat != GL_INVALID_ENUM);

    return static_cast<TextureFormat>(textureFormat);
}

OsmAnd::GPUAPI::TextureFormat OsmAnd::GPUAPI_OpenGL3::getTextureFormat(const std::shared_ptr< const MapSymbol >& symbol)
{
    GLenum textureFormat = GL_INVALID_ENUM;

    switch(symbol->bitmap->getConfig())
    {
    case SkBitmap::Config::kARGB_8888_Config:
        textureFormat = GL_RGBA8;
        break;
    case SkBitmap::Config::kARGB_4444_Config:
        textureFormat = GL_RGBA4;
        break;
    case SkBitmap::Config::kRGB_565_Config:
        textureFormat = GL_RGB565;
        break;
    }
    
    assert(textureFormat != GL_INVALID_ENUM);

    return static_cast<TextureFormat>(textureFormat);
}

OsmAnd::GPUAPI::SourceFormat OsmAnd::GPUAPI_OpenGL3::getSourceFormat(const std::shared_ptr< const MapTile >& tile)
{
    SourceFormat sourceFormat;
    sourceFormat.format = GL_INVALID_ENUM;
    sourceFormat.type = GL_INVALID_ENUM;
    if (tile->dataType == MapTileDataType::Bitmap)
    {
        const auto& bitmapTile = std::static_pointer_cast<const MapBitmapTile>(tile);

        switch(bitmapTile->bitmap->getConfig())
        {
        case SkBitmap::Config::kARGB_8888_Config:
            sourceFormat.format = GL_RGBA;
            sourceFormat.type = GL_UNSIGNED_BYTE;
            break;
        case SkBitmap::Config::kARGB_4444_Config:
            sourceFormat.format = GL_RGBA;
            sourceFormat.type = GL_UNSIGNED_SHORT_4_4_4_4;
            break;
        case SkBitmap::Config::kRGB_565_Config:
            sourceFormat.format = GL_RGB;
            sourceFormat.type = GL_UNSIGNED_SHORT_5_6_5;
            break;
        }
    }
    else if (tile->dataType == MapTileDataType::ElevationData)
    {
        sourceFormat.format = GL_RED;
        sourceFormat.type = GL_FLOAT;
    }

    return sourceFormat;
}

OsmAnd::GPUAPI::SourceFormat OsmAnd::GPUAPI_OpenGL3::getSourceFormat(const std::shared_ptr< const MapSymbol >& symbol)
{
    SourceFormat sourceFormat;
    sourceFormat.format = GL_INVALID_ENUM;
    sourceFormat.type = GL_INVALID_ENUM;

    switch(symbol->bitmap->getConfig())
    {
    case SkBitmap::Config::kARGB_8888_Config:
        sourceFormat.format = GL_RGBA;
        sourceFormat.type = GL_UNSIGNED_BYTE;
        break;
    case SkBitmap::Config::kARGB_4444_Config:
        sourceFormat.format = GL_RGBA;
        sourceFormat.type = GL_UNSIGNED_SHORT_4_4_4_4;
        break;
    case SkBitmap::Config::kRGB_565_Config:
        sourceFormat.format = GL_RGB;
        sourceFormat.type = GL_UNSIGNED_SHORT_5_6_5;
        break;
    }

    return sourceFormat;
}

void OsmAnd::GPUAPI_OpenGL3::allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const TextureFormat format)
{
    GLenum textureFormat = static_cast<GLenum>(format);

    glTexStorage2D(target, levels, textureFormat, width, height);
    GL_CHECK_RESULT;
}

void OsmAnd::GPUAPI_OpenGL3::uploadDataToTexture2D(
    GLenum target, GLint level,
    GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
    const GLvoid *data, GLsizei dataRowLengthInElements, GLsizei elementSize,
    const SourceFormat sourceFormat)
{
    GL_CHECK_PRESENT(glPixelStorei);
    GL_CHECK_PRESENT(glTexSubImage2D);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, dataRowLengthInElements);
    GL_CHECK_RESULT;

    glTexSubImage2D(target, level,
        xoffset, yoffset, width, height,
        static_cast<GLenum>(sourceFormat.format),
        static_cast<GLenum>(sourceFormat.type),
        data);
    GL_CHECK_RESULT;
}

void OsmAnd::GPUAPI_OpenGL3::setMipMapLevelsLimit( GLenum target, const uint32_t mipmapLevelsCount )
{
    GL_CHECK_PRESENT(glTexParameteri);

    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipmapLevelsCount);
    GL_CHECK_RESULT;
}

void OsmAnd::GPUAPI_OpenGL3::glGenVertexArrays_wrapper( GLsizei n, GLuint* arrays )
{
    GL_CHECK_PRESENT(glGenVertexArrays);

    glGenVertexArrays(n, arrays);
}

void OsmAnd::GPUAPI_OpenGL3::glBindVertexArray_wrapper( GLuint array )
{
    GL_CHECK_PRESENT(glBindVertexArray);

    glBindVertexArray(array);
}

void OsmAnd::GPUAPI_OpenGL3::glDeleteVertexArrays_wrapper( GLsizei n, const GLuint* arrays )
{
    GL_CHECK_PRESENT(glDeleteVertexArrays);

    glDeleteVertexArrays(n, arrays);
}

void OsmAnd::GPUAPI_OpenGL3::preprocessShader( QString& code )
{
    const auto& shaderSource = QString::fromLatin1(
        // Declare version of GLSL used
        "#version 430 core                                                                                                  ""\n"
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
        "                                                                                                                   ""\n");

    auto shaderSourcePreprocessed = shaderSource;
    shaderSourcePreprocessed.replace("%VertexTextureFetchSupported%", QString::number(isSupported_vertexShaderTextureLookup ? 1 : 0));
    shaderSourcePreprocessed.replace("%TextureLodSupported%", QString::number(isSupported_textureLod ? 1 : 0));

    code.prepend(shaderSourcePreprocessed);
}

void OsmAnd::GPUAPI_OpenGL3::preprocessVertexShader( QString& code )
{
    preprocessShader(code);
}

void OsmAnd::GPUAPI_OpenGL3::preprocessFragmentShader( QString& code )
{
    QString common;
    preprocessShader(common);

    const auto& shaderSource = QLatin1String(
        // Fragment shader output declaration
        "#define FRAGMENT_COLOR_OUTPUT out_FragColor                                                                        ""\n"
        "out vec4 out_FragColor;                                                                                            ""\n"
        "                                                                                                                   ""\n");

    code.prepend(shaderSource);
    code.prepend(common);
}

void OsmAnd::GPUAPI_OpenGL3::optimizeVertexShader( QString& code )
{
}

void OsmAnd::GPUAPI_OpenGL3::optimizeFragmentShader( QString& code )
{
}

void OsmAnd::GPUAPI_OpenGL3::setTextureBlockSampler(const GLenum textureBlock, const SamplerType samplerType)
{
    GL_CHECK_PRESENT(glBindSampler);

    glBindSampler(textureBlock - GL_TEXTURE0, _textureSamplers[static_cast<int>(samplerType)]);
    GL_CHECK_RESULT;
}

void OsmAnd::GPUAPI_OpenGL3::applyTextureBlockToTexture(const GLenum texture, const GLenum textureBlock)
{
    // In OpenGL 3.0+ there's nothing to do here
}

void OsmAnd::GPUAPI_OpenGL3::glPushGroupMarkerEXT_wrapper(GLsizei length, const GLchar* marker)
{
    GL_CHECK_PRESENT(glPushGroupMarkerEXT);
    glPushGroupMarkerEXT(length, marker);
}

void OsmAnd::GPUAPI_OpenGL3::glPopGroupMarkerEXT_wrapper()
{
    GL_CHECK_PRESENT(glPopGroupMarkerEXT);
    glPopGroupMarkerEXT();
}

void OsmAnd::GPUAPI_OpenGL3::pushDebugGroupMarker(const QString& title)
{
    if (isSupported_GREMEDY_string_marker)
    {
        GL_CHECK_PRESENT(glStringMarkerGREMEDY);

        QString marker;
        {
            QMutexLocker scopedLocker(&_gdebuggerGroupsStackMutex);
            _gdebuggerGroupsStack.push_back(title);
            marker = _gdebuggerGroupsStack.join(QChar('/'));
        }
        marker = QLatin1String("Group begin '") + marker + QLatin1String("':");
        glStringMarkerGREMEDY(marker.length(), qPrintable(marker));
    }
    GPUAPI_OpenGL::pushDebugGroupMarker(title);
}

void OsmAnd::GPUAPI_OpenGL3::popDebugGroupMarker()
{
    GPUAPI_OpenGL::popDebugGroupMarker();
    if (isSupported_GREMEDY_string_marker)
    {
        GL_CHECK_PRESENT(glStringMarkerGREMEDY);

        QString marker;
        {
            QMutexLocker scopedLocker(&_gdebuggerGroupsStackMutex);
            marker = _gdebuggerGroupsStack.join(QChar('/'));
            _gdebuggerGroupsStack.pop_back();
        }
        marker = QLatin1String("Group end '") + marker + QLatin1String("'.");
        glStringMarkerGREMEDY(marker.length(), qPrintable(marker));
    }
}
