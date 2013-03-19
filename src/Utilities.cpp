#include "Utilities.h"

#include <stdint.h>
#include <limits>
#include <cmath>
#include <QtNumeric>

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
    if (qIsFinite(eval) || qIsNaN(eval))
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

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::Utilities::extractFirstNumberPosition( const QString& value, int& first, int& last )
{
    first = -1;
    last = -1;
    int curPos = 0;
    for(auto itChr = value.begin(); itChr != value.end() && (first == -1 || last == -1); ++itChr, curPos++)
    {
        auto chr = *itChr;
        if(first == -1 && chr.isDigit())
            first = curPos;
        if(last == -1 && first != -1 && !chr.isDigit())
            last = curPos - 1;
    }
    if(first != -1 && last == -1)
        last = value.length() - 1;
    return first != -1;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::parseSpeed( const QString& value, double defValue )
{
    if(value == "none")
        return std::numeric_limits<double>::max();
    
    int first, last;
    if(!extractFirstNumberPosition(value, first, last))
        return defValue;
    bool ok;
    auto result = value.mid(first, last - first + 1).toDouble(&ok);
    if(!ok)
        return defValue;
    result /= 3.6;
    if(value.contains("mph"))
        result *= 1.6;
    return result;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::parseLength( const QString& value, double defValue )
{
    int first, last;
    if(!extractFirstNumberPosition(value, first, last))
        return defValue;
    bool ok;
    auto result = value.mid(first, last - first + 1).toDouble(&ok);
    if(!ok)
        return defValue;
    if(value.contains("ft") || value.contains('"'))
        result *= 0.3048;
    if(value.contains('\''))
    {
        auto inchesSubstr = value.mid(value.indexOf('"') + 1);
        if(!extractFirstNumberPosition(inchesSubstr, first, last))
            return defValue;
        bool ok;
        auto inches = inchesSubstr.mid(first, last - first + 1).toDouble(&ok);
        if(ok)
            result += inches * 0.0254;        
    }
    return result;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::parseWeight( const QString& value, double defValue )
{
    int first, last;
    if(!extractFirstNumberPosition(value, first, last))
        return defValue;
    bool ok;
    auto result = value.mid(first, last - first + 1).toDouble(&ok);
    if(!ok)
        return defValue;
    if(value.contains("lbs"))
        result = (result * 0.4535) / 1000.0; // lbs -> kg -> ton
    return result;
}

OSMAND_CORE_API int OSMAND_CORE_CALL OsmAnd::Utilities::parseArbitraryInt( const QString& value, int defValue )
{
    int first, last;
    if(!extractFirstNumberPosition(value, first, last))
        return defValue;
    bool ok;
    auto result = value.mid(first, last - first + 1).toInt(&ok);
    if(!ok)
        return defValue;
    return result;
}
