#include "Address.h"

OsmAnd::Address::Address(const std::shared_ptr<const ObfAddressSectionInfo>& obfSection_, const AddressType addressType_)
    : obfSection(obfSection_)
    , addressType(addressType_)
{
}

OsmAnd::Address::~Address()
{
}

QString OsmAnd::Address::toString() const
{
    return ADDRESS_TYPE_NAMES[addressType];
}
