#include "AtlasMapRendererSymbolsStage_OpenGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "AtlasMapRenderer_OpenGL.h"
#include "MapSymbol.h"
#include "VectorMapSymbol.h"
#include "BillboardVectorMapSymbol.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "OnPathRasterMapSymbol.h"
#include "BillboardRasterMapSymbol.h"
#include "OnSurfaceRasterMapSymbol.h"
#include "MapSymbolsGroup.h"

OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::AtlasMapRendererSymbolsStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer_)
    : AtlasMapRendererSymbolsStage(renderer_)
    , AtlasMapRendererStageHelper_OpenGL(this)
    , _onPathSymbol2dMaxGlyphsPerDrawCall(-1)
    , _onPathSymbol3dMaxGlyphsPerDrawCall(-1)
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

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::render()
{
    bool ok = true;

    GL_PUSH_GROUP_MARKER(QLatin1String("symbols"));

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glDrawElements);

    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    prepare();

    int lastUsedProgram = -1;
    for (const auto& renderable_ : constOf(renderableSymbols))
    {
        if (const auto& renderable = std::dynamic_pointer_cast<const RenderableBillboardSymbol>(renderable_))
        {
            ok = ok && renderBillboardSymbol(
                renderable,
                lastUsedProgram);
        }
        else if (const auto& renderable = std::dynamic_pointer_cast<const RenderableOnPathSymbol>(renderable_))
        {
            ok = ok && renderOnPathSymbol(
                renderable,
                lastUsedProgram);
        }
        else if (const auto& renderable = std::dynamic_pointer_cast<const RenderableOnSurfaceSymbol>(renderable_))
        {
            ok = ok && renderOnSurfaceSymbol(
                renderable,
                lastUsedProgram);
        }
    }

    // Deactivate any symbol texture
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
    int& lastUsedProgram)
{
    if (std::dynamic_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol))
    {
        return renderBillboardRasterSymbol(
            renderable,
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
    int& lastUsedProgram)
{
    // Draw the glyphs
    if (renderable->is2D)
    {
        return renderOnPath2dSymbol(
            renderable,
            lastUsedProgram);
    }
    else
    {
        return renderOnPath3dSymbol(
            renderable,
            lastUsedProgram);
    }
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderOnSurfaceSymbol(
    const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
    int& lastUsedProgram)
{
    if (std::dynamic_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol))
    {
        return renderOnSurfaceRasterSymbol(
            renderable,
            lastUsedProgram);
    }
    else if (std::dynamic_pointer_cast<const VectorMapSymbol>(renderable->mapSymbol))
    {
        return renderOnSurfaceVectorSymbol(
            renderable,
            lastUsedProgram);
    }

    return false;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::release()
{
    bool ok = true;
    ok = ok && releaseBillboardRaster();
    ok = ok && releaseOnPath();
    ok = ok && releaseOnSurfaceRaster();
    ok = ok && releaseOnSurfaceVector();
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
        "uniform highp vec2 param_vs_symbolOffsetFromTarget;                                                                ""\n"
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
        // So it's possible to calculate current vertex location:
        // Initially, get location of current vertex in screen coordinates
        "    vec2 vertexOnScreen;                                                                                           ""\n"
        "    vertexOnScreen.x = in_vs_vertexPosition.x * float(param_vs_symbolSize.x);                                      ""\n"
        "    vertexOnScreen.y = in_vs_vertexPosition.y * float(param_vs_symbolSize.y);                                      ""\n"
        "    vertexOnScreen = vertexOnScreen + symbolLocationOnScreen.xy;                                                   ""\n"
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
        return false;

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
    int& lastUsedProgram)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const BillboardRasterMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (lastUsedProgram != *_billboardRasterProgram.id)
    {
        GL_PUSH_GROUP_MARKER("use 'billboard-raster' program");

        // Raster symbols use premultiplied alpha (due to SKIA)
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        GL_CHECK_RESULT;

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

        lastUsedProgram = _billboardRasterProgram.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) billboard raster \"%3\"]")
        .arg(QString().sprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->getDebugTitle())
        .arg(qPrintable(symbol->content)));

    // Set symbol offset from target
    glUniform2f(_billboardRasterProgram.vs.param.symbolOffsetFromTarget, renderable->offsetFromTarget.x, renderable->offsetFromTarget.y);
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

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseBillboardRaster()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_billboardRasterSymbolVAO.isValid())
    {
        gpuAPI->releaseVAO(_billboardRasterSymbolVAO);
        _billboardRasterSymbolVAO.reset();
    }

    if (_billboardRasterSymbolIBO.isValid())
    {
        glDeleteBuffers(1, &_billboardRasterSymbolIBO);
        GL_CHECK_RESULT;
        _billboardRasterSymbolIBO.reset();
    }
    if (_billboardRasterSymbolVBO.isValid())
    {
        glDeleteBuffers(1, &_billboardRasterSymbolVBO);
        GL_CHECK_RESULT;
        _billboardRasterSymbolVBO.reset();
    }
    
    if (_billboardRasterProgram.id.isValid())
    {
        glDeleteProgram(_billboardRasterProgram.id);
        GL_CHECK_RESULT;
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

    _onPathSymbol2dMaxGlyphsPerDrawCall = (gpuAPI->maxVertexUniformVectors - 8) / 5;

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
        "                                                                                                                   ""\n"
        // Get on-screen vertex coordinates
        "    float cos_a = cos(glyph.angle);                                                                                ""\n"
        "    float sin_a = sin(glyph.angle);                                                                                ""\n"
        "    vec2 p;                                                                                                        ""\n"
        "    p.x = in_vs_vertexPosition.x * glyph.width;                                                                    ""\n"
        "    p.y = in_vs_vertexPosition.y * param_vs_glyphHeight;                                                           ""\n"
        "    vec4 v;                                                                                                        ""\n"
        "    v.x = glyph.anchorPoint.x + (p.x*cos_a - p.y*sin_a);                                                           ""\n"
        "    v.y = glyph.anchorPoint.y + (p.x*sin_a + p.y*cos_a);                                                           ""\n"
        "    v.z = -param_vs_distanceFromCamera;                                                                            ""\n"
        "    v.w = 1.0;                                                                                                     ""\n"
        "    gl_Position = param_vs_mOrthographicProjection * v;                                                            ""\n"
        "                                                                                                                   ""\n"
        // Prepare texture coordinates
        "    v2f_texCoords.s = glyph.widthOfPreviousN + in_vs_vertexTexCoords.s*glyph.widthN;                               ""\n"
        "    v2f_texCoords.t = in_vs_vertexTexCoords.t; // Height is compatible as-is                                       ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    preprocessedVertexShader.replace("%MaxGlyphsPerDrawCall%", QString::number(_onPathSymbol2dMaxGlyphsPerDrawCall));
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
    glyphs.resize(_onPathSymbol2dMaxGlyphsPerDrawCall);
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
        return false;

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

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnPath3D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    _onPathSymbol3dMaxGlyphsPerDrawCall = (gpuAPI->maxVertexUniformVectors - 8) / 5;

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
    preprocessedVertexShader.replace("%MaxGlyphsPerDrawCall%", QString::number(_onPathSymbol3dMaxGlyphsPerDrawCall));
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
    glyphs.resize(_onPathSymbol3dMaxGlyphsPerDrawCall);
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
        return false;

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

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderOnPath2dSymbol(
    const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
    int& lastUsedProgram)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnPathRasterMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (lastUsedProgram != *_onPath2dProgram.id)
    {
        GL_PUSH_GROUP_MARKER("use 'on-path-2d' program");

        // Raster symbols use premultiplied alpha (due to SKIA)
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        GL_CHECK_RESULT;

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

        lastUsedProgram = _onPath2dProgram.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) SOP-2D \"%3\"]")
        .arg(QString().sprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->getDebugTitle())
        .arg(qPrintable(symbol->content)));

    // Set glyph height
    glUniform1f(_onPath2dProgram.vs.param.glyphHeight, gpuResource->height);
    GL_CHECK_RESULT;

    // Set distance from camera to symbol
    glUniform1f(_onPath2dProgram.vs.param.distanceFromCamera, renderable->distanceToCamera);
    GL_CHECK_RESULT;

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
    int glyphsDrawn = 0;
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
    int& lastUsedProgram)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnPathRasterMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (lastUsedProgram != *_onPath3dProgram.id)
    {
        GL_PUSH_GROUP_MARKER("use 'on-path-3d' program");

        // Raster symbols use premultiplied alpha (due to SKIA)
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        GL_CHECK_RESULT;

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

        lastUsedProgram = _onPath3dProgram.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) SOP-3D \"%3\"]")
        .arg(QString().sprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->getDebugTitle())
        .arg(qPrintable(symbol->content)));

    // Set glyph height
    glUniform1f(_onPath3dProgram.vs.param.glyphHeight, gpuResource->height*internalState.pixelInWorldProjectionScale);
    GL_CHECK_RESULT;

    // Set distance from camera
    const auto zDistanceFromCamera = (internalState.mOrthographicProjection * glm::vec4(0.0f, 0.0f, -renderable->distanceToCamera, 1.0f)).z;
    glUniform1f(_onPath3dProgram.vs.param.zDistanceFromCamera, zDistanceFromCamera);
    GL_CHECK_RESULT;

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
    int glyphsDrawn = 0;
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

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath()
{
    bool ok = true;
    ok = ok && releaseOnPath3D();
    ok = ok && releaseOnPath2D();
    return ok;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath2D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_onPathSymbol2dVAO.isValid())
    {
        gpuAPI->releaseVAO(_onPathSymbol2dVAO);
        _onPathSymbol2dVAO.reset();
    }

    if (_onPathSymbol2dIBO.isValid())
    {
        glDeleteBuffers(1, &_onPathSymbol2dIBO);
        GL_CHECK_RESULT;
        _onPathSymbol2dIBO.reset();
    }
    if (_onPathSymbol2dVBO.isValid())
    {
        glDeleteBuffers(1, &_onPathSymbol2dVBO);
        GL_CHECK_RESULT;
        _onPathSymbol2dVBO.reset();
    }
    
    if (_onPath2dProgram.id.isValid())
    {
        glDeleteProgram(_onPath2dProgram.id);
        GL_CHECK_RESULT;
        _onPath2dProgram = OnPathSymbol2dProgram();
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath3D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_onPathSymbol3dVAO.isValid())
    {
        gpuAPI->releaseVAO(_onPathSymbol3dVAO);
        _onPathSymbol3dVAO.reset();
    }


    if (_onPathSymbol3dIBO.isValid())
    {
        glDeleteBuffers(1, &_onPathSymbol3dIBO);
        GL_CHECK_RESULT;
        _onPathSymbol3dIBO.reset();
    }
    if (_onPathSymbol3dVBO.isValid())
    {
        glDeleteBuffers(1, &_onPathSymbol3dVBO);
        GL_CHECK_RESULT;
        _onPathSymbol3dVBO.reset();
    }

    if (_onPath3dProgram.id.isValid())
    {
        glDeleteProgram(_onPath3dProgram.id);
        GL_CHECK_RESULT;
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
        "uniform highp vec2 param_vs_symbolOffsetFromTarget;                                                                ""\n"
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
        return false;

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
    int& lastUsedProgram)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnSurfaceRasterMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (lastUsedProgram != *_onSurfaceRasterProgram.id)
    {
        GL_PUSH_GROUP_MARKER("use 'on-surface-raster' program");

        // Raster symbols use premultiplied alpha (due to SKIA)
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        GL_CHECK_RESULT;

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

        lastUsedProgram = _onSurfaceRasterProgram.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) on-surface raster \"%3\"]")
        .arg(QString().sprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->getDebugTitle())
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

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnSurfaceRaster()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_onSurfaceRasterSymbolVAO.isValid())
    {
        gpuAPI->releaseVAO(_onSurfaceRasterSymbolVAO);
        _onSurfaceRasterSymbolVAO.reset();
    }
    
    if (_onSurfaceRasterSymbolIBO.isValid())
    {
        glDeleteBuffers(1, &_onSurfaceRasterSymbolIBO);
        GL_CHECK_RESULT;
        _onSurfaceRasterSymbolIBO.reset();
    }
    if (_onSurfaceRasterSymbolVBO.isValid())
    {
        glDeleteBuffers(1, &_onSurfaceRasterSymbolVBO);
        GL_CHECK_RESULT;
        _onSurfaceRasterSymbolVBO.reset();
    }
    
    if (_onSurfaceRasterProgram.id.isValid())
    {
        glDeleteProgram(_onSurfaceRasterProgram.id);
        GL_CHECK_RESULT;
        _onSurfaceRasterProgram = OnSurfaceSymbolProgram();
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnSurfaceVector()
{
    const auto gpuAPI = getGPUAPI();

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
        return false;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderOnSurfaceVectorSymbol(
    const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
    int& lastUsedProgram)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnSurfaceVectorMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::MeshInGPU>(renderable->gpuResource);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (lastUsedProgram != *_onSurfaceVectorProgram.id)
    {
        GL_PUSH_GROUP_MARKER("use 'on-surface-vector' program");

        // Raster symbols use non-multiplied alpha
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_CHECK_RESULT;

        // Activate program
        glUseProgram(_onSurfaceVectorProgram.id);
        GL_CHECK_RESULT;

        // Just in case un-use any possibly used VAO
        gpuAPI->unuseVAO();

        lastUsedProgram = _onSurfaceVectorProgram.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) on-surface vector]")
        .arg(QString().sprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->getDebugTitle()));

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
    const auto& position31 =
        (renderable->instanceParameters && renderable->instanceParameters->overridesPosition31)
        ? renderable->instanceParameters->position31
        : symbol->getPosition31();
    float scaleFactor = 1.0f;
    switch (symbol->scaleType)
    {
        case VectorMapSymbol::ScaleType::Raw:
            scaleFactor = symbol->scale;
            break;
        case VectorMapSymbol::ScaleType::InMeters:
            scaleFactor = symbol->scale / Utilities::getMetersPerTileUnit(
                currentState.zoomBase,
                position31.y >> (ZoomLevel31 - currentState.zoomBase),
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

    // Deactivate symbol texture
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
        case VectorMapSymbol::PrimitiveType::LineLoop:
            primitivesType = GL_LINE_LOOP;
            count = gpuResource->vertexBuffer->itemsCount;
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

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnSurfaceVector()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteProgram);

    if (_onSurfaceVectorProgram.id.isValid())
    {
        glDeleteProgram(_onSurfaceVectorProgram.id);
        GL_CHECK_RESULT;
        _onSurfaceVectorProgram = OnSurfaceVectorProgram();
    }

    return true;
}
