/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
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

#ifndef _OSMAND_CORE_OBF_MAP_SECTION_READER_H_
#define _OSMAND_CORE_OBF_MAP_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

namespace OsmAnd {

    class ObfReader;
    class ObfMapSectionInfo;
    namespace Model {
        class MapObject;
    } // namespace Model
    class IQueryController;
    namespace ObfMapSectionReader_Metrics {
        struct Metric_loadMapObjects;
    } // namespace ObfMapSectionReader_Metrics

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
            const FilterMapObjectsByIdSignature filterById = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::MapObject>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric = nullptr);
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_OBF_MAP_SECTION_READER_H_
