#include "StreetIntersection.h"
#include "ObfStreet.h"

OsmAnd::StreetIntersection::StreetIntersection(
        std::shared_ptr<const ObfStreet> street_,
        QString nativeName,
        QHash<QString, QString> localizedNames,
        PointI position31)
    : Address(AddressType::StreetIntersection, nativeName, localizedNames, position31)
    , street(street_)
{
}

OsmAnd::StreetIntersection::~StreetIntersection()
{
}
