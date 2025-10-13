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
    class ObfAddressSectionInfo;
    class Street;

    // class City in Java
    class OSMAND_CORE_API StreetGroup Q_DECL_FINAL : public Address
    {
        Q_DISABLE_COPY_AND_MOVE(StreetGroup);

    private:
    protected:
    public:
        StreetGroup(const std::shared_ptr<const ObfAddressSectionInfo>& obfSection);
        virtual ~StreetGroup();
        virtual QString toString() const;

        ObfAddressStreetGroupSubtype type; // CityType

        uint32_t dataOffset;

        QList<std::shared_ptr<const Street>> streets;
        std::vector<int32_t> bbox31; // unused
    };
}

#endif // !defined(_OSMAND_CORE_STREET_GROUP_H_)
