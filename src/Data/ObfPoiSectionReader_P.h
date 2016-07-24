#ifndef _OSMAND_CORE_OBF_POI_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_POI_SECTION_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMap>
#include "restore_internal_warnings.h"

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
        enum {
            ZoomToSkipFilterRead = 5,
            ZoomToSkipFilter = 3,
        };

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
                const AreaI* const bbox31,
                const TileAcceptorFunction tileFilter,
                const ZoomLevel zoomFilter,
                const QSet<ObfPoiCategoryId>* const categoriesFilter,
                const ObfPoiSectionReader::VisitorFunction visitor,
                const std::shared_ptr<const IQueryController>& queryController);
        static bool scanTiles(
                const ObfReader_P& reader,
                QMap<uint32_t, uint64_t>& outDataOffsetsMap,
                QSet<uint64_t>& tilesToSkip,
                const ZoomLevel parentZoom,
                const TileId parentTileId,
                const AreaI* const bbox31,
                const TileAcceptorFunction tileFilter,
                const ZoomLevel zoomFilter,
                const QSet<ObfPoiCategoryId>* const categoriesFilter);
        static bool scanTileForMatchingCategories(
                const ObfReader_P& reader,
                const QSet<ObfPoiCategoryId>& categories);

        static void readAmenitiesByName(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfPoiSectionInfo>& section,
                const QString& query,
                QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
                const AreaI* const bbox31,
                const TileAcceptorFunction tileFilter,
                const QSet<ObfPoiCategoryId>* const categoriesFilter,
                const ObfPoiSectionReader::VisitorFunction visitor,
                const std::shared_ptr<const IQueryController>& queryController);
        static void scanNameIndex(const ObfReader_P& reader,
                const QString &query,
                QSet<uint32_t>& outDataOffsets, const AreaI * const bbox31,
                const TileAcceptorFunction tileFilter);
        static void readNameIndexData(
                const ObfReader_P& reader,
                QSet<uint32_t>& outDataOffsets,
                const AreaI* const bbox31,
                const TileAcceptorFunction tileFilter);
        static void readNameIndexDataAtom(
                const ObfReader_P& reader,
                QSet<uint32_t>& outDataOffsets,
                const AreaI* const bbox31,
                const TileAcceptorFunction tileFilter);

        static bool readAmenitiesDataBox(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfPoiSectionInfo>& section,
                QSet<ObfObjectId>& processedObjects,
                QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
                const QString& query,
                const AreaI* const bbox31,
                const TileAcceptorFunction tileFilter,
                const ZoomLevel zoomFilter,
                QSet<uint64_t>* const pTilesToSkip,
                const QSet<ObfPoiCategoryId>* const categoriesFilter,
                const ObfPoiSectionReader::VisitorFunction visitor,
                const std::shared_ptr<const IQueryController>& queryController);
        static void readAmenity(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfPoiSectionInfo>& section,
                std::shared_ptr<const Amenity>& outAmenity,
                const QString& query,
                const TileId boxTileId,
                const ZoomLevel boxZoom,
                const AreaI* const bbox31,
                const QSet<ObfPoiCategoryId>* const categoriesFilter,
                const std::shared_ptr<const IQueryController>& queryController);
    public:
        static void loadCategories(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfPoiSectionInfo>& section,
                std::shared_ptr<const ObfPoiSectionCategories>& outCategories,
                const std::shared_ptr<const IQueryController>& queryController);

        static void loadSubtypes(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfPoiSectionInfo>& section,
                std::shared_ptr<const ObfPoiSectionSubtypes>& outSubtypes,
                const std::shared_ptr<const IQueryController>& queryController);

        static void loadAmenities(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfPoiSectionInfo>& section,
                QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
                const AreaI* const bbox31,
                const TileAcceptorFunction tileFilter,
                const ZoomLevel zoomFilter,
                const QSet<ObfPoiCategoryId>* const categoriesFilter,
                const ObfPoiSectionReader::VisitorFunction visitor,
                const std::shared_ptr<const IQueryController>& queryController);

        static void scanAmenitiesByName(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfPoiSectionInfo>& section,
                const QString& query,
                QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
                const AreaI* const bbox31,
                const TileAcceptorFunction tileFilter,
                const QSet<ObfPoiCategoryId>* const categoriesFilter,
                const ObfPoiSectionReader::VisitorFunction visitor,
                const std::shared_ptr<const IQueryController>& queryController);

        friend class OsmAnd::ObfReader_P;
        friend class OsmAnd::ObfPoiSectionReader;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_POI_SECTION_READER_P_H_)
