#include <OsmAndCore/Search/GeocodingResult.h>

#include <cmath>

double OsmAnd::GeocodingResult::getDistanceP()
{
//    if (point && searchPoint)
//    if (searchPoint)
        // Need distance between searchPoint and nearest RouteSegmentPoint here, to approximate distance from neareest named road
//        return std::sqrt(point.distSquare);
//    else
//        return -1;
    return 0;
}

double OsmAnd::GeocodingResult::getDistance() const
{
    return 0;
}

QString OsmAnd::GeocodingResult::toString() const {
    QString result;
//    if (building)
//        result.append(building.getName());
//    if (street)
//        result.append(QLatin1String(" str. ")).append(street.getName()).append(QLatin1String(" city ")).append(city.getName());
//    else if (streetName)
    if (!streetName.isEmpty())
        result.append(QLatin1String(" str. ")).append(streetName);
//    else if (city)
//        result.append(QLatin1String(" city ")).append(city.getName());
//    if (connectionPoint && searchPoint)
        result.append(" dist=").append(QString::number(getDistance()));
    return result;
}
