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
        typedef std::function<bool(const std::shared_ptr<const OsmAnd::Amenity>&)> VisitorFunction;

    private:
        ObfPoiSectionReader();
        ~ObfPoiSectionReader();
    protected:
    public:
        static void loadCategories(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            std::shared_ptr<const ObfPoiSectionCategories>& outCategories,
            const IQueryController* const controller = nullptr);

        static void loadSubtypes(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            std::shared_ptr<const ObfPoiSectionSubtypes>& outSubtypes,
            const IQueryController* const controller = nullptr);

        static void loadAmenities(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
            const ZoomLevel minZoom = MinZoomLevel,
            const ZoomLevel maxZoom = MaxZoomLevel,
            const AreaI* const bbox31 = nullptr,
            const QSet<ObfPoiCategoryId>* const categoriesFilter = nullptr,
            const ObfPoiSectionReader::VisitorFunction visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void scanAmenitiesByName(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfPoiSectionInfo>& section,
            const QString& query,
            QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
            const ZoomLevel minZoom = MinZoomLevel,
            const ZoomLevel maxZoom = MaxZoomLevel,
            const AreaI* const bbox31 = nullptr,
            const QSet<ObfPoiCategoryId>* const categoriesFilter = nullptr,
            const ObfPoiSectionReader::VisitorFunction visitor = nullptr,
            const IQueryController* const controller = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_POI_SECTION_READER_H_)
