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

namespace OsmAnd
{
    class OSMAND_CORE_API ObfBuilding Q_DECL_FINAL
    {
    public:
        ObfBuilding(Building building, ObfObjectId id);

        const Building building;
        const ObfObjectId id;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_BUILDING_H_)
