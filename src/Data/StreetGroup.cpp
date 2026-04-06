#include "StreetGroup.h"

#include <QStringBuilder>

OsmAnd::StreetGroup::StreetGroup(const std::shared_ptr<const ObfAddressSectionInfo>& obfSection_)
    : Address(obfSection_, AddressType::StreetGroup)
    , dataOffset(0)
{
}

OsmAnd::StreetGroup::~StreetGroup()
{
}

QString OsmAnd::StreetGroup::toString() const
{
    return QStringLiteral("city ") % nativeName;
}

QStringList OsmAnd::StreetGroup::getOtherNames(bool transliterate) const
{
    QStringList l;
    if (localizedNames.contains(QStringLiteral("en")))
    {
        l.append(localizedNames[QStringLiteral("en")]);
    }
    if (!localizedNames.isEmpty())
    {
        QHash<QString, QString>::const_iterator i = localizedNames.constBegin();
        while (i != localizedNames.constEnd())
        {
            QString key = i.key();
            if (key == QStringLiteral("admin_level") || key == QStringLiteral("place"))
            {
                ++i;
                continue;
            }
            l.append(i.value());
            ++i;
        }
    }
    return l;
}
