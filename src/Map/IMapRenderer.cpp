#include "IMapRenderer.h"

OsmAnd::IMapRenderer::IMapRenderer()
    : _isRenderingInitialized(false)
    , _frameInvalidated(false)
    , configuration(_configuration)
    , isRenderingInitialized(_isRenderingInitialized)
    , state(_requestedState)
    , frameInvalidated(_frameInvalidated)
    , visibleTiles(_visibleTiles)
{
}

OsmAnd::IMapRenderer::~IMapRenderer()
{
}

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
#   include "OpenGL/AtlasMapRenderer_OpenGL.h"
OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL OsmAnd::createAtlasMapRenderer_OpenGL()
{
    return std::shared_ptr<OsmAnd::IMapRenderer>(new AtlasMapRenderer_OpenGL());
}
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED

#if defined(OSMAND_OPENGLES2_RENDERER_SUPPORTED)
#   include "OpenGLES2/AtlasMapRenderer_OpenGLES2.h"
OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL OsmAnd::createAtlasMapRenderer_OpenGLES2()
{
    return std::shared_ptr<OsmAnd::IMapRenderer>(new AtlasMapRenderer_OpenGLES2());
}
#endif // OSMAND_OPENGLES2_RENDERER_SUPPORTED
