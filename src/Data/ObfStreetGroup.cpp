#include "ObfStreetGroup.h"

OsmAnd::ObfStreetGroup::ObfStreetGroup(
        std::shared_ptr<const OsmAnd::ObfAddressSectionInfo> obfSection_,
        OsmAnd::StreetGroup streetGroup_,
        OsmAnd::ObfObjectId id_,
        uint32_t dataOffset_)
    : obfSection(obfSection_)
    , streetGroup(streetGroup_)
    , id(id_)
    , dataOffset(dataOffset_)
{

}
