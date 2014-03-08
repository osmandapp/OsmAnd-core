#ifndef _OSMAND_CORE_MAP_TYPES_H_
#define _OSMAND_CORE_MAP_TYPES_H_

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

    enum class MapAnimatorTimingFunction
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
    };
}

#endif // !defined(_OSMAND_CORE_MAP_TYPES_H_)
