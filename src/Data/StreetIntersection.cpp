#include "StreetIntersection.h"
#include "ObfStreet.h"

OsmAnd::StreetIntersection::StreetIntersection(const std::shared_ptr<const ObfStreet>& street_)
    : Address(street_->obfSection, AddressType::StreetIntersection)
    , street(street_)
{
}

OsmAnd::StreetIntersection::~StreetIntersection()
{
}
