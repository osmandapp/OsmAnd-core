#include "AtlasMapRendererDebugStage_OpenGL.h"

#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <SkColor.h>

OsmAnd::AtlasMapRendererDebugStage_OpenGL::AtlasMapRendererDebugStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer)
    : AtlasMapRendererStage_OpenGL(renderer)
    , _vaoRect2D(0)
    , _vboRect2D(0)
    , _iboRect2D(0)
    , _vaoLine2D(0)
    , _vboLine2D(0)
    , _iboLine2D(0)
    , _vaoLine3D(0)
    , _vboLine3D(0)
    , _iboLine3D(0)
    , _vaoQuad3D(0)
    , _vboQuad3D(0)
    , _iboQuad3D(0)
{
    memset(&_programRect2D, 0, sizeof(_programRect2D));
    memset(&_programLine2D, 0, sizeof(_programLine2D));
    memset(&_programLine3D, 0, sizeof(_programLine3D));
    memset(&_programQuad3D, 0, sizeof(_programQuad3D));
}

OsmAnd::AtlasMapRendererDebugStage_OpenGL::~AtlasMapRendererDebugStage_OpenGL()
{
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::initialize()
{
    initializeRects2D();
    initializeLines2D();
    initializeLines3D();
    initializeQuads3D();
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::render()
{
    GL_PUSH_GROUP_MARKER(QLatin1String("debug"));

    renderRects2D();
    renderLines2D();
    renderLines3D();
    renderQuads3D();

    GL_POP_GROUP_MARKER;
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::release()
{
    releaseRects2D();
    releaseLines2D();
    releaseLines3D();
    releaseQuads3D();
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::initializeRects2D()
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
        "    gl_Position = param_vs_mProjectionViewModel * v;                                                               ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    assert(vsId != 0);

    // Compile fragment shader
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
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    assert(fsId != 0);

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    _programRect2D.id = gpuAPI->linkProgram(2, shaders);
    assert(_programRect2D.id != 0);

    gpuAPI->clearVariablesLookup();
    gpuAPI->findVariableLocation(_programRect2D.id, _programRect2D.vs.in.vertexPosition, "in_vs_vertexPosition", GLShaderVariableType::In);
    gpuAPI->findVariableLocation(_programRect2D.id, _programRect2D.vs.param.mProjectionViewModel, "param_vs_mProjectionViewModel", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programRect2D.id, _programRect2D.vs.param.rect, "param_vs_rect", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programRect2D.id, _programRect2D.vs.param.angle, "param_vs_angle", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programRect2D.id, _programRect2D.fs.param.color, "param_fs_color", GLShaderVariableType::Uniform);
    gpuAPI->clearVariablesLookup();

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

    // Create Vertex Array Object
    gpuAPI->glGenVertexArrays_wrapper(1, &_vaoRect2D);
    GL_CHECK_RESULT;
    gpuAPI->glBindVertexArray_wrapper(_vaoRect2D);
    GL_CHECK_RESULT;

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_vboRect2D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _vboRect2D);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float)* 2, vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(_programRect2D.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(_programRect2D.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float)* 2, nullptr);
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_iboRect2D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboRect2D);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::renderRects2D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glDrawElements);

    gpuAPI->glBindVertexArray_wrapper(_vaoRect2D);
    GL_CHECK_RESULT;

    // Activate program
    glUseProgram(_programRect2D.id);
    GL_CHECK_RESULT;

    // Set projection*view*model matrix:
    glUniformMatrix4fv(_programRect2D.vs.param.mProjectionViewModel, 1, GL_FALSE, glm::value_ptr(internalState.mOrthographicProjection));
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

    // Deselect VAO
    gpuAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::releaseRects2D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if(_iboRect2D != 0)
    {
        glDeleteBuffers(1, &_iboRect2D);
        GL_CHECK_RESULT;
        _iboRect2D = 0;
    }
    if(_vboRect2D != 0)
    {
        glDeleteBuffers(1, &_vboRect2D);
        GL_CHECK_RESULT;
        _vboRect2D = 0;
    }
    if(_vaoRect2D != 0)
    {
        gpuAPI->glDeleteVertexArrays_wrapper(1, &_vaoRect2D);
        GL_CHECK_RESULT;
        _vaoRect2D = 0;
    }
    if(_programRect2D.id != 0)
    {
        glDeleteProgram(_programRect2D.id);
        GL_CHECK_RESULT;
        memset(&_programRect2D, 0, sizeof(_programRect2D));
    }
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::initializeLines2D()
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
        "INPUT vec2 in_vs_vertexPosition; // (1.0, 0.0) for first point, (0.0, 1.0) for second                              ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mProjectionViewModel;                                                                        ""\n"
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
        "    gl_Position = param_vs_mProjectionViewModel * v;                                                               ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    assert(vsId != 0);

    // Compile fragment shader
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
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    assert(fsId != 0);

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    _programLine2D.id = gpuAPI->linkProgram(2, shaders);
    assert(_programLine2D.id != 0);

    gpuAPI->clearVariablesLookup();
    gpuAPI->findVariableLocation(_programLine2D.id, _programLine2D.vs.in.vertexPosition, "in_vs_vertexPosition", GLShaderVariableType::In);
    gpuAPI->findVariableLocation(_programLine2D.id, _programLine2D.vs.param.mProjectionViewModel, "param_vs_mProjectionViewModel", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programLine2D.id, _programLine2D.vs.param.v0, "param_vs_v0", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programLine2D.id, _programLine2D.vs.param.v1, "param_vs_v1", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programLine2D.id, _programLine2D.fs.param.color, "param_fs_color", GLShaderVariableType::Uniform);
    gpuAPI->clearVariablesLookup();

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

    // Create Vertex Array Object
    gpuAPI->glGenVertexArrays_wrapper(1, &_vaoLine2D);
    GL_CHECK_RESULT;
    gpuAPI->glBindVertexArray_wrapper(_vaoLine2D);
    GL_CHECK_RESULT;

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_vboLine2D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _vboLine2D);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float)* 2, vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(_programLine2D.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(_programLine2D.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float)* 2, nullptr);
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_iboLine2D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboLine2D);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::renderLines2D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glDrawElements);

    gpuAPI->glBindVertexArray_wrapper(_vaoLine2D);
    GL_CHECK_RESULT;

    // Activate program
    glUseProgram(_programLine2D.id);
    GL_CHECK_RESULT;

    // Set projection*view*model matrix:
    glUniformMatrix4fv(_programLine2D.vs.param.mProjectionViewModel, 1, GL_FALSE, glm::value_ptr(internalState.mOrthographicProjection));
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

    // Deselect VAO
    gpuAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::releaseLines2D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if(_iboLine2D != 0)
    {
        glDeleteBuffers(1, &_iboLine2D);
        GL_CHECK_RESULT;
        _iboLine2D = 0;
    }
    if(_vboLine2D != 0)
    {
        glDeleteBuffers(1, &_vboLine2D);
        GL_CHECK_RESULT;
        _vboLine2D = 0;
    }
    if(_vaoLine2D != 0)
    {
        gpuAPI->glDeleteVertexArrays_wrapper(1, &_vaoLine2D);
        GL_CHECK_RESULT;
        _vaoLine2D = 0;
    }
    if(_programLine2D.id != 0)
    {
        glDeleteProgram(_programLine2D.id);
        GL_CHECK_RESULT;
        memset(&_programLine2D, 0, sizeof(_programLine2D));
    }
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::initializeLines3D()
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
        "INPUT vec2 in_vs_vertexPosition; // (1.0, 0.0) for first point, (0.0, 1.0) for second                              ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mProjectionViewModel;                                                                        ""\n"
        "uniform vec4 param_vs_v0;                                                                                          ""\n"
        "uniform vec4 param_vs_v1;                                                                                          ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    vec4 v;                                                                                                        ""\n"
        "    v = in_vs_vertexPosition.x*param_vs_v0 + in_vs_vertexPosition.y*param_vs_v1;                                   ""\n"
        "                                                                                                                   ""\n"
        "    gl_Position = param_vs_mProjectionViewModel * v;                                                               ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    assert(vsId != 0);

    // Compile fragment shader
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
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    assert(fsId != 0);

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    _programLine3D.id = gpuAPI->linkProgram(2, shaders);
    assert(_programLine3D.id != 0);

    gpuAPI->clearVariablesLookup();
    gpuAPI->findVariableLocation(_programLine3D.id, _programLine3D.vs.in.vertexPosition, "in_vs_vertexPosition", GLShaderVariableType::In);
    gpuAPI->findVariableLocation(_programLine3D.id, _programLine3D.vs.param.mProjectionViewModel, "param_vs_mProjectionViewModel", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programLine3D.id, _programLine3D.vs.param.v0, "param_vs_v0", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programLine3D.id, _programLine3D.vs.param.v1, "param_vs_v1", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programLine3D.id, _programLine3D.fs.param.color, "param_fs_color", GLShaderVariableType::Uniform);
    gpuAPI->clearVariablesLookup();

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

    // Create Vertex Array Object
    gpuAPI->glGenVertexArrays_wrapper(1, &_vaoLine3D);
    GL_CHECK_RESULT;
    gpuAPI->glBindVertexArray_wrapper(_vaoLine3D);
    GL_CHECK_RESULT;

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_vboLine3D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _vboLine3D);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float)* 2, vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(_programLine3D.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(_programLine3D.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float)* 2, nullptr);
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_iboLine3D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboLine3D);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::renderLines3D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glDrawElements);

    gpuAPI->glBindVertexArray_wrapper(_vaoLine3D);
    GL_CHECK_RESULT;

    // Activate program
    glUseProgram(_programLine3D.id);
    GL_CHECK_RESULT;

    // Set projection*view*model matrix:
    const auto& mProjectionView = internalState.mPerspectiveProjection * internalState.mCameraView;
    glUniformMatrix4fv(_programLine3D.vs.param.mProjectionViewModel, 1, GL_FALSE, glm::value_ptr(mProjectionView));
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

    // Deselect VAO
    gpuAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::releaseLines3D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if(_iboLine3D != 0)
    {
        glDeleteBuffers(1, &_iboLine3D);
        GL_CHECK_RESULT;
        _iboLine3D = 0;
    }
    if(_vboLine3D != 0)
    {
        glDeleteBuffers(1, &_vboLine3D);
        GL_CHECK_RESULT;
        _vboLine3D = 0;
    }
    if(_vaoLine3D != 0)
    {
        gpuAPI->glDeleteVertexArrays_wrapper(1, &_vaoLine3D);
        GL_CHECK_RESULT;
        _vaoLine3D = 0;
    }
    if(_programLine3D.id != 0)
    {
        glDeleteProgram(_programLine3D.id);
        GL_CHECK_RESULT;
        memset(&_programLine3D, 0, sizeof(_programLine3D));
    }
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::initializeQuads3D()
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
        // (1.0, 0.0, 0.0, 0.0) for first point
        // (0.0, 1.0, 0.0, 0.0) for second point
        // (0.0, 0.0, 1.0, 0.0) for third point
        // (0.0, 0.0, 0.0, 1.0) for fourth point
        "INPUT vec4 in_vs_vertexPosition;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mProjectionViewModel;                                                                        ""\n"
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
        "    gl_Position = param_vs_mProjectionViewModel * v;                                                               ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    assert(vsId != 0);

    // Compile fragment shader
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
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    assert(fsId != 0);

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    _programQuad3D.id = gpuAPI->linkProgram(2, shaders);
    assert(_programQuad3D.id != 0);

    gpuAPI->clearVariablesLookup();
    gpuAPI->findVariableLocation(_programQuad3D.id, _programQuad3D.vs.in.vertexPosition, "in_vs_vertexPosition", GLShaderVariableType::In);
    gpuAPI->findVariableLocation(_programQuad3D.id, _programQuad3D.vs.param.mProjectionViewModel, "param_vs_mProjectionViewModel", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programQuad3D.id, _programQuad3D.vs.param.v0, "param_vs_v0", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programQuad3D.id, _programQuad3D.vs.param.v1, "param_vs_v1", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programQuad3D.id, _programQuad3D.vs.param.v2, "param_vs_v2", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programQuad3D.id, _programQuad3D.vs.param.v3, "param_vs_v3", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_programQuad3D.id, _programQuad3D.fs.param.color, "param_fs_color", GLShaderVariableType::Uniform);
    gpuAPI->clearVariablesLookup();

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

    // Create Vertex Array Object
    gpuAPI->glGenVertexArrays_wrapper(1, &_vaoQuad3D);
    GL_CHECK_RESULT;
    gpuAPI->glBindVertexArray_wrapper(_vaoQuad3D);
    GL_CHECK_RESULT;

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_vboQuad3D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _vboQuad3D);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float)*4, vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(_programQuad3D.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(_programQuad3D.vs.in.vertexPosition, 4, GL_FLOAT, GL_FALSE, sizeof(float)*4, nullptr);
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_iboQuad3D);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboQuad3D);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::renderQuads3D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glDrawElements);

    gpuAPI->glBindVertexArray_wrapper(_vaoQuad3D);
    GL_CHECK_RESULT;

    // Activate program
    glUseProgram(_programQuad3D.id);
    GL_CHECK_RESULT;

    // Set projection*view*model matrix:
    const auto& mProjectionView = internalState.mPerspectiveProjection * internalState.mCameraView;
    glUniformMatrix4fv(_programQuad3D.vs.param.mProjectionViewModel, 1, GL_FALSE, glm::value_ptr(mProjectionView));
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

    // Deselect VAO
    gpuAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::releaseQuads3D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if(_iboQuad3D != 0)
    {
        glDeleteBuffers(1, &_iboQuad3D);
        GL_CHECK_RESULT;
        _iboQuad3D = 0;
    }
    if(_vboQuad3D != 0)
    {
        glDeleteBuffers(1, &_vboQuad3D);
        GL_CHECK_RESULT;
        _vboQuad3D = 0;
    }
    if(_vaoQuad3D != 0)
    {
        gpuAPI->glDeleteVertexArrays_wrapper(1, &_vaoQuad3D);
        GL_CHECK_RESULT;
        _vaoQuad3D = 0;
    }
    if(_programQuad3D.id != 0)
    {
        glDeleteProgram(_programQuad3D.id);
        GL_CHECK_RESULT;
        memset(&_programQuad3D, 0, sizeof(_programQuad3D));
    }
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::clear()
{
    _rects2D.clear();
    _lines2D.clear();
    _lines3D.clear();
    _quads3D.clear();
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::addRect2D(const AreaF& rect, const uint32_t argbColor, const float angle /*= 0.0f*/)
{
    _rects2D.push_back(qMove(Rect2D(rect, argbColor, angle)));
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::addLine2D(const QVector<glm::vec2>& line, const uint32_t argbColor)
{
    assert(line.size() >= 2);
    _lines2D.push_back(qMove(Line2D(line, argbColor)));
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::addLine3D(const QVector<glm::vec3>& line, const uint32_t argbColor)
{
    assert(line.size() >= 2);
    _lines3D.push_back(qMove(Line3D(line, argbColor)));
}

void OsmAnd::AtlasMapRendererDebugStage_OpenGL::addQuad3D(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const uint32_t argbColor)
{
    _quads3D.push_back(qMove(Quad3D(p0, p1, p2, p3, argbColor)));
}
