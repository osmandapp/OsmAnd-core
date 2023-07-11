#include "Model3D.h"

#include <OsmAndCore/Nullable.h>

OsmAnd::Model3D::Model3D(const QVector<VertexInfo>& vertexInfos_, const BBox& bbox_)
    : _vertexInfos(vertexInfos_)
    , _bbox(bbox_)
{
}

OsmAnd::Model3D::~Model3D()
{
}

void OsmAnd::Model3D::setCustomMaterialColors(const QHash<QString, FColorRGBA>& customMaterialColors)
{
    _customMaterialColors = customMaterialColors;
}

void OsmAnd::Model3D::useDefaultMaterialColors()
{
    _customMaterialColors = QHash<QString, FColorRGBA>();
}

int OsmAnd::Model3D::getVerticesCount() const
{
    return _vertexInfos.size();
}

QVector<OsmAnd::Model3D::Vertex> OsmAnd::Model3D::getVertices() const
{
    QVector<Vertex> vertices;

    for (const auto& vertexInfo : _vertexInfos)
    {
        Vertex vertex;

        vertex.xyz[0] = vertexInfo.xyz[0];
        vertex.xyz[1] = vertexInfo.xyz[1];
        vertex.xyz[2] = vertexInfo.xyz[2];

        // Check if custom color is set for material of current vertex
        Nullable<FColorRGBA> customColor;
        if (!vertexInfo.materialName.isEmpty() && !_customMaterialColors.isEmpty())
        {
            const auto citColors = _customMaterialColors.constFind(vertexInfo.materialName);
            if (citColors != _customMaterialColors.cend())
                customColor = *citColors;
        }

        if (customColor.isSet())
        {
            vertex.rgba[0] = customColor->r;
            vertex.rgba[1] = customColor->g;
            vertex.rgba[2] = customColor->b;
            vertex.rgba[3] = customColor->a;
        }
        else
        {
            vertex.rgba[0] = vertexInfo.rgba[0];
            vertex.rgba[1] = vertexInfo.rgba[1];
            vertex.rgba[2] = vertexInfo.rgba[2];
            vertex.rgba[3] = vertexInfo.rgba[3];
        }

        vertices.push_back(vertex);
    }

    return vertices;
}

OsmAnd::Model3D::BBox OsmAnd::Model3D::getBBox() const
{
    return _bbox;
}