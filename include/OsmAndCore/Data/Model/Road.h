/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ROAD_H_
#define __ROAD_H_

#include <cstdint>
#include <memory>

#include <QVector>
#include <QMap>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class RoutePlanner;
    class ObfRoutingSectionReader_P;
    class ObfRoutingSubsectionInfo;

    namespace Model {

        STRONG_ENUM_EX(RoadDirection, int32_t)
        {
            OneWayForward = -1,
            TwoWay = 0,
            OneWayReverse = +1
        };

        STRONG_ENUM_EX(RoadRestriction, int32_t)
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
        private:
        protected:
            const std::shared_ptr<const Road> _ref;

            uint64_t _id;
            QMap<uint32_t, QString> _names;
            QVector< PointI > _points;
            QVector< uint32_t > _types;
            QMap< uint32_t, QVector<uint32_t> > _pointsTypes;
            QMap< uint64_t, RoadRestriction > _restrictions;

            Road(const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection);
            Road(const std::shared_ptr<const Road>& that, int insertIdx, uint32_t x31, uint32_t y31);
        public:
            virtual ~Road();

            const std::shared_ptr<const ObfRoutingSubsectionInfo> subsection;
            const uint64_t& id;
            const QMap<uint32_t, QString>& names;
            const QVector< PointI >& points;
            const QVector< uint32_t >& types;
            const QMap< uint32_t, QVector<uint32_t> >& pointsTypes;
            const QMap< uint64_t, RoadRestriction >& restrictions;

            double getDirectionDelta(uint32_t originIdx, bool forward) const;
            double getDirectionDelta(uint32_t originIdx, bool forward, float distance) const;

            bool isLoop() const;
            RoadDirection getDirection() const;
            bool isRoundabout() const;
            QString getHighway() const;
            int getLanes() const;

        friend class OsmAnd::ObfRoutingSectionReader_P;
        friend class OsmAnd::RoutePlanner;
        };

    } // namespace Model

} // namespace OsmAnd

#endif // __ROAD_H_
