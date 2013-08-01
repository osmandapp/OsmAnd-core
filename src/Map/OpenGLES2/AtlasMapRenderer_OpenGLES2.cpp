#include "AtlasMapRenderer_OpenGLES2.h"

#include <assert.h>

#include <QtMath>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <SkBitmap.h>

#include "IMapTileProvider.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::AtlasMapRenderer_OpenGLES2::AtlasMapRenderer_OpenGLES2()
{
}

OsmAnd::AtlasMapRenderer_OpenGLES2::~AtlasMapRenderer_OpenGLES2()
{
}

OsmAnd::RenderAPI* OsmAnd::AtlasMapRenderer_OpenGLES2::allocateRenderAPI()
{
    auto api = new RenderAPI_OpenGLES2();
    api->initialize();
    return api;
}
