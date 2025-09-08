#ifndef _OSMAND_CORE_WEATHER_COMMON_TYPES_H_
#define _OSMAND_CORE_WEATHER_COMMON_TYPES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    enum class WeatherLayer
    {
        Undefined = -1,
        
        Low = 0,
        High = 1,
    };

    enum class WeatherType
    {
        Undefined = -1,
        
        Raster = 0,
        Contour = 1,
    };

    enum class WeatherSource
    {
        Undefined = -1,
        
        GFS = 0,
        ECMWF = 1,
    };
}

#endif // !defined(_OSMAND_CORE_WEATHER_COMMON_TYPES_H_)
