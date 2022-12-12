#ifndef _OSMAND_CORE_MODEL_3D_H_
#define _OSMAND_CORE_MODEL_3D_H_

#include "QtExtensions.h"
#include "Color.h"

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
    private:
        QVector<VertexInfo> _vertexInfos;
        QHash<QString, FColorRGBA> _customMaterialColors;
    public:
#pragma pack(push, 1)
        struct Vertex {
            float xyz[3];
            float rgba[4];
        };
#pragma pack(pop)

        Model3D(const QVector<VertexInfo> vertexInfos);
        virtual ~Model3D();

        void setCustomMaterialColors(const QHash<QString, FColorRGBA>& customMaterialColors);
        void useDefaultMaterialColors();
        
        const int getVerticesCount() const;
        const QVector<Model3D::Vertex> getVertices() const;
    };
}

#endif // !defined(_OSMAND_CORE_MODEL_3D_H_)