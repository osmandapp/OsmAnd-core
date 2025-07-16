#include "AtlasMapRendererSymbolsStageModel3D_OpenGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "AtlasMapRenderer_OpenGL.h"
#include "GPUAPI_OpenGL.h"

OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::AtlasMapRendererSymbolsStageModel3D_OpenGL(AtlasMapRendererSymbolsStage_OpenGL* const symbolsStage)
    : AtlasMapRendererSymbolsStageModel3D(symbolsStage)
{ 
    // Initialize multisample framebuffer
    _multisampleFramebuffer.framebuffer = 0;
    _multisampleFramebuffer.colorRenderbuffer = 0;
    _multisampleFramebuffer.depthRenderbuffer = 0;
    _multisampleFramebuffer.resolveTexture = 0;
    _multisampleFramebuffer.initialized = false;
    _multisampleFramebuffer.width = 0;
    _multisampleFramebuffer.height = 0;
    _multisampleFramebuffer.samples = 4; // Default to 4x MSAA
    
    // Initialize quad program
    _quadProgram.id = 0;
    _quadVAO = 0;
    _quadVBO = 0;
}

OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::~AtlasMapRendererSymbolsStageModel3D_OpenGL()
{
    releaseMultisampleFramebuffer();
}

OsmAnd::GPUAPI_OpenGL* OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::getGPUAPI() const
{
    return static_cast<GPUAPI_OpenGL*>(getSymbolsStage()->getGPUAPI());
}

OsmAnd::AtlasMapRenderer_OpenGL* OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::getRenderer() const
{
    return getSymbolsStage()->getRenderer();
}

OsmAnd::AtlasMapRendererSymbolsStage_OpenGL* OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::getSymbolsStage() const
{
    return static_cast<AtlasMapRendererSymbolsStage_OpenGL*>(symbolsStage);
}

bool OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::initialize()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);
    GL_CHECK_PRESENT(glDeleteShader);
    GL_CHECK_PRESENT(glDeleteProgram);

    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    _program.id = 0;
    if (!_program.binaryCache.isEmpty())
        _program.id = gpuAPI->linkProgram(0, nullptr, _program.binaryCache, _program.cacheFormat, true, &variablesMap);
    if (!_program.id.isValid())
    {
        // Compile vertex shader
        const QString vertexShader = QLatin1String(
            // Input data
            "INPUT vec3 in_vs_vertexPosition;                                                                                   ""\n"
            "INPUT vec3 in_vs_vertexNormal;                                                                                     ""\n"
            "INPUT vec4 in_vs_vertexColor;                                                                                      ""\n"
            "                                                                                                                   ""\n"
            // Output data to next shader stages
            "PARAM_OUTPUT vec3 v2f_pointPosition;                                                                               ""\n"
            "PARAM_OUTPUT vec3 v2f_pointNormal;                                                                                 ""\n"
            "PARAM_OUTPUT vec4 v2f_pointColor;                                                                                  ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform mat4 param_vs_mPerspectiveProjectionView;                                                                  ""\n"
            "uniform vec4 param_vs_resultScale;                                                                                 ""\n"
            "                                                                                                                   ""\n"
            // Parameters: per-model data
            "uniform mat4 param_vs_mModel;                                                                                      ""\n"
            "uniform vec4 param_vs_mainColor;                                                                                   ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    vec4 v = vec4(in_vs_vertexPosition, 1.0);                                                                      ""\n"
            "    v = param_vs_mModel * v;                                                                                       ""\n"
            "    v2f_pointPosition = v.xyz;                                                                                     ""\n"
            "    mat3 nModel = mat3(param_vs_mModel[0].xyz, param_vs_mModel[1].xyz, param_vs_mModel[2].xyz);                    ""\n"
            "    v2f_pointNormal = nModel * in_vs_vertexNormal;                                                                 ""\n"
            "    v2f_pointColor.argb = in_vs_vertexColor.x > -1.0 ? in_vs_vertexColor.xyzw : param_vs_mainColor.argb;           ""\n"
            "    v = param_vs_mPerspectiveProjectionView * v;                                                                   ""\n"
            "    gl_Position = v * param_vs_resultScale;                                                                        ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);
        const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
        if (vsId == 0)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to compile AtlasMapRendererSymbolsStageModel3D_OpenGL vertex shader");
            return false;
        }

        // Compile fragment shader
        const QString fragmentShader = QLatin1String(
            // Input data
            "PARAM_INPUT vec3 v2f_pointPosition;                                                                                ""\n"
            "PARAM_INPUT vec3 v2f_pointNormal;                                                                                  ""\n"
            "PARAM_INPUT vec4 v2f_pointColor;                                                                                   ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform vec3 param_fs_cameraPosition;                                                                              ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    vec3 v = normalize(param_fs_cameraPosition - v2f_pointPosition);                                               ""\n"
            "    vec3 n = normalize(v2f_pointNormal);                                                                           ""\n"
            "    vec3 l = normalize(vec3(1.0, -1.0, 1.0));                                                                      ""\n"
            "    vec3 r = reflect(l, n);                                                                                        ""\n"
            "    float h = pow((clamp(dot(r, v), 0.5, 1.0) - 0.5) * 2.0, 6.0) / 2.0;                                            ""\n"
            "    float d = clamp(dot(r, n), 0.0, 1.0) + 0.5;                                                                    ""\n"
            "    FRAGMENT_COLOR_OUTPUT = vec4(mix(v2f_pointColor.rgb * d, vec3(1.0), h), v2f_pointColor.a);                     ""\n"
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
                "Failed to compile AtlasMapRendererSymbolsStageModel3D_OpenGL fragment shader");
            return false;
        }
        const GLuint shaders[] = { vsId, fsId };
        _program.id = gpuAPI->linkProgram(2, shaders, _program.binaryCache, _program.cacheFormat, true, &variablesMap);
    }
    if (!_program.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererSymbolsStageModel3D_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_program.id, variablesMap);
    ok = ok && lookup->lookupLocation(_program.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.in.vertexNormal, "in_vs_vertexNormal", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.in.vertexColor, "in_vs_vertexColor", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.param.mainColor, "param_vs_mainColor", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.mModel, "param_vs_mModel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.fs.param.cameraPosition, "param_fs_cameraPosition", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_program.id);
        GL_CHECK_RESULT;
        _program.id.reset();

        return false;
    }

    // Initialize quad program for fullscreen compositing
    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > quadVariablesMap;
    _quadProgram.id = 0;
    if (!_quadProgram.binaryCache.isEmpty())
        _quadProgram.id = gpuAPI->linkProgram(0, nullptr, _quadProgram.binaryCache, _quadProgram.cacheFormat, true, &quadVariablesMap);
    if (!_quadProgram.id.isValid())
    {
        QString quadVertexShader = QLatin1String(

            "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
            "INPUT vec2 in_vs_vertexTexCoord;                                                                                   ""\n"
            "                                                                                                                   ""\n"
            "PARAM_OUTPUT vec2 v2f_texCoord;                                                                                    ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    v2f_texCoord = in_vs_vertexTexCoord;                                                                           ""\n"
            "    gl_Position = vec4(in_vs_vertexPosition, 0.0, 1.0);                                                            ""\n"
            "}                                                                                                                  ""\n");

        gpuAPI->preprocessVertexShader(quadVertexShader);
        gpuAPI->optimizeVertexShader(quadVertexShader);

        const auto quadVsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(quadVertexShader));
        if (quadVsId == 0)
        {
            LogPrintf(LogSeverityLevel::Error, "Failed to compile quad vertex shader");
            return false;
        }

        QString quadFragmentShader = QLatin1String(
            "PARAM_INPUT vec2 v2f_texCoord;                                                                                     ""\n"
            "                                                                                                                   ""\n"
            "uniform sampler2D param_fs_texture;                                                                                ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    vec4 color = SAMPLE_TEXTURE_2D(param_fs_texture, v2f_texCoord);                                                ""\n"
            "    if (color.rgb == vec3(0.0, 0.0, 0.0)) discard;                                                                 ""\n"
            "    FRAGMENT_COLOR_OUTPUT = color;                                                                                 ""\n"
            "}                                                                                                                  ""\n");

        gpuAPI->preprocessFragmentShader(quadFragmentShader);
        gpuAPI->optimizeFragmentShader(quadFragmentShader);

        const auto quadFsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(quadFragmentShader));
        if (quadFsId == 0)
        {
            glDeleteShader(quadVsId);
            GL_CHECK_RESULT;

            LogPrintf(LogSeverityLevel::Error, "Failed to compile quad fragment shader");
            return false;
        }

        const GLuint quadShaders[] = { quadVsId, quadFsId };
        _quadProgram.id = gpuAPI->linkProgram(2, quadShaders, _quadProgram.binaryCache, _quadProgram.cacheFormat, true, &quadVariablesMap);
    }

    if (!_quadProgram.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to link quad program");
        return false;
    }

    bool quadOk = true;
    const auto& quadLookup = gpuAPI->obtainVariablesLookupContext(_quadProgram.id, quadVariablesMap);
    quadOk = quadOk && quadLookup->lookupLocation(_quadProgram.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    quadOk = quadOk && quadLookup->lookupLocation(_quadProgram.vs.in.vertexTexCoord, "in_vs_vertexTexCoord", GlslVariableType::In);
    quadOk = quadOk && quadLookup->lookupLocation(_quadProgram.fs.param.texture, "param_fs_texture", GlslVariableType::Uniform);

    if (!quadOk)
    {
        glDeleteProgram(_quadProgram.id);
        GL_CHECK_RESULT;
        _quadProgram.id.reset();
        return false;
    }

    // Create fullscreen quad VAO/VBO
    glGenVertexArrays(1, &_quadVAO);
    GL_CHECK_RESULT;
    glGenBuffers(1, &_quadVBO);
    GL_CHECK_RESULT;

    glBindVertexArray(_quadVAO);
    GL_CHECK_RESULT;

    float quadVertices[] = {
        // positions   // texCoords
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
    };

    glBindBuffer(GL_ARRAY_BUFFER, _quadVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    glEnableVertexAttribArray(*_quadProgram.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_quadProgram.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    GL_CHECK_RESULT;

    glEnableVertexAttribArray(*_quadProgram.vs.in.vertexTexCoord);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_quadProgram.vs.in.vertexTexCoord, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    GL_CHECK_RESULT;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindVertexArray(0);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::render(
    const std::shared_ptr<const RenderableModel3DSymbol>& renderable,
    AlphaChannelType& currentAlphaChannelType)
{
    const auto& internalState = getSymbolsStage()->getInternalState();

    int viewportWidth = internalState.glmViewport[2];
    int viewportHeight = internalState.glmViewport[3];

    if (resizeMultisampleFramebuffer(viewportWidth, viewportHeight))
    {
        return renderToMultisampleFramebuffer(renderable, currentAlphaChannelType);
    }
    else
    {
        return renderModel3D(renderable, currentAlphaChannelType);
    }
}

bool OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::release(const bool gpuContextLost)
{
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_program.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_program.id);
            GL_CHECK_RESULT;
        }
        _program.id = 0;
    }

    if (_quadProgram.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_quadProgram.id);
            GL_CHECK_RESULT;
        }
        _quadProgram.id = 0;
    }

    if (!gpuContextLost)
    {
        if (_quadVAO != 0)
        {
            glDeleteVertexArrays(1, &_quadVAO);
            _quadVAO = 0;
        }
        if (_quadVBO != 0)
        {
            glDeleteBuffers(1, &_quadVBO);
            _quadVBO = 0;
        }
    }

    if (!gpuContextLost)
    {
        releaseMultisampleFramebuffer();
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::createMultisampleFramebuffer(int width, int height)
{
    // Check if multisampling is supported
    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    GL_CHECK_RESULT;
    
    if (maxSamples < _multisampleFramebuffer.samples)
    {
        _multisampleFramebuffer.samples = maxSamples;
        if (maxSamples == 0)
        {
            return false;
        }
    }

    glGenFramebuffers(1, &_multisampleFramebuffer.framebuffer);
    GL_CHECK_RESULT;
    glBindFramebuffer(GL_FRAMEBUFFER, _multisampleFramebuffer.framebuffer);
    GL_CHECK_RESULT;

    glGenRenderbuffers(1, &_multisampleFramebuffer.colorRenderbuffer);
    GL_CHECK_RESULT;
    glBindRenderbuffer(GL_RENDERBUFFER, _multisampleFramebuffer.colorRenderbuffer);
    GL_CHECK_RESULT;
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, _multisampleFramebuffer.samples, GL_RGBA8, width, height);
    GL_CHECK_RESULT;
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _multisampleFramebuffer.colorRenderbuffer);
    GL_CHECK_RESULT;

    glGenRenderbuffers(1, &_multisampleFramebuffer.depthRenderbuffer);
    GL_CHECK_RESULT;
    glBindRenderbuffer(GL_RENDERBUFFER, _multisampleFramebuffer.depthRenderbuffer);
    GL_CHECK_RESULT;
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, _multisampleFramebuffer.samples, GL_DEPTH_COMPONENT24, width, height);
    GL_CHECK_RESULT;
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _multisampleFramebuffer.depthRenderbuffer);
    GL_CHECK_RESULT;

    glGenTextures(1, &_multisampleFramebuffer.resolveTexture);
    GL_CHECK_RESULT;
    glBindTexture(GL_TEXTURE_2D, _multisampleFramebuffer.resolveTexture);
    GL_CHECK_RESULT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL_CHECK_RESULT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK_RESULT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GL_CHECK_RESULT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GL_CHECK_RESULT;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    GL_CHECK_RESULT;

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LogPrintf(LogSeverityLevel::Error, "Multisample framebuffer is not complete");
        releaseMultisampleFramebuffer();
        return false;
    }
    
    _multisampleFramebuffer.width = width;
    _multisampleFramebuffer.height = height;
    _multisampleFramebuffer.initialized = true;
    
    // Bind back to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK_RESULT;

    return true;
}

void OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::releaseMultisampleFramebuffer()
{
    if (_multisampleFramebuffer.framebuffer != 0)
    {
        glDeleteFramebuffers(1, &_multisampleFramebuffer.framebuffer);
        _multisampleFramebuffer.framebuffer = 0;
    }
    if (_multisampleFramebuffer.colorRenderbuffer != 0)
    {
        glDeleteRenderbuffers(1, &_multisampleFramebuffer.colorRenderbuffer);
        _multisampleFramebuffer.colorRenderbuffer = 0;
    }
    if (_multisampleFramebuffer.depthRenderbuffer != 0)
    {
        glDeleteRenderbuffers(1, &_multisampleFramebuffer.depthRenderbuffer);
        _multisampleFramebuffer.depthRenderbuffer = 0;
    }
    if (_multisampleFramebuffer.resolveTexture != 0)
    {
        glDeleteTextures(1, &_multisampleFramebuffer.resolveTexture);
        _multisampleFramebuffer.resolveTexture = 0;
    }
    
    _multisampleFramebuffer.initialized = false;
    _multisampleFramebuffer.width = 0;
    _multisampleFramebuffer.height = 0;
}

bool OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::resizeMultisampleFramebuffer(int width, int height)
{
    if (_multisampleFramebuffer.width == width && _multisampleFramebuffer.height == height)
    {
        return _multisampleFramebuffer.initialized;
    }

    releaseMultisampleFramebuffer();
    return createMultisampleFramebuffer(width, height);
}

bool OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::renderModel3D(
    const std::shared_ptr<const RenderableModel3DSymbol>& renderable,
    AlphaChannelType& currentAlphaChannelType)
{
    const auto symbolsStage = getSymbolsStage();
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getSymbolsStage()->getInternalState();

    const auto& symbol = std::static_pointer_cast<const Model3DMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::MeshInGPU>(renderable->gpuResource);

    if (symbolsStage->_lastUsedProgram != _program.id)
    {
        GL_PUSH_GROUP_MARKER("use 'model-3d' program");

        // Activate program
        glUseProgram(_program.id);
        GL_CHECK_RESULT;

        // Set perspective projection-view matrix
        glUniformMatrix4fv(_program.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
        GL_CHECK_RESULT;

        // Scale the result
        glUniform4f(_program.vs.param.resultScale,
            1.0f,
            currentState.flip ? -1.0f : 1.0f,
            1.0f,
            1.0f);
        GL_CHECK_RESULT;

        // Set camera position
        glUniform3f(_program.fs.param.cameraPosition,
            internalState.worldCameraPosition.x,
            internalState.worldCameraPosition.y,
            internalState.worldCameraPosition.z);
        GL_CHECK_RESULT;

        // Just in case un-use any possibly used VAO
        gpuAPI->unuseVAO();

        // Change depth test function to perform <= depth test (regardless of elevation presence)
        glDepthFunc(GL_LEQUAL);
        GL_CHECK_RESULT;

        glDepthMask(GL_TRUE);
        GL_CHECK_RESULT;

        symbolsStage->_lastUsedProgram = _program.id;

        GL_POP_GROUP_MARKER;
    }

    GL_PUSH_GROUP_MARKER(QString("[%1(%2) 3D model]")
        .arg(QString::asprintf("%p", symbol->groupPtr))
        .arg(symbol->group.lock()->toString()));


    // Enable depth buffer offset for all vector symbols (against z-fighting)
    glEnable(GL_POLYGON_OFFSET_FILL);
    GL_CHECK_RESULT;

    if (currentAlphaChannelType != AlphaChannelType::Straight)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_CHECK_RESULT;
        currentAlphaChannelType = AlphaChannelType::Straight;
    }

    // Model matrix
    glUniformMatrix4fv(
        _program.vs.param.mModel,
        1,
        GL_FALSE,
        glm::value_ptr(renderable->mModel));
    GL_CHECK_RESULT;

    // Set main color
    glUniform4f(_program.vs.param.mainColor,
        symbol->mainColor.r,
        symbol->mainColor.g,
        symbol->mainColor.b,
        symbol->mainColor.a);
    GL_CHECK_RESULT;

    // Activate vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->vertexBuffer->refInGPU)));
    GL_CHECK_RESULT;

    // Vertex position attribute
    glEnableVertexAttribArray(*_program.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_program.vs.in.vertexPosition,
        3, GL_FLOAT, GL_FALSE,
        sizeof(VectorMapSymbol::VertexWithNormals),
        reinterpret_cast<GLvoid*>(offsetof(VectorMapSymbol::VertexWithNormals, positionXYZ)));
    GL_CHECK_RESULT;

    // Vertex normal attribute
    glEnableVertexAttribArray(*_program.vs.in.vertexNormal);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_program.vs.in.vertexNormal,
        3, GL_FLOAT, GL_FALSE,
        sizeof(VectorMapSymbol::VertexWithNormals),
        reinterpret_cast<GLvoid*>(offsetof(VectorMapSymbol::VertexWithNormals, normalXYZ)));
    GL_CHECK_RESULT;

    // Vertex color attribute
    glEnableVertexAttribArray(*_program.vs.in.vertexColor);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_program.vs.in.vertexColor,
        4, GL_FLOAT, GL_FALSE,
        sizeof(VectorMapSymbol::VertexWithNormals),
        reinterpret_cast<GLvoid*>(offsetof(VectorMapSymbol::VertexWithNormals, color)));
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

    // Disable depth buffer offset for other symbols
    glDisable(GL_POLYGON_OFFSET_FILL);
    GL_CHECK_RESULT;

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::renderToMultisampleFramebuffer(
    const std::shared_ptr<const RenderableModel3DSymbol>& renderable,
    AlphaChannelType& currentAlphaChannelType)
{
    const auto& internalState = getSymbolsStage()->getInternalState();

    // Store current framebuffer
    GLint currentFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer);
    GL_CHECK_RESULT;

    // Bind multisample framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, _multisampleFramebuffer.framebuffer);
    GL_CHECK_RESULT;

    // Store current clear color
    GLfloat currentClearColor[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, currentClearColor);
    GL_CHECK_RESULT;

    // Clear the multisample framebuffer with transparent black
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    GL_CHECK_RESULT;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_RESULT;

    // Restore original clear color
    glClearColor(currentClearColor[0], currentClearColor[1], currentClearColor[2], currentClearColor[3]);
    GL_CHECK_RESULT;

    glViewport(0, 0, _multisampleFramebuffer.width, _multisampleFramebuffer.height);
    GL_CHECK_RESULT;


    if (!renderModel3D(renderable, currentAlphaChannelType))
    {
        return false;
    }

    resolveMultisampleFramebuffer();

    // Restore original framebuffer and viewport
    glBindFramebuffer(GL_FRAMEBUFFER, currentFramebuffer);
    GL_CHECK_RESULT;
    glViewport(internalState.glmViewport[0], internalState.glmViewport[1], internalState.glmViewport[2], internalState.glmViewport[3]);
    GL_CHECK_RESULT;

    compositeResolvedTexture();

    return true;
}

void OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::resolveMultisampleFramebuffer()
{
    // Create a temporary framebuffer for resolving
    GLuint resolveFramebuffer = 0;
    glGenFramebuffers(1, &resolveFramebuffer);
    GL_CHECK_RESULT;
    glBindFramebuffer(GL_FRAMEBUFFER, resolveFramebuffer);
    GL_CHECK_RESULT;

    // Attach the resolve texture to the temporary framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _multisampleFramebuffer.resolveTexture, 0);
    GL_CHECK_RESULT;

    // Bind the multisample framebuffer for reading
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _multisampleFramebuffer.framebuffer);
    GL_CHECK_RESULT;

    // Bind the resolve framebuffer for drawing
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFramebuffer);
    GL_CHECK_RESULT;

    // Blit the multisample buffer to the resolve texture
    glBlitFramebuffer(0, 0, _multisampleFramebuffer.width, _multisampleFramebuffer.height,
                      0, 0, _multisampleFramebuffer.width, _multisampleFramebuffer.height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    GL_CHECK_RESULT;

    // Clean up temporary framebuffer
    glDeleteFramebuffers(1, &resolveFramebuffer);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::compositeResolvedTexture()
{
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    GL_CHECK_RESULT;
    
    // Store current blend state
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint blendSrc, blendDst;
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrc);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDst);
    GL_CHECK_RESULT;
    
    // Disable depth testing for 2D overlay
    glDisable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;
    
    // Enable blending for proper compositing (same as regular rendering)
    glEnable(GL_BLEND);
    GL_CHECK_RESULT;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_RESULT;
    
    glUseProgram(_quadProgram.id);
    GL_CHECK_RESULT;

    glActiveTexture(GL_TEXTURE0);
    GL_CHECK_RESULT;
    glBindTexture(GL_TEXTURE_2D, _multisampleFramebuffer.resolveTexture);
    GL_CHECK_RESULT;
    glUniform1i(*_quadProgram.fs.param.texture, 0);
    GL_CHECK_RESULT;

    glBindVertexArray(_quadVAO);
    GL_CHECK_RESULT;
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    GL_CHECK_RESULT;
    glBindVertexArray(0);
    GL_CHECK_RESULT;
    
    // Restore state
    glUseProgram(currentProgram);
    GL_CHECK_RESULT;
    glEnable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;

    if (!blendEnabled)
    {
        glDisable(GL_BLEND);
    }
    else
    {
        glBlendFunc(blendSrc, blendDst);
    }

    GL_CHECK_RESULT;
}
