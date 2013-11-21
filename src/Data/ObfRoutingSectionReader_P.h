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

#ifndef __OBF_ROUTING_SECTION_READER_P_H_
#define __OBF_ROUTING_SECTION_READER_P_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QSet>
#include <QMap>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <ObfRoutingSectionInfo_P.h>

namespace OsmAnd {

    class ObfReader_P;
    class ObfRoutingSectionInfo;
    class ObfRoutingEncodingRule;
    class ObfRoutingSubsectionInfo;
    class ObfRoutingBorderLineHeader;
    class ObfRoutingBorderLinePoint;
    namespace Model {
        class Road;
    } // namespace Model
    class IQueryController;
    class IQueryFilter;

    class ObfRoutingSectionReader;
    class ObfRoutingSectionReader_P
    {
    private:
        ObfRoutingSectionReader_P();
        ~ObfRoutingSectionReader_P();
    protected:
    
        enum {
            ShiftCoordinates = 4,
        };

        static void read(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfRoutingSectionInfo>& section);

        static void readEncodingRule(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            const std::shared_ptr<ObfRoutingSectionInfo_P::EncodingRule>& rule);
        static void readSubsectionHeader(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfRoutingSubsectionInfo>& subsection,
            const std::shared_ptr<ObfRoutingSubsectionInfo>& parent, uint32_t depth = std::numeric_limits<uint32_t>::max());
        static void readSubsectionChildrenHeaders(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfRoutingSubsectionInfo>& subsection,
            uint32_t depth = std::numeric_limits<uint32_t>::max());
        static void readSubsectionData(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            QList< std::shared_ptr<const Model::Road> >* resultOut = nullptr,
            QMap< uint64_t, std::shared_ptr<const Model::Road> >* resultMapOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const Model::Road>&)> visitor = nullptr);
        static void readSubsectionRoadsIds(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            QList<uint64_t>& ids);
        static void readSubsectionRestriction(
            const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            const QMap< uint32_t, std::shared_ptr<Model::Road> >& roads, const QList<uint64_t>& roadsInternalIdToGlobalIdMap);
        static void readRoad(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            const QList<uint64_t>& idsTable, uint32_t& internalId, const std::shared_ptr<Model::Road>& road );

        static void readBorderBoxLinesHeaders(const std::unique_ptr<ObfReader_P>& reader,
            QList< std::shared_ptr<const ObfRoutingBorderLineHeader> >* resultOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingBorderLineHeader>&)> visitor = nullptr);
        static void readBorderLineHeader(const std::unique_ptr<ObfReader_P>& reader,
            const std::shared_ptr<ObfRoutingBorderLineHeader>& borderLine, uint32_t outerOffset);
        static void readBorderLinePoints(const std::unique_ptr<ObfReader_P>& reader,
            QList< std::shared_ptr<const ObfRoutingBorderLinePoint> >* resultOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingBorderLinePoint>&)> visitor = nullptr);
        static void readBorderLinePoint(const std::unique_ptr<ObfReader_P>& reader,
            const std::shared_ptr<ObfRoutingBorderLinePoint>& point);

        static void querySubsections(const std::unique_ptr<ObfReader_P>& reader, const QList< std::shared_ptr<ObfRoutingSubsectionInfo> >& in,
            QList< std::shared_ptr<const ObfRoutingSubsectionInfo> >* resultOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingSubsectionInfo>&)> visitor = nullptr);

        static void loadSubsectionData(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            QList< std::shared_ptr<const Model::Road> >* resultOut = nullptr,
            QMap< uint64_t, std::shared_ptr<const Model::Road> >* resultMapOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Road>&)> visitor = nullptr);

        static void loadSubsectionBorderBoxLinesPoints(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            QList< std::shared_ptr<const ObfRoutingBorderLinePoint> >* resultOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingBorderLineHeader>&)> visitorLine = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingBorderLinePoint>&)> visitorPoint = nullptr);

    friend class OsmAnd::ObfRoutingSectionReader;
    friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // __OBF_ROUTING_SECTION_READER_P_H_
