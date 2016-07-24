#ifndef _OSMAND_CORE_OBF_STREET_H_
#define _OSMAND_CORE_OBF_STREET_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/ObfAddress.h>
#include <OsmAndCore/Data/ObfStreetGroup.h>
#include <OsmAndCore/Data/StreetGroup.h>
#include <OsmAndCore/Data/Street.h>
#include <OsmAndCore/PointsAndAreas.h>

namespace OsmAnd
{
    class OSMAND_CORE_API ObfStreet Q_DECL_FINAL : public ObfAddress
    {
    public:
        ObfStreet(
                const std::shared_ptr<const ObfStreetGroup>& obfStreetGroup,
                std::shared_ptr<const Street> street,
                ObfObjectId id = ObfObjectId::invalidId(),
                uint32_t offset = 0,
                uint32_t firstBuildingInnerOffset = 0,
                uint32_t firstIntersectionInnerOffset = 0);

        std::shared_ptr<const ObfStreetGroup> obfStreetGroup() const;
        StreetGroup streetGroup() const;
        Street street() const;
        std::shared_ptr<const Street> streetPtr() const;
        uint32_t offset() const;
        uint32_t firstBuildingInnerOffset() const;
        uint32_t firstIntersectionInnerOffset() const;

    private:
        std::shared_ptr<const ObfStreetGroup> _obfStreetGroup;
        std::shared_ptr<const Street> _street;
        uint32_t _offset;
        uint32_t _firstBuildingInnerOffset;
        uint32_t _firstIntersectionInnerOffset;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_STREET_H_)

