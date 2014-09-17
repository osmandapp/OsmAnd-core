#ifndef _OSMAND_CORE_MAP_COMMON_TYPES_H_
#define _OSMAND_CORE_MAP_COMMON_TYPES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    enum class MapFoundationType
    {
        Undefined = -1,

        Mixed,
        FullLand,
        FullWater,
    };

    enum class AlphaChannelData
    {
        Present,
        NotPresent,
        Undefined
    };

    typedef int MapSymbolIntersectionClassId;

    enum class MapStyleRulesetType
    {
        Invalid = -1,

        Point,
        Polyline,
        Polygon,
        Text,
        Order,

        __LAST
    };
    enum : unsigned int {
        MapStyleRulesetTypesCount = static_cast<unsigned int>(MapStyleRulesetType::__LAST)
    };

    enum class MapStyleValueDataType
    {
        Boolean,
        Integer,
        Float,
        String,
        Color,
    };

    enum class RasterMapLayerId
    {
        Invalid = -1,

        BaseLayer,
        Overlay0,
        Overlay1,

        __LAST,
    };
    enum : unsigned int {
        RasterMapLayersCount = static_cast<unsigned int>(RasterMapLayerId::__LAST)
    };
}

#endif // !defined(_OSMAND_CORE_MAP_COMMON_TYPES_H_)
