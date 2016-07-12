#include "Building.h"

#include "Street.h"
#include "StreetGroup.h"


OsmAnd::Building::Building(
        std::shared_ptr<const OsmAnd::ObfStreet> street,
        QString nativeName,
        QHash<QString, QString> localizedNames,
        OsmAnd::PointI position31,
        OsmAnd::Building::Interpolation interpolation,
        QString postcode)
    : Address(AddressType::Building, nativeName, localizedNames, position31)
    , interpolation(interpolation_)
    , street(street_)
    , streetGroup(street->streetGroup)
{

}

OsmAnd::Building::Building(
        std::shared_ptr<const OsmAnd::StreetGroup> streetGroup,
        QString nativeName,
        QHash<QString, QString> localizedNames,
        OsmAnd::PointI position31,
        OsmAnd::Building::Interpolation interpolation,
        QString postcode)
    : Address(AddressType::Building, nativeName, localizedNames, position31)
    , interpolation(interpolation_)
    , street(nullptr)
    , streetGroup(streetGroup_)
{

}

OsmAnd::Building::~Building()
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
