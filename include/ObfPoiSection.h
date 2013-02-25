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
#include <QList>
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
        ObfPoiSection(class ObfReader* owner);
        virtual ~ObfPoiSection();

        //QList< std::shared_ptr<PoiCategory> > _categories;
        //List<AmenityType> categoriesType = new ArrayList<AmenityType>();
        
        double _leftLongitude;
        double _rightLongitude;
        double _topLatitude;
        double _bottomLatitude;

        static void loadCategories(OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section, QList< std::shared_ptr<Model::Amenity::Category> >& categories);
        static void loadAmenities(OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section, QList< std::shared_ptr<OsmAnd::Model::Amenity> >& amenities);
    protected:
        static void read(ObfReader* reader, ObfPoiSection* section);

    private:
        static void readBoundaries(ObfReader* reader, ObfPoiSection* section);
        static void readCategories(ObfReader* reader, ObfPoiSection* section, QList< std::shared_ptr<Model::Amenity::Category> >& categories);
        static void readCategory(ObfReader* reader, Model::Amenity::Category* category);
        static void readAmenities(ObfReader* reader, ObfPoiSection* section, QList< std::shared_ptr<Model::Amenity> >& amenities);
        struct Tile
        {
            uint32_t _zoom;
            int32_t _x;
            int32_t _y;
            uint64_t _hash;
            int32_t _offset;
        };
        static void readTiles(ObfReader* reader, ObfPoiSection* section, QList< std::shared_ptr<Tile> >& tiles);
        static void readTile(ObfReader* reader, ObfPoiSection* section, QList< std::shared_ptr<Tile> >& tiles, Tile* parent);
        static void readAmenitiesFromTiles(ObfReader* reader, ObfPoiSection* section, const QList< std::shared_ptr<Tile> >& tiles, QList< std::shared_ptr<Model::Amenity> >& amenities);
        static void readAmenitiesFromTile(ObfReader* reader, ObfPoiSection* section, Tile* tile, QList< std::shared_ptr<Model::Amenity> >& amenities);
        static void readAmenity(ObfReader* reader, ObfPoiSection* section, int32_t px, int32_t py, uint32_t pzoom, std::shared_ptr<Model::Amenity>& amenity);
        
    friend class ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_POI_REGION_SECTION_H_