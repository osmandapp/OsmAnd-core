#include "Building.h"

#include "ObfStreetGroup.h"
#include "ObfStreet.h"
#include "StreetGroup.h"
#include "Street.h"


OsmAnd::Building::Building(
        QString nativeName,
        QHash<QString, QString> localizedNames,
        OsmAnd::PointI position31,
        OsmAnd::Building::Interpolation interpolation,
        QString postcode)
    : Address(nativeName, localizedNames, position31)
    , _interpolation(interpolation)
    , _postcode(postcode)
{

}

QString OsmAnd::Building::toString() const
{
    return _nativeName;
}

OsmAnd::Building::Interpolation OsmAnd::Building::interpolation() const
{
    return _interpolation;
}

QString OsmAnd::Building::postcode() const
{
    return _postcode;
}

OsmAnd::Building::Interpolation::Interpolation(
        OsmAnd::Building::Interpolation::Type type_,
        QString nativeName,
        QHash<QString, QString> localizedNames,
        OsmAnd::PointI position31)
    : Address(nativeName, localizedNames, position31)
    , type(type_)
{

}

QString OsmAnd::Building::Interpolation::toString() const
{
    return QStringLiteral("interpolation ") + _nativeName;
}
