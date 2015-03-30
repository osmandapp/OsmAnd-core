#include "Building.h"

#include "Street.h"
#include "StreetGroup.h"

OsmAnd::Building::Building(const std::shared_ptr<const Street>& street_)
    : street(street_)
    , streetGroup(street->streetGroup)
    , id(ObfObjectId::invalidId())
    , interpolation(Interpolation::Disabled)
{
}

OsmAnd::Building::Building(const std::shared_ptr<const StreetGroup>& streetGroup_)
    : street(nullptr)
    , streetGroup(streetGroup_)
    , id(ObfObjectId::invalidId())
    , interpolation(Interpolation::Disabled)
{
}

OsmAnd::Building::~Building()
{
}
