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

#include <stdint.h>
#include <limits>
#include <memory>
#include <functional>

#include <google/protobuf/io/coded_stream.h>

#include <QList>
#include <QMap>
#include <QString>
#include <QVector>

#include <OsmAndCore.h>
#include <ObfSection.h>
#include <QueryFilter.h>

namespace OsmAnd {

    namespace gpb = google::protobuf;

    namespace Model {
        class Road;
    } // namespace Model

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

            bool isRoundabout() const;
            int getDirection() const;
        };
        QList< std::shared_ptr<EncodingRule> > _encodingRules;

        uint32_t _borderBoxOffset;
        uint32_t _baseBorderBoxOffset;
        uint32_t _borderBoxLength;
        uint32_t _baseBorderBoxLength;

        class OSMAND_CORE_API Subsection
        {
        private:
        protected:
            Subsection(const std::shared_ptr<Subsection>& parent);
            Subsection(const std::shared_ptr<ObfRoutingSection>& section);
            
            uint32_t _offset;
            uint32_t _length;

            AreaI _area31;

            uint32_t _dataOffset;

            uint32_t _subsectionsOffset;
            QList< std::shared_ptr<Subsection> > _subsections;
        public:
            virtual ~Subsection();

            bool containsData() const;
            const AreaI& area31;
            
            const std::shared_ptr<Subsection> parent;
            const std::shared_ptr<ObfRoutingSection> section;

        friend struct OsmAnd::ObfRoutingSection;
        };
        QList< std::shared_ptr<Subsection> > _subsections;
        QList< std::shared_ptr<Subsection> > _baseSubsections;

        class OSMAND_CORE_API BorderLineHeader
        {
        private:
        protected:
            BorderLineHeader();

            uint32_t _x;
            uint32_t _y;
            uint32_t _x2;
            bool _x2present;
            uint32_t _y2;
            bool _y2present;
            uint32_t _offset;
        public:
            virtual ~BorderLineHeader();

            uint32_t& x;
            uint32_t& y;
            uint32_t& x2;
            bool& x2present;
            uint32_t& y2;
            bool& y2present;
            uint32_t& offset;

            friend struct OsmAnd::ObfRoutingSection;
        };

        class OSMAND_CORE_API BorderLinePoint
        {
        private:
        protected:
            BorderLinePoint();

            uint64_t _id;
            PointI _location;
            QVector<uint32_t> _types;
            float _distanceToStartPoint;
            float _distanceToEndPoint;
        public:
            virtual ~BorderLinePoint();

            const uint64_t& id;
            const PointI& location;
            const QVector<uint32_t>& types;
            float& distanceToStartPoint;
            float& distanceToEndPoint;

            std::shared_ptr<BorderLinePoint> bboxedClone(uint32_t x31) const;
            
            friend struct OsmAnd::ObfRoutingSection;
        };

        static void querySubsections(ObfReader* reader, const QList< std::shared_ptr<Subsection> >& in,
            QList< std::shared_ptr<Subsection> >* resultOut = nullptr,
            QueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<OsmAnd::ObfRoutingSection::Subsection>&)> visitor = nullptr);
        static void loadSubsectionData(ObfReader* reader, const std::shared_ptr<Subsection>& subsection,
            QList< std::shared_ptr<Model::Road> >* resultOut = nullptr,
            QMap< uint64_t, std::shared_ptr<Model::Road> >* resultMapOut = nullptr,
            QueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<OsmAnd::Model::Road>&)> visitor = nullptr);

        static void loadSubsectionBorderBoxLinesPoints(ObfReader* reader, const ObfRoutingSection* section,
            QList< std::shared_ptr<BorderLinePoint> >* resultOut = nullptr,
            QueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<BorderLineHeader>&)> visitorLine = nullptr,
            std::function<bool (const std::shared_ptr<BorderLinePoint>&)> visitorPoint = nullptr);
    protected:
        enum {
            ShiftCoordinates = 4,
        };
        static void read(ObfReader* reader, const std::shared_ptr<ObfRoutingSection>& section);
        static void readEncodingRule(ObfReader* reader, ObfRoutingSection* section, EncodingRule* rule);
        static void readSubsectionHeader(ObfReader* reader, const std::shared_ptr<Subsection>& subsection, Subsection* parent, uint32_t depth = std::numeric_limits<uint32_t>::max());
        static void readSubsectionChildrenHeaders(ObfReader* reader, const std::shared_ptr<Subsection>& subsection, uint32_t depth = std::numeric_limits<uint32_t>::max());
        static void readSubsectionData(ObfReader* reader, const std::shared_ptr<Subsection>& subsection,
            QList< std::shared_ptr<Model::Road> >* resultOut = nullptr,
            QMap< uint64_t, std::shared_ptr<Model::Road> >* resultMapOut = nullptr,
            QueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<OsmAnd::Model::Road>&)> visitor = nullptr);
        static void readSubsectionRoadsIds(ObfReader* reader, Subsection* subsection, QList<uint64_t>& ids);
        static void readSubsectionRestriction(ObfReader* reader, Subsection* subsection, const QMap< uint32_t, std::shared_ptr<Model::Road> >& roads, const QList<uint64_t>& roadsInternalIdToGlobalIdMap);
        static void readRoad(ObfReader* reader, Subsection* subsection, const QList<uint64_t>& idsTable, uint32_t& internalId, Model::Road* road);

        static void readBorderBoxLinesHeaders(ObfReader* reader,
            QList< std::shared_ptr<BorderLineHeader> >* resultOut = nullptr,
            QueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<BorderLineHeader>&)> visitor = nullptr);
        static void readBorderLineHeader(ObfReader* reader, BorderLineHeader* borderLine, uint32_t outerOffset);
        static void readBorderLinePoints(ObfReader* reader,
            QList< std::shared_ptr<BorderLinePoint> >* resultOut = nullptr,
            QueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<BorderLinePoint>&)> visitor = nullptr);
        static void readBorderLinePoint(ObfReader* reader,
            BorderLinePoint* point);
    private:

    friend class OsmAnd::ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_ROUTING_SECTION_H_
