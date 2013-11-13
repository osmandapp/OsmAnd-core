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

#ifndef __OBF_ADDRESS_SECTION_READER_H_
#define __OBF_ADDRESS_SECTION_READER_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataTypes.h>

namespace OsmAnd {

    class ObfReader;
    class ObfAddressSectionInfo;
    namespace Model {
        class StreetGroup;
        class Street;
        class Building;
        class StreetIntersection;
    } // namespace Model
    class IQueryController;

    class OSMAND_CORE_API ObfAddressSectionReader
    {
    private:
        ObfAddressSectionReader();
        ~ObfAddressSectionReader();
    protected:
    public:
        static void loadStreetGroups(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
            QList< std::shared_ptr<const Model::StreetGroup> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetGroup>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr,
            QSet<ObfAddressBlockType>* blockTypeFilter = nullptr);

        static void loadStreetsFromGroup(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Model::StreetGroup>& group,
            QList< std::shared_ptr<const Model::Street> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Street>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void loadBuildingsFromStreet(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Model::Street>& street,
            QList< std::shared_ptr<const Model::Building> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Building>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void loadIntersectionsFromStreet(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Model::Street>& street,
            QList< std::shared_ptr<const Model::StreetIntersection> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetIntersection>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);
    };

} // namespace OsmAnd

#endif // __OBF_ADDRESS_SECTION_READER_H_
