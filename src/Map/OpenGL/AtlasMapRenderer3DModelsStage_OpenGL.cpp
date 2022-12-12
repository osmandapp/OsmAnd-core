#include "AtlasMapRenderer3DModelsStage_OpenGL.h"

#include "AtlasMapRenderer_Metrics.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

OsmAnd::AtlasMapRenderer3DModelsStage_OpenGL::AtlasMapRenderer3DModelsStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer_)
    : AtlasMapRenderer3DModelsStage(renderer_)
    , AtlasMapRendererStageHelper_OpenGL(this)
{
    typedef std::tuple<ZoomLevel, float> ZoomScale;
    QList<ZoomScale> scalesByZoomRange;
    scalesByZoomRange.push_back(ZoomScale(MinZoomLevel, 2.0f));
    scalesByZoomRange.push_back(ZoomScale(ZoomLevel4, 2.0f));
    scalesByZoomRange.push_back(ZoomScale(ZoomLevel9, 2.5f));
    scalesByZoomRange.push_back(ZoomScale(ZoomLevel18, 3.5f));
    scalesByZoomRange.push_back(ZoomScale(ZoomLevel22, 8.0f));
    scalesByZoomRange.push_back(ZoomScale(MaxZoomLevel, 10.0f));

    for (auto i = 1; i < scalesByZoomRange.size(); i++)
    {
        auto left = scalesByZoomRange.at(i - 1);
        auto right = scalesByZoomRange.at(i);

        auto leftZoom = std::get<0>(left);
        auto rightZoom = std::get<0>(right);
        auto zoomRange = rightZoom - leftZoom;

        auto leftScale = std::get<1>(left);
        auto rightScale = std::get<1>(right);
        auto scaleRange = rightScale - leftScale;

        auto lastRange = i + 1 == scalesByZoomRange.size();
        auto endZoomInclusive = lastRange ? rightZoom : rightZoom - 1;
        for (auto zoom = leftZoom; zoom <= endZoomInclusive; zoom = static_cast<ZoomLevel>(zoom + 1))
        {
            float scale = leftScale + scaleRange * (zoom - leftZoom) / zoomRange;
            _allZoomScales.push_back(scale);
        }
    }
}

OsmAnd::AtlasMapRenderer3DModelsStage_OpenGL::~AtlasMapRenderer3DModelsStage_OpenGL()
{
}

bool OsmAnd::AtlasMapRenderer3DModelsStage_OpenGL::initialize()
{
    ObjParser parser(QString("./model/model.obj"), QString("./model/"));
    _successfulLoadModel = parser.parse(_model);

    // Uncomment for custom coloring
    // QHash<QString, FColorRGBA> customColors;
    // customColors.insert("Body Material", FColorRGBA(0, 0, 1, 1));
    // customColors.insert("Glass Material", FColorRGBA(0, 1, 0, 1));
    // _model->setCustomMaterialColors(customColors);

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
        "INPUT vec3 in_vs_vertexPosition;                                                                                   ""\n"
        "INPUT vec4 in_vs_vertexColor;                                                                                      ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "PARAM_OUTPUT vec4 v2f_color;                                                                                       ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mModel;                                                                                      ""\n"
        "uniform mat4 param_vs_mPerspectiveProjectionView;                                                                  ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    gl_Position = param_vs_mPerspectiveProjectionView * param_vs_mModel * vec4(in_vs_vertexPosition, 1.0);         ""\n"
        "    v2f_color = in_vs_vertexColor;                                                                                 ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    if (vsId == 0)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to compile AtlasMapRenderer3DModelsStage_OpenGL vertex shader");
        return false;
    }

    // Compile fragment shader
    const QString fragmentShader = QLatin1String(
        // Input data
        "PARAM_INPUT vec4 v2f_color;                                                                                        ""\n"
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
            "Failed to compile AtlasMapRenderer3DModelsStage_OpenGL fragment shader");
        return false;
    }

    // Link everything into program object
    const GLuint shaders[] = { vsId, fsId };
    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    _program.id = gpuAPI->linkProgram(2, shaders, true, &variablesMap);
    if (!_program.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link AtlasMapRenderer3DModelsStage_OpenGL program");
        return false;
    }

    bool ok = true;
    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_program.id, variablesMap);
    ok = ok && lookup->lookupLocation(_program.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);    
    ok = ok && lookup->lookupLocation(_program.vs.in.vertexColor, "in_vs_vertexColor", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.params.mModel, "param_vs_mModel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.params.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    if (!ok)
    {
        glDeleteProgram(_program.id);
        GL_CHECK_RESULT;
        _program.id.reset();

        return false;
    }

    _3DModelVAO = gpuAPI->allocateUninitializedVAO();

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_3DModelVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _3DModelVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, _model->getVerticesCount() * sizeof(Model3D::Vertex), _model->getVertices().constData(), GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_program.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(
        *_program.vs.in.vertexPosition,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Model3D::Vertex),
        reinterpret_cast<GLvoid*>(offsetof(Model3D::Vertex, xyz)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_program.vs.in.vertexColor);
    GL_CHECK_RESULT;
    glVertexAttribPointer(
        *_program.vs.in.vertexColor,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Model3D::Vertex),
        reinterpret_cast<GLvoid*>(offsetof(Model3D::Vertex, rgba)));
    GL_CHECK_RESULT;

    // Unbind buffers
    gpuAPI->initializeVAO(_3DModelVAO);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    return true;
}

bool OsmAnd::AtlasMapRenderer3DModelsStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* const metric_)
{
    if (!_successfulLoadModel)
        return true;

    const auto gpuAPI = getGPUAPI();

    GL_PUSH_GROUP_MARKER(QLatin1String("3DModels"));

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glDrawArrays);

    const auto& internalState = getInternalState();

    gpuAPI->useVAO(_3DModelVAO);

    // Activate program
    glUseProgram(_program.id);
    GL_CHECK_RESULT;

    float modelScale = 1 / currentState.visualZoom;
    const auto zoomLevel = currentState.zoomLevel;
    const auto visualZoom = currentState.visualZoom;
    const auto zoomScale = _allZoomScales.at(zoomLevel);
    
    if (visualZoom >= 1.0f)
    {
        if (zoomLevel == MaxZoomLevel)
            modelScale *= zoomScale;
        else
        {
            auto nextZoomScale = _allZoomScales.at(zoomLevel + 1);
            auto deltaScale = nextZoomScale - zoomScale;
            auto zoomFraction = visualZoom - 1.0f;
            modelScale *= zoomScale + deltaScale * zoomFraction;
        }
    }
    else
    {
        if (zoomLevel == MinZoomLevel)
            modelScale *= zoomScale;
        else
        {
            auto prevZoomScale = _allZoomScales.at(zoomLevel - 1);
            auto deltaScale = zoomScale - prevZoomScale;
            auto zoomFraction = -2 * (visualZoom - 1);
            modelScale *= zoomScale - deltaScale * zoomFraction;
        }
    }

    const auto scaleMatrix = glm::scale(glm::vec3(modelScale, modelScale, modelScale));
    const auto translateMatrix = glm::translate(glm::vec3(0, 0.6f, 0));
    const auto modelMatrix = scaleMatrix * translateMatrix;
    glUniformMatrix4fv(
        _program.vs.params.mModel,
        1,
        GL_FALSE,
        glm::value_ptr(modelMatrix));
    GL_CHECK_RESULT;

    glUniformMatrix4fv(
        _program.vs.params.mPerspectiveProjectionView,
        1,
        GL_FALSE,
        glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;

    // Draw models actually
    glDrawArrays(GL_TRIANGLES, 0, _model->getVerticesCount());
    GL_CHECK_RESULT;

    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    gpuAPI->unuseVAO();

    GL_POP_GROUP_MARKER;

    return true;
}

bool OsmAnd::AtlasMapRenderer3DModelsStage_OpenGL::release(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);

    if (_3DModelVAO.isValid())
    {
        gpuAPI->releaseVAO(_3DModelVAO, gpuContextLost);
        _3DModelVAO.reset();
    }

    if (_3DModelVBO.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteBuffers(1, &_3DModelVBO);
            GL_CHECK_RESULT;
        }
        _3DModelVBO.reset();
    }

    if (_program.id.isValid())
    {
        if (!gpuContextLost)
        {
            glDeleteProgram(_program.id);
            GL_CHECK_RESULT;
        }
        _program.id.reset();
    }

    return true;
}