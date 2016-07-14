#ifndef _OSMAND_CORE_STREET_INTERSECTION_H_
#define _OSMAND_CORE_STREET_INTERSECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/Address.h>
#include <OsmAndCore/Data/ObfStreet.h>

namespace OsmAnd
{
    class ObfStreet;

    class OSMAND_CORE_API StreetIntersection Q_DECL_FINAL : public Address
    {
    public:
        StreetIntersection(
                std::shared_ptr<const ObfStreet> street,
                QString nativeName = {},
                QHash<QString, QString> localizedNames = {},
                PointI position31 = {});

        inline virtual QString toString() const
        {
            return "intersection " + nativeName + " (" + street->street.nativeName;
        }

        const std::shared_ptr<const ObfStreet> street;
    };
}

#endif // !defined(_OSMAND_CORE_STREET_INTERSECTION_H_)
