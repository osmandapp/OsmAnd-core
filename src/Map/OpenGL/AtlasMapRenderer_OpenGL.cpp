#include "AtlasMapRenderer_OpenGL.h"

#include <cassert>

#include <QtMath>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <SkBitmap.h>

#include "IMapTileProvider.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::AtlasMapRenderer_OpenGL::AtlasMapRenderer_OpenGL()
{
}

OsmAnd::AtlasMapRenderer_OpenGL::~AtlasMapRenderer_OpenGL()
{
}

OsmAnd::RenderAPI* OsmAnd::AtlasMapRenderer_OpenGL::allocateRenderAPI()
{
    auto api = new RenderAPI_OpenGL();
    api->initialize();
    return api;
}
