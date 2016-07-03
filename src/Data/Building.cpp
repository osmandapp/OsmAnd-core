#include "Building.h"

#include "Street.h"
#include "StreetGroup.h"

OsmAnd::Building::Building(const std::shared_ptr<const Street>& street_)
    : Address(street_ -> obfSection, AddressType::Building)
    , street(street_)
    , streetGroup(street->streetGroup)
    , id(ObfObjectId::invalidId())
    , interpolation(Interpolation::Disabled)
{
}

OsmAnd::Building::Building(const std::shared_ptr<const StreetGroup>& streetGroup_)
    : Address(streetGroup_ -> obfSection, AddressType::Building)
    , street(nullptr)
    , streetGroup(streetGroup_)
    , id(ObfObjectId::invalidId())
    , interpolation(Interpolation::Disabled)
{
}

OsmAnd::Building::~Building()
{
}

QString OsmAnd::Building::toString() const
{
    return nativeName;
}
