#ifndef _OSMAND_CORE_CIRCLE_PRIMITIVE_MAP_SYMBOL_H_
#define _OSMAND_CORE_CIRCLE_PRIMITIVE_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/PrimitiveMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API CirclePrimitiveMapSymbol : public PrimitiveMapSymbol
    {
        Q_DISABLE_COPY(CirclePrimitiveMapSymbol);

    private:
    protected:
    public:
        CirclePrimitiveMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable,
            const int order,
            const IntersectionModeFlags intersectionModeFlags);
        virtual ~CirclePrimitiveMapSymbol();

        void generate(
            const FColorARGB color = FColorARGB(1.0f, 1.0f, 1.0f, 1.0f),
            const unsigned int pointsCount = 360,
            float radius = 1.0f);
    };
}

#endif // !defined(_OSMAND_CORE_CIRCLE_PRIMITIVE_MAP_SYMBOL_H_)
