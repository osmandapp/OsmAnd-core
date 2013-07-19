#include "MapRenderer_OpenGLES2.h"

#include <assert.h>

#include <QtMath>

#include "IMapRenderer.h"
#include "IMapTileProvider.h"
#include "IMapBitmapTileProvider.h"
#include "IMapElevationDataProvider.h"
#include "OsmAndCore/Logging.h"
#include "OsmAndCore/Utilities.h"

#if 0
#   include <SkBitmap.h>
#   include <SkImageEncoder.h>
#endif

OsmAnd::MapRenderer_OpenGLES2::MapRenderer_OpenGLES2()
    : _textureSampler_Bitmap_NoAtlas(0)
    , _textureSampler_Bitmap_Atlas(0)
    , _textureSampler_ElevationData_NoAtlas(0)
    , _textureSampler_ElevationData_Atlas(0)
    , _maxAnisotropy(-1)
{
}

OsmAnd::MapRenderer_OpenGLES2::~MapRenderer_OpenGLES2()
{
}

GLenum OsmAnd::MapRenderer_OpenGLES2::validateResult()
{
    auto result = glGetError();
    if(result == GL_NO_ERROR)
        return result;

    const char* errorString = nullptr;
    switch(result)
    {
        case GL_NO_ERROR:
            errorString = "no error";
            break;
        case GL_INVALID_ENUM:
            errorString = "invalid enumerant";
            break;
        case GL_INVALID_VALUE:
            errorString = "invalid value";
            break;
        case GL_INVALID_OPERATION:
            errorString = "invalid operation";
            break;
        case GL_STACK_OVERFLOW:
            errorString = "stack overflow";
            break;
        case GL_STACK_UNDERFLOW:
            errorString = "stack underflow";
            break;
        case GL_OUT_OF_MEMORY:
            errorString = "out of memory";
            break;
        default:
            errorString = "(unknown)";
            break;
    }
    LogPrintf(LogSeverityLevel::Error, "OpenGLES2 error 0x%08x : %s\n", result, errorString);

    return result;
}

void OsmAnd::MapRenderer_OpenGLES2::initializeRendering()
{
    MapRenderer_BaseOpenGL::initializeRendering();

    const auto glVersionString = glGetString(GL_VERSION);
    GL_CHECK_RESULT;
    GLint glVersion[2];
    glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
    GL_CHECK_RESULT;
    glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "Using OpenGLES2 version %d.%d [%s]\n", glVersion[0], glVersion[1], glVersionString);
    assert(glVersion[0] >= 3);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&_maxTextureSize));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture size %dx%d\n", _maxTextureSize, _maxTextureSize);

    GLint maxTextureUnitsInFragmentShader;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnitsInFragmentShader);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture units in fragment shader %d\n", maxTextureUnitsInFragmentShader);
    assert(maxTextureUnitsInFragmentShader >= (IMapRenderer::TileLayerId::IdsCount - IMapRenderer::RasterMap));

    GLint maxTextureUnitsInVertexShader;
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxTextureUnitsInVertexShader);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal texture units in vertex shader %d\n", maxTextureUnitsInVertexShader);
    assert(maxTextureUnitsInVertexShader >= IMapRenderer::RasterMap);

    GLint maxUniformsPerProgram;
    glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &maxUniformsPerProgram);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 maximal parameter variables per program %d\n", maxUniformsPerProgram);

    const auto glesExtensions = QString::fromLatin1(glGetString(GL_EXTENSIONS));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGLES2 extensions: %s\n", qPrintable(glesExtensions));
    assert(glesExtensions.contains(QString::fromLatin1("OES_vertex_array_object")));
    assert(glesExtensions.contains(QString::fromLatin1("OES_texture_float")));
    
    //assert(glesExtensions.contains(QString::fromLatin1("OES_vertex_array_object")));

    /*NOT SUPPORTED in ES2.0
    // Bitmap (Atlas)
    glGenSamplers(1, &_textureSampler_Bitmap_Atlas);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_Bitmap_Atlas, GL_TEXTURE_WRAP_S, GL_CLAMP);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_Bitmap_Atlas, GL_TEXTURE_WRAP_T, GL_CLAMP);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_Bitmap_Atlas, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_Bitmap_Atlas, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK_RESULT;

    // ElevationData (Atlas)
    glGenSamplers(1, &_textureSampler_ElevationData_Atlas);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_ElevationData_Atlas, GL_TEXTURE_WRAP_S, GL_CLAMP);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_ElevationData_Atlas, GL_TEXTURE_WRAP_T, GL_CLAMP);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_ElevationData_Atlas, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_ElevationData_Atlas, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GL_CHECK_RESULT;

    // Bitmap (No atlas)
    glGenSamplers(1, &_textureSampler_Bitmap_NoAtlas);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_Bitmap_NoAtlas, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_Bitmap_NoAtlas, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_Bitmap_NoAtlas, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_Bitmap_NoAtlas, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK_RESULT;
    
    // ElevationData (No atlas)
    glGenSamplers(1, &_textureSampler_ElevationData_NoAtlas);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_ElevationData_NoAtlas, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_ElevationData_NoAtlas, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_ElevationData_NoAtlas, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL_CHECK_RESULT;
    glSamplerParameteri(_textureSampler_ElevationData_NoAtlas, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GL_CHECK_RESULT;
*/
    //NOT in ES
    //glShadeModel(GL_SMOOTH);
    //GL_CHECK_RESULT;
/*
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    GL_CHECK_RESULT;
    glEnable(GL_POLYGON_SMOOTH);
    GL_CHECK_RESULT;

    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    GL_CHECK_RESULT;
    glEnable(GL_LINE_SMOOTH);
    GL_CHECK_RESULT;

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    GL_CHECK_RESULT;

    glClearDepth(1.0f);
    GL_CHECK_RESULT;

    glEnable(GL_DEPTH_TEST);
    GL_CHECK_RESULT;

    glDepthFunc(GL_LEQUAL);
    GL_CHECK_RESULT;*/
}

void OsmAnd::MapRenderer_OpenGLES2::releaseRendering()
{
    /*
    if(_textureSampler_Bitmap_NoAtlas != 0)
    {
        glDeleteSamplers(1, &_textureSampler_Bitmap_NoAtlas);
        GL_CHECK_RESULT;
        _textureSampler_Bitmap_NoAtlas = 0;
    }

    if(_textureSampler_Bitmap_Atlas != 0)
    {
        glDeleteSamplers(1, &_textureSampler_Bitmap_Atlas);
        GL_CHECK_RESULT;
        _textureSampler_Bitmap_Atlas = 0;
    }

    if(_textureSampler_ElevationData_NoAtlas != 0)
    {
        glDeleteSamplers(1, &_textureSampler_ElevationData_NoAtlas);
        GL_CHECK_RESULT;
        _textureSampler_ElevationData_NoAtlas = 0;
    }

    if(_textureSampler_ElevationData_Atlas != 0)
    {
        glDeleteSamplers(1, &_textureSampler_ElevationData_Atlas);
        GL_CHECK_RESULT;
        _textureSampler_ElevationData_Atlas = 0;
    }
*/
    MapRenderer_BaseOpenGL::releaseRendering();
}
