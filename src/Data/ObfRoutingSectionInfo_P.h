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

#ifndef __OBF_ROUTING_SECTION_INFO_P_H_
#define __OBF_ROUTING_SECTION_INFO_P_H_

#include <cstdint>
#include <memory>

#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfRoutingSectionReader_P;
    namespace Model {
        STRONG_ENUM_EX(RoadDirection, int32_t);
        class Road;
    } // namespace Model
    class RoutePlanner;
    class RoutePlannerContext;
    class RoutingRulesetContext;

    class ObfRoutingSectionInfo;
    class ObfRoutingSectionInfo_P
    {
        Q_DISABLE_COPY(ObfRoutingSectionInfo_P)
    private:
    protected:
        ObfRoutingSectionInfo_P(ObfRoutingSectionInfo* owner);

        ObfRoutingSectionInfo* const owner;

        struct EncodingRule
        {
            enum Type : uint32_t
            {
                Access = 1,
                OneWay = 2,
                Highway = 3,
                Maxspeed = 4,
                Roundabout = 5,
                TrafficSignals = 6,
                RailwayCrossing = 7,
                Lanes = 8,
            };

            uint32_t _id;
            QString _tag;
            QString _value;
            Type _type;
            union
            {
                int32_t asSignedInt;
                uint32_t asUnsignedInt;
                float asFloat;
            } _parsedValue;

            bool isRoundabout() const;
            Model::RoadDirection getDirection() const;
        };
        QList< std::shared_ptr<EncodingRule> > _encodingRules;

        uint32_t _borderBoxOffset;
        uint32_t _baseBorderBoxOffset;
        uint32_t _borderBoxLength;
        uint32_t _baseBorderBoxLength;
    public:
        virtual ~ObfRoutingSectionInfo_P();

    friend class OsmAnd::ObfRoutingSectionInfo;
    friend class OsmAnd::ObfRoutingSectionReader_P;
    friend class OsmAnd::Model::Road;
    friend class OsmAnd::RoutePlanner;
    friend class OsmAnd::RoutePlannerContext;
    friend class OsmAnd::RoutingRulesetContext;
    };

} // namespace OsmAnd

#endif // __OBF_ROUTING_SECTION_INFO_P_H_
