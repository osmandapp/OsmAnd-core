#include "AtlasMapRendererSymbolsStage_OpenGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "AtlasMapRenderer_OpenGL.h"
#include "AtlasMapRenderer_Metrics.h"
#include "MapSymbol.h"
#include "VectorMapSymbol.h"
#include "BillboardVectorMapSymbol.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "OnPathRasterMapSymbol.h"
#include "BillboardRasterMapSymbol.h"
#include "OnSurfaceRasterMapSymbol.h"
#include "MapSymbolsGroup.h"
#include "Stopwatch.h"

OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::AtlasMapRendererSymbolsStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer_)
    : AtlasMapRendererSymbolsStage(renderer_)
    , AtlasMapRendererStageHelper_OpenGL(this)
    , _onPathSymbol2dMaxGlyphsPerDrawCall(0)
    , _onPathSymbol3dMaxGlyphsPerDrawCall(0)
{
}

OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::~AtlasMapRendererSymbolsStage_OpenGL()
{
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initialize()
{
    bool ok = true;
    ok = ok && initializeBillboardRaster();
    ok = ok && initializeOnPath();
    ok = ok && initializeOnSurfaceRaster();
    ok = ok && initializeOnSurfaceVector();
    return ok;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* const metric_)
{
    const auto metric = dynamic_cast<AtlasMapRenderer_Metrics::Metric_renderFrame*>(metric_);

    bool ok = true;

    GL_PUSH_GROUP_MARKER(QLatin1String("symbols"));

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glDrawElements);

    const auto gpuAPI = getGPUAPI();
    //const auto& internalState = getInternalState();

    prepare(metric);

    // Initially, configure for straight alpha channel type
    auto currentAlphaChannelType = AlphaChannelType::Straight;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_RESULT;

    GLname lastUsedProgram;
    for (const auto& renderable_ : constOf(renderableSymbols))
    {
        if (const auto& renderable = std::dynamic_pointer_cast<const RenderableBillboardSymbol>(renderable_))
        {
            Stopwatch renderBillboardSymbolStopwatch(metric != nullptr);
            ok = ok && renderBillboardSymbol(
                renderable,
                currentAlphaChannelType,
                lastUsedProgram);
            if (metric)
            {
                metric->elapsedTimeForBillboardSymbolsRendering += renderBillboardSymbolStopwatch.elapsed();
                metric->billboardSymbolsRendered += 1;
            }
        }
        else if (const auto& renderable = std::dynamic_pointer_cast<const RenderableOnPathSymbol>(renderable_))
        {
            Stopwatch renderOnPathSymbolStopwatch(metric != nullptr);
            ok = ok && renderOnPathSymbol(
                renderable,
                currentAlphaChannelType,
                lastUsedProgram);
            if (metric)
            {
                metric->elapsedTimeForOnPathSymbolsRendering += renderOnPathSymbolStopwatch.elapsed();
                metric->onPathSymbolsRendered += 1;
            }
        }
        else if (const auto& renderable = std::dynamic_pointer_cast<const RenderableOnSurfaceSymbol>(renderable_))
        {
            Stopwatch renderOnSurfaceSymbolStopwatch(metric != nullptr);
            ok = ok && renderOnSurfaceSymbol(
                renderable,
                currentAlphaChannelType,
                lastUsedProgram);
            if (metric)
            {
                metric->elapsedTimeForOnSurfaceSymbolsRendering += renderOnSurfaceSymbolStopwatch.elapsed();
                metric->onSurfaceSymbolsRendered += 1;
            }
        }
    }

    // Unbind symbol texture from texture sampler
    glActiveTexture(GL_TEXTURE0 + 0);
    GL_CHECK_RESULT;
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_RESULT;

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    gpuAPI->unuseVAO();

    GL_POP_GROUP_MARKER;

    return ok;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderBillboardSymbol(
    const std::shared_ptr<const RenderableBillboardSymbol>& renderable,
    AlphaChannelType &currentAlphaChannelType,
    GLname& lastUsedProgram)
{
    if (std::dynamic_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol))
    {
        return renderBillboardRasterSymbol(
            renderable,
            currentAlphaChannelType,
            lastUsedProgram);
    }
    /*else if (std::dynamic_pointer_cast<const VectorMapSymbol>(renderable->mapSymbol))
    {
    renderBillboardVectorSymbol(
    renderable,
    viewport,
    intersections,
    lastUsedProgram,
    distanceFromCamera);
    }*/

    return false;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderOnPathSymbol(
    const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
    AlphaChannelType &currentAlphaChannelType,
    GLname& lastUsedProgram)
{
    // Draw the glyphs
    if (renderable->is2D)
    {
        return renderOnPath2dSymbol(
            renderable,
            currentAlphaChannelType,
            lastUsedProgram);
    }
    else
    {
        return renderOnPath3dSymbol(
            renderable,
            currentAlphaChannelType,
            lastUsedProgram);
    }
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderOnSurfaceSymbol(
    const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
    AlphaChannelType &currentAlphaChannelType,
    GLname& lastUsedProgram)
{
    if (std::dynamic_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol))
    {
        return renderOnSurfaceRasterSymbol(
            renderable,
            currentAlphaChannelType,
            lastUsedProgram);
    }
    else if (std::dynamic_pointer_cast<const VectorMapSymbol>(renderable->mapSymbol))
    {
        return renderOnSurfaceVectorSymbol(
            renderable,
            currentAlphaChannelType,
            lastUsedProgram);
    }

    return false;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::release(const bool gpuContextLost)
{
    bool ok = true;
    ok = ok && releaseBillboardRaster(gpuContextLost);
    ok = ok && releaseOnPath(gpuContextLost);
    ok = ok && releaseOnSurfaceRaster(gpuContextLost);
    ok = ok && releaseOnSurfaceVector(gpuContextLost);
    return ok;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeBillboardRaster()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);
    GL_CHECK_PRESENT(glDeleteShader);
    GL_CHECK_PRESENT(glDeleteProgram);

    // Compile vertex shader
    const QString vertexShader = QLatin1String(
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "INPUT vec2 in_vs_vertexTexCoords;                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "PARAM_OUTPUT vec2 v2f_texCoords;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mPerspectiveProjectionView;                                                                  ""\n"
        "uniform mat4 param_vs_mOrthographicProjection;                                                                     ""\n"
        "uniform vec4 param_vs_viewport; // x, y, width, height                                                             ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-symbol data
        "uniform vec2 param_vs_symbolOffsetFromTarget;                                                                      ""\n"
        "uniform ivec2 param_vs_symbolSize;                                                                                 ""\n"
        "uniform float param_vs_distanceFromCamera;                                                                         ""\n"
        "uniform ivec2 param_vs_onScreenOffset;                                                                             ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        // Calculate location of symbol in world coordinate system.
        "    vec4 symbolLocation;                                                                                           ""\n"
        "    symbolLocation.xz = param_vs_symbolOffsetFromTarget.xy * %TileSize3D%.0;                                       ""\n"
        "    symbolLocation.y = 0.0; //TODO: A height from heightmap should be used here                                    ""\n"
        "    symbolLocation.w = 1.0;                                                                                        ""\n"
        "                                                                                                                   ""\n"
        // Project location of symbol from world coordinate system to screen
        "    vec4 projectedSymbolLocation = param_vs_mPerspectiveProjectionView * symbolLocation;                           ""\n"
        "                                                                                                                   ""\n"
        // "Normalize" location in screen coordinates to get [-1 .. 1] range
        "    vec3 symbolLocationOnScreen = projectedSymbolLocation.xyz / projectedSymbolLocation.w;                         ""\n"
        "                                                                                                                   ""\n"
        // Using viewport size, get real screen coordinates and correct depth to be [0 .. 1]
        "    symbolLocationOnScreen.xy = symbolLocationOnScreen.xy * 0.5 + 0.5;                                             ""\n"
        "    symbolLocationOnScreen.x = symbolLocationOnScreen.x * param_vs_viewport.z + param_vs_viewport.x;               ""\n"
        "    symbolLocationOnScreen.y = symbolLocationOnScreen.y * param_vs_viewport.w + param_vs_viewport.y;               ""\n"
        "    symbolLocationOnScreen.z = (1.0 + symbolLocationOnScreen.z) * 0.5;                                             ""\n"
        "                                                                                                                   ""\n"
        // Add on-screen offset
        "    symbolLocationOnScreen.xy += vec2(param_vs_onScreenOffset);                                                    ""\n"
        "                                                                                                                   ""\n"
        // symbolLocationOnScreen.xy now contains correct coordinates in viewport,
        // which can be used in orthographic projection (if it was configured to match viewport).
        //
        // To provide pixel-perfect rendering of billboard raster symbols:
        // symbolLocationOnScreen.(x|y) has to be rounded and +0.5 in case param_vs_symbolSize.(x|y) is even
        // symbolLocationOnScreen.(x|y) has to be rounded in case param_vs_symbolSize.(x|y) is odd
        "    symbolLocationOnScreen.x = floor(symbolLocationOnScreen.x) + mod(float(param_vs_symbolSize.x), 2.0) * 0.5;     ""\n"
        "    symbolLocationOnScreen.y = floor(symbolLocationOnScreen.y) + mod(float(param_vs_symbolSize.y), 2.0) * 0.5;     ""\n"
        "                                                                                                                   ""\n"
        // So it's possible to calculate current vertex location:
        // Initially, get location of current vertex in screen coordinates
        "    vec2 vertexOnScreen;                                                                                           ""\n"
        "    vertexOnScreen.x = in_vs_vertexPosition.x * float(param_vs_symbolSize.x);                                      ""\n"
        "    vertexOnScreen.y = in_vs_vertexPosition.y * float(param_vs_symbolSize.y);                                      ""\n"
        "    vertexOnScreen = vertexOnScreen + symbolLocationOnScreen.xy;                                                   ""\n"
        "                                                                                                                   ""\n"
        // To provide pixel-perfect result, vertexOnScreen needs to be rounded
        "    vertexOnScreen = floor(vertexOnScreen + vec2(0.5, 0.5));                                                       ""\n"
        "                                                                                                                   ""\n"
        // There's no need to perform unprojection into orthographic world space, just multiply these coordinates by
        // orthographic projection matrix (View and Model being identity)
        "  vec4 vertex;                                                                                                     ""\n"
        "  vertex.xy = vertexOnScreen.xy;                                                                                   ""\n"
        "  vertex.z = -param_vs_distanceFromCamera;                                                                         ""\n"
        "  vertex.w = 1.0;                                                                                                  ""\n"
        "  gl_Position = param_vs_mOrthographicProjection * vertex;                                                         ""\n"
        "                                                                                                                   ""\n"
        // Texture coordinates are simply forwarded from input
        "   v2f_texCoords = in_vs_vertexTexCoords;                                                                          ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    preprocessedVertexShader.replace("%TileSize3D%", QString::number(AtlasMapRenderer::TileSize3D));
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    if (vsId == 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererSymbolsStage_OpenGL vertex shader");
        return false;
    }

    // Compile fragment shader
    const QString fragmentShader = QLatin1String(
        // Input data
        "PARAM_INPUT vec2 v2f_texCoords;                                                                                    ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        // Parameters: per-symbol data
        "uniform lowp sampler2D param_fs_sampler;                                                                           ""\n"
        "uniform lowp vec4 param_fs_modulationColor;                                                                        ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    FRAGMENT_COLOR_OUTPUT = SAMPLE_TEXTURE_2D(                                                                     ""\n"
        "        param_fs_sampler,                                                                                          ""\n"
        "        v2f_texCoords) * param_fs_modulationColor;                                                                 ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
    gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    if (fsId == 0)
    {
        glDeleteShader(vsId);
        GL_CHECK_RESULT;

        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererSymbolsStage_OpenGL fragment shader");
        return false;
    }

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    _billboardRasterProgram.id = gpuAPI->linkProgram(2, shaders, true, &variablesMap);
    if (!_billboardRasterProgram.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererSymbolsStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_billboardRasterProgram.id, variablesMap);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.in.vertexTexCoords, "in_vs_vertexTexCoords", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.mOrthographicProjection, "param_vs_mOrthographicProjection", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.viewport, "param_vs_viewport", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.symbolOffsetFromTarget, "param_vs_symbolOffsetFromTarget", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.symbolSize, "param_vs_symbolSize", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.distanceFromCamera, "param_vs_distanceFromCamera", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.onScreenOffset, "param_vs_onScreenOffset", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.fs.param.sampler, "param_fs_sampler", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.fs.param.modulationColor, "param_fs_modulationColor", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_billboardRasterProgram.id);
        GL_CHECK_RESULT;
        _billboardRasterProgram.id.reset();

        return false;
    }

#pragma pack(push, 1)
    struct Vertex
    {
        // XY coordinates. Z is assumed to be 0
        float positionXY[2];

        // UV coordinates
        float textureUV[2];
    };
#pragma pack(pop)

    // Vertex data
    Vertex vertices[4] =
    {
        // In OpenGL, UV origin is BL. But since same rule applies to uploading texture data,
        // texture in memory is vertically flipped, so swap bottom and top UVs
        { { -0.5f, -0.5f }, { 0.0f, 1.0f } },//BL
        { { -0.5f,  0.5f }, { 0.0f, 0.0f } },//TL
        { {  0.5f,  0.5f }, { 1.0f, 0.0f } },//TR
        { {  0.5f, -0.5f }, { 1.0f, 1.0f } } //BR
    };
    const auto verticesCount = 4;

    // Index data
    GLushort indices[6] =
    {
        0, 1, 2,
        0, 2, 3
    };
    const auto indicesCount = 6;

    _billboardRasterSymbolVAO = gpuAPI->allocateUninitializedVAO();

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_billboardRasterSymbolVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _billboardRasterSymbolVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_billboardRasterProgram.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_billboardRasterProgram.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, positionXY)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_billboardRasterProgram.vs.in.vertexTexCoords);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_billboardRasterProgram.vs.in.vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, textureUV)));
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_billboardRasterSymbolIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _billboardRasterSymbolIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->initializeVAO(_billboardRasterSymbolVAO);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderBillboardRasterSymbol(
    const std::shared_ptr<const RenderableBillboardSymbol>& renderable,
    AlphaChannelType &currentAlphaChannelType,
    GLname& lastUsedProgram)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const BillboardRasterMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
    //const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (lastUsedProgram != _billboardRasterProgram.id)
    {
        GL_PUSH_GROUP_MARKER("use 'billboard-raster' program");

        // Set symbol VAO
        gpuAPI->useVAO(_billboardRasterSymbolVAO);

        // Activate program
        glUseProgram(_billboardRasterProgram.id);
        GL_CHECK_RESULT;

        // Set perspective projection-view matrix
        glUniformMatrix4fv(_billboardRasterProgram.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
        GL_CHECK_RESULT;

        // Set orthographic projection matrix
        glUniformMatrix4fv(_billboardRasterProgram.vs.param.mOrthographicProjection, 1, GL_FALSE, glm::value_ptr(internalState.mOrthographicProjection));
        GL_CHECK_RESULT;

        // Set viewport
        glUniform4fv(_billboardRasterProgram.vs.param.viewport, 1, glm::value_ptr(internalState.glmViewport));
        GL_CHECK_RESULT;

        // Activate texture block for symbol textures
        glActiveTexture(GL_TEXTURE0 + 0);
        GL_CHECK_RESULT;

        // Set proper sampler for texture block
        gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + 0, GPUAPI_OpenGL::SamplerType::Symbol);

        // Bind texture to sampler
        glUniform1i(_billboardRasterProgram.fs.param.sampler, 0);
        GL_CHECK_RESULT;

        lastUsedProgram = _billboardRasterProgram.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) billboard raster \"%3\"]")
        .arg(QString::asprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->toString())
        .arg(qPrintable(symbol->content)));

    //////////////////////////////////////////////////////////////////////////
    //if (symbol->content == "seamark_buoy_yellow_can")
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    // Set symbol offset from target
    glUniform2f(_billboardRasterProgram.vs.param.symbolOffsetFromTarget,
        renderable->offsetFromTarget.x,
        renderable->offsetFromTarget.y);
    GL_CHECK_RESULT;

    // Set symbol size
    glUniform2i(_billboardRasterProgram.vs.param.symbolSize, gpuResource->width, gpuResource->height);
    GL_CHECK_RESULT;

    // Set distance from camera to symbol
    glUniform1f(_billboardRasterProgram.vs.param.distanceFromCamera, renderable->distanceToCamera);
    GL_CHECK_RESULT;

    // Set on-screen offset
    const auto& offsetOnScreen =
        (renderable->instanceParameters && renderable->instanceParameters->overridesOffset)
        ? renderable->instanceParameters->offset
        : symbol->offset;
    glUniform2i(_billboardRasterProgram.vs.param.onScreenOffset, offsetOnScreen.x, -offsetOnScreen.y);
    GL_CHECK_RESULT;

    if (currentAlphaChannelType != gpuResource->alphaChannelType)
    {
        switch (gpuResource->alphaChannelType)
        {
            case AlphaChannelType::Premultiplied:
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                GL_CHECK_RESULT;
                break;
            case AlphaChannelType::Straight:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                GL_CHECK_RESULT;
                break;
            default:
                break;
        }

        currentAlphaChannelType = gpuResource->alphaChannelType;
    }

    // Activate symbol texture
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
    GL_CHECK_RESULT;

    // Set modulation color
    glUniform4f(_billboardRasterProgram.fs.param.modulationColor,
        symbol->modulationColor.r,
        symbol->modulationColor.g,
        symbol->modulationColor.b,
        symbol->modulationColor.a);
    GL_CHECK_RESULT;

    // Apply settings from texture block to texture
    gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + 0);

    // Draw symbol actually
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    GL_CHECK_RESULT;

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseBillboardRaster(const bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_billboardRasterSymbolVAO.isValid())
    {
        gpuAPI->releaseVAO(_billboardRasterSymbolVAO, gpuContextLost);
        _billboardRasterSymbolVAO.reset();
    }

    if (_billboardRasterSymbolIBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_billboardRasterSymbolIBO);
            GL_CHECK_RESULT;
        }
        _billboardRasterSymbolIBO.reset();
    }
    if (_billboardRasterSymbolVBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_billboardRasterSymbolVBO);
            GL_CHECK_RESULT;
        }
        _billboardRasterSymbolVBO.reset();
    }
    
    if (_billboardRasterProgram.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_billboardRasterProgram.id);
            GL_CHECK_RESULT;
        }
        _billboardRasterProgram = BillboardRasterSymbolProgram();
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnPath()
{
    bool ok = true;
    ok = ok && initializeOnPath2D();
    ok = ok && initializeOnPath3D();
    return ok;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnPath2D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    const auto alreadyOccupiedUniforms =
        4 /*param_vs_mPerspectiveProjectionView*/ +
        1 /*param_vs_glyphHeight*/ +
        1 /*param_vs_zDistanceFromCamera*/;
    _onPathSymbol2dMaxGlyphsPerDrawCall = (gpuAPI->maxVertexUniformVectors - alreadyOccupiedUniforms) / 5;
    if (initializeOnPath2DProgram(_onPathSymbol2dMaxGlyphsPerDrawCall))
    {
        LogPrintf(LogSeverityLevel::Info,
            "This device is capable of rendering %d glyphs of a on-path-2D symbol at a time",
            _onPathSymbol2dMaxGlyphsPerDrawCall);
    }
    else
    {
        bool initializedProgram = false;
        if (_onPathSymbol2dMaxGlyphsPerDrawCall > 1)
        {
            for (auto testMaxGlyphsPerDrawCall = _onPathSymbol2dMaxGlyphsPerDrawCall - 1; testMaxGlyphsPerDrawCall >= 1; testMaxGlyphsPerDrawCall--)
            {
                if (!initializeOnPath2DProgram(testMaxGlyphsPerDrawCall))
                    continue;

                LogPrintf(LogSeverityLevel::Warning,
                    "Seems like buggy driver. This device should be capable of rendering %d glyphs of a on-path-2D symbol at a time, but only %d glyphs variant compiles",
                    _onPathSymbol2dMaxGlyphsPerDrawCall,
                    testMaxGlyphsPerDrawCall);
                _onPathSymbol2dMaxGlyphsPerDrawCall = testMaxGlyphsPerDrawCall;
                initializedProgram = true;
                break;
            }
        }
        
        if (!initializedProgram)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Seems like buggy driver. This device should be capable of rendering %d glyphs of a on-path-2D symbol at a time, but it fails to compile program even for 1",
                _onPathSymbol2dMaxGlyphsPerDrawCall);
            return false;
        }
    }

#pragma pack(push, 1)
    struct Vertex
    {
        // XY coordinates
        float positionXY[2];

        // Index of glyph
        //NOTE: Here should be int to omit conversion float->int, but it's not supported in OpenGLES 2.0
        float glyphIndex;

        // UV coordinates
        float textureUV[2];
    };
#pragma pack(pop)

    // Vertex data
    const Vertex templateVertices[4] =
    {
        // In OpenGL, UV origin is BL. But since same rule applies to uploading texture data,
        // texture in memory is vertically flipped, so swap bottom and top UVs
        { { -0.5f, -0.5f }, 0, { 0.0f, 1.0f } },//BL
        { { -0.5f,  0.5f }, 0, { 0.0f, 0.0f } },//TL
        { {  0.5f,  0.5f }, 0, { 1.0f, 0.0f } },//TR
        { {  0.5f, -0.5f }, 0, { 1.0f, 1.0f } } //BR
    };
    QVector<Vertex> vertices(4 * _onPathSymbol2dMaxGlyphsPerDrawCall);
    auto pVertex = vertices.data();
    for (int glyphIdx = 0; glyphIdx < _onPathSymbol2dMaxGlyphsPerDrawCall; glyphIdx++)
    {
        auto& p0 = *(pVertex++);
        p0 = templateVertices[0];
        p0.glyphIndex = glyphIdx;

        auto& p1 = *(pVertex++);
        p1 = templateVertices[1];
        p1.glyphIndex = glyphIdx;

        auto& p2 = *(pVertex++);
        p2 = templateVertices[2];
        p2.glyphIndex = glyphIdx;

        auto& p3 = *(pVertex++);
        p3 = templateVertices[3];
        p3.glyphIndex = glyphIdx;
    }

    // Index data
    QVector<GLushort> indices(6 * _onPathSymbol2dMaxGlyphsPerDrawCall);
    auto pIndex = indices.data();
    for (int glyphIdx = 0; glyphIdx < _onPathSymbol2dMaxGlyphsPerDrawCall; glyphIdx++)
    {
        *(pIndex++) = glyphIdx * 4 + 0;
        *(pIndex++) = glyphIdx * 4 + 1;
        *(pIndex++) = glyphIdx * 4 + 2;

        *(pIndex++) = glyphIdx * 4 + 0;
        *(pIndex++) = glyphIdx * 4 + 2;
        *(pIndex++) = glyphIdx * 4 + 3;
    }

    _onPathSymbol2dVAO = gpuAPI->allocateUninitializedVAO();

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_onPathSymbol2dVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _onPathSymbol2dVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.constData(), GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_onPath2dProgram.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_onPath2dProgram.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, positionXY)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_onPath2dProgram.vs.in.glyphIndex);
    GL_CHECK_RESULT;
    //NOTE: Here should be glVertexAttribIPointer to omit conversion float->int, but it's not supported in OpenGLES 2.0
    glVertexAttribPointer(*_onPath2dProgram.vs.in.glyphIndex, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, glyphIndex)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_onPath2dProgram.vs.in.vertexTexCoords);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_onPath2dProgram.vs.in.vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, textureUV)));
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_onPathSymbol2dIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _onPathSymbol2dIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(GLushort), indices.constData(), GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->initializeVAO(_onPathSymbol2dVAO);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnPath2DProgram(const unsigned int maxGlyphsPerDrawCall)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteShader);
    GL_CHECK_PRESENT(glDeleteProgram);

    // Compile vertex shader
    const QString vertexShader = QLatin1String(
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "INPUT float in_vs_glyphIndex;                                                                                      ""\n"
        "INPUT vec2 in_vs_vertexTexCoords;                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "PARAM_OUTPUT vec2 v2f_texCoords;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mOrthographicProjection;                                                                     ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-symbol data
        "uniform float param_vs_glyphHeight;                                                                                ""\n"
        "uniform float param_vs_distanceFromCamera;                                                                         ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-glyph data
        "struct Glyph                                                                                                       ""\n"
        "{                                                                                                                  ""\n"
        "    vec2 anchorPoint;                                                                                              ""\n"
        "    float width;                                                                                                   ""\n"
        "    float angle;                                                                                                   ""\n"
        "    float widthOfPreviousN;                                                                                        ""\n"
        "    float widthN;                                                                                                  ""\n"
        "};                                                                                                                 ""\n"
        "uniform Glyph param_vs_glyphs[%MaxGlyphsPerDrawCall%];                                                             ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    Glyph glyph = param_vs_glyphs[int(in_vs_glyphIndex)];                                                          ""\n"
        "    vec2 anchorPoint = glyph.anchorPoint;                                                                          ""\n"
        "    float cos_a = cos(glyph.angle);                                                                                ""\n"
        "    float sin_a = sin(glyph.angle);                                                                                ""\n"
        "                                                                                                                   ""\n"
        // Pixel-perfect rendering is available when angle is 0, 90, 180 or 270 degrees, what will produce
        // cos_a 0, 1 or -1
        //"    if (abs(cos_a - int(cos_a)) < 0.0001)                                                                          ""\n"
        //"    {                                                                                                              ""\n"
        //"        anchorPoint.x = floor(anchorPoint.x) + mod(glyph.width, 2.0) * 0.5;                                        ""\n"
        //"        anchorPoint.y = floor(anchorPoint.y) + mod(param_vs_glyphHeight, 2.0) * 0.5;                               ""\n"
        //"    }                                                                                                              ""\n"
        "                                                                                                                   ""\n"
        // Get on-screen glyph point offset
        "    vec2 glyphPoint;                                                                                               ""\n"
        "    glyphPoint.x = in_vs_vertexPosition.x * glyph.width;                                                           ""\n"
        "    glyphPoint.y = in_vs_vertexPosition.y * param_vs_glyphHeight;                                                  ""\n"
        "                                                                                                                   ""\n"
        // Get on-screen vertex coordinates
        "    vec4 vertexOnScreen;                                                                                           ""\n"
        "    vertexOnScreen.x = anchorPoint.x + (glyphPoint.x*cos_a - glyphPoint.y*sin_a);                                  ""\n"
        "    vertexOnScreen.y = anchorPoint.y + (glyphPoint.x*sin_a + glyphPoint.y*cos_a);                                  ""\n"
        "    vertexOnScreen.z = -param_vs_distanceFromCamera;                                                               ""\n"
        "    vertexOnScreen.w = 1.0;                                                                                        ""\n"
        "                                                                                                                   ""\n"
        // Pixel-perfect rendering is available when angle is 0, 90, 180 or 270 degrees, what will produce
        // cos_a 0, 1 or -1
        //"    if (abs(cos_a - int(cos_a)) < 0.0001)                                                                          ""\n"
        //"    {                                                                                                              ""\n"
        //"        vertexOnScreen.x = floor(vertexOnScreen.x + 0.5);                                                          ""\n"
        //"        vertexOnScreen.y = floor(vertexOnScreen.y + 0.5);                                                          ""\n"
        //"    }                                                                                                              ""\n"
        "                                                                                                                   ""\n"
        // Project vertex
        "    gl_Position = param_vs_mOrthographicProjection * vertexOnScreen;                                               ""\n"
        "                                                                                                                   ""\n"
        // Prepare texture coordinates
        "    v2f_texCoords.s = glyph.widthOfPreviousN + in_vs_vertexTexCoords.s*glyph.widthN;                               ""\n"
        "    v2f_texCoords.t = in_vs_vertexTexCoords.t; // Height is compatible as-is                                       ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    preprocessedVertexShader.replace("%MaxGlyphsPerDrawCall%", QString::number(maxGlyphsPerDrawCall));
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    if (vsId == 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererSymbolsStage_OpenGL vertex shader");
        return false;
    }

    // Compile fragment shader
    const QString fragmentShader = QLatin1String(
        // Input data
        "PARAM_INPUT vec2 v2f_texCoords;                                                                                    ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        // Parameters: per-symbol data
        "uniform lowp sampler2D param_fs_sampler;                                                                           ""\n"
        "uniform lowp vec4 param_fs_modulationColor;                                                                        ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    FRAGMENT_COLOR_OUTPUT = SAMPLE_TEXTURE_2D(                                                                     ""\n"
        "        param_fs_sampler,                                                                                          ""\n"
        "        v2f_texCoords) * param_fs_modulationColor;                                                                 ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
    gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    if (fsId == 0)
    {
        glDeleteShader(vsId);
        GL_CHECK_RESULT;

        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererSymbolsStage_OpenGL fragment shader");
        return false;
    }

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    _onPath2dProgram.id = gpuAPI->linkProgram(2, shaders, true, &variablesMap);
    if (!_onPath2dProgram.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererSymbolsStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_onPath2dProgram.id, variablesMap);
    ok = ok && lookup->lookupLocation(_onPath2dProgram.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_onPath2dProgram.vs.in.glyphIndex, "in_vs_glyphIndex", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_onPath2dProgram.vs.in.vertexTexCoords, "in_vs_vertexTexCoords", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_onPath2dProgram.vs.param.mOrthographicProjection, "param_vs_mOrthographicProjection", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onPath2dProgram.vs.param.glyphHeight, "param_vs_glyphHeight", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onPath2dProgram.vs.param.distanceFromCamera, "param_vs_distanceFromCamera", GlslVariableType::Uniform);
    auto& glyphs = _onPath2dProgram.vs.param.glyphs;
    glyphs.resize(maxGlyphsPerDrawCall);
    int glyphStructIndex = 0;
    for (auto& glyph : glyphs)
    {
        const auto glyphStructPrefix =
            QString::fromLatin1("param_vs_glyphs[%glyphIndex%]").replace(QLatin1String("%glyphIndex%"), QString::number(glyphStructIndex++));

        ok = ok && lookup->lookupLocation(glyph.anchorPoint, glyphStructPrefix + ".anchorPoint", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(glyph.width, glyphStructPrefix + ".width", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(glyph.angle, glyphStructPrefix + ".angle", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(glyph.widthOfPreviousN, glyphStructPrefix + ".widthOfPreviousN", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(glyph.widthN, glyphStructPrefix + ".widthN", GlslVariableType::Uniform);
    }
    ok = ok && lookup->lookupLocation(_onPath2dProgram.fs.param.sampler, "param_fs_sampler", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onPath2dProgram.fs.param.modulationColor, "param_fs_modulationColor", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_onPath2dProgram.id);
        GL_CHECK_RESULT;
        _onPath2dProgram.id.reset();

        return false;
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnPath3D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    const auto alreadyOccupiedUniforms =
        4 /*param_vs_mPerspectiveProjectionView*/ +
        1 /*param_vs_glyphHeight*/ +
        1 /*param_vs_zDistanceFromCamera*/;
    _onPathSymbol3dMaxGlyphsPerDrawCall = (gpuAPI->maxVertexUniformVectors - alreadyOccupiedUniforms) / 5;
    if (initializeOnPath3DProgram(_onPathSymbol3dMaxGlyphsPerDrawCall))
    {
        LogPrintf(LogSeverityLevel::Info,
            "This device is capable of rendering %d glyphs of a on-path-3D symbol at a time",
            _onPathSymbol3dMaxGlyphsPerDrawCall);
    }
    else
    {
        bool initializedProgram = false;
        if (_onPathSymbol3dMaxGlyphsPerDrawCall > 1)
        {
            for (auto testMaxGlyphsPerDrawCall = _onPathSymbol3dMaxGlyphsPerDrawCall - 1; testMaxGlyphsPerDrawCall >= 1; testMaxGlyphsPerDrawCall--)
            {
                if (!initializeOnPath3DProgram(testMaxGlyphsPerDrawCall))
                    continue;

                LogPrintf(LogSeverityLevel::Warning,
                    "Seems like buggy driver. This device should be capable of rendering %d glyphs of a on-path-3D symbol at a time, but only %d glyphs variant compiles",
                    _onPathSymbol3dMaxGlyphsPerDrawCall,
                    testMaxGlyphsPerDrawCall);
                _onPathSymbol3dMaxGlyphsPerDrawCall = testMaxGlyphsPerDrawCall;
                initializedProgram = true;
                break;
            }
        }

        if (!initializedProgram)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Seems like buggy driver. This device should be capable of rendering %d glyphs of a on-path-3D symbol at a time, but it fails to compile program even for 1",
                _onPathSymbol3dMaxGlyphsPerDrawCall);
            return false;
        }
    }

#pragma pack(push, 1)
    struct Vertex
    {
        // XY coordinates
        float positionXY[2];

        // Index of glyph
        //NOTE: Here should be int to omit conversion float->int, but it's not supported in OpenGLES 2.0
        float glyphIndex;

        // UV coordinates
        float textureUV[2];
    };
#pragma pack(pop)

    // Vertex data
    const Vertex templateVertices[4] =
    {
        // In OpenGL, UV origin is BL. But since same rule applies to uploading texture data,
        // texture in memory is vertically flipped, so swap bottom and top UVs
        { { -0.5f, -0.5f }, 0, { 1.0f - 0.0f, 1.0f } },//BL
        { { -0.5f,  0.5f }, 0, { 1.0f - 0.0f, 0.0f } },//TL
        { {  0.5f,  0.5f }, 0, { 1.0f - 1.0f, 0.0f } },//TR
        { {  0.5f, -0.5f }, 0, { 1.0f - 1.0f, 1.0f } } //BR
    };
    QVector<Vertex> vertices(4 * _onPathSymbol3dMaxGlyphsPerDrawCall);
    auto pVertex = vertices.data();
    for (int glyphIdx = 0; glyphIdx < _onPathSymbol3dMaxGlyphsPerDrawCall; glyphIdx++)
    {
        auto& p0 = *(pVertex++);
        p0 = templateVertices[0];
        p0.glyphIndex = glyphIdx;

        auto& p1 = *(pVertex++);
        p1 = templateVertices[1];
        p1.glyphIndex = glyphIdx;

        auto& p2 = *(pVertex++);
        p2 = templateVertices[2];
        p2.glyphIndex = glyphIdx;

        auto& p3 = *(pVertex++);
        p3 = templateVertices[3];
        p3.glyphIndex = glyphIdx;
    }

    // Index data
    QVector<GLushort> indices(6 * _onPathSymbol3dMaxGlyphsPerDrawCall);
    auto pIndex = indices.data();
    for (int glyphIdx = 0; glyphIdx < _onPathSymbol3dMaxGlyphsPerDrawCall; glyphIdx++)
    {
        *(pIndex++) = glyphIdx * 4 + 0;
        *(pIndex++) = glyphIdx * 4 + 1;
        *(pIndex++) = glyphIdx * 4 + 2;

        *(pIndex++) = glyphIdx * 4 + 0;
        *(pIndex++) = glyphIdx * 4 + 2;
        *(pIndex++) = glyphIdx * 4 + 3;
    }

    _onPathSymbol3dVAO = gpuAPI->allocateUninitializedVAO();

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_onPathSymbol3dVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _onPathSymbol3dVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.constData(), GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_onPath3dProgram.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_onPath3dProgram.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, positionXY)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_onPath3dProgram.vs.in.glyphIndex);
    GL_CHECK_RESULT;
    //NOTE: Here should be glVertexAttribIPointer to omit conversion float->int, but it's not supported in OpenGLES 2.0
    glVertexAttribPointer(*_onPath3dProgram.vs.in.glyphIndex, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, glyphIndex)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_onPath3dProgram.vs.in.vertexTexCoords);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_onPath3dProgram.vs.in.vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, textureUV)));
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_onPathSymbol3dIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _onPathSymbol3dIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(GLushort), indices.constData(), GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->initializeVAO(_onPathSymbol3dVAO);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnPath3DProgram(const unsigned int maxGlyphsPerDrawCall)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteShader);
    GL_CHECK_PRESENT(glDeleteProgram);

    // Compile vertex shader
    const QString vertexShader = QLatin1String(
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "INPUT float in_vs_glyphIndex;                                                                                      ""\n"
        "INPUT vec2 in_vs_vertexTexCoords;                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "PARAM_OUTPUT vec2 v2f_texCoords;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mPerspectiveProjectionView;                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-symbol data
        "uniform float param_vs_glyphHeight;                                                                                ""\n"
        "uniform float param_vs_zDistanceFromCamera;                                                                        ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-glyph data
        "struct Glyph                                                                                                       ""\n"
        "{                                                                                                                  ""\n"
        "    vec2 anchorPoint;                                                                                              ""\n"
        "    float width;                                                                                                   ""\n"
        "    float angle;                                                                                                   ""\n"
        "    float widthOfPreviousN;                                                                                        ""\n"
        "    float widthN;                                                                                                  ""\n"
        "};                                                                                                                 ""\n"
        "uniform Glyph param_vs_glyphs[%MaxGlyphsPerDrawCall%];                                                             ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    Glyph glyph = param_vs_glyphs[int(in_vs_glyphIndex)];                                                          ""\n"
        "                                                                                                                   ""\n"
        // Get on-screen vertex coordinates
        "    float cos_a = cos(glyph.angle);                                                                                ""\n"
        "    float sin_a = sin(glyph.angle);                                                                                ""\n"
        "    vec2 p;                                                                                                        ""\n"
        "    p.x = in_vs_vertexPosition.x * glyph.width;                                                                    ""\n"
        "    p.y = in_vs_vertexPosition.y * param_vs_glyphHeight;                                                           ""\n"
        "    vec4 v;                                                                                                        ""\n"
        "    v.x = glyph.anchorPoint.x + (p.x*cos_a - p.y*sin_a);                                                           ""\n"
        "    v.y = 0.0;                                                                                                     ""\n"
        "    v.z = glyph.anchorPoint.y + (p.x*sin_a + p.y*cos_a);                                                           ""\n"
        "    v.w = 1.0;                                                                                                     ""\n"
        "    gl_Position = param_vs_mPerspectiveProjectionView * v;                                                         ""\n"
        "    gl_Position.z = param_vs_zDistanceFromCamera;                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Prepare texture coordinates
        "    v2f_texCoords.s = glyph.widthOfPreviousN + in_vs_vertexTexCoords.s*glyph.widthN;                               ""\n"
        "    v2f_texCoords.t = in_vs_vertexTexCoords.t; // Height is compatible as-is                                       ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    preprocessedVertexShader.replace("%MaxGlyphsPerDrawCall%", QString::number(maxGlyphsPerDrawCall));
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    if (vsId == 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererSymbolsStage_OpenGL vertex shader");
        return false;
    }

    // Compile fragment shader
    const QString fragmentShader = QLatin1String(
        // Input data
        "PARAM_INPUT vec2 v2f_texCoords;                                                                                    ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        // Parameters: per-symbol data
        "uniform lowp sampler2D param_fs_sampler;                                                                           ""\n"
        "uniform lowp vec4 param_fs_modulationColor;                                                                        ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    FRAGMENT_COLOR_OUTPUT = SAMPLE_TEXTURE_2D(                                                                     ""\n"
        "        param_fs_sampler,                                                                                          ""\n"
        "        v2f_texCoords) * param_fs_modulationColor;                                                                 ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
    gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    if (fsId == 0)
    {
        glDeleteShader(vsId);
        GL_CHECK_RESULT;

        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererSymbolsStage_OpenGL fragment shader");
        return false;
    }

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    _onPath3dProgram.id = gpuAPI->linkProgram(2, shaders, true, &variablesMap);
    if (!_onPath3dProgram.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererSymbolsStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_onPath3dProgram.id, variablesMap);
    ok = ok && lookup->lookupLocation(_onPath3dProgram.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_onPath3dProgram.vs.in.glyphIndex, "in_vs_glyphIndex", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_onPath3dProgram.vs.in.vertexTexCoords, "in_vs_vertexTexCoords", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_onPath3dProgram.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onPath3dProgram.vs.param.glyphHeight, "param_vs_glyphHeight", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onPath3dProgram.vs.param.zDistanceFromCamera, "param_vs_zDistanceFromCamera", GlslVariableType::Uniform);
    auto& glyphs = _onPath3dProgram.vs.param.glyphs;
    glyphs.resize(maxGlyphsPerDrawCall);
    int glyphStructIndex = 0;
    for (auto& glyph : glyphs)
    {
        const auto glyphStructPrefix =
            QString::fromLatin1("param_vs_glyphs[%glyphIndex%]").replace(QLatin1String("%glyphIndex%"), QString::number(glyphStructIndex++));

        ok = ok && lookup->lookupLocation(glyph.anchorPoint, glyphStructPrefix + ".anchorPoint", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(glyph.width, glyphStructPrefix + ".width", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(glyph.angle, glyphStructPrefix + ".angle", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(glyph.widthOfPreviousN, glyphStructPrefix + ".widthOfPreviousN", GlslVariableType::Uniform);
        ok = ok && lookup->lookupLocation(glyph.widthN, glyphStructPrefix + ".widthN", GlslVariableType::Uniform);
    }
    ok = ok && lookup->lookupLocation(_onPath3dProgram.fs.param.sampler, "param_fs_sampler", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onPath3dProgram.fs.param.modulationColor, "param_fs_modulationColor", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_onPath3dProgram.id);
        GL_CHECK_RESULT;
        _onPath3dProgram.id.reset();

        return false;
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderOnPath2dSymbol(
    const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
    AlphaChannelType &currentAlphaChannelType,
    GLname& lastUsedProgram)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnPathRasterMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
    //const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (lastUsedProgram != _onPath2dProgram.id)
    {
        GL_PUSH_GROUP_MARKER("use 'on-path-2d' program");

        // Set symbol VAO
        gpuAPI->useVAO(_onPathSymbol2dVAO);

        // Activate program
        glUseProgram(_onPath2dProgram.id);
        GL_CHECK_RESULT;

        // Set orthographic projection matrix
        glUniformMatrix4fv(_onPath2dProgram.vs.param.mOrthographicProjection, 1, GL_FALSE, glm::value_ptr(internalState.mOrthographicProjection));
        GL_CHECK_RESULT;

        // Activate texture block for symbol textures
        glActiveTexture(GL_TEXTURE0 + 0);
        GL_CHECK_RESULT;

        // Set proper sampler for texture block
        gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + 0, GPUAPI_OpenGL::SamplerType::Symbol);

        // Bind texture to sampler
        glUniform1i(_onPath2dProgram.fs.param.sampler, 0);
        GL_CHECK_RESULT;

        lastUsedProgram = _onPath2dProgram.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) SOP-2D \"%3\"]")
        .arg(QString::asprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->toString())
        .arg(qPrintable(symbol->content)));

    // Set glyph height
    glUniform1f(_onPath2dProgram.vs.param.glyphHeight, gpuResource->height);
    GL_CHECK_RESULT;

    // Set distance from camera to symbol
    glUniform1f(_onPath2dProgram.vs.param.distanceFromCamera, renderable->distanceToCamera);
    GL_CHECK_RESULT;

    if (currentAlphaChannelType != gpuResource->alphaChannelType)
    {
        switch (gpuResource->alphaChannelType)
        {
            case AlphaChannelType::Premultiplied:
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                GL_CHECK_RESULT;
                break;
            case AlphaChannelType::Straight:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                GL_CHECK_RESULT;
                break;
            default:
                break;
        }

        currentAlphaChannelType = gpuResource->alphaChannelType;
    }

    // Activate symbol texture
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
    GL_CHECK_RESULT;

    // Apply settings from texture block to texture
    gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + 0);

    // Set modulation color
    glUniform4f(_onPath2dProgram.fs.param.modulationColor,
        symbol->modulationColor.r,
        symbol->modulationColor.g,
        symbol->modulationColor.b,
        symbol->modulationColor.a);
    GL_CHECK_RESULT;

    // Draw chains of glyphs
    const auto glyphsCount = renderable->glyphsPlacement.size();
    unsigned int glyphsDrawn = 0;
    auto pGlyph = renderable->glyphsPlacement.constData();
    const auto pFirstGlyphVS = _onPath2dProgram.vs.param.glyphs.constData();
    float widthOfPreviousN = 0.0f;
    while (glyphsDrawn < glyphsCount)
    {
        const auto glyphsToDraw = qMin(glyphsCount - glyphsDrawn, _onPathSymbol2dMaxGlyphsPerDrawCall);
        auto pGlyphVS = pFirstGlyphVS;

        for (auto glyphIdx = 0; glyphIdx < glyphsToDraw; glyphIdx++)
        {
            const auto& glyph = *(pGlyph++);
            const auto& vsGlyph = *(pGlyphVS++);

            // Set anchor point of glyph
            glUniform2fv(vsGlyph.anchorPoint, 1, glm::value_ptr(glyph.anchorPoint));
            GL_CHECK_RESULT;

            // Set glyph width
            glUniform1f(vsGlyph.width, glyph.width);
            GL_CHECK_RESULT;

            // Set angle
            glUniform1f(vsGlyph.angle, glyph.angle);
            GL_CHECK_RESULT;

            // Set normalized width of all previous glyphs
            glUniform1f(vsGlyph.widthOfPreviousN, widthOfPreviousN);
            GL_CHECK_RESULT;

            // Set normalized width of this glyph
            const auto widthN = glyph.width*gpuResource->uTexelSizeN;
            glUniform1f(vsGlyph.widthN, widthN);
            GL_CHECK_RESULT;

            widthOfPreviousN += widthN;
        }

        // Draw chain of glyphs actually
        glDrawElements(GL_TRIANGLES, 6 * glyphsToDraw, GL_UNSIGNED_SHORT, nullptr);
        GL_CHECK_RESULT;

        glyphsDrawn += glyphsToDraw;
    }

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderOnPath3dSymbol(
    const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
    AlphaChannelType &currentAlphaChannelType,
    GLname& lastUsedProgram)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnPathRasterMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
    //const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (lastUsedProgram != _onPath3dProgram.id)
    {
        GL_PUSH_GROUP_MARKER("use 'on-path-3d' program");

        // Set symbol VAO
        gpuAPI->useVAO(_onPathSymbol3dVAO);

        // Activate program
        glUseProgram(_onPath3dProgram.id);
        GL_CHECK_RESULT;

        // Set view-projection matrix
        glUniformMatrix4fv(_onPath3dProgram.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
        GL_CHECK_RESULT;

        // Activate texture block for symbol textures
        glActiveTexture(GL_TEXTURE0 + 0);
        GL_CHECK_RESULT;

        // Set proper sampler for texture block
        gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + 0, GPUAPI_OpenGL::SamplerType::Symbol);

        // Bind texture to sampler
        glUniform1i(_onPath3dProgram.fs.param.sampler, 0);
        GL_CHECK_RESULT;

        lastUsedProgram = _onPath3dProgram.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) SOP-3D \"%3\"]")
        .arg(QString::asprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->toString())
        .arg(qPrintable(symbol->content)));

    // Set glyph height
    glUniform1f(_onPath3dProgram.vs.param.glyphHeight, gpuResource->height*internalState.pixelInWorldProjectionScale);
    GL_CHECK_RESULT;

    // Set distance from camera
    const auto zDistanceFromCamera = (internalState.mOrthographicProjection * glm::vec4(0.0f, 0.0f, -renderable->distanceToCamera, 1.0f)).z;
    glUniform1f(_onPath3dProgram.vs.param.zDistanceFromCamera, zDistanceFromCamera);
    GL_CHECK_RESULT;

    if (currentAlphaChannelType != gpuResource->alphaChannelType)
    {
        switch (gpuResource->alphaChannelType)
        {
            case AlphaChannelType::Premultiplied:
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                GL_CHECK_RESULT;
                break;
            case AlphaChannelType::Straight:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                GL_CHECK_RESULT;
                break;
            default:
                break;
        }

        currentAlphaChannelType = gpuResource->alphaChannelType;
    }

    // Activate symbol texture
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
    GL_CHECK_RESULT;

    // Set modulation color
    glUniform4f(_onPath3dProgram.fs.param.modulationColor,
        symbol->modulationColor.r,
        symbol->modulationColor.g,
        symbol->modulationColor.b,
        symbol->modulationColor.a);
    GL_CHECK_RESULT;

    // Apply settings from texture block to texture
    gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + 0);

    // Draw chains of glyphs
    const auto glyphsCount = renderable->glyphsPlacement.size();
    unsigned int glyphsDrawn = 0;
    auto pGlyph = renderable->glyphsPlacement.constData();
    const auto pFirstGlyphVS = _onPath3dProgram.vs.param.glyphs.constData();
    float widthOfPreviousN = 0.0f;
    while (glyphsDrawn < glyphsCount)
    {
        const auto glyphsToDraw = qMin(glyphsCount - glyphsDrawn, _onPathSymbol3dMaxGlyphsPerDrawCall);
        auto pGlyphVS = pFirstGlyphVS;

        for (auto glyphIdx = 0; glyphIdx < glyphsToDraw; glyphIdx++)
        {
            const auto& glyph = *(pGlyph++);
            const auto& vsGlyph = *(pGlyphVS++);

            // Set anchor point of glyph
            glUniform2fv(vsGlyph.anchorPoint, 1, glm::value_ptr(glyph.anchorPoint));
            GL_CHECK_RESULT;

            // Set glyph width
            glUniform1f(vsGlyph.width, glyph.width*internalState.pixelInWorldProjectionScale);
            GL_CHECK_RESULT;

            // Set angle
            glUniform1f(vsGlyph.angle, Utilities::normalizedAngleRadians(glyph.angle + M_PI));
            GL_CHECK_RESULT;

            // Set normalized width of all previous glyphs
            glUniform1f(vsGlyph.widthOfPreviousN, widthOfPreviousN);
            GL_CHECK_RESULT;

            // Set normalized width of this glyph
            const auto widthN = glyph.width*gpuResource->uTexelSizeN;
            glUniform1f(vsGlyph.widthN, widthN);
            GL_CHECK_RESULT;

            widthOfPreviousN += widthN;
        }

        // Draw chain of glyphs actually
        glDrawElements(GL_TRIANGLES, 6 * glyphsToDraw, GL_UNSIGNED_SHORT, nullptr);
        GL_CHECK_RESULT;

        glyphsDrawn += glyphsToDraw;
    }

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath(const bool gpuContextLost)
{
    bool ok = true;
    ok = ok && releaseOnPath3D(gpuContextLost);
    ok = ok && releaseOnPath2D(gpuContextLost);
    return ok;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath2D(const bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_onPathSymbol2dVAO.isValid())
    {
        gpuAPI->releaseVAO(_onPathSymbol2dVAO, gpuContextLost);
        _onPathSymbol2dVAO.reset();
    }

    if (_onPathSymbol2dIBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_onPathSymbol2dIBO);
            GL_CHECK_RESULT;
        }
        _onPathSymbol2dIBO.reset();
    }
    if (_onPathSymbol2dVBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_onPathSymbol2dVBO);
            GL_CHECK_RESULT;
        }
        _onPathSymbol2dVBO.reset();
    }
    
    if (_onPath2dProgram.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_onPath2dProgram.id);
            GL_CHECK_RESULT;
        }
        _onPath2dProgram = OnPathSymbol2dProgram();
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath3D(const bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_onPathSymbol3dVAO.isValid())
    {
        gpuAPI->releaseVAO(_onPathSymbol3dVAO, gpuContextLost);
        _onPathSymbol3dVAO.reset();
    }

    if (_onPathSymbol3dIBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_onPathSymbol3dIBO);
            GL_CHECK_RESULT;
        }
        _onPathSymbol3dIBO.reset();
    }
    if (_onPathSymbol3dVBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_onPathSymbol3dVBO);
            GL_CHECK_RESULT;
        }
        _onPathSymbol3dVBO.reset();
    }

    if (_onPath3dProgram.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_onPath3dProgram.id);
            GL_CHECK_RESULT;
        }
        _onPath3dProgram = OnPathSymbol3dProgram();
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnSurfaceRaster()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);
    GL_CHECK_PRESENT(glDeleteShader);
    GL_CHECK_PRESENT(glDeleteProgram);

    // Compile vertex shader
    const QString vertexShader = QLatin1String(
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "INPUT vec2 in_vs_vertexTexCoords;                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "PARAM_OUTPUT vec2 v2f_texCoords;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mPerspectiveProjectionView;                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-symbol data
        "uniform vec2 param_vs_symbolOffsetFromTarget;                                                                      ""\n"
        "uniform float param_vs_direction;                                                                                  ""\n"
        "uniform ivec2 param_vs_symbolSize;                                                                                 ""\n"
        "uniform float param_vs_zDistanceFromCamera;                                                                        ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        // Get vertex coordinates in world
        "    float cos_a = cos(param_vs_direction);                                                                         ""\n"
        "    float sin_a = sin(param_vs_direction);                                                                         ""\n"
        "    vec2 p;                                                                                                        ""\n"
        "    p.x = in_vs_vertexPosition.x * float(param_vs_symbolSize.x);                                                   ""\n"
        "    p.y = in_vs_vertexPosition.y * float(param_vs_symbolSize.y);                                                   ""\n"
        "    vec4 v;                                                                                                        ""\n"
        "    v.x = param_vs_symbolOffsetFromTarget.x * %TileSize3D%.0 + (p.x*cos_a - p.y*sin_a);                            ""\n"
        "    v.y = 0.0;                                                                                                     ""\n"
        "    v.z = param_vs_symbolOffsetFromTarget.y * %TileSize3D%.0 + (p.x*sin_a + p.y*cos_a);                            ""\n"
        "    v.w = 1.0;                                                                                                     ""\n"
        "    gl_Position = param_vs_mPerspectiveProjectionView * v;                                                         ""\n"
        "    gl_Position.z = param_vs_zDistanceFromCamera;                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Prepare texture coordinates
        "    v2f_texCoords = in_vs_vertexTexCoords;                                                                         ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    preprocessedVertexShader.replace("%TileSize3D%", QString::number(AtlasMapRenderer::TileSize3D));
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    if (vsId == 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererSymbolsStage_OpenGL vertex shader");
        return false;
    }

    // Compile fragment shader
    const QString fragmentShader = QLatin1String(
        // Input data
        "PARAM_INPUT vec2 v2f_texCoords;                                                                                    ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        // Parameters: per-symbol data
        "uniform lowp sampler2D param_fs_sampler;                                                                           ""\n"
        "uniform lowp vec4 param_fs_modulationColor;                                                                        ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    FRAGMENT_COLOR_OUTPUT = SAMPLE_TEXTURE_2D(                                                                     ""\n"
        "        param_fs_sampler,                                                                                          ""\n"
        "        v2f_texCoords) * param_fs_modulationColor;                                                                 ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
    gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    if (fsId == 0)
    {
        glDeleteShader(vsId);
        GL_CHECK_RESULT;

        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererSymbolsStage_OpenGL fragment shader");
        return false;
    }

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    _onSurfaceRasterProgram.id = gpuAPI->linkProgram(2, shaders, true, &variablesMap);
    if (!_onSurfaceRasterProgram.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererSymbolsStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_onSurfaceRasterProgram.id, variablesMap);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.in.vertexTexCoords, "in_vs_vertexTexCoords", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.param.symbolOffsetFromTarget, "param_vs_symbolOffsetFromTarget", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.param.direction, "param_vs_direction", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.param.symbolSize, "param_vs_symbolSize", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.param.zDistanceFromCamera, "param_vs_zDistanceFromCamera", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.fs.param.sampler, "param_fs_sampler", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.fs.param.modulationColor, "param_fs_modulationColor", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_onSurfaceRasterProgram.id);
        GL_CHECK_RESULT;
        _onSurfaceRasterProgram.id.reset();

        return false;
    }

#pragma pack(push, 1)
    struct Vertex
    {
        // XY coordinates. Z is assumed to be 0
        float positionXY[2];

        // UV coordinates
        float textureUV[2];
    };
#pragma pack(pop)

    // Vertex data
    Vertex vertices[4] =
    {
        // In OpenGL, UV origin is BL. But since same rule applies to uploading texture data,
        // texture in memory is vertically flipped, so swap bottom and top UVs
        { { -0.5f, -0.5f }, { 0.0f, 1.0f } },//BL
        { { -0.5f,  0.5f }, { 0.0f, 0.0f } },//TL
        { {  0.5f,  0.5f }, { 1.0f, 0.0f } },//TR
        { {  0.5f, -0.5f }, { 1.0f, 1.0f } } //BR
    };
    const auto verticesCount = 4;

    // Index data
    GLushort indices[6] =
    {
        0, 1, 2,
        0, 2, 3
    };
    const auto indicesCount = 6;

    _onSurfaceRasterSymbolVAO = gpuAPI->allocateUninitializedVAO();

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_onSurfaceRasterSymbolVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _onSurfaceRasterSymbolVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_onSurfaceRasterProgram.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_onSurfaceRasterProgram.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, positionXY)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_onSurfaceRasterProgram.vs.in.vertexTexCoords);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_onSurfaceRasterProgram.vs.in.vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, textureUV)));
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_onSurfaceRasterSymbolIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _onSurfaceRasterSymbolIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->initializeVAO(_onSurfaceRasterSymbolVAO);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderOnSurfaceRasterSymbol(
    const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
    AlphaChannelType &currentAlphaChannelType,
    GLname& lastUsedProgram)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnSurfaceRasterMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
    //const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (lastUsedProgram != _onSurfaceRasterProgram.id)
    {
        GL_PUSH_GROUP_MARKER("use 'on-surface-raster' program");

        // Set symbol VAO
        gpuAPI->useVAO(_onSurfaceRasterSymbolVAO);
        
        // Activate program
        glUseProgram(_onSurfaceRasterProgram.id);
        GL_CHECK_RESULT;

        // Set perspective projection-view matrix
        glUniformMatrix4fv(_onSurfaceRasterProgram.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
        GL_CHECK_RESULT;

        // Activate texture block for symbol textures
        glActiveTexture(GL_TEXTURE0 + 0);
        GL_CHECK_RESULT;

        // Set proper sampler for texture block
        gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + 0, GPUAPI_OpenGL::SamplerType::Symbol);

        // Bind texture to sampler
        glUniform1i(_onSurfaceRasterProgram.fs.param.sampler, 0);
        GL_CHECK_RESULT;

        lastUsedProgram = _onSurfaceRasterProgram.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) on-surface raster \"%3\"]")
        .arg(QString::asprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->toString())
        .arg(qPrintable(symbol->content)));

    // Set symbol offset from target
    glUniform2f(_onSurfaceRasterProgram.vs.param.symbolOffsetFromTarget, renderable->offsetFromTarget.x, renderable->offsetFromTarget.y);
    GL_CHECK_RESULT;

    // Set symbol size
    glUniform2i(_onSurfaceRasterProgram.vs.param.symbolSize, gpuResource->width*internalState.pixelInWorldProjectionScale, gpuResource->height*internalState.pixelInWorldProjectionScale);
    GL_CHECK_RESULT;

    // Set direction
    glUniform1f(_onSurfaceRasterProgram.vs.param.direction, qDegreesToRadians(renderable->direction));
    GL_CHECK_RESULT;

    // Set distance from camera
    const auto zDistanceFromCamera = (internalState.mOrthographicProjection * glm::vec4(0.0f, 0.0f, -renderable->distanceToCamera, 1.0f)).z;
    glUniform1f(_onSurfaceRasterProgram.vs.param.zDistanceFromCamera, zDistanceFromCamera);
    GL_CHECK_RESULT;

    if (currentAlphaChannelType != gpuResource->alphaChannelType)
    {
        switch (gpuResource->alphaChannelType)
        {
            case AlphaChannelType::Premultiplied:
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                GL_CHECK_RESULT;
                break;
            case AlphaChannelType::Straight:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                GL_CHECK_RESULT;
                break;
            default:
                break;
        }

        currentAlphaChannelType = gpuResource->alphaChannelType;
    }

    // Activate symbol texture
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
    GL_CHECK_RESULT;

    // Set modulation color
    glUniform4f(_onSurfaceRasterProgram.fs.param.modulationColor,
        symbol->modulationColor.r,
        symbol->modulationColor.g,
        symbol->modulationColor.b,
        symbol->modulationColor.a);
    GL_CHECK_RESULT;

    // Apply settings from texture block to texture
    gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + 0);

    // Draw symbol actually
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    GL_CHECK_RESULT;

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnSurfaceRaster(const bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_onSurfaceRasterSymbolVAO.isValid())
    {
        gpuAPI->releaseVAO(_onSurfaceRasterSymbolVAO, gpuContextLost);
        _onSurfaceRasterSymbolVAO.reset();
    }
    
    if (_onSurfaceRasterSymbolIBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_onSurfaceRasterSymbolIBO);
            GL_CHECK_RESULT;
        }
        _onSurfaceRasterSymbolIBO.reset();
    }
    if (_onSurfaceRasterSymbolVBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_onSurfaceRasterSymbolVBO);
            GL_CHECK_RESULT;
        }
        _onSurfaceRasterSymbolVBO.reset();
    }
    
    if (_onSurfaceRasterProgram.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_onSurfaceRasterProgram.id);
            GL_CHECK_RESULT;
        }
        _onSurfaceRasterProgram = OnSurfaceSymbolProgram();
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnSurfaceVector()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteShader);
    GL_CHECK_PRESENT(glDeleteProgram);

    // Compile vertex shader
    const QString vertexShader = QLatin1String(
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "INPUT vec4 in_vs_vertexColor;                                                                                      ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "PARAM_OUTPUT vec4 v2f_color;                                                                                       ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "                                                                                                                   ""\n"
        // Parameters: per-symbol data
        "uniform mat4 param_vs_mModelViewProjection;                                                                        ""\n"
        "uniform float param_vs_zDistanceFromCamera;                                                                        ""\n"
        "uniform lowp vec4 param_vs_modulationColor;                                                                        ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        // Get vertex coordinates in world
        "    vec4 v;                                                                                                        ""\n"
        "    v.x = in_vs_vertexPosition.x;                                                                                  ""\n"
        "    v.y = 0.0;                                                                                                     ""\n"
        "    v.z = in_vs_vertexPosition.y;                                                                                  ""\n"
        "    v.w = 1.0;                                                                                                     ""\n"
        "    gl_Position = param_vs_mModelViewProjection * v;                                                               ""\n"
        "    gl_Position.z = param_vs_zDistanceFromCamera;                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Prepare color
        "    v2f_color.argb = in_vs_vertexColor.xyzw * param_vs_modulationColor.argb;                                       ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    preprocessedVertexShader.replace("%TileSize3D%", QString::number(AtlasMapRenderer::TileSize3D));
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    if (vsId == 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererSymbolsStage_OpenGL vertex shader");
        return false;
    }

    // Compile fragment shader
    const QString fragmentShader = QLatin1String(
        // Input data
        "PARAM_INPUT vec4 v2f_color;                                                                                        ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        // Parameters: per-symbol data
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    FRAGMENT_COLOR_OUTPUT = v2f_color;                                                                             ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
    gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    if (fsId == 0)
    {
        glDeleteShader(vsId);
        GL_CHECK_RESULT;

        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererSymbolsStage_OpenGL fragment shader");
        return false;
    }

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    _onSurfaceVectorProgram.id = gpuAPI->linkProgram(2, shaders, true, &variablesMap);
    if (!_onSurfaceVectorProgram.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererSymbolsStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_onSurfaceVectorProgram.id, variablesMap);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.in.vertexColor, "in_vs_vertexColor", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.mModelViewProjection, "param_vs_mModelViewProjection", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.zDistanceFromCamera, "param_vs_zDistanceFromCamera", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.modulationColor, "param_vs_modulationColor", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_onSurfaceVectorProgram.id);
        GL_CHECK_RESULT;
        _onSurfaceVectorProgram.id.reset();

        return false;
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderOnSurfaceVectorSymbol(
    const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
    AlphaChannelType &currentAlphaChannelType,
    GLname& lastUsedProgram)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnSurfaceVectorMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::MeshInGPU>(renderable->gpuResource);
    //const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (lastUsedProgram != _onSurfaceVectorProgram.id)
    {
        GL_PUSH_GROUP_MARKER("use 'on-surface-vector' program");

        // Activate program
        glUseProgram(_onSurfaceVectorProgram.id);
        GL_CHECK_RESULT;

        // Just in case un-use any possibly used VAO
        gpuAPI->unuseVAO();

        lastUsedProgram = _onSurfaceVectorProgram.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) on-surface vector]")
        .arg(QString::asprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->toString()));

    if (currentAlphaChannelType != AlphaChannelType::Straight)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_CHECK_RESULT;
        currentAlphaChannelType = AlphaChannelType::Straight;
    }

    // Activate vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->vertexBuffer->refInGPU)));
    GL_CHECK_RESULT;

    glEnableVertexAttribArray(*_onSurfaceVectorProgram.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_onSurfaceVectorProgram.vs.in.vertexPosition,
        2, GL_FLOAT, GL_FALSE,
        sizeof(VectorMapSymbol::Vertex),
        reinterpret_cast<GLvoid*>(offsetof(VectorMapSymbol::Vertex, positionXY)));
    GL_CHECK_RESULT;

    glEnableVertexAttribArray(*_onSurfaceVectorProgram.vs.in.vertexColor);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_onSurfaceVectorProgram.vs.in.vertexColor,
        4, GL_FLOAT, GL_FALSE,
        sizeof(VectorMapSymbol::Vertex),
        reinterpret_cast<GLvoid*>(offsetof(VectorMapSymbol::Vertex, color)));
    GL_CHECK_RESULT;

    // Activate index buffer (if present)
    if (gpuResource->indexBuffer)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->indexBuffer->refInGPU)));
        GL_CHECK_RESULT;
    }
    else
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        GL_CHECK_RESULT;
    }

    // Get proper scale
    PointI position31;
    if (gpuResource->position31 != nullptr)
    {
        position31 = PointI(gpuResource->position31->x, gpuResource->position31->y);
    }
    else
    {
        position31 = (renderable->instanceParameters && renderable->instanceParameters->overridesPosition31)
        ? renderable->instanceParameters->position31
        : symbol->getPosition31();
    }
    float scaleFactor = 1.0f;
    switch (symbol->scaleType)
    {
        case VectorMapSymbol::ScaleType::Raw:
            scaleFactor = symbol->scale;
            break;
        case VectorMapSymbol::ScaleType::In31:
            scaleFactor = symbol->scale * AtlasMapRenderer::TileSize3D /
                                Utilities::getPowZoom(31 - currentState.zoomLevel);
            break;
        case VectorMapSymbol::ScaleType::InMeters:
            scaleFactor = symbol->scale / Utilities::getMetersPerTileUnit(
                currentState.zoomLevel,
                position31.y >> (ZoomLevel31 - currentState.zoomLevel),
                AtlasMapRenderer::TileSize3D);
            break;
    }
    const auto mScale = glm::scale(glm::vec3(scaleFactor, scaleFactor, scaleFactor));

    // Calculate position translate
    const auto mPosition = glm::translate(glm::vec3(
        renderable->offsetFromTarget.x * AtlasMapRenderer::TileSize3D,
        0.0f,
        renderable->offsetFromTarget.y * AtlasMapRenderer::TileSize3D));

    // Calculate direction
    const auto mDirection = glm::rotate(renderable->direction, glm::vec3(0.0f, -1.0f, 0.0f));

    // Calculate and set model-view-projection matrix (scale -> direction -> position -> camera -> projection)
    const auto mModelViewProjection = internalState.mPerspectiveProjectionView * mPosition * mDirection * mScale;
    glUniformMatrix4fv(_onSurfaceVectorProgram.vs.param.mModelViewProjection, 1, GL_FALSE, glm::value_ptr(mModelViewProjection));
    GL_CHECK_RESULT;

    // Set distance from camera
    const auto zDistanceFromCamera = (internalState.mOrthographicProjection * glm::vec4(0.0f, 0.0f, -renderable->distanceToCamera, 1.0f)).z;
    glUniform1f(_onSurfaceVectorProgram.vs.param.zDistanceFromCamera, zDistanceFromCamera);
    GL_CHECK_RESULT;

    // Set modulation color
    glUniform4f(_onSurfaceVectorProgram.vs.param.modulationColor,
        symbol->modulationColor.r,
        symbol->modulationColor.g,
        symbol->modulationColor.b,
        symbol->modulationColor.a);
    GL_CHECK_RESULT;

    // Unbind symbol texture from texture sampler
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_RESULT;

    // Draw symbol actually
    GLenum primitivesType = GL_INVALID_ENUM;
    GLsizei count = 0;
    switch (symbol->primitiveType)
    {
        case VectorMapSymbol::PrimitiveType::TriangleFan:
            primitivesType = GL_TRIANGLE_FAN;
            count = gpuResource->vertexBuffer->itemsCount;
            break;
        case VectorMapSymbol::PrimitiveType::TriangleStrip:
            primitivesType = GL_TRIANGLE_STRIP;
            count = gpuResource->vertexBuffer->itemsCount;
            break;
        case VectorMapSymbol::PrimitiveType::Triangles:
            primitivesType = GL_TRIANGLES;
            count = gpuResource->vertexBuffer->itemsCount;
            break;
        case VectorMapSymbol::PrimitiveType::LineLoop:
            primitivesType = GL_LINE_LOOP;
            count = gpuResource->vertexBuffer->itemsCount;
            break;
        case VectorMapSymbol::PrimitiveType::Invalid:
        default:
            break;
    }
    if (gpuResource->indexBuffer)
    {
        static_assert(sizeof(GLushort) == sizeof(VectorMapSymbol::Index), "sizeof(GLushort) == sizeof(VectorMapSymbol::Index)");
        glDrawElements(primitivesType, count, GL_UNSIGNED_SHORT, nullptr);
        GL_CHECK_RESULT;
    }
    else
    {
        glDrawArrays(primitivesType, 0, count);
        GL_CHECK_RESULT;
    }

    // Turn off all buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnSurfaceVector(const bool gpuContextLost)
{
    //const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteProgram);

    if (_onSurfaceVectorProgram.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_onSurfaceVectorProgram.id);
            GL_CHECK_RESULT;
        }
        _onSurfaceVectorProgram = OnSurfaceVectorProgram();
    }

    return true;
}
