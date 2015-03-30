#include "StreetGroup.h"

OsmAnd::StreetGroup::StreetGroup(const std::shared_ptr<const ObfAddressSectionInfo>& obfSection_)
    : Address(obfSection_, AddressType::StreetGroup)
    , id(ObfObjectId::invalidId())
    , dataOffset(0)
{
}

OsmAnd::StreetGroup::~StreetGroup()
{
}
