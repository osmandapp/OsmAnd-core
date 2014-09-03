#include "GPUAPI_OpenGL.h"

#include <cassert>

#include "QtExtensions.h"
#include <QtMath>

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include <SkCanvas.h>
#include "restore_internal_warnings.h"

#include "IMapRenderer.h"
#include "IMapTiledDataProvider.h"
#include "IMapRasterBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "MapSymbol.h"
#include "RasterMapSymbol.h"
#include "VectorMapSymbol.h"
#include "Logging.h"
#include "Utilities.h"

#undef GL_CHECK_RESULT
#undef GL_GET_RESULT
#if OSMAND_DEBUG
#   define GL_CHECK_RESULT validateResult()
#   define GL_GET_RESULT validateResult()
#else
#   define GL_CHECK_RESULT
#   define GL_GET_RESULT glGetError()
#endif

OsmAnd::GPUAPI_OpenGL::GPUAPI_OpenGL()
    : _glVersion(0)
    , _glslVersion(0)
    , _maxTextureSize(0)
    , _isSupported_vertexShaderTextureLookup(false)
    , _isSupported_textureLod(false)
    , _isSupported_texturesNPOT(false)
    , _isSupported_texture_storage(false)
    , _isSupported_texture_float(false)
    , _isSupported_texture_rg(false)
    , _isSupported_vertex_array_object(false)
    , _maxVertexUniformVectors(-1)
    , _maxFragmentUniformVectors(-1)
    , glVersion(_glVersion)
    , glslVersion(_glslVersion)
    , extensions(_extensions)
    , compressedFormats(_compressedFormats)
    , maxTextureSize(_maxTextureSize)
    , isSupported_vertexShaderTextureLookup(_isSupported_vertexShaderTextureLookup)
    , isSupported_textureLod(_isSupported_textureLod)
    , isSupported_texturesNPOT(_isSupported_texturesNPOT)
    , isSupported_EXT_debug_marker(_isSupported_EXT_debug_marker)
    , isSupported_texture_storage(_isSupported_texture_storage)
    , isSupported_texture_float(_isSupported_texture_float)
    , isSupported_texture_rg(_isSupported_texture_rg)
    , isSupported_vertex_array_object(_isSupported_vertex_array_object)
    , maxVertexUniformVectors(_maxVertexUniformVectors)
    , maxFragmentUniformVectors(_maxFragmentUniformVectors)
{
}

OsmAnd::GPUAPI_OpenGL::~GPUAPI_OpenGL()
{
}

GLuint OsmAnd::GPUAPI_OpenGL::compileShader(GLenum shaderType, const char* source)
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
    if (didCompile != GL_TRUE)
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
            LogPrintf(LogSeverityLevel::Error, "Failed to compile GLSL shader:\n%s\nSources:\n%s", log, source);
            free(log);
        }

        glDeleteShader(shader);
        shader = 0;
        return shader;
    }

    return shader;
}

GLuint OsmAnd::GPUAPI_OpenGL::linkProgram(GLuint shadersCount, const GLuint* shaders, const bool autoReleaseShaders /*= true*/)
{
    GL_CHECK_PRESENT(glCreateProgram);
    GL_CHECK_PRESENT(glAttachShader);
    GL_CHECK_PRESENT(glDetachShader);
    GL_CHECK_PRESENT(glLinkProgram);
    GL_CHECK_PRESENT(glGetProgramiv);
    GL_CHECK_PRESENT(glGetProgramInfoLog);
    GL_CHECK_PRESENT(glDeleteProgram);

    GLuint program = 0;

    program = glCreateProgram();
    GL_CHECK_RESULT;

    for (auto shaderIdx = 0u; shaderIdx < shadersCount; shaderIdx++)
    {
        glAttachShader(program, shaders[shaderIdx]);
        GL_CHECK_RESULT;
    }

    glLinkProgram(program);
    GL_CHECK_RESULT;

    for (auto shaderIdx = 0u; shaderIdx < shadersCount; shaderIdx++)
    {
        glDetachShader(program, shaders[shaderIdx]);
        GL_CHECK_RESULT;

        if (autoReleaseShaders)
        {
            glDeleteShader(shaders[shaderIdx]);
            GL_CHECK_RESULT;
        }
    }

    GLint linkSuccessful;
    glGetProgramiv(program, GL_LINK_STATUS, &linkSuccessful);
    GL_CHECK_RESULT;
    if (linkSuccessful != GL_TRUE)
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

        return 0;
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

std::shared_ptr<OsmAnd::GPUAPI_OpenGL::ProgramVariablesLookupContext> OsmAnd::GPUAPI_OpenGL::obtainVariablesLookupContext(const GLuint& program)
{
    return std::shared_ptr<ProgramVariablesLookupContext>(new ProgramVariablesLookupContext(this, program));
}

void OsmAnd::GPUAPI_OpenGL::findVariableLocation(const GLuint& program, GLint& location, const QString& name, const GLShaderVariableType& type)
{
    GL_CHECK_PRESENT(glGetAttribLocation);
    GL_CHECK_PRESENT(glGetUniformLocation);

    if (type == GLShaderVariableType::In)
        location = glGetAttribLocation(program, qPrintable(name));
    else if (type == GLShaderVariableType::Uniform)
        location = glGetUniformLocation(program, qPrintable(name));
    GL_CHECK_RESULT;
    if (location == -1)
        LogPrintf(LogSeverityLevel::Error, "Variable '%s' (%s) was not found in GLSL program %d", qPrintable(name), type == GLShaderVariableType::In ? "In" : "Uniform", program);
    assert(location != -1);
}

bool OsmAnd::GPUAPI_OpenGL::uploadTileToGPU(const std::shared_ptr< const MapTiledData >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    // Upload bitmap tiles
    if (tile->dataType == MapTiledData::DataType::RasterBitmapTile)
    {
        return uploadTileAsTextureToGPU(tile, resourceInGPU);
    }
    else if (tile->dataType == MapTiledData::DataType::ElevationDataTile)
    {
        if (isSupported_vertexShaderTextureLookup)
        {
            return uploadTileAsTextureToGPU(tile, resourceInGPU);
        }
        else
        {
            return uploadTileAsArrayBufferToGPU(tile, resourceInGPU);
        }
    }

    assert(false);
    return false;
}

bool OsmAnd::GPUAPI_OpenGL::uploadSymbolToGPU(const std::shared_ptr< const MapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    if (const auto rasterMapSymbol = std::dynamic_pointer_cast<const RasterMapSymbol>(symbol))
    {
        return uploadSymbolAsTextureToGPU(rasterMapSymbol, resourceInGPU);
    }
    else if (const auto primitiveMapSymbol = std::dynamic_pointer_cast<const VectorMapSymbol>(symbol))
    {
        return uploadSymbolAsMeshToGPU(primitiveMapSymbol, resourceInGPU);
    }

    assert(false);
    return false;
}

bool OsmAnd::GPUAPI_OpenGL::releaseResourceInGPU(const ResourceInGPU::Type type, const RefInGPU& refInGPU)
{
    switch (type)
    {
        case ResourceInGPU::Type::Texture:
        {
            GL_CHECK_PRESENT(glDeleteTextures);
            GL_CHECK_PRESENT(glIsTexture);

            GLuint texture = static_cast<GLuint>(reinterpret_cast<intptr_t>(refInGPU));
#if OSMAND_DEBUG
            if (!glIsTexture(texture))
            {
                LogPrintf(LogSeverityLevel::Error, "%d is not an OpenGL texture on thread %p", texture, QThread::currentThreadId());
                return false;
            }
            GL_CHECK_RESULT;
#endif
            glDeleteTextures(1, &texture);
            GL_CHECK_RESULT;

            return true;
        }
        case ResourceInGPU::Type::ElementArrayBuffer:
        case ResourceInGPU::Type::ArrayBuffer:
        {
            GL_CHECK_PRESENT(glDeleteBuffers);
            GL_CHECK_PRESENT(glIsBuffer);

            GLuint buffer = static_cast<GLuint>(reinterpret_cast<intptr_t>(refInGPU));
#if OSMAND_DEBUG
            if (!glIsBuffer(buffer))
            {
                LogPrintf(LogSeverityLevel::Error, "%d is not an OpenGL buffer on thread %p", buffer, QThread::currentThreadId());
                return false;
            }
            GL_CHECK_RESULT;
#endif
            glDeleteBuffers(1, &buffer);
            GL_CHECK_RESULT;

            return true;
        }
    }

    return false;
}

bool OsmAnd::GPUAPI_OpenGL::uploadTileAsTextureToGPU(const std::shared_ptr< const MapTiledData >& tile_, std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    GL_CHECK_PRESENT(glGenTextures);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glGenerateMipmap);
    GL_CHECK_PRESENT(glTexParameteri);

    // Depending on tile type, determine texture properties:
    GLsizei sourcePixelByteSize = 0;
    bool mipmapGenerationSupported = false;
    bool tileUsesPalette = false;
    uint32_t tileSize = 0;
    size_t dataRowLength = 0;
    const void* tileData = nullptr;
    if (tile_->dataType == MapTiledData::DataType::RasterBitmapTile)
    {
        const auto tile = std::static_pointer_cast<const RasterBitmapTile>(tile_);

        switch (tile->bitmap->config())
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
                tileUsesPalette = true;
                break;
            default:
                assert(false);
                return false;
        }
        tileSize = tile->bitmap->width();
        dataRowLength = tile->bitmap->rowBytes();
        tileData = tile->bitmap->getPixels();

        // No need to generate mipmaps if textureLod is not supported
        mipmapGenerationSupported = isSupported_textureLod;
    }
    else if (tile_->dataType == MapTiledData::DataType::ElevationDataTile)
    {
        const auto tile = std::static_pointer_cast<const ElevationDataTile>(tile_);

        sourcePixelByteSize = 4;
        tileSize = tile->size;
        dataRowLength = tile->rowLength;
        tileData = tile->data;
        mipmapGenerationSupported = false;
    }
    else
    {
        assert(false);
        return false;
    }
    const auto textureFormat = getTextureFormat(tile_);
    const auto sourceFormat = getSourceFormat(tile_);

    // Calculate texture size. Tiles are always stored in square textures.
    // Also, since atlas-texture support for tiles was deprecated, only 1 tile per texture is allowed.

    // If tile has NPOT size, then it needs to be rounded-up to nearest POT value
    const auto tileSizePOT = Utilities::getNextPowerOfTwo(tileSize);
    const auto textureSize = (tileSizePOT != tileSize && !isSupported_texturesNPOT) ? tileSizePOT : tileSize;
    const bool useAtlasTexture = (textureSize != tileSize);

    // Get number of mipmap levels
    auto mipmapLevels = 1u;
    if (mipmapGenerationSupported)
        mipmapLevels += qLn(textureSize) / M_LN2;

    // If tile size matches size of texture, upload is quite straightforward
    if (!useAtlasTexture)
    {
        // Create texture id
        GLuint texture;
        glGenTextures(1, &texture);
        GL_CHECK_RESULT;
        assert(texture != 0);

        // Activate texture
        glBindTexture(GL_TEXTURE_2D, texture);
        GL_CHECK_RESULT;

        if (!tileUsesPalette)
        {
            // Allocate square 2D texture
            allocateTexture2D(GL_TEXTURE_2D, mipmapLevels, textureSize, textureSize, textureFormat);

            // Upload data
            uploadDataToTexture2D(GL_TEXTURE_2D, 0,
                0, 0, (GLsizei)tileSize, (GLsizei)tileSize,
                tileData, dataRowLength / sourcePixelByteSize, sourcePixelByteSize,
                sourceFormat);

            // Set maximal mipmap level
            setMipMapLevelsLimit(GL_TEXTURE_2D, mipmapLevels - 1);

            // Generate mipmap levels
            if (mipmapLevels > 1)
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
            if (mipmapLevels > generatedLevels)
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
        const auto textureInGPU = new TextureInGPU(this, reinterpret_cast<RefInGPU>(texture), textureSize, textureSize, mipmapLevels);
        resourceInGPU.reset(static_cast<ResourceInGPU*>(textureInGPU));

        return true;
    }

    // Otherwise, create an atlas texture with 1 slot only:
    //NOTE: No support for palette atlas textures
    assert(!tileUsesPalette);

    // Find proper atlas textures pool by format of texture and full size of tile (including padding)
    AtlasTypeId atlasTypeId;
    atlasTypeId.format = textureFormat;
    atlasTypeId.tileSize = tileSize;
    atlasTypeId.tilePadding = 0;
    const auto atlasTexturesPool = obtainAtlasTexturesPool(atlasTypeId);
    if (!atlasTexturesPool)
        return false;

    // Get free slot from that pool
    const auto slotInGPU = allocateTile(atlasTexturesPool,
        [this, textureSize, mipmapLevels, atlasTexturesPool, textureFormat]
        () -> AtlasTextureInGPU*
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
            allocateTexture2D(GL_TEXTURE_2D, mipmapLevels, textureSize, textureSize, textureFormat);
            GL_CHECK_RESULT;

            // Set maximal mipmap level
            setMipMapLevelsLimit(GL_TEXTURE_2D, mipmapLevels - 1);

            // Deselect texture
            glBindTexture(GL_TEXTURE_2D, 0);
            GL_CHECK_RESULT;

            return new AtlasTextureInGPU(this, reinterpret_cast<RefInGPU>(texture), textureSize, mipmapLevels, atlasTexturesPool);
        });

    // Upload tile to allocated slot in atlas texture

    // Select atlas as active texture
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(slotInGPU->atlasTexture->refInGPU)));
    GL_CHECK_RESULT;

    // Upload data
    uploadDataToTexture2D(GL_TEXTURE_2D, 0,
        0, 0, (GLsizei)tileSize, (GLsizei)tileSize,
        tileData, dataRowLength / sourcePixelByteSize, sourcePixelByteSize,
        sourceFormat);
    GL_CHECK_RESULT;

    // Generate mipmap
    if (slotInGPU->atlasTexture->mipmapLevels > 1)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        GL_CHECK_RESULT;
    }

    // Deselect atlas as active texture
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_RESULT;

    resourceInGPU = slotInGPU;

    return true;
}

bool OsmAnd::GPUAPI_OpenGL::uploadTileAsArrayBufferToGPU(const std::shared_ptr< const MapTiledData >& tile_, std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);

    if (tile_->dataType != MapTiledData::DataType::ElevationDataTile)
    {
        assert(false);
        return false;
    }
    const auto tile = std::static_pointer_cast<const ElevationDataTile>(tile_);

    // Create array buffer
    GLuint buffer;
    glGenBuffers(1, &buffer);
    GL_CHECK_RESULT;

    // Bind it
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    GL_CHECK_RESULT;

    // Upload data
    const auto itemsCount = tile->size*tile->size;
    assert(tile->size*sizeof(float) == tile->rowLength);
    glBufferData(GL_ARRAY_BUFFER, itemsCount*sizeof(float), tile->data, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    // Unbind it
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    auto arrayBufferInGPU = new ArrayBufferInGPU(this, reinterpret_cast<RefInGPU>(buffer), itemsCount);
    resourceInGPU.reset(static_cast<ResourceInGPU*>(arrayBufferInGPU));

    return true;
}

bool OsmAnd::GPUAPI_OpenGL::uploadSymbolAsTextureToGPU(const std::shared_ptr< const RasterMapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    GL_CHECK_PRESENT(glGenTextures);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glGenerateMipmap);
    GL_CHECK_PRESENT(glTexParameteri);

    // Determine texture properties:
    GLsizei sourcePixelByteSize = 0;
    bool symbolUsesPalette = false;
    switch (symbol->bitmap->config())
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
            symbolUsesPalette = true;
            break;
        default:
            LogPrintf(LogSeverityLevel::Error, "Tried to upload symbol bitmap with unsupported config %d to GPU", symbol->bitmap->config());
            assert(false);
            return false;
    }
    const auto textureFormat = getTextureFormat(symbol);

    // Symbols don't use mipmapping, so there is no difference between POT vs NPOT size of texture.
    // In OpenGLES 2.0 and OpenGL 2.0+, NPOT textures are supported in general.
    // OpenGLES 2.0 has some limitations without isSupported_texturesNPOT:
    //  - no mipmaps
    //  - only LINEAR or NEAREST minification filter.

    // Create texture id
    GLuint texture;
    glGenTextures(1, &texture);
    GL_CHECK_RESULT;
    assert(texture != 0);

    // Activate texture
    glBindTexture(GL_TEXTURE_2D, texture);
    GL_CHECK_RESULT;

    if (!symbolUsesPalette)
    {
        // Allocate square 2D texture
        allocateTexture2D(GL_TEXTURE_2D, 1, symbol->bitmap->width(), symbol->bitmap->height(), textureFormat);

        // Upload data
        uploadDataToTexture2D(GL_TEXTURE_2D, 0,
            0, 0, (GLsizei)symbol->bitmap->width(), (GLsizei)symbol->bitmap->height(),
            symbol->bitmap->getPixels(), symbol->bitmap->rowBytes() / sourcePixelByteSize, sourcePixelByteSize,
            getSourceFormat(symbol));

        // Set maximal mipmap level to 0
        setMipMapLevelsLimit(GL_TEXTURE_2D, 0);
    }
    else
    {
        //TODO: palettes are not yet supported
        assert(false);
    }

    // Deselect atlas as active texture
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_RESULT;

    // Create resource-in-GPU descriptor
    const auto textureInGPU = new TextureInGPU(this, reinterpret_cast<RefInGPU>(texture), symbol->bitmap->width(), symbol->bitmap->height(), 1);
    resourceInGPU.reset(static_cast<ResourceInGPU*>(textureInGPU));

    return true;
}

bool OsmAnd::GPUAPI_OpenGL::uploadSymbolAsMeshToGPU(const std::shared_ptr< const VectorMapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);

    // Primitive map symbol has to have vertices, so checks are worthless
    assert(symbol->vertices && symbol->verticesCount > 0);

    // Create vertex buffer
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    GL_CHECK_RESULT;

    // Bind it
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    GL_CHECK_RESULT;

    // Upload data
    glBufferData(GL_ARRAY_BUFFER, symbol->verticesCount*sizeof(VectorMapSymbol::Vertex), symbol->vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    // Unbind it
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    // Create ArrayBuffer resource
    const std::shared_ptr<ArrayBufferInGPU> vertexBufferResource(new ArrayBufferInGPU(this, reinterpret_cast<RefInGPU>(vertexBuffer), symbol->verticesCount));

    // Primitive map symbol may have no index buffer, so check if it needs to be created
    std::shared_ptr<ElementArrayBufferInGPU> indexBufferResource;
    if (symbol->indices != nullptr && symbol->indicesCount > 0)
    {
        // Create index buffer
        GLuint indexBuffer;
        glGenBuffers(1, &indexBuffer);
        GL_CHECK_RESULT;

        // Bind it
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        GL_CHECK_RESULT;

        // Upload data
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, symbol->indicesCount*sizeof(VectorMapSymbol::Index), symbol->indices, GL_STATIC_DRAW);
        GL_CHECK_RESULT;

        // Unbind it
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        GL_CHECK_RESULT;

        // Create ElementArrayBuffer resource
        indexBufferResource.reset(new ElementArrayBufferInGPU(this, reinterpret_cast<RefInGPU>(indexBuffer), symbol->indicesCount));
    }

    // Create mesh resource
    resourceInGPU.reset(new MeshInGPU(this, vertexBufferResource, indexBufferResource));

    return true;
}

void OsmAnd::GPUAPI_OpenGL::waitUntilUploadIsComplete()
{
    // glFinish won't return until all current instructions for GPU (in current context) are complete
    glFinish();
    GL_CHECK_RESULT;
}

void OsmAnd::GPUAPI_OpenGL::pushDebugGroupMarker(const QString& title)
{
    if (isSupported_EXT_debug_marker)
        glPushGroupMarkerEXT_wrapper(0, qPrintable(title));
}

void OsmAnd::GPUAPI_OpenGL::popDebugGroupMarker()
{
    if (isSupported_EXT_debug_marker)
        glPopGroupMarkerEXT_wrapper();
}

OsmAnd::GPUAPI_OpenGL::TextureFormat OsmAnd::GPUAPI_OpenGL::getTextureFormat(const std::shared_ptr< const MapTiledData >& tile)
{
    if (tile->dataType == MapTiledData::DataType::RasterBitmapTile)
    {
        const auto& bitmapTile = std::static_pointer_cast<const RasterBitmapTile>(tile);

        return getTextureFormat(bitmapTile->bitmap->config());
    }
    else if (tile->dataType == MapTiledData::DataType::ElevationDataTile)
    {
        assert(isSupported_vertexShaderTextureLookup);

        if (isSupported_texture_storage)
            return getTextureSizedFormat_float();
        
        // The only supported format
        const GLenum format = GL_LUMINANCE;
        const GLenum type = GL_UNSIGNED_BYTE;

        assert((format >> 16) == 0);
        assert((type >> 16) == 0);
        return (static_cast<TextureFormat>(format) << 16) | type;
    }

    assert(false);
    return static_cast<TextureFormat>(GL_INVALID_ENUM);
}

OsmAnd::GPUAPI_OpenGL::TextureFormat OsmAnd::GPUAPI_OpenGL::getTextureFormat(const std::shared_ptr< const RasterMapSymbol >& symbol)
{
    return getTextureFormat(symbol->bitmap->config());
}

OsmAnd::GPUAPI_OpenGL::TextureFormat OsmAnd::GPUAPI_OpenGL::getTextureFormat(const SkBitmap::Config skBitmapConfig) const
{
    // If current device supports glTexStorage2D, lets use sized format
    if (isSupported_texture_storage)
        return getTextureSizedFormat(skBitmapConfig);

    // But if glTexStorage2D is not supported, we need to fallback to pixel type and format specification
    GLenum format = GL_INVALID_ENUM;
    GLenum type = GL_INVALID_ENUM;
    switch (skBitmapConfig)
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

        default:
            assert(false);
            return static_cast<TextureFormat>(GL_INVALID_ENUM);
    }

    assert(format != GL_INVALID_ENUM);
    assert(type != GL_INVALID_ENUM);
    assert((format >> 16) == 0);
    assert((type >> 16) == 0);
    return (static_cast<TextureFormat>(format) << 16) | type;
}

OsmAnd::GPUAPI_OpenGL::SourceFormat OsmAnd::GPUAPI_OpenGL::getSourceFormat(const std::shared_ptr< const MapTiledData >& tile)
{
    if (tile->dataType == MapTiledData::DataType::RasterBitmapTile)
    {
        const auto& bitmapTile = std::static_pointer_cast<const RasterBitmapTile>(tile);
        return getSourceFormat(bitmapTile->bitmap->config());
    }
    else if (tile->dataType == MapTiledData::DataType::ElevationDataTile)
    {
        return getSourceFormat_float();
    }

    SourceFormat sourceFormat;
    sourceFormat.format = GL_INVALID_ENUM;
    sourceFormat.type = GL_INVALID_ENUM;
    return sourceFormat;
}

OsmAnd::GPUAPI_OpenGL::SourceFormat OsmAnd::GPUAPI_OpenGL::getSourceFormat(const std::shared_ptr< const RasterMapSymbol >& symbol)
{
    return getSourceFormat(symbol->bitmap->config());
}

OsmAnd::GPUAPI_OpenGL::SourceFormat OsmAnd::GPUAPI_OpenGL::getSourceFormat(const SkBitmap::Config skBitmapConfig) const
{
    SourceFormat sourceFormat;
    sourceFormat.format = GL_INVALID_ENUM;
    sourceFormat.type = GL_INVALID_ENUM;

    switch (skBitmapConfig)
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

void OsmAnd::GPUAPI_OpenGL::allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const TextureFormat encodedFormat)
{
    GLenum format = static_cast<GLenum>(encodedFormat >> 16);
    GLenum type = static_cast<GLenum>(encodedFormat & 0xFFFF);
    GLsizei pixelSizeInBytes = 0;
    if (format == GL_RGBA && type == GL_UNSIGNED_BYTE)
        pixelSizeInBytes = 4;
    else if (format == GL_RGBA && type == GL_UNSIGNED_SHORT_4_4_4_4)
        pixelSizeInBytes = 2;
    else if (format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5)
        pixelSizeInBytes = 2;
    else if (format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE)
        pixelSizeInBytes = 1;

    uint8_t* dummyBuffer = new uint8_t[width * height * pixelSizeInBytes];

    glTexImage2D(target, 0, format, width, height, 0, format, type, dummyBuffer);
    GL_CHECK_RESULT;

    delete[] dummyBuffer;
}

OsmAnd::GLname OsmAnd::GPUAPI_OpenGL::allocateUninitializedVAO()
{
    if (isSupported_vertex_array_object)
    {
        GLname vao;

        glGenVertexArrays_wrapper(1, &vao);
        GL_CHECK_RESULT;
        glBindVertexArray_wrapper(vao);
        GL_CHECK_RESULT;

        return vao;
    }

    return GLname();
}

void OsmAnd::GPUAPI_OpenGL::initializeVAO(const GLname vao)
{
    if (isSupported_vertex_array_object)
    {
        glBindVertexArray_wrapper(0);
        GL_CHECK_RESULT;

        return;
    }

    return;
}

void OsmAnd::GPUAPI_OpenGL::useVAO(const GLname vao)
{
    if (isSupported_vertex_array_object)
    {
        glBindVertexArray_wrapper(vao);
        GL_CHECK_RESULT;

        return;
    }

    return;
}

void OsmAnd::GPUAPI_OpenGL::unuseVAO()
{
    if (isSupported_vertex_array_object)
    {
        glBindVertexArray_wrapper(0);
        GL_CHECK_RESULT;

        return;
    }

    return;
}

void OsmAnd::GPUAPI_OpenGL::releaseVAO(const GLname vao)
{
    if (isSupported_vertex_array_object)
    {
        glDeleteVertexArrays_wrapper(1, &vao);
        GL_CHECK_RESULT;

        return;
    }

    return;
}

OsmAnd::GPUAPI_OpenGL::ProgramVariablesLookupContext::ProgramVariablesLookupContext(GPUAPI_OpenGL* gpuAPI_, GLuint program_)
    : gpuAPI(gpuAPI_)
    , program(program_)
{
}

OsmAnd::GPUAPI_OpenGL::ProgramVariablesLookupContext::~ProgramVariablesLookupContext()
{
}

void OsmAnd::GPUAPI_OpenGL::ProgramVariablesLookupContext::lookupLocation(GLint& location, const QString& name, const GLShaderVariableType& type)
{
    auto& variablesByName = _variablesByName[type];
    const auto itPreviousLocation = variablesByName.constFind(name);
    if (itPreviousLocation != variablesByName.constEnd())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Variable '%s' (%s) was already located in program %d at %d",
            qPrintable(name),
            type == GLShaderVariableType::In ? "In" : "Uniform",
            program,
            *itPreviousLocation);
        assert(true);
    }

    gpuAPI->findVariableLocation(program, location, name, type);

    auto& variablesByLocation = _variablesByLocation[type];
    const auto itOtherName = variablesByLocation.constFind(location);
    if (itOtherName != variablesByLocation.constEnd())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Variable '%s' (%s) in program %d was shares same location at other variable '%s'",
            qPrintable(name),
            type == GLShaderVariableType::In ? "In" : "Uniform",
            program,
            qPrintable(*itOtherName));
        assert(true);
    }
}

void OsmAnd::GPUAPI_OpenGL::ProgramVariablesLookupContext::lookupLocation(GLlocation& location, const QString& name, const GLShaderVariableType& type)
{
    lookupLocation(*location, name, type);
}
