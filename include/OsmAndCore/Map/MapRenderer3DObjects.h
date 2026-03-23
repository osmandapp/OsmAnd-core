#ifndef _OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_H_
#define _OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_H_

#include <OsmAndCore/stdlib_common.h>

#include <glm/glm.hpp>

#include <QVector>
#include <OsmAndCore/PointsAndAreas.h>

namespace OsmAnd
{
    struct BuildingVertex
    {
        glm::ivec2 location31;
        float height;
        float terrainHeight;
        glm::vec3 normal;
        glm::vec4 color;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_H_)
