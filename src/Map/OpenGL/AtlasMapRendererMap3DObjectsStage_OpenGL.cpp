#include "AtlasMapRendererMap3DObjectsStage_OpenGL.h"

#include "OpenGL/GPUAPI_OpenGL.h"
#include "AtlasMapRenderer_OpenGL.h"

using namespace OsmAnd;

AtlasMapRendererMap3DObjectsStage_OpenGL::AtlasMapRendererMap3DObjectsStage_OpenGL(AtlasMapRenderer_OpenGL* renderer)
    : AtlasMapRendererMap3DObjectsStage(renderer)
    , AtlasMapRendererStageHelper_OpenGL(this)
{
}

AtlasMapRendererMap3DObjectsStage_OpenGL::~AtlasMapRendererMap3DObjectsStage_OpenGL()
{
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::initializeProgram()
{
    const auto gpuAPI = getGPUAPI();
    
    const char* vsSrc =
        "attribute vec2 aPos;\n"
        "void main() {\n"
        "  gl_Position = vec4(aPos, 0.0, 1.0);\n"
        "}\n";
    const char* fsSrc =
        "void main() {\n"
        "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
        "}\n";

    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, vsSrc);
    if (!vsId)
        return false;

    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, fsSrc);
    if (!fsId)
        return false;

    const GLuint shaders[] = { vsId, fsId };
    QByteArray binaryCache;
    GLenum cacheFormat;
    _program = gpuAPI->linkProgram(2, shaders, {}, binaryCache, cacheFormat, true, nullptr);
    
    return _program.isValid();
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::initialize()
{
    const auto gpuAPI = getGPUAPI();
    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    // Triangle positions in clip space centered on screen
    const float vertices[] = {
        0.0f,  0.2f,
       -0.2f, -0.2f,
        0.2f, -0.2f,
    };

    _vao = gpuAPI->allocateUninitializedVAO();
    
    glGenBuffers(1, &_vbo);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    
    glEnableVertexAttribArray(0);
    GL_CHECK_RESULT;
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, reinterpret_cast<const GLvoid*>(0));
    GL_CHECK_RESULT;
    
    gpuAPI->initializeVAO(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_RESULT;

    return initializeProgram();
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* const /*metric*/)
{
    const auto gpuAPI = getGPUAPI();
    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glDrawArrays);

    glUseProgram(_program);
    GL_CHECK_RESULT;
    gpuAPI->useVAO(_vao);
    GL_CHECK_RESULT;
    glDrawArrays(GL_TRIANGLES, 0, 3);
    GL_CHECK_RESULT;

    return true;
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::release(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();
    if (_vbo)
    {
        glDeleteBuffers(1, &_vbo);
        GL_CHECK_RESULT;
        _vbo = 0;
    }
    if (_vao.isValid())
    {
        gpuAPI->releaseVAO(_vao, gpuContextLost);
        _vao.reset();
    }
    if (_program)
    {
        glDeleteProgram(_program);
        GL_CHECK_RESULT;
        _program = 0;
    }
    return true;
}


