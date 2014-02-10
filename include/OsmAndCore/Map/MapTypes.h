#ifndef _OSMAND_CORE_MAP_TYPES_H_
#define _OSMAND_CORE_MAP_TYPES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    STRONG_ENUM(MapFoundationType)
    {
        Undefined = -1,

        Mixed,
        FullLand,
        FullWater,
    };

    STRONG_ENUM(AlphaChannelData)
    {
        Present,
        NotPresent,
        Undefined
    };
}

#endif // !defined(_OSMAND_CORE_MAP_TYPES_H_)
