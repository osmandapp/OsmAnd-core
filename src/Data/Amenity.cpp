#include "Amenity.h"

OsmAnd::Amenity::Amenity(const std::shared_ptr<const ObfPoiSectionInfo>& obfSection_)
    : obfSection(obfSection_)
    , zoomLevel(InvalidZoomLevel)
    , id(ObfObjectId::invalidId())
{
}

OsmAnd::Amenity::~Amenity()
{
}
