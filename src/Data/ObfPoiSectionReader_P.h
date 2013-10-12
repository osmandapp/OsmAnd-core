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

#ifndef __OBF_POI_SECTION_READER_P_H_
#define __OBF_POI_SECTION_READER_P_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfReader_P;
    class ObfPoiSectionInfo;
    namespace Model {
        class Amenity;
        class AmenityCategory;
    } // namespace Model
    class IQueryController;

    class ObfPoiSectionReader;
    class OSMAND_CORE_API ObfPoiSectionReader_P
    {
    private:
        ObfPoiSectionReader_P();
        ~ObfPoiSectionReader_P();
    protected:
        static void read(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfPoiSectionInfo>& section);
        static void readBoundaries(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfPoiSectionInfo>& section);

        enum {
            SubcategoryIdShift = 7,
            CategoryIdMask = (1u << SubcategoryIdShift) - 1,
        };
        static void readCategories(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QList< std::shared_ptr<const Model::AmenityCategory> >& categories);
        static void readCategory(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<Model::AmenityCategory>& category);
        static void readAmenities(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QSet<uint32_t>* desiredCategories,
            QList< std::shared_ptr<const Model::Amenity> >* amenitiesOut,
            const ZoomLevel zoom, uint32_t zoomDepth, const AreaI* bbox31,
            std::function<bool (const std::shared_ptr<const Model::Amenity>&)> visitor,
            IQueryController* controller);

        struct Tile
        {
            uint32_t _zoom;
            uint32_t _x;
            uint32_t _y;
            uint64_t _hash;
            int32_t _offset;
        };
        static bool readTile(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QList< std::shared_ptr<Tile> >& tiles,
            Tile* parent,
            QSet<uint32_t>* desiredCategories,
            uint32_t zoom, uint32_t zoomDepth, const AreaI* bbox31,
            IQueryController* controller,
            QSet< uint64_t >* tilesToSkip);
        static bool checkTileCategories(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QSet<uint32_t>* desiredCategories);
        static void readAmenitiesFromTile(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section, Tile* tile,
            QSet<uint32_t>* desiredCategories,
            QList< std::shared_ptr<const Model::Amenity> >* amenitiesOut,
            const ZoomLevel zoom, uint32_t zoomDepth, const AreaI* bbox31,
            std::function<bool (const std::shared_ptr<const Model::Amenity>&)> visitor,
            IQueryController* controller,
            QSet< uint64_t >* amenitiesToSkip);
        static void readAmenity(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
            const PointI& pTile, uint32_t pzoom, std::shared_ptr<Model::Amenity>& amenity,
            QSet<uint32_t>* desiredCategories,
            const AreaI* bbox31,
            IQueryController* controller);

        static void loadCategories(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section,
            QList< std::shared_ptr<const Model::AmenityCategory> >& categories);

        static void loadAmenities(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section,
            const ZoomLevel zoom, uint32_t zoomDepth = 3, const AreaI* bbox31 = nullptr,
            QSet<uint32_t>* desiredCategories = nullptr,
            QList< std::shared_ptr<const OsmAnd::Model::Amenity> >* amenitiesOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Amenity>&)> visitor = nullptr,
            IQueryController* controller = nullptr);

        friend class OsmAnd::ObfReader_P;
        friend class OsmAnd::ObfPoiSectionReader;
    };

} // namespace OsmAnd

#endif // __OBF_POI_SECTION_READER_P_H_
