#include "RenderAPI_OpenGL_Common.h"

#include <assert.h>

#include <QtMath>

#include "IMapRenderer.h"
#include "IMapTileProvider.h"
#include "IMapBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "OsmAndCore/Logging.h"
#include "OsmAndCore/Utilities.h"

#undef GL_CHECK_RESULT
#undef GL_GET_RESULT
#if defined(_DEBUG) || defined(DEBUG)
#   define GL_CHECK_RESULT validateResult()
#   define GL_GET_RESULT validateResult()
#else
#   define GL_CHECK_RESULT
#   define GL_GET_RESULT glGetError()
#endif

OsmAnd::RenderAPI_OpenGL_Common::RenderAPI_OpenGL_Common()
    : _maxTextureSize(0)
    , maxTextureSize(_maxTextureSize)
{
}

OsmAnd::RenderAPI_OpenGL_Common::~RenderAPI_OpenGL_Common()
{
}

GLuint OsmAnd::RenderAPI_OpenGL_Common::compileShader( GLenum shaderType, const char* source )
{
    GL_CHECK_PRESENT(glCreateShader);
    GL_CHECK_PRESENT(glShaderSource);
    GL_CHECK_PRESENT(glCompileShader);
    GL_CHECK_PRESENT(glGetShaderiv);
    GL_CHECK_PRESENT(glGetShaderInfoLog);
    GL_CHECK_PRESENT(glDeleteShader);

    GLuint shader;

    shader = glCreateShader(shaderType);
    GL_CHECK_RESULT;

    const GLint sourceLen = static_cast<GLint>(strlen(source));
    glShaderSource(shader, 1, &source, &sourceLen);
    GL_CHECK_RESULT;

    glCompileShader(shader);
    GL_CHECK_RESULT;

    // Check if compiled
    GLint didCompile;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &didCompile);
    GL_CHECK_RESULT;
    if(didCompile != GL_TRUE)
    {
        GLint logBufferLen = 0;
        GLsizei logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logBufferLen);
        GL_CHECK_RESULT;
        if (logBufferLen > 1)
        {
            GLchar* log = (GLchar*)malloc(logBufferLen);
            glGetShaderInfoLog(shader, logBufferLen, &logLen, log);
            GL_CHECK_RESULT;
            assert(logLen + 1 == logBufferLen);
            LogPrintf(LogSeverityLevel::Error, "Failed to compile GLSL shader:\n%s\n", log);
            free(log);
        }

        glDeleteShader(shader);
        shader = 0;
        return shader;
    }

    return shader;
}

GLuint OsmAnd::RenderAPI_OpenGL_Common::linkProgram( GLuint shadersCount, GLuint *shaders )
{
    GL_CHECK_PRESENT(glCreateProgram);
    GL_CHECK_PRESENT(glAttachShader);
    GL_CHECK_PRESENT(glLinkProgram);
    GL_CHECK_PRESENT(glGetProgramiv);
    GL_CHECK_PRESENT(glGetProgramInfoLog);
    GL_CHECK_PRESENT(glDeleteProgram);

    GLuint program = 0;

    program = glCreateProgram();
    GL_CHECK_RESULT;

    for(auto shaderIdx = 0u; shaderIdx < shadersCount; shaderIdx++)
    {
        glAttachShader(program, shaders[shaderIdx]);
        GL_CHECK_RESULT;
    }

    glLinkProgram(program);
    GL_CHECK_RESULT;

    GLint linkSuccessful;
    glGetProgramiv(program, GL_LINK_STATUS, &linkSuccessful);
    GL_CHECK_RESULT;
    if(linkSuccessful != GL_TRUE)
    {
        GLint logBufferLen = 0;
        GLsizei logLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logBufferLen);
        GL_CHECK_RESULT;
        if (logBufferLen > 1)
        {
            GLchar* log = (GLchar*)malloc(logBufferLen);
            glGetProgramInfoLog(program, logBufferLen, &logLen, log);
            GL_CHECK_RESULT;
            assert(logLen + 1 == logBufferLen);
            LogPrintf(LogSeverityLevel::Error, "Failed to link GLSL program:\n%s\n", log);
            free(log);
        }

        glDeleteProgram(program);
        GL_CHECK_RESULT;
        program = 0;
        return program;
    }

    // Show some info
    GLint attributesCount;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attributesCount);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "GLSL program %d has %d input variable(s)\n", program, attributesCount);

    GLint uniformsCount;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniformsCount);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "GLSL program %d has %d parameter variable(s)\n", program, uniformsCount);

    return program;
}

void OsmAnd::RenderAPI_OpenGL_Common::glTexSubImage2D_wrapperEx(
    GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    const GLvoid *pixels,
    GLsizei rowLengthInPixels /*= 0*/)
{
    GL_CHECK_PRESENT(glPixelStorei);
    GL_CHECK_PRESENT(glTexSubImage2D);

    // Set texture data row length (including stride) in pixels
    glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLengthInPixels);
    GL_CHECK_RESULT;

    // Upload data
    glTexSubImage2D(target, level,
        xoffset, yoffset, width, height,
        format, type,
        pixels);
    GL_CHECK_RESULT;

    // Reset texture data row length (including stride) in pixels
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    GL_CHECK_RESULT;
}

void OsmAnd::RenderAPI_OpenGL_Common::clearVariablesLookup()
{
    _programVariables.clear();
}

void OsmAnd::RenderAPI_OpenGL_Common::findVariableLocation( GLuint program, GLint& location, const QString& name, const VariableType& type )
{
    GL_CHECK_PRESENT(glGetAttribLocation);
    GL_CHECK_PRESENT(glGetUniformLocation);

    if(type == VariableType::In)
        location = glGetAttribLocation(program, name.toStdString().c_str());
    else if(type == VariableType::Uniform)
        location = glGetUniformLocation(program, name.toStdString().c_str());
    GL_CHECK_RESULT;
    if(location == -1)
        LogPrintf(LogSeverityLevel::Error, "Variable '%s' (%s) was not found in GLSL program %d\n", name.toStdString().c_str(), type == In ? "In" : "Uniform", program);
    assert(location != -1);
    assert(!_programVariables[program].contains(type, location));
    _programVariables[program].insert(type, location);
}

bool OsmAnd::RenderAPI_OpenGL_Common::uploadTileToGPU( const TileId& tileId, const ZoomLevel& zoom, const std::shared_ptr< IMapTileProvider::Tile >& tile, std::shared_ptr< ResourceInGPU >& resourceInGPU )
{
    if(tile->type == IMapTileProvider::Type::Bitmap || tile->type == IMapTileProvider::Type::ElevationData)
    {
        return uploadTileAsTextureToGPU(tileId, zoom, tile, resourceInGPU);
    }

    return false;
}

bool OsmAnd::RenderAPI_OpenGL_Common::releaseResourceInGPU( const ResourceInGPU::Type& type, const RefInGPU& refInGPU )
{
    if(type == ResourceInGPU::Type::Texture)
    {
        GL_CHECK_PRESENT(glDeleteTextures);

        GLuint texture = static_cast<GLuint>(reinterpret_cast<intptr_t>(refInGPU));
        glDeleteTextures(1, &texture);
        GL_CHECK_RESULT;

        return true;
    }

    return false;
}

bool OsmAnd::RenderAPI_OpenGL_Common::uploadTileAsTextureToGPU( const TileId& tileId, const ZoomLevel& zoom, const std::shared_ptr< IMapTileProvider::Tile >& tile, std::shared_ptr< ResourceInGPU >& resourceInGPU )
{
    GL_CHECK_PRESENT(glGenTextures);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glGenerateMipmap);
    GL_CHECK_PRESENT(glTexParameteri);

    // Depending on tile type, determine texture properties
    uint32_t tileSize = -1;
    uint32_t basePadding = 0;
    GLenum textureFormat = GL_INVALID_ENUM;
    GLenum texturePixelType = GL_INVALID_ENUM;
    size_t texturePixelByteSize = 0;
    bool generateMipmap = false;
    if(tile->type == IMapTileProvider::Bitmap)
    {
        auto bitmapTile = static_cast<IMapBitmapTileProvider::Tile*>(tile.get());

        switch (bitmapTile->format)
        {
        case IMapBitmapTileProvider::RGBA_8888:
            textureFormat = GL_RGBA;
            texturePixelType = GL_UNSIGNED_BYTE;
            texturePixelByteSize = 4;
            break;
        case IMapBitmapTileProvider::RGBA_4444:
            textureFormat = GL_RGBA;
            texturePixelType = GL_UNSIGNED_SHORT_4_4_4_4;
            texturePixelByteSize = 2;
            break;
        case IMapBitmapTileProvider::RGB_565:
            textureFormat = GL_RGB;
            texturePixelType = GL_UNSIGNED_SHORT_5_6_5;
            texturePixelByteSize = 2;
            break;
        }
        tileSize = tile->width;
        assert(tile->width == tile->height);
        basePadding = BaseBitmapAtlasTilePadding;
        generateMipmap = true;
    }
    else if(tile->type == IMapTileProvider::ElevationData)
    {
        tileSize = tile->width;
        assert(tile->width == tile->height);
        textureFormat = GL_LUMINANCE;
        texturePixelType = GL_FLOAT;
        texturePixelByteSize = 4;
        basePadding = 0;
        generateMipmap = false;
    }
    assert((textureFormat >> 16) == 0);
    assert((texturePixelType >> 16) == 0);

    // Find out if tile will be uploaded to atlas or not. Atlases are used in following cases:
    // 1. Atlases usage is requested.
    // 2. Atlases usage is not requested, but tile size is NPOT. In this case only 1 tile will be placed on a texture that is next POT
    const bool uploadToAtlas = (optimalTilesPerAtlasSqrt != 1) || (Utilities::getNextPowerOfTwo(tileSize) != tileSize);
    if(!uploadToAtlas)
    {
        // Create texture id
        GLuint texture;
        glGenTextures(1, &texture);
        GL_CHECK_RESULT;
        assert(texture != 0);

        // Activate texture
        glBindTexture(GL_TEXTURE_2D, texture);
        GL_CHECK_RESULT;

        // Get number of mipmap levels
        uint32_t mipmapLevels = 1;
        if(generateMipmap)
        {
            mipmapLevels += qLn(tileSize) / M_LN2;
            if(mipmapLevels > MipmapLodLevelsMax)
                mipmapLevels = MipmapLodLevelsMax;
        }

        // Allocate data
        glTexStorage2D_wrapper(GL_TEXTURE_2D, mipmapLevels, tileSize, tileSize, textureFormat, texturePixelType);
        GL_CHECK_RESULT;

        // Upload data
        glTexSubImage2D_wrapperEx(GL_TEXTURE_2D, 0,
            0, 0, (GLsizei)tileSize, (GLsizei)tileSize,
            textureFormat,
            texturePixelType,
            tile->data,
            tile->rowLength / texturePixelByteSize);
        GL_CHECK_RESULT;

#if defined(GL_TEXTURE_MAX_LEVEL)
        // Set maximal mipmap level
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmapLevels - 1);
        GL_CHECK_RESULT;
#endif // defined(GL_TEXTURE_MAX_LEVEL)

        // Generate mipmap levels
        if(mipmapLevels > 1)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
            GL_CHECK_RESULT;
        }

        // Deselect atlas as active texture
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_RESULT;

        // Create resource-in-GPU descriptor
        auto textureInGPU = new TextureInGPU(this, reinterpret_cast<RefInGPU>(texture), tileSize, mipmapLevels);
        resourceInGPU.reset(static_cast<ResourceInGPU*>(textureInGPU));

        // Save reference to allocated resource
        _allocatedResources.push_back(resourceInGPU);

        return true;
    }

    // If we've got here, this tile has to be uploaded to an atlas texture, so
    // find that atlas texture to upload to

    // Find proper atlas textures pool by format of texture and full size of tile (including padding)
    AtlasTypeId atlasTypeId;
    atlasTypeId.format = textureFormat;
    atlasTypeId.pixelType = texturePixelType;
    atlasTypeId.tileSize = tileSize;
    atlasTypeId.tilePadding = basePadding;
    const auto& atlasTexturesPool = obtainAtlasTexturesPool(atlasTypeId);
    if(!atlasTexturesPool)
        return false;

    // Get free slot from that pool
    const auto& tileInGPU = allocateTile(atlasTexturesPool,
        [this, atlasTexturesPool, textureFormat, texturePixelType, generateMipmap]() -> AtlasTextureInGPU*
        {
            uint32_t textureSize;

            if(optimalTilesPerAtlasSqrt != 1)
            {
                auto idealSize = optimalTilesPerAtlasSqrt * (atlasTexturesPool->typeId.tileSize + 2*atlasTexturesPool->typeId.tilePadding*MipmapLodLevelsMax);
                const auto largerSize = Utilities::getNextPowerOfTwo(idealSize);
                const auto smallerSize = largerSize >> 1;

                // If larger texture is more than ideal over 15%, select reduced size
                if(static_cast<float>(largerSize) > idealSize * 1.15f)
                    textureSize = qMin(static_cast<uint32_t>(_maxTextureSize), smallerSize);
                else
                    textureSize = qMin(static_cast<uint32_t>(_maxTextureSize), largerSize);
            }
            else //if(Utilities::getNextPowerOfTwo(tileSize) != tileSize) // Tile is of NPOT size
            {
                textureSize = Utilities::getNextPowerOfTwo(atlasTexturesPool->typeId.tileSize + 2*atlasTexturesPool->typeId.tilePadding*MipmapLodLevelsMax);
            }

            uint32_t mipmapLevels = 1;
            if(generateMipmap)
            {
                mipmapLevels += qLn(textureSize) / M_LN2;
                if(mipmapLevels > MipmapLodLevelsMax)
                    mipmapLevels = MipmapLodLevelsMax;
            }

            // Allocate texture id
            GLuint texture;
            glGenTextures(1, &texture);
            GL_CHECK_RESULT;
            assert(texture != 0);

            // Select this texture
            glBindTexture(GL_TEXTURE_2D, texture);
            GL_CHECK_RESULT;

            // Allocate space for this texture
            glTexStorage2D_wrapper(GL_TEXTURE_2D, mipmapLevels, textureSize, textureSize, textureFormat, texturePixelType);
            GL_CHECK_RESULT;

#if defined(GL_TEXTURE_MAX_LEVEL)
            // Set maximal mipmap level
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmapLevels - 1);
            GL_CHECK_RESULT;
#endif // defined(GL_TEXTURE_MAX_LEVEL)

            // Deselect texture
            glBindTexture(GL_TEXTURE_2D, 0);
            GL_CHECK_RESULT;

            return new AtlasTextureInGPU(this, reinterpret_cast<RefInGPU>(texture), textureSize, mipmapLevels, atlasTexturesPool);
        });
    const auto& atlasTexture = tileInGPU->atlasTexture;

    // Upload tile to allocated slot in atlas texture
    
    // Select atlas as active texture
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(atlasTexture->refInGPU)));
    GL_CHECK_RESULT;
    
    // Tile area offset
    const auto fullTileSize = tileSize + 2 * atlasTexture->padding;
    const auto yOffset = (tileInGPU->slotIndex / atlasTexture->slotsPerSide) * fullTileSize;
    const auto xOffset = (tileInGPU->slotIndex % atlasTexture->slotsPerSide) * fullTileSize;
    const auto tileRowLengthInPixels = tile->rowLength / texturePixelByteSize;

    if(atlasTexture->padding > 0)
    {
        // Fill corners
        const auto pixelsCount = atlasTexture->padding * atlasTexture->padding;
        const size_t cornerDataSize = texturePixelByteSize * pixelsCount;
        uint8_t* pCornerData = new uint8_t[cornerDataSize];
        const GLvoid* pCornerPixel;

        // Top-left corner
        pCornerPixel = tile->data;
        for(auto idx = 0u; idx < pixelsCount; idx++)
            memcpy(pCornerData + (texturePixelByteSize * idx), pCornerPixel, texturePixelByteSize);
        glTexSubImage2D_wrapperEx(GL_TEXTURE_2D, 0,
            xOffset, yOffset, atlasTexture->padding, atlasTexture->padding,
            textureFormat, texturePixelType,
            pCornerData);
        GL_CHECK_RESULT;

        // Top-right corner
        pCornerPixel = reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * texturePixelByteSize;
        for(auto idx = 0u; idx < pixelsCount; idx++)
            memcpy(pCornerData + (texturePixelByteSize * idx), pCornerPixel, texturePixelByteSize);
        glTexSubImage2D_wrapperEx(GL_TEXTURE_2D, 0,
            xOffset + atlasTexture->padding + tileSize, yOffset, atlasTexture->padding, atlasTexture->padding,
            textureFormat, texturePixelType,
            pCornerData);
        GL_CHECK_RESULT;

        // Bottom-left corner
        pCornerPixel = reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * tile->rowLength;
        for(auto idx = 0u; idx < pixelsCount; idx++)
            memcpy(pCornerData + (texturePixelByteSize * idx), pCornerPixel, texturePixelByteSize);
        glTexSubImage2D_wrapperEx(GL_TEXTURE_2D, 0,
            xOffset, yOffset + atlasTexture->padding + tileSize, atlasTexture->padding, atlasTexture->padding,
            textureFormat, texturePixelType,
            pCornerData);
        GL_CHECK_RESULT;

        // Bottom-right corner
        pCornerPixel = reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * tile->rowLength + (tileSize - 1) * texturePixelByteSize;
        for(auto idx = 0u; idx < pixelsCount; idx++)
            memcpy(pCornerData + (texturePixelByteSize * idx), pCornerPixel, texturePixelByteSize);
        glTexSubImage2D_wrapperEx(GL_TEXTURE_2D, 0,
            xOffset + atlasTexture->padding + tileSize, yOffset + atlasTexture->padding + tileSize, atlasTexture->padding, atlasTexture->padding,
            textureFormat, texturePixelType,
            pCornerData);
        GL_CHECK_RESULT;

        delete[] pCornerData;

        // Left column duplicate
        for(auto idx = 0u; idx < atlasTexture->padding; idx++)
        {
            glTexSubImage2D_wrapperEx(GL_TEXTURE_2D, 0,
                xOffset + idx, yOffset + atlasTexture->padding, 1, (GLsizei)tileSize,
                textureFormat, texturePixelType,
                tile->data, tileRowLengthInPixels);
            GL_CHECK_RESULT;
        }

        // Top row duplicate
        for(auto idx = 0u; idx < atlasTexture->padding; idx++)
        {
            glTexSubImage2D_wrapperEx(GL_TEXTURE_2D, 0,
                xOffset + atlasTexture->padding, yOffset + idx, (GLsizei)tileSize, 1,
                textureFormat, texturePixelType,
                tile->data, tileRowLengthInPixels);
            GL_CHECK_RESULT;
        }

        // Right column duplicate
        for(auto idx = 0u; idx < atlasTexture->padding; idx++)
        {
            glTexSubImage2D_wrapperEx(GL_TEXTURE_2D, 0,
                xOffset + atlasTexture->padding + tileSize + idx, yOffset + atlasTexture->padding, 1, (GLsizei)tileSize,
                textureFormat, texturePixelType,
                reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * texturePixelByteSize, tileRowLengthInPixels);
            GL_CHECK_RESULT;
        }

        // Bottom row duplicate
        for(auto idx = 0u; idx < atlasTexture->padding; idx++)
        {
            glTexSubImage2D_wrapperEx(GL_TEXTURE_2D, 0,
                xOffset + atlasTexture->padding, yOffset + atlasTexture->padding + tileSize + idx, (GLsizei)tileSize, 1,
                textureFormat, texturePixelType,
                reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * tile->rowLength, tileRowLengthInPixels);
            GL_CHECK_RESULT;
        }
    }

    // Main data
    glTexSubImage2D_wrapperEx(GL_TEXTURE_2D, 0,
        xOffset + atlasTexture->padding, yOffset + atlasTexture->padding, (GLsizei)tileSize, (GLsizei)tileSize,
        textureFormat, texturePixelType,
        tile->data, tileRowLengthInPixels);
    GL_CHECK_RESULT;

    // [Re]generate mipmap
    if(atlasTexture->mipmapLevels > 1)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        GL_CHECK_RESULT;
    }

    // Deselect atlas as active texture
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_RESULT;

    resourceInGPU = tileInGPU;

    return true;
}
