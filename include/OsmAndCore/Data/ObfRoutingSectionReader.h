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

#ifndef __OBF_ROUTING_SECTION_READER_H_
#define __OBF_ROUTING_SECTION_READER_H_

#include <stdint.h>
#include <memory>
#include <functional>

#include <QMap>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfReader;
    class ObfRoutingSectionInfo;
    class ObfRoutingSubsectionInfo;
    class ObfRoutingBorderLineHeader;
    class ObfRoutingBorderLinePoint;
    namespace Model {
        class Road;
    } // namespace Model
    class IQueryFilter;

    class OSMAND_CORE_API ObfRoutingSectionReader
    {
    private:
        ObfRoutingSectionReader();
        ~ObfRoutingSectionReader();
    protected:
    public:
        static void querySubsections(const std::shared_ptr<ObfReader>& reader, const QList< std::shared_ptr<ObfRoutingSubsectionInfo> >& in,
            QList< std::shared_ptr<const ObfRoutingSubsectionInfo> >* resultOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingSubsectionInfo>&)> visitor = nullptr);

        static void loadSubsectionData(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            QList< std::shared_ptr<const Model::Road> >* resultOut = nullptr,
            QMap< uint64_t, std::shared_ptr<const Model::Road> >* resultMapOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Road>&)> visitor = nullptr);

        static void loadSubsectionBorderBoxLinesPoints(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            QList< std::shared_ptr<const ObfRoutingBorderLinePoint> >* resultOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingBorderLineHeader>&)> visitorLine = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingBorderLinePoint>&)> visitorPoint = nullptr);
    };

} // namespace OsmAnd

#endif // __OBF_ROUTING_SECTION_READER_H_
