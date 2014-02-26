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
    } STRONG_ENUM_TERMINATOR;

    STRONG_ENUM(AlphaChannelData)
    {
        Present,
        NotPresent,
        Undefined
    } STRONG_ENUM_TERMINATOR;

    STRONG_ENUM(MapAnimatorEasingType)
    {
        None = -1,

        Linear = 0,
        Quadratic,
        Cubic,
        Quartic,
        Quintic,
        Sinusoidal,
        Exponential,
        Circular
    } STRONG_ENUM_TERMINATOR;
}

#endif // !defined(_OSMAND_CORE_MAP_TYPES_H_)
