#include "Building.h"

#include "ObfStreetGroup.h"
#include "ObfStreet.h"
#include "StreetGroup.h"
#include "Street.h"


OsmAnd::Building::Building(
        std::shared_ptr<const OsmAnd::ObfStreet> street_,
        QString nativeName,
        QHash<QString, QString> localizedNames,
        OsmAnd::PointI position31,
        OsmAnd::Building::Interpolation interpolation_,
        QString postcode_)
    : Address(AddressType::Building, nativeName, localizedNames, position31)
    , street(street_)
    , streetGroup(street->obfStreetGroup)
    , interpolation(interpolation_)
    , postcode(postcode_)
{

}

OsmAnd::Building::Building(
        std::shared_ptr<const OsmAnd::ObfStreetGroup> streetGroup_,
        QString nativeName,
        QHash<QString, QString> localizedNames,
        OsmAnd::PointI position31,
        OsmAnd::Building::Interpolation interpolation_,
        QString postcode_)
    : Address(AddressType::Building, nativeName, localizedNames, position31)
    , street(nullptr)
    , streetGroup(streetGroup_)
    , interpolation(interpolation_)
    , postcode(postcode_)
{

}

QString OsmAnd::Building::toString() const
{
    return nativeName;
}

OsmAnd::Building::Interpolation::Interpolation(
        OsmAnd::Building::Interpolation::Type type_,
        QString nativeName,
        QHash<QString, QString> localizedNames,
        OsmAnd::PointI position31)
    : Address(AddressType::Building, nativeName, localizedNames, position31)
    , type(type_)
{

}

QString OsmAnd::Building::Interpolation::toString() const
{
    return QStringLiteral("interpolation ") + nativeName;
}
