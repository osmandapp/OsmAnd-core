#include "AtlasMapRendererStageHelper_OpenGL.h"

#include "AtlasMapRenderer_OpenGL.h"
#include "AtlasMapRendererStage.h"
#include "AtlasMapRendererInternalState_OpenGL.h"
#include "OpenGL/GPUAPI_OpenGL.h"

OsmAnd::AtlasMapRendererStageHelper_OpenGL::AtlasMapRendererStageHelper_OpenGL(const AtlasMapRendererStage* const stage_)
    : _stage(stage_)
{
}

OsmAnd::AtlasMapRendererStageHelper_OpenGL::~AtlasMapRendererStageHelper_OpenGL()
{
}

OsmAnd::AtlasMapRenderer_OpenGL* OsmAnd::AtlasMapRendererStageHelper_OpenGL::getRenderer() const
{
    return static_cast<AtlasMapRenderer_OpenGL*>(_stage->renderer);
}

OsmAnd::GPUAPI_OpenGL* OsmAnd::AtlasMapRendererStageHelper_OpenGL::getGPUAPI() const
{
    return static_cast<GPUAPI_OpenGL*>(_stage->gpuAPI.get());
}
