#include "AtlasMapRenderer_OpenGL.h"

#include "OpenGL/GPUAPI_OpenGL.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::AtlasMapRenderer_OpenGL::AtlasMapRenderer_OpenGL()
{
}

OsmAnd::AtlasMapRenderer_OpenGL::~AtlasMapRenderer_OpenGL()
{
}

OsmAnd::GPUAPI* OsmAnd::AtlasMapRenderer_OpenGL::allocateGPUAPI()
{
    auto api = new GPUAPI_OpenGL();
    api->initialize();
    return api;
}
