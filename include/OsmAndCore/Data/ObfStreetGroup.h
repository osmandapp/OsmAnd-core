#ifndef _OSMAND_CORE_OBF_STREET_GROUP_H_
#define _OSMAND_CORE_OBF_STREET_GROUP_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/Address.h>
#include <OsmAndCore/Data/StreetGroup.h>

namespace OsmAnd
{
    class ObfAddressSectionInfo;

    class OSMAND_CORE_API ObfStreetGroup Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ObfStreetGroup);

    public:
        ObfStreetGroup(
                std::shared_ptr<const ObfAddressSectionInfo> obfSection,
                StreetGroup streetGroup,
                ObfObjectId id = ObfObjectId::invalidId(),
                uint32_t dataOffset = 0);

        const std::shared_ptr<const ObfAddressSectionInfo> obfSection;
        const StreetGroup streetGroup;
        const ObfObjectId id = ObfObjectId::invalidId();
        const uint32_t dataOffset = 0;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_STREET_GROUP_H_)
