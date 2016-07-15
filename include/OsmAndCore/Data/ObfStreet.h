#ifndef _OSMAND_CORE_OBF_STREET_H_
#define _OSMAND_CORE_OBF_STREET_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/ObfStreetGroup.h>
#include <OsmAndCore/Data/StreetGroup.h>
#include <OsmAndCore/Data/Street.h>
#include <OsmAndCore/PointsAndAreas.h>

namespace OsmAnd
{
    class OSMAND_CORE_API ObfStreet Q_DECL_FINAL
    {
    public:
        ObfStreet(
                const std::shared_ptr<const ObfStreetGroup>& obfStreetGroup,
                Street street,
                ObfObjectId id = ObfObjectId::invalidId(),
                uint32_t offset = 0,
                uint32_t firstBuildingInnerOffset = 0,
                uint32_t firstIntersectionInnerOffset = 0);

        inline StreetGroup streetGroup() const
        {
            return obfStreetGroup->streetGroup;
        }

        const std::shared_ptr<const ObfStreetGroup> obfStreetGroup;
        const Street street;
        const ObfObjectId id;
        const uint32_t offset;
        const uint32_t firstBuildingInnerOffset;
        const uint32_t firstIntersectionInnerOffset;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_STREET_H_)

