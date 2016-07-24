#include "StreetIntersection.h"
#include "ObfStreet.h"

OsmAnd::StreetIntersection::StreetIntersection(
        std::shared_ptr<const Street> street,
        QString nativeName,
        QHash<QString, QString> localizedNames,
        OsmAnd::PointI position31)
    : Address(nativeName, localizedNames, position31)
    , _street(street)
{

}

QString OsmAnd::StreetIntersection::toString() const
{
    return "intersection " + _nativeName + " (" + _street->nativeName();
}

OsmAnd::Street OsmAnd::StreetIntersection::street() const
{
    return *_street;
}

std::shared_ptr<const OsmAnd::Street> OsmAnd::StreetIntersection::streetPtr() const
{
    return _street;
}
