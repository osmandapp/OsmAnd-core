#include <OsmAndCore/Search/CoordinateSearch.h>
#include <GeographicLib/GeoCoords.hpp>

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>

QUrl OsmAnd::CoordinateSearch::toUrl(QString s)
{
    if (s.startsWith("http://") || s.startsWith("https://"))
    {
        QUrl result = QUrl(s);
        if (result.isValid())
            return result;
    }
    return QUrl();
}

OsmAnd::LatLon OsmAnd::CoordinateSearch::search(QString query)
{
    OsmAnd::LatLon result = OsmAnd::LatLon();
    query = query.simplified();

    QUrl url = toUrl(query);
    if (!url.isEmpty())
    {
        // Extract coordinates from urls

        // http://osmand.net/go?lat=34&lon=-106&z=11
        QUrlQuery urlQuery = QUrlQuery(url);
        QString latitude = urlQuery.queryItemValue("lat"), longitude = urlQuery.queryItemValue("lon");
        bool latitudeOk, longitudeOk;
        result.latitude = latitude.toDouble(&latitudeOk);
        result.longitude = longitude.toDouble(&longitudeOk);
        if (latitudeOk && longitudeOk)
            return result;

        // https://www.openstreetmap.org/#map=15/50.9307/4.7201
        QString fragment = url.fragment();
        auto converted = QList<double>();
        for (auto part : fragment.split("/"))
        {
            bool ok;
            double d = part.toDouble(&ok);
            if (ok)
                converted.append(d);
        }
        if (converted.size() == 2)
            return OsmAnd::LatLon(converted[0], converted[1]);
    }

    if (query.startsWith("geo:"))
    {
        query = query.replace("geo:", "");
        QString coordinates = "";
        for (QChar ch : query)
        {
            if (ch.isNumber() || ch == '-' || ch == ',' || ch == '.')
                coordinates.append(ch);
            else
                break;
        }
        query = coordinates;
    }

    GeographicLib::GeoCoords geoCoords;
    geoCoords.Reset(query.toStdString());
    return LatLon(geoCoords.Latitude(), geoCoords.Longitude());
}
