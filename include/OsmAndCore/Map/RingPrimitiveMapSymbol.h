#ifndef _OSMAND_CORE_RING_PRIMITIVE_MAP_SYMBOL_H_
#define _OSMAND_CORE_RING_PRIMITIVE_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/PrimitiveMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API RingPrimitiveMapSymbol : public PrimitiveMapSymbol
    {
        Q_DISABLE_COPY(RingPrimitiveMapSymbol);

    private:
    protected:
    public:
        RingPrimitiveMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable,
            const int order,
            const IntersectionModeFlags intersectionModeFlags);
        virtual ~RingPrimitiveMapSymbol();

        void generateAsLine(
            const FColorARGB color = FColorARGB(1.0f, 1.0f, 1.0f, 1.0f),
            const unsigned int pointsCount = 360,
            float radius = 1.0f);
    };
}

#endif // !defined(_OSMAND_CORE_RING_PRIMITIVE_MAP_SYMBOL_H_)
