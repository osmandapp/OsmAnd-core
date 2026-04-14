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
        if (localizedNames.contains(QStringLiteral("en")))
        {
            QString enName = localizedNames[QStringLiteral("en")];
            if (localeName.isEmpty() || localeName != enName)
            {
                l.append(enName);
            }
        }
        if (!localizedNames.isEmpty())
        {
            QHash<QString, QString>::const_iterator i = localizedNames.constBegin();
            while (i != localizedNames.constEnd())
            {
                QString key = i.key();
                QString name = localizedNames[key];
                if (key == QStringLiteral("admin_level") || key == QStringLiteral("place") || localeName == name)
                {
                    ++i;
                    continue;
                }
                l.append(i.value());
                ++i;
            }
        }
        if (localeName != obj.nativeName)
        {
            l.append(obj.nativeName);
        }
        return l;
    }
}

#endif // !defined(_OSMAND_CORE_COMMONIMPL_H_)
