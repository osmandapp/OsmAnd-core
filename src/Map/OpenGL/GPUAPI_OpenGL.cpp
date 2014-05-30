#include "GPUAPI_OpenGL.h"

#include <cassert>

#include "QtExtensions.h"
#include <QtMath>

#include <SkBitmap.h>
#include <SkCanvas.h>

#include "IMapRenderer.h"
#include "IMapTiledDataProvider.h"
#include "IMapRasterBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "MapSymbolProvidersCommon.h"
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
    : _maxTextureSize(0)
    , _isSupported_vertexShaderTextureLookup(false)
    , _isSupported_textureLod(false)
    , _isSupported_texturesNPOT(false)
    , _maxVertexUniformVectors(-1)
    , _maxFragmentUniformVectors(-1)
    , extensions(_extensions)
    , compressedFormats(_compressedFormats)
    , maxTextureSize(_maxTextureSize)
    , isSupported_vertexShaderTextureLookup(_isSupported_vertexShaderTextureLookup)
    , isSupported_textureLod(_isSupported_textureLod)
    , isSupported_texturesNPOT(_isSupported_texturesNPOT)
    , isSupported_EXT_debug_marker(_isSupported_EXT_debug_marker)
    , maxVertexUniformVectors(_maxVertexUniformVectors)
    , maxFragmentUniformVectors(_maxFragmentUniformVectors)
{
}

OsmAnd::GPUAPI_OpenGL::~GPUAPI_OpenGL()
{
}

GLuint OsmAnd::GPUAPI_OpenGL::compileShader( GLenum shaderType, const char* source )
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

    for(auto shaderIdx = 0u; shaderIdx < shadersCount; shaderIdx++)
    {
        glAttachShader(program, shaders[shaderIdx]);
        GL_CHECK_RESULT;
    }

    glLinkProgram(program);
    GL_CHECK_RESULT;

    for(auto shaderIdx = 0u; shaderIdx < shadersCount; shaderIdx++)
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

bool OsmAnd::GPUAPI_OpenGL::uploadTileToGPU(const std::shared_ptr< const MapTile >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    // Upload bitmap tiles
    if (tile->dataType == MapTileDataType::Bitmap)
    {
        return uploadTileAsTextureToGPU(tile, resourceInGPU);
    }
    else if (tile->dataType == MapTileDataType::ElevationData)
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

    return false;
}

bool OsmAnd::GPUAPI_OpenGL::uploadSymbolToGPU(const std::shared_ptr< const MapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    return uploadSymbolAsTextureToGPU(symbol, resourceInGPU);
}

bool OsmAnd::GPUAPI_OpenGL::releaseResourceInGPU( const ResourceInGPU::Type type, const RefInGPU& refInGPU )
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

bool OsmAnd::GPUAPI_OpenGL::uploadTileAsTextureToGPU(const std::shared_ptr< const MapTile >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    GL_CHECK_PRESENT(glGenTextures);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glGenerateMipmap);
    GL_CHECK_PRESENT(glTexParameteri);

    // Depending on tile type, determine texture properties:
    GLsizei sourcePixelByteSize = 0;
    bool mipmapGenerationSupported = false;
    bool tileUsesPalette = false;
    if (tile->dataType == MapTileDataType::Bitmap)
    {
        const auto bitmapTile = std::static_pointer_cast<const MapBitmapTile>(tile);

        switch(bitmapTile->bitmap->getConfig())
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

        // No need to generate mipmaps if textureLod is not supported
        mipmapGenerationSupported = isSupported_textureLod;
    }
    else if (tile->dataType == MapTileDataType::ElevationData)
    {
        sourcePixelByteSize = 4;
        mipmapGenerationSupported = false;
    }
    else
    {
        assert(false);
        return false;
    }
    const auto textureFormat = getTextureFormat(tile);
        
    // Calculate texture size. Tiles are always stored in square textures.
    // Also, since atlas-texture support for tiles was deprecated, only 1 tile per texture is allowed.

    // If tile has NPOT size, then it needs to be rounded-up to nearest POT value
    const auto tileSizePOT = Utilities::getNextPowerOfTwo(tile->size);
    const auto textureSize = (tileSizePOT != tile->size && !isSupported_texturesNPOT) ? tileSizePOT : tile->size;
    const bool useAtlasTexture = (textureSize != tile->size);

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
                0, 0, (GLsizei)tile->size, (GLsizei)tile->size,
                tile->data, tile->rowLength / sourcePixelByteSize, sourcePixelByteSize,
                getSourceFormat(tile));

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
    atlasTypeId.format = getTextureFormat(tile);
    atlasTypeId.tileSize = tile->size;
    atlasTypeId.tilePadding = 0;
    const auto atlasTexturesPool = obtainAtlasTexturesPool(atlasTypeId);
    if (!atlasTexturesPool)
        return false;

    // Get free slot from that pool
    const auto slotInGPU = allocateTile(atlasTexturesPool,
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
            allocateTexture2D(GL_TEXTURE_2D, mipmapLevels, textureSize, textureSize, getTextureFormat(tile));
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
        0, 0, (GLsizei)tile->size, (GLsizei)tile->size,
        tile->data, tile->rowLength / sourcePixelByteSize, sourcePixelByteSize,
        getSourceFormat(tile));
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

bool OsmAnd::GPUAPI_OpenGL::uploadTileAsArrayBufferToGPU(const std::shared_ptr< const MapTile >& tile, std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);

    if (tile->dataType != MapTileDataType::ElevationData)
    {
        assert(false);
        return false;
    }
    const auto elevationTile = std::static_pointer_cast<const ElevationDataTile>(tile);
    
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

bool OsmAnd::GPUAPI_OpenGL::uploadSymbolAsTextureToGPU(const std::shared_ptr< const MapSymbol >& symbol, std::shared_ptr< const ResourceInGPU >& resourceInGPU)
{
    GL_CHECK_PRESENT(glGenTextures);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glGenerateMipmap);
    GL_CHECK_PRESENT(glTexParameteri);

    // Determine texture properties:
    GLsizei sourcePixelByteSize = 0;
    bool symbolUsesPalette = false;
    switch(symbol->bitmap->getConfig())
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
        assert(false);
        return false;
    }
    const auto textureFormat = getTextureFormat(symbol);

    // Symbols don't use mipmapping, so there is no difference between POT vs NPOT size of texture.
    // In OpenGLES 2.0 and OpenGL 3.0+, NPOT textures are supported in general.
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
