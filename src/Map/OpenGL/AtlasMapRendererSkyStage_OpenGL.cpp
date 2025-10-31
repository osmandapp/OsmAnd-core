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
            "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
            "                                                                                                                   ""\n"
            // Output data to next shader stages
            "PARAM_OUTPUT vec2 v2f_position;                                                                                    ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform mat4 param_vs_mProjection;                                                                                 ""\n"
            "uniform vec4 param_vs_planeSize;                                                                                   ""\n"
            "uniform vec4 param_vs_resultScale;                                                                                 ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    vec4 v;                                                                                                        ""\n"
            "    v.xy = in_vs_vertexPosition * param_vs_planeSize.xy;                                                           ""\n"
            "    v.z = param_vs_planeSize.z;                                                                                    ""\n"
            "    v.w = 1.0;                                                                                                     ""\n"
            "                                                                                                                   ""\n"
            "    v2f_position = in_vs_vertexPosition;                                                                           ""\n"
            "    v = param_vs_mProjection * v;                                                                                  ""\n"
            "    gl_Position = v * param_vs_resultScale;                                                                        ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        const QString fragmentShader = QLatin1String(
            // Input data
            "PARAM_INPUT vec2 v2f_position;                                                                                     ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform vec4 param_fs_skySize;                                                                                     ""\n"
            "uniform vec4 param_fs_skyColor;                                                                                    ""\n"
            "uniform lowp vec4 param_fs_fogColor;                                                                               ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    lowp vec4 color;                                                                                               ""\n"
            "    float minR = param_fs_skySize.y + param_fs_skySize.z;                                                          ""\n"
            "    float midR = param_fs_skySize.y + param_fs_skySize.w;                                                          ""\n"
            "    float maxR = midR * midR / minR;                                                                               ""\n"
            "    float shift = v2f_position.y + param_fs_skySize.z;                                                             ""\n"
            "    float radius = shift > 0.0 ? mix(midR, minR, shift / minR) : mix(midR, maxR, -shift / maxR) ;                  ""\n"
            "    vec2 position = vec2(v2f_position.x * param_fs_skySize.x, v2f_position.y + radius - param_fs_skySize.y);       ""\n"
            "    float altitude = length(position) - radius;                                                                    ""\n"
            "    bool sky = altitude > 0.0;                                                                                     ""\n"
            "    altitude = (sky ? altitude : 0.0) * param_fs_skyColor.w / 10.0;                                                ""\n"
            "    float low = 1.0 / pow(1.1 + altitude, 4.0);                                                                    ""\n"
            "    float high = 1.0 - low;                                                                                        ""\n"
            "    color.r = (0.7647059 * low + high * exp(1.0 - altitude / 3.0) * 0.3101728) * param_fs_skyColor.r;              ""\n"
            "    color.g = (0.8039216 * low + high * exp(1.0 - altitude / 5.0) * 0.3390262) * param_fs_skyColor.g;              ""\n"
            "    color.b = (0.9019608 * low + high * exp(1.0 - altitude / 12.0) * 0.3678794) * param_fs_skyColor.b;             ""\n"
            "    color.a = 1.0;                                                                                                 ""\n"
            "    FRAGMENT_COLOR_OUTPUT = sky ? color : param_fs_fogColor;                                                       ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedFragmentShader = fragmentShader;
        QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        // Read precompiled shaders if available or otherwise compile them and put the binary code in cache if possible
        _program.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, setupOptions.pathToOpenGLShadersCache, _program.cacheFormat);

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
                    "Failed to compile AtlasMapRendererSkyStage_OpenGL vertex shader");
                return false;
            }
            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererSkyStage_OpenGL fragment shader");
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
                    setupOptions.pathToOpenGLShadersCache,
                    _program.binaryCache,
                    _program.cacheFormat);
            }
        }
    }
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
    ok = ok && lookup->lookupLocation(_program.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.fs.param.skySize, "param_fs_skySize", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.fs.param.skyColor, "param_fs_skyColor", GlslVariableType::Uniform);
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
        {  1.0f, -1.0f },
        {  1.0f, 1.0f },
        { -1.0f, 1.0f },
        { -1.0f, -1.0f }
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

    gpuAPI->initializeVAO(_skyplaneVAO);

    // Unbind buffers
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glDisableVertexAttribArray(*_program.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
    const auto skyPerspectiveProjection = glm::frustum(
        -internalState.projectionPlaneHalfWidth, internalState.projectionPlaneHalfWidth,
        -internalState.projectionPlaneHalfHeight, internalState.projectionPlaneHalfHeight,
        internalState.zNear, internalState.zFar * 2.0f);
    glUniformMatrix4fv(_program.vs.param.mProjection,
        1,
        GL_FALSE,
        glm::value_ptr(skyPerspectiveProjection));
    GL_CHECK_RESULT;

    // Set size of the skyplane
    glUniform4f(_program.vs.param.planeSize,
        internalState.skyplaneSize.x,
        internalState.skyplaneSize.y,
        -internalState.zFar,
        0.0f);
    GL_CHECK_RESULT;

    // Scale the result
    glUniform4f(_program.vs.param.resultScale,
        1.0f,
        currentState.flip ? -1.0f : 1.0f,
        1.0f,
        1.0f);
    GL_CHECK_RESULT;

    // Set parameters of the sky
    glUniform4f(_program.fs.param.skySize,
        internalState.aspectRatio,
        internalState.skyLine,
        internalState.skyTargetToCenter,
        internalState.skyBackToCenter);
    GL_CHECK_RESULT;

    // Apply sky color as a filter (when camera is near to surface)
    const static double spaceColorBarrier = 200000.0;
    const auto skyTintFactor = 1.0f - qBound(0.0f,
        static_cast<float>(internalState.distanceFromCameraToGroundInMeters / spaceColorBarrier), 1.0f);
    const auto skyHeight = internalState.skyHeightInKilometers / (1.0f - internalState.skyLine);
    glUniform4f(_program.fs.param.skyColor,
        1.0f - skyTintFactor * (1.0f - currentState.skyColor.r),
        1.0f - skyTintFactor * (1.0f - currentState.skyColor.g),
        1.0f - skyTintFactor * (1.0f - currentState.skyColor.b),
        skyHeight);
    GL_CHECK_RESULT;

    // Set the color of fog
    glUniform4f(_program.fs.param.fogColor,
        currentState.fogColor.r,
        currentState.fogColor.g,
        currentState.fogColor.b,
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
        _program.id = 0;
    }

    return true;
}
