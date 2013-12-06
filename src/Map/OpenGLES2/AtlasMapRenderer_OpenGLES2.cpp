#include "AtlasMapRenderer_OpenGLES2.h"

#include "OpenGLES2/GPUAPI_OpenGLES2.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::AtlasMapRenderer_OpenGLES2::AtlasMapRenderer_OpenGLES2()
{
}

OsmAnd::AtlasMapRenderer_OpenGLES2::~AtlasMapRenderer_OpenGLES2()
{
}

OsmAnd::GPUAPI* OsmAnd::AtlasMapRenderer_OpenGLES2::allocateGPUAPI()
{
    auto api = new GPUAPI_OpenGLES2();
    api->initialize();
    return api;
}
