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

#ifndef __OBF_POI_REGION_SECTION_H_
#define __OBF_POI_REGION_SECTION_H_

#include <OsmAndCore.h>
#include <memory>
#include <list>
#include <QString>
#include <google/protobuf/io/coded_stream.h>
#include <ObfSection.h>
#include <Amenity.h>

namespace OsmAnd {

    namespace gpb = google::protobuf;

    /**
    'POI' section of OsmAnd Binary File
    */
    struct OSMAND_CORE_API ObfPoiSection : public ObfSection
    {
        struct OSMAND_CORE_API PoiCategory
        {
            QString _name;
            std::list<QString> _subcategories;
        };

        ObfPoiSection(class ObfReader* owner);
        virtual ~ObfPoiSection();

        //std::list< std::shared_ptr<PoiCategory> > _categories;
        //List<AmenityType> categoriesType = new ArrayList<AmenityType>();
        
        double _leftLongitude;
        double _rightLongitude;
        double _topLatitude;
        double _bottomLatitude;

        static void loadCategories(OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section, std::list< std::shared_ptr<OsmAnd::ObfPoiSection::PoiCategory> >& categories);
        static void loadAmenities(OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section, std::list< std::shared_ptr<OsmAnd::Model::Amenity> >& amenities);
    protected:
        static void read(ObfReader* reader, ObfPoiSection* section);

    private:
        static void readPoiBoundariesIndex(ObfReader* reader, ObfPoiSection* section);
        static void readCategories(ObfReader* reader, ObfPoiSection* section, std::list< std::shared_ptr<OsmAnd::ObfPoiSection::PoiCategory> >& categories);
        static void readCategory(ObfReader* reader, ObfPoiSection::PoiCategory* category);
        static void readAmenities(ObfReader* reader, ObfPoiSection* section, std::list< std::shared_ptr<OsmAnd::Model::Amenity> >& amenities);
        static void readPoiBox(ObfReader* reader, ObfPoiSection* section);
        
    friend class ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_POI_REGION_SECTION_H_