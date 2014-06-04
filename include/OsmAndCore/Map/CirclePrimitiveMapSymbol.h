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

        //void set
    };
}

#endif // !defined(_OSMAND_CORE_CIRCLE_PRIMITIVE_MAP_SYMBOL_H_)
