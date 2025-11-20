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
        glm::vec3 normal;
    };

    struct Buildings3D
    {
        QVector<BuildingVertex> vertices;
        QVector<uint16_t> indices;
        QVector<FColorARGB> colors;
        QVector<uint64_t> ids;
        QVector<int> vertexCounts;
        QVector<int> indexCounts;
        QVector<int> vertexOffsets;
        QVector<int> indexOffsets;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_H_)
