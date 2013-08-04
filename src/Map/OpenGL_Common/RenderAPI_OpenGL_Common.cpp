#include "RenderAPI_OpenGL_Common.h"

#include <assert.h>

#include <QtMath>

#include <SkBitmap.h>
#include <SkCanvas.h>

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
    , _isSupported_vertexShaderTextureLookup(false)
    , extensions(_extensions)
    , compressedFormats(_compressedFormats)
    , maxTextureSize(_maxTextureSize)
    , isSupported_vertexShaderTextureLookup(_isSupported_vertexShaderTextureLookup)
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
            LogPrintf(LogSeverityLevel::Error, "Failed to compile GLSL shader:\n%s", log);
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
            LogPrintf(LogSeverityLevel::Error, "Failed to link GLSL program:\n%s", log);
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
    LogPrintf(LogSeverityLevel::Info, "GLSL program %d has %d input variable(s)", program, attributesCount);

    GLint uniformsCount;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniformsCount);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "GLSL program %d has %d parameter variable(s)", program, uniformsCount);

    return program;
}

void OsmAnd::RenderAPI_OpenGL_Common::clearVariablesLookup()
{
    _programVariables.clear();
}

void OsmAnd::RenderAPI_OpenGL_Common::findVariableLocation( GLuint program, GLint& location, const QString& name, const GLShaderVariableType& type )
{
    GL_CHECK_PRESENT(glGetAttribLocation);
    GL_CHECK_PRESENT(glGetUniformLocation);

    if(type == GLShaderVariableType::In)
        location = glGetAttribLocation(program, name.toStdString().c_str());
    else if(type == GLShaderVariableType::Uniform)
        location = glGetUniformLocation(program, name.toStdString().c_str());
    GL_CHECK_RESULT;
    if(location == -1)
        LogPrintf(LogSeverityLevel::Error, "Variable '%s' (%s) was not found in GLSL program %d", name.toStdString().c_str(), type == GLShaderVariableType::In ? "In" : "Uniform", program);
    assert(location != -1);
    assert(!_programVariables[program].contains(type, location));
    _programVariables[program].insert(type, location);
}

bool OsmAnd::RenderAPI_OpenGL_Common::uploadTileToGPU( const TileId& tileId, const ZoomLevel& zoom, const std::shared_ptr< IMapTileProvider::Tile >& tile, const uint32_t& tilesPerAtlasTextureLimit, std::shared_ptr< ResourceInGPU >& resourceInGPU )
{
    // Upload bitmap tiles
    if(tile->type == IMapTileProvider::Type::Bitmap)
    {
        return uploadTileAsTextureToGPU(tileId, zoom, tile, tilesPerAtlasTextureLimit, resourceInGPU);
    }
    else if(tile->type == IMapTileProvider::Type::ElevationData)
    {
        if(isSupported_vertexShaderTextureLookup)
        {
            return uploadTileAsTextureToGPU(tileId, zoom, tile, tilesPerAtlasTextureLimit, resourceInGPU);
        }
        else
        {
            return uploadTileAsArrayBufferToGPU(tile, resourceInGPU);
        }
    }

    return false;
}

bool OsmAnd::RenderAPI_OpenGL_Common::releaseResourceInGPU( const ResourceInGPU::Type& type, const RefInGPU& refInGPU )
{
    switch (type)
    {
    case ResourceInGPU::Type::Texture:
        {
            GL_CHECK_PRESENT(glDeleteTextures);

            GLuint texture = static_cast<GLuint>(reinterpret_cast<intptr_t>(refInGPU));
            glDeleteTextures(1, &texture);
            GL_CHECK_RESULT;

            return true;
        }
    case ResourceInGPU::Type::ArrayBuffer:
        {
            GL_CHECK_PRESENT(glDeleteBuffers);

            GLuint arrayBuffer = static_cast<GLuint>(reinterpret_cast<intptr_t>(refInGPU));
            glDeleteBuffers(1, &arrayBuffer);
            GL_CHECK_RESULT;

            return true;
        }
    }

    return false;
}

bool OsmAnd::RenderAPI_OpenGL_Common::uploadTileAsTextureToGPU( const TileId& tileId, const ZoomLevel& zoom, const std::shared_ptr< IMapTileProvider::Tile >& tile, const uint32_t& tilesPerAtlasTextureLimit, std::shared_ptr< ResourceInGPU >& resourceInGPU )
{
    GL_CHECK_PRESENT(glGenTextures);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glGenerateMipmap);
    GL_CHECK_PRESENT(glTexParameteri);

    // Depending on tile type, determine texture properties
    uint32_t tileSize = -1;
    uint32_t basePadding = 0;
    size_t sourcePixelByteSize = 0;
    bool generateMipmap = false;
    bool paletteTexture = false;
    if(tile->type == IMapTileProvider::Bitmap)
    {
        auto bitmapTile = static_cast<IMapBitmapTileProvider::Tile*>(tile.get());

        switch (bitmapTile->bitmap->getConfig())
        {
        case SkBitmap::Config::kARGB_8888_Config:
            sourcePixelByteSize = 4;
            break;
        case SkBitmap::Config::kARGB_4444_Config:
        case SkBitmap::Config::kRGB_565_Config:
            sourcePixelByteSize = 2;
            break;
        case SkBitmap::Config::kIndex8_Config:
            sourcePixelByteSize = 1;
            paletteTexture = true;
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
        sourcePixelByteSize = 4;
        basePadding = 0;
        generateMipmap = false;
    }

    // Get texture size
    uint32_t textureSize = tileSize;
    const auto maxTilesPerAtlas = tilesPerAtlasTextureLimit;
    if(maxTilesPerAtlas == 0)
    {
        textureSize = _maxTextureSize;
    }
    else if(maxTilesPerAtlas > 1)
    {
        auto idealSize = maxTilesPerAtlas * (tileSize + 2*basePadding*MipmapLodLevelsMax);
        const auto largerSize = Utilities::getNextPowerOfTwo(idealSize);
        const auto smallerSize = largerSize >> 1;

        // If larger texture is more than ideal over 15%, select reduced size
        if(static_cast<float>(largerSize) > idealSize * 1.15f)
            textureSize = qMin(static_cast<uint32_t>(_maxTextureSize), smallerSize);
        else
            textureSize = qMin(static_cast<uint32_t>(_maxTextureSize), largerSize);
    }
    else if(Utilities::getNextPowerOfTwo(tileSize) != tileSize) // Tile is of NPOT size
    {
        textureSize = Utilities::getNextPowerOfTwo(tileSize + 2*basePadding*MipmapLodLevelsMax);
    }

    // Get number of mipmap levels
    uint32_t mipmapLevels = 1;
    if(generateMipmap)
    {
        mipmapLevels += qLn(textureSize) / M_LN2;
        if(mipmapLevels > MipmapLodLevelsMax)
            mipmapLevels = MipmapLodLevelsMax;
    }

    // Find out if tile will be uploaded to atlas or not. Atlases are used in following cases:
    // 1. Atlases usage is requested.
    // 2. Atlases usage is not requested, but tile size is NPOT. In this case only 1 tile will be placed on a texture that is next POT
    const bool uploadToAtlas = (textureSize != tileSize);
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

        if(!paletteTexture)
        {
            // Allocate data
            allocateTexture2D(GL_TEXTURE_2D, mipmapLevels, tileSize, tileSize, tile);

            // Upload data
            uploadDataToTexture2D(GL_TEXTURE_2D, 0,
                0, 0, (GLsizei)tileSize, (GLsizei)tileSize,
                tile->data, tile->rowLength / sourcePixelByteSize,
                tile);

            // Set maximal mipmap level
            setMipMapLevelsLimit(GL_TEXTURE_2D, mipmapLevels - 1);

            // Generate mipmap levels
            if(mipmapLevels > 1)
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
            bitmapTile->bitmap->deepCopyTo(&decompressedTexture, SkBitmap::Config::kARGB_8888_Config);

            // Generate mipmaps
            decompressedTexture.buildMipMap();
            auto generatedLevels = decompressedTexture.extractMipLevel(nullptr, SK_FixedMax, SK_FixedMax);
            if(mipmapLevels > generatedLevels)
                mipmapLevels = generatedLevels;

            LogPrintf(LogSeverityLevel::Info, "colors = %d", bitmapTile->bitmap->getColorTable()->count());

            // For each mipmap level starting from 0, copy data to packed array
            // and prepare merged palette
            for(auto mipmapLevel = 1; mipmapLevel < mipmapLevels; mipmapLevel++)
            {
                SkBitmap mipmapLevelData;

                auto test22 = decompressedTexture.extractMipLevel(&mipmapLevelData, SK_Fixed1<<mipmapLevel, SK_Fixed1<<mipmapLevel);

                SkBitmap recomressed;
                recomressed.setConfig(SkBitmap::kIndex8_Config, tileSize >> mipmapLevel, tileSize >> mipmapLevel);
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
        auto textureInGPU = new TextureInGPU(this, reinterpret_cast<RefInGPU>(texture), tileSize, mipmapLevels);
        resourceInGPU.reset(static_cast<ResourceInGPU*>(textureInGPU));

        return true;
    }

    //NOTE: No support for palette atlas textures
    assert(!paletteTexture);
    
    // If we've got here, this tile has to be uploaded to an atlas texture, so
    // find that atlas texture to upload to

    // Find proper atlas textures pool by format of texture and full size of tile (including padding)
    AtlasTypeId atlasTypeId;
    atlasTypeId.format = getTileTextureFormat(tile);
    atlasTypeId.tileSize = tileSize;
    atlasTypeId.tilePadding = basePadding;
    const auto& atlasTexturesPool = obtainAtlasTexturesPool(atlasTypeId);
    if(!atlasTexturesPool)
        return false;

    // Get free slot from that pool
    const auto& tileInGPU = allocateTile(atlasTexturesPool,
        [this, textureSize, mipmapLevels, atlasTexturesPool, tile]() -> AtlasTextureInGPU*
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
            allocateTexture2D(GL_TEXTURE_2D, mipmapLevels, textureSize, textureSize, tile);
            GL_CHECK_RESULT;

            // Set maximal mipmap level
            setMipMapLevelsLimit(GL_TEXTURE_2D, mipmapLevels - 1);

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
    const auto tileRowLengthInPixels = tile->rowLength / sourcePixelByteSize;

    if(atlasTexture->padding > 0)
    {
        // Fill corners
        const auto pixelsCount = atlasTexture->padding * atlasTexture->padding;
        const size_t cornerDataSize = sourcePixelByteSize * pixelsCount;
        uint8_t* pCornerData = new uint8_t[cornerDataSize];
        const GLvoid* pCornerPixel;

        // Top-left corner
        pCornerPixel = tile->data;
        for(auto idx = 0u; idx < pixelsCount; idx++)
            memcpy(pCornerData + (sourcePixelByteSize * idx), pCornerPixel, sourcePixelByteSize);
        uploadDataToTexture2D(GL_TEXTURE_2D, 0,
            xOffset, yOffset, atlasTexture->padding, atlasTexture->padding,
            pCornerData, 0,
            tile);
        GL_CHECK_RESULT;

        // Top-right corner
        pCornerPixel = reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * sourcePixelByteSize;
        for(auto idx = 0u; idx < pixelsCount; idx++)
            memcpy(pCornerData + (sourcePixelByteSize * idx), pCornerPixel, sourcePixelByteSize);
        uploadDataToTexture2D(GL_TEXTURE_2D, 0,
            xOffset + atlasTexture->padding + tileSize, yOffset, atlasTexture->padding, atlasTexture->padding,
            pCornerData, 0,
            tile);
        GL_CHECK_RESULT;

        // Bottom-left corner
        pCornerPixel = reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * tile->rowLength;
        for(auto idx = 0u; idx < pixelsCount; idx++)
            memcpy(pCornerData + (sourcePixelByteSize * idx), pCornerPixel, sourcePixelByteSize);
        uploadDataToTexture2D(GL_TEXTURE_2D, 0,
            xOffset, yOffset + atlasTexture->padding + tileSize, atlasTexture->padding, atlasTexture->padding,
            pCornerData, 0,
            tile);
        GL_CHECK_RESULT;

        // Bottom-right corner
        pCornerPixel = reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * tile->rowLength + (tileSize - 1) * sourcePixelByteSize;
        for(auto idx = 0u; idx < pixelsCount; idx++)
            memcpy(pCornerData + (sourcePixelByteSize * idx), pCornerPixel, sourcePixelByteSize);
        uploadDataToTexture2D(GL_TEXTURE_2D, 0,
            xOffset + atlasTexture->padding + tileSize, yOffset + atlasTexture->padding + tileSize, atlasTexture->padding, atlasTexture->padding,
            pCornerData, 0,
            tile);
        GL_CHECK_RESULT;

        delete[] pCornerData;

        // Left column duplicate
        for(auto idx = 0u; idx < atlasTexture->padding; idx++)
        {
            uploadDataToTexture2D(GL_TEXTURE_2D, 0,
                xOffset + idx, yOffset + atlasTexture->padding, 1, (GLsizei)tileSize,
                tile->data, tileRowLengthInPixels,
                tile);
            GL_CHECK_RESULT;
        }

        // Top row duplicate
        for(auto idx = 0u; idx < atlasTexture->padding; idx++)
        {
            uploadDataToTexture2D(GL_TEXTURE_2D, 0,
                xOffset + atlasTexture->padding, yOffset + idx, (GLsizei)tileSize, 1,
                tile->data, tileRowLengthInPixels,
                tile);
            GL_CHECK_RESULT;
        }

        // Right column duplicate
        for(auto idx = 0u; idx < atlasTexture->padding; idx++)
        {
            uploadDataToTexture2D(GL_TEXTURE_2D, 0,
                xOffset + atlasTexture->padding + tileSize + idx, yOffset + atlasTexture->padding, 1, (GLsizei)tileSize,
                reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * sourcePixelByteSize, tileRowLengthInPixels,
                tile);
            GL_CHECK_RESULT;
        }

        // Bottom row duplicate
        for(auto idx = 0u; idx < atlasTexture->padding; idx++)
        {
            uploadDataToTexture2D(GL_TEXTURE_2D, 0,
                xOffset + atlasTexture->padding, yOffset + atlasTexture->padding + tileSize + idx, (GLsizei)tileSize, 1,
                reinterpret_cast<const uint8_t*>(tile->data) + (tileSize - 1) * tile->rowLength, tileRowLengthInPixels,
                tile);
            GL_CHECK_RESULT;
        }
    }

    // Main data
    uploadDataToTexture2D(GL_TEXTURE_2D, 0,
        xOffset + atlasTexture->padding, yOffset + atlasTexture->padding, (GLsizei)tileSize, (GLsizei)tileSize,
        tile->data, tileRowLengthInPixels,
        tile);
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

bool OsmAnd::RenderAPI_OpenGL_Common::uploadTileAsArrayBufferToGPU( const std::shared_ptr< IMapTileProvider::Tile >& tile, std::shared_ptr< ResourceInGPU >& resourceInGPU )
{
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);

    assert(tile->type == IMapTileProvider::Type::ElevationData);
    auto elevationTile = static_cast<IMapElevationDataProvider::Tile*>(tile.get());
    
    // Create array buffer
    GLuint buffer;
    glGenBuffers(1, &buffer);
    GL_CHECK_RESULT;

    // Bind it
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    GL_CHECK_RESULT;

    // Upload data
    const auto itemsCount = tile->width * tile->height;
    assert(tile->width*sizeof(float) == tile->rowLength);
    glBufferData(GL_ARRAY_BUFFER, itemsCount * sizeof(float), tile->data, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    // Unbind it
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    auto arrayBufferInGPU = new ArrayBufferInGPU(this, reinterpret_cast<RefInGPU>(buffer), itemsCount);
    resourceInGPU.reset(static_cast<ResourceInGPU*>(arrayBufferInGPU));

    return true;
}
