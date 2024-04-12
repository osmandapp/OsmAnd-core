#include "Model3D.h"

OsmAnd::Model3D::Model3D(const QVector<Vertex>& vertices_, const QVector<Material>& materials_, const BBox& bbox_)
    : vertices(vertices_)
    , materials(materials_)
    , bbox(bbox_)
{
}

OsmAnd::Model3D::Model3D(const QString& error_)
    : error(error_)
    , bbox(BBox())
{
}

OsmAnd::Model3D::~Model3D()
{
}