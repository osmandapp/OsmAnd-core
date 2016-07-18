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

    inline Bitmask<AddressType> fullAddressTypeMask()
    {
        Bitmask<AddressType> result;
        for (auto type : ADDRESS_TYPE_NAMES.keys())
            result.set(type);
        return result;
//        return Bitmask<AddressType>().set(ADDRESS_TYPE_NAMES.keys().begin());
//        return Bitmask<AddressType>()
//                .set(AddressType::StreetGroup)
//                .set(AddressType::Street)
//                .set(AddressType::Building)
//                .set(AddressType::StreetIntersection);
    }

    class OSMAND_CORE_API Address
    {
    public:
        virtual QString toString() const = 0;

        QString nativeName() const;
        QHash<QString, QString> localizedNames() const;
        PointI position31() const;

    protected:
        Address(
                QString nativeName,
                QHash<QString, QString> localizedNames,
                PointI position31);

        QString _nativeName;
        QHash<QString, QString> _localizedNames;
        PointI _position31;
    };
}

#endif // !defined(_OSMAND_CORE_ADDRESS_H_)
