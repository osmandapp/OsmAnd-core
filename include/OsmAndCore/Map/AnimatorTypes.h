#ifndef _OSMAND_CORE_ANIMATOR_TYPES_H_
#define _OSMAND_CORE_ANIMATOR_TYPES_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

namespace OsmAnd
{
    namespace Animator
    {
        typedef const void* Key;

        enum class TimingFunction
        {
            Invalid = -1,

            Linear,
            Victor_ReverseExponentialZoomOut,
            Victor_ReverseExponentialZoomIn,

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

        enum class AnimatedValue
        {
            Unknown = -1,

            Zoom,
            Azimuth,
            ElevationAngle,
            Target,
            PrimaryPixel,
            SecondaryPixel
        };
    }
}

#endif // !defined(_OSMAND_CORE_ANIMATOR_TYPES_H_)
