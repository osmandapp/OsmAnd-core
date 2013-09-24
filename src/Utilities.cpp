#include "Utilities.h"

#include <cassert>
#include <limits>
#include <cmath>

#include <QtNumeric>
#include <QtCore>

#include "Logging.h"

const uint64_t l = 1UL << 31;

OSMAND_CORE_API int32_t OSMAND_CORE_CALL OsmAnd::Utilities::get31TileNumberX( double longitude )
{
    longitude = normalizeLongitude(longitude);
    return static_cast<int32_t>((longitude + 180.0) / 360.0*l);
}

OSMAND_CORE_API int32_t OSMAND_CORE_CALL OsmAnd::Utilities::get31TileNumberY( double latitude )
{
    latitude = normalizeLatitude(latitude);
    double eval = log( tan(toRadians(latitude)) + 1.0/cos(toRadians(latitude)) );
    if(eval > M_PI)
        eval = M_PI;
    return static_cast<int32_t>((1.0 - eval / M_PI) / 2.0*l);
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::get31LongitudeX( double x )
{
    return getLongitudeFromTile(21, x / 1024.);
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::get31LatitudeY( double y )
{
    return getLatitudeFromTile(21, y / 1024.);
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getTileNumberX( float zoom, double longitude )
{
    if( qAbs(longitude - 180.) < std::numeric_limits<double>::epsilon() )
        return getPowZoom(zoom) - 1;

    longitude = normalizeLongitude(longitude);
    return (longitude + 180.)/360. * getPowZoom(zoom);
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getTileNumberY( float zoom, double latitude )
{
    latitude = normalizeLatitude(latitude);
    double eval = log( tan(toRadians(latitude)) + 1/cos(toRadians(latitude)) );
    if (qIsInf(eval) || qIsNaN(eval))
    {
        latitude = latitude < 0 ? - 89.9 : 89.9;
        eval = log( tan(toRadians(latitude)) + 1/cos(toRadians(latitude)) );
    }
    double result = (1 - eval / M_PI) / 2 * getPowZoom(zoom);
    return result;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::normalizeLatitude( double latitude )
{
    while (latitude < -90.0 || latitude > 90.0)
    {
        if (latitude < 0.0)
            latitude += 180.0;
        else
            latitude -= 180.0;
    }

    if(latitude < -85.0511)
        return -85.0511;
    else if(latitude > 85.0511)
        return 85.0511;
    
    return latitude;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::normalizeLongitude( double longitude )
{
    while (longitude < -180.0 || longitude >= 180.0)
    {
        if (longitude < 0.0)
            longitude += 360.0;
        else
            longitude -= 360.0;
    }
    return longitude;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::degreesDiff(double a1, double a2){
    double diff = a1 - a2;
    while(diff > 180.0)
    {
        diff -= 360.0;
    }
    while(diff <= -180.0)
    {
        diff += 360.0;
    }
    return diff;

}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::toRadians( double angle )
{
    return angle / 180.0 * M_PI;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getPowZoom( float zoom )
{
    if(zoom >= 0.0f && qFuzzyCompare(zoom, static_cast<uint8_t>(zoom)))
        return 1 << static_cast<uint8_t>(zoom); 

    return qPow(2, zoom);
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getLongitudeFromTile( float zoom, double x )
{
    return x / getPowZoom(zoom) * 360.0 - 180.0;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getLatitudeFromTile( float zoom, double y )
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
    for(auto itChr = value.cbegin(); itChr != value.cend() && (first == -1 || last == -1); ++itChr, curPos++)
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
    if(value == "none") {
        if(wasParsed) { *wasParsed = true; }
        return 40;
    }

    int first, last;
    if(!extractFirstNumberPosition(value, first, last, false, true)) {
        if(wasParsed) { *wasParsed = false;}
        return defValue;
    }
    bool ok;
    auto result = value.mid(first, last - first + 1).toDouble(&ok);
    if(wasParsed){ *wasParsed = ok;}
    if(!ok) {
        return defValue;
    }
    result /= 3.6;
    if(value.contains("mph")) {
        result *= 1.6;
    }
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

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::x31toMeters( int32_t x31 )
{
    return static_cast<double>(x31) * 0.011;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::y31toMeters( int32_t y31 )
{
    return static_cast<double>(y31) * 0.01863;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::squareDistance31( int32_t x31a, int32_t y31a, int32_t x31b, int32_t y31b )
{
    const auto dx = Utilities::x31toMeters(x31a - x31b);
    const auto dy = Utilities::y31toMeters(y31a - y31b);
    return dx * dx + dy * dy;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::distance31( int32_t x31a, int32_t y31a, int32_t x31b, int32_t y31b )
{
    return qSqrt(squareDistance31(x31a, y31a, x31b, y31b));
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::squareDistance31( const PointI& a, const PointI& b )
{
    const auto dx = Utilities::x31toMeters(a.x - b.x);
    const auto dy = Utilities::y31toMeters(a.y - b.y);
    return dx * dx + dy * dy;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::distance31( const PointI& a, const PointI& b )
{
    return qSqrt(squareDistance31(a, b));
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

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::projection31( int32_t x31a, int32_t y31a, int32_t x31b, int32_t y31b, int32_t x31c, int32_t y31c )
{
    // Scalar multiplication between (AB, AC)
    auto p =
        Utilities::x31toMeters(x31b - x31a) * Utilities::x31toMeters(x31c - x31a) +
        Utilities::y31toMeters(y31b - y31a) * Utilities::y31toMeters(y31c - y31a);
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

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::Utilities::findFiles( const QDir& origin, const QStringList& masks, QFileInfoList& files, bool recursively /*= true */ )
{
    const auto& fileInfoList = origin.entryInfoList(masks, QDir::Files);
    files.append(fileInfoList);

    if(recursively)
    {
        const auto& subdirs = origin.entryInfoList(QStringList(), QDir::AllDirs | QDir::NoDotAndDotDot);
        for(auto itSubdir = subdirs.cbegin(); itSubdir != subdirs.cend(); ++itSubdir)
            findFiles(QDir(itSubdir->absoluteFilePath()), masks, files, recursively);
    }
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::polygonArea( const QVector<PointI>& points )
{
    double area = 0.0;

    assert(points.first() == points.last());

    auto itPrevPoint = points.cbegin();
    auto itPoint = itPrevPoint + 1;
    for(; itPoint != points.cend(); itPrevPoint = itPoint, ++itPoint)
    {
        const auto& p0 = *itPrevPoint;
        const auto& p1 = *itPoint;

        area += static_cast<double>(p0.x) * static_cast<double>(p1.y) - static_cast<double>(p1.x) * static_cast<double>(p0.y);
    }
    area = qAbs(area) * 0.5;

    return area;
}

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::Utilities::rayIntersectX( const PointF& v0_, const PointF& v1_, float mY, float& mX )
{
    // prev node above line
    // x,y node below line

    const auto& v0 = (v0_.y > v1_.y) ? v1_ : v0_;
    const auto& v1 = (v0_.y > v1_.y) ? v0_ : v1_;

    if(qFuzzyCompare(v1.y, mY) || qFuzzyCompare(v0.y, mY))
        mY -= 1.0f;

    if(v0.y > mY || v1.y < mY)
        return false;

    if(v1 == v0)
    {
        // the node on the boundary !!!
        mX = v1.x;
        return true;
    }

    // that tested on all cases (left/right)
    mX = v1.x + (mY - v1.y) * (v1.x - v0.x) / (v1.y - v0.y);
    return true;
}

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::Utilities::rayIntersect( const PointF& v0, const PointF& v1, const PointF& v )
{
    float t;
    if(!rayIntersectX(v0, v1, v.y, t))
        return false;

    if(t < v.x)
        return true;

    return false;
}

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::Utilities::rayIntersectX( const PointI& v0_, const PointI& v1_, int32_t mY, int32_t& mX )
{
    // prev node above line
    // x,y node below line

    const auto& v0 = (v0_.y > v1_.y) ? v1_ : v0_;
    const auto& v1 = (v0_.y > v1_.y) ? v0_ : v1_;

    if(v1.y == mY || v0.y == mY)
        mY -= 1;

    if(v0.y > mY || v1.y < mY)
        return false;

    if(v1 == v0)
    {
        // the node on the boundary !!!
        mX = v1.x;
        return true;
    }

    // that tested on all cases (left/right)
    mX = static_cast<int32_t>(v1.x + static_cast<double>(mY - v1.y) * static_cast<double>(v1.x - v0.x) / static_cast<double>(v1.y - v0.y));
    return true;
}

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::Utilities::rayIntersect( const PointI& v0, const PointI& v1, const PointI& v )
{
    int32_t t;
    if(!rayIntersectX(v0, v1, v.y, t))
        return false;

    if(t < v.x)
        return true;

    return false;
}

OSMAND_CORE_API OsmAnd::AreaI OSMAND_CORE_CALL OsmAnd::Utilities::tileBoundingBox31( const TileId& tileId, const ZoomLevel& zoom )
{
    AreaI output;

    const auto zoomShift = ZoomLevel31 - zoom;

    output.top = tileId.y << zoomShift;
    output.left = tileId.x << zoomShift;
    output.bottom = ((tileId.y + 1) << zoomShift) - 1;
    output.right = ((tileId.x + 1) << zoomShift) - 1;

    assert(output.top >= 0 && output.top <= std::numeric_limits<int32_t>::max());
    assert(output.left >= 0 && output.left <= std::numeric_limits<int32_t>::max());
    assert(output.bottom >= 0 && output.bottom <= std::numeric_limits<int32_t>::max());
    assert(output.right >= 0 && output.right <= std::numeric_limits<int32_t>::max());
    assert(output.right >= output.left);
    assert(output.bottom >= output.top);

    return output;
}

OSMAND_CORE_API OsmAnd::AreaI OSMAND_CORE_CALL OsmAnd::Utilities::areaRightShift( const AreaI& input, uint32_t shift )
{
    AreaI output;
    uint32_t tail;

    output.top = input.top >> shift;
    output.left = input.left >> shift;

    tail = input.bottom & ((1 << shift) - 1);
    output.bottom = (input.bottom >> shift) + (tail ? 1 : 0);
    tail = input.right & ((1 << shift) - 1);
    output.right = (input.right >> shift) + (tail ? 1 : 0);

    assert(output.top >= 0 && output.top <= std::numeric_limits<int32_t>::max());
    assert(output.left >= 0 && output.left <= std::numeric_limits<int32_t>::max());
    assert(output.bottom >= 0 && output.bottom <= std::numeric_limits<int32_t>::max());
    assert(output.right >= 0 && output.right <= std::numeric_limits<int32_t>::max());
    assert(output.right >= output.left);
    assert(output.bottom >= output.top);

    return output;
}

OSMAND_CORE_API OsmAnd::AreaI OSMAND_CORE_CALL OsmAnd::Utilities::areaLeftShift( const AreaI& input, uint32_t shift )
{
    AreaI output;

    output.top = input.top << shift;
    output.left = input.left << shift;
    output.bottom = input.bottom << shift;
    output.right = input.right << shift;

    assert(output.top >= 0 && output.top <= std::numeric_limits<int32_t>::max());
    assert(output.left >= 0 && output.left <= std::numeric_limits<int32_t>::max());
    assert(output.bottom >= 0 && output.bottom <= std::numeric_limits<int32_t>::max());
    assert(output.right >= 0 && output.right <= std::numeric_limits<int32_t>::max());
    assert(output.right >= output.left);
    assert(output.bottom >= output.top);

    return output;
}

OSMAND_CORE_API uint32_t OSMAND_CORE_CALL OsmAnd::Utilities::getNextPowerOfTwo( const uint32_t& value )
{
    if(value == 0)
        return 0;

    auto n = value;

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    return n;
}

OSMAND_CORE_API double OSMAND_CORE_CALL OsmAnd::Utilities::getMetersPerTileUnit( const float& zoom, const double& yTile, const double& unitsPerTile )
{
    // Equatorial circumference of the Earth in meters
    const static double C = 40075017.0;

    const auto powZoom = getPowZoom(zoom);
    const auto sinhValue = sinh( (2.0 * M_PI * yTile)/powZoom - M_PI );
    auto res = C / (powZoom * unitsPerTile * qSqrt(sinhValue*sinhValue + 1.0));

    return res;
}

OSMAND_CORE_API OsmAnd::TileId OSMAND_CORE_CALL OsmAnd::Utilities::normalizeTileId( const TileId& input, const ZoomLevel& zoom )
{
    TileId output = input;

    const auto maxTileIndex = static_cast<signed>(1u << zoom);

    while(output.x < 0)
        output.x += maxTileIndex;
    while(output.y < 0)
        output.y += maxTileIndex;

    if(zoom < 31)
    {
        while(output.x >= maxTileIndex)
            output.x -= maxTileIndex;
        while(output.y >= maxTileIndex)
            output.y -= maxTileIndex;
    }

    return output;
}

OSMAND_CORE_API OsmAnd::PointI OSMAND_CORE_CALL OsmAnd::Utilities::normalizeCoordinates( const PointI& input, const ZoomLevel& zoom )
{
    PointI output = input;

    const auto maxTileIndex = static_cast<signed>(1u << zoom);

    while(output.x < 0)
        output.x += maxTileIndex;
    while(output.y < 0)
        output.y += maxTileIndex;

    if(zoom < 31)
    {
        while(output.x >= maxTileIndex)
            output.x -= maxTileIndex;
        while(output.y >= maxTileIndex)
            output.y -= maxTileIndex;
    }

    return output;
}

OSMAND_CORE_API void OSMAND_CORE_CALL OsmAnd::Utilities::scanlineFillPolygon( const unsigned int& verticesCount, const PointF* vertices, std::function<void (const PointI&)> fillPoint )
{
    // Find min-max of Y
    float yMinF, yMaxF;
    yMinF = yMaxF = vertices[0].y;
    for(auto idx = 1u; idx < verticesCount; idx++)
    {
        const auto& y = vertices[idx].y;
        if(y > yMaxF)
            yMaxF = y;
        if(y < yMinF)
            yMinF = y;
    }
    const auto rowMin = qFloor(yMinF);
    const auto rowMax = qFloor(yMaxF);

    // Build set of edges
    struct Edge
    {
        const PointF* v0;
        int startRow;
        const PointF* v1;
        int endRow;
        float xOrigin;
        float slope;
        int nextRow;
    };
    QVector<Edge*> edges;
    edges.reserve(verticesCount);
    auto edgeIdx = 0u;
    for(auto idx = 0u, prevIdx = verticesCount - 1; idx < verticesCount; prevIdx = idx++)
    {
        auto v0 = &vertices[prevIdx];
        auto v1 = &vertices[idx];

        if(v0->y == v1->y)
        {
            // Horizontal edge
            auto edge = new Edge();
            edge->v0 = v0;
            edge->v1 = v1;
            edge->startRow = qFloor(edge->v0->y);
            edge->endRow = qFloor(edge->v1->y);
            edge->xOrigin = edge->v0->x;
            edge->slope = 0;
            edge->nextRow = qFloor(edge->v0->y) + 1;
            edges.push_back(edge);
            //LogPrintf(LogSeverityLevel::Debug, "Edge %p y(%d %d)(%f %f), next row = %d", edge, edge->startRow, edge->endRow, edge->v0->y, edge->v1->y, edge->nextRow);

            continue;
        }

        const PointF* pLower = nullptr;
        const PointF* pUpper = nullptr;
        if(v0->y < v1->y)
        {
            // Up-going edge
            pLower = v0;
            pUpper = v1;
        }
        else if(v0->y > v1->y)
        {
            // Down-going edge
            pLower = v1;
            pUpper = v0;
        }
        
        // Fill edge 
        auto edge = new Edge();
        edge->v0 = pLower;
        edge->v1 = pUpper;
        edge->startRow = qFloor(edge->v0->y);
        edge->endRow = qFloor(edge->v1->y);
        edge->slope = (edge->v1->x - edge->v0->x) / (edge->v1->y - edge->v0->y);
        edge->xOrigin = edge->v0->x - edge->slope * (edge->v0->y - qFloor(edge->v0->y));
        edge->nextRow = qFloor(edge->v1->y) + 1;
        for(auto vertexIdx = 0u; vertexIdx < verticesCount; vertexIdx++)
        {
            const auto& v = vertices[vertexIdx];

            if(v.y > edge->v0->y && qFloor(v.y) < edge->nextRow)
                edge->nextRow = qFloor(v.y);
        }
        //LogPrintf(LogSeverityLevel::Debug, "Edge %p y(%d %d)(%f %f), next row = %d", edge, edge->startRow, edge->endRow, edge->v0->y, edge->v1->y, edge->nextRow);
        edges.push_back(edge);
    }

    // Sort edges by ascending Y
    qSort(edges.begin(), edges.end(), [](Edge* l, Edge* r) -> bool
    {
        return l->v0->y > r->v0->y;
    });

    // Loop in [yMin .. yMax]
    QVector<Edge*> aet;
    aet.reserve(edges.size());
    for(auto rowIdx = rowMin; rowIdx <= rowMax;)
    {
        //LogPrintf(LogSeverityLevel::Debug, "------------------ %d -----------------", rowIdx);

        // Find active edges
        int nextRow = rowMax;
        for(auto itEdge = edges.cbegin(); itEdge != edges.cend(); ++itEdge)
        {
            auto edge = *itEdge;

            const auto isHorizontal = (edge->startRow == edge->endRow);
            if(nextRow > edge->nextRow && edge->nextRow > rowIdx && !isHorizontal)
                nextRow = edge->nextRow;

            if(edge->startRow != rowIdx)
                continue;

            if(isHorizontal)
            {
                // Fill horizontal edge
                const auto xMin = qFloor(qMin(edge->v0->x, edge->v1->x));
                const auto xMax = qFloor(qMax(edge->v0->x, edge->v1->x));
                /*for(auto x = xMin; x <= xMax; x++)
                    fillPoint(PointI(x, rowIdx));*/
                continue;
            }

            //LogPrintf(LogSeverityLevel::Debug, "line %d. Adding edge %p y(%f %f)", rowIdx, edge, edge->v0->y, edge->v1->y);
            aet.push_back(edge);
            continue;
        }

        // If there are no active edges, we've finished filling
        if(aet.isEmpty())
            break;
        assert(aet.size() % 2 == 0);

        // Sort aet by X
        qSort(aet.begin(), aet.end(), [](Edge* l, Edge* r) -> bool
        {
            return l->v0->x > r->v0->x;
        });
        
        // Find next row
        for(; rowIdx < nextRow; rowIdx++)
        {
            const unsigned int pairsCount = aet.size() / 2;

            auto itEdgeL = aet.cbegin();
            auto itEdgeR = itEdgeL + 1;

            for(auto pairIdx = 0u; pairIdx < pairsCount; pairIdx++, itEdgeL = ++itEdgeR, ++itEdgeR)
            {
                auto lEdge = *itEdgeL;
                auto rEdge = *itEdgeR;

                // Fill from l to r
                auto lXf = lEdge->xOrigin + (rowIdx - lEdge->startRow + 0.5f) * lEdge->slope;
                auto rXf = rEdge->xOrigin + (rowIdx - rEdge->startRow + 0.5f) * rEdge->slope;
                auto xMinF = qMin(lXf, rXf);
                auto xMaxF = qMax(lXf, rXf);
                auto xMin = qFloor(xMinF);
                auto xMax = qFloor(xMaxF);
                
                LogPrintf(LogSeverityLevel::Debug, "line %d from %d(%f) to %d(%f)", rowIdx, xMin, xMinF, xMax, xMaxF);
                /*for(auto x = xMin; x <= xMax; x++)
                    fillPoint(PointI(x, rowIdx));*/
            }
        }

        // Deactivate those edges that have end at yNext
        for(auto itEdge = aet.begin(); itEdge != aet.end();)
        {
            auto edge = *itEdge;

            if(edge->endRow <= nextRow)
            {
                // When we're done processing the edge, fill it Y-by-X
                auto startCol = qFloor(edge->v0->x);
                auto endCol = qFloor(edge->v1->x);
                auto revSlope = 1.0f / edge->slope;
                auto yOrigin = edge->v0->y - revSlope * (edge->v0->x - qFloor(edge->v0->x));
                auto xMax = qMax(startCol, endCol);
                auto xMin = qMin(startCol, endCol);
                for(auto colIdx = xMin; colIdx <= xMax; colIdx++)
                {
                    auto yf = yOrigin + (colIdx - startCol + 0.5f) * revSlope;
                    auto y = qFloor(yf);

                    LogPrintf(LogSeverityLevel::Debug, "col %d(s%d) added Y = %d (%f)", colIdx, colIdx - startCol, y, yf);
                    fillPoint(PointI(colIdx, y));
                }

                //////////////////////////////////////////////////////////////////////////
                auto yMax = qMax(edge->startRow, edge->endRow);
                auto yMin = qMin(edge->startRow, edge->endRow);
                for(auto rowIdx_ = yMin; rowIdx_ <= yMax; rowIdx_++)
                {
                    auto xf = edge->xOrigin + (rowIdx_ - edge->startRow + 0.5f) * edge->slope;
                    auto x = qFloor(xf);

                    LogPrintf(LogSeverityLevel::Debug, "row %d(s%d) added Y = %d (%f)", rowIdx_, rowIdx_ - edge->startRow, x, xf);
                    fillPoint(PointI(x, rowIdx_));
                }
                //////////////////////////////////////////////////////////////////////////

                //LogPrintf(LogSeverityLevel::Debug, "line %d. Removing edge %p y(%f %f)", rowIdx, edge, edge->v0->y, edge->v1->y);
                itEdge = aet.erase(itEdge);
            }
            else
                ++itEdge;
        }
    }

    // Cleanup
    for(auto itEdge = edges.cbegin(); itEdge != edges.cend(); ++itEdge)
        delete *itEdge;
}
