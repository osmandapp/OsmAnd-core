#include "CoordinateSearch.h"
#include "Logging.h"

#include <GeographicLib/GeoCoords.hpp>

#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>
#include <QVector>

#include <cmath>


OsmAnd::LatLon decodeShortLinkString(QString s)
{
    QString intToBase64{QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_~")};
    // convert old shortlink format to current one
    s = s.replace('@', '~');
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
    return {lat, lon};
}

QUrl OsmAnd::CoordinateSearch::toHttpUrl(QString const &s)
{
    if (s.startsWith(QLatin1String("http://")) || s.startsWith(QLatin1String("https://")))
    {
        QUrl result{s};
        if (result.isValid())
            return result;
    }
    return QUrl{};
}

QString OsmAnd::CoordinateSearch::withoutPrefix(QString const &query)
{
    QString result = query;
    int numberCount = 0;
    QVector<QString> prefixes{QStringLiteral("geo:"), QStringLiteral("loc:")};
    for (auto prefix : prefixes)
        if (query.startsWith(prefix))
        {
            result = result.replace(prefix, "");
            QString coordinates = "";
            for (QChar ch : result)
                if (ch.isNumber() || ch == '-' || ch == '.' || (ch == ',' && numberCount == 0))
                {
                    coordinates.append(ch);
                    if (ch == ',')
                        numberCount++;
                }
                else
                {
                    break;
                }
            result = coordinates;
        }
    return result;
}

OsmAnd::LatLon OsmAnd::CoordinateSearch::search(QString const &query)
{
    LatLon result{};
    QString q = query;
    q = q.simplified();

    QUrl url = toHttpUrl(q);
    if (!url.isEmpty())
    {
        // Extract coordinates from urls

        QString host = url.host();
        QString path = url.path();

        bool isOsmSite = host == QLatin1String("osm.org") || host.endsWith(QLatin1String("openstreetmap.org"));
        if (isOsmSite)
        {
            bool isOsmShortLink =  path.startsWith(QLatin1String("/go/"));
            if (isOsmShortLink)
            {
                QString shortLinkString = path.replace(QLatin1String("/go/"), "");
                return decodeShortLinkString(shortLinkString);
            }
        }


        // http://osmand.net/go?lat=34&lon=-106&z=11
        QUrlQuery urlQuery{url};
        QString latItemName, lonItemName;
        QVector<QVector<QString>> latLonKeyNames{
            {QStringLiteral("lat"),  QStringLiteral("lon")},
            {QStringLiteral("mlat"), QStringLiteral("mlon")},
            {QStringLiteral("y"),    QStringLiteral("x")}
        };
        for (auto keyNames : latLonKeyNames)
            if (urlQuery.hasQueryItem(keyNames[0]) && urlQuery.hasQueryItem(keyNames[1]))
            {
                latItemName = keyNames[0];
                lonItemName = keyNames[1];
                break;
            }
        if (!latItemName.isNull() && !lonItemName.isNull())
        {
            auto latitude = urlQuery.queryItemValue(latItemName);
            auto longitude = urlQuery.queryItemValue(lonItemName);
            bool latitudeOk, longitudeOk;
            result.latitude = latitude.toDouble(&latitudeOk);
            result.longitude = longitude.toDouble(&longitudeOk);
            if (latitudeOk && longitudeOk)
                return result;
        }

        // https://www.openstreetmap.org/#map=15/50.9307/4.7201
        // http://share.here.com/l/52.5134272,13.3778416,Hannah-Arendt-Stra%C3%9Fe?z=16.0&t=normal
        for (auto fragment : QVector<QString>({url.fragment(), path}))
        {
            auto parts = fragment.split(QRegExp{QLatin1String("/|,")});
            // Special case: https://www.openstreetmap.org/#15/50.9307/4.7201
            if (isOsmSite)
            {
                bool isZoomFirstInFragment;
                parts[0].toInt(&isZoomFirstInFragment);
                if (isZoomFirstInFragment)
                    parts.removeFirst();
            }
            QStringListIterator i(parts);
            QVector<double> converted;
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
                }
                else if (okPrev)
                {
                    hasCoordinates = false;
                }
            }
            if (converted.size() == 2)
                return {converted[0], converted[1]};
        }
        QVector<QString> latLonSignleKeyNames {
             QStringLiteral("q"),               // Google Maps query
             QStringLiteral("saddr"),           // Google Maps directions search
             QStringLiteral("daddr"),
             QStringLiteral("c"),               // Baidu
             QStringLiteral("ll"),              // Yandex.Maps
             QStringLiteral("map")              // Here Maps
        };
        for (auto keyName : latLonSignleKeyNames)
        {
            QString part = withoutPrefix(urlQuery.queryItemValue(keyName));
            if (!part.isEmpty() && part.contains(','))
            {
                QRegExp reBeforeSecondComma{QLatin1String("[^,]*,[^,]*")};
                reBeforeSecondComma.indexIn(part);
                q = reBeforeSecondComma.cap();
                break;
            }
        }
    }
    else
    {
        q = withoutPrefix(q);
    }

    GeographicLib::GeoCoords geoCoords;
    try
    {
        geoCoords.Reset(q.toStdString());
        return {geoCoords.Latitude(), geoCoords.Longitude()};
    }
    catch(GeographicLib::GeographicErr err)
    {
        auto errString = QStringLiteral("Error occured during coordinate parsing: ") + err.what();
        LogPrintf(LogSeverityLevel::Info, errString.toLatin1());
        return {};
    }
}
