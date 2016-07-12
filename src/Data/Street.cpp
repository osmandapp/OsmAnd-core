#include "Street.h"
#include "StreetGroup.h"

#include <QStringBuilder>

OsmAnd::Street::Street(QString nativeName, QHash<QString, QString> localizedNames, OsmAnd::PointI position31)
    : _nativeName(nativeName)
    , _localizedNames(localizedNames)
    , _position31(position31)
{

}

QString OsmAnd::Street::toString() const
{
    return QStringLiteral("str. ") % _nativeName;
}

OsmAnd::PointI OsmAnd::Street::position31() const
{
    return _position31;
}

QHash<QString, QString> OsmAnd::Street::localizedNames() const
{
    return _localizedNames;
}

QString OsmAnd::Street::nativeName() const
{
    return _nativeName;
}
