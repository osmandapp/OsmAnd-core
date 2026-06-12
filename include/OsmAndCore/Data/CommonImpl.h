#ifndef _OSMAND_CORE_COMMONIMPL_H_
#define _OSMAND_CORE_COMMONIMPL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>
#include <QMap>

namespace OsmAnd
{
    template <typename T>
    QStringList getOtherNamesImpl(const T& obj, bool transliterate, const QString& localeName)
    {
        QStringList l;
        const QHash<QString, QString>& localizedNames = obj.localizedNames;
        const QList<QString>& order = obj.localizedNamesOrder;
        if (localizedNames.contains(QStringLiteral("en")))
        {
            QString enName = localizedNames[QStringLiteral("en")];
            if (localeName.isEmpty() || localeName != enName)
            {
                l.append(enName);
            }
        }
        for (const QString& key : order)
        {
            if (!localizedNames.contains(key))
                continue;
            if (key == QStringLiteral("en"))
                continue;

            QString name = localizedNames.value(key);
            if (key == QStringLiteral("admin_level") || key == QStringLiteral("place") || localeName == name)
            {
                continue;
            }
            l.append(name);
        }
        if (localeName != obj.nativeName)
        {
            l.append(obj.nativeName);
        }
        return l;
    }
}

#endif // !defined(_OSMAND_CORE_COMMONIMPL_H_)
