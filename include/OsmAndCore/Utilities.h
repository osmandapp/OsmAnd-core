#ifndef _OSMAND_CORE_UTILITIES_H_
#define _OSMAND_CORE_UTILITIES_H_

#include <OsmAndCore/stdlib_common.h>
#include <climits>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QtMath>
#include <QtNumeric>
#include <QString>
#include <QStringList>
#include <QList>
#include <QDir>
#include <QFile>
#include <QVector>
#include <QSet>
#include <QByteArray>
#include <QLocale>
#include <QDateTime>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/Bitmask.h>
#include <OsmAndCore/Logging.h>

// Most polylines width is under 50 meters
#define MAX_ENLARGE_PRIMITIVIZED_AREA_METERS 50
#define ENLARGE_PRIMITIVIZED_AREA_COEFF 0.2f

#define ENLARGE_VISIBLE_AREA_COEFF 0.5f

namespace OsmAnd
{
    struct OSMAND_CORE_API Utilities Q_DECL_FINAL
    {
        inline static std::tm localtime(const std::time_t& time)
        {
            std::tm tm_snapshot;
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
            localtime_s(&tm_snapshot, &time);
#else
            localtime_r(&time, &tm_snapshot); // POSIX
#endif
            return tm_snapshot;
        }

        inline static int64_t roundMillisecondsToHours(int64_t dateTime)
        {
            const int64_t oneHour = 1000 * 60 * 60;
            const int64_t timeOfHour = dateTime / oneHour * oneHour;
            return timeOfHour + (dateTime - timeOfHour < (oneHour >> 1) ? 0 : oneHour);
        }

        inline static QString getDateTimeString(int64_t dateTime)
        {
            int64_t time = qBound(-100000000000000ll, static_cast<long long>(dateTime), 100000000000000ll);
            QLocale locale = QLocale(QLocale::English, QLocale::UnitedStates);
            return locale.toString(QDateTime::fromMSecsSinceEpoch(time, Qt::UTC), QStringLiteral("yyyyMMdd_hh00"));
        }

        inline static QString getMGRSLetter(int zoneNumber)
        {
            switch (zoneNumber)
            {
                case 0: return QStringLiteral("A");
                case 1: return QStringLiteral("B");
                case 2: return QStringLiteral("C");
                case 3: return QStringLiteral("D");
                case 4: return QStringLiteral("E");
                case 5: return QStringLiteral("F");
                case 6: return QStringLiteral("G");
                case 7: return QStringLiteral("H");
                case 8: return QStringLiteral("J");
                case 9: return QStringLiteral("K");
                case 10: return QStringLiteral("L");
                case 11: return QStringLiteral("M");
                case 12: return QStringLiteral("N");
                case 13: return QStringLiteral("P");
                case 14: return QStringLiteral("Q");
                case 15: return QStringLiteral("R");
                case 16: return QStringLiteral("S");
                case 17: return QStringLiteral("T");
                case 18: return QStringLiteral("U");
                case 19: return QStringLiteral("V");
                case 20: return QStringLiteral("W");
                case 21: return QStringLiteral("X");
                case 22: return QStringLiteral("Y");
                case 23: return QStringLiteral("Z");
            }
            return QString();
        }

        inline static QString getMGRSSquareColumn(const PointI& zoneUTM, const PointD& coordinates)
        {
            return getMGRSLetter(static_cast<int>(std::floor(coordinates.x)) - 1 + (zoneUTM.x - 1) % 3 * 8);
        }

        inline static QString getMGRSSquareRow(const PointI& zoneUTM, const PointD& coordinates)
        {
            return getMGRSLetter((static_cast<int>(std::floor(coordinates.y)) + ((zoneUTM.x & 1) > 0 ? 0 : 5)) % 20);
        }

        inline static void removeTrailingZeros(QString& str)
        {
            while (str.back() == '0')
            {
                str.chop(1);
            }
            if (str.back() == '.')
                str.chop(1);
        }

        inline static QString getDegreeMinuteSecondString(double coordinate)
        {
            auto crd = std::abs(coordinate);
            auto deg = static_cast<int>(std::floor(crd));
            crd = (crd - deg) * 60.0;
            auto min = static_cast<int>(std::floor(crd));
            crd = (crd - min) * 600.0;
            auto sec = std::round(crd) / 10.0;
            if (sec > 59.9)
            {
                sec = 0.0;
                min++;
                if (min > 59)
                {
                    min = 0;
                    deg++;
                }
            }
            QString smin, ssec;
            if (sec >= 0.05)
            {
                auto secs = QStringLiteral("%1").arg(sec, 4, 'f', 1, QLatin1Char('0'));
                Utilities::removeTrailingZeros(secs);
                ssec = QStringLiteral("%1\"").arg(secs);
            }
            if (min >= 1 || !ssec.isEmpty())
                smin = QStringLiteral("%1'").arg(min, 2, 10, QLatin1Char('0'));
            auto sdeg = QStringLiteral("%1°").arg(deg);
            return QStringLiteral("%1%2%3").arg(sdeg).arg(smin).arg(ssec);
        }

        inline static QString getDegreeMinuteString(double coordinate)
        {
            auto crd = std::abs(coordinate);
            auto deg = static_cast<int>(std::floor(crd));
            crd = (crd - deg) * 60000.0;
            auto min = std::round(crd) / 1000.0;
            if (min > 59.999)
            {
                min = 0.0;
                deg++;
            }
            QString smin;
            if (min >= 0.0005)
            {
                auto mins = QStringLiteral("%1").arg(min, 6, 'f', 3, QLatin1Char('0'));
                Utilities::removeTrailingZeros(mins);
                smin = QStringLiteral("%1'").arg(mins);
            }
            auto sdeg = QStringLiteral("%1°").arg(deg);
            return QStringLiteral("%1%2").arg(sdeg).arg(smin);
        }

        inline static double toRadians(const double angle)
        {
            return angle / 180.0 * M_PI;
        }

        inline static int32_t get31TileNumberX(const double longitude)
        {
            const auto l = (1UL << 31);
            return static_cast<int32_t>((normalizeLongitude(longitude) + 180.0) / 360.0*l);
        }

        inline static int32_t get31TileNumberY(const double latitude)
        {
            const auto l = (1UL << 31);

            const auto latitude_ = normalizeLatitude(latitude);
            double eval = log(tan(toRadians(latitude_)) + 1.0 / cos(toRadians(latitude_)));
            if (eval > M_PI)
                eval = M_PI;
            return static_cast<int32_t>((1.0 - eval / M_PI) / 2.0*l);
        }

        inline static PointI convertLatLonTo31(const LatLon& latLon)
        {
            return { get31TileNumberX(latLon.longitude), get31TileNumberY(latLon.latitude) };
        }

        inline static double get31LongitudeX(const double x)
        {
            return getLongitudeFromTile(21, x / 1024.);
        }

        inline static double get31LatitudeY(const double y)
        {
            return getLatitudeFromTile(21, y / 1024.);
        }

        inline static LatLon convert31ToLatLon(const PointI& point31)
        {
            return { get31LatitudeY(point31.y), get31LongitudeX(point31.x) };
        }

        inline static double getTileNumberX(const float zoom, const double longitude)
        {
            if (qAbs(longitude - 180.) < std::numeric_limits<double>::epsilon())
                return getPowZoom(zoom) - 1;

            return (normalizeLongitude(longitude) + 180.) / 360. * getPowZoom(zoom);
        }

        inline static double getTileNumberY(const float zoom, double latitude)
        {
            latitude = normalizeLatitude(latitude);
            double eval = log(tan(toRadians(latitude)) + 1 / cos(toRadians(latitude)));
            if (qIsInf(eval) || qIsNaN(eval))
            {
                latitude = latitude < 0 ? -89.9 : 89.9;
                eval = log(tan(toRadians(latitude)) + 1 / cos(toRadians(latitude)));
            }
            double result = (1 - eval / M_PI) / 2 * getPowZoom(zoom);
            return result;
        }

        inline static float convert31toFloat(const int32_t value, const ZoomLevel zoom)
        {
            return static_cast<float>((static_cast<double>(value) / (1u << (ZoomLevel::MaxZoomLevel - zoom))));
        }

        inline static PointF convert31toFloat(const PointI& p, const ZoomLevel zoom)
        {
            const auto tileSize31 = (1u << (ZoomLevel::MaxZoomLevel - zoom));
            return { static_cast<float>((static_cast<double>(p.x) / tileSize31)), static_cast<float>((static_cast<double>(p.y) / tileSize31)) };
        }

        inline static PointD convert31toDouble(const PointI64& p, const ZoomLevel zoom)
        {
            const auto tileSize31 = (1u << (ZoomLevel::MaxZoomLevel - zoom));
            return { (static_cast<double>(p.x) / tileSize31), (static_cast<double>(p.y) / tileSize31) };
        }

        inline static PointI convertFloatTo31(const PointF& point, const PointI& target31, const ZoomLevel zoom)
        {
            const auto tileSize31 = (1u << (ZoomLevel::MaxZoomLevel - zoom));
            const auto offsetFromTarget31 = static_cast<PointD>(point) * static_cast<double>(tileSize31);
            const PointI64 location = static_cast<PointI64>(target31) + static_cast<PointI64>(offsetFromTarget31);
            return Utilities::normalizeCoordinates(location, ZoomLevel::ZoomLevel31);
        }

        inline static double normalizeLatitude(double latitude)
        {
            while (latitude < -90.0 || latitude > 90.0)
            {
                if (latitude < 0.0)
                    latitude += 180.0;
                else
                    latitude -= 180.0;
            }

            if (latitude < -85.0511)
                return -85.0511;
            else if (latitude > 85.0511)
                return 85.0511;

            return latitude;
        }

        inline static double normalizeLongitude(double longitude)
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

        inline static int32_t metersTo31(const double positionInMeters)
        {
            auto earthInMeters = 2.0 * M_PI * 6378137;
            auto earthIn31 = 1.0 + INT32_MAX;
            auto positionIn31 = static_cast<int64_t>(std::floor((positionInMeters / earthInMeters + 0.5) * earthIn31));
            if (positionIn31 > INT32_MAX)
                positionIn31 = positionIn31 - INT32_MAX - 1;
            else if (positionIn31 < 0)
                positionIn31 = positionIn31 + INT32_MAX + 1;
            return static_cast<int32_t>(positionIn31);
        }

        inline static PointI metersTo31(const PointD& locationInMeters)
        {
            return PointI(metersTo31(locationInMeters.x), INT32_MAX - metersTo31(locationInMeters.y));
        }

        inline static double metersFrom31(const double position31)
        {
            auto earthInMeters = 2.0 * M_PI * 6378137;
            auto earthIn31 = 1.0 + INT32_MAX;
            return (position31 / earthIn31 - 0.5) * earthInMeters;
        }

        inline static PointD metersFrom31(const PointD& location31)
        {
            auto earthIn31 = 1.0 + INT32_MAX;
            return PointD(metersFrom31(location31.x), metersFrom31(earthIn31 - location31.y));
        }

        inline static double getPowZoom(const float zoom)
        {
            if (zoom >= 0.0f && qFuzzyCompare(zoom, static_cast<uint8_t>(zoom)))
                return 1u << static_cast<uint8_t>(zoom);

            return qPow(2, zoom);
        }
        
        inline static double getSignedAngle(const PointD& vector1N, const PointD& vector2N)
        {
            const int sign = dotProduct(vector1N, PointD(vector2N.y, -vector2N.x)) < 0 ? -1 : +1;
            const auto signedAngle = sign * qAcos(qBound(-1.0, dotProduct(vector1N, vector2N), 1.0));
            return Utilities::normalizedAngleRadians(signedAngle);
        }

        inline static double dotProduct(const PointD& vector1, const PointD& vector2)
        {
            return vector1.x * vector2.x + vector1.y * vector2.y;
        }

        inline static PointD getScaleDivisor31ToPixel(const PointI& areaSizeInPixels, const ZoomLevel zoom)
        {
            PointD scaleDivisor31ToPixel;

            const auto tileDivisor = getPowZoom(MaxZoomLevel - zoom);
            scaleDivisor31ToPixel.x = tileDivisor / static_cast<double>(areaSizeInPixels.x);
            scaleDivisor31ToPixel.y = tileDivisor / static_cast<double>(areaSizeInPixels.y);

            return scaleDivisor31ToPixel;
        }

        inline static double getTileDistanceWidth(float zoom)
        {
            double lat1 = 30;
            double lon1 = getLongitudeFromTile(zoom, 0);
            double lat2 = 30;
            double lon2 = getLongitudeFromTile(zoom, 1);
            return distance(lon1, lat1, lon2, lat2);
        }

        inline static double getLongitudeFromTile(const float zoom, const double x)
        {
            return x / getPowZoom(zoom) * 360.0 - 180.0;
        }

        inline static double getLatitudeFromTile(const float zoom, const double y)
        {
            int sign = y < 0 ? -1 : 1;
            double result = atan(sign * sinh(M_PI * (1 - 2 * y / getPowZoom(zoom)))) * 180. / M_PI;
            return result;
        }

        inline static PointD getAnglesFrom31(const PointI& p)
        {
            const auto intFull = static_cast<double>(INT32_MAX) + 1.0;
            const auto sign = p.y < 0 ? -1.0 : 1.0;
            return PointD(
                static_cast<double>(p.x) / intFull * M_PI * 2.0 - M_PI,
                atan(sign * sinh(M_PI * (1.0 - 2.0 * static_cast<double>(p.y) / intFull))));
        }

        inline static PointI get31FromAngles(const PointD& a)
        {
            const int64_t intMax = INT32_MAX;
            const auto intFull = intMax + 1;
            auto y = std::min(std::max(a.y, -M_PI_2), M_PI_2);
            auto eval = log(tan(y) + 1.0 / cos(y));
            auto res = eval >= -M_PI && eval <= M_PI ? eval : (y < 0.0 ? -M_PI : M_PI);
            return PointI(
                static_cast<int32_t>(static_cast<int64_t>((a.x + M_PI) / (M_PI * 2.0) * intFull) % intFull),
                static_cast<int32_t>(std::min(static_cast<int64_t>((1.0 - res / M_PI) / 2.0 * intFull), intMax)));
        }

#if !defined(SWIG)
        inline static PointI64 get64FromAngles(const PointD& a)
        {
            const int64_t intMax = INT32_MAX;
            const auto intFull = intMax + 1;
            auto y = std::min(std::max(a.y, -M_PI_2), M_PI_2);
            auto eval = log(tan(y) + 1.0 / cos(y));
            const auto angFull = 2.0 * M_PI;
            auto res = eval >= -angFull && eval <= angFull ? eval : (y < 0.0 ? -angFull : angFull);
            return PointI64(
                static_cast<int64_t>((a.x + M_PI) / (M_PI * 2.0) * intFull) % intFull,
                static_cast<int64_t>((1.0 - res / M_PI) / 2.0 * intFull));
        }
#endif // !defined(SWIG)

        inline static PointD getZoneUTM(const PointD& location, double* refLonDeg)
        {
            auto result = location * 180.0 / M_PI;
            if (result.x >= 180.0)
                result.x -= 360.0;
            else if (result.x < -180.0)
                result.x += 360.0;
            result.x /= 6.0;
            result.x += 31.0;
            result.y /= 8.0;
            result.y += 13.0;
            double refLon = (floor(result.x) - 31.0) * 6.0 + 3.0;
            if (result.y >= 23.5 || result.y < 3.0)
            {
                result.x = std::numeric_limits<double>::quiet_NaN();
                result.y = std::numeric_limits<double>::quiet_NaN();
            }
            else if (result.y >= 22.0 && result.x >= 31.0 && result.x < 38.0)
            {
                result.y = (result.y - 22.0) / 1.5 + 22.0;
                double s = result.x < 32.5 ? 31.0 : (result.x < 34.5 ? 33.0 : (result.x < 36.5 ? 35.0 : 37.0));
                result.x = s
                    + (result.x - (result.x < 32.5 ? s : s - 0.5)) / (result.x >= 32.5 && result.x < 36.5 ? 2.0 : 1.5);
                refLon = result.x >= 37.0 ? 39.0 : (result.x >= 35.0 ? 27.0 : (result.x >= 33.0 ? 15.0 : 3.0));
            }
            else if (result.y >= 20.0 && result.y < 21.0 && result.x >= 31.0 && result.x < 33.0)
            {
                result.x = result.x < 31.5 ? 31.0 + (result.x - 31.0) / 0.5 : 32.0 + (result.x - 31.5) / 1.5;
                refLon = result.x >= 32.0 ? 9.0 : 3.0;
            }
            if (refLonDeg)
                *refLonDeg = refLon;
            return result;
        }

        inline static int getCodedZoneUTM(const PointI& location31, const bool unite = true)
        {
            auto zUTM = getZoneUTM(getAnglesFrom31(location31), nullptr);
            auto zone = PointI(static_cast<int32_t>(std::floor(zUTM.x)), static_cast<int32_t>(std::floor(zUTM.y)));
            if (isnan(zUTM.y))
            {
                zone.x = 0;
                zone.y = 127;
            }
            if (unite && (zone.x < 31 || zone.x > 37 || zone.y < 20 || zone.y == 21 || (zone.y == 20 && zone.x > 32)))
                zone.y = 0;
    
            return zone.y << 6 | zone.x;
        }

        inline static PointD getCoordinatesUTM(const PointD& lonlat, const double refLon)
        {
            auto sinlat = sin(lonlat.y);
            //double f = 1.0 / 298.257223563;
            //double n = f / (2.0 - f);
            double nn = 0.081819190842621486; //2.0 * sqrt(n) / (1.0 + n);
            double t = sinh(atanh(sinlat) - nn * atanh(nn * sinlat));
            double xi = atan(t / cos(lonlat.x - refLon));
            double xi2 = xi * 2.0;
            double xi4 = xi * 4.0;
            double xi6 = xi * 6.0;
            double eta = atanh(sin(lonlat.x - refLon) / sqrt(1.0 + t * t));
            double eta2 = eta * 2.0;
            double eta4 = eta * 4.0;
            double eta6 = eta * 6.0;
            double a1 = 8.3773181881925413e-4; //0.5 * n - 2.0 / 3.0 * n * n + 5.0 / 16.0 * pow(n, 3.0);
            double a2 = 7.6084969586991665e-7; //13.0 / 48.0 * n * n - 3.0 / 5.0 * pow(n, 3.0);
            double a3 = 1.2034877875966644e-9; //61.0 / 240.0 * pow(n, 3.0);
            PointD result(eta + a1 * cos(xi2) * sinh(eta2) + a2 * cos(xi4) * sinh(eta4) + a3 * cos(xi6) * sinh(eta6),
                xi + a1 * sin(xi2) * cosh(eta2) + a2 * sin(xi4) * cosh(eta4) + a3 * sin(xi6) * cosh(eta6));
            //double a = 6378.137 / (1.0 + n) * (1.0 + n * n / 4.0 + pow(n, 4.0) / 64.0 + pow(n, 6.0) / 256.0);
            result *= 6364.9021661650868; //a * 0.9996;
            result.x += 500.0;
            result.y += 10000.0;
            return result;
        }

        inline static ZoomLevel clipZoomLevel(ZoomLevel zoom)
        {
            return qBound(MinZoomLevel, zoom, MaxZoomLevel);
        }

        inline static float zoomFractionToVisual(float zoomFraction)
        {
            assert(qAbs(zoomFraction) < 1.0f);

            // [ 0.0f ...  1.0f) -> [1.0f ... 2.0f)
            // (-1.0f ... -0.0f] -> (0.5f ... 1.0f]

            return zoomFraction >= 0.0f
                ? 1.0f + zoomFraction
                : 1.0f + zoomFraction / 2.0f;
        }

        inline static float visualZoomToFraction(float visualZoom)
        {
            assert(visualZoom > 0.5f && visualZoom < 2.0f);

            return visualZoom >= 1.0f
                ? visualZoom - 1.0f
                : (visualZoom - 1.0f) * 2.0f;
        }

        inline static double x31toMeters(const int32_t x31)
        {
            return static_cast<double>(x31) * 0.011;
        }

        inline static double y31toMeters(const int32_t y31)
        {
            return static_cast<double>(y31) * 0.01863;
        }

        inline static int64_t metersToX31(const double meters)
        {
            return static_cast<int64_t>(meters / 0.011);
        }

        inline static int64_t metersToY31(const double meters)
        {
            return static_cast<int64_t>(meters / 0.01863);
        }
        
        static double x31ToMeters(int x1, int x2, int y);

        static double y31ToMeters(int y1, int y2, int x);
        
        inline static double squareDistance31(const int32_t x31a, const int32_t y31a, const int32_t x31b, const int32_t y31b)
        {
            const auto dx = Utilities::x31ToMeters(x31a, x31b, y31a);
            const auto dy = Utilities::y31ToMeters(y31a, y31b, x31a);
            return dx * dx + dy * dy;
        }

        inline static double distance31(const int32_t x31a, const int32_t y31a, const int32_t x31b, const int32_t y31b)
        {
            return qSqrt(squareDistance31(x31a, y31a, x31b, y31b));
        }

        inline static double squareDistance31(const PointI& a, const PointI& b)
        {
            const auto dx = Utilities::x31toMeters(a.x - b.x);
            const auto dy = Utilities::y31toMeters(a.y - b.y);
            return dx * dx + dy * dy;
        }

        inline static double distance31(const PointI& a, const PointI& b)
        {
            return qSqrt(squareDistance31(a, b));
        }

        inline static double distance(const double xLonA, const double yLatA, const double xLonB, const double yLatB)
        {
            double R = 6372.8; // for haversine use R = 6372.8 km instead of 6371 km
            double dLat = toRadians(yLatB - yLatA);
            double dLon = toRadians(xLonB - xLonA);
            double sinHalfLat = qSin(dLat / 2.0);
            double sinHalfLon = qSin(dLon / 2.0);
            double a = sinHalfLat * sinHalfLat +
                qCos(toRadians(yLatA)) * qCos(toRadians(yLatB)) *
                sinHalfLon * sinHalfLon;
            //double c = 2.0 * qAtan2(qSqrt(a), qSqrt(1.0 - a));
            //return R * c * 1000.0;
            // simplyfy haversine:
            return (2 * R * 1000 * qAsin(qSqrt(a)));
        }

        inline static double distance(const LatLon& a, const LatLon& b)
        {
            return distance(a.longitude, a.latitude, b.longitude, b.latitude);
        }

        inline static double distance(const Nullable<LatLon>& a, const Nullable<LatLon>& b)
        {
            return (a.isSet() && b.isSet()) ? distance(*a, *b) : NAN;
        }

        inline static double projection31(const int32_t x31a, const int32_t y31a, const int32_t x31b, const int32_t y31b, const int32_t x31c, const int32_t y31c)
        {
            // Scalar multiplication between (AB, AC)
            auto p =
            Utilities::x31ToMeters(x31b, x31a, y31a) * Utilities::x31ToMeters(x31c, x31a, y31a) + Utilities::y31ToMeters(y31b, y31a, y31a) * Utilities::y31ToMeters(y31c, y31a, y31a);
            return p;
        }

        inline static double projection31(const PointI& a, const PointI& b, const PointI& c)
        {
            return projection31(a.x, a.y, b.x, b.y, c.x, c.y);
        }

        inline static double projectionCoeff31(const PointI& a, const PointI& b, const PointI& c)
        {
            double mDist = Utilities::x31toMeters(a.x - b.x) * Utilities::x31toMeters(a.x - b.x) +
                           Utilities::x31toMeters(a.y - b.y) * Utilities::x31toMeters(a.y - b.y);
            double projection = projection31(a.x, a.y, b.x, b.y, c.x, c.y);
            if (projection < 0) {
                return 0;
            } else if (projection >= mDist) {
                return 1;
            } else {
                return (projection / mDist);
            }
        }
        
        inline static double measuredDist31(int x1, int y1, int x2, int y2) {
            return distance(get31LongitudeX(x1), get31LatitudeY(y1), get31LongitudeX(x2), get31LatitudeY(y2));
        }

        inline static double normalizedAngleRadians(double angle)
        {
            while (angle > M_PI)
                angle -= 2.0 * M_PI;
            while (angle <= -M_PI)
                angle += 2.0 * M_PI;
            return angle;
        }

        inline static double normalizedAngleDegrees(double angle)
        {
            while (angle > 180.0)
                angle -= 360.0;
            while (angle <= -180.0)
                angle += 360.0;
            return angle;
        }

#if !defined(SWIG)
        inline static int64_t doubledPolygonArea(const QVector<PointI>& points)
        {
            int64_t area = 0.0;

            // assert(points.first() == points.last());

            auto p0 = points.constData();
            auto p1 = p0 + 1;
            for (auto idx = 1, count = points.size(); idx < count; idx++, p0++, p1++)
            {
                area +=
                    static_cast<int64_t>(p0->x) * static_cast<int64_t>(p1->y) -
                    static_cast<int64_t>(p1->x) * static_cast<int64_t>(p0->y);
            }
            area = qAbs(area);

            return area;
        }
#endif // !defined(SWIG)

        inline static double computeSignedArea(const QVector<PointI>& points)
        {
            double area = 0.0;
            const int n = points.size();
            for (int i = 0; i < n; ++i)
            {
                const auto& p0 = points[i];
                const auto& p1 = points[(i + 1) % n];
                area += static_cast<double>(p0.x) * p1.y - static_cast<double>(p1.x) * p0.y;
            }

            return 0.5 * area;
        }

        inline static double polygonArea(const QVector<PointI>& points)
        {
            return static_cast<double>(doubledPolygonArea(points))* 0.5;
        }

        inline static bool rayIntersectX(const PointD& v0_, const PointD& v1_, double mY, double& mX)
        {
            // prev node above line
            // x,y node below line

            const auto& v0 = (v0_.y > v1_.y) ? v1_ : v0_;
            const auto& v1 = (v0_.y > v1_.y) ? v0_ : v1_;

            if (qFuzzyCompare(v1.y, mY) || qFuzzyCompare(v0.y, mY))
                mY -= 1.0;

            if (v0.y > mY || v1.y < mY)
                return false;

            if (v1 == v0)
            {
                // the node on the boundary !!!
                mX = v1.x;
                return true;
            }

            // that tested on all cases (left/right)
            mX = v1.x + (mY - v1.y) * (v1.x - v0.x) / (v1.y - v0.y);
            return true;
        }

        inline static bool rayIntersect(const PointD& v0, const PointD& v1, const PointD& v)
        {
            double t;
            if (!rayIntersectX(v0, v1, v.y, t))
                return false;

            if (t < v.x)
                return true;

            return false;
        }

        inline static bool rayIntersectX(const PointF& v0_, const PointF& v1_, float mY, float& mX)
        {
            // prev node above line
            // x,y node below line

            const auto& v0 = (v0_.y > v1_.y) ? v1_ : v0_;
            const auto& v1 = (v0_.y > v1_.y) ? v0_ : v1_;

            if (qFuzzyCompare(v1.y, mY) || qFuzzyCompare(v0.y, mY))
                mY -= 1.0f;

            if (v0.y > mY || v1.y < mY)
                return false;

            if (v1 == v0)
            {
                // the node on the boundary !!!
                mX = v1.x;
                return true;
            }

            // that tested on all cases (left/right)
            mX = v1.x + (mY - v1.y) * (v1.x - v0.x) / (v1.y - v0.y);
            return true;
        }

        inline static bool rayIntersect(const PointF& v0, const PointF& v1, const PointF& v)
        {
            float t;
            if (!rayIntersectX(v0, v1, v.y, t))
                return false;

            if (t < v.x)
                return true;

            return false;
        }

        inline static bool rayIntersectX(const PointI& v0_, const PointI& v1_, int32_t mY, int32_t& mX)
        {
            // prev node above line
            // x,y node below line

            const auto& v0 = (v0_.y > v1_.y) ? v1_ : v0_;
            const auto& v1 = (v0_.y > v1_.y) ? v0_ : v1_;

            if (v1.y == mY || v0.y == mY)
                mY -= 1;

            if (v0.y > mY || v1.y < mY)
                return false;

            if (v1 == v0)
            {
                // the node on the boundary !!!
                mX = v1.x;
                return true;
            }

            // that tested on all cases (left/right)
            mX = static_cast<int32_t>(v1.x + static_cast<double>(mY - v1.y) * static_cast<double>(v1.x - v0.x) / static_cast<double>(v1.y - v0.y));
            return true;
        }

        inline static bool rayIntersect(const PointI& v0, const PointI& v1, const PointI& v)
        {
            int32_t t;
            if (!rayIntersectX(v0, v1, v.y, t))
                return false;

            if (t < v.x)
                return true;

            return false;
        }

        inline static double squaredDistanceBetweenPointAndLine(const PointI l0, const PointI l1, const PointI p, bool* pOutInOnLine = nullptr)
        {
            // Make p center of the coordinate system
            const auto a = PointI64(l0) - PointI64(p);
            const auto b = PointI64(l1) - PointI64(p);

            // Calculate distance between point and line using length of projection of point p on line l0-l1
            const auto vL = b - a;
            const auto squaredLineLength = vL.squareNorm();

            // In case denominator is zero, it means l0 the same as l1, in that case return distance to either l0 or l1
            if (squaredLineLength == 0)
            {
                // Also, in this case projection point is always equal to l0 (and l1), thus always on line
                if (pOutInOnLine != nullptr)
                    *pOutInOnLine = true;

                return static_cast<double>(a.squareNorm());
            }

            // Calculate squared distance (p == 0, l0 == 1, l1 == 2):
            //       |(x2 - x1)*(y1 - y0) - (x1 - x0)*(y2 - y1)|
            // d = -----------------------------------------------
            //            sqrt( (x2 - x1)^2 + (y2 - y1)^2 )
            // Since p became center of the coordinate system, formula above can be rewritten as
            //         |(x2 - x1)*y1 - x1*(y2 - y1)|
            // d = -------------------------------------
            //       sqrt( (x2 - x1)^2 + (y2 - y1)^2 )
            const auto nominator = qAbs(static_cast<double>(vL.x)*static_cast<double>(a.y) - static_cast<double>(a.x)*static_cast<double>(vL.y));
            const auto squaredDistance = (nominator * nominator) / static_cast<double>(squaredLineLength);

            // If requested to check in projected point used to calculate distance is on the line itself,
            // do some extra calculations
            if (pOutInOnLine != nullptr)
            {
                // In case u is [0.0 ... 1.0], projected point is on line
                // uNominator = (p.x - l0.x)*vL.x + (p.y - l0.y)*vL.y
                // Since p became center of the coordinate system, formula above can be rewritten as
                // uNominator = (-l0.x)*vL.x + (-l0.y)*vL.y
                const auto uDotProduct = static_cast<double>(-a.x)*static_cast<double>(vL.x) + static_cast<double>(-a.y)*static_cast<double>(vL.y);
                const auto u = static_cast<double>(uDotProduct) / static_cast<double>(squaredLineLength);
                *pOutInOnLine = (u >= 0.0 && u <= 1.0);
            }

            return squaredDistance;
        }

        inline static double distanceBetweenPointAndLine(const PointI l0, const PointI l1, const PointI p, bool* pOutInOnLine = nullptr)
        {
            return qSqrt(squaredDistanceBetweenPointAndLine(l0, l1, p, pOutInOnLine));
        }

        inline static double minimalSquaredDistanceToLineSegmentFromPoint(
            const QVector<PointI>& line,
            const PointI p,
            int* pOutSegmentIndex0 = nullptr,
            int* pOutSegmentIndex1 = nullptr)
        {
            const auto pointsCount = line.size();
            double squaredMinDistance = std::numeric_limits<double>::max();
            *pOutSegmentIndex0 = -1;
            *pOutSegmentIndex1 = -1;

            // Find segment that is nearest to point p using only projected-p that is contained by the line
            auto pCurrentPoint = line.constData();
            auto pPrevPoint = pCurrentPoint++;
            for (auto currentPointIdx = 1; currentPointIdx < pointsCount; currentPointIdx++, pPrevPoint++, pCurrentPoint++)
            {
                bool isOnLine = false;
                const auto squaredDistance = squaredDistanceBetweenPointAndLine(*pPrevPoint, *pCurrentPoint, p, &isOnLine);
                if (isOnLine && squaredDistance < squaredMinDistance)
                {
                    *pOutSegmentIndex0 = currentPointIdx - 1;
                    *pOutSegmentIndex1 = currentPointIdx;
                    squaredMinDistance = squaredDistance;
                }
            }

            // Find segment that is nearest to point p using only points from the line
            pCurrentPoint = line.constData();
            for (auto currentPointIdx = 0; currentPointIdx < pointsCount; currentPointIdx++, pCurrentPoint++)
            {
                const auto squaredDistance = (PointI64(*pCurrentPoint) - PointI64(p)).squareNorm();
                if (squaredDistance < squaredMinDistance)
                {
                    *pOutSegmentIndex0 = currentPointIdx;
                    *pOutSegmentIndex1 = currentPointIdx;
                    squaredMinDistance = squaredDistance;
                }
            }

            return squaredMinDistance;
        }

        inline static double minimalDistanceToLineSegmentFromPoint(
            const QVector<PointI>& line,
            const PointI p,
            int* pOutSegmentIndex0 = nullptr,
            int* pOutSegmentIndex1 = nullptr)
        {
            return qSqrt(minimalSquaredDistanceToLineSegmentFromPoint(line, p, pOutSegmentIndex0, pOutSegmentIndex1));
        }

        inline static double degreesDiff(const double a1, const double a2)
        {
            auto diff = a1 - a2;
            while (diff > 180.0)
                diff -= 360.0;
            while (diff <= -180.0)
                diff += 360.0;
            return diff;
        }

#if !defined(SWIG)
        inline static AreaI64 boundingBox31FromAreaInMeters(const double radiusInMeters, const PointI center31)
        {
            const auto metersPerUnit = getMetersPerTileUnit(
                ZoomLevel31,
                center31.y,
                1);
            const auto size = static_cast<int32_t>((radiusInMeters / metersPerUnit) * 2.0);

            return AreaI64::fromCenterAndSize(center31.x, center31.y, size, size);
        }

        inline static AreaI boundingBox31FromLatLon(const LatLon topLeft, const LatLon bottomRight)
        {
            return AreaI(Utilities::convertLatLonTo31(topLeft), Utilities::convertLatLonTo31(bottomRight));
        }
#endif // !defined(SWIG)

        inline static AreaI tileBoundingBox31(const TileId tileId, const ZoomLevel zoom)
        {
            AreaI output;

            const auto zoomShift = ZoomLevel31 - zoom;

            output.top() = tileId.y << zoomShift;
            output.left() = tileId.x << zoomShift;
            output.bottom() = (((int64_t)tileId.y + 1) << zoomShift) - 1;
            output.right() = (((int64_t)tileId.x + 1) << zoomShift) - 1;

            assert(output.top() >= 0 && output.top() <= std::numeric_limits<int32_t>::max());
            assert(output.left() >= 0 && output.left() <= std::numeric_limits<int32_t>::max());
            assert(output.bottom() >= 0 && output.bottom() <= std::numeric_limits<int32_t>::max());
            assert(output.right() >= 0 && output.right() <= std::numeric_limits<int32_t>::max());
            assert(output.right() >= output.left());
            assert(output.bottom() >= output.top());

            return output;
        }

        inline static AreaI roundBoundingBox31(const AreaI bbox31, const ZoomLevel zoom)
        {
            const auto tilesCount = static_cast<int32_t>(1u << static_cast<unsigned int>(ZoomLevel31 - zoom));
            const auto roundingMask = static_cast<uint32_t>(tilesCount - 1);

            AreaI roundedBBox31;

            roundedBBox31.top() = bbox31.top() & ~roundingMask;

            roundedBBox31.left() = bbox31.left() & ~roundingMask;

            roundedBBox31.bottom() = bbox31.bottom() & ~roundingMask;
            if ((bbox31.bottom() & roundingMask) != 0)
                roundedBBox31.bottom() += tilesCount;
            roundedBBox31.bottom() -= 1;

            roundedBBox31.right() = bbox31.right() & ~roundingMask;
            if ((bbox31.right() & roundingMask) != 0)
                roundedBBox31.right() += tilesCount;
            roundedBBox31.right() -= 1;

            return roundedBBox31;
        }

        inline static uint32_t interleaveBy1(const uint16_t input)
        {
            auto output = static_cast<uint32_t>(input);
            output = (output ^ (output << 8)) & 0x00ff00ff;
            output = (output ^ (output << 4)) & 0x0f0f0f0f;
            output = (output ^ (output << 2)) & 0x33333333;
            output = (output ^ (output << 1)) & 0x55555555;
            return output;
        }

        inline static uint32_t encodeMortonCode(const uint16_t x, const uint16_t y)
        {
            return interleaveBy1(x) | (interleaveBy1(y) << 1);
        }

        inline static uint16_t deinterleaveBy1(const uint32_t input)
        {
            auto output = input & 0x55555555;
            output = (output ^ (output >> 1)) & 0x33333333;
            output = (output ^ (output >> 2)) & 0x0f0f0f0f;
            output = (output ^ (output >> 4)) & 0x00ff00ff;
            output = (output ^ (output >> 8)) & 0x0000ffff;
            return static_cast<uint16_t>(output);
        }

        inline static void decodeMortonCode(const uint32_t code, uint16_t& outX, uint16_t& outY)
        {
            outX = deinterleaveBy1(code);
            outY = deinterleaveBy1(code >> 1);
        }

        inline static QVector<PointI> simplifyPathOutsideBBox(const QVector<PointI>& path31, const AreaI& bbox31)
        {
            const auto pointsCount = path31.size();
            if (pointsCount < 2)
                return path31;
            QVector<PointI> result;
            result.reserve(pointsCount);
            auto pPoint31 = path31.constData();
            const auto left = bbox31.left();
            const auto right = bbox31.right();
            const auto top = bbox31.top();
            const auto bottom = bbox31.bottom();
            int x, y, prevX, prevY, code;
            int prevCode = 0;
            bool skipped = false;
            for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++)
            {
                x = pPoint31->x;
                y = pPoint31->y;
                code = (x < left ? 1 : (x > right ? 2 : 0)) | (y < top ? 4 : (y > bottom ? 8 : 0));
                if (code != 0 && (code & prevCode) != 0)
                    skipped = true;
                else
                {
                    if (skipped)
                    {
                        result.resize(result.size() + 1);
                        result.last().x = prevX;
                        result.last().y = prevY;
                        skipped = false;
                    }
                    result.resize(result.size() + 1);
                    result.last().x = x;
                    result.last().y = y;
                    prevCode = code;
                }
                prevX = x;
                prevY = y;
                pPoint31++;
            }
            if (skipped)
            {
                result.resize(result.size() + 1);
                result.last().x = prevX;
                result.last().y = prevY;
            }
            return result;
        }

        inline static AreaI getEnlargedPrimitivesArea(const AreaI& area31)
        {
            // Enlarge area in which objects will be primitivised.
            // It allows to properly draw polylines on tile bounds
            const auto enlarge31X = qMin(
                Utilities::metersToX31(MAX_ENLARGE_PRIMITIVIZED_AREA_METERS),
                static_cast<int64_t>(area31.width() * ENLARGE_PRIMITIVIZED_AREA_COEFF));
            const auto enlarge31Y = qMin(
                Utilities::metersToY31(MAX_ENLARGE_PRIMITIVIZED_AREA_METERS),
                static_cast<int64_t>(area31.height() * ENLARGE_PRIMITIVIZED_AREA_COEFF));
            return area31.getEnlargedBy(PointI(enlarge31X, enlarge31Y));
        }

        inline static AreaI getEnlargedVisibleArea(const AreaI& area31)
        {
            auto enlargedArea31 = Utilities::getEnlargedPrimitivesArea(area31);
            const auto enlarge31X = static_cast<int64_t>(enlargedArea31.width() * ENLARGE_VISIBLE_AREA_COEFF);
            const auto enlarge31Y = static_cast<int64_t>(enlargedArea31.height() * ENLARGE_VISIBLE_AREA_COEFF);
            return enlargedArea31.getEnlargedBy(PointI(enlarge31X, enlarge31Y));
        }

        static QVector<TileId> getAllMetaTileIds(const TileId anyMetaTileId)
        {
            return getMetaTileIds(TileId::fromXY(anyMetaTileId.x >> 1, anyMetaTileId.y >> 1));
        }

        static QVector<TileId> getMetaTileIds(const TileId overscaledTileId)
        {
            assert(overscaledTileId.x <= std::numeric_limits<int32_t>::max() >> 2);
            assert(overscaledTileId.y <= std::numeric_limits<int32_t>::max() >> 2);

            const auto unevenTileId = TileId::fromXY(overscaledTileId.x << 1, overscaledTileId.y << 1);
            QVector<TileId> metaTileIds(4);
            metaTileIds[0] = unevenTileId + TileId::fromXY(0, 0);
            metaTileIds[1] = unevenTileId + TileId::fromXY(1, 0);
            metaTileIds[2] = unevenTileId + TileId::fromXY(0, 1);
            metaTileIds[3] = unevenTileId + TileId::fromXY(1, 1);

            return metaTileIds;
        }

        static QVector<TileId> getTileIdsUnderscaledByZoomShift(
            const TileId tileId,
            const unsigned int absZoomShift)
        {
            if (absZoomShift == 0)
            {
                QVector<TileId> result(1);
                result[0] = tileId;
                return result;
            }
            const auto resultingTilesPerSideCount = (1u << absZoomShift);
            const auto resultingTilesCount = resultingTilesPerSideCount * resultingTilesPerSideCount;
            assert(resultingTilesCount <= std::numeric_limits<uint16_t>::max());

            TileId originTileId = tileId;
            originTileId.x <<= absZoomShift;
            originTileId.y <<= absZoomShift;
            QVector<TileId> resultingTiles(resultingTilesCount);
            const auto pResultingTiles = resultingTiles.data();
            for (auto x = 0u; x < resultingTilesPerSideCount; x++)
                for (auto y = 0u; y < resultingTilesPerSideCount; y++)
                    pResultingTiles[encodeMortonCode(x, y)] = originTileId + TileId::fromXY(x, y);

            return resultingTiles;
        }

        inline static TileId getTileIdOverscaledByZoomShift(
            const TileId tileId,
            const unsigned int absZoomShift,
            PointF* outNOffsetInTile = nullptr,
            PointF* outNSizeInTile = nullptr)
        {
            TileId shiftedTileId = tileId;
            shiftedTileId.x >>= absZoomShift;
            shiftedTileId.y >>= absZoomShift;

            if (outNOffsetInTile || outNSizeInTile)
            {
                PointF nOffsetInTile;
                PointF nSizeInTile;
                if (absZoomShift < 20)
                {
                    nSizeInTile.x = nSizeInTile.y = 1.0f / (1u << absZoomShift);
                    nOffsetInTile.x = static_cast<float>(tileId.x - (shiftedTileId.x << absZoomShift)) * nSizeInTile.x;
                    nOffsetInTile.y = static_cast<float>(tileId.y - (shiftedTileId.y << absZoomShift)) * nSizeInTile.y;
                }
                else
                {
                    nSizeInTile.x = nSizeInTile.y = 1.0 / static_cast<double>(1ull << absZoomShift);
                    nOffsetInTile.x = static_cast<double>(tileId.x - (shiftedTileId.x << absZoomShift)) * nSizeInTile.x;
                    nOffsetInTile.y = static_cast<double>(tileId.y - (shiftedTileId.y << absZoomShift)) * nSizeInTile.y;
                }
                if (outNOffsetInTile)
                    *outNOffsetInTile = nOffsetInTile;
                if (outNSizeInTile)
                    *outNSizeInTile = nSizeInTile;
            }

            return shiftedTileId;
        }

        inline static AreaI areaRightShift(const AreaI& input, const uint32_t shift)
        {
            AreaI output;
            uint32_t tail;

            output.top() = input.top() >> shift;
            output.left() = input.left() >> shift;

            tail = input.bottom() & ((1u << shift) - 1);
            output.bottom() = (input.bottom() >> shift) + (tail ? 1 : 0);
            tail = input.right() & ((1u << shift) - 1);
            output.right() = (input.right() >> shift) + (tail ? 1 : 0);

            assert(output.top() >= 0 && output.top() <= std::numeric_limits<int32_t>::max());
            assert(output.left() >= 0 && output.left() <= std::numeric_limits<int32_t>::max());
            assert(output.bottom() >= 0 && output.bottom() <= std::numeric_limits<int32_t>::max());
            assert(output.right() >= 0 && output.right() <= std::numeric_limits<int32_t>::max());
            assert(output.right() >= output.left());
            assert(output.bottom() >= output.top());

            return output;
        }

        inline static AreaI areaLeftShift(const AreaI& input, const uint32_t shift)
        {
            AreaI output;

            output.top() = input.top() << shift;
            output.left() = input.left() << shift;
            output.bottom() = input.bottom() << shift;
            output.right() = input.right() << shift;

            assert(output.top() >= 0 && output.top() <= std::numeric_limits<int32_t>::max());
            assert(output.left() >= 0 && output.left() <= std::numeric_limits<int32_t>::max());
            assert(output.bottom() >= 0 && output.bottom() <= std::numeric_limits<int32_t>::max());
            assert(output.right() >= 0 && output.right() <= std::numeric_limits<int32_t>::max());
            assert(output.right() >= output.left());
            assert(output.bottom() >= output.top());

            return output;
        }

        inline static uint32_t getNextPowerOfTwo(const uint32_t value)
        {
            if (value == 0)
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

        inline static uint32_t getPreviousPowerOfTwo(const uint32_t value)
        {
            auto n = value;

            n = n | (n >> 1);
            n = n | (n >> 2);
            n = n | (n >> 4);
            n = n | (n >> 8);
            n = n | (n >> 16);

            return n - (n >> 1);
        }

        inline static int commonDivisor(const int x, const int y)
        {
            assert(x != 0 || y != 0);
            
            return y == 0
                ? qAbs(x)
                : commonDivisor(y, x % y);
        }

        inline static double getMetersPerTileUnit(const float zoom, const double yTile, const double unitsPerTile)
        {
            // Equatorial circumference of the Earth in meters
            const static double C = 40075017.0;

            const auto powZoom = getPowZoom(zoom);
            const auto sinhValue = sinh((2.0 * M_PI * yTile) / powZoom - M_PI);
            const auto res = C / (powZoom * unitsPerTile * qSqrt(sinhValue*sinhValue + 1.0));

            return res;
        }

        static PointD getTileEllipsoidNumberAndOffsetY(int zoom, double latitude, int tileSize);

        static TileId getTileId(const PointI& point31, ZoomLevel zoom, PointF* pOutOffsetN = nullptr, PointI* pOutOffset = nullptr);
        static TileId normalizeTileId(TileId input, ZoomLevel zoom);
        static PointI normalizeCoordinates(const PointI& input, ZoomLevel zoom);
#if !defined(SWIG)
        static PointI normalizeCoordinates(const PointI64& input, ZoomLevel zoom);
#endif // !defined(SWIG)

        inline static PointI wrapCoordinates(const PointI64& input)
        {
            const int64_t negMask = INT64_MAX ^ INT32_MAX;
            const int64_t posMask = INT32_MAX;
            const PointI64 output(
                input.x < 0 ? input.x | negMask : input.x & posMask,
                input.y < 0 ? input.y | negMask : input.y & posMask);
            return PointI(static_cast<int32_t>(output.x), static_cast<int32_t>(output.y));
        }

        inline static PointI shortestVector31(const PointI& offset)
        {
            return shortestVector(offset, ZoomLevel::ZoomLevel31);
        }

        inline static PointI shortestVector(const PointI& offset_, const ZoomLevel zoomLevel)
        {
            const int32_t maxTileNumber = static_cast<int32_t>((1u << zoomLevel) - 1);
            const int middleTileNumber = maxTileNumber / 2 + 1;
            PointI offset = offset_;
            if (offset.x >= middleTileNumber)
                offset.x = offset.x - maxTileNumber - 1;
            else if (offset.x < -middleTileNumber)
                offset.x = offset.x + maxTileNumber + 1;
            if (offset.y >= middleTileNumber)
                offset.y = offset.y - maxTileNumber - 1;
            else if (offset.y < -middleTileNumber)
                offset.y = offset.y + maxTileNumber + 1;
            return offset;
        }

        inline static PointI shortestVector31(const PointI& p0, const PointI& p1)
        {
            return shortestVector31(p1 - p0);
        }

        inline static PointI shortestLongitudeVector(const PointI& offset)
        {
            const int intHalf = INT32_MAX / 2 + 1;
            PointI offset31 = offset;
            if (offset31.x >= intHalf)
                offset31.x = offset31.x - INT32_MAX - 1;
            else if (offset31.x < -intHalf)
                offset31.x = offset31.x + INT32_MAX + 1;
            return offset31;
        }

        inline static glm::dvec3 getGlobeRadialVector(const PointD& angles)
        {
            const auto csy = qCos(angles.y);
            const glm::dvec3 result(csy * qSin(angles.x), csy * qCos(angles.x), -qSin(angles.y));
            return result;
        }

        inline static glm::dvec3 getGlobeRadialVector(const PointI& location31, PointD* outAngles = nullptr)
        {
            const auto angles = getAnglesFrom31(location31);
            const auto result = getGlobeRadialVector(angles);
            if (outAngles != nullptr)
                *outAngles = angles;
            return result;
        }

        inline static glm::vec3 planeWorldCoordinates(
            const PointI& location31,
            const PointI& target31,
            const ZoomLevel zoomLevel,
            const float tileSizeInWorld,
            const double elevation)
        {
            const auto offset31 = shortestVector31(target31, location31);
            const auto offsetFromTarget = convert31toFloat(offset31, zoomLevel) * tileSizeInWorld;
            return glm::vec3(offsetFromTarget.x, static_cast<float>(elevation), offsetFromTarget.y);
        }

        inline static glm::vec3 sphericalWorldCoordinates(
            const PointI& location31,
            const glm::dmat3& mGlobeRotation,
            const double globeRadius,
            const double elevation,
            PointD* outAngles = nullptr)
        {
            const auto n = mGlobeRotation * getGlobeRadialVector(location31, outAngles);
            const auto v = n * (globeRadius + elevation);
            return glm::vec3(v.x, v.y - globeRadius, v.z);
        }

        inline static double snapToGridDecimal(const double value)
        {
            // Snap value to the closest number from range [1, 2, 5, 10] * (10 ^ n)
            double factor = std::pow(10.0, std::floor(log10(value)));
            double result = value / factor;
            if (result < 2.0)
                result = 1.0;
            else if (result < 5.0)
                result = 2.0;
            else if (result < 10.0)
                result = 5.0;
            else
                result = 10.0;
            result *= factor;
            return result;
        }

        inline static double snapToGridSexagesimal(const double value)
        {
            // Snap value to the closest number from range [1, 2, 3, 4, 5, 6, 10, 12, 15, 20, 30, 60]
            double result = value;
            if (result < 6.0)
                result = std::round(result);
            else if (result < 10.0)
                result = 6.0;
            else if (result < 12.0)
                result = 10.0;
            else if (result < 15.0)
                result = 12.0;
            else if (result < 20.0)
                result = 15.0;
            else if (result < 30.0)
                result = 20.0;
            else if (result < 60.0)
                result = 30.0;
            else
                result = 60.0;
            return result;
        }

        inline static double snapToGridDMS(const double value)
        {
            // Snap value to the closest degree:minute:second.n
            if (value >= 1.0)
                return snapToGridDecimal(value);
            double factor = 60.0;
            double gap = value * factor;
            if (gap >= 1.0)
                return snapToGridSexagesimal(gap) / factor;
            factor *= 60.0;
            gap = value * factor;
            if (gap >= 1.0)
                return snapToGridSexagesimal(gap) / factor;
            factor *= 10.0;
            gap = value * factor;
            return snapToGridDecimal(gap) / factor;
        }

        inline static double snapToGridDM(const double value)
        {
            // Snap value to the closest degree:minute.n
            if (value >= 1.0)
                return snapToGridDecimal(value);
            double factor = 60.0;
            double gap = value * factor;
            if (gap >= 1.0)
                return snapToGridSexagesimal(gap) / factor;
            factor *= 10.0;
            gap = value * factor;
            return snapToGridDecimal(gap) / factor;
        }

        enum class CHCode : uint8_t
        {
            Left = 0,
            Right,
            Bottom,
            Top,
        };
        typedef Bitmask<CHCode> CHValue;

        template<typename T>
        inline static CHValue computeCohenSutherlandValue(const Point<T>& p, const Area<T>& box)
        {
            CHValue value;

            if (p.x < box.left())           // to the left of clip box
                value |= CHCode::Left;
            else if (p.x > box.right())     // to the right of clip box
                value |= CHCode::Right;
            if (p.y < box.top())            // above the clip box
                value |= CHCode::Bottom;
            else if (p.y > box.bottom())    // below the clip box
                value |= CHCode::Top;

            return value;
        }

        static int extractFirstInteger(const QString& s);
        static int extractIntegerNumber(const QString& s);
        static bool extractFirstNumberPosition(const QString& value, int& first, int& last, bool allowSigned, bool allowDot);
        static double parseSpeed(const QString& value, const double defValue, bool* wasParsed = nullptr);
        static double parseLength(const QString& value, const double defValue, bool* wasParsed = nullptr);
        static double parseWeight(const QString& value, const double defValue, bool* wasParsed = nullptr);
        static ColorARGB parseColor(const QString& value, const ColorARGB defValue, bool* wasParsed = nullptr);
        static bool isColorBright(const ColorARGB color);
        static bool isColorBright(const ColorRGB color);
        static int parseArbitraryInt(const QString& value, const int defValue, bool* wasParsed = nullptr);
        static long parseArbitraryLong(const QString& value, const long defValue, bool* wasParsed = nullptr);
        static unsigned int parseArbitraryUInt(const QString& value, const unsigned int defValue, bool* wasParsed = nullptr);
        static unsigned long parseArbitraryULong(const QString& value, const unsigned long defValue, bool* wasParsed = nullptr);
        static float parseArbitraryFloat(const QString& value, const float defValue, bool* wasParsed = nullptr);
        static bool parseArbitraryBool(const QString& value, const bool defValue, bool* wasParsed = nullptr);

        static int javaDoubleCompare(const double l, const double r);
        static void findFiles(
            const QDir& origin,
            const QStringList& masks,
            QFileInfoList& files,
            const bool recursively = true);
        static void findDirectories(
            const QDir& origin,
            const QStringList& masks,
            QFileInfoList& directories,
            const bool recursively = true);

        inline static QSet<ZoomLevel> enumerateZoomLevels(const ZoomLevel from, const ZoomLevel to)
        {
            QSet<ZoomLevel> result;
            result.reserve(to - from + 1);
            for (int level = from; level <= to; level++)
                result.insert(static_cast<ZoomLevel>(level));

            return result;
        }
        static QString stringifyZoomLevels(const QSet<ZoomLevel>& zoomLevels);

        static QString getQuadKey(const uint32_t x, const uint32_t y, const uint32_t z);

        struct ItemPointOnPath Q_DECL_FINAL
        {
            int priority;
            float itemCenterOffset;
            float itemCenterN;

            struct PriorityComparator Q_DECL_FINAL
            {
                inline bool operator()(const ItemPointOnPath& l, const ItemPointOnPath& r) const
                {
                    return (l.priority < r.priority);
                }
            };
        };
        static QList<ItemPointOnPath> calculateItemPointsOnPath(
            const float pathLength,
            const float itemLength,
            const float padding = 0.0f,
            const float spacing = 0.0f);

        static QString resolveColorFromPalette(const QString& input, const bool usePalette6);
        static LatLon rhumbDestinationPoint(LatLon latLon, double distance, double bearing);
        static LatLon rhumbDestinationPoint(double lat, double lon, double distance, double bearing);
        static std::pair<int, int> calculateFinalXYFromBaseAndPrecisionXY(int bazeZoom, int finalZoom, int precisionXY,
                                                                          int xBase, int yBase, bool ignoreNotEnoughPrecision);
        static bool isPointInsidePolygon(const PointI point,
                                                        const QVector<PointI> &polygon);

        inline static void resizeVector(const PointF& start, PointF& end, float sizeIncrement)
        {
            auto vec = end - start;
            auto vecLength = vec.norm();
            auto vecDir = vec / vecLength;
            end = start + vecDir * (vecLength + sizeIncrement);
        }

        inline static PointF computeNormalToLine(const PointF& start, const PointF& end, bool clockwise)
        {
            auto normal = (end - start).normalized();
            PointF res;
            if (clockwise)
            {
                res.x = normal.y;
                res.y = -normal.x;
            }
            else
            {
                res.x = -normal.y;
                res.y = normal.x;
            }
            return res;
        }

        inline static glm::vec3 calculatePlaneN(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
        {
            const auto ab = b - a;
            const auto ac = c - a;

            const auto x = ab.y * ac.z - ab.z * ac.y;
            const auto y = ab.z * ac.x - ab.x * ac.z;
            const auto z = ab.x * ac.y - ab.y * ac.x;
            return glm::normalize(glm::vec3(x, y, z));
        }
        
        static bool calculateIntersection(const PointI64& p1, const PointI64& p0, const AreaI& bbox, PointI64& pX);
        static bool calculateIntersection(const PointI& p1, const PointI& p0, const AreaI& bbox, PointI& pX);
        
        static double calculateShortestPath(const PointI64& start64, const PointI& start31, const PointI& finish31,
            PointI64& minCoordinates, PointI64& maxCoordinates, QVector<PointI64>* path = nullptr);

        // Log formatted coordinates for https://www.gpsvisualizer.com/
        inline static void logDebugTileBBox(
            const TileId tileId,
            const ZoomLevel zoom,
            const QString& name = "bbox",
            const QString& color = "red")
        {
            QVector<PointI> path;
            path.push_back(PointI(tileId.x + 0, tileId.y + 0));
            path.push_back(PointI(tileId.x + 1, tileId.y + 0));
            path.push_back(PointI(tileId.x + 1, tileId.y + 1));
            path.push_back(PointI(tileId.x + 0, tileId.y + 1));
            path.push_back(path.front());
            logDebugPath(path, zoom, name, color);
        }

        inline static void logDebugBBox(
            const AreaI& bbox,
            const ZoomLevel zoom = ZoomLevel31,
            const QString& name = "bbox",
            const QString& color = "red")
        {
            QVector<PointI> path;
            path.push_back(bbox.topLeft);
            path.push_back(bbox.topRight());
            path.push_back(bbox.bottomRight);
            path.push_back(bbox.bottomLeft());
            path.push_back(path.first());
            logDebugPath(path, zoom, name, color);
        }

        inline static void logDebugPath(
            const QVector<PointI>& path,
            const ZoomLevel zoom = ZoomLevel31,
            const QString& name = "path",
            const QString& color = "green")
        {
            LogPrintf(LogSeverityLevel::Debug, "type, lat, lon, name, color");
            for (auto i = 0; i < path.size(); i++)
            {
                const auto point = path[i];
                const auto lat = getLatitudeFromTile(zoom, point.y);
                const auto lon = getLongitudeFromTile(zoom, point.x);
                if (i == 0)
                {
                    LogPrintf(LogSeverityLevel::Debug, qPrintable(QString::fromLatin1("T, %1, %2, %3, %4")
                        .arg(lat, 2, 'f', 10, '0')
                        .arg(lon, 2, 'f', 10, '0')
                        .arg(name)
                        .arg(color)));
                }
                else
                {
                    LogPrintf(LogSeverityLevel::Debug, qPrintable(QString::fromLatin1("T, %1, %2")
                    .arg(lat, 2, 'f', 10, '0')
                    .arg(lon, 2, 'f', 10, '0')));
                }
            }
        }

        inline static void logDebugPoint(
            const PointI point,
            const ZoomLevel zoom = ZoomLevel31,
            const QString& name = "point",
            const QString& color = "blue")
        {
            LogPrintf(LogSeverityLevel::Debug, "type, lat, lon, name, color");
            const auto lat = getLatitudeFromTile(zoom, point.y);
            const auto lon = getLongitudeFromTile(zoom, point.x);
            LogPrintf(LogSeverityLevel::Debug, qPrintable(QString::fromLatin1("W, %1, %2, %3, %4")
                    .arg(lat, 2, 'f', 10, '0')
                    .arg(lon, 2, 'f', 10, '0')
                    .arg(name)
                    .arg(color)));
        }
        
        inline static bool contains(const QVector<PointI> &polygon, const PointI &point) {
            return countIntersections(polygon, point) % 2 == 1;
        }
        
        inline static QString splitAndClearRepeats(const QString & ref, const QString & symbol)
        {
            QList<QString> arr = ref.split(symbol);
            QString res = "";
            QString prev = "";
            for (const QString & s : arr)
            {
                if (s.isNull() || s.isEmpty() || prev == s)
                    continue;
                if (!res.isEmpty())
                {
                    res += symbol;
                }
                res += s;
                prev = s;
            }
            return res;
        }

    private:
        Utilities();
        ~Utilities();
        static int countIntersections(const QVector<PointI> &points, const PointI &point);
    };
}

#endif // !defined(_OSMAND_CORE_UTILITIES_H_)
