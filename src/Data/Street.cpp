#include "Street.h"

#include "StreetGroup.h"

OsmAnd::Street::Street(const std::shared_ptr<const StreetGroup>& streetGroup_)
    : Address(streetGroup_->obfSection, AddressType::Street)
    , streetGroup(streetGroup_)
    , id(ObfObjectId::invalidId())
    , offset(0)
    , firstBuildingInnerOffset(0)
    , firstIntersectionInnerOffset(0)
{
}

OsmAnd::Street::~Street()
{
}
