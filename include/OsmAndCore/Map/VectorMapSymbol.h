#ifndef _OSMAND_CORE_VECTOR_MAP_SYMBOL_H_
#define _OSMAND_CORE_VECTOR_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <QVector>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API VectorMapSymbol : public MapSymbol
    {
        Q_DISABLE_COPY_AND_MOVE(VectorMapSymbol);

    public:
#pragma pack(push, 1)
        struct Vertex
        {
            // XY coordinates: Y up, X right
            float positionXY[2];

            // Color
            FColorARGB color;
        };
#pragma pack(pop)
        typedef uint16_t Index;

        struct VerticesAndIndexes
        {
        public:
            VerticesAndIndexes();
            ~VerticesAndIndexes();
        public:
            PointI* position31;
            
            Vertex* vertices;
            unsigned int verticesCount;
            
            Index* indices;
            unsigned int indicesCount;
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
        std::shared_ptr<VerticesAndIndexes> _verticesAndIndexes;
    protected:
        mutable QReadWriteLock _lock;

    protected:
        VectorMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group);
    public:
        virtual ~VectorMapSymbol();
        
        const std::shared_ptr<VerticesAndIndexes> getVerticesAndIndexes() const;
        void setVerticesAndIndexes(const std::shared_ptr<VerticesAndIndexes>& verticesAndIndexes);

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

    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_MAP_SYMBOL_H_)
