#ifndef _OSMAND_CORE_ROAD_H_
#define _OSMAND_CORE_ROAD_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QVector>
#include <QHash>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfMapObject.h>

namespace OsmAnd
{
    class ObfRoutingSectionReader_P;
    class ObfRoutingSectionInfo;

    enum class RoadDirection : int32_t
    {
        OneWayForward = -1,
        TwoWay = 0,
        OneWayReverse = +1
    };

    enum class RoadRestriction : int32_t
    {
        Special_ReverseWayOnly = -1,

        Invalid = 0,

        NoRightTurn = 1,
        NoLeftTurn = 2,
        NoUTurn = 3,
        NoStraightOn = 4,
        OnlyRightTurn = 5,
        OnlyLeftTurn = 6,
        OnlyStraightOn = 7,
    };

    class OSMAND_CORE_API Road Q_DECL_FINAL : public ObfMapObject
    {
        Q_DISABLE_COPY_AND_MOVE(Road)
    private:
    protected:
        Road(const std::shared_ptr<const ObfRoutingSectionInfo>& section);
    public:
        virtual ~Road();

        // General information
        const std::shared_ptr<const ObfRoutingSectionInfo> section;

        // Road information
        QHash< uint32_t, QVector<uint32_t> > pointsTypes;
        QHash< ObfObjectId, RoadRestriction > restrictions;

    friend class OsmAnd::ObfRoutingSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_ROAD_H_)
