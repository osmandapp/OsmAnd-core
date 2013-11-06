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

#ifndef __OBF_ADDRESS_SECTION_READER_P_H_
#define __OBF_ADDRESS_SECTION_READER_P_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <OsmAndCore.h>
#include <QtGlobal>
namespace OsmAnd {
    STRONG_ENUM(ObfAddressBlockType);
}
inline uint qHash(const OsmAnd::ObfAddressBlockType value, uint seed = 0) Q_DECL_NOTHROW;

#include <QList>
#include <QSet>

#include <CommonTypes.h>
#include <ObfMapSectionInfo_P.h>
#include <MapTypes.h>

namespace OsmAnd {

    class ObfReader_P;
    class ObfAddressSectionInfo;
    class ObfAddressBlocksSectionInfo;
    class ObfAddressSectionReader;
    namespace Model {
        class StreetGroup;
        class Street;
        class Building;
        class StreetIntersection;
    } // namespace Model
    STRONG_ENUM(ObfAddressBlockType);
    class IQueryController;

    class ObfAddressSectionReader_P
    {
    private:
        ObfAddressSectionReader_P();
        ~ObfAddressSectionReader_P();
    protected:
        static void read(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfAddressSectionInfo>& section);

        static void readAddressBlocksSectionHeader(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfAddressBlocksSectionInfo>& section);

        static void readStreetGroups(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
            QList< std::shared_ptr<const Model::StreetGroup> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetGroup>&)> visitor,
            const IQueryController* const controller,
            QSet<ObfAddressBlockType>* blockTypeFilter = nullptr);
        static void readStreetGroupsFromAddressBlocksSection(
            const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfAddressBlocksSectionInfo>& section,
            QList< std::shared_ptr<const Model::StreetGroup> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetGroup>&)> visitor,
            const IQueryController* const controller);
        static void readStreetGroupHeader( const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfAddressBlocksSectionInfo>& section,
            unsigned int offset, std::shared_ptr<OsmAnd::Model::StreetGroup>& outGroup );

        static void readStreetsFromGroup(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::StreetGroup>& group,
            QList< std::shared_ptr<const Model::Street> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Street>&)> visitor,
            const IQueryController* const controller);

        static void readStreet(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::StreetGroup>& group,
            const std::shared_ptr<Model::Street>& street);

        static void readBuildingsFromStreet(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
            QList< std::shared_ptr<const Model::Building> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Building>&)> visitor,
            const IQueryController* const controller);

        static void readBuilding(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
            const std::shared_ptr<Model::Building>& building);

        static void readIntersectionsFromStreet(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
            QList< std::shared_ptr<const Model::StreetIntersection> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetIntersection>&)> visitor,
            const IQueryController* const controller);

        static void readIntersectedStreet(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
            const std::shared_ptr<Model::StreetIntersection>& intersection);

        static void loadStreetGroups(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
            QList< std::shared_ptr<const Model::StreetGroup> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetGroup>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr,
            QSet<ObfAddressBlockType>* blockTypeFilter = nullptr);

        static void loadStreetsFromGroup(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::StreetGroup>& group,
            QList< std::shared_ptr<const Model::Street> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Street>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void loadBuildingsFromStreet(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
            QList< std::shared_ptr<const Model::Building> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Building>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void loadIntersectionsFromStreet(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
            QList< std::shared_ptr<const Model::StreetIntersection> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetIntersection>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

    friend class OsmAnd::ObfReader_P;
    friend class OsmAnd::ObfAddressSectionReader;
    };

} // namespace OsmAnd

#endif // __OBF_ADDRESS_SECTION_READER_P_H_
