#ifndef _OSMAND_CORE_MODEL_3D_H_
#define _OSMAND_CORE_MODEL_3D_H_

#include "QtExtensions.h"
#include "Color.h"
#include "PointsAndAreas.h"

#include <QVector>
#include <QHash>
#include <QString>

namespace OsmAnd
{
    class Model3D
    {
    public:
        struct VertexInfo
        {
            float xyz[3];
            float rgba[4];
            QString materialName;
        };

        struct BBox
        {
            float minX, maxX;
            float minY, maxY;
            float minZ, maxZ;
        };
    private:
        QVector<VertexInfo> _vertexInfos;
        QHash<QString, FColorRGBA> _customMaterialColors;

        BBox _bbox;
    public:
#pragma pack(push, 1)
        struct Vertex {
            float xyz[3];
            float rgba[4];
        };
#pragma pack(pop)

        Model3D(const QVector<VertexInfo>& vertexInfos, const BBox& bbox);
        virtual ~Model3D();

        void setCustomMaterialColors(const QHash<QString, FColorRGBA>& customMaterialColors);
        void useDefaultMaterialColors();
        
        const int getVerticesCount() const;
        const QVector<Model3D::Vertex> getVertices() const;

        const QList<PointF> getHorizontalBBox() const;
        const BBox getBBox() const;
    };
}

#endif // !defined(_OSMAND_CORE_MODEL_3D_H_)