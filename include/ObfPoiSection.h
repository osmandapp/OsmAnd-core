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
#include <IQueryFilter.h>
#include <IQueryController.h>
#include <CommonTypes.h>

namespace OsmAnd {

    class ObfReader;
    namespace gpb = google::protobuf;

    /**
    'POI' section of OsmAnd Binary File
    */
    class OSMAND_CORE_API ObfPoiSection : public ObfSection
    {
    private:
        AreaI _area31;

        enum {
            SubcategoryIdShift = 7,
            CategoryIdMask = (1u << SubcategoryIdShift) - 1,
        };
        static void readBoundaries(ObfReader* reader, ObfPoiSection* section);
        static void readCategories(ObfReader* reader, ObfPoiSection* section, QList< std::shared_ptr<Model::Amenity::Category> >& categories);
        static void readCategory(ObfReader* reader, Model::Amenity::Category* category);
        static void readAmenities(ObfReader* reader, ObfPoiSection* section,
            QSet<uint32_t>* desiredCategories,
            QList< std::shared_ptr<OsmAnd::Model::Amenity> >* amenitiesOut,
            uint32_t zoom, uint32_t zoomDepth, const AreaI* bbox31,
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
            uint32_t zoom, uint32_t zoomDepth, const AreaI* bbox31,
            IQueryController* controller,
            QSet< uint64_t >* tilesToSkip);
        static bool checkTileCategories(ObfReader* reader, ObfPoiSection* section,
            QSet<uint32_t>* desiredCategories);
        static void readAmenitiesFromTile(ObfReader* reader, ObfPoiSection* section, Tile* tile,
            QSet<uint32_t>* desiredCategories,
            QList< std::shared_ptr<OsmAnd::Model::Amenity> >* amenitiesOut,
            uint32_t zoom, uint32_t zoomDepth, const AreaI* bbox31,
            std::function<bool (const std::shared_ptr<OsmAnd::Model::Amenity>&)> visitor,
            IQueryController* controller,
            QSet< uint64_t >* amenitiesToSkip);
        static void readAmenity(ObfReader* reader, ObfPoiSection* section, const PointI& pTile, uint32_t pzoom, std::shared_ptr<Model::Amenity>& amenity,
            QSet<uint32_t>* desiredCategories,
            const AreaI* bbox31,
            IQueryController* controller);
    protected:
        ObfPoiSection(ObfReader* owner);

        static void read(ObfReader* reader, ObfPoiSection* section);
    public:
        virtual ~ObfPoiSection();

        const AreaI& area31;

        static void loadCategories(OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section, QList< std::shared_ptr<Model::Amenity::Category> >& categories);
        static void loadAmenities(OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section, 
            uint32_t zoom, uint32_t zoomDepth = 3, const AreaI* bbox31 = nullptr,
            QSet<uint32_t>* desiredCategories = nullptr,
            QList< std::shared_ptr<OsmAnd::Model::Amenity> >* amenitiesOut = nullptr,
            std::function<bool (const std::shared_ptr<OsmAnd::Model::Amenity>&)> visitor = nullptr,
            IQueryController* controller = nullptr);
        
    friend class OsmAnd::ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_POI_SECTION_H_