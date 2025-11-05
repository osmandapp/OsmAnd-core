#include "AtlasMapRendererSymbolsStageModel3D_OpenGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "AtlasMapRenderer_OpenGL.h"

OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::AtlasMapRendererSymbolsStageModel3D_OpenGL(AtlasMapRendererSymbolsStage_OpenGL* const symbolsStage)
    : AtlasMapRendererSymbolsStageModel3D(symbolsStage)
{ 
}

OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::~AtlasMapRendererSymbolsStageModel3D_OpenGL()
{
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
    const auto symbolsStage = getSymbolsStage();
    const auto nextInitSymbolType =
        static_cast<AtlasMapRendererStage::InitSymbolType>(static_cast<int>(symbolsStage->_initSymbolType) + 1);
    symbolsStage->_initSymbolType = AtlasMapRendererStage::InitSymbolType::Incomplete;

    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glDisableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);
    GL_CHECK_PRESENT(glDeleteShader);
    GL_CHECK_PRESENT(glDeleteProgram);

    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    _program.id = 0;
    if (!_program.binaryCache.isEmpty())
        _program.id = gpuAPI->linkProgram(0, nullptr, _program.binaryCache, _program.cacheFormat, true, &variablesMap);
    if (!_program.id.isValid())
    {
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

        // Read precompiled shaders if available or otherwise compile them and put the binary code in cache if possible
        _program.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, symbolsStage->setupOptions.pathToOpenGLShadersCache, _program.cacheFormat);

        if (!_program.binaryCache.isEmpty())
        {
            _program.id = gpuAPI->linkProgram(
                0, nullptr, _program.binaryCache, _program.cacheFormat, true, &variablesMap);
        }
        if (_program.binaryCache.isEmpty() || !_program.id.isValid())
        {
            const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
            if (vsId == 0)
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile Model3D vertex shader");
                return false;
            }
            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile Model3D fragment shader");
                return false;
            }
            const GLuint shaders[] = { vsId, fsId };
            _program.id = gpuAPI->linkProgram(
                2, shaders, _program.binaryCache, _program.cacheFormat, true, &variablesMap);
            if (_program.id.isValid() && !_program.binaryCache.isEmpty())
            {
                gpuAPI->writeProgramBinary(
                    preprocessedVertexShader,
                    preprocessedFragmentShader,
                    symbolsStage->setupOptions.pathToOpenGLShadersCache,
                    _program.binaryCache,
                    _program.cacheFormat);
            }
        }
    }
    if (!_program.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link Model3D shader program");
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

        LogPrintf(LogSeverityLevel::Error,
            "Failed to find variable in Model3D shader program");
        return false;
    }

    symbolsStage->_initSymbolType = nextInitSymbolType;

    return true;
}

OsmAnd::MapRendererStage::StageResult OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::render(
    const std::shared_ptr<const RenderableModel3DSymbol>& renderable,
    AlphaChannelType& currentAlphaChannelType)
{
    const auto symbolsStage = getSymbolsStage();
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getSymbolsStage()->getInternalState();

    const auto& symbol = std::static_pointer_cast<const Model3DMapSymbol>(renderable->mapSymbol);
    const auto& gpuResource = std::static_pointer_cast<const GPUAPI::MeshInGPU>(renderable->gpuResource);
    const auto& symbolGroupPtr = symbol->groupPtr;

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

    // Calculate position of model in world coordinates
    const auto elevationInWorld = renderable->elevationInMeters
        / (currentState.flatEarth ? renderable->metersPerUnit : internalState.metersPerUnit);
    PointD angles;
    const auto positionInWorld = currentState.flatEarth
        ? Utilities::planeWorldCoordinates(renderable->position31,
            currentState.target31, currentState.zoomLevel, AtlasMapRenderer::TileSize3D, elevationInWorld)
        : Utilities::sphericalWorldCoordinates(renderable->position31,
            internalState.mGlobeRotationPrecise, internalState.globeRadius, elevationInWorld, &angles);
    auto rotateModel = glm::mat4(1.0f);
    if (!currentState.flatEarth)
    {
        const auto mRotationX = glm::rotate(static_cast<float>(angles.y), glm::vec3(-1.0f, 0.0f, 0.0f));
        const auto mRotationZ = glm::rotate(static_cast<float>(angles.x), glm::vec3(0.0f, 0.0f, -1.0f));
        rotateModel = glm::mat4(internalState.mGlobeRotationPrecise) * mRotationZ * mRotationX;
    }
    const auto placeModel = glm::translate(positionInWorld);
    const auto mModel = placeModel * rotateModel * renderable->mModel;

    // Model matrix
    glUniformMatrix4fv(
        _program.vs.param.mModel,
        1,
        GL_FALSE,
        glm::value_ptr(mModel));
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

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glDisableVertexAttribArray(*_program.vs.in.vertexColor);
    GL_CHECK_RESULT;
    glDisableVertexAttribArray(*_program.vs.in.vertexNormal);
    GL_CHECK_RESULT;
    glDisableVertexAttribArray(*_program.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    // Disable depth buffer offset for other symbols
    glDisable(GL_POLYGON_OFFSET_FILL);
    GL_CHECK_RESULT;

    GL_POP_GROUP_MARKER;

    return MapRendererStage::StageResult::Success;
}

bool OsmAnd::AtlasMapRendererSymbolsStageModel3D_OpenGL::release(const bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

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

    return true;
}