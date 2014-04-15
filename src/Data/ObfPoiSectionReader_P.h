#ifndef _OSMAND_CORE_OBF_POI_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_POI_SECTION_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "Reference.h"

namespace OsmAnd
{
    class ObfReader_P;
    class ObfPoiSectionInfo;
    namespace Model
    {
        class Amenity;
        class AmenityCategory;
    }
    class IQueryController;

    class ObfPoiSectionReader;
    class ObfPoiSectionReader_P
    {
    private:
        ObfPoiSectionReader_P();
        ~ObfPoiSectionReader_P();
    protected:
        static void read(const ObfReader_P& reader, const std::shared_ptr<ObfPoiSectionInfo>& section);
        static void readBoundaries(const ObfReader_P& reader, const std::shared_ptr<ObfPoiSectionInfo>& section);

        enum {
            SubcategoryIdShift = 7,
            CategoryIdMask = (1u << SubcategoryIdShift) - 1,
        };
        static void readCategories(const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QList< Fwd<Model::AmenityCategory>::RefC >& categories);
        static void readCategory(const ObfReader_P& reader, const std::shared_ptr<Model::AmenityCategory>& category);
        static void readAmenities(const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QSet<uint32_t>* desiredCategories,
            QList< Fwd<Model::Amenity>::RefC >* amenitiesOut,
            const ZoomLevel zoom, uint32_t zoomDepth, const AreaI* bbox31,
            std::function<bool (Fwd<Model::Amenity>::NCRefC)> visitor,
            const IQueryController* const controller);

        struct Tile
        {
            uint32_t _zoom;
            uint32_t _x;
            uint32_t _y;
            uint64_t _hash;
            int32_t _offset;
        };
        static bool readTile(const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QList< std::shared_ptr<Tile> >& tiles,
            Tile* parent,
            QSet<uint32_t>* desiredCategories,
            uint32_t zoom, uint32_t zoomDepth, const AreaI* bbox31,
            const IQueryController* const controller,
            QSet< uint64_t >* tilesToSkip);
        static bool checkTileCategories(const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QSet<uint32_t>* desiredCategories);
        static void readAmenitiesFromTile(const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section, Tile* tile,
            QSet<uint32_t>* desiredCategories,
            QList< Fwd<Model::Amenity>::RefC >* amenitiesOut,
            const ZoomLevel zoom, uint32_t zoomDepth, const AreaI* bbox31,
            std::function<bool (Fwd<Model::Amenity>::NCRefC)> visitor,
            const IQueryController* const controller,
            QSet< uint64_t >* amenitiesToSkip);
        static void readAmenity(const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
            const PointI& pTile, uint32_t pzoom, Fwd<Model::Amenity>::OutRefC& amenity,
            QSet<uint32_t>* desiredCategories,
            const AreaI* bbox31,
            const IQueryController* const controller);

        static void loadCategories(const ObfReader_P& reader, const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section,
            QList< std::shared_ptr<const Model::AmenityCategory> >& categories);

        static void loadAmenities(const ObfReader_P& reader, const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section,
            const ZoomLevel zoom, uint32_t zoomDepth = 3, const AreaI* bbox31 = nullptr,
            QSet<uint32_t>* desiredCategories = nullptr,
            QList< Fwd<Model::Amenity>::RefC >* amenitiesOut = nullptr,
            std::function<bool (Fwd<Model::Amenity>::NCRefC)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        friend class OsmAnd::ObfReader_P;
        friend class OsmAnd::ObfPoiSectionReader;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_POI_SECTION_READER_P_H_)
