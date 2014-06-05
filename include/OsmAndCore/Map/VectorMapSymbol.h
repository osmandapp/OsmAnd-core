#ifndef _OSMAND_CORE_VECTOR_MAP_SYMBOL_H_
#define _OSMAND_CORE_VECTOR_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbol.h>
#include <OsmAndCore/Map/IPositionedMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API VectorMapSymbol
        : public MapSymbol
        , public IPositionedMapSymbol
    {
        Q_DISABLE_COPY(VectorMapSymbol);

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
            LineLoop
        };

    private:
    protected:
        VectorMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable,
            const int order,
            const IntersectionModeFlags intersectionModeFlags);
    public:
        virtual ~VectorMapSymbol();

        Vertex* vertices;
        unsigned int verticesCount;

        Index* indices;
        unsigned int indicesCount;

        PrimitiveType primitiveType;

        void releaseVerticesAndIndices();

        PointI position31;

        virtual PointI getPosition31() const;
        virtual void setPosition31(const PointI position);
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_MAP_SYMBOL_H_)
