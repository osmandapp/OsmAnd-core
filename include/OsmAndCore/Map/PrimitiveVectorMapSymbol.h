#ifndef _OSMAND_CORE_PRIMITIVE_VECTOR_MAP_SYMBOL_H_
#define _OSMAND_CORE_PRIMITIVE_VECTOR_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/VectorMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API PrimitiveVectorMapSymbol : public VectorMapSymbol
    {
        Q_DISABLE_COPY(PrimitiveVectorMapSymbol);

    private:
    protected:
    public:
        PrimitiveVectorMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable,
            const int order,
            const IntersectionModeFlags intersectionModeFlags);
        virtual ~PrimitiveVectorMapSymbol();

        void generateCircle(
            const FColorARGB color = FColorARGB(1.0f, 1.0f, 1.0f, 1.0f),
            const unsigned int pointsCount = 360,
            float radius = 1.0f);

        void generateRingLine(
            const FColorARGB color = FColorARGB(1.0f, 1.0f, 1.0f, 1.0f),
            const unsigned int pointsCount = 360,
            float radius = 1.0f);
    };
}

#endif // !defined(_OSMAND_CORE_PRIMITIVE_VECTOR_MAP_SYMBOL_H_)
