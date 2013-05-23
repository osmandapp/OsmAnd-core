#include "Utilities.h"

#include <limits>
#include <cmath>
#include <QtNumeric>
#include <QtCore>

const uint64_t l = 1UL << 31;

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::get31TileNumberX( double longitude )
{
    longitude = checkLongitude(longitude);
    return (longitude + 180) / 360*l;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::get31TileNumberY( double latitude )
{
    latitude = checkLatitude(latitude);
    double eval = log( tan(toRadians(latitude)) + 1.0/cos(toRadians(latitude)) );
    if(eval > M_PI)
        eval = M_PI;
    return (1.0 - eval / M_PI) / 2.0*l;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::get31LongitudeX( uint32_t x )
{
    return getLongitudeFromTile(21, (double)x / 1024.);
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::get31LatitudeY( uint32_t y )
{
    return getLatitudeFromTile(21, (double)y / 1024.);
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getTileNumberX( float zoom, double longitude )
{
    if( qAbs(longitude - 180.) < std::numeric_limits<double>::epsilon() )
        return getPowZoom(zoom) - 1;

    longitude = checkLongitude(longitude);
    return (longitude + 180.)/360. * getPowZoom(zoom);
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getTileNumberY( float zoom, double latitude )
{
    latitude = checkLatitude(latitude);
    double eval = log( tan(toRadians(latitude)) + 1/cos(toRadians(latitude)) );
    if (qIsInf(eval) || qIsNaN(eval))
    {
        latitude = latitude < 0 ? - 89.9 : 89.9;
        eval = log( tan(toRadians(latitude)) + 1/cos(toRadians(latitude)) );
    }
    double result = (1 - eval / M_PI) / 2 * getPowZoom(zoom);
    return result;
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
    return angle / 180.0 * M_PI;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getPowZoom( float zoom )
{
    if(zoom >= 0 && zoom - floor(zoom) < 0.05f){
        return 1 << ((int)zoom); 
    } else {
        return pow(2, zoom);
    }
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getLongitudeFromTile( float zoom, uint32_t x )
{
    return x / getPowZoom(zoom) * 360.0 - 180.0;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getLatitudeFromTile( float zoom, uint32_t y )
{
    int sign = y < 0 ? -1 : 1;
    double result = atan(sign * sinh(M_PI * (1 - 2 * y / getPowZoom(zoom)))) * 180. / M_PI;
    return result;
}

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::Utilities::extractFirstNumberPosition( const QString& value, int& first, int& last, bool allowSigned, bool allowDot )
{
    first = -1;
    last = -1;
    int curPos = 0;
    for(auto itChr = value.begin(); itChr != value.end() && (first == -1 || last == -1); ++itChr, curPos++)
    {
        auto chr = *itChr;
        if(first == -1 && chr.isDigit())
            first = curPos;
        if(last == -1 && first != -1 && !chr.isDigit() && ((allowDot && chr != '.') || !allowDot))
            last = curPos - 1;
    }
    if(first >= 1 && allowSigned && value[first - 1] == '-')
        first -= 1;
    if(first != -1 && last == -1)
        last = value.length() - 1;
    return first != -1;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::parseSpeed( const QString& value, double defValue, bool* wasParsed/* = nullptr*/ )
{
    if(value == "none")
        return std::numeric_limits<double>::max();
    
    int first, last;
    if(!extractFirstNumberPosition(value, first, last, false, true))
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

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::parseLength( const QString& value, double defValue, bool* wasParsed/* = nullptr*/ )
{
    int first, last;
    if(!extractFirstNumberPosition(value, first, last, false, true))
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
        if(!extractFirstNumberPosition(inchesSubstr, first, last, false, true))
            return defValue;
        bool ok;
        auto inches = inchesSubstr.mid(first, last - first + 1).toDouble(&ok);
        if(ok)
            result += inches * 0.0254;        
    }
    return result;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::parseWeight( const QString& value, double defValue, bool* wasParsed/* = nullptr*/ )
{
    int first, last;
    if(wasParsed)
        *wasParsed = false;
    if(!extractFirstNumberPosition(value, first, last, false, true))
        return defValue;
    bool ok;
    auto result = value.mid(first, last - first + 1).toDouble(&ok);
    if(!ok)
        return defValue;

    if(wasParsed)
        *wasParsed = true;
    if(value.contains("lbs"))
        result = (result * 0.4535) / 1000.0; // lbs -> kg -> ton
    return result;
}

OSMAND_CORE_API int OSMAND_CORE_CALL OsmAnd::Utilities::parseArbitraryInt( const QString& value, int defValue, bool* wasParsed/* = nullptr*/ )
{
    int first, last;
    if(wasParsed)
        *wasParsed = false;
    if(!extractFirstNumberPosition(value, first, last, true, false))
        return defValue;
    bool ok;
    auto result = value.mid(first, last - first + 1).toInt(&ok);
    if(!ok)
        return defValue;

    if(wasParsed)
        *wasParsed = true;
    return result;
}

OSMAND_CORE_API long OSMAND_CORE_CALL OsmAnd::Utilities::parseArbitraryLong( const QString& value, long defValue, bool* wasParsed/* = nullptr*/ )
{
    int first, last;
    if(wasParsed)
        *wasParsed = false;
    if(!extractFirstNumberPosition(value, first, last, true, false))
        return defValue;
    bool ok;
    auto result = value.mid(first, last - first + 1).toLong(&ok);
    if(!ok)
        return defValue;

    if(wasParsed)
        *wasParsed = true;
    return result;
}

OSMAND_CORE_API unsigned int OSMAND_CORE_CALL OsmAnd::Utilities::parseArbitraryUInt( const QString& value, unsigned int defValue, bool* wasParsed/* = nullptr*/ )
{
    int first, last;
    if(wasParsed)
        *wasParsed = false;
    if(!extractFirstNumberPosition(value, first, last, false, false))
        return defValue;
    bool ok;
    auto result = value.mid(first, last - first + 1).toUInt(&ok);
    if(!ok)
        return defValue;

    if(wasParsed)
        *wasParsed = true;
    return result;
}

OSMAND_CORE_API unsigned long OSMAND_CORE_CALL OsmAnd::Utilities::parseArbitraryULong( const QString& value, unsigned long defValue, bool* wasParsed/* = nullptr*/ )
{
    int first, last;
    if(wasParsed)
        *wasParsed = false;
    if(!extractFirstNumberPosition(value, first, last, false, false))
        return defValue;
    bool ok;
    auto result = value.mid(first, last - first + 1).toULong(&ok);
    if(!ok)
        return defValue;

    if(wasParsed)
        *wasParsed = true;
    return result;
}

OSMAND_CORE_API float OSMAND_CORE_CALL OsmAnd::Utilities::parseArbitraryFloat( const QString& value, float defValue, bool* wasParsed /*= nullptr*/ )
{
    int first, last;
    if(wasParsed)
        *wasParsed = false;
    if(!extractFirstNumberPosition(value, first, last, true, true))
        return defValue;
    bool ok;
    auto result = value.mid(first, last - first + 1).toFloat(&ok);
    if(!ok)
        return defValue;

    if(wasParsed)
        *wasParsed = true;
    return result;
}

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::Utilities::parseArbitraryBool( const QString& value, bool defValue, bool* wasParsed /*= nullptr*/ )
{
    if(wasParsed)
        *wasParsed = false;

    if(value.isEmpty())
        return defValue;

    auto result = (value.compare("true", Qt::CaseInsensitive) == 0);

    if(wasParsed)
        *wasParsed = true;
    return result;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::x31toMeters( int64_t x31 )
{
    return static_cast<double>(x31) * 0.011;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::y31toMeters( int64_t y31 )
{
    return static_cast<double>(y31) * 0.01863;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::squareDistance31( uint32_t x31a, uint32_t y31a, uint32_t x31b, uint32_t y31b )
{
    const auto dx = Utilities::x31toMeters(static_cast<int64_t>(x31a) - static_cast<int64_t>(x31b));
    const auto dy = Utilities::y31toMeters(static_cast<int64_t>(y31a) - static_cast<int64_t>(y31b));
    return dx * dx + dy * dy;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::distance31( uint32_t x31a, uint32_t y31a, uint32_t x31b, uint32_t y31b )
{
    return qSqrt(squareDistance31(x31a, y31a, x31b, y31b));
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::distance( double xLonA, double yLatA, double xLonB, double yLatB )
{
    double R = 6371; // km
    double dLat = toRadians(yLatB - yLatA);
    double dLon = toRadians(xLonB - xLonA); 
    double a =
        qSin(dLat/2.0) * qSin(dLat/2.0) +
        qCos(toRadians(yLatA)) * qCos(toRadians(yLatB)) * 
        qSin(dLon/2.0) * qSin(dLon/2.0); 
    double c = 2.0 * qAtan2(qSqrt(a), qSqrt(1.0 - a)); 
    return R * c * 1000.0;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::projection31( uint32_t x31a, uint32_t y31a, uint32_t x31b, uint32_t y31b, uint32_t x31c, uint32_t y31c )
{
    // Scalar multiplication between (AB, AC)
    auto p =
        Utilities::x31toMeters(static_cast<int64_t>(x31b) - static_cast<int64_t>(x31a)) * Utilities::x31toMeters(static_cast<int64_t>(x31c) - static_cast<int64_t>(x31a)) +
        Utilities::y31toMeters(static_cast<int64_t>(y31b) - static_cast<int64_t>(y31a)) * Utilities::y31toMeters(static_cast<int64_t>(y31c) - static_cast<int64_t>(y31a));
    return p;
}

OSMAND_CORE_API  double OSMAND_CORE_CALL OsmAnd::Utilities::normalizedAngleRadians( double angle )
{
    while(angle > M_PI)
        angle -= 2.0 * M_PI;
    while(angle <= -M_PI)
        angle += 2.0 * M_PI;
    return angle;
}

OSMAND_CORE_API  double OSMAND_CORE_CALL OsmAnd::Utilities::normalizedAngleDegrees( double angle )
{
    while(angle > 180.0)
        angle -= 360.0;
    while(angle <= -180.0)
        angle += 360.;
    return angle;
}

OSMAND_CORE_API  int OSMAND_CORE_CALL OsmAnd::Utilities::javaDoubleCompare( double l, double r )
{
    const auto lNaN = qIsNaN(l);
    const auto rNaN = qIsNaN(r);
    const auto& li64 = *reinterpret_cast<uint64_t*>(&l);
    const auto& ri64 = *reinterpret_cast<uint64_t*>(&r);
    const auto lPos = (li64 >> 63) == 0;
    const auto rPos = (ri64 >> 63) == 0;
    const auto lZero = (li64 << 1) == 0;
    const auto rZero = (ri64 << 1) == 0;
    
    // NaN is considered by this method to be equal to itself and greater than all other double values (including +inf).
    if(lNaN && rNaN)
        return 0;
    if(lNaN)
        return +1;
    if(rNaN)
        return -1;

    // 0.0 is considered by this method to be greater than -0.0
    if(lZero && rZero)
    {
        if(lPos && !rPos)
            return -1;
        if(!lPos && rPos)
            return +1;
    }
    
    // All other cases
    return qCeil(l) - qCeil(r);
}

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::Utilities::findFiles( const QDir& origin, const QStringList& masks, QList< std::shared_ptr<QFile> >& files, bool recursively /*= true */ )
{
    auto fileList = origin.entryInfoList(masks, QDir::Files);
    for(auto itFile = fileList.begin(); itFile != fileList.end(); ++itFile)
        files.push_back(std::shared_ptr<QFile>(new QFile(itFile->absoluteFilePath())));

    if(recursively)
    {
        auto subdirs = origin.entryInfoList(QStringList(), QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
        for(auto itSubdir = subdirs.begin(); itSubdir != subdirs.end(); ++itSubdir)
            findFiles(QDir(itSubdir->absoluteFilePath()), masks, files, recursively);
    }
}
