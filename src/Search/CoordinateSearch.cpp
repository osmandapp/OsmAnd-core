#include <OsmAndCore/Search/CoordinateSearch.h>
#include <GeographicLib/GeoCoords.hpp>

#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>

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

        for (auto itemName : QList<QString>({"saddr", "daddr", "q", "c"}))
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
