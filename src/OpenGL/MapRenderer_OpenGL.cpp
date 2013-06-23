#include "MapRenderer_OpenGL.h"

#include <assert.h>

#include "OsmAndLogging.h"

OsmAnd::MapRenderer_OpenGL::MapRenderer_OpenGL()
{
}

OsmAnd::MapRenderer_OpenGL::~MapRenderer_OpenGL()
{

}

void OsmAnd::MapRenderer_OpenGL::validateResult()
{
    auto result = glGetError();
    if(result == GL_NO_ERROR)
        return;

    LogPrintf(LogSeverityLevel::Error, "OpenGL error 0x%08x : %s\n", result, gluErrorString(result));
}

GLuint OsmAnd::MapRenderer_OpenGL::compileShader( GLenum shaderType, const char* source )
{
    GLuint shader;

    assert(glCreateShader);
    shader = glCreateShader(shaderType);
    GL_CHECK_RESULT;

    const GLint sourceLen = static_cast<GLint>(strlen(source));
    assert(glShaderSource);
    glShaderSource(shader, 1, &source, &sourceLen);
    GL_CHECK_RESULT;

    assert(glCompileShader);
    glCompileShader(shader);
    GL_CHECK_RESULT;

    // Check if compiled (if possible)
    if(glGetObjectParameterivARB != nullptr)
    {
        GLint didCompile;
        glGetObjectParameterivARB(shader, GL_COMPILE_STATUS, &didCompile);
        GL_CHECK_RESULT;
        if(!didCompile && glGetShaderInfoLog != nullptr)
        {
            GLint logBufferLen = 0;	
            GLsizei logLen = 0;
            assert(glGetShaderiv);
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logBufferLen);       
            GL_CHECK_RESULT;
            if (logBufferLen > 1)
            {
                GLchar* log = (GLchar*)malloc(logBufferLen);
                glGetShaderInfoLog(shader, logBufferLen, &logLen, log);
                GL_CHECK_RESULT;
                LogPrintf(LogSeverityLevel::Error, "Failed to compile GLSL shader: %s", log);
                free(log);
            }
        }
    }

    return shader;
}
