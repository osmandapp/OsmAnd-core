#ifndef _OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_H_
#define _OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_H_

#include <OsmAndCore/stdlib_common.h>

#include <glm/glm.hpp>

#include <QVector>

namespace OsmAnd
{
    struct BuildingVertex
    {
        glm::ivec2 location31;
        float height;
        glm::vec3 normal;
    };

    struct Building3D
    {
        QVector<BuildingVertex> vertices;
        QVector<uint16_t> indices;
        FColorARGB color;
        uint64_t bboxHash;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_H_)
