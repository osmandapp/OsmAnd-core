#include "AtlasMapRendererSkyStage_OpenGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "AtlasMapRenderer_OpenGL.h"

OsmAnd::AtlasMapRendererSkyStage_OpenGL::AtlasMapRendererSkyStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer_)
    : AtlasMapRendererSkyStage(renderer_)
    , AtlasMapRendererStageHelper_OpenGL(this)
{
}

OsmAnd::AtlasMapRendererSkyStage_OpenGL::~AtlasMapRendererSkyStage_OpenGL()
{
}

void OsmAnd::AtlasMapRendererSkyStage_OpenGL::initialize()
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
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mProjectionViewModel;                                                                        ""\n"
        "uniform vec2 param_vs_planeSize;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    vec4 v;                                                                                                        ""\n"
        "    v.xy = in_vs_vertexPosition * param_vs_planeSize;                                                              ""\n"
        "    v.z = 0.0;                                                                                                     ""\n"
        "    v.w = 1.0;                                                                                                     ""\n"
        "                                                                                                                   ""\n"
        "    gl_Position = param_vs_mProjectionViewModel * v;                                                               ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    assert(vsId != 0);

    // Compile fragment shader
    const QString fragmentShader = QLatin1String(
        // Parameters: common data
        "uniform lowp vec4 param_fs_skyColor;                                                                               ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    FRAGMENT_COLOR_OUTPUT = param_fs_skyColor;                                                                     ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
    gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);
    auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    assert(fsId != 0);

    // Link everything into program object
    const GLuint shaders[] = { vsId, fsId };
    _program.id = gpuAPI->linkProgram(2, shaders);
    assert(_program.id);

    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_program.id);
    lookup->lookupLocation(_program.vs.in.vertexPosition, "in_vs_vertexPosition", GLShaderVariableType::In);
    lookup->lookupLocation(_program.vs.param.mProjectionViewModel, "param_vs_mProjectionViewModel", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_program.vs.param.planeSize, "param_vs_planeSize", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_program.fs.param.skyColor, "param_fs_skyColor", GLShaderVariableType::Uniform);

    // Vertex data (x,y)
    float vertices[4][2] =
    {
        { -0.5f, -0.5f },
        { -0.5f,  0.5f },
        {  0.5f,  0.5f },
        {  0.5f, -0.5f }
    };
    const auto verticesCount = 4;

    // Index data
    GLushort indices[6] =
    {
        0, 1, 2,
        0, 2, 3
    };
    const auto indicesCount = 6;

    _skyplaneVAO = gpuAPI->allocateUninitializedVAO();

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_skyplaneVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _skyplaneVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float) * 2, vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_program.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_program.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float)* 2, nullptr);
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_skyplaneIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _skyplaneIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    // Unbind buffers
    gpuAPI->initializeVAO(_skyplaneVAO);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererSkyStage_OpenGL::render()
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    GL_PUSH_GROUP_MARKER(QLatin1String("sky"));

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glDrawElements);

    gpuAPI->useVAO(_skyplaneVAO);

    // Activate program
    glUseProgram(_program.id);
    GL_CHECK_RESULT;

    // Set projection*view*model matrix:
    const auto mFogTranslate = glm::translate(glm::vec3(0.0f, 0.0f, -internalState.correctedFogDistance));
    const auto mModel = internalState.mAzimuthInv * mFogTranslate;
    const auto mProjectionViewModel = internalState.mPerspectiveProjectionView * mModel;
    glUniformMatrix4fv(_program.vs.param.mProjectionViewModel, 1, GL_FALSE, glm::value_ptr(mProjectionViewModel));
    GL_CHECK_RESULT;

    // Set size of the skyplane
    glUniform2f(_program.vs.param.planeSize, internalState.skyplaneSize.x, internalState.skyplaneSize.y);
    GL_CHECK_RESULT;

    // Set sky parameters
    glUniform4f(_program.fs.param.skyColor, currentState.skyColor.r, currentState.skyColor.g, currentState.skyColor.b, 1.0f);
    GL_CHECK_RESULT;

    // Draw the skyplane actually
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    GL_CHECK_RESULT;

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    gpuAPI->unuseVAO();

    GL_POP_GROUP_MARKER;
}

void OsmAnd::AtlasMapRendererSkyStage_OpenGL::release()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);

    if (_skyplaneVAO)
    {
        gpuAPI->releaseVAO(_skyplaneVAO);
        _skyplaneVAO.reset();
    }

    if (_skyplaneIBO)
    {
        glDeleteBuffers(1, &_skyplaneIBO);
        GL_CHECK_RESULT;
        _skyplaneIBO.reset();
    }
    if (_skyplaneVBO)
    {
        glDeleteBuffers(1, &_skyplaneVBO);
        GL_CHECK_RESULT;
        _skyplaneVBO.reset();
    }

    if (_program.id)
    {
        glDeleteProgram(_program.id);
        GL_CHECK_RESULT;
        _program = Program();
    }
}
