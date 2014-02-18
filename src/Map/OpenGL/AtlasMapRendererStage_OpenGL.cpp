#include "AtlasMapRendererStage_OpenGL.h"

#include "AtlasMapRenderer_OpenGL.h"

OsmAnd::AtlasMapRendererStage_OpenGL::AtlasMapRendererStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer_)
    : MapRendererStage(renderer_)
    , internalState(renderer_->_internalState)
{
}

OsmAnd::AtlasMapRendererStage_OpenGL::~AtlasMapRendererStage_OpenGL()
{
}

OsmAnd::AtlasMapRenderer_OpenGL* OsmAnd::AtlasMapRendererStage_OpenGL::getRenderer() const
{
    return static_cast<AtlasMapRenderer_OpenGL*>(renderer);
}

OsmAnd::GPUAPI_OpenGL* OsmAnd::AtlasMapRendererStage_OpenGL::getGPUAPI() const
{
    return static_cast<GPUAPI_OpenGL*>(gpuAPI.get());
}
