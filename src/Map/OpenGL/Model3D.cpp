#include "Model3D.h"

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

const int OsmAnd::Model3D::getVerticesCount() const
{
    return _vertexInfos.size();
}

const QVector<OsmAnd::Model3D::Vertex> OsmAnd::Model3D::getVertices() const
{
    QVector<Vertex> vertices;

    for (auto vertexInfo : _vertexInfos)
    {
        Vertex vertex = Vertex {};

        vertex.xyz[0] = vertexInfo.xyz[0];
        vertex.xyz[1] = vertexInfo.xyz[1];
        vertex.xyz[2] = vertexInfo.xyz[2];

        const FColorRGBA* pCustomColor = nullptr;
        if (!vertexInfo.materialName.isEmpty() && !_customMaterialColors.isEmpty())
        {
            const auto& colorsIt = _customMaterialColors.constFind(vertexInfo.materialName);
            if (colorsIt != _customMaterialColors.cend())
                pCustomColor = &colorsIt.value();
        }

        if (pCustomColor)
        {
            vertex.rgba[0] = pCustomColor->r;
            vertex.rgba[1] = pCustomColor->g;
            vertex.rgba[2] = pCustomColor->b;
            vertex.rgba[3] = pCustomColor->a;
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

const QList<OsmAnd::PointF> OsmAnd::Model3D::getHorizontalBBox() const
{
    QList<PointF> horizontalBBox;
    horizontalBBox.push_back(PointF(_bbox.minX, _bbox.minZ));
    horizontalBBox.push_back(PointF(_bbox.minX, _bbox.maxZ));
    horizontalBBox.push_back(PointF(_bbox.maxX, _bbox.minZ));
    horizontalBBox.push_back(PointF(_bbox.maxX, _bbox.maxZ));
    return horizontalBBox;
}

const OsmAnd::Model3D::BBox OsmAnd::Model3D::getBBox() const
{
    return _bbox;
}