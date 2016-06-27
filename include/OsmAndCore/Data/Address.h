#ifndef _OSMAND_CORE_ADDRESS_H_
#define _OSMAND_CORE_ADDRESS_H_

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
    class ObfAddressSectionInfo;

    enum class AddressType
    {
        StreetGroup,
        Street,
        Building,
        StreetIntersection
    };

    class OSMAND_CORE_API Address
    {
        Q_DISABLE_COPY_AND_MOVE(Address);

    private:
    protected:
        Address(const std::shared_ptr<const ObfAddressSectionInfo>& obfSection, const AddressType addressType);
    public:
        virtual ~Address();

        const std::shared_ptr<const ObfAddressSectionInfo> obfSection;
        const AddressType addressType;
    };
}

#endif // !defined(_OSMAND_CORE_ADDRESS_H_)
