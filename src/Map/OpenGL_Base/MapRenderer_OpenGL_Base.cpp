#include "MapRenderer_OpenGL_Base.h"

#include <assert.h>

#include <QtMath>

#include "IMapRenderer.h"
#include "IMapTileProvider.h"
#include "IMapBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "OsmAndCore/Logging.h"
#include "OsmAndCore/Utilities.h"

#if 0
#   include <SkBitmap.h>
#   include <SkImageEncoder.h>
#endif

OsmAnd::MapRenderer_BaseOpenGL::MapRenderer_BaseOpenGL()
    : _maxTextureSize(0)
{
}

OsmAnd::MapRenderer_BaseOpenGL::~MapRenderer_BaseOpenGL()
{
}

GLuint OsmAnd::MapRenderer_BaseOpenGL::compileShader( GLenum shaderType, const char* source )
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

uint32_t OsmAnd::MapRenderer_BaseOpenGL::getTextureFormatId( GLenum sourceFormat, GLenum sourcePixelDataType )
{
    assert((sourceFormat >> 16) == 0);
    assert((sourcePixelDataType >> 16) == 0);

    return (sourceFormat << 16) | sourcePixelDataType;
}

GLuint OsmAnd::MapRenderer_BaseOpenGL::linkProgram( GLuint shadersCount, GLuint *shaders )
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

void OsmAnd::MapRenderer_BaseOpenGL::uploadTileToTexture(
    TileLayerId layerId,
    const TileId& tileId,
    uint32_t zoom,
    const std::shared_ptr<IMapTileProvider::Tile>& tile,
    uint64_t& outAtlasPoolId,
    void*& outTextureRef,
    int& outAtlasSlotIndex,
    size_t& outUsedMemory)
{
    GL_CHECK_PRESENT(glGenTextures);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glPixelStorei);
    GL_CHECK_PRESENT(glTexSubImage2D);
    GL_CHECK_PRESENT(glGenerateMipmap);

    auto& tileLayer = _tileLayers[layerId];

    // Depending on tile type, determine texture properties
    uint32_t tileSize = -1;
    uint32_t basePadding = 0;
    GLenum sourceFormat = GL_INVALID_ENUM;
    GLenum sourcePixelDataType = GL_INVALID_ENUM;
    size_t sourcePixelByteSize = 0;
    bool generateMipmap = false;
    if(tile->type == IMapTileProvider::Bitmap)
    {
        auto bitmapTile = static_cast<IMapBitmapTileProvider::Tile*>(tile.get());

        switch (bitmapTile->format)
        {
        case IMapBitmapTileProvider::RGBA_8888:
            sourceFormat = GL_RGBA;
            sourcePixelDataType = GL_UNSIGNED_BYTE;
            sourcePixelByteSize = 4;
            break;
        case IMapBitmapTileProvider::RGBA_4444:
            sourceFormat = GL_RGBA;
            sourcePixelDataType = GL_UNSIGNED_SHORT_4_4_4_4;
            sourcePixelByteSize = 2;
            break;
        case IMapBitmapTileProvider::RGB_565:
            sourceFormat = GL_RGB;
            sourcePixelDataType = GL_UNSIGNED_SHORT_5_6_5;
            sourcePixelByteSize = 2;
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
        sourceFormat = GL_LUMINANCE;
        sourcePixelDataType = GL_FLOAT;
        sourcePixelByteSize = 4;
        basePadding = 0;
        generateMipmap = false;
    }

    // Find out if we need atlas or not. We need atlas in following cases:
    // 1. Atlases are allowed
    // 2. Atlases are prohibited, but tile size is NPOT
    auto itAtlasPool = tileLayer._atlasTexturePools.end();
    if(_activeConfig.textureAtlasesAllowed || Utilities::getNextPowerOfTwo(tileSize) != tileSize)
    {
        // Try to find proper atlas tile id
        outAtlasPoolId = (static_cast<uint64_t>(getTextureFormatId(sourceFormat, sourcePixelDataType)) << 32) | tileSize;
        itAtlasPool = tileLayer._atlasTexturePools.find(outAtlasPoolId);
        if(itAtlasPool == tileLayer._atlasTexturePools.end())
            itAtlasPool = tileLayer._atlasTexturePools.insert(outAtlasPoolId, IMapRenderer::AtlasTexturePool());
    }

    // Configure atlas, if not yet configured
    if(itAtlasPool != tileLayer._atlasTexturePools.end() && itAtlasPool->_textureSize == 0)
    {
        auto& atlasPool = *itAtlasPool;

        if(_activeConfig.textureAtlasesAllowed)
        {
            auto idealSize = MaxTilesPerTextureAtlasSide * (tileSize + 2 * basePadding * MipmapLodLevelsMax);
            const auto largerSize = Utilities::getNextPowerOfTwo(idealSize);
            const auto smallerSize = largerSize >> 1;

            // If larger texture is more than ideal over 15%, select reduced size
            if(static_cast<float>(largerSize) > idealSize * 1.15f)
                atlasPool._textureSize = qMin(_maxTextureSize, smallerSize);
            else
                atlasPool._textureSize = qMin(_maxTextureSize, largerSize);
        }
        else //if(Utilities::getNextPowerOfTwo(tileSize) != tileSize) // Tile is of NPOT size
        {
            atlasPool._textureSize = Utilities::getNextPowerOfTwo(tileSize + 2 * basePadding * MipmapLodLevelsMax);
        }

        atlasPool._mipmapLevels = 1;
        if(generateMipmap)
        {
            atlasPool._mipmapLevels += qLn(atlasPool._textureSize) / M_LN2;
            if(atlasPool._mipmapLevels > MipmapLodLevelsMax)
                atlasPool._mipmapLevels = MipmapLodLevelsMax;
        }
        atlasPool._padding = basePadding * atlasPool._mipmapLevels;
        const auto fullTileSize = tileSize + 2 * atlasPool._padding;
        atlasPool._tilePaddingN = static_cast<float>(atlasPool._padding) / static_cast<float>(atlasPool._textureSize);
        atlasPool._tileSizeN = static_cast<float>(fullTileSize) / static_cast<float>(atlasPool._textureSize);
        atlasPool._texelSizeN = 1.0f / static_cast<float>(atlasPool._textureSize);
        atlasPool._halfTexelSizeN = 0.5f / static_cast<float>(atlasPool._textureSize);
        atlasPool._slotsPerSide = atlasPool._textureSize / fullTileSize;

        outUsedMemory = fullTileSize * fullTileSize * sourcePixelByteSize;
    }

    // Use atlas textures if possible
    if(itAtlasPool != tileLayer._atlasTexturePools.end())
    {
        auto& atlasPool = *itAtlasPool;

        GLuint atlasTexture = 0;
        int atlasSlotIndex = -1;

        // If we have freed slots on previous atlases, let's use them
        if(!atlasPool._freedSlots.isEmpty())
        {
            const auto& itFreeSlot = atlasPool._freedSlots.begin();
            atlasTexture = static_cast<GLuint>(reinterpret_cast<intptr_t>(itFreeSlot.key()));
            atlasSlotIndex = itFreeSlot.value();

            // Mark slot as occupied
            atlasPool._freedSlots.erase(itFreeSlot);
            tileLayer._textureRefCount[itFreeSlot.key()]++;
        }
        // If we've never allocated any atlas yet, or free slot is beyond allowed range - allocate new
        else if(atlasPool._lastNonFullTextureRef == nullptr || atlasPool._firstFreeSlotIndex == atlasPool._slotsPerSide * atlasPool._slotsPerSide)
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
            allocateTexture2D(GL_TEXTURE_2D, atlasPool._mipmapLevels, atlasPool._textureSize, atlasPool._textureSize, sourceFormat, sourcePixelDataType);
            GL_CHECK_RESULT;

            // Deselect texture
            glBindTexture(GL_TEXTURE_2D, 0);
            GL_CHECK_RESULT;

            atlasPool._lastNonFullTextureRef = reinterpret_cast<void*>(texture);
            atlasPool._firstFreeSlotIndex = 1;
            tileLayer._textureRefCount.insert(atlasPool._lastNonFullTextureRef, 1);

            atlasTexture = texture;
            atlasSlotIndex = 0;

            //TODO: Texture atlases still suffer from texture bleeding as as they are used far from camera
#if 0
            // In debug mode, fill entire texture with single RED color if this is bitmap
            if(tile->type == IMapTileProvider::Bitmap)
            {
                glBindTexture(GL_TEXTURE_2D, texture);
                GL_CHECK_RESULT;

                uint8_t* fillBuffer = new uint8_t[atlasPool._textureSize * atlasPool._textureSize * 4];
                for(uint32_t idx = 0; idx < atlasPool._textureSize * atlasPool._textureSize; idx++)
                {
                    *reinterpret_cast<uint32_t*>(&fillBuffer[idx * 4]) = SkPackARGB32(0xff, 0xff, 0x00, 0x00);
                }
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                GL_CHECK_RESULT;
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, atlasPool._textureSize, atlasPool._textureSize, GL_RGBA, GL_UNSIGNED_BYTE, fillBuffer);
                GL_CHECK_RESULT;
                delete[] fillBuffer;

                glBindTexture(GL_TEXTURE_2D, 0);
                GL_CHECK_RESULT;
            }
#endif
        }
        // Or let's just continue using current atlas
        else
        {
            atlasTexture = static_cast<GLuint>(reinterpret_cast<intptr_t>(atlasPool._lastNonFullTextureRef));
            atlasSlotIndex = atlasPool._firstFreeSlotIndex;
            atlasPool._firstFreeSlotIndex++;
            tileLayer._textureRefCount[atlasPool._lastNonFullTextureRef]++;
        }

        // Select atlas as active texture
        glBindTexture(GL_TEXTURE_2D, atlasTexture);
        GL_CHECK_RESULT;

        // Tile area offset
        const auto fullTileSize = tileSize + 2 * atlasPool._padding;
        const auto yOffset = (atlasSlotIndex / atlasPool._slotsPerSide) * fullTileSize;
        const auto xOffset = (atlasSlotIndex % atlasPool._slotsPerSide) * fullTileSize;

        if(atlasPool._padding > 0)
        {
            // Fill corners
            const auto pixelsCount = atlasPool._padding * atlasPool._padding;
            const size_t cornerDataSize = sourcePixelByteSize * pixelsCount;
            uint8_t* pCornerData = new uint8_t[cornerDataSize];
            const GLvoid* pCornerPixel;

            // Set stride
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            GL_CHECK_RESULT;

            // Top-left corner
            pCornerPixel = tile->data;
            for(auto idx = 0u; idx < pixelsCount; idx++)
                memcpy(pCornerData + (sourcePixelByteSize * idx), pCornerPixel, sourcePixelByteSize);
            glTexSubImage2D(GL_TEXTURE_2D, 0,
                xOffset, yOffset, atlasPool._padding, atlasPool._padding,
                sourceFormat,
                sourcePixelDataType,
                pCornerData);
            GL_CHECK_RESULT;

            // Top-right corner
            pCornerPixel = reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * sourcePixelByteSize;
            for(auto idx = 0u; idx < pixelsCount; idx++)
                memcpy(pCornerData + (sourcePixelByteSize * idx), pCornerPixel, sourcePixelByteSize);
            glTexSubImage2D(GL_TEXTURE_2D, 0,
                xOffset + atlasPool._padding + tileSize, yOffset, atlasPool._padding, atlasPool._padding,
                sourceFormat,
                sourcePixelDataType,
                pCornerData);
            GL_CHECK_RESULT;

            // Bottom-left corner
            pCornerPixel = reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * tile->rowLength;
            for(auto idx = 0u; idx < pixelsCount; idx++)
                memcpy(pCornerData + (sourcePixelByteSize * idx), pCornerPixel, sourcePixelByteSize);
            glTexSubImage2D(GL_TEXTURE_2D, 0,
                xOffset, yOffset + atlasPool._padding + tileSize, atlasPool._padding, atlasPool._padding,
                sourceFormat,
                sourcePixelDataType,
                pCornerData);
            GL_CHECK_RESULT;

            // Bottom-right corner
            pCornerPixel = reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * tile->rowLength + (tileSize - 1) * sourcePixelByteSize;
            for(auto idx = 0u; idx < pixelsCount; idx++)
                memcpy(pCornerData + (sourcePixelByteSize * idx), pCornerPixel, sourcePixelByteSize);
            glTexSubImage2D(GL_TEXTURE_2D, 0,
                xOffset + atlasPool._padding + tileSize, yOffset + atlasPool._padding + tileSize, atlasPool._padding, atlasPool._padding,
                sourceFormat,
                sourcePixelDataType,
                pCornerData);
            GL_CHECK_RESULT;

            delete[] pCornerData;

            // Set stride
            glPixelStorei(GL_UNPACK_ROW_LENGTH, tile->rowLength / sourcePixelByteSize);
            GL_CHECK_RESULT;

            // Left column duplicate
            for(auto idx = 0u; idx < atlasPool._padding; idx++)
            {
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                    xOffset + idx, yOffset + atlasPool._padding, 1, (GLsizei)tileSize,
                    sourceFormat,
                    sourcePixelDataType,
                    tile->data);
                GL_CHECK_RESULT;
            }

            // Top row duplicate
            for(auto idx = 0u; idx < atlasPool._padding; idx++)
            {
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                    xOffset + atlasPool._padding, yOffset + idx, (GLsizei)tileSize, 1,
                    sourceFormat,
                    sourcePixelDataType,
                    tile->data);
                GL_CHECK_RESULT;
            }

            // Right column duplicate
            for(auto idx = 0u; idx < atlasPool._padding; idx++)
            {
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                    xOffset + atlasPool._padding + tileSize + idx, yOffset + atlasPool._padding, 1, (GLsizei)tileSize,
                    sourceFormat,
                    sourcePixelDataType,
                    reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * sourcePixelByteSize);
                GL_CHECK_RESULT;
            }

            // Bottom row duplicate
            for(auto idx = 0u; idx < atlasPool._padding; idx++)
            {
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                    xOffset + atlasPool._padding, yOffset + atlasPool._padding + tileSize + idx, (GLsizei)tileSize, 1,
                    sourceFormat,
                    sourcePixelDataType,
                    reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * tile->rowLength);
                GL_CHECK_RESULT;
            }
        }

        // Set stride
        glPixelStorei(GL_UNPACK_ROW_LENGTH, tile->rowLength / sourcePixelByteSize);
        GL_CHECK_RESULT;

        // Main data
        glTexSubImage2D(GL_TEXTURE_2D, 0,
            xOffset + atlasPool._padding, yOffset + atlasPool._padding, (GLsizei)tileSize, (GLsizei)tileSize,
            sourceFormat,
            sourcePixelDataType,
            tile->data);
        GL_CHECK_RESULT;

        // [Re]generate mipmap
        if(atlasPool._mipmapLevels > 1)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
            GL_CHECK_RESULT;
        }

        // Set stride
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        GL_CHECK_RESULT;

#if 0
        if(tile->type == IMapTileProvider::Bitmap)
        {
            SkBitmap bitmap;
            bitmap.setConfig(SkBitmap::kARGB_8888_Config, atlasPool._textureSize, atlasPool._textureSize);
            bitmap.allocPixels();

            glPixelStorei(GL_PACK_ROW_LENGTH, bitmap.rowBytesAsPixels());
            GL_CHECK_RESULT;

            glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, bitmap.getPixels());
            GL_CHECK_RESULT;

            glPixelStorei(GL_PACK_ROW_LENGTH, 0);
            GL_CHECK_RESULT;

            SkImageEncoder::EncodeFile(QString("opengl_texture_atlas_%1.png").arg(atlasTexture).toStdString().c_str(), bitmap, SkImageEncoder::kPNG_Type, 100);
        }
#endif

        // Deselect atlas as active texture
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_RESULT;

        outTextureRef = reinterpret_cast<void*>(atlasTexture);
        outAtlasSlotIndex = atlasSlotIndex;
    }
    // If not, fall back to use of one texture per tile. This will only happen in case if atlases are prohibited and tiles are POT
    else
    {
        // Get number of mipmap levels
        uint32_t mipmapLevels = 1;
        if(generateMipmap)
        {
            mipmapLevels += qLn(tileSize) / M_LN2;
            if(mipmapLevels > MipmapLodLevelsMax)
                mipmapLevels = MipmapLodLevelsMax;
        }

        // Create texture id
        GLuint texture;
        glGenTextures(1, &texture);
        GL_CHECK_RESULT;
        assert(texture != 0);

        // Activate texture
        glBindTexture(GL_TEXTURE_2D, texture);
        GL_CHECK_RESULT;

        // Set texture packing
        glPixelStorei(GL_UNPACK_ROW_LENGTH, tile->rowLength / sourcePixelByteSize);
        GL_CHECK_RESULT;

        // Allocate data
        allocateTexture2D(GL_TEXTURE_2D, mipmapLevels, tileSize, tileSize, sourceFormat, sourcePixelDataType);
        GL_CHECK_RESULT;

        // Upload data
        glTexSubImage2D(GL_TEXTURE_2D, 0,
            0, 0, (GLsizei)tileSize, (GLsizei)tileSize,
            sourceFormat,
            sourcePixelDataType,
            tile->data);
        GL_CHECK_RESULT;

        // Generate mipmap
        if(mipmapLevels > 1)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
            GL_CHECK_RESULT;
        }

        // Set stride
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        GL_CHECK_RESULT;

        // Deselect atlas as active texture
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_RESULT;

        outAtlasPoolId = 0;
        outUsedMemory = tileSize * tileSize * sourcePixelByteSize;
        outTextureRef = reinterpret_cast<void*>(texture);
        outAtlasSlotIndex = -1;
    }
}

void OsmAnd::MapRenderer_BaseOpenGL::releaseTexture( void* textureRef )
{
    GL_CHECK_PRESENT(glDeleteTextures);

    GLuint texture = static_cast<GLuint>(reinterpret_cast<intptr_t>(textureRef));

    glDeleteTextures(1, &texture);
    GL_CHECK_RESULT;
}

void OsmAnd::MapRenderer_BaseOpenGL::findVariableLocation( GLuint program, GLint& location, const QString& name, const VariableType& type )
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
