#ifndef _OSMAND_CORE_MAP_SYMBOL_H_
#define _OSMAND_CORE_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Callable.h>

namespace OsmAnd
{
    class MapSymbolsGroup;

    class OSMAND_CORE_API MapSymbol
    {
        Q_DISABLE_COPY(MapSymbol);

    public:
        enum IntersectionModeFlags : uint32_t
        {
            RegularIntersectionProcessing = 0,

            IgnoredByIntersectionTest = 1 << 0,
            TransparentForIntersectionLookup = 1 << 1,
        };

    private:
    protected:
        MapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable,
            const int order,
            const IntersectionModeFlags intersectionModeFlags);
    public:
        virtual ~MapSymbol();

        const std::weak_ptr<MapSymbolsGroup> group;
        MapSymbolsGroup* const groupPtr;

        const bool isShareable;

        const int order;
        const IntersectionModeFlags intersectionModeFlags;

        bool isHidden;
        FColorARGB modulationColor;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_SYMBOL_H_)
