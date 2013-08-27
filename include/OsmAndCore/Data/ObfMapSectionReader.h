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

#ifndef __OBF_MAP_SECTION_READER_H_
#define __OBF_MAP_SECTION_READER_H_

#include <stdint.h>
#include <memory>
#include <functional>

#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

namespace OsmAnd {

    class ObfReader;
    class ObfMapSectionInfo;
    namespace Model {
        class MapObject;
    } // namespace Model
    class IQueryController;

    class OSMAND_CORE_API ObfMapSectionReader
    {
    private:
        ObfMapSectionReader();
        ~ObfMapSectionReader();
    protected:
    public:
        static void loadMapObjects(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
            ZoomLevel zoom, const AreaI* bbox31 = nullptr,
            QList< std::shared_ptr<const OsmAnd::Model::MapObject> >* resultOut = nullptr, MapFoundationType* foundationOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::MapObject>&)> visitor = nullptr,
            IQueryController* controller = nullptr);
    };

} // namespace OsmAnd

#endif // __OBF_MAP_SECTION_READER_H_
