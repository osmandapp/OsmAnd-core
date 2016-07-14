#ifndef _OSMAND_CORE_ADDRESS_H_
#define _OSMAND_CORE_ADDRESS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>
#include <QMap>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/Address.h>

namespace OsmAnd
{
    enum class AddressType
    {
        StreetGroup,
        Street,
        Building,
        StreetIntersection
    };

    static const QMap<AddressType, QString> ADDRESS_TYPE_NAMES{
            {AddressType::StreetGroup, "Street group"},
            {AddressType::Street, "Street"},
            {AddressType::Building, "Building"},
            {AddressType::StreetIntersection, "Street intersection"},
    };

    class OSMAND_CORE_API Address
    {
    protected:
        Address(AddressType addressType,
                QString nativeName,
                QHash<QString, QString> localizedNames,
                PointI position31);
    public:
        virtual QString toString() const = 0;

        const AddressType addressType;
        const QString nativeName;
        const QHash<QString, QString> localizedNames;
        const PointI position31;
    };
}

#endif // !defined(_OSMAND_CORE_ADDRESS_H_)
