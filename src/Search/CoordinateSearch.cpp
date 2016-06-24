#include <OsmAndCore/Search/CoordinateSearch.h>
#include <GeographicLib/GeoCoords.hpp>

#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>

#include <cmath>


OsmAnd::LatLon decodeShortLinkString(QString s)
{
    QString intToBase64 = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_~");
    // convert old shortlink format to current one
    s = s.replace("@", "~");
    int i = 0;
    long x = 0;
    long y = 0;
    int z = -8;

    for (i = 0; i < s.length(); i++)
    {
        QChar c = s.at(i);
        int digit = intToBase64.indexOf(c);
        if (digit < 0)
            break;
        // distribute 6 bits into x and y
        x <<= 3;
        y <<= 3;
        for (int j = 2; j >= 0; j--)
        {
            x |= ((digit & (1 << (j + j + 1))) == 0 ? 0 : (1 << j));
            y |= ((digit & (1 << (j + j    ))) == 0 ? 0 : (1 << j));
        }
        z += 3;
    }
    double lon = x * std::pow(2, 2 - 3 * i) * 90.0 - 180;
    double lat = y * std::pow(2, 2 - 3 * i) * 45.0 -  90;
    // adjust z
    if (i < s.length() && s[i] == '-')
    {
        z -= 2;
        if (i + 1 < s.length() && s[i + 1] == '-')
            z++;
    }
    return OsmAnd::LatLon(lat, lon);
}

QUrl OsmAnd::CoordinateSearch::toUrl(QString const &s)
{
    if (s.startsWith("http://") || s.startsWith("https://"))
    {
        QUrl result = QUrl(s);
        if (result.isValid())
            return result;
    }
    return QUrl();
}

QString OsmAnd::CoordinateSearch::withoutPrefix(QString const &query)
{
    QString result = query;
    auto prefixes = QList<QString>({"geo:", "loc:"});
    for (auto prefix : prefixes)
        if (query.startsWith(prefix))
        {
            result = result.replace(prefix, "");
            QString coordinates = "";
            for (QChar ch : result)
                if (ch.isNumber() || ch == '-' || ch == ',' || ch == '.')
                    coordinates.append(ch);
                else
                    break;
            result = coordinates;
        }
    return result;
}

OsmAnd::LatLon OsmAnd::CoordinateSearch::search(QString const &query)
{
    OsmAnd::LatLon result = OsmAnd::LatLon();
    QString q = query;
    q = q.simplified();

    QUrl url = toUrl(q);
    if (!url.isEmpty())
    {
        // Extract coordinates from urls

        QString host = url.host();
        QString path = url.path();

        if (host == "osm.org" || host.endsWith("openstreetmap.org"))
            if (path.startsWith("/go/")) // short URL form
                return decodeShortLinkString(path.replace(R"(^/go/)", ""));


        // http://osmand.net/go?lat=34&lon=-106&z=11
        QUrlQuery urlQuery = QUrlQuery(url);
        QString latItemName, lonItemName;
        for (auto names : QList<QList<QString>>({
                                                QList<QString>({"lat", "lon"}),
                                                QList<QString>({"mlat", "mlon"}
                                                               )}))
            if (urlQuery.hasQueryItem(names[0]) && urlQuery.hasQueryItem(names[1]))
            {
                latItemName = names[0];
                lonItemName = names[1];
                break;
            }
        if (!latItemName.isNull() && !lonItemName.isNull())
        {
            QString latitude = urlQuery.queryItemValue(latItemName);
            QString longitude = urlQuery.queryItemValue(lonItemName);
            bool latitudeOk, longitudeOk;
            result.latitude = latitude.toDouble(&latitudeOk);
            result.longitude = longitude.toDouble(&longitudeOk);
            if (latitudeOk && longitudeOk)
                return result;
        }

        // https://www.openstreetmap.org/#map=15/50.9307/4.7201
        QString fragment = url.fragment();
        auto parts = fragment.split(QRegExp("/|,"));
        QStringListIterator i(parts);
        QList<double> converted;
        bool hasCoordinates = true, okPrev = false, ok;
        while (hasCoordinates && i.hasNext())
        {
            double d = i.next().toDouble(&ok);
            if (ok)
            {
                okPrev = true;
                converted.append(d);
                if (converted.size() > 2)
                    hasCoordinates = false;
            } else if (okPrev) {
                hasCoordinates = false;
            }
        }
        if (converted.size() == 2)
            return OsmAnd::LatLon(converted[0], converted[1]);

        for (auto itemName : QList<QString>(
            {
             "q",               // Google Maps query
             "saddr", "daddr",  // Google Maps directions search
             "c",               // Baidu
             "ll"               // Yandex.Maps
        }))
        {
            QString part = withoutPrefix(urlQuery.queryItemValue(itemName));
            if (!part.isEmpty() && part.contains(','))
            {
                q = part;
                break;
            }
        }
    } else {
        q = withoutPrefix(q);
    }

    GeographicLib::GeoCoords geoCoords;
    try
    {
        geoCoords.Reset(q.toStdString());
        return LatLon(geoCoords.Latitude(), geoCoords.Longitude());
    }
    catch(GeographicLib::GeographicErr err)
    {
        return LatLon(0.0, 0.0);  // Probably add property "empty" to LatLon?
    }
}
