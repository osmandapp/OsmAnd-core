#include "AtlasMapRendererSymbolsStage_OpenGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "QtExtensions.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "Utilities.h"
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
#include "GlmExtensions.h"
#include <OsmAndCore/Map/MapObjectsSymbolsProvider.h>
#include <OsmAndCore/Data/BinaryMapObject.h>

OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::AtlasMapRendererSymbolsStage_OpenGL(AtlasMapRenderer_OpenGL* renderer_)
    : AtlasMapRendererSymbolsStage(renderer_)
    , AtlasMapRendererStageHelper_OpenGL(this)
    , _onPathSymbol2dMaxGlyphsPerDrawCall(0)
    , _onPathSymbol3dMaxGlyphsPerDrawCall(0)
{
}

OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::~AtlasMapRendererSymbolsStage_OpenGL() = default;

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initialize()
{
    bool ok = true;
    ok = ok && initializeBillboardRaster();
    ok = ok && initializeOnPath();
    ok = ok && initializeOnSurfaceRaster();
    ok = ok && initializeOnSurfaceVector();
    ok = ok && initializeVisibilityCheck();
    return ok;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* metric_)
{
    const auto metric = dynamic_cast<AtlasMapRenderer_Metrics::Metric_renderFrame*>(metric_);

    bool ok = true;

    GL_PUSH_GROUP_MARKER(QLatin1String("symbols"));

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glActiveTexture);

    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    _lastUsedProgram = 0;

    // Disable actual drawing for symbol preparation phase
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    GL_CHECK_RESULT;
    
    prepare(metric);
    
    // Resume drawing
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    GL_CHECK_RESULT;

    // Initially, configure for straight alpha channel type
    auto currentAlphaChannelType = AlphaChannelType::Straight;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_RESULT;

    // Prepare JSON report
    QJsonArray jsonArray;
    bool withJson = renderer->withJSON();

    for (const auto& renderableSymbol : constOf(renderableSymbols))
    {
        if (const auto& renderableBillboardSymbol = std::dynamic_pointer_cast<const RenderableBillboardSymbol>(renderableSymbol))
        {
            Stopwatch renderBillboardSymbolStopwatch(metric != nullptr);
            ok = ok && renderBillboardSymbol(renderableBillboardSymbol, currentAlphaChannelType);
            if (withJson)
            {
                QJsonObject jsonObject;
                jsonObject.insert(QStringLiteral("type"), QStringLiteral("billboard"));
                const auto& symbol =
                    std::static_pointer_cast<const BillboardRasterMapSymbol>(renderableBillboardSymbol->mapSymbol);
                const auto& position31 = (renderableBillboardSymbol->instanceParameters
                    && renderableBillboardSymbol->instanceParameters->overridesPosition31)
                    ? renderableBillboardSymbol->instanceParameters->position31
                    : symbol->getPosition31();
                jsonObject.insert(QStringLiteral("lat"), Utilities::getLatitudeFromTile(ZoomLevel31, position31.y));
                jsonObject.insert(QStringLiteral("lon"), Utilities::getLongitudeFromTile(ZoomLevel31, position31.x));
                reportCommonParameters(jsonObject, *renderableSymbol);
                jsonArray.append(jsonObject);
            }
            if (metric)
            {
                metric->elapsedTimeForBillboardSymbolsRendering += renderBillboardSymbolStopwatch.elapsed();
                metric->billboardSymbolsRendered += 1;
            }
        }
        else if (const auto& renderableOnPathSymbol = std::dynamic_pointer_cast<const RenderableOnPathSymbol>(renderableSymbol))
        {
            Stopwatch renderOnPathSymbolStopwatch(metric != nullptr);
            ok = ok && renderOnPathSymbol(renderableOnPathSymbol, currentAlphaChannelType);
            if (withJson)
            {
                QJsonObject jsonObject;
                jsonObject.insert(QStringLiteral("type"),
                    renderableOnPathSymbol->is2D ? QStringLiteral("on-path-2D") : QStringLiteral("on-path-3D"));
                const auto& symbol =
                    std::static_pointer_cast<const OnPathRasterMapSymbol>(renderableOnPathSymbol->mapSymbol);
                const auto& position31 = (renderableOnPathSymbol->instanceParameters
                    && renderableOnPathSymbol->instanceParameters->overridesPinPointOnPath)
                    ? renderableOnPathSymbol->instanceParameters->pinPointOnPath.point31
                    : symbol->pinPointOnPath.point31;
                jsonObject.insert(QStringLiteral("lat"), Utilities::getLatitudeFromTile(ZoomLevel31, position31.y));
                jsonObject.insert(QStringLiteral("lon"), Utilities::getLongitudeFromTile(ZoomLevel31, position31.x));
                reportCommonParameters(jsonObject, *renderableSymbol);
                jsonArray.append(jsonObject);
            }
            if (metric)
            {
                metric->elapsedTimeForOnPathSymbolsRendering += renderOnPathSymbolStopwatch.elapsed();
                metric->onPathSymbolsRendered += 1;
            }
        }
        else if (const auto& renderableOnSurfaceSymbol = std::dynamic_pointer_cast<const RenderableOnSurfaceSymbol>(renderableSymbol))
        {
            Stopwatch renderOnSurfaceSymbolStopwatch(metric != nullptr);
            ok = ok && renderOnSurfaceSymbol(renderableOnSurfaceSymbol, currentAlphaChannelType);
            if (metric)
            {
                metric->elapsedTimeForOnSurfaceSymbolsRendering += renderOnSurfaceSymbolStopwatch.elapsed();
                metric->onSurfaceSymbolsRendered += 1;
            }
        }
    }

    if (withJson)
    {
        auto jsonDocument = new QJsonDocument();
        jsonDocument->setArray(jsonArray);
        renderer->setJSON(jsonDocument);
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
    AlphaChannelType &currentAlphaChannelType)
{
    if (std::dynamic_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol))
    {
        return renderBillboardRasterSymbol(
            renderable,
            currentAlphaChannelType);
    }

    return false;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderOnPathSymbol(
    const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
    AlphaChannelType &currentAlphaChannelType)
{
    // Draw the glyphs
    if (renderable->is2D)
    {
        return renderOnPath2dSymbol(
            renderable,
            currentAlphaChannelType);
    }
    else
    {
        return renderOnPath3dSymbol(
            renderable,
            currentAlphaChannelType);
    }
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::renderOnSurfaceSymbol(
    const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
    AlphaChannelType &currentAlphaChannelType)
{
    if (std::dynamic_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol))
    {
        return renderOnSurfaceRasterSymbol(
            renderable,
            currentAlphaChannelType);
    }
    else if (std::dynamic_pointer_cast<const VectorMapSymbol>(renderable->mapSymbol))
    {
        return renderOnSurfaceVectorSymbol(
            renderable,
            currentAlphaChannelType);
    }

    return false;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::release(bool gpuContextLost)
{
    bool ok = true;
    ok = ok && releaseBillboardRaster(gpuContextLost);
    ok = ok && releaseOnPath(gpuContextLost);
    ok = ok && releaseOnSurfaceRaster(gpuContextLost);
    ok = ok && releaseOnSurfaceVector(gpuContextLost);
    ok = ok && releaseVisibilityCheck(gpuContextLost);
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
        "uniform highp ivec4 param_vs_target31; // x, y, zoom, tileSize31                                                   ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-tile data
        "uniform vec4 param_vs_elevation_scale;                                                                             ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-symbol data
        "uniform highp ivec2 param_vs_position31;                                                                           ""\n"
        "uniform highp vec2 param_vs_offsetInTile;                                                                          ""\n"
        "uniform ivec2 param_vs_symbolSize;                                                                                 ""\n"
        "uniform float param_vs_distanceFromCamera;                                                                         ""\n"
        "uniform ivec2 param_vs_onScreenOffset;                                                                             ""\n"
        "uniform float param_vs_elevationInMeters;                                                                          ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        // Calculate position of symbol in world coordinate system.
        "    highp ivec2 offsetFromTarget31 = param_vs_position31 - param_vs_target31.xy;                                   ""\n"
        "    highp ivec2 correctOverlap;                                                                                    ""\n"
        "    correctOverlap.x = offsetFromTarget31.x >= 1073741824 ? -1 : (offsetFromTarget31.x < -1073741824 ? 1 : 0);     ""\n"
        "    correctOverlap.y = offsetFromTarget31.y >= 1073741824 ? -1 : (offsetFromTarget31.y < -1073741824 ? 1 : 0);     ""\n"
        "    offsetFromTarget31 += correctOverlap * 2147483647 + correctOverlap;                                            ""\n"
        "    highp vec2 offsetFromTarget = vec2(0.0);                                                                       ""\n"
        "#if INTEGER_OPERATIONS_SUPPORTED                                                                                   ""\n"
        "    {                                                                                                              ""\n"
        "        highp ivec2 offsetFromTargetT = offsetFromTarget31 / param_vs_target31.w;                                  ""\n"
        "        highp vec2 offsetFromTargetF = vec2(offsetFromTarget31 - offsetFromTargetT * param_vs_target31.w);         ""\n"
        "        offsetFromTarget = vec2(offsetFromTargetT) + offsetFromTargetF / float(param_vs_target31.w);               ""\n"
        "    }                                                                                                              ""\n"
        "#else // !INTEGER_OPERATIONS_SUPPORTED                                                                             ""\n"
        "    offsetFromTarget = vec2(offsetFromTarget31);                                                                   ""\n"
        "    offsetFromTarget /= float(param_vs_target31.w);                                                                ""\n"
        "#endif // INTEGER_OPERATIONS_SUPPORTED                                                                             ""\n"
        "    vec4 symbolInWorld;                                                                                            ""\n"
        "    symbolInWorld.xz = offsetFromTarget * %TileSize3D%.0;                                                          ""\n"
        "    symbolInWorld.y = 0.0;                                                                                         ""\n"
        "    symbolInWorld.w = 1.0;                                                                                         ""\n"
        "                                                                                                                   ""\n"
        // Retrieve symbol position elevation
        "    if (abs(param_vs_elevation_scale.w) > 0.0)                                                                     ""\n"
        "    {                                                                                                              ""\n"
        "        float metersPerUnit = mix(param_vs_elevation_scale.x, param_vs_elevation_scale.y, param_vs_offsetInTile.y);""\n"
        "        float heightInMeters = param_vs_elevationInMeters * param_vs_elevation_scale.w;                            ""\n"
        "        symbolInWorld.y = (heightInMeters / metersPerUnit) * param_vs_elevation_scale.z;                           ""\n"
        "    }                                                                                                              ""\n"
        "                                                                                                                   ""\n"
        // Project position of symbol from world coordinate system to clipping space and to normalized device coordinates ([-1 .. 1])
        "    vec4 symbolInClipping = param_vs_mPerspectiveProjectionView * symbolInWorld;                                   ""\n"
        "    vec3 symbolInNDC = symbolInClipping.xyz / symbolInClipping.w;                                                  ""\n"
        "                                                                                                                   ""\n"
        // Using viewport size, get real screen coordinates and correct depth to be [0 .. 1]
        "    vec3 symbolOnScreen;                                                                                           ""\n"
        "    symbolOnScreen.xy = symbolInNDC.xy * 0.5 + 0.5;                                                                ""\n"
        "    symbolOnScreen.x = symbolOnScreen.x * param_vs_viewport.z + param_vs_viewport.x;                               ""\n"
        "    symbolOnScreen.y = symbolOnScreen.y * param_vs_viewport.w + param_vs_viewport.y;                               ""\n"
        "    symbolOnScreen.z = (1.0 + symbolOnScreen.z) * 0.5;                                                             ""\n"
        "                                                                                                                   ""\n"
        // Add on-screen offset
        "    symbolOnScreen.xy += vec2(param_vs_onScreenOffset);                                                            ""\n"
        "                                                                                                                   ""\n"
        // symbolOnScreen.xy now contains correct coordinates in viewport,
        // which can be used in orthographic projection (if it was configured to match viewport).
        //
        // To provide pixel-perfect rendering of billboard raster symbols:
        // symbolOnScreen.(x|y) has to be rounded and +0.5 in case param_vs_symbolSize.(x|y) is even
        // symbolOnScreen.(x|y) has to be rounded in case param_vs_symbolSize.(x|y) is odd
        "    symbolOnScreen = floor(symbolOnScreen);                                                                        ""\n"
        "#if INTEGER_OPERATIONS_SUPPORTED                                                                                   ""\n"
        "    symbolOnScreen.x += float(1 - param_vs_symbolSize.x & 1) * 0.5;                                                ""\n"
        "    symbolOnScreen.y += float(1 - param_vs_symbolSize.y & 1) * 0.5;                                                ""\n"
        "#else // !INTEGER_OPERATIONS_SUPPORTED                                                                             ""\n"
        "    symbolOnScreen.xy += (vec2(1.0) - mod(vec2(param_vs_symbolSize), vec2(2.0))) * 0.5;                            ""\n"
        "#endif // INTEGER_OPERATIONS_SUPPORTED                                                                             ""\n"
        "                                                                                                                   ""\n"
        // So it's possible to calculate current vertex location:
        // Initially, get location of current vertex in screen coordinates
        "    vec2 vertexOnScreen = in_vs_vertexPosition * vec2(param_vs_symbolSize) + symbolOnScreen.xy;                    ""\n"
        "                                                                                                                   ""\n"
        // To provide pixel-perfect result, vertexOnScreen needs to be rounded
        "    vertexOnScreen = floor(vertexOnScreen + vec2(0.5, 0.5));                                                       ""\n"
        "                                                                                                                   ""\n"
        // There's no need to perform unprojection into orthographic world space, just multiply these coordinates by
        // orthographic projection matrix (View and Model being identity), yet use Z from perspective NDC
        "    vec4 vertex;                                                                                                   ""\n"
        "    vertex.xy = vertexOnScreen.xy;                                                                                 ""\n"
        "    vertex.z = 0.0;                                                                                                ""\n"
        "    vertex.w = 1.0;                                                                                                ""\n"
        "    gl_Position = param_vs_mOrthographicProjection * vertex;                                                       ""\n"
        "    gl_Position.z = symbolInNDC.z * gl_Position.w;                                                                 ""\n"
        "                                                                                                                   ""\n"
        // Texture coordinates are simply forwarded from input
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
    ok = ok
         && lookup->lookupLocation(
             _billboardRasterProgram.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.mOrthographicProjection, "param_vs_mOrthographicProjection", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.viewport, "param_vs_viewport", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.target31, "param_vs_target31", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.elevation_scale, "param_vs_elevation_scale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.position31, "param_vs_position31", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.offsetInTile, "param_vs_offsetInTile", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.symbolSize, "param_vs_symbolSize", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.onScreenOffset, "param_vs_onScreenOffset", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_billboardRasterProgram.vs.param.elevationInMeters, "param_vs_elevationInMeters", GlslVariableType::Uniform);
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
    AlphaChannelType &currentAlphaChannelType)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const BillboardRasterMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (_lastUsedProgram != _billboardRasterProgram.id)
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

        // Set target31
        glUniform4i(
            _billboardRasterProgram.vs.param.target31,
            currentState.target31.x,
            currentState.target31.y,
            currentState.zoomLevel,
            static_cast<GLint>(1u << (ZoomLevel::MaxZoomLevel - currentState.zoomLevel))
        );
        GL_CHECK_RESULT;

        // Activate texture block for symbol textures
        glActiveTexture(GL_TEXTURE0 + 0);
        GL_CHECK_RESULT;

        // Set proper sampler for texture block
        gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + 0, GPUAPI_OpenGL::SamplerType::Symbol);

        // Bind texture to sampler
        glUniform1i(_billboardRasterProgram.fs.param.sampler, 0);
        GL_CHECK_RESULT;

        // Change depth test function to accept everything to avoid issues with terrain (regardless of elevation presence)
        glDepthFunc(GL_ALWAYS);
        GL_CHECK_RESULT;

        _lastUsedProgram = _billboardRasterProgram.id;

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

    // Parameters: per-tile data
    if (renderable->positionInWorld.y != 0.0f)
    {
        const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(
            currentState.zoomLevel,
            renderable->tileId.y,
            AtlasMapRenderer::TileSize3D);
        const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(
            currentState.zoomLevel,
            renderable->tileId.y + 1,
            AtlasMapRenderer::TileSize3D);
        glUniform4f(
            _billboardRasterProgram.vs.param.elevation_scale,
            (float)upperMetersPerUnit,
            (float)lowerMetersPerUnit,
            currentState.elevationConfiguration.zScaleFactor,
            currentState.elevationConfiguration.dataScaleFactor);
        GL_CHECK_RESULT;
    }
    else
    {
        glUniform4f(_billboardRasterProgram.vs.param.elevation_scale, 0.0f, 0.0f, 0.0f, 0.0f);
        GL_CHECK_RESULT;
    }

    // Per-symbol data
    const auto& position31 = (renderable->instanceParameters && renderable->instanceParameters->overridesPosition31)
                                 ? renderable->instanceParameters->position31
                                 : symbol->getPosition31();
    glUniform2i(_billboardRasterProgram.vs.param.position31, position31.x, position31.y);

    glUniform2f(_billboardRasterProgram.vs.param.offsetInTile, renderable->offsetInTileN.x, renderable->offsetInTileN.y);
    GL_CHECK_RESULT;

    // Set symbol size
    glUniform2i(_billboardRasterProgram.vs.param.symbolSize, gpuResource->width, gpuResource->height);
    GL_CHECK_RESULT;

    // Set on-screen offset
    const auto& offsetOnScreen =
        (renderable->instanceParameters && renderable->instanceParameters->overridesOffset)
        ? renderable->instanceParameters->offset
        : symbol->offset;
    glUniform2i(_billboardRasterProgram.vs.param.onScreenOffset, offsetOnScreen.x, -offsetOnScreen.y);
    GL_CHECK_RESULT;

    // Set elevation
    glUniform1f(_billboardRasterProgram.vs.param.elevationInMeters, renderable->elevationInMeters);
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

    glActiveTexture(GL_TEXTURE0 + 0);
    GL_CHECK_RESULT;

    // Activate symbol texture
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
    GL_CHECK_RESULT;
   
    // Apply symbols opacity factor to modulation color
    auto modulationColor = symbol->modulationColor;
    auto opacityFactor = renderable->opacityFactor * currentState.symbolsOpacity;
    modulationColor.a *= opacityFactor;
    if (currentAlphaChannelType == AlphaChannelType::Premultiplied)
    {
        modulationColor.r *= opacityFactor;
        modulationColor.g *= opacityFactor;
        modulationColor.b *= opacityFactor;
    }

    // Set modulation color
    glUniform4f(_billboardRasterProgram.fs.param.modulationColor,
        modulationColor.r,
        modulationColor.g,
        modulationColor.b,
        modulationColor.a);
    GL_CHECK_RESULT;

    // Apply settings from texture block to texture
    gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + 0);

    // Draw symbol actually
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    GL_CHECK_RESULT;

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseBillboardRaster(bool gpuContextLost)
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
        1 /*param_vs_zDistanceFromCamera*/ +
        1 /*param_vs_currentOffset*/;
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
        "uniform vec2 param_vs_currentOffset;                                                                               ""\n"
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
        "    vec2 anchorPoint = glyph.anchorPoint + param_vs_currentOffset;                                                 ""\n"
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
    ok = ok && lookup->lookupLocation(_onPath2dProgram.vs.param.currentOffset, "param_vs_currentOffset", GlslVariableType::Uniform);
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
        1 /*param_vs_zDistanceFromCamera*/ +
        1 /*param_vs_currentOffset*/;
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
        p0.glyphIndex = (float)glyphIdx;

        auto& p1 = *(pVertex++);
        p1 = templateVertices[1];
        p1.glyphIndex = (float)glyphIdx;

        auto& p2 = *(pVertex++);
        p2 = templateVertices[2];
        p2.glyphIndex = (float)glyphIdx;

        auto& p3 = *(pVertex++);
        p3 = templateVertices[3];
        p3.glyphIndex = (float)glyphIdx;
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
        "uniform vec2 param_vs_currentOffset;                                                                               ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-glyph data
        "struct Glyph                                                                                                       ""\n"
        "{                                                                                                                  ""\n"
        "    vec3 anchorPoint;                                                                                              ""\n"
        "    vec4 angle;                                                                                                    ""\n"
        "    float width;                                                                                                   ""\n"
        "    float widthOfPreviousN;                                                                                        ""\n"
        "    float widthN;                                                                                                  ""\n"
        "};                                                                                                                 ""\n"
        "uniform Glyph param_vs_glyphs[%MaxGlyphsPerDrawCall%];                                                             ""\n"
        "                                                                                                                   ""\n"
        "vec3 rotateY(vec3 p, float angle)                                                                                  ""\n"
        "{                                                                                                                  ""\n"
        "    float sn = sin(angle);                                                                                         ""\n"
        "    float cs = cos(angle);                                                                                         ""\n"
        "    float x = p.x * cs - p.z * sn;                                                                                 ""\n"
        "    float z = p.x * sn + p.z * cs;                                                                                 ""\n"
        "    return vec3(x, p.y, z);                                                                                        ""\n"
        "}                                                                                                                  ""\n"
        "                                                                                                                   ""\n"
        "vec3 rotateXZ(vec3 p, float angle, vec2 u)                                                                         ""\n"
        "{                                                                                                                  ""\n"
        "    float sn = sin(angle);                                                                                         ""\n"
        "    float cs = cos(angle);                                                                                         ""\n"
        "    float csr = 1.0 - cs;                                                                                          ""\n"
        "    float x = p.x * (cs + u.x * u.x * csr) - p.y * u.y * sn + p.z * u.x * u.y * csr;                               ""\n"
        "    float y = p.x * u.y * sn + p.y * cs - p.z * u.x * sn;                                                          ""\n"
        "    float z = p.x * u.x * u.y * csr + p.y * u.x * sn + p.z * (cs + u.y * u.y * csr);                               ""\n"
        "    return vec3(x, y, z);                                                                                          ""\n"
        "}                                                                                                                  ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    Glyph glyph = param_vs_glyphs[int(in_vs_glyphIndex)];                                                          ""\n"
        "                                                                                                                   ""\n"
        // Get on-screen vertex coordinates
        "    vec3 p;                                                                                                        ""\n"
        "    p.x = in_vs_vertexPosition.x * glyph.width;                                                                    ""\n"
        "    p.y = 0.0;                                                                                                     ""\n"
        "    p.z = in_vs_vertexPosition.y * param_vs_glyphHeight;                                                           ""\n"
        "    p = rotateY(p, glyph.angle.y);                                                                                 ""\n"
        "    p = rotateXZ(p, glyph.angle.x, glyph.angle.zw);                                                                ""\n"
        "    vec4 v;                                                                                                        ""\n"
        "    v.x = glyph.anchorPoint.x + p.x - param_vs_currentOffset.x;                                                    ""\n"
        "    v.y = glyph.anchorPoint.y + p.y;                                                                               ""\n"
        "    v.z = glyph.anchorPoint.z + p.z - param_vs_currentOffset.y;                                                    ""\n"
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
    ok = ok && lookup->lookupLocation(_onPath3dProgram.vs.param.currentOffset, "param_vs_currentOffset", GlslVariableType::Uniform);
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
    AlphaChannelType &currentAlphaChannelType)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnPathRasterMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (_lastUsedProgram != _onPath2dProgram.id)
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

        // Change depth test function to accept everything to avoid issues with terrain (regardless of elevation presence)
        glDepthFunc(GL_ALWAYS);
        GL_CHECK_RESULT;

        _lastUsedProgram = _onPath2dProgram.id;

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

    // Apply symbols opacity factor to modulation color
    auto modulationColor = symbol->modulationColor;
    auto opacityFactor = renderable->opacityFactor * currentState.symbolsOpacity;
    modulationColor.a *= opacityFactor;
    if (currentAlphaChannelType == AlphaChannelType::Premultiplied)
    {
        modulationColor.r *= opacityFactor;
        modulationColor.g *= opacityFactor;
        modulationColor.b *= opacityFactor;
    }

    // Set modulation color
    glUniform4f(_onPath2dProgram.fs.param.modulationColor,
        modulationColor.r,
        modulationColor.g,
        modulationColor.b,
        modulationColor.a);
    GL_CHECK_RESULT;

    // Set current offset
    const auto currentOffset31 = Utilities::shortestVector31(renderable->target31, currentState.target31);
    const auto currentOffset = Utilities::convert31toFloat(currentOffset31, currentState.zoomLevel)
        * static_cast<float>(AtlasMapRenderer::TileSize3D);
    const auto pinPointInWorld = glm::vec3(
        renderable->pinPointInWorld.x - currentOffset.x,
        renderable->pinPointInWorld.y,
        renderable->pinPointInWorld.z - currentOffset.y);
    const auto pointOnScreen = glm_extensions::project(
        pinPointInWorld,
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport).xy();
    const auto offsetOnScreen = pointOnScreen - renderable->pinPointOnScreen;
    glUniform2f(_onPath2dProgram.vs.param.currentOffset, offsetOnScreen.x, offsetOnScreen.y);
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
            glUniform1f(vsGlyph.angle, glyph.angleY);
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
    AlphaChannelType &currentAlphaChannelType)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnPathRasterMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (_lastUsedProgram != _onPath3dProgram.id)
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

        // Change depth test function to perform <= depth test (regardless of elevation presence)
        glDepthFunc(GL_LEQUAL);
        GL_CHECK_RESULT;

        _lastUsedProgram = _onPath3dProgram.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) SOP-3D \"%3\"]")
        .arg(QString::asprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->toString())
        .arg(qPrintable(symbol->content)));

    // Set glyph height
    glUniform1f(_onPath3dProgram.vs.param.glyphHeight, gpuResource->height * renderable->pixelSizeInWorld);
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

    // Apply symbols opacity factor to modulation color
    auto modulationColor = symbol->modulationColor;
    auto opacityFactor = renderable->opacityFactor * currentState.symbolsOpacity;
    modulationColor.a *= opacityFactor;
    if (currentAlphaChannelType == AlphaChannelType::Premultiplied)
    {
        modulationColor.r *= opacityFactor;
        modulationColor.g *= opacityFactor;
        modulationColor.b *= opacityFactor;
    }

    // Set modulation color
    glUniform4f(_onPath3dProgram.fs.param.modulationColor,
        modulationColor.r,
        modulationColor.g,
        modulationColor.b,
        modulationColor.a);
    GL_CHECK_RESULT;

    // Apply settings from texture block to texture
    gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + 0);

    // Set current offset
    const auto currentOffset31 = Utilities::shortestVector31(renderable->target31, currentState.target31);
    const auto currentOffset = Utilities::convert31toFloat(currentOffset31, currentState.zoomLevel)
        * static_cast<float>(AtlasMapRenderer::TileSize3D);
    glUniform2f(_onPath3dProgram.vs.param.currentOffset, currentOffset.x, currentOffset.y);
    GL_CHECK_RESULT;

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
            glm::vec3 elevatedAnchorPoint(glyph.anchorPoint.x, glyph.elevation, glyph.anchorPoint.y);
            glUniform3fv(vsGlyph.anchorPoint, 1, glm::value_ptr(elevatedAnchorPoint));
            GL_CHECK_RESULT;

            // Set glyph width
            glUniform1f(vsGlyph.width, glyph.width * renderable->pixelSizeInWorld);
            GL_CHECK_RESULT;

            // Set angle
            glm::vec4 angle(
                glyph.angleXZ,
                Utilities::normalizedAngleRadians(glyph.angleY + M_PI),
                glyph.rotationX,
                glyph.rotationZ);
            glUniform4fv(vsGlyph.angle, 1, glm::value_ptr(angle));
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

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath(bool gpuContextLost)
{
    bool ok = true;
    ok = ok && releaseOnPath3D(gpuContextLost);
    ok = ok && releaseOnPath2D(gpuContextLost);
    return ok;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath2D(bool gpuContextLost)
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

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath3D(bool gpuContextLost)
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
        // Parameters: per-tile data
        "uniform vec4 param_vs_elevation_scale;                                                                             ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-symbol data
        "uniform vec2 param_vs_symbolOffsetFromTarget;                                                                      ""\n"
        "uniform float param_vs_direction;                                                                                  ""\n"
        "uniform vec2 param_vs_symbolSize;                                                                                  ""\n"
        "uniform float param_vs_zDistanceFromCamera;                                                                        ""\n"
        "uniform float param_vs_elevationInMeters;                                                                          ""\n"
        "uniform highp vec2 param_vs_offsetInTile;                                                                          ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        // Get vertex coordinates in world
        "    float cos_a = cos(param_vs_direction);                                                                         ""\n"
        "    float sin_a = sin(param_vs_direction);                                                                         ""\n"
        "    vec2 p;                                                                                                        ""\n"
        "    p.x = in_vs_vertexPosition.x * param_vs_symbolSize.x;                                                          ""\n"
        "    p.y = in_vs_vertexPosition.y * param_vs_symbolSize.y;                                                          ""\n"
        "    vec4 v;                                                                                                        ""\n"
        "    v.x = param_vs_symbolOffsetFromTarget.x * %TileSize3D%.0 + (p.x*cos_a - p.y*sin_a);                            ""\n"
        "    v.y = 0.0;                                                                                                     ""\n"
        "    v.z = param_vs_symbolOffsetFromTarget.y * %TileSize3D%.0 + (p.x*sin_a + p.y*cos_a);                            ""\n"
        "    v.w = 1.0;                                                                                                     ""\n"
        // Retrieve symbol position elevation
        "    if (abs(param_vs_elevation_scale.w) > 0.0)                                                                     ""\n"
        "    {                                                                                                              ""\n"
        "        float metersPerUnit = mix(param_vs_elevation_scale.x, param_vs_elevation_scale.y, param_vs_offsetInTile.y);""\n"
        "        float heightInMeters = param_vs_elevationInMeters * param_vs_elevation_scale.w;                            ""\n"
        "        v.y = (heightInMeters / metersPerUnit) * param_vs_elevation_scale.z;                                       ""\n"
        "    }                                                                                                              ""\n"
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
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.param.offsetInTile, "param_vs_offsetInTile", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.param.symbolOffsetFromTarget, "param_vs_symbolOffsetFromTarget", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.param.direction, "param_vs_direction", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.param.symbolSize, "param_vs_symbolSize", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.param.zDistanceFromCamera, "param_vs_zDistanceFromCamera", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.param.elevation_scale, "param_vs_elevation_scale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceRasterProgram.vs.param.elevationInMeters, "param_vs_elevationInMeters", GlslVariableType::Uniform);
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
    glVertexAttribPointer(
        *_onSurfaceRasterProgram.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, positionXY)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_onSurfaceRasterProgram.vs.in.vertexTexCoords);
    GL_CHECK_RESULT;
    glVertexAttribPointer(
        *_onSurfaceRasterProgram.vs.in.vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, textureUV)));
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
    AlphaChannelType &currentAlphaChannelType)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnSurfaceRasterMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (_lastUsedProgram != _onSurfaceRasterProgram.id)
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

        // Change depth test function to accept everything to avoid issues with terrain (regardless of elevation presence)
        glDepthFunc(GL_ALWAYS);
        GL_CHECK_RESULT;

        _lastUsedProgram = _onSurfaceRasterProgram.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) on-surface raster \"%3\"]")
        .arg(QString::asprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->toString())
        .arg(qPrintable(symbol->content)));

    // Parameters: per-tile data
    if (renderable->positionInWorld.y != 0.0f)
    {
        const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(
            currentState.zoomLevel,
            renderable->tileId.y,
            AtlasMapRenderer::TileSize3D);
        const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(
            currentState.zoomLevel,
            renderable->tileId.y + 1,
            AtlasMapRenderer::TileSize3D);
        glUniform4f(
            _onSurfaceRasterProgram.vs.param.elevation_scale,
            (float)upperMetersPerUnit,
            (float)lowerMetersPerUnit,
            currentState.elevationConfiguration.zScaleFactor,
            currentState.elevationConfiguration.dataScaleFactor);
        GL_CHECK_RESULT;
    }
    else
    {
        glUniform4f(_onSurfaceRasterProgram.vs.param.elevation_scale, 0.0f, 0.0f, 0.0f, 0.0f);
        GL_CHECK_RESULT;
    }

    glUniform2f(_onSurfaceRasterProgram.vs.param.offsetInTile, renderable->offsetInTileN.x, renderable->offsetInTileN.y);
    GL_CHECK_RESULT;

    // Set elevation
    glUniform1f(_onSurfaceRasterProgram.vs.param.elevationInMeters, renderable->elevationInMeters);
    GL_CHECK_RESULT;

    // Set symbol offset from target
    const auto currentOffset31 = Utilities::shortestVector31(renderable->target31, currentState.target31);
    const auto currentOffset = Utilities::convert31toFloat(currentOffset31, currentState.zoomLevel);
    glUniform2f(_onSurfaceRasterProgram.vs.param.symbolOffsetFromTarget,
        renderable->offsetFromTarget.x - currentOffset.x,
        renderable->offsetFromTarget.y - currentOffset.y);
    GL_CHECK_RESULT;

    // Set symbol size. Scale size to keep raster's original size on screen with 3D-terrain
    const float cameraHeight = internalState.distanceFromCameraToGround;
    const float sizeScale = cameraHeight > renderable->positionInWorld.y && !qFuzzyIsNull(cameraHeight)
        ? (cameraHeight - renderable->positionInWorld.y) / cameraHeight
        : 1.0f;
    glUniform2f(
        _onSurfaceRasterProgram.vs.param.symbolSize,
        gpuResource->width * internalState.pixelInWorldProjectionScale * sizeScale,
        gpuResource->height * internalState.pixelInWorldProjectionScale * sizeScale);
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

    // Apply symbols opacity factor to modulation color
    auto modulationColor = symbol->modulationColor;
    modulationColor.a *= currentState.symbolsOpacity;
    if (currentAlphaChannelType == AlphaChannelType::Premultiplied)
    {
        modulationColor.r *= currentState.symbolsOpacity;
        modulationColor.g *= currentState.symbolsOpacity;
        modulationColor.b *= currentState.symbolsOpacity;
    }

    // Set modulation color
    glUniform4f(_onSurfaceRasterProgram.fs.param.modulationColor,
        modulationColor.r,
        modulationColor.g,
        modulationColor.b,
        modulationColor.a);
    GL_CHECK_RESULT;

    // Apply settings from texture block to texture
    gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + 0);

    // Draw symbol actually
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    GL_CHECK_RESULT;

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnSurfaceRaster(bool gpuContextLost)
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
        "uniform vec4 param_vs_elevation_scale;                                                                             ""\n"
        "uniform mat4 param_vs_mPerspectiveProjectionView;                                                                  ""\n"
        "uniform mat4 param_vs_mModel;                                                                                      ""\n"
        "uniform float param_vs_zDistanceFromCamera;                                                                        ""\n"
        "uniform lowp vec4 param_vs_modulationColor;                                                                        ""\n"
        "uniform vec2 param_vs_tileId;                                                                                      ""\n"
        "uniform vec4 param_vs_lookupOffsetAndScale;                                                                        ""\n"
        "uniform vec4 param_vs_cameraPositionAndZfar;                                                                       ""\n"
        "uniform float param_vs_elevationInMeters;                                                                          ""\n"
        "uniform highp vec2 param_vs_offsetInTile;                                                                          ""\n"
        "uniform highp sampler2D param_vs_elevation_dataSampler;                                                            ""\n"
        "uniform highp vec4 param_vs_texCoordsOffsetAndScale;                                                               ""\n"
        "uniform highp vec4 param_vs_elevationLayerDataPlace;                                                               ""\n"
        "                                                                                                                   ""\n"
        "float interpolatedHeight(in vec2 inTexCoords)                                                                      ""\n"
        "{                                                                                                                  ""\n"
        "    vec2 heixelSize = param_vs_elevationLayerDataPlace.zw * 2.0;                                                   ""\n"
        "    vec2 texCoords = (inTexCoords - param_vs_elevationLayerDataPlace.zw) / heixelSize;                             ""\n"
        "    vec2 pixOffset = fract(texCoords);                                                                             ""\n"
        "    texCoords = floor(texCoords) * heixelSize + param_vs_elevationLayerDataPlace.zw;                               ""\n"
        "    vec2 minCoords = param_vs_elevationLayerDataPlace.xy - heixelSize;                                             ""\n"
        "    vec2 maxCoords = minCoords + heixelSize * (%HeixelsPerTileSide%.0 + 2.0);                                      ""\n"
        "    float blHeixel = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, clamp(texCoords, minCoords, maxCoords)).r;  ""\n"
        "    texCoords.x += heixelSize.x;                                                                                   ""\n"
        "    float brHeixel = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, clamp(texCoords, minCoords, maxCoords)).r;  ""\n"
        "    texCoords.y += heixelSize.y;                                                                                   ""\n"
        "    float trHeixel = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, clamp(texCoords, minCoords, maxCoords)).r;  ""\n"
        "    texCoords.x -= heixelSize.x;                                                                                   ""\n"
        "    float tlHeixel = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, clamp(texCoords, minCoords, maxCoords)).r;  ""\n"
        "    float avbPixel = mix(blHeixel, brHeixel, pixOffset.x);                                                         ""\n"
        "    float avtPixel = mix(tlHeixel, trHeixel, pixOffset.x);                                                         ""\n"
        "    return mix(avbPixel, avtPixel, pixOffset.y);                                                                   ""\n"
        "}                                                                                                                  ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        // Get vertex coordinates in world
        "    vec4 v;                                                                                                        ""\n"
        "    v.x = in_vs_vertexPosition.x;                                                                                  ""\n"
        "    v.y = 0.0;                                                                                                     ""\n"
        "    v.z = in_vs_vertexPosition.y;                                                                                  ""\n"
        "    v.w = 1.0;                                                                                                     ""\n"
        "    if (abs(param_vs_elevation_scale.w) > 0.0 && abs(param_vs_elevationInMeters) == 0.0)                           ""\n"
        "    {                                                                                                              ""\n"
        "        vec2 vertexTexCoords = v.xz * param_vs_lookupOffsetAndScale.z + param_vs_lookupOffsetAndScale.xy;          ""\n"
        "        v = param_vs_mModel * v;                                                                                   ""\n"
        "        vertexTexCoords -= param_vs_tileId;                                                                        ""\n"
        "        vec2 elevationTexCoords = vertexTexCoords * param_vs_texCoordsOffsetAndScale.zw;                           ""\n"
        "        elevationTexCoords += param_vs_texCoordsOffsetAndScale.xy;                                                 ""\n"
        "        float heightInMeters = interpolatedHeight(elevationTexCoords);                                             ""\n"
        "        heightInMeters *= param_vs_elevation_scale.w;                                                              ""\n"
        "        float metersPerUnit = mix(param_vs_elevation_scale.x, param_vs_elevation_scale.y, vertexTexCoords.t);      ""\n"
        "        v.y += (heightInMeters / metersPerUnit) * param_vs_elevation_scale.z;                                      ""\n"
        "        float dist = distance(param_vs_cameraPositionAndZfar.xyz, v.xyz);                                          ""\n"
        "        float extraZfar = 2.0 * dist / param_vs_cameraPositionAndZfar.w;                                           ""\n"
        "        float extraCam = dist / length(param_vs_cameraPositionAndZfar.xyz);                                        ""\n"
        "        v.y += min(extraZfar, extraCam) + 0.1;                                                                     ""\n"
        "        gl_Position = param_vs_mPerspectiveProjectionView * v;                                                     ""\n"
        "    }                                                                                                              ""\n"
        "    else if (abs(param_vs_elevationInMeters) > 0.0)                                                                ""\n"
        "    {                                                                                                              ""\n"
        "        v = param_vs_mModel * v;                                                                                   ""\n"
        "        float metersPerUnit = mix(param_vs_elevation_scale.x, param_vs_elevation_scale.y, param_vs_offsetInTile.y);""\n"
        "        float heightInMeters = param_vs_elevationInMeters * param_vs_elevation_scale.w;                            ""\n"
        "        v.y = (heightInMeters / metersPerUnit) * param_vs_elevation_scale.z;                                       ""\n"
        "        gl_Position = param_vs_mPerspectiveProjectionView * v;                                                     ""\n"
        "        gl_Position.z = param_vs_zDistanceFromCamera;                                                              ""\n"
        "    }                                                                                                              ""\n"
        "    else {                                                                                                         ""\n"
        "        v = param_vs_mModel * v;                                                                                   ""\n"
        "        gl_Position = param_vs_mPerspectiveProjectionView * v;                                                     ""\n"
        "        gl_Position.z = param_vs_zDistanceFromCamera;                                                              ""\n"
        "    }                                                                                                              ""\n"
        "                                                                                                                   ""\n"
        // Prepare color
        "    v2f_color.argb = in_vs_vertexColor.xyzw * param_vs_modulationColor.argb;                                       ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    preprocessedVertexShader.replace("%TileSize3D%", QString::number(AtlasMapRenderer::TileSize3D));
    preprocessedVertexShader.replace("%HeixelsPerTileSide%",
        QString::number(AtlasMapRenderer::HeixelsPerTileSide - 1));
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
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.mModel, "param_vs_mModel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.zDistanceFromCamera, "param_vs_zDistanceFromCamera", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.modulationColor, "param_vs_modulationColor", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.tileId, "param_vs_tileId", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.offsetInTile, "param_vs_offsetInTile", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.lookupOffsetAndScale, "param_vs_lookupOffsetAndScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.cameraPositionAndZfar, "param_vs_cameraPositionAndZfar", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.elevation_scale, "param_vs_elevation_scale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.elevation_dataSampler, "param_vs_elevation_dataSampler", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.elevationInMeters, "param_vs_elevationInMeters", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.texCoordsOffsetAndScale, "param_vs_texCoordsOffsetAndScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_onSurfaceVectorProgram.vs.param.elevationLayerDataPlace, "param_vs_elevationLayerDataPlace", GlslVariableType::Uniform);
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
    AlphaChannelType &currentAlphaChannelType)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnSurfaceVectorMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::MeshInGPU>(renderable->gpuResource);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Check if correct program is being used
    if (_lastUsedProgram != _onSurfaceVectorProgram.id)
    {
        GL_PUSH_GROUP_MARKER("use 'on-surface-vector' program");

        // Activate program
        glUseProgram(_onSurfaceVectorProgram.id);
        GL_CHECK_RESULT;

        // Set perspective projection-view matrix
        glUniformMatrix4fv(_onSurfaceVectorProgram.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
        GL_CHECK_RESULT;

        // Just in case un-use any possibly used VAO
        gpuAPI->unuseVAO();

        // Change depth test function to perform <= depth test (regardless of elevation presence)
        glDepthFunc(GL_LEQUAL);
        GL_CHECK_RESULT;

        _lastUsedProgram = _onSurfaceVectorProgram.id;

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
    position31.x = position31.x < 0 ? position31.x + INT32_MAX + 1 : position31.x;
    position31.y = position31.y < 0 ? position31.y + INT32_MAX + 1 : position31.y;
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
    const auto mScale = glm::scale(glm::vec3(scaleFactor, 1.0f, scaleFactor));

    // Set current offset
    const auto currentOffset31 = Utilities::shortestVector31(renderable->target31, currentState.target31);
    const auto currentOffset = Utilities::convert31toFloat(currentOffset31, currentState.zoomLevel);

    // Calculate position translate
    const auto mPosition = glm::translate(glm::vec3(
        (renderable->offsetFromTarget.x - currentOffset.x) * AtlasMapRenderer::TileSize3D,
        0.0f,
        (renderable->offsetFromTarget.y - currentOffset.y) * AtlasMapRenderer::TileSize3D));

    // Calculate direction
    const auto mDirection = glm::rotate(glm::radians(renderable->direction), glm::vec3(0.0f, -1.0f, 0.0f));

    // Calculate and set model matrix (scale -> direction -> position)
    const auto mModel = mPosition * mDirection * mScale;
    glUniformMatrix4fv(_onSurfaceVectorProgram.vs.param.mModel, 1, GL_FALSE, glm::value_ptr(mModel));
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

    // If symbol has no tiled parts - render it flat using single elevation value
    if (gpuResource->zoomLevel == InvalidZoomLevel || gpuResource->partSizes == nullptr)
    {
        // Parameters: per-tile data
        if (renderable->positionInWorld.y != 0.0f)
        {
            const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(
                currentState.zoomLevel,
                renderable->tileId.y,
                AtlasMapRenderer::TileSize3D);
            const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(
                currentState.zoomLevel,
                renderable->tileId.y + 1,
                AtlasMapRenderer::TileSize3D);
            glUniform4f(
                _onSurfaceVectorProgram.vs.param.elevation_scale,
                (float)upperMetersPerUnit,
                (float)lowerMetersPerUnit,
                currentState.elevationConfiguration.zScaleFactor,
                currentState.elevationConfiguration.dataScaleFactor);
            GL_CHECK_RESULT;
        }
        else
        {
            glUniform4f(_onSurfaceVectorProgram.vs.param.elevation_scale, 0.0f, 0.0f, 0.0f, 0.0f);
            GL_CHECK_RESULT;
        }

        glUniform2f(_onSurfaceVectorProgram.vs.param.offsetInTile, renderable->offsetInTileN.x, renderable->offsetInTileN.y);
        GL_CHECK_RESULT;

        // Set common elevation
        glUniform1f(_onSurfaceVectorProgram.vs.param.elevationInMeters, renderable->elevationInMeters);
        GL_CHECK_RESULT;

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
            default:
                assert(false);
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
    }
    else
    {
        // Reset common elevation
        glUniform1f(_onSurfaceVectorProgram.vs.param.elevationInMeters, 0.0f);
        GL_CHECK_RESULT;

        // Set symbol position and scale for heightmap lookup
        PointD startPosition = Utilities::convert31toDouble(position31, currentState.zoomLevel);
        const auto startTileId = TileId::fromXY(startPosition.x, startPosition.y);
        startPosition.x -= startTileId.x;
        startPosition.y -= startTileId.y;
        scaleFactor /= AtlasMapRenderer::TileSize3D;
        glUniform4f(_onSurfaceVectorProgram.vs.param.lookupOffsetAndScale,
            startPosition.x,
            startPosition.y,
            scaleFactor,
            scaleFactor);
        GL_CHECK_RESULT;

        // Set camera position and zFar distance to compute suitable elevation shift (against z-fighting)
        glUniform4f(_onSurfaceVectorProgram.vs.param.cameraPositionAndZfar,
            internalState.worldCameraPosition.x,
            internalState.worldCameraPosition.y,
            internalState.worldCameraPosition.z,
            internalState.zFar);
        GL_CHECK_RESULT;

        const int elevationDataSamplerIndex = 0;
        
        glActiveTexture(GL_TEXTURE0 + elevationDataSamplerIndex);
        GL_CHECK_RESULT;

        glUniform1i(_onSurfaceVectorProgram.vs.param.elevation_dataSampler, elevationDataSamplerIndex);
        GL_CHECK_RESULT;

        gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + elevationDataSamplerIndex, GPUAPI_OpenGL::SamplerType::ElevationDataTile);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        GL_CHECK_RESULT;

        // Activate vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->vertexBuffer->refInGPU)));
        GL_CHECK_RESULT;

        int32_t partOffset = 0;

        int maxMissingDataZoomShift = 0;
        int maxUnderZoomShift = 0;
        auto minZoom = ZoomLevel0;
        auto maxZoom = ZoomLevel0;
        if (currentState.elevationDataProvider)
        {
            maxMissingDataZoomShift = currentState.elevationDataProvider->getMaxMissingDataZoomShift();
            maxUnderZoomShift = currentState.elevationDataProvider->getMaxMissingDataUnderZoomShift();
            minZoom = currentState.elevationDataProvider->getMinZoom();
            maxZoom = currentState.elevationDataProvider->getMaxZoom();
        }
        double tileSize;
        for (const auto partSizes : *gpuResource->partSizes)
        {
            
            auto baseTileId = partSizes.first;
            const auto zoomLevel = gpuResource->zoomLevel;
            const auto tileIdN = Utilities::normalizeTileId(baseTileId, zoomLevel);
            int zoomShift = zoomLevel - currentState.zoomLevel;
            if (zoomShift > 0)
            {
                baseTileId.x >>= zoomShift;
                baseTileId.y >>= zoomShift;
            }
            glUniform2f(_onSurfaceVectorProgram.vs.param.tileId,
                            static_cast<float>(baseTileId.x - startTileId.x),
                            static_cast<float>(baseTileId.y - startTileId.y));
            GL_CHECK_RESULT;
            const auto baseTileIdN = Utilities::normalizeTileId(baseTileId, currentState.zoomLevel);
            std::shared_ptr<const GPUAPI::ResourceInGPU> elevationResource;
            if (zoomShift >= 0)
            {
                tileSize = static_cast<double>(AtlasMapRenderer::TileSize3D);
                if (elevationResource = captureElevationDataResource(baseTileIdN, currentState.zoomLevel))
                {
                    configureElevationData(elevationResource, _onSurfaceVectorProgram,
                        baseTileIdN,
                        currentState.zoomLevel,
                        PointF(0.0f, 0.0f),
                        PointF(1.0f, 1.0f),
                        tileSize,
                        elevationDataSamplerIndex);
                }
            }
            if (!elevationResource)
            {
                for (int absZoomShift = 1; absZoomShift <= maxMissingDataZoomShift; absZoomShift++)
                {
                    const auto underscaledZoom = static_cast<int>(currentState.zoomLevel) + absZoomShift;
                    if (underscaledZoom >= minZoom && underscaledZoom <= maxZoom && absZoomShift <= maxUnderZoomShift &&
                        currentState.zoomLevel >= minZoom && zoomLevel >= underscaledZoom)
                    {
                        auto tileId = tileIdN;
                        zoomShift = zoomLevel - underscaledZoom;
                        tileId.x >>= zoomShift;
                        tileId.y >>= zoomShift;
                        const auto underscaledZoomLevel = static_cast<ZoomLevel>(underscaledZoom);
                        if (elevationResource = captureElevationDataResource(tileId, underscaledZoomLevel))
                        {
                            const auto subtilesPerSide = (1u << absZoomShift);
                            const PointF texCoordsScale(subtilesPerSide, subtilesPerSide);                            
                            const PointF texCoordsOffset((tileId.x >> absZoomShift << absZoomShift) - tileId.x,
                                (tileId.y >> absZoomShift << absZoomShift) - tileId.y);
                            tileSize = static_cast<double>(AtlasMapRenderer::TileSize3D) /
                                static_cast<double>(subtilesPerSide);
                            configureElevationData(elevationResource, _onSurfaceVectorProgram,
                                tileId,
                                underscaledZoomLevel,
                                texCoordsOffset,
                                texCoordsScale,
                                tileSize,
                                elevationDataSamplerIndex);
                            break;
                        }
                    }
                    const auto overscaledZoom = static_cast<int>(currentState.zoomLevel) - absZoomShift;
                    if (overscaledZoom >= minZoom && overscaledZoom <= maxZoom && zoomLevel >= overscaledZoom)
                    {
                        PointF texCoordsOffset;
                        PointF texCoordsScale;
                        const auto overscaledTileIdN = Utilities::getTileIdOverscaledByZoomShift(
                            baseTileIdN,
                            absZoomShift,
                            &texCoordsOffset,
                            &texCoordsScale);
                        const auto overscaledZoomLevel = static_cast<ZoomLevel>(overscaledZoom);
                        if (elevationResource = captureElevationDataResource(
                            overscaledTileIdN,
                            overscaledZoomLevel))
                        {
                            tileSize = AtlasMapRenderer::TileSize3D *
                                static_cast<double>(1ull << currentState.zoomLevel - overscaledZoomLevel);
                            configureElevationData(elevationResource, _onSurfaceVectorProgram,
                                overscaledTileIdN,
                                overscaledZoomLevel,
                                texCoordsOffset,
                                texCoordsScale,
                                tileSize,
                                elevationDataSamplerIndex);
                            break;
                        }
                    }
                }
            }
            if (!elevationResource)
            {
                glUniform4f(_onSurfaceVectorProgram.vs.param.elevation_scale, 0.0f, 0.0f, 0.0f, 0.0f);
                GL_CHECK_RESULT;
            }

            glEnableVertexAttribArray(*_onSurfaceVectorProgram.vs.in.vertexPosition);
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_onSurfaceVectorProgram.vs.in.vertexPosition,
                2, GL_FLOAT, GL_FALSE,
                sizeof(VectorMapSymbol::Vertex),
                reinterpret_cast<GLvoid*>(partOffset + offsetof(VectorMapSymbol::Vertex, positionXY)));
            GL_CHECK_RESULT;

            glEnableVertexAttribArray(*_onSurfaceVectorProgram.vs.in.vertexColor);
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_onSurfaceVectorProgram.vs.in.vertexColor,
                4, GL_FLOAT, GL_FALSE,
                sizeof(VectorMapSymbol::Vertex),
                reinterpret_cast<GLvoid*>(partOffset + offsetof(VectorMapSymbol::Vertex, color)));
            GL_CHECK_RESULT;

            // Draw symbol actually
            glDrawArrays(GL_TRIANGLES, 0, partSizes.second);
            GL_CHECK_RESULT;

            partOffset += partSizes.second * sizeof(VectorMapSymbol::Vertex);
        }

        glActiveTexture(GL_TEXTURE0 + elevationDataSamplerIndex);
        GL_CHECK_RESULT;

        // Unbind symbol texture from texture sampler
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_RESULT;

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        GL_CHECK_RESULT;
    }

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnSurfaceVector(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

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

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeVisibilityCheck()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glBindTexture);
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
        "INPUT lowp float in_vs_vertexPosition;                                                                             ""\n"
        // Parameters: common data
        "uniform vec3 param_vs_firstPointPosition;                                                                          ""\n"
        "uniform vec3 param_vs_secondPointPosition;                                                                         ""\n"
        "uniform vec3 param_vs_thirdPointPosition;                                                                          ""\n"
        "uniform vec3 param_vs_fourthPointPosition;                                                                         ""\n"
        "uniform vec4 param_vs_cameraInWorld;                                                                               ""\n"
        "uniform mat4 param_vs_mModelViewProjection;                                                                        ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        // Get vertex coordinates in world
        "    vec3 v = in_vs_vertexPosition < 2.5 ? param_vs_thirdPointPosition : param_vs_fourthPointPosition;              ""\n"
        "    v = in_vs_vertexPosition < 1.5 ? param_vs_secondPointPosition : v;                                             ""\n"
        "    v = in_vs_vertexPosition < 0.5 ? param_vs_firstPointPosition : v;                                              ""\n"
        "    v.y += 0.2;                                                                                                    ""\n"
        "    v += normalize(param_vs_cameraInWorld.xyz - v) * 0.3;                                                          ""\n"
        "    gl_PointSize = param_vs_cameraInWorld.w;                                                                       ""\n"
        "    gl_Position = param_vs_mModelViewProjection * vec4(v.x, v.y, v.z, 1.0);                                        ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
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
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    FRAGMENT_COLOR_OUTPUT = vec4(0.0, 0.0, 0.0, 0.0);                                                              ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedFragmentShader = fragmentShader;
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
    _visibilityCheckProgram.id = gpuAPI->linkProgram(2, shaders, true, &variablesMap);
    if (!_visibilityCheckProgram.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererSymbolsStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_visibilityCheckProgram.id, variablesMap);
    ok = ok && lookup->lookupLocation(_visibilityCheckProgram.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_visibilityCheckProgram.vs.param.firstPointPosition, "param_vs_firstPointPosition", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_visibilityCheckProgram.vs.param.secondPointPosition, "param_vs_secondPointPosition", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_visibilityCheckProgram.vs.param.thirdPointPosition, "param_vs_thirdPointPosition", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_visibilityCheckProgram.vs.param.fourthPointPosition, "param_vs_fourthPointPosition", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_visibilityCheckProgram.vs.param.cameraInWorld, "param_vs_cameraInWorld", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_visibilityCheckProgram.vs.param.mModelViewProjection, "param_vs_mModelViewProjection", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_visibilityCheckProgram.id);
        GL_CHECK_RESULT;
        _visibilityCheckProgram.id.reset();

        return false;
    }

    // Unbind symbol texture from texture sampler
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_RESULT;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    GLfloat baseVertex[] = {0.0f, 1.0f, 2.0f, 3.0f};

    _visibilityCheckVAO = gpuAPI->allocateUninitializedVAO();

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_visibilityCheckVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _visibilityCheckVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, sizeof(baseVertex), baseVertex, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_visibilityCheckProgram.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_visibilityCheckProgram.vs.in.vertexPosition, 1, GL_FLOAT, GL_FALSE, 0, 0);
    GL_CHECK_RESULT;

    gpuAPI->initializeVAO(_visibilityCheckVAO);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    _queryMaxCount = INT32_MAX;
    _queryResults.reserve(4096);
    _querySizeFactor = 0.0f;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseVisibilityCheck(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_visibilityCheckVAO.isValid())
    {
        gpuAPI->releaseVAO(_visibilityCheckVAO, gpuContextLost);
        _visibilityCheckVAO.reset();
    }

    if (_visibilityCheckVBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_visibilityCheckVBO);
            GL_CHECK_RESULT;
        }
        _visibilityCheckVBO.reset();
    }

    if (_visibilityCheckProgram.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_visibilityCheckProgram.id);
            GL_CHECK_RESULT;
        }
        _visibilityCheckProgram = VisibilityCheckProgram();
    }

    return true;
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::clearTerrainVisibilityFiltering()
{
    if (_querySizeFactor == 0.0f)
    {
        _querySizeFactor =
            static_cast<float>(std::min(currentState.viewport.width(), currentState.viewport.height())) / 100.0f;
    }

    _nextQueryIndex = 0;
    _queryResultsCount = 0;
    _queryMapEven.clear();
    _queryMapOdd.clear();
}

int OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::startTerrainVisibilityFiltering(
    const PointF& pointOnScreen,
    const glm::vec3& firstPointInWorld,
    const glm::vec3& secondPointInWorld,
    const glm::vec3& thirdPointInWorld,
    const glm::vec3& fourthPointInWorld)
{
    if (!withTerrainFilter())
        return -1;

    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const auto filterQuery = pointOnScreen.x > -1.0f && pointOnScreen.y > -1.0f;

    PointF scaledPoint;
    int evenCode;
    int oddCode;

    float pointSize = 1.0f;

    if (filterQuery)
    {
        scaledPoint = pointOnScreen / _querySizeFactor;
        evenCode = static_cast<int>(scaledPoint.y) * 10000 + static_cast<int>(scaledPoint.x);
        oddCode = static_cast<int>(scaledPoint.y + 0.5f) * 10000 + static_cast<int>(scaledPoint.x + 0.5f);
        auto citQueryIndex = _queryMapEven.constFind(evenCode);
        if (citQueryIndex != _queryMapEven.cend())
            return *citQueryIndex;
        citQueryIndex = _queryMapOdd.constFind(oddCode);
        if (citQueryIndex != _queryMapOdd.cend())
            return *citQueryIndex;

        // The larger checking point of symbol requires the more distinctive visible terrain location
        const auto cameraVectorN = internalState.worldCameraPosition / internalState.distanceFromCameraToTarget;
        pointSize = _querySizeFactor * (90.0f - currentState.elevationAngle) / 
            glm::dot(internalState.worldCameraPosition - firstPointInWorld, cameraVectorN);
        if (pointSize < 1.0f)
            pointSize = 1.0f;
    }

    // Check if correct program is being used
    if (_lastUsedProgram != _visibilityCheckProgram.id)
    {
        GL_PUSH_GROUP_MARKER("use 'visibility-check' program");

        // Set VAO
        gpuAPI->useVAO(_visibilityCheckVAO);

        // Activate program
        glUseProgram(_visibilityCheckProgram.id);
        GL_CHECK_RESULT;

        // Set perspective projection-view matrix
        glUniformMatrix4fv(
            _visibilityCheckProgram.vs.param.mModelViewProjection,
            1,
            GL_FALSE,
            glm::value_ptr(internalState.mPerspectiveProjectionView));
        GL_CHECK_RESULT;

        // Change depth test function to perform > depth test
        glDepthFunc(GL_GREATER);
        GL_CHECK_RESULT;

        _lastUsedProgram = _visibilityCheckProgram.id;

        GL_POP_GROUP_MARKER;
    }

    // Set camera position
    glUniform4f(
        _visibilityCheckProgram.vs.param.cameraInWorld,
        internalState.worldCameraPosition.x,
        internalState.worldCameraPosition.y,
        internalState.worldCameraPosition.z,
        pointSize);

    glUniform3f(_visibilityCheckProgram.vs.param.firstPointPosition,
        firstPointInWorld.x,
        firstPointInWorld.y,
        firstPointInWorld.z);
    GL_CHECK_RESULT;

    glUniform3f(_visibilityCheckProgram.vs.param.secondPointPosition,
        secondPointInWorld.x,
        secondPointInWorld.y,
        secondPointInWorld.z);
    GL_CHECK_RESULT;

    glUniform3f(_visibilityCheckProgram.vs.param.thirdPointPosition,
        thirdPointInWorld.x,
        thirdPointInWorld.y,
        thirdPointInWorld.z);
    GL_CHECK_RESULT;

    glUniform3f(_visibilityCheckProgram.vs.param.fourthPointPosition,
        fourthPointInWorld.x,
        fourthPointInWorld.y,
        fourthPointInWorld.z);
    GL_CHECK_RESULT;
  
    auto queryIndex = _nextQueryIndex < _queryMaxCount ? gpuAPI->checkElementVisibility(_nextQueryIndex, pointSize) : -1;
    
    if (queryIndex < 0)
    {
        _queryMaxCount = _nextQueryIndex;
        queryIndex = _queryResultsCount;
        for (int i = 0; i < _queryMaxCount; i++)
        {
            const auto result = gpuAPI->elementIsVisible(i);
            if (queryIndex < _queryResults.size())
                _queryResults[queryIndex] = result;
            else
                _queryResults.push_back(result);
            queryIndex++;
        }
        _queryResultsCount = queryIndex;
        _nextQueryIndex = 0;
        gpuAPI->checkElementVisibility(_nextQueryIndex, pointSize);
    }
    else
        queryIndex += _queryResultsCount;

    _nextQueryIndex += 1;

    if (filterQuery)
    {
        _queryMapEven.insert(evenCode, queryIndex);
        _queryMapOdd.insert(oddCode, queryIndex);
    }

    return queryIndex;
}

bool OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::applyTerrainVisibilityFiltering(const int queryIndex,
    AtlasMapRenderer_Metrics::Metric_renderFrame* metric) const
{
    Stopwatch stopwatch(metric != nullptr);

    bool result;

    if (queryIndex < _queryResultsCount)
        result = _queryResults[queryIndex];
    else
    {
        const auto gpuAPI = getGPUAPI();
        result = gpuAPI->elementIsVisible(queryIndex - _queryResultsCount);
    }

    if (metric)
    {
        metric->elapsedTimeForApplyTerrainVisibilityFilteringCalls += stopwatch.elapsed();
        metric->applyTerrainVisibilityFilteringCalls++;
        if (result)
            metric->acceptedByTerrainVisibilityFiltering++;
        else
            metric->rejectedByTerrainVisibilityFiltering++;
    }

    return result;
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::configureElevationData(
    const std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU>& elevationDataResource,
    const OnSurfaceVectorProgram& program,
    const TileId tileIdN,
    const ZoomLevel zoomLevel,
    const PointF& texCoordsOffsetN,
    const PointF& texCoordsScaleN,
    const double tileSize,
    const int elevationDataSamplerIndex)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glBindTexture);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    // Per-tile elevation data configuration
    const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(
        zoomLevel,
        tileIdN.y,
        tileSize);
    const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(
        zoomLevel,
        tileIdN.y + 1,
        tileSize);
    glUniform4f(
        program.vs.param.elevation_scale,
        (float)upperMetersPerUnit,
        (float)lowerMetersPerUnit,
        currentState.elevationConfiguration.zScaleFactor,
        currentState.elevationConfiguration.dataScaleFactor);
    GL_CHECK_RESULT;

    if (gpuAPI->isSupported_vertexShaderTextureLookup)
    {
        glActiveTexture(GL_TEXTURE0 + elevationDataSamplerIndex);
        GL_CHECK_RESULT;

        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(elevationDataResource->refInGPU)));
        GL_CHECK_RESULT;

        gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + elevationDataSamplerIndex);

        if (elevationDataResource->type == GPUAPI::ResourceInGPU::Type::SlotOnAtlasTexture)
        {
            const auto tileOnAtlasTexture =
                std::static_pointer_cast<const GPUAPI::SlotOnAtlasTextureInGPU>(elevationDataResource);
            const auto& texture = tileOnAtlasTexture->atlasTexture;

            const auto rowIndex = tileOnAtlasTexture->slotIndex / texture->slotsPerSide;
            const auto colIndex = tileOnAtlasTexture->slotIndex - rowIndex * texture->slotsPerSide;

            // NOTE: Must be in sync with IMapElevationDataProvider::Data::getValue
            const PointF innerSize(texture->tileSizeN - 3.0f * texture->uTexelSizeN,
                texture->tileSizeN - 3.0f * texture->vTexelSizeN);
            const PointF texCoordsScale(innerSize.x * texCoordsScaleN.x, innerSize.y * texCoordsScaleN.y);
            const PointF texPlace(colIndex * texture->tileSizeN + texture->uHalfTexelSizeN + texture->uTexelSizeN,
                rowIndex * texture->tileSizeN + texture->vHalfTexelSizeN + texture->vTexelSizeN);
            const PointF texCoordsOffset(texPlace.x + innerSize.x * texCoordsOffsetN.x,
                texPlace.y + innerSize.y * texCoordsOffsetN.y);

            glUniform4f(program.vs.param.texCoordsOffsetAndScale,
                texCoordsOffset.x,
                texCoordsOffset.y,
                texCoordsScale.x,
                texCoordsScale.y);
            GL_CHECK_RESULT;
            glUniform4f(program.vs.param.elevationLayerDataPlace,
                texPlace.x,
                texPlace.y,
                texture->uHalfTexelSizeN,
                texture->vHalfTexelSizeN);
            GL_CHECK_RESULT;

        }
        else // if (elevationDataResource->type == GPUAPI::ResourceInGPU::Type::Texture)
        {
            const auto& texture = std::static_pointer_cast<const GPUAPI::TextureInGPU>(elevationDataResource);

            const PointF innerSize(
                1.0f - 3.0f * texture->uTexelSizeN,
                1.0f - 3.0f * texture->vTexelSizeN
            );
            const PointF texCoordsScale(innerSize.x * texCoordsScaleN.x, innerSize.y * texCoordsScaleN.y);
            const PointF texPlace(texture->uHalfTexelSizeN + texture->uTexelSizeN,
                texture->vHalfTexelSizeN + texture->vTexelSizeN);
            const PointF texCoordsOffset(texPlace.x + innerSize.x * texCoordsOffsetN.x,
                texPlace.y + innerSize.y * texCoordsOffsetN.y);

            glUniform4f(program.vs.param.texCoordsOffsetAndScale,
                texCoordsOffset.x,
                texCoordsOffset.y,
                texCoordsScale.x,
                texCoordsScale.y);
            GL_CHECK_RESULT;
            glUniform4f(program.vs.param.elevationLayerDataPlace,
                texPlace.x,
                texPlace.y,
                texture->uHalfTexelSizeN,
                texture->vHalfTexelSizeN);
            GL_CHECK_RESULT;
        }
    }
    else
    {
        glUniform4f(program.vs.param.elevation_scale, 0.0f, 0.0f, 0.0f, 0.0f);
        GL_CHECK_RESULT;
    }
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::cancelElevation(
    const OnSurfaceVectorProgram& program,
    const int elevationDataSamplerIndex,
    GLlocation& activeElevationVertexAttribArray)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glBindTexture);

    glUniform4f(program.vs.param.elevation_scale, 0.0f, 0.0f, 0.0f, 0.0f);
    GL_CHECK_RESULT;

    if (gpuAPI->isSupported_vertexShaderTextureLookup)
    {
        glActiveTexture(GL_TEXTURE0 + elevationDataSamplerIndex);
        GL_CHECK_RESULT;

        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_RESULT;
    }
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::reportCommonParameters(
    QJsonObject& jsonObject, const RenderableSymbol& renderableSymbol)
{
    const auto& mapSymbol =
        std::static_pointer_cast<const RasterMapSymbol>(renderableSymbol.mapSymbol);
    jsonObject.insert(QStringLiteral("class"),
        mapSymbol->contentClass == OsmAnd::MapSymbol::ContentClass::Caption ? QStringLiteral("caption")
            : (mapSymbol->contentClass == OsmAnd::MapSymbol::ContentClass::Icon ? QStringLiteral("icon")
                : QStringLiteral("unknown")));
    jsonObject.insert(QStringLiteral("content"), mapSymbol->content);
    if (mapSymbol->contentClass == OsmAnd::MapSymbol::ContentClass::Caption)
    {
        jsonObject.insert(QStringLiteral("language"),
            mapSymbol->languageId == OsmAnd::LanguageId::Native ? QStringLiteral("native")
            : (mapSymbol->languageId == OsmAnd::LanguageId::Localized ? QStringLiteral("localized")
                : QStringLiteral("invariant")));
    }
    jsonObject.insert(QStringLiteral("width"), mapSymbol->size.x);
    jsonObject.insert(QStringLiteral("height"), mapSymbol->size.y);
    jsonObject.insert(QStringLiteral("order"), mapSymbol->order);
    jsonObject.insert("opacity", renderableSymbol.opacityFactor);
    jsonObject.insert(QStringLiteral("zoom"), currentState.surfaceZoomLevel);
    if (const auto mapObjectSymbolsGroup =
        dynamic_cast<MapObjectsSymbolsProvider::MapObjectSymbolsGroup*>(mapSymbol->groupPtr))
    {
        if (mapObjectSymbolsGroup->mapObject->points31.size() > 1) {
            const auto& start = mapObjectSymbolsGroup->mapObject->points31[0];
            const auto& end = mapObjectSymbolsGroup->mapObject->points31[mapObjectSymbolsGroup->mapObject->points31.size() - 1];
            QJsonObject startJsonObject;
            startJsonObject.insert(QStringLiteral("lat"), Utilities::getLatitudeFromTile(ZoomLevel31, start.y));
            startJsonObject.insert(QStringLiteral("lon"), Utilities::getLongitudeFromTile(ZoomLevel31, start.x));
            QJsonObject endJsonObject;
            endJsonObject.insert(QStringLiteral("lat"), Utilities::getLatitudeFromTile(ZoomLevel31, end.y));
            endJsonObject.insert(QStringLiteral("lon"), Utilities::getLongitudeFromTile(ZoomLevel31, end.x));
            jsonObject.insert(QStringLiteral("startPoint"), startJsonObject);
            jsonObject.insert(QStringLiteral("endPoint"), endJsonObject);
        }
        if (const auto binaryMapObject =
            dynamic_cast<const BinaryMapObject*>(&(*(mapObjectSymbolsGroup->mapObject))))
        {
            jsonObject.insert(QStringLiteral("id"), static_cast<long long>(binaryMapObject->id.getOsmId() >> 1));
        }
    }
}
