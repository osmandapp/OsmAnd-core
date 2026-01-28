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
        glm::vec3 color;
    };

    struct Buildings3D
    {
        QVector<BuildingVertex> vertices;
        QVector<uint16_t> indices;
        QVector<uint64_t> buildingIDs;
        QVector<int> vertexCounts;
        QVector<int> indexCounts;
        QVector<BuildingVertex> outlineVertices;
        QVector<int> outlineVertexCounts;
        QVector<uint16_t> outlineIndices;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_3D_OBJECTS_H_)
