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
    QList< Fwd<Model::AmenityCategory>::RefC >& categories)
{
    ObfPoiSectionReader_P::loadCategories(reader->_d, section, categories);
}

void OsmAnd::ObfPoiSectionReader::loadAmenities(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section,
    const ZoomLevel zoom, uint32_t zoomDepth /*= 3*/, const AreaI* bbox31 /*= nullptr*/, QSet<uint32_t>* desiredCategories /*= nullptr*/,
    QList< Fwd<Model::Amenity>::RefC >* amenitiesOut /*= nullptr*/,
    std::function<bool (Fwd<Model::Amenity>::NCRefC)> visitor /*= nullptr*/, const IQueryController* const controller /*= nullptr*/ )
{
    ObfPoiSectionReader_P::loadAmenities(reader->_d, section, zoom, zoomDepth, bbox31, desiredCategories, amenitiesOut, visitor, controller);
}
