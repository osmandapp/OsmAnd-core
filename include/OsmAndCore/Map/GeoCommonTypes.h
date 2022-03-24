#ifndef _OSMAND_CORE_GEO_COMMON_TYPES_H_
#define _OSMAND_CORE_GEO_COMMON_TYPES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    typedef uint8_t BandIndex;
    
    enum class WeatherBand
    {
        Undefined = 0,
        Cloud = 1,
        Temperature = 2,
        Pressure = 3,
        WindSpeed = 4,
        Precipitation = 5
    };
}

#endif // !defined(_OSMAND_CORE_GEO_COMMON_TYPES_H_)
