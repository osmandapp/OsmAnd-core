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

#ifndef __OBF_POI_SECTION_H_
#define __OBF_POI_SECTION_H_

#include <functional>
#include <memory>

#include <QList>
#include <QHash>
#include <QSet>

#include <google/protobuf/io/coded_stream.h>

#include <OsmAndCore.h>
#include <ObfSection.h>
#include <Amenity.h>
#include <QueryFilter.h>
#include <IQueryController.h>
#include <Area.h>

namespace OsmAnd {

    namespace gpb = google::protobuf;

    /**
    'POI' section of OsmAnd Binary File
    */
    struct OSMAND_CORE_API ObfPoiSection : public ObfSection
    {
        ObfPoiSection(class ObfReader* owner);
        virtual ~ObfPoiSection();

        AreaI _area31;
        AreaD _areaGeo;
        
        static void loadCategories(OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section, QList< std::shared_ptr<Model::Amenity::Category> >& categories);
        static void loadAmenities(OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section, 
            QSet<uint32_t>* desiredCategories = nullptr,
            QList< std::shared_ptr<OsmAnd::Model::Amenity> >* amenitiesOut = nullptr,
            QueryFilter* filter = nullptr, uint32_t zoomToSkipFilter = 3,
            std::function<bool (const std::shared_ptr<OsmAnd::Model::Amenity>&)> visitor = nullptr,
            IQueryController* controller = nullptr);
    protected:
        static void read(ObfReader* reader, ObfPoiSection* section);

    private:
        const static uint32_t SubcategoryIdShift = 7;
        const static uint32_t CategoryIdMask = (1 << SubcategoryIdShift) - 1;
        static void readBoundaries(ObfReader* reader, ObfPoiSection* section);
        static void readCategories(ObfReader* reader, ObfPoiSection* section, QList< std::shared_ptr<Model::Amenity::Category> >& categories);
        static void readCategory(ObfReader* reader, Model::Amenity::Category* category);
        static void readAmenities(ObfReader* reader, ObfPoiSection* section,
            QSet<uint32_t>* desiredCategories,
            QList< std::shared_ptr<OsmAnd::Model::Amenity> >* amenitiesOut,
            QueryFilter* filter, uint32_t zoomToSkipFilter,
            std::function<bool (const std::shared_ptr<OsmAnd::Model::Amenity>&)> visitor,
            IQueryController* controller);
        struct Tile
        {
            uint32_t _zoom;
            uint32_t _x;
            uint32_t _y;
            uint64_t _hash;
            int32_t _offset;
        };
        static bool readTile(ObfReader* reader, ObfPoiSection* section,
            QList< std::shared_ptr<Tile> >& tiles,
            Tile* parent,
            QSet<uint32_t>* desiredCategories,
            QueryFilter* filter,
            uint32_t zoomToSkipFilter,
            IQueryController* controller,
            QSet< uint64_t >* tilesToSkip);
        static bool checkTileCategories(ObfReader* reader, ObfPoiSection* section,
            QSet<uint32_t>* desiredCategories);
        static void readAmenitiesFromTile(ObfReader* reader, ObfPoiSection* section, Tile* tile,
            QSet<uint32_t>* desiredCategories,
            QList< std::shared_ptr<OsmAnd::Model::Amenity> >* amenitiesOut,
            QueryFilter* filter, uint32_t zoomToSkipFilter,
            std::function<bool (const std::shared_ptr<OsmAnd::Model::Amenity>&)> visitor,
            IQueryController* controller,
            QSet< uint64_t >* amenitiesToSkip);
        static void readAmenity(ObfReader* reader, ObfPoiSection* section, int32_t px, int32_t py, uint32_t pzoom, std::shared_ptr<Model::Amenity>& amenity,
            QSet<uint32_t>* desiredCategories,
            QueryFilter* filter,
            IQueryController* controller);
        
    friend class OsmAnd::ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_POI_SECTION_H_