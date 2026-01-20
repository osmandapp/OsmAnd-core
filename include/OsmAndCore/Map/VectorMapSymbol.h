#ifndef _OSMAND_CORE_VECTOR_MAP_SYMBOL_H_
#define _OSMAND_CORE_VECTOR_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <QVector>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbol.h>
#include <OsmAndCore/Map/Model3D.h>

namespace OsmAnd
{
    class OSMAND_CORE_API VectorMapSymbol : public MapSymbol
    {
        Q_DISABLE_COPY_AND_MOVE(VectorMapSymbol);

    public:
#pragma pack(push, 1)
        struct Vertex
        {
            // Coordinates
            float positionXYZD[4];

            // Color
            FColorARGB color;
        };
#pragma pack(pop)
        typedef uint16_t Index;
#pragma pack(push, 1)
        struct VertexWithNormals
        {
            // Position coordinates
            float positionXYZ[3];

            // Normal coordinates
            float normalXYZ[3];

            // Color
            FColorARGB color;
        };
#pragma pack(pop)

        struct VerticesAndIndices
        {
        public:
            VerticesAndIndices();
            ~VerticesAndIndices();
        public:
            PointI* position31;
            
            Vertex* vertices;
            VertexWithNormals* verticesWithNormals;
            unsigned int verticesCount;
            
            Index* indices;
            unsigned int indicesCount;

            std::shared_ptr<std::vector<std::pair<TileId, int32_t>>> partSizes;
            ZoomLevel zoomLevel;
            bool isDenseObject;
            bool isSeenThrough;
        };
        
        enum class PrimitiveType
        {
            Invalid = -1,

            TriangleFan,
            TriangleStrip,
            LineLoop,
            Triangles
        };

        enum class ScaleType
        {
            Raw,
            In31,
            InMeters
        };

    private:
        std::shared_ptr<VerticesAndIndices> _verticesAndIndices;
    protected:
        mutable QReadWriteLock _lock;

    protected:
        VectorMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group);
    public:
        virtual ~VectorMapSymbol();
        
        static const float _absentElevation;

        const std::shared_ptr<VerticesAndIndices> getVerticesAndIndices() const;
        void setVerticesAndIndices(const std::shared_ptr<VerticesAndIndices>& verticesAndIndices);

        PrimitiveType primitiveType;

        ScaleType scaleType;
        double scale;

        void releaseVerticesAndIndices();

        static void generateCirclePrimitive(
            VectorMapSymbol& mapSymbol,
            const FColorARGB color = FColorARGB(1.0f, 1.0f, 1.0f, 1.0f),
            const unsigned int pointsCount = 360,
            float radius = 1.0f);

        static void generateRingLinePrimitive(
            VectorMapSymbol& mapSymbol,
            const FColorARGB color = FColorARGB(1.0f, 1.0f, 1.0f, 1.0f),
            const unsigned int pointsCount = 360,
            float radius = 1.0f);

        static void generateModel3DPrimitive(
            VectorMapSymbol& mapSymbol,
            const std::shared_ptr<const Model3D>& model3D,
            const QHash<QString, FColorARGB>& customMaterialColors);
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_MAP_SYMBOL_H_)
