#include "ObfStreetGroup.h"

OsmAnd::ObfStreetGroup::ObfStreetGroup(
        std::shared_ptr<const OsmAnd::ObfAddressSectionInfo> obfSection,
        OsmAnd::StreetGroup streetGroup,
        OsmAnd::ObfObjectId id,
        uint32_t dataOffset)
    : ObfAddress(AddressType::StreetGroup, id)
    , _obfSection(obfSection)
    , _streetGroup(streetGroup)
    , _dataOffset(dataOffset)
{

}

uint32_t OsmAnd::ObfStreetGroup::dataOffset() const
{
    return _dataOffset;
}

OsmAnd::StreetGroup OsmAnd::ObfStreetGroup::streetGroup() const
{
    return _streetGroup;
}

std::shared_ptr<const OsmAnd::ObfAddressSectionInfo> OsmAnd::ObfStreetGroup::obfSection() const
{
    return _obfSection;
}
