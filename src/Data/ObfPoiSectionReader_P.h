#ifndef _OSMAND_CORE_OBF_POI_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_POI_SECTION_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "DataCommonTypes.h"
#include "ObfPoiSectionReader.h"
#include "ObfPoiSectionInfo.h"

namespace OsmAnd
{
    class ObfReader_P;
    class ObfPoiSectionInfo;
    class Amenity;
    class IQueryController;

    class ObfPoiSectionReader;
    class ObfPoiSectionReader_P Q_DECL_FINAL
    {
    public:
        typedef ObfPoiSectionReader::VisitorFunction VisitorFunction;

    private:
        ObfPoiSectionReader_P();
        ~ObfPoiSectionReader_P();
    protected:
        static void read(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfPoiSectionInfo>& section);

        static void readCategories(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfPoiSectionCategories>& categories);
        static void readCategoriesEntry(
            const ObfReader_P& reader,
            QString& outMainCategory,
            QStringList& outSubcategories);
        static void ensureCategoriesLoaded(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section);

        static void readSubtypes(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfPoiSectionSubtypes>& subtypes);
        static void readSubtypesStructure(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfPoiSectionSubtypes>& subtypes);
        static void readSubtype(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfPoiSectionSubtype>& subtype);
        static void ensureSubtypesLoaded(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section);

        static void readAmenities(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
            const ZoomLevel minZoom,
            const ZoomLevel maxZoom,
            const AreaI* const bbox31,
            const QSet<ObfPoiCategoryId>* const categoriesFilter,
            const ObfPoiSectionReader::VisitorFunction visitor,
            const IQueryController* const controller);
        static void scanTiles(
            const ObfReader_P& reader,
            QSet<uint32_t>& outDataOffsets,
            const ZoomLevel parentZoom,
            const TileId parentTileId,
            const ZoomLevel minZoom,
            const ZoomLevel maxZoom,
            const AreaI* const bbox31,
            const QSet<ObfPoiCategoryId>* const categoriesFilter);
        static bool scanTileForMatchingCategories(
            const ObfReader_P& reader,
            const QSet<ObfPoiCategoryId>& categories);

        static void readAmenitiesByName(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            const QString& query,
            QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
            const ZoomLevel minZoom,
            const ZoomLevel maxZoom,
            const AreaI* const bbox31,
            const QSet<ObfPoiCategoryId>* const categoriesFilter,
            const ObfPoiSectionReader::VisitorFunction visitor,
            const IQueryController* const controller);
        static void scanNameIndex(
            const ObfReader_P& reader,
            const QString& query,
            QSet<uint32_t>& outDataOffsets,
            const ZoomLevel minZoom,
            const ZoomLevel maxZoom,
            const AreaI* const bbox31);
        static void readNameIndexData(
            const ObfReader_P& reader,
            QSet<uint32_t>& outDataOffsets,
            const ZoomLevel minZoom,
            const ZoomLevel maxZoom,
            const AreaI* const bbox31);
        static void readNameIndexDataAtom(
            const ObfReader_P& reader,
            QSet<uint32_t>& outDataOffsets,
            const ZoomLevel minZoom,
            const ZoomLevel maxZoom,
            const AreaI* const bbox31);

        static void readAmenitiesDataBox(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QSet<ObfObjectId>& processedObjects,
            QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
            const QString& query,
            const ZoomLevel minZoom,
            const ZoomLevel maxZoom,
            const AreaI* const bbox31,
            const QSet<ObfPoiCategoryId>* const categoriesFilter,
            const ObfPoiSectionReader::VisitorFunction visitor,
            const IQueryController* const controller);
        static void readAmenity(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            std::shared_ptr<const Amenity>& outAmenity,
            const QString& query,
            const ZoomLevel zoom,
            const TileId boxTileId,
            const AreaI* const bbox31,
            const QSet<ObfPoiCategoryId>* const categoriesFilter,
            const IQueryController* const controller);
    public:
        static void loadCategories(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            std::shared_ptr<const ObfPoiSectionCategories>& outCategories,
            const IQueryController* const controller);

        static void loadSubtypes(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            std::shared_ptr<const ObfPoiSectionSubtypes>& outSubtypes,
            const IQueryController* const controller);

        static void loadAmenities(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
            const ZoomLevel minZoom,
            const ZoomLevel maxZoom,
            const AreaI* const bbox31,
            const QSet<ObfPoiCategoryId>* const categoriesFilter,
            const ObfPoiSectionReader::VisitorFunction visitor,
            const IQueryController* const controller);

        static void scanAmenitiesByName(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            const QString& query,
            QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
            const ZoomLevel minZoom,
            const ZoomLevel maxZoom,
            const AreaI* const bbox31,
            const QSet<ObfPoiCategoryId>* const categoriesFilter,
            const ObfPoiSectionReader::VisitorFunction visitor,
            const IQueryController* const controller);

    friend class OsmAnd::ObfReader_P;
    friend class OsmAnd::ObfPoiSectionReader;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_POI_SECTION_READER_P_H_)
