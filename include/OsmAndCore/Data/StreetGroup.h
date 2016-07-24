#ifndef _OSMAND_CORE_STREET_GROUP_H_
#define _OSMAND_CORE_STREET_GROUP_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/Address.h>

namespace OsmAnd
{
    class OSMAND_CORE_API StreetGroup Q_DECL_FINAL : public Address
    {
    public:
        StreetGroup(
                ObfAddressStreetGroupType type,
                ObfAddressStreetGroupSubtype subtype,
                QString nativeName = {},
                QHash<QString, QString> localizedNames = {},
                PointI position31 = {});
        virtual QString toString() const;

        const ObfAddressStreetGroupType type;
        const ObfAddressStreetGroupSubtype subtype;
    };
}

#endif // !defined(_OSMAND_CORE_STREET_GROUP_H_)
