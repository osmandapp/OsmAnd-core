#include "Address.h"
#include "Street.h"
#include "StreetGroup.h"

#include <QStringBuilder>

OsmAnd::Street::Street(QString nativeName, QHash<QString, QString> localizedNames, OsmAnd::PointI position31)
    : Address(AddressType::Street, nativeName, localizedNames, position31)
{

}

QString OsmAnd::Street::toString() const
{
    return QStringLiteral("str. ") % nativeName;
}
