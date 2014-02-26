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

    STRONG_ENUM(MapAnimatorTimingFunction)
    {
        Invalid = -1,

        Linear = 0,

#define _DECLARE_TIMING_FUNCTION(name)                                                                  \
    EaseIn##name,                                                                                       \
    EaseOut##name,                                                                                      \
    EaseInOut##name,                                                                                    \
    EaseOutIn##name

        _DECLARE_TIMING_FUNCTION(Quadratic),
        _DECLARE_TIMING_FUNCTION(Cubic),
        _DECLARE_TIMING_FUNCTION(Quartic),
        _DECLARE_TIMING_FUNCTION(Quintic),
        _DECLARE_TIMING_FUNCTION(Sinusoidal),
        _DECLARE_TIMING_FUNCTION(Exponential),
        _DECLARE_TIMING_FUNCTION(Circular),

#undef _DECLARE_TIMING_FUNCTION
    } STRONG_ENUM_TERMINATOR;
}

#endif // !defined(_OSMAND_CORE_MAP_TYPES_H_)
