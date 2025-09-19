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

    static const int64_t WeatherSourceHoursAlignment[] = {/*GFS*/ 1, /*ECMWF*/ 3};
    static const int64_t HOUR_IN_MILLISECONDS = 60 * 60 * 1000;

    inline int64_t AlignDateTime(int64_t dateTime, WeatherSource source)
    {
        if (source == WeatherSource::Undefined)
        {
            return dateTime;
        }

        const int64_t period = WeatherSourceHoursAlignment[(int)source] * HOUR_IN_MILLISECONDS;
        const int64_t alignedDateTime = (dateTime / period) * period;
        return alignedDateTime;
    }
}

#endif // !defined(_OSMAND_CORE_WEATHER_COMMON_TYPES_H_)
