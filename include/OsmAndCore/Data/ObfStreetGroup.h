#ifndef _OSMAND_CORE_OBF_STREET_GROUP_H_
#define _OSMAND_CORE_OBF_STREET_GROUP_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/Data/Address.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/ObfAddress.h>
#include <OsmAndCore/Data/StreetGroup.h>
#include <OsmAndCore/PointsAndAreas.h>

namespace OsmAnd
{
    class ObfAddressSectionInfo;

    class OSMAND_CORE_API ObfStreetGroup Q_DECL_FINAL : public ObfAddress
    {
        Q_DISABLE_COPY_AND_MOVE(ObfStreetGroup)

    public:
        ObfStreetGroup(
                std::shared_ptr<const ObfAddressSectionInfo> obfSection,
                StreetGroup streetGroup,
                ObfObjectId id = ObfObjectId::invalidId(),
                uint32_t dataOffset = 0);

        std::shared_ptr<const ObfAddressSectionInfo> obfSection() const;
        StreetGroup streetGroup() const;
        uint32_t dataOffset() const;

    private:
        std::shared_ptr<const ObfAddressSectionInfo> _obfSection;
        StreetGroup _streetGroup;
        uint32_t _dataOffset;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_STREET_GROUP_H_)
