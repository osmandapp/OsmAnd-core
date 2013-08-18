#include "ObfPoiSectionReader.h"
#include "ObfPoiSectionReader_P.h"

#include "ObfReader.h"

OsmAnd::ObfPoiSectionReader::ObfPoiSectionReader()
{
}

OsmAnd::ObfPoiSectionReader::~ObfPoiSectionReader()
{
}

void OsmAnd::ObfPoiSectionReader::loadCategories(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section,
    QList< std::shared_ptr<const Model::AmenityCategory> >& categories )
{
    ObfPoiSectionReader_P::loadCategories(reader->_d, section, categories);
}

void OsmAnd::ObfPoiSectionReader::loadAmenities(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section,
    const ZoomLevel& zoom, uint32_t zoomDepth /*= 3*/, const AreaI* bbox31 /*= nullptr*/, QSet<uint32_t>* desiredCategories /*= nullptr*/,
    QList< std::shared_ptr<const OsmAnd::Model::Amenity> >* amenitiesOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::Amenity>&)> visitor /*= nullptr*/, IQueryController* controller /*= nullptr*/ )
{
    ObfPoiSectionReader_P::loadAmenities(reader->_d, section, zoom, zoomDepth, bbox31, desiredCategories, amenitiesOut, visitor, controller);
}
