#include "Model3D.h"

OsmAnd::Model3D::Model3D(const QVector<Vertex>& vertices_, const QVector<Material>& materials_, const BBox& bbox_,
    const FColorARGB& mainColor_ /* = FColorARGB() */)
    : vertices(vertices_)
    , materials(materials_)
    , bbox(bbox_)
    , mainColor(mainColor_)
{
}

OsmAnd::Model3D::~Model3D()
{
}