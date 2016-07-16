#include "Address.h"

OsmAnd::Address::Address(
        QString nativeName,
        QHash<QString, QString> localizedNames,
        OsmAnd::PointI position31)
    : _nativeName(nativeName)
    , _localizedNames(localizedNames)
    , _position31(position31)
{

}

OsmAnd::PointI OsmAnd::Address::position31() const
{
    return _position31;
}

QHash<QString, QString> OsmAnd::Address::localizedNames() const
{
    return _localizedNames;
}

QString OsmAnd::Address::nativeName() const
{
    return _nativeName;
}
