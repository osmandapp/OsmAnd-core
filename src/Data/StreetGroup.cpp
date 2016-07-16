#include "Address.h"
#include "StreetGroup.h"

#include <QStringBuilder>

OsmAnd::StreetGroup::StreetGroup(
        OsmAnd::ObfAddressStreetGroupType type_,
        OsmAnd::ObfAddressStreetGroupSubtype subtype_,
        QString nativeName,
        QHash<QString, QString> localizedNames,
        OsmAnd::PointI position31)
    : OsmAnd::Address(nativeName, localizedNames, position31)
    , type(type_)
    , subtype(subtype_)
{

}

QString OsmAnd::StreetGroup::toString() const
{
    return QStringLiteral("city ") % _nativeName;
}
