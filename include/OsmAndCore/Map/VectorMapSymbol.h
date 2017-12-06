#ifndef _OSMAND_CORE_VECTOR_MAP_SYMBOL_H_
#define _OSMAND_CORE_VECTOR_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

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

        enum class PrimitiveType
        {
            Invalid = -1,

            TriangleFan,
            TriangleStrip,
            LineLoop
        };

        enum class ScaleType
        {
            Raw,
            InMeters
        };

    private:
    protected:
        VectorMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group);
    public:
        virtual ~VectorMapSymbol();

        Vertex* vertices;
        unsigned int verticesCount;

        Index* indices;
        unsigned int indicesCount;

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

        static void generateLinePrimitive(
            VectorMapSymbol& mapSymbol,
            const double lineWidth = 3.0,
            const FColorARGB color = FColorARGB(1.0f, 1.0f, 1.0f, 1.0f));
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_MAP_SYMBOL_H_)
