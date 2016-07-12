#ifndef _OSMAND_CORE_BUILDING_H_
#define _OSMAND_CORE_BUILDING_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/MemoryCommon.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Data/Address.h>
#include <OsmAndCore/Data/DataCommonTypes.h>

namespace OsmAnd
{
    class ObfStreet;
    class StreetGroup;

    class OSMAND_CORE_API Building Q_DECL_FINAL: public Address
    {
    public:
        class Interpolation Q_DECL_FINAL: public Address
        {
        public:
            enum class Type : int32_t
            {
                Disabled = 0,
                All = -1,
                Even = -2,
                Odd = -3,
                Alphabetic = -4
            };
            Interpolation(Type type = Type::Disabled,
                          QString nativeName = {},
                          QHash<QString, QString> localizedNames = {},
                          PointI position31 = {});
            const Type type;
        };

        Building(
                std::shared_ptr<const ObfStreet> street,
                Interpolation interpolation,
                QString nativeName = {},
                QHash<QString, QString> localizedNames = {},
                PointI position31 = {});
        Building(
                std::shared_ptr<const StreetGroup> streetGroup,
                Interpolation interpolation,
                QString nativeName = {},
                QHash<QString, QString> localizedNames = {},
                PointI position31 = {});
        virtual ~Building();
        virtual QString toString() const;

        const std::shared_ptr<const ObfStreet> street;
        const std::shared_ptr<const StreetGroup> streetGroup;
        const Interpolation interpolation;

        QString postcode;
    };
}

#endif // !defined(_OSMAND_CORE_BUILDING_H_)
