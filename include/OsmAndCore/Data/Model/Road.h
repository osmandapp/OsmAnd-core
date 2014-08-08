#ifndef _OSMAND_CORE_ROAD_H_
#define _OSMAND_CORE_ROAD_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QVector>
#include <QString>
#include <QHash>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class ObfRoutingSectionReader_P;
    class ObfRoutingSectionInfo;

    namespace Model
    {
        //enum class RoadDirection : int32_t
        //{
        //    OneWayForward = -1,
        //    TwoWay = 0,
        //    OneWayReverse = +1
        //};

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

        class OSMAND_CORE_API Road
        {
            Q_DISABLE_COPY(Road);
        private:
        protected:
            const std::shared_ptr<const Road> _ref;

            uint64_t _id;
            QHash<uint32_t, QString> _names;
            AreaI _bbox31;
            QVector< PointI > _points31;
            QVector< uint32_t > _types;
            QHash< uint32_t, QVector<uint32_t> > _pointsTypes;
            QHash< uint64_t, RoadRestriction > _restrictions;

            Road(const std::shared_ptr<const ObfRoutingSectionInfo>& section);
            /*Road(const std::shared_ptr<const Road>& that, int insertIdx, uint32_t x31, uint32_t y31);*/
        public:
            virtual ~Road();

            const std::shared_ptr<const ObfRoutingSectionInfo> section;

            const uint64_t& id;
            const QHash<uint32_t, QString>& names;
            const AreaI& bbox31;
            const QVector< PointI >& points31;
            const QVector< uint32_t >& types;
            const QHash< uint32_t, QVector<uint32_t> >& pointsTypes;
            const QHash< uint64_t, RoadRestriction >& restrictions;

            QString getNameInNativeLanguage() const;
            QString getNameInLanguage(const QString& lang) const;

            /*double getDirectionDelta(uint32_t originIdx, bool forward) const;
            double getDirectionDelta(uint32_t originIdx, bool forward, float distance) const;

            bool isLoop() const;
            RoadDirection getDirection() const;
            bool isRoundabout() const;
            QString getHighway() const;
            int getLanes() const;*/

        friend class OsmAnd::ObfRoutingSectionReader_P;
        };
    }
}

#endif // !defined(_OSMAND_CORE_ROAD_H_)
