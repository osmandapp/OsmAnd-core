#ifndef _OSMAND_CORE_OBF_POI_SECTION_READER_H_
#define _OSMAND_CORE_OBF_POI_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataCommonTypes.h>

namespace OsmAnd
{
    class ObfReader;
    class ObfPoiSectionCategories;
    class ObfPoiSectionSubtypes;
    class ObfPoiSectionInfo;
    class Amenity;
    class IQueryController;

    class OSMAND_CORE_API ObfPoiSectionReader
    {
    public:
        typedef std::function<bool(const std::shared_ptr<const OsmAnd::Amenity>& amenity)> VisitorFunction;

    private:
        ObfPoiSectionReader();
        ~ObfPoiSectionReader();
    protected:
    public:
        static void loadCategories(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            std::shared_ptr<const ObfPoiSectionCategories>& outCategories,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void loadSubtypes(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            std::shared_ptr<const ObfPoiSectionSubtypes>& outSubtypes,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void loadAmenities(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
            const AreaI* const bbox31 = nullptr,
            const TileAcceptorFunction tileFilter = nullptr,
            const ZoomLevel zoomFilter = InvalidZoomLevel,
            const QSet<ObfPoiCategoryId>* const categoriesFilter = nullptr,
            const QPair<int, int>* poiAdditionalFilter = nullptr,
            const ObfPoiSectionReader::VisitorFunction visitor = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void scanAmenitiesByName(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            const QString& query,
            QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
            const PointI* const xy31 = nullptr,
            const AreaI* const bbox31 = nullptr,
            const TileAcceptorFunction tileFilter = nullptr,
            const QSet<ObfPoiCategoryId>* const categoriesFilter = nullptr,
            const QPair<int, int>* poiAdditionalFilter = nullptr,
            const ObfPoiSectionReader::VisitorFunction visitor = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr,
            const bool strictMatch = false);
        
        static void ensureCategoriesLoaded(
            const ObfReader& reader,
                                           const std::shared_ptr<const ObfPoiSectionInfo>& section);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_POI_SECTION_READER_H_)
