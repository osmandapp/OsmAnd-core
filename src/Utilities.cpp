#include "Utilities.h"

#include <stdint.h>
#include <limits>
#include <cmath>

OSMAND_CORE_API const double OsmAnd::Utilities::pi = 3.14159265358979323846;
const int64_t l = 1UL << 31;

OSMAND_CORE_API int OSMAND_CORE_CALL OsmAnd::Utilities::get31TileNumberX( double longitude )
{
    longitude = checkLongitude(longitude);
    return (int)((longitude + 180)/360 * l);
}

OSMAND_CORE_API int OSMAND_CORE_CALL OsmAnd::Utilities::get31TileNumberY( double latitude )
{
    latitude = checkLatitude(latitude);
    double eval = log( tan(toRadians(latitude)) + 1.0/cos(toRadians(latitude)) );
    if(eval > pi)
        eval = pi;
    return  (int) ((1. - eval / pi) / 2. * l);
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::get31LongitudeX( int x )
{
    return getLongitudeFromTile(21, (double)x / 1024.);
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::get31LatitudeY( int y )
{
    return getLatitudeFromTile(21, (double)y / 1024.);
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getTileNumberX( float zoom, double longitude )
{
    if( fabs(longitude - 180.) < std::numeric_limits<double>::epsilon() )
        return getPowZoom(zoom) - 1;

    longitude = checkLongitude(longitude);
    return (longitude + 180.)/360. * getPowZoom(zoom);
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getTileNumberY( float zoom, double latitude )
{
    latitude = checkLatitude(latitude);
    double eval = log( tan(toRadians(latitude)) + 1/cos(toRadians(latitude)) );
#if defined(_MSC_VER)
    if (_finite(eval) || _isnan(eval))
#elif defined(__ANDROID__)
	if (isfinite(eval) || isnan(eval))
#else
    if (std::isfinite(eval) || std::isnan(eval))
#endif
    {
        latitude = latitude < 0 ? - 89.9 : 89.9;
        eval = log( tan(toRadians(latitude)) + 1/cos(toRadians(latitude)) );
    }
    double result = (1 - eval / pi) / 2 * getPowZoom(zoom);
    return  result;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::checkLatitude( double latitude )
{
    while (latitude < -90. || latitude > 90.) {
        if (latitude < 0.) {
            latitude += 180.;
        } else {
            latitude -= 180.;
        }
    }
    if(latitude < -85.0511) {
        return -85.0511;
    } else if(latitude > 85.0511){
        return 85.0511;
    }
    return latitude;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::checkLongitude( double longitude )
{
    while (longitude < -180.0 || longitude > 180.0) {
        if (longitude < 0.0) {
            longitude += 360.0;
        } else {
            longitude -= 360.0;
        }
    }
    return longitude;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::toRadians( double angle )
{
    return angle / 180.0 * pi;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getPowZoom( float zoom )
{
    if(zoom >= 0 && zoom - floor(zoom) < 0.05f){
        return 1 << ((int)zoom); 
    } else {
        return pow(2, zoom);
    }
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getLongitudeFromTile( float zoom, double x )
{
    return x / getPowZoom(zoom) * 360.0 - 180.0;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getLatitudeFromTile( float zoom, double y )
{
    int sign = y < 0 ? -1 : 1;
    double result = atan(sign * sinh(pi * (1 - 2 * y / getPowZoom(zoom)))) * 180. / pi;
    return result;
}