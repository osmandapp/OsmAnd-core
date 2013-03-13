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

#ifndef __OBF_ROUTING_SECTION_H_
#define __OBF_ROUTING_SECTION_H_

#include <cstdint>
#include <memory>
#include <google/protobuf/io/coded_stream.h>
#include <QList>
#include <QString>
#include <OsmAndCore.h>
#include <ObfSection.h>

namespace OsmAnd {

    namespace gpb = google::protobuf;

    /**
    'Routing' section of OsmAnd Binary File
    */
    struct OSMAND_CORE_API ObfRoutingSection : public ObfSection
    {
        ObfRoutingSection(class ObfReader* owner);
        virtual ~ObfRoutingSection();

        struct OSMAND_CORE_API EncodingRule
        {
            EncodingRule();
            virtual ~EncodingRule();

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
        };
        QList< std::shared_ptr<EncodingRule> > _encodingRules;

        uint32_t _borderBoxOffset;
        uint32_t _baseBorderBoxOffset;
        uint32_t _borderBoxLength;
        uint32_t _baseBorderBoxLength;

        struct OSMAND_CORE_API Subsection
        {
            Subsection();
            virtual ~Subsection();

            uint32_t _offset;
            uint32_t _length;

            uint32_t _left31;
            uint32_t _right31;
            uint32_t _top31;
            uint32_t _bottom31;

            uint32_t _dataOffset;

            QList< std::shared_ptr<Subsection> > _subsections;
        };
        QList< std::shared_ptr<Subsection> > _subsections;
        QList< std::shared_ptr<Subsection> > _baseSubsections;

    protected:
        static void read(ObfReader* reader, ObfRoutingSection* section);
        static void readEncodingRule(ObfReader* reader, ObfRoutingSection* section, EncodingRule* rule);
        static void readSubsection(ObfReader* reader, ObfRoutingSection* section, Subsection* subsection, Subsection* parent);

    private:

    friend class ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_ROUTING_SECTION_H_