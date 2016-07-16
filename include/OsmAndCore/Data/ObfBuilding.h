#ifndef _OSMAND_CORE_OBF_BUILDING_H_
#define _OSMAND_CORE_OBF_BUILDING_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/MemoryCommon.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Data/Building.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/ObfAddress.h>

namespace OsmAnd
{
    class OSMAND_CORE_API ObfBuilding Q_DECL_FINAL : ObfAddress
    {
    public:
        ObfBuilding(
                Building building,
                ObfObjectId id,
                std::shared_ptr<const ObfStreet> street);
        ObfBuilding(
                Building building,
                ObfObjectId id,
                std::shared_ptr<const ObfStreetGroup> streetGroup);

        Building building() const;
        std::shared_ptr<const ObfStreet> street() const;
        std::shared_ptr<const ObfStreetGroup> streetGroup() const;

    private:
        Building _building;
        std::shared_ptr<const ObfStreet> _street;
        std::shared_ptr<const ObfStreetGroup> _streetGroup;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_BUILDING_H_)
