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

namespace OsmAnd
{
    class ObfAddressSectionInfo;

    enum class AddressType
    {
        StreetGroup,
        Street,
        Building,
        StreetIntersection
    };

    static const QMap<AddressType, QString> ADDRESS_TYPE_NAMES
    {
            { AddressType::StreetGroup, "Street group" },
            { AddressType::Street, "Street" },
            { AddressType::Building, "Building" },
            { AddressType::StreetIntersection, "Street intersection" },
    };

    class OSMAND_CORE_API Address
    {
        Q_DISABLE_COPY_AND_MOVE(Address);

    private:
    protected:
        Address(const std::shared_ptr<const ObfAddressSectionInfo>& obfSection, const AddressType addressType);
    public:
        virtual ~Address();
        virtual QString toString() const;

        virtual QString getName(const QString lang, bool transliterate) const;
        
        const std::shared_ptr<const ObfAddressSectionInfo> obfSection;
        const AddressType addressType;

        ObfObjectId id;
        PointI position31;
        QString nativeName;
        QHash<QString, QString> localizedNames;
    };
}

#endif // !defined(_OSMAND_CORE_ADDRESS_H_)
