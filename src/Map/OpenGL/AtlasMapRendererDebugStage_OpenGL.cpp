#include "AtlasMapRendererDebugStage_OpenGL.h"

#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "ignore_warnings_on_external_includes.h"
#include <SkColor.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRenderer_OpenGL.h"
#include "AtlasMapRenderer_Metrics.h"
#include "Stopwatch.h"

OsmAnd::AtlasMapRendererDebugStage_OpenGL::AtlasMapRendererDebugStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer_)
    : AtlasMapRendererDebugStage(renderer_)
    , AtlasMapRendererStageHelper_OpenGL(this)
{
}

OsmAnd::AtlasMapRendererDebugStage_OpenGL::~AtlasMapRendererDebugStage_OpenGL()
{
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::initialize()
{
    bool ok = true;
    ok = ok && initializePoints2D();
    ok = ok && initializeRects2D();
    ok = ok && initializeLines2D();
    ok = ok && initializeLines3D();
    ok = ok && initializeQuads3D();
    return ok;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* const metric_)
{
    const auto metric = dynamic_cast<AtlasMapRenderer_Metrics::Metric_renderFrame*>(metric_);

    bool ok = true;

    GL_PUSH_GROUP_MARKER(QLatin1String("debug"));

    Stopwatch points2dStopwatch(metric != nullptr);
    ok = ok && renderPoints2D();
    if (metric)
        metric->elapsedTimeForDebugPoints2D = points2dStopwatch.elapsed();

    Stopwatch rects2dStopwatch(metric != nullptr);
    ok = ok && renderRects2D();
    if (metric)
        metric->elapsedTimeForDebugRects2D = rects2dStopwatch.elapsed();

    Stopwatch lines2dStopwatch(metric != nullptr);
    ok = ok && renderLines2D();
    if (metric)
        metric->elapsedTimeForDebugLines2D = lines2dStopwatch.elapsed();

    Stopwatch lines3dStopwatch(metric != nullptr);
    ok = ok && renderLines3D();
    if (metric)
        metric->elapsedTimeForDebugLines3D = lines3dStopwatch.elapsed();

    Stopwatch quads3dStopwatch(metric != nullptr);
    ok = ok && renderQuads3D();
    if (metric)
        metric->elapsedTimeForDebugQuad3D = quads3dStopwatch.elapsed();

    GL_POP_GROUP_MARKER;

    return ok;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::release(bool gpuContextLost)
{
    bool ok = true;
    ok = ok && releasePoints2D(gpuContextLost);
    ok = ok && releaseRects2D(gpuContextLost);
    ok = ok && releaseLines2D(gpuContextLost);
    ok = ok && releaseLines3D(gpuContextLost);
    ok = ok && releaseQuads3D(gpuContextLost);
    return ok;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::initializePoints2D()
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
    _programPoint2D.id = 0;
    if (!_programPoint2D.binaryCache.isEmpty())
    {
        _programPoint2D.id = gpuAPI->linkProgram(0, nullptr,
            _programPoint2D.binaryCache, _programPoint2D.cacheFormat, true, &variablesMap);
    }
    if (!_programPoint2D.id.isValid())
    {
        const QString vertexShader = QLatin1String(
            // Input data
            "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform mat4 param_vs_mProjectionViewModel;                                                                        ""\n"
            "uniform vec4 param_vs_resultScale;                                                                                 ""\n"
            "uniform vec3 param_vs_point;                                                                                       ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    vec2 v = param_vs_point.xy + in_vs_vertexPosition * param_vs_point.z;                                          ""\n"
            "    vec4 vertex = param_vs_mProjectionViewModel * vec4(v.x, v.y, -1.0, 1.0);                                       ""\n"
            "    gl_Position = vertex * param_vs_resultScale;                                                                   ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        const QString fragmentShader = QLatin1String(
            // Parameters: common data
            "uniform lowp vec4 param_fs_color;                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    FRAGMENT_COLOR_OUTPUT = param_fs_color;                                                                        ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedFragmentShader = fragmentShader;
        QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        // Read precompiled shaders if available or otherwise compile them and put the binary code in cache if possible
        _programPoint2D.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, setupOptions.pathToOpenGLShadersCache, _programPoint2D.cacheFormat);

        if (!_programPoint2D.binaryCache.isEmpty())
        {
            _programPoint2D.id = gpuAPI->linkProgram(0, nullptr,
                _programPoint2D.binaryCache, _programPoint2D.cacheFormat, true, &variablesMap);
        }
        if (_programPoint2D.binaryCache.isEmpty() || !_programPoint2D.id.isValid())
        {
            const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
            if (vsId == 0)
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererDebugStage_OpenGL vertex shader");
                return false;
            }
            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererDebugStage_OpenGL fragment shader");
                return false;
            }
            GLuint shaders[] = { vsId, fsId };
            _programPoint2D.id = gpuAPI->linkProgram(2, shaders,
                _programPoint2D.binaryCache, _programPoint2D.cacheFormat, true, &variablesMap);
            if (_programPoint2D.id.isValid() && !_programPoint2D.binaryCache.isEmpty())
            {
                gpuAPI->writeProgramBinary(
                    preprocessedVertexShader,
                    preprocessedFragmentShader,
                    setupOptions.pathToOpenGLShadersCache,
                    _programPoint2D.binaryCache,
                    _programPoint2D.cacheFormat);
            }
        }
    }
    if (!_programPoint2D.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererDebugStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_programPoint2D.id, variablesMap);
    ok = ok && lookup->lookupLocation(_programPoint2D.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_programPoint2D.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programPoint2D.vs.param.point, "param_vs_point", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programPoint2D.fs.param.color, "param_fs_color", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_programPoint2D.id);
        GL_CHECK_RESULT;
        _programPoint2D.id.reset();

        return false;
    }

    // Vertex data (x,y)
    float vertices[9][2] =
    {
        {  0.0f,  0.0f },
        { -0.35f, -0.35f },
        { -0.5f,  0.0f },
        { -0.35f,  0.35f },
        {  0.0f,  0.5f },
        {  0.35f,  0.35f },
        {  0.5f,  0.0f },
        {  0.35f, -0.35f },
        {  0.0f, -0.5f }
    };
    const auto verticesCount = 9;

    // Index data
    GLushort indices[24] =
    {
        0, 1, 2,
        0, 2, 3,
        0, 3, 4,
        0, 4, 5,
        0, 5, 6,
        0, 6, 7,
        0, 7, 8,
        0, 8, 1
    };
    const auto indicesCount = 24;

    _vaoPoint2D = gpuAPI->allocateUninitializedVAO();

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_vboPoint2D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _vboPoint2D);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float)* 2, vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_programPoint2D.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_programPoint2D.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float)* 2, nullptr);
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_iboPoint2D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboPoint2D);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->initializeVAO(_vaoPoint2D);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::renderPoints2D()
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glDrawElements);

    gpuAPI->useVAO(_vaoPoint2D);

    // Activate program
    glUseProgram(_programPoint2D.id);
    GL_CHECK_RESULT;

    // Set projection*view*model matrix:
    glUniformMatrix4fv(_programPoint2D.vs.param.mProjectionViewModel,
        1, GL_FALSE, glm::value_ptr(internalState.mOrthographicProjection));
    GL_CHECK_RESULT;

    // Scale the result
    glUniform4f(_programPoint2D.vs.param.resultScale,
        1.0f,
        currentState.flip ? -1.0f : 1.0f,
        1.0f,
        1.0f);
    GL_CHECK_RESULT;

    const auto pointSize =
        static_cast<float>(std::min(currentState.viewport.width(), currentState.viewport.height())) / 50.0f;

    for(const auto& primitive : constOf(_points2D))
    {
        const auto& point = std::get<0>(primitive);
        const auto& color = std::get<1>(primitive);

        // Set point coordinates and size
        glUniform3f(_programPoint2D.vs.param.point, point.x, point.y, pointSize);
        GL_CHECK_RESULT;

        // Set point color
        glUniform4f(_programPoint2D.fs.param.color,
            SkColorGetR(color) / 255.0f,
            SkColorGetG(color) / 255.0f,
            SkColorGetB(color) / 255.0f,
            SkColorGetA(color) / 255.0f);
        GL_CHECK_RESULT;

        // Draw the point actually
        glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, nullptr);
        GL_CHECK_RESULT;
    }

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    gpuAPI->unuseVAO();

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::releasePoints2D(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_vaoPoint2D.isValid())
    {
        gpuAPI->releaseVAO(_vaoPoint2D, gpuContextLost);
        _vaoPoint2D.reset();
    }

    if (_iboPoint2D.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_iboPoint2D);
            GL_CHECK_RESULT;
        }
        _iboPoint2D.reset();
    }
    if (_vboPoint2D.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_vboPoint2D);
            GL_CHECK_RESULT;
        }
        _vboPoint2D.reset();
    }

    if (_programPoint2D.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_programPoint2D.id);
            GL_CHECK_RESULT;
        }
        _programPoint2D.id = 0;
    }

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::initializeRects2D()
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
    _programRect2D.id = 0;
    if (!_programRect2D.binaryCache.isEmpty())
    {
        _programRect2D.id = gpuAPI->linkProgram(0, nullptr,
            _programRect2D.binaryCache, _programRect2D.cacheFormat, true, &variablesMap);
    }
    if (!_programRect2D.id.isValid())
    {
        const QString vertexShader = QLatin1String(
            // Input data
            "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform mat4 param_vs_mProjectionViewModel;                                                                        ""\n"
            "uniform vec4 param_vs_resultScale;                                                                                 ""\n"
            "uniform vec4 param_vs_rect;                                                                                        ""\n"
            "uniform float param_vs_angle;                                                                                      ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    vec2 rectCenter = param_vs_rect.yx;                                                                            ""\n"
            "    vec2 p = in_vs_vertexPosition*param_vs_rect.wz + rectCenter;                                                   ""\n"
            "    vec4 v;                                                                                                        ""\n"
            "    float cos_a = cos(param_vs_angle);                                                                             ""\n"
            "    float sin_a = sin(param_vs_angle);                                                                             ""\n"
            "    p -= rectCenter;                                                                                               ""\n"
            "    v.x = rectCenter.x + p.x*cos_a - p.y*sin_a;                                                                    ""\n"
            "    v.y = rectCenter.y + p.x*sin_a + p.y*cos_a;                                                                    ""\n"
            "    v.z = -1.0;                                                                                                    ""\n"
            "    v.w = 1.0;                                                                                                     ""\n"
            "                                                                                                                   ""\n"
            "    v = param_vs_mProjectionViewModel * v;                                                                         ""\n"
            "    gl_Position = v * param_vs_resultScale;                                                                        ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        const QString fragmentShader = QLatin1String(
            // Parameters: common data
            "uniform lowp vec4 param_fs_color;                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    FRAGMENT_COLOR_OUTPUT = param_fs_color;                                                                        ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedFragmentShader = fragmentShader;
        QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        // Read precompiled shaders if available or otherwise compile them and put the binary code in cache if possible
        _programRect2D.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, setupOptions.pathToOpenGLShadersCache, _programRect2D.cacheFormat);

        if (!_programRect2D.binaryCache.isEmpty())
        {
            _programRect2D.id = gpuAPI->linkProgram(0, nullptr,
                _programRect2D.binaryCache, _programRect2D.cacheFormat, true, &variablesMap);
        }
        if (_programRect2D.binaryCache.isEmpty() || !_programRect2D.id.isValid())
        {
            const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
            if (vsId == 0)
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererDebugStage_OpenGL vertex shader");
                return false;
            }
            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererDebugStage_OpenGL fragment shader");
                return false;
            }
            GLuint shaders[] = { vsId, fsId };
            _programRect2D.id = gpuAPI->linkProgram(2, shaders,
                _programRect2D.binaryCache, _programRect2D.cacheFormat, true, &variablesMap);
            if (_programRect2D.id.isValid() && !_programRect2D.binaryCache.isEmpty())
            {
                gpuAPI->writeProgramBinary(
                    preprocessedVertexShader,
                    preprocessedFragmentShader,
                    setupOptions.pathToOpenGLShadersCache,
                    _programRect2D.binaryCache,
                    _programRect2D.cacheFormat);
            }
        }
    }
    if (!_programRect2D.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererDebugStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_programRect2D.id, variablesMap);
    ok = ok && lookup->lookupLocation(_programRect2D.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_programRect2D.vs.param.mProjectionViewModel, "param_vs_mProjectionViewModel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programRect2D.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programRect2D.vs.param.rect, "param_vs_rect", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programRect2D.vs.param.angle, "param_vs_angle", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programRect2D.fs.param.color, "param_fs_color", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_programRect2D.id);
        GL_CHECK_RESULT;
        _programRect2D.id.reset();

        return false;
    }

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

    _vaoRect2D = gpuAPI->allocateUninitializedVAO();

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_vboRect2D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _vboRect2D);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float)* 2, vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_programRect2D.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_programRect2D.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float)* 2, nullptr);
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_iboRect2D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboRect2D);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->initializeVAO(_vaoRect2D);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::renderRects2D()
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glDrawElements);

    gpuAPI->useVAO(_vaoRect2D);

    // Activate program
    glUseProgram(_programRect2D.id);
    GL_CHECK_RESULT;

    // Set projection*view*model matrix:
    glUniformMatrix4fv(_programRect2D.vs.param.mProjectionViewModel, 1, GL_FALSE, glm::value_ptr(internalState.mOrthographicProjection));
    GL_CHECK_RESULT;

    // Scale the result
    glUniform4f(_programRect2D.vs.param.resultScale,
        1.0f,
        currentState.flip ? -1.0f : 1.0f,
        1.0f,
        1.0f);
    GL_CHECK_RESULT;

    for(const auto& primitive : constOf(_rects2D))
    {
        const auto& rect = std::get<0>(primitive);
        const auto& color = std::get<1>(primitive);
        const auto& angle = std::get<2>(primitive);

        // Set rectangle coordinates
        const auto center = rect.center();
        glUniform4f(_programRect2D.vs.param.rect, currentState.windowSize.y - center.y, center.x, rect.height(), rect.width());
        GL_CHECK_RESULT;

        // Set rotation angle
        glUniform1f(_programRect2D.vs.param.angle, angle);
        GL_CHECK_RESULT;

        // Set rectangle color
        glUniform4f(_programRect2D.fs.param.color, SkColorGetR(color) / 255.0f, SkColorGetG(color) / 255.0f, SkColorGetB(color) / 255.0f, SkColorGetA(color) / 255.0f);
        GL_CHECK_RESULT;

        // Draw the rectangle actually
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
        GL_CHECK_RESULT;
    }

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    gpuAPI->unuseVAO();

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::releaseRects2D(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_vaoRect2D.isValid())
    {
        gpuAPI->releaseVAO(_vaoRect2D, gpuContextLost);
        _vaoRect2D.reset();
    }

    if (_iboRect2D.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_iboRect2D);
            GL_CHECK_RESULT;
        }
        _iboRect2D.reset();
    }
    if (_vboRect2D.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_vboRect2D);
            GL_CHECK_RESULT;
        }
        _vboRect2D.reset();
    }

    if (_programRect2D.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_programRect2D.id);
            GL_CHECK_RESULT;
        }
        _programRect2D.id = 0;
    }

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::initializeLines2D()
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
    _programLine2D.id = 0;
    if (!_programLine2D.binaryCache.isEmpty())
    {
        _programLine2D.id = gpuAPI->linkProgram(0, nullptr,
            _programLine2D.binaryCache, _programLine2D.cacheFormat, true, &variablesMap);
    }
    if (!_programLine2D.id.isValid())
    {
        const QString vertexShader = QLatin1String(
            // Input data
            "INPUT vec2 in_vs_vertexPosition; // (1.0, 0.0) for first point, (0.0, 1.0) for second                              ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform mat4 param_vs_mProjectionViewModel;                                                                        ""\n"
            "uniform vec4 param_vs_resultScale;                                                                                 ""\n"
            "uniform vec2 param_vs_v0;                                                                                          ""\n"
            "uniform vec2 param_vs_v1;                                                                                          ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    vec4 v;                                                                                                        ""\n"
            "    v.xy = in_vs_vertexPosition.x*param_vs_v0 + in_vs_vertexPosition.y*param_vs_v1;                                ""\n"
            "    v.z = -1.0;                                                                                                    ""\n"
            "    v.w = 1.0;                                                                                                     ""\n"
            "                                                                                                                   ""\n"
            "    v = param_vs_mProjectionViewModel * v;                                                                         ""\n"
            "    gl_Position = v * param_vs_resultScale;                                                               ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        const QString fragmentShader = QLatin1String(
            // Parameters: common data
            "uniform lowp vec4 param_fs_color;                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    FRAGMENT_COLOR_OUTPUT = param_fs_color;                                                                        ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedFragmentShader = fragmentShader;
        QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        // Read precompiled shaders if available or otherwise compile them and put the binary code in cache if possible
        _programLine2D.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, setupOptions.pathToOpenGLShadersCache, _programLine2D.cacheFormat);

        if (!_programLine2D.binaryCache.isEmpty())
        {
            _programLine2D.id = gpuAPI->linkProgram(0, nullptr,
                _programLine2D.binaryCache, _programLine2D.cacheFormat, true, &variablesMap);
        }
        if (_programLine2D.binaryCache.isEmpty() || !_programLine2D.id.isValid())
        {
            const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
            if (vsId == 0)
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererDebugStage_OpenGL vertex shader");
                return false;
            }
            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererDebugStage_OpenGL fragment shader");
                return false;
            }
            GLuint shaders[] = { vsId, fsId };
            _programLine2D.id = gpuAPI->linkProgram(2, shaders,
                _programLine2D.binaryCache, _programLine2D.cacheFormat, true, &variablesMap);
            if (_programLine2D.id.isValid() && !_programLine2D.binaryCache.isEmpty())
            {
                gpuAPI->writeProgramBinary(
                    preprocessedVertexShader,
                    preprocessedFragmentShader,
                    setupOptions.pathToOpenGLShadersCache,
                    _programLine2D.binaryCache,
                    _programLine2D.cacheFormat);
            }
        }
    }
    if (!_programLine2D.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererDebugStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_programLine2D.id, variablesMap);
    ok = ok && lookup->lookupLocation(_programLine2D.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_programLine2D.vs.param.mProjectionViewModel, "param_vs_mProjectionViewModel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programLine2D.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programLine2D.vs.param.v0, "param_vs_v0", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programLine2D.vs.param.v1, "param_vs_v1", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programLine2D.fs.param.color, "param_fs_color", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_programLine2D.id);
        GL_CHECK_RESULT;
        _programLine2D.id.reset();

        return false;
    }

    // Vertex data (x,y)
    float vertices[2][2] =
    {
        { 1.0f, 0.0f },
        { 0.0f, 1.0f }
    };
    const auto verticesCount = 2;

    // Index data
    GLushort indices[2] =
    {
        0, 1
    };
    const auto indicesCount = 2;

    _vaoLine2D = gpuAPI->allocateUninitializedVAO();

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_vboLine2D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _vboLine2D);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float)* 2, vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_programLine2D.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_programLine2D.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float)* 2, nullptr);
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_iboLine2D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboLine2D);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->initializeVAO(_vaoLine2D);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::renderLines2D()
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glDrawElements);

    gpuAPI->useVAO(_vaoLine2D);

    // Activate program
    glUseProgram(_programLine2D.id);
    GL_CHECK_RESULT;

    // Set projection*view*model matrix:
    glUniformMatrix4fv(_programLine2D.vs.param.mProjectionViewModel, 1, GL_FALSE, glm::value_ptr(internalState.mOrthographicProjection));
    GL_CHECK_RESULT;

    // Scale the result
    glUniform4f(_programLine2D.vs.param.resultScale,
        1.0f,
        currentState.flip ? -1.0f : 1.0f,
        1.0f,
        1.0f);
    GL_CHECK_RESULT;

    for(const auto& primitive : constOf(_lines2D))
    {
        const auto& line = primitive.first;
        const auto& color = primitive.second;

        // Set line color
        glUniform4f(_programLine2D.fs.param.color, SkColorGetR(color) / 255.0f, SkColorGetG(color) / 255.0f, SkColorGetB(color) / 255.0f, SkColorGetA(color) / 255.0f);
        GL_CHECK_RESULT;

        // Iterate over pairs of points
        auto itV0 = line.cbegin();
        auto itV1 = itV0 + 1;
        for(const auto itEnd = line.cend(); itV1 != itEnd; itV0 = itV1, ++itV1)
        {
            const auto& v0 = *itV0;
            const auto& v1 = *itV1;

            // Set line coordinates
            glUniform2f(_programLine2D.vs.param.v0, v0.x, currentState.windowSize.y - v0.y);
            GL_CHECK_RESULT;
            glUniform2f(_programLine2D.vs.param.v1, v1.x, currentState.windowSize.y - v1.y);
            GL_CHECK_RESULT;

            // Draw the line actually
            glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, nullptr);
            GL_CHECK_RESULT;
        }
    }

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    gpuAPI->unuseVAO();

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::releaseLines2D(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_vaoLine2D.isValid())
    {
        gpuAPI->releaseVAO(_vaoLine2D, gpuContextLost);
        _vaoLine2D.reset();
    }

    if (_iboLine2D.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_iboLine2D);
            GL_CHECK_RESULT;
        }
        _iboLine2D.reset();
    }
    if (_vboLine2D.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_vboLine2D);
            GL_CHECK_RESULT;
        }
        _vboLine2D.reset();
    }

    if (_programLine2D.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_programLine2D.id);
            GL_CHECK_RESULT;
        }
        _programLine2D.id = 0;
    }

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::initializeLines3D()
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
    _programLine3D.id = 0;
    if (!_programLine3D.binaryCache.isEmpty())
    {
        _programLine3D.id = gpuAPI->linkProgram(0, nullptr,
            _programLine3D.binaryCache, _programLine3D.cacheFormat, true, &variablesMap);
    }
    if (!_programLine3D.id.isValid())
    {
        const QString vertexShader = QLatin1String(
            // Input data
            "INPUT vec2 in_vs_vertexPosition; // (1.0, 0.0) for first point, (0.0, 1.0) for second                              ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform mat4 param_vs_mProjectionViewModel;                                                                        ""\n"
            "uniform vec4 param_vs_resultScale;                                                                                 ""\n"
            "uniform vec4 param_vs_v0;                                                                                          ""\n"
            "uniform vec4 param_vs_v1;                                                                                          ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    vec4 v;                                                                                                        ""\n"
            "    v = in_vs_vertexPosition.x*param_vs_v0 + in_vs_vertexPosition.y*param_vs_v1;                                   ""\n"
            "                                                                                                                   ""\n"
            "    v = param_vs_mProjectionViewModel * v;                                                                         ""\n"
            "    gl_Position = v * param_vs_resultScale;                                                                        ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        const QString fragmentShader = QLatin1String(
            // Parameters: common data
            "uniform lowp vec4 param_fs_color;                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    FRAGMENT_COLOR_OUTPUT = param_fs_color;                                                                        ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedFragmentShader = fragmentShader;
        QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        // Read precompiled shaders if available or otherwise compile them and put the binary code in cache if possible
        _programLine3D.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, setupOptions.pathToOpenGLShadersCache, _programLine3D.cacheFormat);

        if (!_programLine3D.binaryCache.isEmpty())
        {
            _programLine3D.id = gpuAPI->linkProgram(0, nullptr,
                _programLine3D.binaryCache, _programLine3D.cacheFormat, true, &variablesMap);
        }
        if (_programLine3D.binaryCache.isEmpty() || !_programLine3D.id.isValid())
        {
            const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
            if (vsId == 0)
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererDebugStage_OpenGL vertex shader");
                return false;
            }
            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererDebugStage_OpenGL fragment shader");
                return false;
            }
            GLuint shaders[] = { vsId, fsId };
            _programLine3D.id = gpuAPI->linkProgram(2, shaders,
                _programLine3D.binaryCache, _programLine3D.cacheFormat, true, &variablesMap);
            if (_programLine3D.id.isValid() && !_programLine3D.binaryCache.isEmpty())
            {
                gpuAPI->writeProgramBinary(
                    preprocessedVertexShader,
                    preprocessedFragmentShader,
                    setupOptions.pathToOpenGLShadersCache,
                    _programLine3D.binaryCache,
                    _programLine3D.cacheFormat);
            }
        }
    }
    if (!_programLine3D.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererDebugStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_programLine3D.id, variablesMap);
    ok = ok && lookup->lookupLocation(_programLine3D.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_programLine3D.vs.param.mProjectionViewModel, "param_vs_mProjectionViewModel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programLine3D.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programLine3D.vs.param.v0, "param_vs_v0", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programLine3D.vs.param.v1, "param_vs_v1", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programLine3D.fs.param.color, "param_fs_color", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_programLine3D.id);
        GL_CHECK_RESULT;
        _programLine3D.id.reset();

        return false;
    }

    // Vertex data (x,y)
    float vertices[2][2] =
    {
        { 1.0f, 0.0f },
        { 0.0f, 1.0f }
    };
    const auto verticesCount = 2;

    // Index data
    GLushort indices[2] =
    {
        0, 1
    };
    const auto indicesCount = 2;

    _vaoLine3D = gpuAPI->allocateUninitializedVAO();

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_vboLine3D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _vboLine3D);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float)* 2, vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_programLine3D.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_programLine3D.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float)* 2, nullptr);
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_iboLine3D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboLine3D);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->initializeVAO(_vaoLine3D);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::renderLines3D()
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glDrawElements);

    gpuAPI->useVAO(_vaoLine3D);

    // Activate program
    glUseProgram(_programLine3D.id);
    GL_CHECK_RESULT;

    // Set projection*view*model matrix:
    glUniformMatrix4fv(_programLine3D.vs.param.mProjectionViewModel, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;

    // Scale the result
    glUniform4f(_programLine3D.vs.param.resultScale,
        1.0f,
        currentState.flip ? -1.0f : 1.0f,
        1.0f,
        1.0f);
    GL_CHECK_RESULT;

    for(const auto& primitive : constOf(_lines3D))
    {
        const auto& line = primitive.first;
        const auto& color = primitive.second;

        // Set line color
        glUniform4f(_programLine3D.fs.param.color, SkColorGetR(color) / 255.0f, SkColorGetG(color) / 255.0f, SkColorGetB(color) / 255.0f, SkColorGetA(color) / 255.0f);
        GL_CHECK_RESULT;

        // Iterate over pairs of points
        auto itV0 = line.cbegin();
        auto itV1 = itV0 + 1;
        for(const auto itEnd = line.cend(); itV1 != itEnd; itV0 = itV1, ++itV1)
        {
            const auto& v0 = *itV0;
            const auto& v1 = *itV1;

            // Set line coordinates
            glUniform4f(_programLine3D.vs.param.v0, v0.x, v0.y, v0.z, 1.0f);
            GL_CHECK_RESULT;
            glUniform4f(_programLine3D.vs.param.v1, v1.x, v1.y, v1.z, 1.0f);
            GL_CHECK_RESULT;

            // Draw the line actually
            glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, nullptr);
            GL_CHECK_RESULT;
        }
    }

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    gpuAPI->unuseVAO();

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::releaseLines3D(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_vaoLine3D.isValid())
    {
        gpuAPI->releaseVAO(_vaoLine3D, gpuContextLost);
        _vaoLine3D.reset();
    }

    if (_iboLine3D.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_iboLine3D);
            GL_CHECK_RESULT;
        }
        _iboLine3D.reset();
    }
    if (_vboLine3D.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_vboLine3D);
            GL_CHECK_RESULT;
        }
        _vboLine3D.reset();
    }

    if (_programLine3D.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_programLine3D.id);
            GL_CHECK_RESULT;
        }
        _programLine3D.id = 0;
    }

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::initializeQuads3D()
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
    _programQuad3D.id = 0;
    if (!_programQuad3D.binaryCache.isEmpty())
    {
        _programQuad3D.id = gpuAPI->linkProgram(0, nullptr,
            _programQuad3D.binaryCache, _programQuad3D.cacheFormat, true, &variablesMap);
    }
    if (!_programQuad3D.id.isValid())
    {
        const QString vertexShader = QLatin1String(
            // Input data
            // (1.0, 0.0, 0.0, 0.0) for first point
            // (0.0, 1.0, 0.0, 0.0) for second point
            // (0.0, 0.0, 1.0, 0.0) for third point
            // (0.0, 0.0, 0.0, 1.0) for fourth point
            "INPUT vec4 in_vs_vertexPosition;                                                                                   ""\n"
            "                                                                                                                   ""\n"
            // Parameters: common data
            "uniform mat4 param_vs_mProjectionViewModel;                                                                        ""\n"
            "uniform vec4 param_vs_resultScale;                                                                                 ""\n"
            "uniform vec4 param_vs_v0;                                                                                          ""\n"
            "uniform vec4 param_vs_v1;                                                                                          ""\n"
            "uniform vec4 param_vs_v2;                                                                                          ""\n"
            "uniform vec4 param_vs_v3;                                                                                          ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    vec4 v =                                                                                                       ""\n"
            "        in_vs_vertexPosition.x*param_vs_v0 +                                                                       ""\n"
            "        in_vs_vertexPosition.y*param_vs_v1 +                                                                       ""\n"
            "        in_vs_vertexPosition.z*param_vs_v2 +                                                                       ""\n"
            "        in_vs_vertexPosition.w*param_vs_v3;                                                                        ""\n"
            "                                                                                                                   ""\n"
            "    v = param_vs_mProjectionViewModel * v;                                                                         ""\n"
            "    gl_Position = v * param_vs_resultScale;                                                                        ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        const QString fragmentShader = QLatin1String(
            // Parameters: common data
            "uniform lowp vec4 param_fs_color;                                                                                  ""\n"
            "                                                                                                                   ""\n"
            "void main()                                                                                                        ""\n"
            "{                                                                                                                  ""\n"
            "    FRAGMENT_COLOR_OUTPUT = param_fs_color;                                                                        ""\n"
            "}                                                                                                                  ""\n");
        auto preprocessedFragmentShader = fragmentShader;
        QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        // Read precompiled shaders if available or otherwise compile them and put the binary code in cache if possible
        _programQuad3D.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, setupOptions.pathToOpenGLShadersCache, _programQuad3D.cacheFormat);

        if (!_programQuad3D.binaryCache.isEmpty())
        {
            _programQuad3D.id = gpuAPI->linkProgram(0, nullptr,
                _programQuad3D.binaryCache, _programQuad3D.cacheFormat, true, &variablesMap);
        }
        if (_programQuad3D.binaryCache.isEmpty() || !_programQuad3D.id.isValid())
        {
            const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
            if (vsId == 0)
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererDebugStage_OpenGL vertex shader");
                return false;
            }
            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererDebugStage_OpenGL fragment shader");
                return false;
            }
            GLuint shaders[] = { vsId, fsId };
            _programQuad3D.id = gpuAPI->linkProgram(2, shaders,
                _programQuad3D.binaryCache, _programQuad3D.cacheFormat, true, &variablesMap);
            if (_programQuad3D.id.isValid() && !_programQuad3D.binaryCache.isEmpty())
            {
                gpuAPI->writeProgramBinary(
                    preprocessedVertexShader,
                    preprocessedFragmentShader,
                    setupOptions.pathToOpenGLShadersCache,
                    _programQuad3D.binaryCache,
                    _programQuad3D.cacheFormat);
            }
        }
    }
    if (!_programQuad3D.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRendererDebugStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_programQuad3D.id, variablesMap);
    ok = ok && lookup->lookupLocation(_programQuad3D.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_programQuad3D.vs.param.mProjectionViewModel, "param_vs_mProjectionViewModel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programQuad3D.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programQuad3D.vs.param.v0, "param_vs_v0", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programQuad3D.vs.param.v1, "param_vs_v1", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programQuad3D.vs.param.v2, "param_vs_v2", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programQuad3D.vs.param.v3, "param_vs_v3", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_programQuad3D.fs.param.color, "param_fs_color", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_programQuad3D.id);
        GL_CHECK_RESULT;
        _programQuad3D.id.reset();

        return false;
    }

    // Vertex data (x,y,z,w)
    float vertices[4][4] =
    {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };
    const auto verticesCount = 4;

    // Index data
    GLushort indices[6] =
    {
        0, 1, 2,
        0, 2, 3
    };
    const auto indicesCount = 6;

    _vaoQuad3D = gpuAPI->allocateUninitializedVAO();

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_vboQuad3D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _vboQuad3D);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float)*4, vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_programQuad3D.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_programQuad3D.vs.in.vertexPosition, 4, GL_FLOAT, GL_FALSE, sizeof(float)*4, nullptr);
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_iboQuad3D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboQuad3D);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->initializeVAO(_vaoQuad3D);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::renderQuads3D()
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glDrawElements);

    gpuAPI->useVAO(_vaoQuad3D);

    // Activate program
    glUseProgram(_programQuad3D.id);
    GL_CHECK_RESULT;

    // Set projection*view*model matrix:
    glUniformMatrix4fv(_programQuad3D.vs.param.mProjectionViewModel, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;

    // Scale the result
    glUniform4f(_programQuad3D.vs.param.resultScale,
        1.0f,
        currentState.flip ? -1.0f : 1.0f,
        1.0f,
        1.0f);
    GL_CHECK_RESULT;

    for(const auto& primitive : constOf(_quads3D))
    {
        const auto& p0 = std::get<0>(primitive);
        const auto& p1 = std::get<1>(primitive);
        const auto& p2 = std::get<2>(primitive);
        const auto& p3 = std::get<3>(primitive);
        const auto& color = std::get<4>(primitive);

        // Set quad color
        glUniform4f(_programQuad3D.fs.param.color, SkColorGetR(color) / 255.0f, SkColorGetG(color) / 255.0f, SkColorGetB(color) / 255.0f, SkColorGetA(color) / 255.0f);
        GL_CHECK_RESULT;

        // Set points
        glUniform4f(_programQuad3D.vs.param.v0, p0.x, p0.y, p0.z, 1.0f);
        GL_CHECK_RESULT;
        glUniform4f(_programQuad3D.vs.param.v1, p1.x, p1.y, p1.z, 1.0f);
        GL_CHECK_RESULT;
        glUniform4f(_programQuad3D.vs.param.v2, p2.x, p2.y, p2.z, 1.0f);
        GL_CHECK_RESULT;
        glUniform4f(_programQuad3D.vs.param.v3, p3.x, p3.y, p3.z, 1.0f);
        GL_CHECK_RESULT;

        // Draw the quad actually
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
        GL_CHECK_RESULT;
    }

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    gpuAPI->unuseVAO();

    return true;
}

bool OsmAnd::AtlasMapRendererDebugStage_OpenGL::releaseQuads3D(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_vaoQuad3D.isValid())
    {
        gpuAPI->releaseVAO(_vaoQuad3D, gpuContextLost);
        _vaoQuad3D.reset();
    }

    if (_iboQuad3D.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_iboQuad3D);
            GL_CHECK_RESULT;
        }
        _iboQuad3D.reset();
    }
    if (_vboQuad3D.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_vboQuad3D);
            GL_CHECK_RESULT;
        }
        _vboQuad3D.reset();
    }

    if (_programQuad3D.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_programQuad3D.id);
            GL_CHECK_RESULT;
        }
        _programQuad3D.id = 0;
    }

    return true;
}
