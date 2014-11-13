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
    QList< std::shared_ptr<const AmenityCategory> >& categories)
{
    ObfPoiSectionReader_P::loadCategories(*reader->_p, section, categories);
}

void OsmAnd::ObfPoiSectionReader::loadAmenities(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section,
    const ZoomLevel zoom, uint32_t zoomDepth /*= 3*/, const AreaI* bbox31 /*= nullptr*/, QSet<uint32_t>* desiredCategories /*= nullptr*/,
    QList< std::shared_ptr<const Amenity> >* amenitiesOut /*= nullptr*/,
    std::function<bool(std::shared_ptr<const Amenity>)> visitor /*= nullptr*/, const IQueryController* const controller /*= nullptr*/)
{
    ObfPoiSectionReader_P::loadAmenities(*reader->_p, section, zoom, zoomDepth, bbox31, desiredCategories, amenitiesOut, visitor, controller);
}
