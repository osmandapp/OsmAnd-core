#include "IMapRenderer.h"

OsmAnd::IMapRenderer::IMapRenderer()
{
}

OsmAnd::IMapRenderer::~IMapRenderer()
{
}

#if defined(OSMAND_OPENGL_RENDERERS_SUPPORTED)
#   include "OpenGL/AtlasMapRenderer_OpenGL.h"
#   if defined(OSMAND_OPENGL2PLUS_RENDERER_SUPPORTED)
#       include "OpenGL/OpenGL2plus/GPUAPI_OpenGL2plus.h"
#   endif // defined(OSMAND_OPENGL2PLUS_RENDERER_SUPPORTED)
#   if defined(OSMAND_OPENGLES2_RENDERER_SUPPORTED)
#       include "OpenGL/OpenGLES2/GPUAPI_OpenGLES2.h"
#   endif // defined(OSMAND_OPENGLES2_RENDERER_SUPPORTED)
#endif // defined(OSMAND_OPENGL_RENDERERS_SUPPORTED)

OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL OsmAnd::createMapRenderer(
    const MapRendererClass mapRendererClass)
{
    switch (mapRendererClass)
    {
#if defined(OSMAND_OPENGL2PLUS_RENDERER_SUPPORTED)
    case MapRendererClass::AtlasMapRenderer_OpenGL2plus:
        return std::shared_ptr<OsmAnd::IMapRenderer>(new AtlasMapRenderer_OpenGL(new GPUAPI_OpenGL2plus()));
#endif // defined(OSMAND_OPENGL2PLUS_RENDERER_SUPPORTED)
#if defined(OSMAND_OPENGLES2_RENDERER_SUPPORTED)
    case MapRendererClass::AtlasMapRenderer_OpenGLES2:
        return std::shared_ptr<OsmAnd::IMapRenderer>(new AtlasMapRenderer_OpenGL(new GPUAPI_OpenGLES2()));
#endif // defined(OSMAND_OPENGLES2_RENDERER_SUPPORTED)
    default:
        return std::shared_ptr<OsmAnd::IMapRenderer>();
    }
}
