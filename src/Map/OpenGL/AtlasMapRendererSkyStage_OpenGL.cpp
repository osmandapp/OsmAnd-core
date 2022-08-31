#include "AtlasMapRendererSkyStage_OpenGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "AtlasMapRenderer_OpenGL.h"
#include "AtlasMapRenderer_Metrics.h"

OsmAnd::AtlasMapRendererSkyStage_OpenGL::AtlasMapRendererSkyStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer_)
    : AtlasMapRendererSkyStage(renderer_)
    , AtlasMapRendererStageHelper_OpenGL(this)
{
}

OsmAnd::AtlasMapRendererSkyStage_OpenGL::~AtlasMapRendererSkyStage_OpenGL()
{
}

bool OsmAnd::AtlasMapRendererSkyStage_OpenGL::initialize()
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
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "PARAM_OUTPUT vec2 v2f_position;                                                                                     ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mProjection;                                                                                 ""\n"
        "uniform vec4 param_vs_planeSize;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    vec4 v;                                                                                                        ""\n"
        "    v.xy = in_vs_vertexPosition * param_vs_planeSize.xy;                                                           ""\n"
        "    v.z = param_vs_planeSize.z;                                                                                    ""\n"
        "    v.w = 1.0;                                                                                                     ""\n"
        "    v.y += param_vs_planeSize.w;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        "    v2f_position = in_vs_vertexPosition;                                                                           ""\n"
        "    v = param_vs_mProjection * v;                                                                                  ""\n"
        "    v.y--;                                                                                                         ""\n"
        "    gl_Position = v;                                                                                               ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    if (vsId == 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRendererSkyStage_OpenGL vertex shader");
        return false;
    }

    // Compile fragment shader
    const QString fragmentShader = QLatin1String(
        // Input data
        "PARAM_INPUT vec2 v2f_position;                                                                                     ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform vec4 param_fs_skySize;                                                                                     ""\n"
        "uniform vec4 param_fs_fogColor;                                                                                    ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    vec4 color;                                                                                                    ""\n"
        "    float position;                                                                                                ""\n"
        "    float skyHeight = param_fs_skySize.y / (1.0f - param_fs_skySize.z);                                            ""\n"
        "    bool sky = v2f_position.y > param_fs_skySize.z;                                                                ""\n"
        "    position = (v2f_position.y - (sky ? param_fs_skySize.z : 0.0f)) * skyHeight;                                   ""\n"
        "    float low = 1.0f / pow(1.1f + position, 4.0f);                                                                 ""\n"
        "    float high = 1.0f - low;                                                                                       ""\n"
        "    color.r = sky ? 0.7647059f * low + high * exp(1.0f - position / 3.0f) * 0.3101728f : param_fs_fogColor.r;      ""\n"
        "    color.g = sky ? 0.8039216f * low + high * exp(1.0f - position / 5.0f) * 0.3390262f : param_fs_fogColor.g;      ""\n"
        "    color.b = sky ? 0.9019608f * low + high * exp(1.0f - position / 12.0f) * 0.3678794f : param_fs_fogColor.b;     ""\n"
        "    color.a = 1.0f;                                                                                                ""\n"
        "    FRAGMENT_COLOR_OUTPUT = color;                                                                                 ""\n"
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
            "Failed to compile AtlasMapRendererSkyStage_OpenGL fragment shader");
        return false;
    }

    // Link everything into program object
    const GLuint shaders[] = { vsId, fsId };
    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    _program.id = gpuAPI->linkProgram(2, shaders, true, &variablesMap);
    if (!_program.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererSkyStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_program.id, variablesMap);
    ok = ok && lookup->lookupLocation(_program.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.param.mProjection, "param_vs_mProjection", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.planeSize, "param_vs_planeSize", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.fs.param.skySize, "param_fs_skySize", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.fs.param.fogColor, "param_fs_fogColor", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_program.id);
        GL_CHECK_RESULT;
        _program.id.reset();

        return false;
    }

    struct Vertex
    {
        // XY coordinates: Y up, X right
        float positionXY[2];

        // Color
        FColorARGB color;
    };

    // Vertex data (x,y)
    float vertices[4][2] =
    {
        {  1.0f, 0.0f },
        {  1.0f, 1.0f },
        { -1.0f, 1.0f },
        { -1.0f, 0.0f }
    };
    const auto verticesCount = 4;

    // Index data
    GLushort indices[6] =
    {
        0, 1, 2,
        2, 3, 0
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

    return true;
}

bool OsmAnd::AtlasMapRendererSkyStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* const metric_)
{
    const auto metric = dynamic_cast<AtlasMapRenderer_Metrics::Metric_renderFrame*>(metric_);

    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    GL_PUSH_GROUP_MARKER(QLatin1String("sky"));

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glDrawElements);

    gpuAPI->useVAO(_skyplaneVAO);

    // Activate program
    glUseProgram(_program.id);
    GL_CHECK_RESULT;

    // Set projection matrix:
    glUniformMatrix4fv(_program.vs.param.mProjection,
        1,
        GL_FALSE,
        glm::value_ptr(internalState.mPerspectiveProjection));
    GL_CHECK_RESULT;

    // Set size of the skyplane
    glUniform4f(_program.vs.param.planeSize,
        internalState.skyplaneSize.x,
        internalState.skyplaneSize.y,
        -internalState.zSkyplane,
        internalState.skyShift);
    GL_CHECK_RESULT;

    // Set parameters of the sky
    glUniform4f(_program.fs.param.skySize,
        0.0f, // TODO: put sky width (in kilometers) here for the spherified horizon
        internalState.skyHeightInKilometers,
        internalState.skyLine,
        0.0f);
    GL_CHECK_RESULT;

    // Set the color of fog
    glUniform4f(_program.fs.param.fogColor,
        currentState.fogConfiguration.color.r,
        currentState.fogConfiguration.color.g,
        currentState.fogConfiguration.color.b,
        1.0f);
    GL_CHECK_RESULT;

    // Draw the skyplane actually
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    GL_CHECK_RESULT;

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    gpuAPI->unuseVAO();

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRendererSkyStage_OpenGL::release(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);

    if (_skyplaneVAO.isValid())
    {
        gpuAPI->releaseVAO(_skyplaneVAO, gpuContextLost);
        _skyplaneVAO.reset();
    }

    if (_skyplaneIBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_skyplaneIBO);
            GL_CHECK_RESULT;
        }
        _skyplaneIBO.reset();
    }
    if (_skyplaneVBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_skyplaneVBO);
            GL_CHECK_RESULT;
        }
        _skyplaneVBO.reset();
    }

    if (_program.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_program.id);
            GL_CHECK_RESULT;
        }
        _program = Program();
    }

    return true;
}
