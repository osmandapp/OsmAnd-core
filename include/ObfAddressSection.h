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

#ifndef __OBF_ADDRESS_REGION_SECTION_H_
#define __OBF_ADDRESS_REGION_SECTION_H_

#include <OsmAndCore.h>
#include <memory>
#include <list>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <string>
#include <google/protobuf/io/coded_stream.h>
#include <ObfSection.h>
#include <StreetGroup.h>
#include <Street.h>
#include <Building.h>
#include <OBF.pb.h>

namespace OsmAnd {

    namespace gpb = google::protobuf;

    /**
    'Address' section of OsmAnd Binary File
    */
    struct OSMAND_CORE_API ObfAddressSection : public ObfSection
    {
        struct OSMAND_CORE_API AddressBlocksSection : public ObfSection
        {
            typedef OBF::OsmAndAddressIndex_CitiesIndex_CitiesType _Type;
            enum Type : int
            {
                CitiesOrTowns = _Type::OsmAndAddressIndex_CitiesIndex_CitiesType_CitiesOrTowns,
                Villages = _Type::OsmAndAddressIndex_CitiesIndex_CitiesType_Villages,
                Postcodes = _Type::OsmAndAddressIndex_CitiesIndex_CitiesType_Postcodes,
            };

            AddressBlocksSection(class ObfReader* owner);
            virtual ~AddressBlocksSection();

            Type _type;

            void loadSteetGroupsFromBlock(std::list< std::shared_ptr<Model::StreetGroup> >& list);
        protected:
            static void read(ObfReader* reader, AddressBlocksSection* section);
            static void readStreetGroups(ObfReader* reader, AddressBlocksSection* section, std::list< std::shared_ptr<Model::StreetGroup> >& list);
            static std::shared_ptr<Model::StreetGroup> readStreetGroupHeader(ObfReader* reader, AddressBlocksSection* section, unsigned int offset);

        private:

        friend struct ObfAddressSection;
        };

        ObfAddressSection(class ObfReader* owner);
        virtual ~ObfAddressSection();

        QString _latinName;
        
        //! ???
        int _indexNameOffset;

        std::list< std::shared_ptr<AddressBlocksSection> > _blocksSections;
        
        //LatLon calculatedCenter = null;

        void loadStreetGroups(std::list< std::shared_ptr<Model::StreetGroup> >& list, uint8_t typeBitmask = std::numeric_limits<uint8_t>::max());
        static void loadStreetsFromGroup(ObfReader* reader, Model::StreetGroup* group, std::list< std::shared_ptr<Model::Street> >& list);
        static void loadBuildingsFromStreet(ObfReader* reader, Model::Street* street, std::list< std::shared_ptr<Model::Building> >& list);
    protected:
        static void read(ObfReader* reader, ObfAddressSection* section);
        static void readStreetGroups(ObfReader* reader, ObfAddressSection* section, std::list< std::shared_ptr<Model::StreetGroup> >& list, uint8_t typeBitmask = std::numeric_limits<uint8_t>::max());
        static void readStreetsFromGroup(ObfReader* reader, Model::StreetGroup* group, std::list< std::shared_ptr<Model::Street> >& list);
        static void readStreet(ObfReader* reader, Model::StreetGroup* group, Model::Street* street);
        static void readBuildingsFromStreet(ObfReader* reader, Model::Street* street, std::list< std::shared_ptr<Model::Building> >& list);

    private:

    friend class ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_ADDRESS_REGION_SECTION_H_