#ifndef _OSMAND_CORE_OBF_POI_SECTION_READER_H_
#define _OSMAND_CORE_OBF_POI_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Reference.h>

namespace OsmAnd
{
    class ObfReader;
    class ObfPoiSectionInfo;
    namespace Model
    {
        class Amenity;
        class AmenityCategory;
    }
    class IQueryController;

    class OSMAND_CORE_API ObfPoiSectionReader
    {
    private:
        ObfPoiSectionReader();
        ~ObfPoiSectionReader();
    protected:
    public:
        static void loadCategories(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section,
            QList< Fwd<Model::AmenityCategory>::RefC >& categories);

        static void loadAmenities(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section,
            const ZoomLevel zoom, uint32_t zoomDepth = 3, const AreaI* bbox31 = nullptr,
            QSet<uint32_t>* desiredCategories = nullptr,
            QList< Fwd<Model::Amenity>::RefC >* amenitiesOut = nullptr,
            std::function<bool (Fwd<Model::Amenity>::NCRefC)> visitor = nullptr,
            const IQueryController* const controller = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_POI_SECTION_READER_H_)
