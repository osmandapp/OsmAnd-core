#ifndef _OSMAND_CORE_MODEL_3D_H_
#define _OSMAND_CORE_MODEL_3D_H_

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Color.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QVector>
#include <QString>
#include <OsmAndCore/restore_internal_warnings.h>

namespace OsmAnd
{
    class OSMAND_CORE_API Model3D
    {
        Q_DISABLE_COPY_AND_MOVE(Model3D);

    public:
        struct Vertex
        {
            float position[3];
            float normal[3];
            int materialIndex;
            FColorARGB color;
        };

        struct Material
        {
            FColorARGB color;
            QString name;
        };

        struct BBox
        {
            float minX, maxX;
            float minY, maxY;
            float minZ, maxZ;

            inline float lengthX() const
            {
                return maxX - minX;
            }

            inline float lengthY() const
            {
                return maxY - minY;
            }

            inline float lengthZ() const
            {
                return maxZ - minZ;
            }
        };

    private:
    protected:
    public:
        Model3D(const QVector<Vertex>& vertices, const QVector<Material>& materials, const BBox& bbox,
            const FColorARGB& mainColor = FColorARGB());
        virtual ~Model3D();
       
        const QVector<Vertex> vertices;
        const QVector<Material> materials;
        const BBox bbox;
        FColorARGB mainColor;
    };
}

#endif // !defined(_OSMAND_CORE_MODEL_3D_H_)